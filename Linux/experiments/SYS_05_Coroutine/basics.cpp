/* SYS_05 · 协程基础：Generator / Task / 生命周期 / 就绪队列
 *
 * 每个用例单独一次进程调用：`./coro_basics <case>`。
 * 输出统一为 `S05|basics|k=v|k=v`，由 judge.py 按用例分组断言。
 *
 * 用例：
 *   gen            co_yield：惰性、单次遍历、提前 break 时帧被析构
 *   task           co_await 链：一次 resume 里靠对称转移跑完三层
 *   except         子协程抛异常 → unhandled_exception 存 exception_ptr → await_resume 重抛
 *   life           final_suspend=suspend_always：跑完仍占帧，destroy 才释放
 *   resume-twice   破坏版：done() 之后 resume 是 UB，正确驱动必须拦住（静态防护）
 *   leak           100 个帧：正确销毁 → 存活归零
 *   leak-broken    100 个帧：不销毁 → 存活停在 100（计数就是判据）
 *   sched-order    就绪队列优先级：高优先级先跑，同一轮每个 fd 至多一次
 */
#include "coro.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <vector>
#include <unistd.h>

static void pr(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
}

/* ---------- gen ---------- */
static coro::Generator<int> squares(int n)
{
    for (int i = 0; i < n; i++)
        co_yield i * i;
}

static coro::Generator<int> naturals()
{
    for (int i = 0;; i++)
        co_yield i;
}

static int case_gen(void)
{
    int sum = 0, cnt = 0;
    long alive_after_iter = 0, alive_after_break = 0;

    {
        auto g = squares(5);

        while (g.next()) {
            sum += g.value();
            cnt++;
        }
        alive_after_iter = coro::g_alive; /* 生成器仍持有帧 */
    }
    alive_after_break = coro::g_alive; /* 作用域结束 → 析构 → 归零 */

    int first = -1;

    {
        auto g = naturals();

        if (g.next())
            first = g.value();
        /* 提前 break：帧由 Generator 析构销毁，不会泄漏 */
    }
    pr("S05|basics|sum=%d|cnt=%d|first=%d|alive_after_iter=%ld|alive_after_break=%ld|created=%ld\n",
       sum, cnt, first, alive_after_iter, alive_after_break, coro::g_created);
    return 0;
}

/* ---------- task ---------- */
static coro::Task<int> leaf(void)
{
    co_return 7;
}

static coro::Task<int> mid(void)
{
    int v = co_await leaf();

    co_return v * 6;
}

static coro::Task<int> outer(void)
{
    int v = co_await mid();

    co_return v;
}

static int case_task(void)
{
    auto t = outer();
    auto h = t.detach();
    int resumes = 0;

    /* 一次 resume 就该跑完三层：co_await 用对称转移跳进子协程，
     * 子协程 final_suspend 再转移回调用者，期间不回到调度器。 */
    if (!h.done()) {
        h.resume();
        resumes++;
    }

    int v = h.promise().value;
    int d = h.done() ? 1 : 0;
    long alive_before_destroy = coro::g_alive;

    h.destroy();
    pr("S05|basics|value=%d|resumes=%d|done=%d|alive_before_destroy=%ld|alive_after=%ld\n",
       v, resumes, d, alive_before_destroy, coro::g_alive);
    return 0;
}

/* ---------- except ---------- */
static char g_caught[64];

static coro::Task<int> boom(void)
{
    throw std::runtime_error("boom");
    co_return 0; /* 不可达，但编译器要求有 co_return */
}

static coro::Task<int> catcher(void)
{
    int got = 0;

    try {
        got = co_await boom();
    } catch (const std::exception &e) {
        std::snprintf(g_caught, sizeof g_caught, "%s", e.what());
    }
    co_return got + 1;
}

static int case_except(void)
{
    auto t = catcher();
    auto h = t.detach();

    if (!h.done())
        h.resume();

    int v = h.promise().value;

    h.destroy();
    pr("S05|basics|caught=%s|outer_value=%d|alive_after=%ld\n", g_caught, v, coro::g_alive);
    return 0;
}

/* ---------- life ---------- */
static coro::Task<int> trivial(void)
{
    co_return 5;
}

static int case_life(void)
{
    long a0 = coro::g_alive;
    auto t = trivial();
    auto h = t.detach();
    long a1 = coro::g_alive; /* 帧已建，但惰性 initial_suspend 还没跑 */
    int done_before = h.done() ? 1 : 0;

    h.resume();
    long a2 = coro::g_alive; /* 跑完仍停在 final_suspend：帧还在 */
    int done_after = h.done() ? 1 : 0;
    int v = h.promise().value;

    h.destroy();
    pr("S05|basics|alive_before=%ld|alive_created=%ld|done_before=%d|alive_after_run=%ld|"
       "done_after=%d|value=%d|alive_after_destroy=%ld\n",
       a0, a1, done_before, a2, done_after, v, coro::g_alive);
    return 0;
}

/* ---------- resume-twice（破坏版，静态防护） ---------- */
static int case_resume_twice(void)
{
    auto t = trivial();
    auto h = t.detach();

    h.resume();
    /* 此处若直接 h.resume()：协程已停在 final_suspend，resume 它是 UB；
     * 若 final_suspend 是 suspend_never，帧已释放，resume 更是野指针。
     * 所以"正确驱动"必须先用 done() 拦住。这里记录拦截次数，不真去踩。 */
    int blocked = 0;
    int would = 1;

    if (h.done())
        blocked++;
    h.destroy();
    pr("S05|basics|would_resume_after_done=%d|blocked=%d|actual_resume_after_done=0|"
       "alive_after=%ld\n",
       would, blocked, coro::g_alive);
    return 0;
}

/* ---------- leak / leak-broken ---------- */
using task_handle = coro::Task<int>::handle_type;

static int case_leak(int do_destroy)
{
    std::vector<task_handle> hs;

    hs.reserve(100);
    for (int i = 0; i < 100; i++) {
        auto t = trivial();
        auto h = t.detach();

        h.resume(); /* 跑完停在 final_suspend */
        hs.push_back(h);
    }

    long alive_before = coro::g_alive;

    if (do_destroy) {
        for (auto h : hs)
            h.destroy();
    }
    long alive_after = coro::g_alive;

    /* 破坏版为了让后续用例能继续跑，在记录读数之后才清理 */
    if (!do_destroy) {
        for (auto h : hs)
            h.destroy();
    }
    pr("S05|basics|frames=%d|do_destroy=%d|alive_before_cleanup=%ld|alive_after=%ld|"
       "alive_final=%ld\n",
       (int)hs.size(), do_destroy, alive_before, alive_after, coro::g_alive);
    return 0;
}

/* ---------- sched-order ---------- */
struct order_ctx {
    int ord[8];
    int n;
    int resumed;
};

static coro::Task<int> waiter(coro::Sched *s, int fd, int id, int high, order_ctx *ctx)
{
    int r = co_await s->wait(fd, EPOLLIN, high != 0);

    if (r == 0) {
        ctx->ord[ctx->n++] = id;
        ctx->resumed++;
    }
    co_return r;
}

static int case_sched_order(void)
{
    coro::Sched s;
    int pipes[5][2];
    std::vector<coro::Task<int>::handle_type> hs;
    order_ctx ctx;

    memset(&ctx, 0, sizeof ctx);
    for (int i = 0; i < 5; i++) {
        if (pipe(pipes[i]) < 0) {
            pr("S05|basics|error=pipe\n");
            return 1;
        }
    }

    s.on_done = [&s](int fd, std::coroutine_handle<> h) {
        s.detach(fd);
        h.destroy();
    };

    /* 0、1 是高优先级；2、3、4 普通 */
    for (int i = 0; i < 5; i++) {
        auto t = waiter(&s, pipes[i][0], i, i < 2 ? 1 : 0, &ctx);
        auto h = t.detach();

        hs.push_back(h);
        s.resume_later(pipes[i][0], h);
    }
    /* 全部可读，让同一轮里五种优先级混在一起 */
    for (int i = 0; i < 5; i++) {
        ssize_t w = write(pipes[i][1], "x", 1);

        if (w != 1) {
            pr("S05|basics|error=write\n");
            return 1;
        }
    }

    int iters = s.run(50);

    /* 高优先级必须整体排在普通之前：前两个位置的两个 id 应是 {0,1} */
    int hi_first = (ctx.n >= 2 && ctx.ord[0] < 2 && ctx.ord[1] < 2) ? 1 : 0;
    int lo_after = 1;

    for (int i = 2; i < ctx.n; i++)
        if (ctx.ord[i] < 2)
            lo_after = 0;

    pr("S05|basics|resumed=%d|order=%d%d%d%d%d|hi_first=%d|lo_after=%d|iters=%d|alive=%ld\n",
       ctx.resumed, ctx.ord[0], ctx.ord[1], ctx.ord[2], ctx.ord[3], ctx.ord[4], hi_first,
       lo_after, iters, coro::g_alive);
    for (int i = 0; i < 5; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *name = argc > 1 ? argv[1] : "";

    printf("S05|case|name=%s\n", name);
    if (strcmp(name, "gen") == 0)
        return case_gen();
    if (strcmp(name, "task") == 0)
        return case_task();
    if (strcmp(name, "except") == 0)
        return case_except();
    if (strcmp(name, "life") == 0)
        return case_life();
    if (strcmp(name, "resume-twice") == 0)
        return case_resume_twice();
    if (strcmp(name, "leak") == 0)
        return case_leak(1);
    if (strcmp(name, "leak-broken") == 0)
        return case_leak(0);
    if (strcmp(name, "sched-order") == 0)
        return case_sched_order();
    printf("S05|basics|error=unknown_case\n");
    return 2;
}
