// ============================================
// 09_thread_pool.cpp
// 日期：2026-06-09（第66天）
// 主题：线程池 - 任务队列 + 工作线程组
// 难度：⭐⭐⭐ 高级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 09_pool 09_thread_pool.cpp -lpthread
// 运行：./09_pool
// ============================================

/*
【核心概念】简要说明

是什么？
- 线程池（Thread Pool）：预先创建一组工作线程，反复复用它们执行任务
- 任务放进队列，空闲的工作线程自动取出执行

为什么需要？
- 频繁创建/销毁线程开销大（创建线程要分配栈、内核数据结构）
- 线程数失控：来一个任务建一个线程，10万个任务 = 10万个线程 → 系统崩溃
- 线程池：固定数量线程复用，任务排队，平滑处理大量短任务

核心特点？
- 工作线程：循环"取任务-执行-再取"，没任务就睡（条件变量）
- 任务队列：用锁保护，生产者（提交任务）和消费者（工作线程）共享
- 优雅关闭：通知所有线程退出，等它们处理完手头任务

【本质】线程池 = 生产者/消费者模型 + 固定数量的消费者线程
       （正好用上昨天... 不，上一个文件学的条件变量！）

【与你学过的知识对照】
  Qt 的 QThreadPool / QtConcurrent  → 就是封装好的线程池
  这个文件 = 手写一个 QThreadPool 的简化版
*/

#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include <functional>
#include <vector>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showConcepts() {
    cout << R"(
============================================================
线程池 - 核心概念
============================================================

【线程池工作原理图】

   提交任务                    任务队列
  ┌────────┐   push    ┌──────────────────┐
  │ 主线程 │ ────────► │ T1 T2 T3 T4 ...   │
  └────────┘           └──────────────────┘
                            │  │  │  pop（抢任务）
                  ┌─────────┘  │  └─────────┐
                  ▼            ▼            ▼
              ┌───────┐   ┌───────┐   ┌───────┐
              │工作线程│   │工作线程│   │工作线程│
              │  #1   │   │  #2   │   │  #3   │
              └───────┘   └───────┘   └───────┘
              循环：取任务→执行→再取（没任务就睡）

【三个核心组件】

  1. 任务队列（queue + mutex）
     - 存放待执行的任务（这里用 function<void()> 表示任务）
     - 用互斥锁保护（多线程同时读写）

  2. 工作线程组（固定 N 个线程）
     - 每个线程循环：拿锁→等任务→取任务→放锁→执行
     - 队列空就用条件变量睡眠，不浪费 CPU

  3. 关闭机制（stop 标志 + broadcast）
     - 设 stop=true，broadcast 唤醒所有线程
     - 线程检查到 stop 且队列空，就退出

【工作线程的核心循环（伪代码）】

  while (true) {
      lock(mutex);
      while (任务队列空 && !stop)
          cond_wait(cond, mutex);   // 没任务就睡
      if (stop && 队列空)
          { unlock; break; }        // 该退出了
      取出一个任务;
      unlock(mutex);
      执行任务();                    // 注意：执行时不持锁！
  }

  关键：执行任务时必须先解锁，否则其他线程没法取任务（退化成串行）
============================================================
)" << endl;
}

// ============================================
// 第二部分：基础操作实践 - 手写线程池类
// ============================================

// 任务类型：用 std::function 表示一个"无参数无返回值"的可调用对象
using Task = function<void()>;

class ThreadPool {
private:
    vector<pthread_t> workers;   // 工作线程组
    queue<Task> tasks;           // 任务队列
    pthread_mutex_t mutex;       // 保护任务队列的锁
    pthread_cond_t cond;         // "有任务"的条件变量
    bool stop;                   // 关闭标志

    // 工作线程入口函数（静态，因为 pthread 要求 void*(void*) 签名）
    // 通过 arg 把 this 指针传进来
    static void* workerLoop(void* arg) {
        ThreadPool* pool = (ThreadPool*)arg;  // 拿回 this
        while (true) {
            Task task;
            // ---- 临界区：从队列取任务 ----
            pthread_mutex_lock(&pool->mutex);

            // 队列空且没关闭 → 睡眠等待（while 防虚假唤醒）
            while (pool->tasks.empty() && !pool->stop) {
                pthread_cond_wait(&pool->cond, &pool->mutex);
            }

            // 已关闭且队列已清空 → 退出
            if (pool->stop && pool->tasks.empty()) {
                pthread_mutex_unlock(&pool->mutex);
                break;
            }

            task = pool->tasks.front();  // 取出一个任务
            pool->tasks.pop();
            pthread_mutex_unlock(&pool->mutex);
            // ---- 临界区结束：解锁后再执行任务 ----

            task();  // 执行任务（此时不持锁，其他线程可继续取任务）
        }
        return nullptr;
    }

public:
    // 构造：创建 threadCount 个工作线程
    ThreadPool(int threadCount) : stop(false) {
        pthread_mutex_init(&mutex, nullptr);
        pthread_cond_init(&cond, nullptr);

        workers.resize(threadCount);
        for (int i = 0; i < threadCount; i++) {
            // 把 this 作为参数传给每个工作线程
            pthread_create(&workers[i], nullptr, workerLoop, this);
        }
        cout << "  [线程池] 创建 " << threadCount << " 个工作线程" << endl;
    }

    // 提交任务：把任务放入队列并通知一个工作线程
    void submit(Task task) {
        pthread_mutex_lock(&mutex);
        tasks.push(task);              // 先放任务
        pthread_cond_signal(&cond);    // 再通知一个空闲线程
        pthread_mutex_unlock(&mutex);
    }

    // 关闭：通知所有线程退出，并等待它们结束
    void shutdown() {
        pthread_mutex_lock(&mutex);
        stop = true;                       // 设置关闭标志
        pthread_cond_broadcast(&cond);     // 唤醒所有线程去检查
        pthread_mutex_unlock(&mutex);

        // 等所有工作线程处理完剩余任务并退出
        for (pthread_t& t : workers) {
            pthread_join(t, nullptr);
        }
        cout << "  [线程池] 所有工作线程已退出" << endl;
    }

    // 析构：清理资源
    ~ThreadPool() {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }
};

// 全局锁，仅用于让演示输出不交错（方便观察）
pthread_mutex_t g_printMutex = PTHREAD_MUTEX_INITIALIZER;

void demo_thread_pool() {
    cout << "\n第二部分：线程池实战" << endl;
    cout << string(60, '-') << endl;

    ThreadPool pool(3);  // 3个工作线程

    // 提交 8 个任务，由 3 个线程并发处理
    for (int i = 1; i <= 8; i++) {
        // 用 lambda 捕获任务编号 i（按值捕获，避免悬空）
        pool.submit([i]() {
            // 加锁打印，避免多线程输出交错
            pthread_mutex_lock(&g_printMutex);
            cout << "    [任务 " << i << "] 开始执行（线程 "
                 << pthread_self() << "）" << endl;
            pthread_mutex_unlock(&g_printMutex);

            usleep(200000);  // 模拟耗时 0.2 秒

            pthread_mutex_lock(&g_printMutex);
            cout << "    [任务 " << i << "] 完成" << endl;
            pthread_mutex_unlock(&g_printMutex);
        });
    }

    sleep(1);          // 等任务大致跑完（演示用）
    pool.shutdown();   // 优雅关闭：处理完剩余任务再退出
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
==============================================
🔍 深入理解1：为什么 workerLoop 必须是 static？（选学）
==============================================

pthread_create 要求线程函数签名是 void*(void*)。
但 C++ 类的普通成员函数有一个隐藏的 this 参数，
实际签名是 void*(ThreadPool*, void*)，和要求不匹配！

解决：把函数声明为 static（静态成员函数没有 this），
     然后手动把 this 通过 arg 参数传进去：
       pthread_create(&t, NULL, workerLoop, this);
     在函数内部再转回来：
       ThreadPool* pool = (ThreadPool*)arg;

（C++11 之后也可以用 std::thread + 成员函数，更优雅，
  但这里用 pthread 是为了理解底层原理。）

==============================================
🔍 深入理解2：为什么执行任务时要先解锁？（选学）⭐⭐⭐⭐⭐
==============================================

工作线程循环里，取出任务后立刻 unlock，然后才执行 task()：

  task = tasks.front(); tasks.pop();
  pthread_mutex_unlock(&mutex);  // ← 先解锁
  task();                        // ← 再执行

如果反过来（持锁执行任务）：
  task();                        // 持锁执行（耗时0.2秒）
  pthread_mutex_unlock(&mutex);

后果：执行任务期间锁一直被占着，其他工作线程全都卡在
     lock 处取不到任务 → 3个线程退化成1个线程串行执行！
     线程池完全失去并发意义。

口诀：锁只保护"取任务"这个瞬间，执行任务时绝不持锁。

==============================================
🔍 深入理解3：shutdown 的两种语义（选学）
==============================================

本例的 shutdown 是"优雅关闭"：
  - 设 stop=true，但工作线程会把队列里剩余任务处理完才退出
  - 条件：if (stop && 队列空) 才 break

另一种是"立即关闭"：
  - 设 stop=true，丢弃队列里未执行的任务，线程立刻退出
  - 条件：if (stop) 就 break（不管队列是否空）

实际线程池库（如 Qt）通常两种都提供，按需选择。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

【错误1：工作线程函数用普通成员函数】
❌ pthread_create(&t, NULL, &ThreadPool::workerLoop, this);
   报错：成员函数签名不匹配（有隐藏 this）
✅ 把 workerLoop 声明为 static，手动传 this 当参数
预防：pthread 回调用 static 函数 + this 传参

【错误2：持锁执行任务（退化成串行）】
❌ lock; task(); unlock;     → 执行期间别人取不到任务
✅ lock; 取任务; unlock; task();  → 执行时不持锁
预防：锁的粒度尽量小，只锁"取任务"，不锁"执行任务"

【错误3：lambda 按引用捕获循环变量】
❌ pool.submit([&i](){ use(i); });  // i 是循环变量
   问题：任务真正执行时，i 可能已变化或失效（悬空引用）
✅ pool.submit([i](){ use(i); });   // 按值捕获，拷贝当前值
预防：提交任务时，捕获的临时变量优先按值捕获

【错误4：关闭时不 broadcast，线程永远睡着】
❌ stop = true;  // 但工作线程还在 cond_wait 睡觉，没人叫醒
✅ stop = true; pthread_cond_broadcast(&cond);  // 叫醒所有
预防：改 stop 后必须 broadcast，让睡着的线程醒来检查

【错误5：shutdown 后还提交任务】
❌ pool.shutdown(); pool.submit(task);  // 线程都退出了，任务没人执行
✅ 提交任务必须在 shutdown 之前完成
预防：明确生命周期，关闭后不再提交

【错误6：忘记 join，主线程提前退出】
❌ shutdown 里只设 stop 不 join，main 直接结束
   问题：工作线程可能还没处理完就被强杀
✅ shutdown 里 broadcast 后逐个 pthread_join
预防：关闭流程 = 设标志 + 唤醒 + join 等待

【错误7：队列空判断用 if 不用 while】
❌ if (tasks.empty()) cond_wait(...);   // 虚假唤醒会崩
✅ while (tasks.empty() && !stop) cond_wait(...);
预防：条件等待永远用 while（和上一个文件同样的坑）
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showQuickReference() {
    cout << R"(
============================================================
线程池设计速查表
============================================================

【三大组件】
  任务队列   queue<function<void()>> + mutex
  工作线程   vector<pthread_t>，每个跑 workerLoop
  关闭机制   bool stop + cond_broadcast + join

【工作线程循环骨架】
  while (true) {
      lock;
      while (队列空 && !stop) cond_wait;
      if (stop && 队列空) { unlock; break; }
      取任务; unlock;     // 先解锁
      执行任务();          // 再执行（不持锁）
  }

【提交任务 submit】
  lock; 队列.push(任务); signal; unlock;

【优雅关闭 shutdown】
  lock; stop=true; broadcast; unlock;
  for each 线程: join;

【关键设计原则】
  - 执行任务时不持锁（保证并发）
  - 改 stop 后必须 broadcast（唤醒睡眠线程）
  - 静态函数 + this 传参（pthread 回调约束）
  - 提交任务的 lambda 优先按值捕获
)" << endl;
}

// ============================================
// 第六部分：练习题（不提供答案）
// ============================================

void showExercises() {
    cout << R"(
============================================================
练习题（独立思考，不看答案）
============================================================

1. 【基础】统计任务执行
   要求：向线程池提交 20 个任务，每个任务把全局计数器+1，
        所有任务完成后打印计数器（应为20）
   提示：计数器要加锁保护，或用条件变量等所有任务完成

2. 【带返回值】并行平方和
   要求：提交任务计算 1²,2²,...,10²，把结果累加到共享变量
   提示：共享结果变量加锁，submit 时用 lambda 捕获编号

3. 【负载观察】线程数对比
   要求：分别用 1、2、4 个工作线程处理 8 个各耗时0.5秒的任务，
        测量总耗时，验证"线程越多越快"（直到达到任务数）
   提示：用 time ./09_pool 或在代码里记录起止时间

4. 【立即关闭】实现 shutdownNow
   要求：增加一个 shutdownNow()，丢弃队列里未执行的任务立即退出
   提示：把退出条件从 (stop && 空) 改成 (stop)，对比优雅关闭

5. 【综合】支持等待全部完成
   要求：增加 waitAll() 方法，阻塞直到队列清空且所有任务执行完
   提示：增加"活跃任务计数" + 一个"全部完成"条件变量
)" << endl;
}

int main() {
    cout << string(60, '=') << endl;
    cout << "线程池 - 任务队列 + 工作线程组" << endl;
    cout << string(60, '=') << endl;

    showConcepts();
    demo_thread_pool();

    showCommonMistakes();
    showQuickReference();
    showExercises();

    cout << "\n程序结束。线程池 = 任务队列 + 固定工作线程 + 优雅关闭。" << endl;
    return 0;
}
