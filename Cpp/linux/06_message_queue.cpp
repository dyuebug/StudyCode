// ============================================
// 06_message_queue.cpp
// 日期：2026-06-07（第65天）
// 主题：POSIX 消息队列
// 难度：⭐⭐ 中级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 06_mq 06_message_queue.cpp -lrt
// ============================================

/*
【核心概念】

是什么？
- POSIX消息队列：内核维护的消息缓冲区，每条消息有独立边界和优先级
- 队列名以 "/" 开头（如 "/myqueue"），在 /dev/mqueue/ 下可见

为什么需要？
- 管道是无边界字节流，消息队列每条消息独立，无需自己分帧
- 支持消息优先级：高优先级消息总是先被读取
- 支持异步通知（mq_notify）：消息到达时自动通知进程

核心特点：
- 有消息边界（vs 管道无边界）
- 有优先级（0最低，MQ_PRIO_MAX-1最高，通常为32767）
- 队列容量有限（/proc/sys/fs/mqueue/msg_max，默认10）
*/

#include <iostream>
#include <string>
using namespace std;

void showConcepts() {
    cout << R"(
============================================================
POSIX 消息队列核心概念
============================================================

【函数卡片速查】

  mq_open(name, flags, mode, attr)  — 创建/打开队列
  mq_send(mqd, msg, len, priority)  — 发送消息
  mq_receive(mqd, buf, size, &prio) — 接收（最高优先级）
  mq_close(mqd)                     — 关闭（不删除）
  mq_unlink(name)                   — 删除队列
  mq_getattr(mqd, &attr)            — 获取队列属性
  mq_notify(mqd, &sigevent)         — 注册异步通知

【mq_open() 参数详解】

  name   — 队列名，必须以"/"开头（如"/myq"）
  flags  — O_CREAT|O_RDONLY|O_WRONLY|O_RDWR|O_NONBLOCK
  mode   — 权限（O_CREAT时需要，如0666）
  attr   — struct mq_attr（可传nullptr用默认值）
    .mq_maxmsg  — 队列最大消息数（受系统限制）
    .mq_msgsize — 每条消息最大字节数

【管道 vs 消息队列对比】

  ┌─────────────┬──────────────┬──────────────────┐
  │             │  管道(pipe)  │  消息队列(mq)    │
  ├─────────────┼──────────────┼──────────────────┤
  │ 消息边界    │ ❌ 无（字节流）│ ✅ 有（每条独立）│
  │ 消息优先级  │ ❌           │ ✅               │
  │ 亲缘关系    │ 匿名需要     │ ❌ 不需要        │
  │ 持久化      │ ❌           │ 进程重启仍存在   │
  │ 异步通知    │ 需select/poll│ ✅ mq_notify     │
  │ 复杂度      │ 简单         │ 稍复杂           │
  └─────────────┴──────────────┴──────────────────┘

============================================================
)";
}

void showBasicUsage() {
    cout << R"(
------------------------------------------------------------
基本用法：发送与接收
------------------------------------------------------------

// ---- 真实Linux代码 ----
#include <mqueue.h>
#include <fcntl.h>

// ---- 发送方（写者）----
struct mq_attr attr{};
attr.mq_maxmsg  = 10;    // 队列最多10条消息
attr.mq_msgsize = 256;   // 每条最大256字节

mqd_t mq = mq_open("/myqueue", O_CREAT | O_WRONLY, 0666, &attr);

const char* msg1 = "普通消息";
const char* msg2 = "紧急消息";

mq_send(mq, msg1, strlen(msg1)+1, 0);   // 优先级0（低）
mq_send(mq, msg2, strlen(msg2)+1, 10);  // 优先级10（高）

mq_close(mq);

// ---- 接收方（读者）----
mqd_t mq = mq_open("/myqueue", O_RDONLY);

struct mq_attr cur_attr;
mq_getattr(mq, &cur_attr);
char buf[cur_attr.mq_msgsize];

unsigned int priority;
// 自动接收最高优先级消息（先收到"紧急消息"）
ssize_t n = mq_receive(mq, buf, sizeof(buf), &priority);
cout << "收到(优先级" << priority << "): " << buf << endl;

n = mq_receive(mq, buf, sizeof(buf), &priority);
cout << "收到(优先级" << priority << "): " << buf << endl;

mq_close(mq);
mq_unlink("/myqueue");  // 删除队列
)";
}

void showNonblocking() {
    cout << R"(
------------------------------------------------------------
非阻塞模式与异步通知
------------------------------------------------------------

// 非阻塞接收（队列空时立即返回 errno=EAGAIN）
mqd_t mq = mq_open("/myq", O_RDONLY | O_NONBLOCK);
ssize_t n = mq_receive(mq, buf, size, &prio);
if (n < 0 && errno == EAGAIN)
    cout << "队列为空" << endl;

// 异步通知：消息到达时发SIGUSR1
struct sigevent sev{};
sev.sigev_notify = SIGEV_SIGNAL;
sev.sigev_signo  = SIGUSR1;
mq_notify(mq, &sev);
// 注意：mq_notify是一次性的，每次收到通知后需重新注册
)";
}

void showMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

❌ 错误1：mq_receive缓冲区小于mq_msgsize
  char buf[64];
  mq_receive(mq, buf, 64, &prio);  // ❌ 若mq_msgsize=256，返回-1/EMSGSIZE
  // ✅ buf大小必须 >= mq_msgsize

❌ 错误2：程序退出后队列仍残留
  mq_close(mq);  // ❌ 只关闭fd，队列仍在/dev/mqueue/
  mq_unlink("/myqueue");  // ✅ 删除队列（类似文件unlink）

❌ 错误3：超过系统限制的队列深度
  attr.mq_maxmsg = 1000;  // ❌ 超过 /proc/sys/fs/mqueue/msg_max（默认10）
  // ✅ 先读取系统限制，或root权限下修改限制

❌ 错误4：mq_notify只触发一次忘记重新注册
  void handler(int sig) {
      // 处理消息...
      // ❌ 忘记重新调用mq_notify → 下次消息到达不会通知
      mq_notify(mq, &sev);  // ✅ 处理后立即重新注册
  }

❌ 错误5：发送方和接收方mq_msgsize不一致
  // 发送方创建：mq_msgsize=256
  // 接收方打开：mq_open不指定attr，继承原队列属性
  // 接收方buf只有64字节 → EMSGSIZE错误
  // ✅ 双方通过mq_getattr()获取实际的mq_msgsize

============================================================
)";
}

int main() {
    cout << string(60, '=') << endl;
    cout << "06_message_queue.cpp - POSIX消息队列" << endl;
    cout << "（概念演示版，完整代码需在Linux/WSL中运行）" << endl;
    cout << string(60, '=') << endl;

    showConcepts();
    showBasicUsage();
    showNonblocking();
    showMistakes();

    cout << R"(
============================================================
Linux IPC 机制总结对比
============================================================

  机制          适用场景                    特点
  ────────────────────────────────────────────────────────
  管道(pipe)    父子进程，简单单向数据流     字节流，无边界，最简单
  FIFO          任意进程，字节流传输         需文件路径，字节流
  消息队列      任意进程，结构化消息         有边界，有优先级
  共享内存      高性能，大数据量             最快，需自己同步
  信号          异步通知，轻量触发           无数据，仅通知
  Socket        网络/本地，通用              最灵活，支持跨机器

============================================================
练习题
============================================================
1. 实现任务调度器：主进程通过消息队列发送任务（带优先级），
   工作进程按优先级顺序处理任务

2. 实现发布-订阅模式：一个发布者向消息队列写消息，
   多个订阅者各自读取（提示：每个订阅者需要独立的队列）

3. 对比三种IPC的性能：分别用管道、消息队列、共享内存传输1000条消息，
   测量总耗时

4. 用mq_notify实现事件驱动的消息处理：
   不用轮询，而是在消息到达时自动处理
============================================================
)";
    return 0;
}
