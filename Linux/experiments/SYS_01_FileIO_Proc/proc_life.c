/* SYS_01 阶段 1：进程生命周期取证
 *
 * 三组，每组都能被一条读数否证：
 *   A  僵尸进程：子进程先退出、父进程还没 wait，/proc/<pid>/stat 的状态应是 Z；
 *      wait 之后同一个路径必须消失（否则是"回收了但目录项还在"，性质不同）。
 *   B  孤儿与 subreaper：中间层进程退出后，孙进程的父进程会变。
 *      不设 subreaper 与设了 subreaper 是唯一的变量，判据只看
 *      "孙的 ppid 是否等于本进程 pid"，不去假设具体变成 1。
 *   C  wait 状态的两种形态：正常退出与信号终止必须被区分开——
 *      把 WIFSIGNALED 的子进程当成"退出码 0"是最常见的误判。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void ev(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void ev(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > 0)
        (void)!write(STDOUT_FILENO, buf, (size_t)n < sizeof buf ? (size_t)n : sizeof buf - 1);
}

/* /proc/<pid>/stat 的第三个字段是状态字符。
 * comm 是被圆括号包起来的，且允许包含空格与括号，所以只能从**最后一个** ')' 往后取。 */
static int proc_state(pid_t pid, char *out)
{
    char path[64];
    char buf[1024];
    int fd;
    ssize_t r;
    char *rp;

    snprintf(path, sizeof path, "/proc/%d/stat", (int)pid);
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;
    r = read(fd, buf, sizeof buf - 1);
    close(fd);
    if (r <= 0)
        return -1;
    buf[r] = '\0';
    rp = strrchr(buf, ')');
    if (!rp || rp[1] != ' ')
        return -1;
    *out = rp[2];
    return 0;
}

static void exp_zombie(void)
{
    pid_t pid = fork();
    char st = '?';

    if (pid < 0) {
        ev("SYS01|zombie|fork_fail|errno=%d\n", errno);
        return;
    }
    if (pid == 0)
        _exit(0); /* 子进程立刻退出，父进程故意不立即 wait */

    /* 父进程必须先让子进程跑完；子进程状态到达 Z 需要它真的执行过 exit。 */
    {
        int tries = 0;
        int rc;

        do {
            usleep(20000);
            rc = proc_state(pid, &st);
            tries++;
        } while ((rc < 0 || st != 'Z') && tries < 50);
    }
    ev("SYS01|zombie|before_wait|pid=%d|state=%c\n", (int)pid, st);

    {
        int status = 0;

        if (waitpid(pid, &status, 0) < 0)
            ev("SYS01|zombie|wait_fail|errno=%d\n", errno);
        else
            ev("SYS01|zombie|reaped|exited=%d|code=%d\n", WIFEXITED(status),
               WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    }

    /* 回收之后再读同一个路径：应当打不开。 */
    {
        char after = '?';

        if (proc_state(pid, &after) < 0)
            ev("SYS01|zombie|after_wait|gone\n");
        else
            ev("SYS01|zombie|after_wait|still_there|state=%c\n", after);
    }
}

/* 中间层进程 fork 出孙进程后立刻退出，孙进程报自己的 ppid。
 * subreaper != 0 时本进程先 prctl(PR_SET_CHILD_SUBREAPER)。
 * 两种情况的差别只有这一个系统调用。 */
static void exp_orphan(int subreaper)
{
    /* fork 会复制地址空间，所以孙进程能直接读到本进程（祖父）的 pid。
     * 不能写成 getppid()——孙进程的 ppid 正是被观测对象，用它当基准就自证了。 */
    const pid_t me = getpid();
    pid_t mid;

    if (subreaper && prctl(PR_SET_CHILD_SUBREAPER, 1L, 0L, 0L, 0L) < 0) {
        ev("SYS01|orphan|subreaper_fail|errno=%d\n", errno);
        return;
    }

    mid = fork();
    if (mid < 0) {
        ev("SYS01|orphan|fork_fail|errno=%d\n", errno);
        return;
    }
    if (mid == 0) {
        pid_t grand = fork();

        if (grand == 0) {
            /* 孙：等中间层真的退出后，再看自己的父进程是谁 */
            usleep(200000);
            ev("SYS01|orphan|grand|subreaper=%d|grand_pid=%d|ppid=%d|grandparent_pid=%d\n",
               subreaper, (int)getpid(), (int)getppid(), (int)me);
            _exit(0);
        }
        _exit(0); /* 中间层立即退出，制造孤儿 */
    }

    {
        int st = 0;

        if (waitpid(mid, &st, 0) < 0)
            ev("SYS01|orphan|wait_mid_fail|errno=%d\n", errno);
    }

    /* subreaper 模式下，孤儿的父进程被重新挂到本进程，所以仍能 wait 到它。
     * 不设 subreaper 时本进程 wait 不到，只能等它被 init 收养后自行退出。 */
    if (subreaper) {
        int st = 0;
        pid_t w;

        do {
            w = waitpid(-1, &st, 0);
        } while (w < 0 && errno == EINTR);

        if (w > 0)
            ev("SYS01|orphan|subreaper_reaped|child=%d|ok\n", (int)w);
        else
            ev("SYS01|orphan|subreaper_wait_fail|errno=%d\n", errno);
    } else {
        usleep(400000); /* 让孙进程把 ppid 打印完 */
        ev("SYS01|orphan|no_subreaper|not_reaped_here\n");
    }
}

/* 同一个父进程下两种子进程退出方式，验证 wait 状态必须区分。 */
static void exp_wait_status(void)
{
    pid_t p1 = fork();

    if (p1 == 0)
        _exit(42);
    if (p1 > 0) {
        int st = 0;

        if (waitpid(p1, &st, 0) > 0)
            ev("SYS01|wait|normal_exit|exited=%d|code=%d\n", WIFEXITED(st),
               WIFEXITED(st) ? WEXITSTATUS(st) : -1);
    }

    {
        pid_t p2 = fork();

        if (p2 == 0) {
            /* 子进程自杀：父进程拿到的不是"退出码 143"，而是"被信号 15 终止" */
            kill(getpid(), SIGTERM);
            _exit(0); /* 信号被忽略时才会到这里 */
        }
        if (p2 > 0) {
            int st = 0;

            if (waitpid(p2, &st, 0) > 0)
                ev("SYS01|wait|signal_exit|signaled=%d|sig=%d\n", WIFSIGNALED(st),
                   WIFSIGNALED(st) ? WTERMSIG(st) : -1);
        }
    }
}

int main(void)
{
    ev("SYS01|proc|start|pid=%d|ppid=%d\n", (int)getpid(), (int)getppid());
    exp_zombie();
    exp_orphan(0);
    exp_orphan(1);
    exp_wait_status();
    ev("SYS01|proc|done\n");
    return 0;
}
