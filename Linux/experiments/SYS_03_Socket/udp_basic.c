/* SYS_03 阶段 3 · B 组：UDP 是数据报
 *
 * 三个用例，各自对应一条与 TCP 相反的事实：
 *   boundary 每次 sendto 对应一次 recvfrom，绝不合并（对比 tcp_basic 的 merged）
 *   trunc    接收缓冲不够时数据报被**静默截断**；MSG_TRUNC 才能拿到真实长度
 *   toobig   超过 65507 字节的载荷直接 EMSGSIZE，没有"分片发送"这回事
 *
 * 与 IPC 组同一个结论：SOCK_DGRAM 没有 EOF，结束条件必须由应用层给出。
 * 这里不需要 EOF，因为每条消息自成一个单元，收几条就是几条。
 */
#define _GNU_SOURCE
#include "net_common.h"

#include <stdlib.h>

#define MSG_LEN 1000
#define BIG_LEN 2000
#define HUGE_LEN 70000

static int udp_server(int *port_out)
{
    struct sockaddr_in addr;
    socklen_t alen = sizeof addr;
    struct timeval tv = { .tv_sec = 0, .tv_usec = 500 * 1000 };
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0)
        return -1;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0)
        goto fail;
    if (getsockname(fd, (struct sockaddr *)&addr, &alen) < 0)
        goto fail;
    /* 收不到就一直等会挂住用例：给接收端一个上界 */
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0)
        goto fail;
    *port_out = ntohs(addr.sin_port);
    return fd;

fail:
    close(fd);
    return -1;
}

static int udp_client(int port)
{
    struct sockaddr_in addr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0)
        return -1;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short)port);
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static void case_boundary(void)
{
    char out[MSG_LEN];
    char in[BIG_LEN + 64];
    int port = 0;
    int sfd = udp_server(&port);
    int cfd = udp_client(port);
    int recv_msgs = 0;
    int lens[3] = { 0, 0, 0 };

    if (sfd < 0 || cfd < 0) {
        pr("SYS03|udp|boundary|setup_failed|errno=%d\n", errno);
        if (sfd >= 0)
            close(sfd);
        if (cfd >= 0)
            close(cfd);
        return;
    }
    memset(out, 'U', sizeof out);

    /* 三条独立的数据报，每次 1000 字节 */
    for (int i = 0; i < 3; i++)
        if (send(cfd, out, sizeof out, 0) < 0)
            pr("SYS03|udp|boundary|send_failed|errno=%d\n", errno);

    for (int i = 0; i < 3; i++) {
        ssize_t n = recv(sfd, in, sizeof in, 0);

        if (n < 0)
            break;
        lens[recv_msgs++] = (int)n;
    }

    pr("SYS03|udp|boundary|sent_msgs=3|lens=%d,%d,%d|recv_msgs=%d|recv_lens=%d,%d,%d|"
       "preserved=%d\n",
       MSG_LEN, MSG_LEN, MSG_LEN, recv_msgs, lens[0], lens[1], lens[2],
       (recv_msgs == 3 && lens[0] == MSG_LEN && lens[1] == MSG_LEN && lens[2] == MSG_LEN) ? 1 : 0);

    close(cfd);
    close(sfd);
}

static void case_trunc(void)
{
    char *big;
    char small[MSG_LEN];
    int port = 0;
    int sfd = udp_server(&port);
    int cfd = udp_client(port);
    ssize_t plain_rc = -1, trunc_rc = -1;

    big = malloc(BIG_LEN);
    if (!big)
        return;
    if (sfd < 0 || cfd < 0) {
        pr("SYS03|udp|trunc|setup_failed|errno=%d\n", errno);
        goto out;
    }
    memset(big, 'T', BIG_LEN);

    /* 第一次：普通 recv，缓冲只有 1000，2000 字节的包被静默截断 */
    if (send(cfd, big, BIG_LEN, 0) < 0)
        pr("SYS03|udp|trunc|send_failed|errno=%d\n", errno);
    plain_rc = recv(sfd, small, sizeof small, 0);

    /* 第二次：带 MSG_TRUNC 问内核"这个包本来多大" */
    if (send(cfd, big, BIG_LEN, 0) < 0)
        pr("SYS03|udp|trunc|send_failed|errno=%d\n", errno);
    trunc_rc = recv(sfd, small, sizeof small, MSG_TRUNC);

    pr("SYS03|udp|trunc|buf=%d|recv_rc=%d|real_len=%d|truncated=%d\n", MSG_LEN, (int)plain_rc,
       (int)trunc_rc, (plain_rc == MSG_LEN && trunc_rc == BIG_LEN) ? 1 : 0);

out:
    if (cfd >= 0)
        close(cfd);
    if (sfd >= 0)
        close(sfd);
    free(big);
}

static void case_toobig(void)
{
    char *huge;
    int port = 0;
    int sfd = udp_server(&port);
    int cfd = udp_client(port);
    int err = 0;
    ssize_t n;

    huge = malloc(HUGE_LEN);
    if (!huge)
        return;
    if (sfd < 0 || cfd < 0) {
        pr("SYS03|udp|toobig|setup_failed|errno=%d\n", errno);
        goto out;
    }
    memset(huge, 'H', HUGE_LEN);

    n = send(cfd, huge, HUGE_LEN, 0);
    if (n < 0)
        err = errno;

    pr("SYS03|udp|toobig|payload=%d|send_rc=%d|errno=%d|detected=%d\n", HUGE_LEN, (int)n, err,
       (n < 0 && err == EMSGSIZE) ? 1 : 0);

out:
    if (cfd >= 0)
        close(cfd);
    if (sfd >= 0)
        close(sfd);
    free(huge);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "boundary";

    if (strcmp(mode, "boundary") == 0)
        case_boundary();
    else if (strcmp(mode, "trunc") == 0)
        case_trunc();
    else if (strcmp(mode, "toobig") == 0)
        case_toobig();
    else {
        pr("用法: udp_basic <boundary|trunc|toobig>\n");
        return 2;
    }
    pr("SYS03|udp|done|mode=%s\n", mode);
    return 0;
}
