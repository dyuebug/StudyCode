/*
 * main.c —— 参数解析与进程入口
 *
 * 默认值针对压力用例：4 线程、空闲 10 s、1024 连接上限。
 * 端口默认 0（内核分配），实际端口打进 SRV|listen，测试脚本从日志取值，
 * 这样用例可以并发、可以重跑而不会撞 EADDRINUSE。
 */
#include "reactor.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(const char *prog)
{
    fprintf(stderr,
            "用法: %s [--port N] [--threads N] [--idle-ms N] [--max-conns N]\n"
            "  --port       监听端口，0 = 内核分配（默认 0）\n"
            "  --threads    worker 线程数（默认 4）\n"
            "  --idle-ms    连接空闲超时毫秒，0 = 不超时（默认 10000）\n"
            "  --max-conns  同时在册连接上限（默认与 MAX_CONNS 取小）\n",
            prog);
}

int main(int argc, char **argv)
{
    struct srv_opts opts = { 0, 10000, 4, MAX_CONNS };

    static const struct option longopts[] = {
        { "port",      required_argument, NULL, 'p' },
        { "threads",   required_argument, NULL, 't' },
        { "idle-ms",   required_argument, NULL, 'i' },
        { "max-conns", required_argument, NULL, 'c' },
        { "help",      no_argument,       NULL, 'h' },
        { NULL, 0, NULL, 0 },
    };

    int ch;
    while ((ch = getopt_long(argc, argv, "p:t:i:c:h", longopts, NULL)) != -1) {
        switch (ch) {
        case 'p': opts.port = (uint16_t)atoi(optarg); break;
        case 't': opts.threads = atoi(optarg); break;
        case 'i': opts.idle_ms = atoi(optarg); break;
        case 'c': opts.max_conns = atoi(optarg); break;
        case 'h': usage(argv[0]); return 0;
        default:  usage(argv[0]); return 2;
        }
    }
    if (opts.threads < 1) {
        opts.threads = 1;
    }
    if (opts.max_conns < 1 || opts.max_conns > MAX_CONNS) {
        opts.max_conns = MAX_CONNS;
    }
    return srv_run(&opts);
}
