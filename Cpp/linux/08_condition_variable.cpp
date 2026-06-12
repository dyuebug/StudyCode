// ============================================
// 08_condition_variable.cpp
// 日期：2026-06-09（第66天）
// 主题：条件变量 - 生产者/消费者模型
// 难度：⭐⭐⭐ 高级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 08_cond 08_condition_variable.cpp -lpthread
// 运行：./08_cond
// ============================================

/*
【核心概念】简要说明

是什么？
- 条件变量（condition variable）：让线程"等待某个条件成立"的同步机制
- 配合互斥锁使用，实现线程间的"通知-等待"

为什么需要？
- 互斥锁只能解决"互斥访问"，不能解决"等待条件"
- 比如消费者要等"队列里有数据"才能取，没数据时该怎么办？
  ❌ 忙等待（while 循环空转）：浪费 CPU
  ✅ 条件变量：让线程睡眠，有数据时被唤醒，零 CPU 浪费

核心特点？
- wait：释放锁 + 睡眠，被唤醒后重新拿锁（这三步是原子的）
- signal：唤醒一个等待的线程
- broadcast：唤醒所有等待的线程

【与你学过的知识对照】
  RTOS 的信号量（semaphore）   → 类似的"等待/通知"思想
  Qt 的 QWaitCondition         → 就是条件变量的 Qt 封装
*/

#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <queue>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void showConcepts() {
    cout << R"(
============================================================
条件变量 - 核心概念
============================================================

【为什么条件变量必须配合互斥锁？】⭐⭐⭐⭐⭐

  考虑消费者的逻辑：
    1. 检查队列是否为空
    2. 如果空，就等待
    3. 如果非空，取出数据

  问题：步骤1和步骤2之间存在"时间窗口"！
    消费者检查发现"空"（步骤1）
    → 此时生产者插入了数据并发出通知
    → 消费者才进入等待（步骤2）
    → 通知已经错过了，消费者永远睡下去（丢失唤醒！）

  解决：用互斥锁保护"检查条件"和"进入等待"，
       pthread_cond_wait 会原子地"解锁+睡眠"，
       保证检查和等待之间没有时间窗口。

【pthread_cond_wait 内部做了什么？】

  pthread_cond_wait(&cond, &mutex) 执行三步（原子）：
    1. 解锁 mutex（让别人能修改条件）
    2. 线程睡眠，等待被唤醒
    3. 被唤醒后，重新锁住 mutex（再继续往下执行）

  所以调用 wait 前必须已经持有锁，wait 返回后依然持有锁。

【标准使用模板】⭐⭐⭐⭐⭐

  // 等待方（消费者）：
  pthread_mutex_lock(&mutex);
  while (条件不满足) {            // ← 必须是 while，不是 if！
      pthread_cond_wait(&cond, &mutex);
  }
  // 条件满足了，执行操作
  pthread_mutex_unlock(&mutex);

  // 通知方（生产者）：
  pthread_mutex_lock(&mutex);
  // 修改条件（比如往队列放数据）
  pthread_cond_signal(&cond);    // 通知等待方
  pthread_mutex_unlock(&mutex);
============================================================
)" << endl;
}

// ============================================
// 第一部分补充：核心函数卡片 ⭐⭐⭐⭐⭐
// ============================================

void showFunctionCards() {
    cout << R"(
============================================================
条件变量核心函数卡片
============================================================

【函数1：pthread_cond_wait() - 等待条件】
语法：int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
参数：
  1. cond  - 条件变量地址
  2. mutex - 配套的互斥锁地址（调用前必须已持有此锁）
行为：原子地"解锁mutex + 睡眠"，被唤醒后"重新锁mutex"
返回：成功返回0

【函数2：pthread_cond_signal() - 唤醒一个】
语法：int pthread_cond_signal(pthread_cond_t* cond);
作用：唤醒至少一个正在 wait 的线程（如果有多个，唤醒哪个不确定）
适用：一个生产者-多个消费者，每次只需唤醒一个

【函数3：pthread_cond_broadcast() - 唤醒所有】
语法：int pthread_cond_broadcast(pthread_cond_t* cond);
作用：唤醒所有正在 wait 的线程
适用：条件变化影响所有等待者（比如"关闭"信号）

【初始化】
  静态：pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  动态：pthread_cond_init(&cond, NULL);  用完 pthread_cond_destroy(&cond);

【signal vs broadcast 怎么选？】
  - 唤醒一个就够（如取走一个任务）→ signal（更高效）
  - 需要所有线程都醒来检查（如退出通知）→ broadcast
)" << endl;
}

// ============================================
// 第二部分：基础操作实践 - 生产者/消费者
// ============================================

// 共享资源：一个有界缓冲队列
queue<int> g_queue;                                   // 数据队列
const size_t MAX_SIZE = 5;                            // 队列容量上限
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;  // 保护队列的锁
pthread_cond_t g_notEmpty = PTHREAD_COND_INITIALIZER; // "非空"条件（消费者等）
pthread_cond_t g_notFull  = PTHREAD_COND_INITIALIZER; // "非满"条件（生产者等）
bool g_done = false;                                  // 生产是否结束的标志

// 生产者：生产 itemCount 个数据
void* producer(void* arg) {
    int itemCount = *(int*)arg;
    for (int i = 1; i <= itemCount; i++) {
        pthread_mutex_lock(&g_mutex);

        // 队列满了就等"非满"条件（用 while 防虚假唤醒）
        while (g_queue.size() >= MAX_SIZE) {
            cout << "  [生产者] 队列满，等待消费..." << endl;
            pthread_cond_wait(&g_notFull, &g_mutex);
        }

        g_queue.push(i);  // 放入数据
        cout << "  [生产者] 生产 " << i
             << "（队列大小=" << g_queue.size() << "）" << endl;

        // 通知消费者"队列非空了"
        pthread_cond_signal(&g_notEmpty);
        pthread_mutex_unlock(&g_mutex);

        usleep(100000);  // 睡0.1秒，让现象更明显
    }

    // 生产结束，设标志并唤醒所有消费者（让它们能退出）
    pthread_mutex_lock(&g_mutex);
    g_done = true;
    pthread_cond_broadcast(&g_notEmpty);  // broadcast：叫醒所有消费者
    pthread_mutex_unlock(&g_mutex);

    return nullptr;
}

// 消费者：不断从队列取数据，直到生产结束且队列空
void* consumer(void* arg) {
    int id = *(int*)arg;
    while (true) {
        pthread_mutex_lock(&g_mutex);

        // 队列空 且 还没结束 → 等待"非空"条件
        while (g_queue.empty() && !g_done) {
            pthread_cond_wait(&g_notEmpty, &g_mutex);
        }

        // 醒来后判断：如果队列空且已结束，就退出
        if (g_queue.empty() && g_done) {
            pthread_mutex_unlock(&g_mutex);
            break;
        }

        int item = g_queue.front();  // 取数据
        g_queue.pop();
        cout << "    [消费者" << id << "] 消费 " << item
             << "（队列大小=" << g_queue.size() << "）" << endl;

        // 通知生产者"队列非满了"
        pthread_cond_signal(&g_notFull);
        pthread_mutex_unlock(&g_mutex);

        usleep(150000);  // 消费稍慢于生产，制造"队列变满"的现象
    }
    cout << "    [消费者" << id << "] 退出" << endl;
    return nullptr;
}

void demo_producer_consumer() {
    cout << "\n第二部分：生产者/消费者实战" << endl;
    cout << string(60, '-') << endl;

    pthread_t prodTid, consTids[2];
    int itemCount = 10;        // 生产10个
    int consIds[2] = {1, 2};   // 2个消费者

    pthread_create(&prodTid, nullptr, producer, &itemCount);
    for (int i = 0; i < 2; i++)
        pthread_create(&consTids[i], nullptr, consumer, &consIds[i]);

    pthread_join(prodTid, nullptr);
    for (int i = 0; i < 2; i++)
        pthread_join(consTids[i], nullptr);

    cout << "  所有任务完成，最终队列大小=" << g_queue.size() << endl;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
==============================================
🔍 深入理解：为什么必须用 while 而不是 if？（选学）⭐⭐⭐⭐⭐
==============================================

这是条件变量最容易踩的坑，务必理解。

错误写法：
  if (g_queue.empty()) {
      pthread_cond_wait(&cond, &mutex);
  }
  // 直接取数据 ← 危险！

正确写法：
  while (g_queue.empty()) {
      pthread_cond_wait(&cond, &mutex);
  }

为什么必须 while？两个原因：

【原因1：虚假唤醒（spurious wakeup）】
  操作系统允许 pthread_cond_wait 在"没有人 signal"的情况下
  也偶尔返回（这是 POSIX 标准明确允许的，为了实现效率）。
  用 if：醒来后不再检查条件，直接往下执行 → 队列其实还是空的 → 崩溃
  用 while：醒来后重新检查条件，发现还是空 → 继续 wait → 安全

【原因2：唤醒丢失/竞争（多消费者场景）】
  假设队列里有1个数据，broadcast 唤醒了2个消费者：
    消费者A 先拿到锁，取走了数据，解锁
    消费者B 后拿到锁，此时队列又空了
  用 if：消费者B 不检查，直接取空队列 → 崩溃
  用 while：消费者B 重新检查发现空 → 继续 wait → 安全

  口诀：唤醒只是"提示去检查"，不是"保证条件成立"。
       所以醒来后必须重新检查 → 用 while。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

【错误1：用 if 代替 while 检查条件】
❌ if (空) wait;        → 虚假唤醒/多消费者竞争时崩溃
✅ while (空) wait;     → 醒来重新检查，安全
预防：等待条件永远用 while 包裹 wait（最重要的规则！）

【错误2：调用 wait 前没有持有锁】
❌ pthread_cond_wait(&cond, &mutex);  // 之前没 lock
   问题：wait 要操作 mutex，未加锁行为未定义
✅ pthread_mutex_lock(&mutex);
   while(...) pthread_cond_wait(&cond, &mutex);
预防：wait 前必须先 lock 配套的 mutex

【错误3：signal 在修改条件之前发出】
❌ pthread_cond_signal(&cond);  // 还没放数据就通知
   g_queue.push(x);
   问题：消费者醒来时数据还没放进去
✅ g_queue.push(x);             // 先改条件
   pthread_cond_signal(&cond);  // 再通知
预防：先修改共享状态，再 signal

【错误4：生产结束不通知，消费者永远卡死】
❌ 生产完直接退出，消费者还在 wait "非空"，永远不醒
✅ 设置 g_done=true，再 broadcast 唤醒所有消费者去检查退出条件
预防：设计退出标志 + broadcast，让等待线程能优雅退出

【错误5：忘记配对的两个条件（有界队列）】
❌ 只用一个条件变量，生产者满了无法等待
✅ 用两个：notFull（生产者等）+ notEmpty（消费者等）
预防：有界缓冲区需要双条件变量

【错误6：cond 和 mutex 不配套】
❌ wait 用 mutexA，但保护条件的是 mutexB
   问题：检查条件和等待用了不同的锁，竞争依旧
✅ 同一个条件变量始终配同一个 mutex
预防：一个条件变量绑定一个固定的 mutex
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showQuickReference() {
    cout << R"(
============================================================
条件变量速查表
============================================================

【核心函数】
  pthread_cond_wait(&cond, &mutex)   等待（解锁+睡眠+醒来重新锁）
  pthread_cond_signal(&cond)         唤醒一个等待线程
  pthread_cond_broadcast(&cond)      唤醒所有等待线程
  pthread_cond_init / destroy        动态初始化 / 销毁

【黄金模板 - 等待方】
  lock(&m);
  while (条件不满足)
      cond_wait(&cond, &m);   // 永远用 while！
  操作共享资源;
  unlock(&m);

【黄金模板 - 通知方】
  lock(&m);
  修改共享状态;               // 先改
  cond_signal(&cond);         // 再通知
  unlock(&m);

【记忆口诀】
  改状态在前，发信号在后；
  等条件用 while，配 mutex 不离手。
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

1. 【基础】单生产者单消费者
   要求：生产者生产数字 1~20，单个消费者全部消费并求和打印
   提示：用一个队列 + notEmpty 条件，注意结束通知

2. 【容量控制】观察队列变满
   要求：把 MAX_SIZE 改成 2，消费者 sleep 久一点，
        观察生产者打印"队列满，等待消费"的现象
   提示：体会 notFull 条件什么时候被触发

3. 【多消费者】任务分发
   要求：1个生产者生产30个任务，3个消费者抢着消费，
        统计每个消费者各消费了多少个
   提示：每个消费者维护自己的计数，注意 broadcast 退出

4. 【signal vs broadcast】实验对比
   要求：把退出时的 broadcast 改成 signal，观察是否有消费者卡死
   提示：思考 signal 只唤醒一个，其他消费者怎么办

5. 【综合】线程安全的阻塞队列类
   要求：把队列+锁+条件变量封装成一个 BlockingQueue 类，
        提供 push() 和 pop() 两个方法
   提示：成员变量放 queue/mutex/cond，构造函数 init，析构 destroy
)" << endl;
}

int main() {
    cout << string(60, '=') << endl;
    cout << "条件变量 - 生产者/消费者模型" << endl;
    cout << string(60, '=') << endl;

    showConcepts();
    showFunctionCards();
    demo_producer_consumer();

    showCommonMistakes();
    showQuickReference();
    showExercises();

    cout << "\n程序结束。记住：等条件用 while，改状态在 signal 前！" << endl;
    return 0;
}
