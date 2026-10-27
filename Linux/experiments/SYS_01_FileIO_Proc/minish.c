/* SYS_01 阶段 1 验收项：一个简化 Shell
 *
 * 支持：内建 cd / pwd / exit、单个重定向（< > >>）、两级管道（a | b）、
 * fork + execvp + waitpid、前台进程组的 SIGINT 处理。
 *
 * 刻意不做的：命令历史、作业控制、通配符、引号转义、多级管道、&& / ||。
 * 每一条"不做"都在 README 的「已知限制」里，避免把简化实现当成 bash。
 *
 * 取证行走 stderr（前缀 SYS01|sh|），命令本身的输出去 stdout，
 * 这样判定器可以只 grep 取证行，不会把 `pwd` 打印的 "/" 误当成证据。
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_TOKENS 64
#define LINE_SIZE 1024

typedef struct {
    char *argv[MAX_TOKENS];
    int argc;
    char *in_path;
    char *out_path;
    int append;
} cmd_t;

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
        (void)!write(STDERR_FILENO, buf, (size_t)n < sizeof buf ? (size_t)n : sizeof buf - 1);
}

static void die(const char *msg)
{
    ev("SYS01|sh|fatal|%s|errno=%d\n", msg, errno);
    _exit(1);
}

/* 逐字节读一行；返回读取的字节数，0 表示 EOF。
 * 不用 fgets 是为了在非交互（脚本）模式下拿到确定的 EOF 语义。 */
static ssize_t read_line(int fd, char *buf, size_t cap)
{
    size_t i = 0;

    while (i + 1 < cap) {
        char c;
        ssize_t n = read(fd, &c, 1);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0)
            break;
        buf[i++] = c;
        if (c == '\n')
            break;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

/* 手写分词：`|` `<` `>` 单独成 token，`>>` 识别为两字符 token。
 * 用 strtok 不行——它会把 `a|b` 当成一个词，边界信息就丢了。
 *
 * 运算符 token 存进调用方给的池子而不是指向原行：原行里它们只占一个字符，
 * 没法就地写成带 '\0' 的独立字符串；用 static 变量又会被同一行里
 * 第二个运算符覆盖（`a < b > c` 就会踩）。 */
static char op_pool[MAX_TOKENS][4];

static int tokenize(char *line, char *tokens[], int cap)
{
    int n = 0;
    int ops = 0;
    char *p = line;

    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\n')
            p++;
        if (!*p)
            break;
        if (n >= cap || ops >= MAX_TOKENS)
            return -1;
        if (*p == '|' || *p == '<' || *p == '>') {
            char *slot = op_pool[ops++];
            int len = 1;

            if (*p == '>' && p[1] == '>')
                len = 2;
            memcpy(slot, p, (size_t)len);
            slot[len] = '\0';
            tokens[n++] = slot;
            p += len;
            continue;
        }
        tokens[n++] = p;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n')
            p++;
        if (*p)
            *p++ = '\0';
    }
    return n;
}

static void run_builtin(char *const argv[], int argc, int *should_exit, int *code)
{
    if (strcmp(argv[0], "cd") == 0) {
        const char *target = argc > 1 ? argv[1] : getenv("HOME");

        if (!target)
            target = "/";
        if (chdir(target) < 0)
            ev("SYS01|sh|builtin|cd|fail|errno=%d\n", errno);
        else
            ev("SYS01|sh|builtin|cd|path=%s\n", target);
        *code = 0;
    } else if (strcmp(argv[0], "pwd") == 0) {
        char cwd[512];

        if (getcwd(cwd, sizeof cwd))
            ev("SYS01|sh|builtin|pwd|path=%s\n", cwd);
        else
            ev("SYS01|sh|builtin|pwd|fail|errno=%d\n", errno);
        *code = 0;
    } else if (strcmp(argv[0], "exit") == 0) {
        *should_exit = 1;
        *code = argc > 1 ? atoi(argv[1]) : 0;
        ev("SYS01|sh|builtin|exit|code=%d\n", *code);
    }
}

static void apply_redirections(const cmd_t *c)
{
    if (c->in_path) {
        int fd = open(c->in_path, O_RDONLY);

        if (fd < 0)
            die("redirect_in_open");
        if (dup2(fd, STDIN_FILENO) < 0)
            die("redirect_in_dup2");
        close(fd);
    }
    if (c->out_path) {
        int flags = O_WRONLY | O_CREAT | (c->append ? O_APPEND : O_TRUNC);
        int fd = open(c->out_path, flags, 0644);

        if (fd < 0)
            die("redirect_out_open");
        if (dup2(fd, STDOUT_FILENO) < 0)
            die("redirect_out_dup2");
        close(fd);
    }
}

static int wait_child(pid_t pid)
{
    int st = 0;

    while (waitpid(pid, &st, 0) < 0) {
        if (errno != EINTR)
            return -1;
    }
    return WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st);
}

static int run_single(cmd_t *c)
{
    pid_t pid = fork();
    int status;

    if (pid < 0) {
        ev("SYS01|sh|fork_fail|errno=%d\n", errno);
        return -1;
    }
    if (pid == 0) {
        signal(SIGINT, SIG_DFL); /* 子进程要能被 Ctrl-C 打断，shell 不能被 */
        apply_redirections(c);
        execvp(c->argv[0], c->argv);
        ev("SYS01|sh|exec_fail|cmd=%s|errno=%d\n", c->argv[0], errno);
        _exit(127);
    }
    status = wait_child(pid);
    ev("SYS01|sh|exec|cmd=%s|pid=%d|status=%d\n", c->argv[0], (int)pid, status);
    return status;
}

static int run_pipe(cmd_t *a, cmd_t *b)
{
    int fds[2];
    pid_t left, right;
    int st_left, st_right;

    if (pipe(fds) < 0) {
        ev("SYS01|sh|pipe_fail|errno=%d\n", errno);
        return -1;
    }

    left = fork();
    if (left == 0) {
        signal(SIGINT, SIG_DFL);
        close(fds[0]);
        if (dup2(fds[1], STDOUT_FILENO) < 0)
            die("pipe_left_dup2");
        close(fds[1]);
        apply_redirections(a);
        execvp(a->argv[0], a->argv);
        ev("SYS01|sh|exec_fail|cmd=%s|errno=%d\n", a->argv[0], errno);
        _exit(127);
    }

    right = fork();
    if (right == 0) {
        signal(SIGINT, SIG_DFL);
        close(fds[1]);
        if (dup2(fds[0], STDIN_FILENO) < 0)
            die("pipe_right_dup2");
        close(fds[0]);
        apply_redirections(b);
        execvp(b->argv[0], b->argv);
        ev("SYS01|sh|exec_fail|cmd=%s|errno=%d\n", b->argv[0], errno);
        _exit(127);
    }

    /* 父进程必须关掉两端：否则右子的 read 永远等不到 EOF。 */
    close(fds[0]);
    close(fds[1]);
    st_left = wait_child(left);
    st_right = wait_child(right);
    ev("SYS01|sh|pipe|left=%s|right=%s|st_left=%d|st_right=%d\n", a->argv[0], b->argv[0],
       st_left, st_right);
    return st_right;
}

int main(void)
{
    char line[LINE_SIZE];
    char *tokens[MAX_TOKENS];
    int last_code = 0;
    long ncmd = 0;

    /* shell 自己忽略 SIGINT：否则 Ctrl-C 会把 shell 一起杀掉，
     * 前台子进程已在 fork 后恢复成 SIG_DFL。 */
    {
        struct sigaction sa;

        memset(&sa, 0, sizeof sa);
        sa.sa_handler = SIG_IGN;
        sigaction(SIGINT, &sa, NULL);
    }

    ev("SYS01|sh|start|pid=%d\n", (int)getpid());

    for (;;) {
        ssize_t n = read_line(STDIN_FILENO, line, sizeof line);

        if (n <= 0)
            break;

        int nt = tokenize(line, tokens, MAX_TOKENS);

        if (nt <= 0)
            continue;

        /* 按 `|` 切成两段；本文只支持一个管道（README 已声明） */
        int bar = -1;

        for (int i = 0; i < nt; i++)
            if (strcmp(tokens[i], "|") == 0) {
                bar = i;
                break;
            }

        cmd_t a, b;

        memset(&a, 0, sizeof a);
        memset(&b, 0, sizeof b);

        /* 解析单段的 argv 与重定向 */
        for (int i = 0; i < nt; i++) {
            if (bar >= 0 && i == bar)
                continue;
            cmd_t *cur = (bar >= 0 && i > bar) ? &b : &a;

            if (strcmp(tokens[i], "<") == 0 && i + 1 < nt) {
                cur->in_path = tokens[++i];
            } else if ((strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0) &&
                       i + 1 < nt) {
                cur->append = (tokens[i][1] == '>');
                cur->out_path = tokens[++i];
            } else if (cur->argc < MAX_TOKENS - 1) {
                cur->argv[cur->argc++] = tokens[i];
            }
        }

        if (a.argc == 0) {
            ev("SYS01|sh|empty_cmd\n");
            continue;
        }

        /* 内建命令必须在父进程执行：cd 若在子进程里跑，父进程的工作目录不会变 */
        int should_exit = 0, code = last_code;

        if (bar < 0 && (strcmp(a.argv[0], "cd") == 0 || strcmp(a.argv[0], "pwd") == 0 ||
                        strcmp(a.argv[0], "exit") == 0)) {
            run_builtin(a.argv, a.argc, &should_exit, &code);
            last_code = code;
            ncmd++;
            if (should_exit)
                break;
            continue;
        }

        if (bar >= 0) {
            if (a.argc == 0 || b.argc == 0) {
                ev("SYS01|sh|pipe_syntax\n");
                continue;
            }
            last_code = run_pipe(&a, &b);
        } else {
            last_code = run_single(&a);
        }
        ncmd++;
    }

    ev("SYS01|sh|exit|code=%d|cmds=%ld\n", last_code, ncmd);
    return last_code;
}
