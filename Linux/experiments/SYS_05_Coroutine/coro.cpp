/* SYS_05 · 调度器实现
 *
 * 单线程、epoll + EPOLLONESHOT。协程的"等待 fd"由 Waiter 完成：
 *   co_await s.wait(fd, EPOLLIN)  →  槽位登记 handle，挂起
 *   事件到达                      →  槽位结果置 0，进入就绪队列
 *   下一轮 run_once 按优先级 resume 一次
 *
 * 为什么用 EPOLLONESHOT：一次 resume 之后，协程可能立刻又挂起在同一个 fd 上；
 * ONESHOT 让"每次挂起都必须重挂"成为硬性要求，从而在协程层面复现阶段 4 的结论
 * （掩码没变也必须 MOD）。
 */
#include "coro.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <sys/epoll.h>
#include <unistd.h>

namespace coro {

long g_alive = 0;
long g_created = 0;

FrameGuard::FrameGuard()
{
    ++g_alive;
    ++g_created;
}

FrameGuard::~FrameGuard()
{
    --g_alive;
}

long long now_ms()
{
    using namespace std::chrono;

    return (long long)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

/* ---------- Sched ---------- */
Sched::Sched()
{
    long lim = sysconf(_SC_OPEN_MAX);

    if (lim <= 0 || lim > 65536)
        lim = 65536;
    maxfd_ = (int)lim;
    slots_.resize((size_t)maxfd_);
    deadline_.assign((size_t)maxfd_, 0);
    epfd_ = epoll_create1(EPOLL_CLOEXEC);
}

Sched::~Sched()
{
    if (epfd_ >= 0)
        close(epfd_);
}

bool Sched::ensure(int fd)
{
    return fd >= 0 && fd < maxfd_;
}

int Sched::arm(int fd, std::coroutine_handle<> h, uint32_t events, int high)
{
    struct epoll_event e;
    int op = slots_[(size_t)fd].alive ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

    memset(&e, 0, sizeof e);
    e.events = events | EPOLLONESHOT;
    e.data.fd = fd;
    if (epoll_ctl(epfd_, op, fd, &e) < 0) {
        /* 槽位与内核状态可能短暂不一致（比如 detach 后又 arm）：两种都试一次 */
        int op2 = (op == EPOLL_CTL_ADD) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

        if (epoll_ctl(epfd_, op2, fd, &e) < 0)
            return -errno;
    }
    if (!slots_[(size_t)fd].alive)
        active_.push_back(fd);
    slots_[(size_t)fd].h = h;
    slots_[(size_t)fd].alive = 1;
    slots_[(size_t)fd].destroyed = 0;
    slots_[(size_t)fd].high = high;
    slots_[(size_t)fd].result = 0;
    /* 挂起即活动：每次 arm 都顺延空闲期限。没挂起就没 deadline，
     * 所以正在跑的协程永远不会被自己超时掉。 */
    if (idle_ms_ > 0)
        deadline_[(size_t)fd] = now_ms() + idle_ms_;
    return 0;
}

bool Sched::Waiter::await_suspend(std::coroutine_handle<> h)
{
    int r;

    if (!s->ensure(fd)) {
        result = -EBADF;
        return false; /* 不挂起：await_resume 直接拿到错误 */
    }
    r = s->arm(fd, h, events, high);
    if (r < 0) {
        result = r;
        return false;
    }
    return true;
}

int Sched::Waiter::await_resume() const
{
    if (result != 0)
        return result;
    return s->slots_[(size_t)fd].result;
}

void Sched::detach(int fd)
{
    if (!ensure(fd))
        return;
    if (slots_[(size_t)fd].alive) {
        epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
        slots_[(size_t)fd].h = {};
        slots_[(size_t)fd].alive = 0;
        slots_[(size_t)fd].destroyed = 0;
        slots_[(size_t)fd].result = 0;
        slots_[(size_t)fd].queued = 0;
        deadline_[(size_t)fd] = 0;
        for (size_t i = 0; i < active_.size(); i++) {
            if (active_[i] == fd) {
                active_.erase(active_.begin() + (long)i);
                break;
            }
        }
    }
}

void Sched::note_destroyed(int fd)
{
    if (ensure(fd))
        slots_[(size_t)fd].destroyed = 1;
}

void Sched::make_ready(int fd, int result)
{
    if (!ensure(fd))
        return;
    slots_[(size_t)fd].result = result;
    if (slots_[(size_t)fd].queued)
        return; /* 本轮已经排过一次：事件与超时撞车时以先到者为准 */
    slots_[(size_t)fd].queued = 1;
    if (slots_[(size_t)fd].high)
        ready_hi_.push_back(fd);
    else
        ready_lo_.push_back(fd);
}

void Sched::resume_later(int fd, std::coroutine_handle<> h)
{
    if (!ensure(fd))
        return;
    if (!slots_[(size_t)fd].alive)
        active_.push_back(fd);
    slots_[(size_t)fd].h = h;
    slots_[(size_t)fd].alive = 1;
    slots_[(size_t)fd].destroyed = 0;
    slots_[(size_t)fd].result = 0;
    make_ready(fd, 0);
}

void Sched::cancel(int fd, int err)
{
    if (!ensure(fd) || !slots_[(size_t)fd].alive)
        return;
    deadline_[(size_t)fd] = 0;
    make_ready(fd, -err);
}

int Sched::alive_slots() const
{
    return (int)active_.size();
}

int Sched::resume_one(int fd)
{
    Slot &s = slots_[(size_t)fd];
    std::coroutine_handle<> h;

    s.queued = 0; /* 已从就绪队列取出 */
    if (!s.alive)
        return 0;
    if (s.destroyed) {
        /* 破坏版路径：帧已销毁但槽位还在。这里**不去 resume**，只计数——
         * 真去 resume 是 UB，用它当"证据"是不可复现的。 */
        ++use_after_destroy_;
        return 0;
    }
    if (!s.h)
        return 0;
    h = s.h;
    s.h = {}; /* 防重入：resume 里若再次挂起，会由 arm 写回 */
    h.resume();
    if (h.done() && on_done)
        on_done(fd, h); /* 谁持有帧谁 destroy；这里只是把事件交出去 */
    return 1;
}

int Sched::run_once(int timeout_ms)
{
    struct epoll_event evs[64];
    int n = epoll_wait(epfd_, evs, 64, timeout_ms);
    int resumed = 0;
    long long t;

    if (n < 0 && errno != EINTR)
        return -1;

    for (int i = 0; i < n; i++) {
        int fd = evs[i].data.fd;

        if (!ensure(fd))
            continue;
        if (slots_[(size_t)fd].destroyed) {
            ++use_after_destroy_;
            continue;
        }
        if (!slots_[(size_t)fd].alive)
            continue;
        make_ready(fd, 0);
    }

    /* 超时：只看存活槽位，按 fd 顺序，保证同一轮内不会因遍历顺序产生差异 */
    t = now_ms();
    for (size_t i = 0; i < active_.size(); i++) {
        int fd = active_[i];

        if (deadline_[(size_t)fd] && t >= deadline_[(size_t)fd]) {
            deadline_[(size_t)fd] = 0;
            cancel(fd, ETIMEDOUT);
        }
    }

    /* 每轮每个 fd 至多 resume 一次：先高优先级队列，再普通队列。
     * 快照长度，防止"一直有高优先级就绪"把低优先级饿死。 */
    {
        size_t hi_n = ready_hi_.size();

        for (size_t k = 0; k < hi_n && !ready_hi_.empty(); k++) {
            int fd = ready_hi_.front();

            ready_hi_.pop_front();
            resumed += resume_one(fd);
        }
    }
    {
        size_t lo_n = ready_lo_.size();

        for (size_t k = 0; k < lo_n && !ready_lo_.empty(); k++) {
            int fd = ready_lo_.front();

            ready_lo_.pop_front();
            resumed += resume_one(fd);
        }
    }
    return resumed;
}

int Sched::run(int max_iters)
{
    int iters = 0;

    while (alive_slots() > 0 && iters < max_iters) {
        if (run_once(20) < 0)
            break;
        ++iters;
    }
    return iters;
}

} /* namespace coro */
