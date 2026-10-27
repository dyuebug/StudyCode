/* SYS_05 · 协程基础设施（C++20 coroutine）
 *
 * 这一层只放四样东西：
 *   1. FrameGuard —— 存活帧计数（promise 构造 +1、析构 -1），让"泄漏"成为可读数的判据
 *   2. Generator<T> —— co_yield，惰性、单次遍历
 *   3. Task<T>     —— co_await 链，对称转移（symmetric transfer），异常经 exception_ptr 传播
 *   4. Sched       —— 单线程 epoll 调度器：把"等待 fd 可读/可写"变成 co_await
 *
 * 所有权约定（贯穿整个实验）：
 *   - 协程帧的生命周期只由**返回对象的析构**或**显式的 destroy()** 决定；
 *   - Task 的 final_suspend 是 suspend_always：协程跑完仍停在帧里，直到持有者 destroy；
 *   - 谁 resume 谁负责在 done() 之后 destroy，且必须从调度器 detach 之后再 destroy。
 */
#ifndef CORO_H
#define CORO_H

#include <coroutine>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <vector>

namespace coro {

/* ---------- 1. 存活帧计数 ---------- */
extern long g_alive;
extern long g_created;

struct FrameGuard {
    FrameGuard();
    ~FrameGuard();
    FrameGuard(const FrameGuard &) = delete;
    FrameGuard &operator=(const FrameGuard &) = delete;
};

/* ---------- 2. Generator ---------- */
template <class T>
class Generator {
public:
    struct promise_type : FrameGuard {
        T cur{};

        Generator get_return_object()
        {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        /* 惰性：创建时不执行，第一次 resume 才进函数体 */
        std::suspend_always initial_suspend() noexcept { return {}; }
        /* 跑完仍停在帧里，等持有者 destroy（否则 yield 的值读不到） */
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(T v) noexcept
        {
            cur = v;
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() { std::terminate(); }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    explicit Generator(handle_type h) : h_(h) {}
    Generator(Generator &&o) noexcept : h_(o.h_) { o.h_ = {}; }
    Generator &operator=(Generator &&) = delete;
    Generator(const Generator &) = delete;
    ~Generator()
    {
        if (h_)
            h_.destroy();
    }

    /* 推进到下一个 yield；返回 false 表示已结束 */
    bool next()
    {
        if (!h_ || h_.done())
            return false;
        h_.resume();
        return !h_.done();
    }
    T value() const { return h_.promise().cur; }
    bool done() const { return !h_ || h_.done(); }

private:
    handle_type h_;
};

/* ---------- 3. Task ---------- */
/* 用 Task<int> 表示"有返回值"；无返回值的场景统一 co_return 0，避免再写特化。
 * 这不是漂亮的设计，但让实验里只需要读一套代码。 */
template <class T>
class Task {
public:
    struct promise_type : FrameGuard {
        T value{};
        std::exception_ptr err;
        std::coroutine_handle<> cont{};
        /* 破坏版用：记录"done 之后又被 resume"的设计级违规，不真去踩 UB */
        int resumed_after_done = 0;

        Task get_return_object()
        {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        /* 惰性：co_await 时才被对称转移进来 */
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }
            /* 对称转移回调用者；没有调用者时交给 noop，控制权回到 resume() 的发起处 */
            std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept
            {
                auto c = h.promise().cont;
                return c ? c : std::noop_coroutine();
            }
            void await_resume() noexcept {}
        };
        FinalAwaiter final_suspend() noexcept { return {}; }

        void return_value(T v) { value = v; }
        void unhandled_exception() { err = std::current_exception(); }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    explicit Task(handle_type h) : h_(h) {}
    Task(Task &&o) noexcept : h_(o.h_) { o.h_ = {}; }
    Task &operator=(Task &&) = delete;
    Task(const Task &) = delete;
    ~Task()
    {
        if (h_)
            h_.destroy();
    }

    struct Awaiter {
        handle_type h;

        bool await_ready() const noexcept { return !h || h.done(); }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept
        {
            h.promise().cont = caller;
            return h; /* 对称转移：直接进子协程，不回到调度器 */
        }
        T await_resume()
        {
            if (h.promise().err)
                std::rethrow_exception(h.promise().err);
            return h.promise().value;
        }
    };

    Awaiter operator co_await() noexcept { return Awaiter{h_}; }

    handle_type handle() const { return h_; }
    bool done() const { return !h_ || h_.done(); }
    /* 交出所有权：调用方此后负责 destroy */
    handle_type detach()
    {
        auto r = h_;
        h_ = {};
        return r;
    }

private:
    handle_type h_;
};

/* ---------- 4. 调度器 ---------- */
class Sched {
public:
    /* 一个 fd 的等待槽。alive=0 表示已 detach；destroyed=1 只有破坏版会置，
     * 让"销毁后仍被事件唤醒"变成可读的 use_after_destroy 计数，而不是真解引用野指针。 */
    struct Slot {
        std::coroutine_handle<> h{};
        int result = 0;
        int alive = 0;
        int destroyed = 0;
        int high = 0;
        /* 本轮已进就绪队列：事件与超时在同一轮撞上时只 resume 一次，
         * 否则同一个挂起点会被唤醒两次（第二次读到的 result 是过期的）。 */
        int queued = 0;
    };

    /* co_await s.wait(fd, EPOLLIN) → 返回 0（就绪）或负 errno（取消/超时） */
    struct Waiter {
        Sched *s;
        int fd;
        uint32_t events;
        int high;
        int result = 0; /* await_suspend 失败时用它；否则读槽位里的结果 */

        bool await_ready() const noexcept { return false; }
        bool await_suspend(std::coroutine_handle<> h);
        int await_resume() const;
    };

    /* co_await s.yield_here(fd) → 挂起后立刻重新排队，下一轮再被 resume。
     * 用于公平性让出：此时数据在自己的缓冲里，等 epoll 事件是等不到的。
     * 必须带 fd：句柄要挂在某个槽位上，否则调度器无法管理它的生命周期。 */
    struct Yield {
        Sched *s;
        int fd;

        bool await_ready() const noexcept { return false; }
        bool await_suspend(std::coroutine_handle<> h)
        {
            s->resume_later(fd, h);
            return true;
        }
        void await_resume() const noexcept {}
    };

    Sched();
    ~Sched();
    Sched(const Sched &) = delete;
    Sched &operator=(const Sched &) = delete;

    Waiter wait(int fd, uint32_t events, bool high = false) { return Waiter{this, fd, events, high ? 1 : 0}; }
    Yield yield_here(int fd) { return Yield{this, fd}; }

    /* 把 fd 的交还调度器管理（关掉 epoll 关注、清槽位） */
    void detach(int fd);

    /* 取消：下一次调度把 -err 交给等待中的协程 */
    void cancel(int fd, int err);

    /* 销毁之后未 detach：破坏版用，事件到达时记一次 use_after_destroy 而不 resume */
    void note_destroyed(int fd);

    /* 空闲超时：idle_ms>0 时，**每次挂起**都把 deadline 顺延到 now+idle_ms；
     * 到时仍未收到事件就 cancel(fd, ETIMEDOUT)。0 表示不设。
     * 之所以挂在 arm 上而不是单独的 touch()：挂起即活动，不需要调用方记住刷新。 */
    void set_idle_ms(long long v) { idle_ms_ = v; }

    /* 就绪队列：高优先级先跑；同优先级 FIFO */
    void make_ready(int fd, int result);
    /* 登记 handle 并让它进入就绪队列：首次启动与公平性让出共用同一条路径 */
    void resume_later(int fd, std::coroutine_handle<> h);

    /* 跑一轮：epoll_wait + 处理超时 + 按优先级各 resume 一次。返回本轮 resume 次数 */
    int run_once(int timeout_ms);
    /* 跑到没有存活槽位（或迭代上限） */
    int run(int max_iters = 100000);

    int epfd() const { return epfd_; }
    long use_after_destroy() const { return use_after_destroy_; }
    int alive_slots() const;
    void set_broken_detach(bool v) { broken_detach_ = v; }

    /* 协程跑完时由调度器回调：谁持有帧，谁在这里 destroy。
     * 放在调度器里而不是协程内部，是为了让"所有权在持有者手上"这条约定可执行。 */
    std::function<void(int fd, std::coroutine_handle<>)> on_done;

private:
    bool ensure(int fd);
    int arm(int fd, std::coroutine_handle<> h, uint32_t events, int high);
    int resume_one(int fd);

    int epfd_ = -1;
    int maxfd_ = 0;
    long long idle_ms_ = 0;
    std::vector<Slot> slots_;
    std::vector<long long> deadline_;
    std::vector<int> active_; /* 有存活槽位的 fd，避免每轮扫描整张 65536 表 */
    std::deque<int> ready_hi_;
    std::deque<int> ready_lo_;
    long use_after_destroy_ = 0;
    bool broken_detach_ = false;
};

long long now_ms();

} /* namespace coro */

#endif /* CORO_H */
