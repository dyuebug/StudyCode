// ============================================
// 07_pthread_basics.cpp
// 日期：2026-06-09（第66天）
// 主题：Linux 多线程基础 - pthread 创建/同步
// 难度：⭐⭐ 中级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 07_pthread 07_pthread_basics.cpp -lpthread
//       ⚠️ 必须加 -lpthread 链接线程库！
// 运行：./07_pthread
// ============================================

/*
【核心概念】简要说明

是什么？
- 线程（Thread）：进程内的执行单元，多个线程共享同一进程的内存空间
- pthread：POSIX Threads，Linux/Unix 标准线程库（C 语言接口）

为什么需要？
- 并发执行：多个任务同时推进（比如下载的同时更新进度条）
- 共享数据方便：线程间共享内存，通信比进程简单
- 开销小：创建线程比创建进程轻量得多

核心特点？
- 共享：同一进程的线程共享全局变量、堆内存、文件描述符
- 独立：每个线程有自己的栈、寄存器、程序计数器
- 危险：共享数据如果不加保护，会产生"数据竞争"（race condition）

【与你学过的知识对照】
  Qt 的 QThread     → pthread 是它的底层
  Qt 的 QMutex      → pthread_mutex_t
  RTOS 的"任务"     → 类似线程，但 RTOS 任务通常不共享内存
  RTOS 的 Mutex     → 同样是互斥锁，概念一致
*/

#include <iostream>
#include <pthread.h>   // pthread 线程库头文件
#include <unistd.h>    // sleep() 函数
#include <cstring>     // strerror()
#include <vector>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showConcepts() {
    cout << R"(
============================================================
Linux 多线程模型 - 核心概念
============================================================

【进程 vs 线程 - 内存视角】

  进程A                          进程B
  ┌──────────────┐              ┌──────────────┐
  │ 代码段        │              │ 代码段        │
  │ 数据段(全局)  │   隔离       │ 数据段(全局)  │
  │ 堆           │ ◄────────►   │ 堆           │
  │ 栈           │              │ 栈           │
  └──────────────┘              └──────────────┘
  两个进程：完全独立的地址空间

  进程内的多个线程：
  ┌────────────────────────────────────┐
  │ 代码段  ← 所有线程共享                │
  │ 数据段  ← 所有线程共享（全局变量）    │
  │ 堆      ← 所有线程共享（new/malloc）  │
  │ ┌────────┐ ┌────────┐ ┌────────┐    │
  │ │线程1栈 │ │线程2栈 │ │线程3栈 │    │ ← 每个线程独立的栈
  │ └────────┘ └────────┘ └────────┘    │
  └────────────────────────────────────┘

  关键结论：
  - 全局变量、堆内存 → 共享（所以能通信，也所以危险）
  - 局部变量（栈上）  → 独立（每个线程自己的）

【为什么需要互斥锁？】

  场景：两个线程同时给全局变量 counter++ 10000 次
  期望结果：20000
  实际结果：可能是 13476、17829... 每次都不一样！

  原因：counter++ 不是"原子操作"，它分三步：
    1. 读取 counter 到寄存器
    2. 寄存器 +1
    3. 写回 counter

  如果线程A刚读完（步骤1），线程B也来读，
  两个线程基于同一个旧值各加一次，结果只加了1次！
  这就是"数据竞争"（race condition）。

  解决：用互斥锁（mutex）保证"同一时刻只有一个线程能操作"
============================================================
)" << endl;
}

// ============================================
// 第一部分补充：核心函数卡片 ⭐⭐⭐⭐⭐
// ============================================

void showFunctionCards() {
    cout << R"(
============================================================
pthread 核心函数卡片
============================================================

【函数1：pthread_create() - 创建线程】

语法：
  int pthread_create(pthread_t* thread,
                     const pthread_attr_t* attr,
                     void* (*start_routine)(void*),
                     void* arg);

参数详解：
  1. thread（输出参数）
     - 类型：pthread_t*
     - 作用：传入一个 pthread_t 变量的地址，函数会把新线程的ID写进去
     - 示例：pthread_t tid; pthread_create(&tid, ...);

  2. attr（线程属性）
     - 类型：const pthread_attr_t*
     - 作用：设置线程属性（栈大小、分离状态等）
     - 示例：传 NULL 表示使用默认属性（最常用）

  3. start_routine（线程函数）
     - 类型：void* (*)(void*)  ← 函数指针！
     - 作用：线程启动后要执行的函数
     - 要求：函数签名必须是 void* func(void* arg)
     - 示例：&myThreadFunc 或 myThreadFunc

  4. arg（传给线程函数的参数）
     - 类型：void*
     - 作用：把数据传给线程函数（通过 void* 万能指针）
     - 示例：(void*)&data 或 (void*)number

返回值：
  - 成功返回 0
  - 失败返回错误码（注意：不是 -1！是正的错误码）
  - ⚠️ pthread 系列函数都是"返回错误码"，不设置 errno

────────────────────────────────────

【函数2：pthread_join() - 等待线程结束】

语法：
  int pthread_join(pthread_t thread, void** retval);

参数详解：
  1. thread（要等待的线程ID）
     - 类型：pthread_t
     - 作用：指定等待哪个线程
     - 注意：传的是 ID 本身，不是地址

  2. retval（接收线程返回值）
     - 类型：void**（指向 void* 的指针）
     - 作用：接收线程函数 return 的值
     - 示例：传 NULL 表示不关心返回值

返回值：成功返回 0，失败返回错误码

作用：阻塞当前线程，直到目标线程结束（类似 Qt 的 wait()）

────────────────────────────────────

【函数3：pthread_detach() - 分离线程】

语法：
  int pthread_detach(pthread_t thread);

作用：
  - 把线程设为"分离状态"，线程结束后自动回收资源
  - 分离后不能再 join（也不需要 join）
  - 适用于"不关心结果、自生自灭"的后台线程

join vs detach 怎么选？
  - 需要等结果 / 需要同步 → join
  - 后台任务、不关心结束 → detach

────────────────────────────────────

【函数4：pthread_mutex_lock/unlock - 互斥锁】

语法：
  int pthread_mutex_lock(pthread_mutex_t* mutex);    // 加锁
  int pthread_mutex_unlock(pthread_mutex_t* mutex);  // 解锁

初始化方式（两种）：
  方式1（静态）：pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
  方式2（动态）：pthread_mutex_init(&m, NULL);  用完 pthread_mutex_destroy(&m);

作用：
  - lock：申请锁。如果锁被别人占着，就阻塞等待
  - unlock：释放锁，唤醒等待的线程

⚠️ 黄金规则：lock 和 unlock 必须成对出现！
)" << endl;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 示例1：最简单的线程 - 创建并等待
// --------------------------------------------

// 线程函数：签名必须是 void* func(void* arg)
// arg：调用方通过 pthread_create 的第4个参数传进来
void* helloThread(void* arg) {
    // 把 void* 参数转回真实类型（这里约定传的是 int*）
    int id = *(int*)arg;
    cout << "  [线程 " << id << "] 你好，我在独立的线程里运行！" << endl;
    return nullptr;  // 线程结束，返回值可被 pthread_join 接收
}

void demo1_basic() {
    cout << "\n2.1 示例1：创建单个线程" << endl;
    cout << string(60, '-') << endl;

    pthread_t tid;        // 存放线程ID的变量
    int id = 1;           // 要传给线程的数据

    // 创建线程：
    //   &tid        → 接收新线程ID
    //   NULL        → 默认属性
    //   helloThread → 线程要执行的函数
    //   &id         → 传给线程函数的参数（注意取地址）
    int ret = pthread_create(&tid, nullptr, helloThread, &id);
    if (ret != 0) {
        // pthread 返回错误码，用 strerror 转成可读文本
        cerr << "创建线程失败: " << strerror(ret) << endl;
        return;
    }

    // 等待线程结束（否则 main 可能先退出，线程还没跑完）
    pthread_join(tid, nullptr);
    cout << "  [主线程] 子线程已结束" << endl;
}

// --------------------------------------------
// 示例2：创建多个线程
// --------------------------------------------

void* worker(void* arg) {
    int id = *(int*)arg;
    cout << "  [工人 " << id << "] 开始干活..." << endl;
    sleep(1);  // 模拟耗时工作（睡眠1秒）
    cout << "  [工人 " << id << "] 干完了！" << endl;
    return nullptr;
}

void demo2_multiple() {
    cout << "\n2.2 示例2：创建多个线程并发执行" << endl;
    cout << string(60, '-') << endl;

    const int N = 3;
    pthread_t tids[N];     // 线程ID数组
    int ids[N];            // 每个线程的参数

    // ⚠️ 关键：每个线程的参数要用独立的变量！
    // 如果都传同一个变量的地址，多个线程会读到同一个（可能已被改写的）值
    for (int i = 0; i < N; i++) {
        ids[i] = i + 1;
        pthread_create(&tids[i], nullptr, worker, &ids[i]);
    }

    // 依次等待所有线程结束
    for (int i = 0; i < N; i++) {
        pthread_join(tids[i], nullptr);
    }
    cout << "  [主线程] 所有工人都完工了" << endl;
}

// --------------------------------------------
// 示例3：获取线程返回值
// --------------------------------------------

// 这个线程计算 1+2+...+n，并返回结果
void* sumThread(void* arg) {
    int n = *(int*)arg;
    // 在堆上分配结果（不能返回栈上局部变量的地址！）
    long* result = new long(0);
    for (int i = 1; i <= n; i++) {
        *result += i;
    }
    return (void*)result;  // 返回堆内存指针
}

void demo3_return_value() {
    cout << "\n2.3 示例3：获取线程返回值" << endl;
    cout << string(60, '-') << endl;

    pthread_t tid;
    int n = 100;
    pthread_create(&tid, nullptr, sumThread, &n);

    void* retval;  // 用来接收返回值
    // 第二个参数传 &retval（void**），线程的返回值会写进 retval
    pthread_join(tid, &retval);

    long* sum = (long*)retval;  // 转回真实类型
    cout << "  1+2+...+" << n << " = " << *sum << endl;
    delete sum;  // ⚠️ 别忘了释放线程里 new 的内存
}

// --------------------------------------------
// 示例4：互斥锁保护共享数据
// --------------------------------------------

long g_counter = 0;                                   // 全局共享变量
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;  // 静态初始化的互斥锁

// 不加锁的版本（会出错）
void* unsafeAdd(void* arg) {
    for (int i = 0; i < 100000; i++) {
        g_counter++;  // 危险！多个线程同时操作，结果不可预测
    }
    return nullptr;
}

// 加锁的版本（正确）
void* safeAdd(void* arg) {
    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&g_mutex);    // 加锁：进入临界区
        g_counter++;                     // 临界区：只有一个线程能执行
        pthread_mutex_unlock(&g_mutex);  // 解锁：离开临界区
    }
    return nullptr;
}

void demo4_mutex() {
    cout << "\n2.4 示例4：互斥锁对比实验" << endl;
    cout << string(60, '-') << endl;

    const int N = 4;  // 4个线程，每个加10万次，期望 = 400000
    pthread_t tids[N];

    // 实验A：不加锁
    g_counter = 0;
    for (int i = 0; i < N; i++)
        pthread_create(&tids[i], nullptr, unsafeAdd, nullptr);
    for (int i = 0; i < N; i++)
        pthread_join(tids[i], nullptr);
    cout << "  不加锁结果：" << g_counter
         << "（期望 400000，通常会偏小！）" << endl;

    // 实验B：加锁
    g_counter = 0;
    for (int i = 0; i < N; i++)
        pthread_create(&tids[i], nullptr, safeAdd, nullptr);
    for (int i = 0; i < N; i++)
        pthread_join(tids[i], nullptr);
    cout << "  加锁结果：  " << g_counter
         << "（正确，永远是 400000）" << endl;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
==============================================
🔍 深入理解1：为什么 void* 是"万能指针"？（选学）
==============================================

如果觉得难，可以跳过，不影响后续学习。

pthread_create 的参数是 void*，线程函数返回也是 void*，为什么？

因为 pthread 是 C 语言库，C 没有模板、没有泛型。
为了能传"任意类型"的数据，就用 void*（无类型指针）：
  - void* 可以指向任何类型的数据
  - 但不能直接解引用（编译器不知道它指向几个字节）
  - 使用前必须强制转换回真实类型：int* p = (int*)arg;

这就是为什么我们的模式总是：
  传入：pthread_create(..., (void*)&data);
  取出：int x = *(int*)arg;

==============================================
🔍 深入理解2：线程栈是独立的，意味着什么？（选学）
==============================================

每个线程有自己的栈（默认通常 8MB）。这带来两个推论：

1. 局部变量是线程安全的
   void* func(void*) {
       int local = 0;  // 每个线程有自己的 local，互不干扰
   }

2. 不能返回栈上变量的地址
   void* bad(void*) {
       int x = 42;
       return &x;  // ❌ 线程结束后栈被回收，这个地址失效！
   }
   正确做法：用 new 在堆上分配（示例3就是这么做的），
   或者用全局/static 变量。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

【错误1：忘记链接 -lpthread】
❌ 编译命令：g++ -o app 07_pthread_basics.cpp
   报错：undefined reference to `pthread_create'
✅ 正确命令：g++ -o app 07_pthread_basics.cpp -lpthread
预防：pthread 程序编译必须加 -lpthread

────────────────────────────────────

【错误2：循环里所有线程传同一个变量地址】
❌ 错误代码：
   for (int i = 0; i < N; i++)
       pthread_create(&tids[i], NULL, worker, &i);  // 都传 &i！
   问题：所有线程共享同一个 i，且主循环还在改 i，
        线程读到的值不可预测（可能都是 N）
✅ 正确代码：
   int ids[N];
   for (int i = 0; i < N; i++) {
       ids[i] = i;
       pthread_create(&tids[i], NULL, worker, &ids[i]);  // 各自独立
   }
预防：每个线程的参数用独立的存储

────────────────────────────────────

【错误3：返回栈上局部变量的地址】
❌ 错误代码：
   void* func(void*) {
       int result = 42;
       return &result;  // 线程结束栈回收，地址失效
   }
✅ 正确代码：
   void* func(void*) {
       int* result = new int(42);  // 堆上分配
       return result;              // join 后记得 delete
   }
预防：返回值要用堆内存或全局变量，绝不返回栈地址

────────────────────────────────────

【错误4：lock 后忘记 unlock（死锁）】
❌ 错误代码：
   pthread_mutex_lock(&m);
   if (error) return;       // ← 提前返回，没解锁！
   pthread_mutex_unlock(&m);
   问题：锁永远不释放，其他线程全部卡死
✅ 正确代码：保证所有分支都 unlock，
   或用 RAII（C++ 可用 std::lock_guard 自动管理）
预防：lock/unlock 成对，注意所有提前返回的分支

────────────────────────────────────

【错误5：主线程不 join 就退出】
❌ 错误代码：
   pthread_create(&tid, NULL, worker, NULL);
   return 0;  // main 退出，整个进程结束，子线程被强行杀掉
   问题：子线程可能还没执行完
✅ 正确代码：
   pthread_create(&tid, NULL, worker, NULL);
   pthread_join(tid, NULL);  // 等子线程结束
预防：需要线程跑完的，必须 join（或用 detach 让其独立）

────────────────────────────────────

【错误6：pthread 函数用 errno 判断错误】
❌ 错误代码：
   if (pthread_create(...) < 0) { perror("..."); }
   问题：pthread 函数不设置 errno，也不返回 -1
✅ 正确代码：
   int ret = pthread_create(...);
   if (ret != 0) { cerr << strerror(ret); }
预防：pthread 函数返回错误码本身（正数），用 strerror(ret) 解析

────────────────────────────────────

【错误7：在多个线程间共享数据却不加锁】
❌ 错误代码：全局 counter++ 被多线程同时执行
   问题：数据竞争，结果随机偏小
✅ 正确代码：用 pthread_mutex 保护，或用 std::atomic
预防：只要有"写"且被多线程访问的共享数据，就要保护
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showQuickReference() {
    cout << R"(
============================================================
pthread 函数速查表
============================================================

【线程管理】
  pthread_create(&tid, attr, func, arg)  创建线程，返回0成功
  pthread_join(tid, &retval)             等待线程结束，取返回值
  pthread_detach(tid)                    分离线程（自动回收）
  pthread_self()                         获取当前线程ID
  pthread_exit(retval)                   线程主动退出

【互斥锁 mutex】
  pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;  静态初始化
  pthread_mutex_init(&m, NULL)           动态初始化
  pthread_mutex_lock(&m)                 加锁（阻塞）
  pthread_mutex_trylock(&m)              尝试加锁（不阻塞）
  pthread_mutex_unlock(&m)               解锁
  pthread_mutex_destroy(&m)              销毁锁

【线程函数签名（固定格式）】
  void* 函数名(void* arg) {
      类型* p = (类型*)arg;   // 转回真实类型
      // ... 工作 ...
      return 返回值;          // void* 类型
  }

【编译命令】
  g++ -std=c++17 -o app file.cpp -lpthread
                                  ↑ 必须！
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

1. 【基础】创建 5 个线程
   要求：每个线程打印自己的编号（1~5）和当前线程ID
   提示：用 pthread_self() 获取线程ID，参数用独立数组传递

2. 【返回值】并行求和
   要求：把数组 [1..100] 分成两半，用两个线程分别求和，
        主线程收集两个返回值再相加得到总和
   提示：每个线程返回堆上的 long*，join 后相加并 delete

3. 【互斥锁】线程安全的计数器
   要求：10 个线程，每个把全局账户余额增加 1000 元，
        最终余额应该正好是 10000，先不加锁观察错误，再加锁修正
   提示：对比示例4，体会加锁前后的差异

4. 【detach】后台日志线程
   要求：创建一个分离线程，每秒打印一次"心跳"，共打印5次，
        主线程不 join，而是 sleep(6) 后退出
   提示：用 pthread_detach，思考为什么不能 join

5. 【综合】死锁复现与修复
   要求：故意写一个会死锁的程序（两个锁，两个线程以相反顺序加锁），
        观察程序卡死，然后用"固定加锁顺序"修复
   提示：线程A先锁1后锁2，线程B先锁2后锁1 → 死锁
        修复：所有线程都按"先锁1后锁2"的统一顺序
)" << endl;
}

int main() {
    cout << string(60, '=') << endl;
    cout << "Linux 多线程基础 - pthread" << endl;
    cout << string(60, '=') << endl;

    showConcepts();
    showFunctionCards();

    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    demo1_basic();
    demo2_multiple();
    demo3_return_value();
    demo4_mutex();

    showCommonMistakes();
    showQuickReference();
    showExercises();

    cout << "\n程序结束。记得编译时加 -lpthread ！" << endl;
    return 0;
}
