// ============================================
// 线程同步基础 - 超详细注释版
// 日期：2026-05-31（第50天）
// 难度：⭐⭐⭐⭐ 高级
// ============================================

/*
【核心概念】线程同步与数据竞争

是什么？
- 线程同步：协调多个线程对共享资源的访问
- 数据竞争：多个线程同时访问同一数据，至少一个是写操作

为什么需要？
- 多个线程同时修改共享数据会导致数据不一致
- 例如：两个线程同时对计数器 +1，结果可能只增加了 1 次

核心特点：
- QMutex（互斥锁）：同一时刻只允许一个线程访问
- QMutexLocker（自动锁）：RAII 方式管理锁
- QReadWriteLock（读写锁）：允许多个读者，但只有一个写者
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QReadWriteLock>
#include <QDebug>

/*
────────────────────────────────────────────
1.1 什么是数据竞争？⭐⭐⭐⭐⭐
────────────────────────────────────────────

【问题场景】
假设有一个全局计数器 counter = 0，两个线程同时执行 counter++：

线程A：读取 counter（0） → 计算 0+1 → 写入 counter（1）
线程B：读取 counter（0） → 计算 0+1 → 写入 counter（1）

预期结果：counter = 2
实际结果：counter = 1（数据丢失！）

【为什么会这样？】
counter++ 不是原子操作，实际分为三步：
1. 读取 counter 的值
2. 计算 counter + 1
3. 写入新值到 counter

如果两个线程同时执行，可能交错执行：
时刻1：线程A 读取 counter（0）
时刻2：线程B 读取 counter（0）
时刻3：线程A 计算 0+1，写入 counter（1）
时刻4：线程B 计算 0+1，写入 counter（1）← 覆盖了线程A的结果！

【数据竞争的危害】
- 数据不一致（计数错误、余额错误）
- 程序崩溃（访问已释放的内存）
- 难以调试（问题不稳定，难以重现）

【解决方案】
使用互斥锁（Mutex）保护共享数据，确保同一时刻只有一个线程访问。
*/

/*
────────────────────────────────────────────
1.2 QMutex 互斥锁 ⭐⭐⭐⭐⭐
────────────────────────────────────────────

【QMutex 类】
- 作用：互斥锁，保护共享数据
- 原理：同一时刻只允许一个线程持有锁
- 核心方法：
  * lock()：获取锁（如果已被占用，则阻塞等待）
  * unlock()：释放锁
  * tryLock()：尝试获取锁（不阻塞，立即返回）

【使用模式】
QMutex mutex;

// 线程A
mutex.lock();      // 获取锁
counter++;         // 访问共享数据
mutex.unlock();    // 释放锁

// 线程B
mutex.lock();      // 等待线程A释放锁
counter++;         // 访问共享数据
mutex.unlock();    // 释放锁

【关键点】
- lock() 和 unlock() 必须成对出现
- 忘记 unlock() 会导致死锁（其他线程永远等待）
- 推荐使用 QMutexLocker 自动管理锁
*/

/*
────────────────────────────────────────────
1.3 QMutexLocker 自动锁管理 ⭐⭐⭐⭐⭐
────────────────────────────────────────────

【QMutexLocker 类】
- 作用：RAII 方式管理锁（构造时加锁，析构时解锁）
- 优点：自动释放锁，避免忘记 unlock()
- 异常安全：即使发生异常，也会自动释放锁

【使用模式】
QMutex mutex;

void someFunction() {
    QMutexLocker locker(&mutex);  // 构造时自动 lock()

    counter++;  // 访问共享数据

    // 函数结束时，locker 析构，自动 unlock()
}

【对比手动 lock/unlock】
// ❌ 手动管理（容易出错）
mutex.lock();
counter++;
if (error) {
    return;  // 忘记 unlock()，死锁！
}
mutex.unlock();

// ✅ 自动管理（推荐）
QMutexLocker locker(&mutex);
counter++;
if (error) {
    return;  // locker 析构，自动 unlock()
}
*/

/*
────────────────────────────────────────────
1.4 QReadWriteLock 读写锁 ⭐⭐⭐⭐
────────────────────────────────────────────

【QReadWriteLock 类】
- 作用：读写锁，允许多个读者，但只有一个写者
- 适用场景：读多写少的情况
- 核心方法：
  * lockForRead()：获取读锁（多个线程可以同时持有）
  * lockForWrite()：获取写锁（独占，其他线程不能读也不能写）
  * unlock()：释放锁

【使用模式】
QReadWriteLock rwLock;

// 读线程（可以多个同时读）
rwLock.lockForRead();
int value = sharedData;  // 读取共享数据
rwLock.unlock();

// 写线程（独占访问）
rwLock.lockForWrite();
sharedData = newValue;   // 修改共享数据
rwLock.unlock();

【QMutex vs QReadWriteLock】
- QMutex：读写都互斥（同一时刻只有一个线程）
- QReadWriteLock：读读不互斥，读写互斥，写写互斥
- 选择：读多写少用 QReadWriteLock，否则用 QMutex
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 示例1：数据竞争演示（不使用锁）
// --------------------------------------------

// 全局共享数据
int globalCounter = 0;

// 工作对象：不使用锁（会出现数据竞争）
class UnsafeWorker : public QObject {
    Q_OBJECT
public:
    explicit UnsafeWorker(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void increment() {
        // 每个线程执行 10000 次 counter++
        for (int i = 0; i < 10000; ++i) {
            globalCounter++;  // ❌ 数据竞争！
        }
        emit finished();
    }

signals:
    void finished();
};

// --------------------------------------------
// 2.2 示例2：使用 QMutex 解决数据竞争
// --------------------------------------------

// 全局互斥锁
QMutex globalMutex;
int safeCounter = 0;

// 工作对象：使用 QMutex 保护共享数据
class SafeWorker : public QObject {
    Q_OBJECT
public:
    explicit SafeWorker(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void increment() {
        // 每个线程执行 10000 次 counter++
        for (int i = 0; i < 10000; ++i) {
            globalMutex.lock();      // 获取锁
            safeCounter++;           // ✅ 安全访问共享数据
            globalMutex.unlock();    // 释放锁
        }
        emit finished();
    }

signals:
    void finished();
};

// --------------------------------------------
// 2.3 示例3：使用 QMutexLocker（推荐）
// --------------------------------------------

int lockerCounter = 0;

class LockerWorker : public QObject {
    Q_OBJECT
public:
    explicit LockerWorker(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void increment() {
        for (int i = 0; i < 10000; ++i) {
            // ✅ 推荐：使用 QMutexLocker 自动管理锁
            QMutexLocker locker(&globalMutex);
            lockerCounter++;
            // locker 析构时自动 unlock()
        }
        emit finished();
    }

signals:
    void finished();
};

// --------------------------------------------
// 2.4 示例4：QReadWriteLock 读写锁
// --------------------------------------------

QReadWriteLock rwLock;
QVector<int> sharedData;

class ReaderWorker : public QObject {
    Q_OBJECT
public:
    explicit ReaderWorker(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void readData() {
        for (int i = 0; i < 100; ++i) {
            // 获取读锁（多个读者可以同时持有）
            rwLock.lockForRead();

            // 读取共享数据
            int size = sharedData.size();
            qDebug() << "读线程" << QThread::currentThreadId() << "读取大小：" << size;

            // 释放读锁
            rwLock.unlock();

            QThread::msleep(10);
        }
        emit finished();
    }

signals:
    void finished();
};

class WriterWorker : public QObject {
    Q_OBJECT
public:
    explicit WriterWorker(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void writeData() {
        for (int i = 0; i < 50; ++i) {
            // 获取写锁（独占访问）
            rwLock.lockForWrite();

            // 修改共享数据
            sharedData.append(i);
            qDebug() << "写线程" << QThread::currentThreadId() << "写入：" << i;

            // 释放写锁
            rwLock.unlock();

            QThread::msleep(20);
        }
        emit finished();
    }

signals:
    void finished();
};

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
==============================================
🔍 深入理解：死锁问题（选学）
==============================================

【什么是死锁？】
两个或多个线程互相等待对方释放锁，导致所有线程都无法继续执行。

【死锁示例】
QMutex mutexA, mutexB;

// 线程1
mutexA.lock();
// ... 执行一些操作
mutexB.lock();  // 等待线程2释放 mutexB
// ...
mutexB.unlock();
mutexA.unlock();

// 线程2
mutexB.lock();
// ... 执行一些操作
mutexA.lock();  // 等待线程1释放 mutexA
// ...
mutexA.unlock();
mutexB.unlock();

【死锁发生的时序】
时刻1：线程1 获取 mutexA
时刻2：线程2 获取 mutexB
时刻3：线程1 尝试获取 mutexB（阻塞，等待线程2释放）
时刻4：线程2 尝试获取 mutexA（阻塞，等待线程1释放）
→ 死锁！两个线程互相等待

【死锁的四个必要条件】
1. 互斥：资源不能被多个线程同时使用
2. 持有并等待：线程持有资源的同时等待其他资源
3. 不可剥夺：资源不能被强制释放
4. 循环等待：存在线程等待环路

【预防死锁的方法】
1. 固定加锁顺序：所有线程按相同顺序获取锁
   // 所有线程都先锁 mutexA，再锁 mutexB
   mutexA.lock();
   mutexB.lock();
   // ...
   mutexB.unlock();
   mutexA.unlock();

2. 使用 tryLock()：尝试获取锁，失败则释放已持有的锁
   if (mutexA.tryLock()) {
       if (mutexB.tryLock()) {
           // 成功获取两个锁
           mutexB.unlock();
       }
       mutexA.unlock();
   }

3. 减少锁的持有时间：尽快释放锁
4. 避免嵌套锁：尽量只持有一个锁

==============================================
*/

/*
==============================================
🔍 深入理解：锁的粒度（选学）
==============================================

【锁的粒度】
- 粗粒度锁：一个锁保护大量数据（简单，但并发性差）
- 细粒度锁：多个锁分别保护不同数据（复杂，但并发性好）

【粗粒度锁示例】
QMutex mutex;
QVector<int> dataA;
QVector<int> dataB;

void updateA() {
    QMutexLocker locker(&mutex);  // 锁住所有数据
    dataA.append(1);
}

void updateB() {
    QMutexLocker locker(&mutex);  // 即使只修改 dataB，也要等待 dataA 的锁
    dataB.append(2);
}

【细粒度锁示例】
QMutex mutexA, mutexB;
QVector<int> dataA;
QVector<int> dataB;

void updateA() {
    QMutexLocker locker(&mutexA);  // 只锁 dataA
    dataA.append(1);
}

void updateB() {
    QMutexLocker locker(&mutexB);  // 只锁 dataB，可以并发执行
    dataB.append(2);
}

【选择建议】
- 初学者：使用粗粒度锁（简单，不易出错）
- 性能要求高：使用细粒度锁（复杂，需要仔细设计）

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
────────────────────────────────────────────
4.1 错误1：忘记释放锁
────────────────────────────────────────────

❌ 错误代码：
QMutex mutex;

void someFunction() {
    mutex.lock();
    counter++;
    if (error) {
        return;  // ❌ 忘记 unlock()，死锁！
    }
    mutex.unlock();
}

结果：
- 其他线程永远等待锁
- 程序卡死

原因：
- 提前 return，跳过了 unlock()

✅ 正确代码：
void someFunction() {
    QMutexLocker locker(&mutex);  // ✅ 自动管理锁
    counter++;
    if (error) {
        return;  // locker 析构，自动 unlock()
    }
}

记忆技巧：
- 永远使用 QMutexLocker，不要手动 lock/unlock

────────────────────────────────────────────
4.2 错误2：锁的粒度过大
────────────────────────────────────────────

❌ 错误代码：
QMutex mutex;

void processData() {
    mutex.lock();

    // 耗时操作（如网络请求、文件读写）
    QThread::sleep(5);  // 持有锁 5 秒

    counter++;  // 实际只需要保护这一行

    mutex.unlock();
}

结果：
- 其他线程等待 5 秒
- 并发性能差

原因：
- 锁保护了不需要保护的代码

✅ 正确代码：
void processData() {
    // 耗时操作（不需要锁）
    QThread::sleep(5);

    // 只在访问共享数据时加锁
    {
        QMutexLocker locker(&mutex);
        counter++;
    }  // 立即释放锁
}

记忆技巧：
- 锁的范围越小越好
- 只保护必须保护的代码

────────────────────────────────────────────
4.3 错误3：在持有锁时调用外部函数
────────────────────────────────────────────

❌ 错误代码：
QMutex mutex;

void externalFunction() {
    // 可能也需要获取 mutex
    QMutexLocker locker(&mutex);
    // ...
}

void myFunction() {
    QMutexLocker locker(&mutex);
    externalFunction();  // ❌ 可能导致死锁
}

结果：
- 如果 externalFunction 也尝试获取 mutex，死锁
- 难以调试（问题隐藏在外部函数中）

原因：
- 不知道外部函数的锁需求

✅ 正确代码：
void myFunction() {
    // 准备数据
    int localData;
    {
        QMutexLocker locker(&mutex);
        localData = sharedData;
    }  // 释放锁

    // 调用外部函数（不持有锁）
    externalFunction(localData);
}

记忆技巧：
- 持有锁时不要调用外部函数
- 先复制数据，释放锁，再调用函数

────────────────────────────────────────────
4.4 错误4：使用错误的锁类型
────────────────────────────────────────────

❌ 错误代码：
// 读多写少的场景，使用 QMutex
QMutex mutex;

void readData() {
    QMutexLocker locker(&mutex);
    // 读取数据（多个读者也要互斥，性能差）
}

结果：
- 多个读线程无法并发执行
- 性能差

原因：
- QMutex 不区分读写，都是互斥的

✅ 正确代码：
// 读多写少的场景，使用 QReadWriteLock
QReadWriteLock rwLock;

void readData() {
    rwLock.lockForRead();  // ✅ 多个读者可以并发
    // 读取数据
    rwLock.unlock();
}

void writeData() {
    rwLock.lockForWrite();  // 独占访问
    // 修改数据
    rwLock.unlock();
}

记忆技巧：
- 读多写少用 QReadWriteLock
- 其他情况用 QMutex

────────────────────────────────────────────
*/

// ============================================
// 第五部分：函数卡片速查
// ============================================

/*
【QMutex 核心方法】

方法：lock()
作用：获取锁（如果已被占用，则阻塞等待）
语法：void lock()
参数：无
返回：无
示例：mutex.lock();

────────────────────────────────────

方法：unlock()
作用：释放锁
语法：void unlock()
参数：无
返回：无
示例：mutex.unlock();

────────────────────────────────────

方法：tryLock()
作用：尝试获取锁（不阻塞，立即返回）
语法：bool tryLock(int timeout = 0)
参数：timeout - 超时时间（毫秒），0 表示立即返回
返回：true - 成功获取锁，false - 锁已被占用
示例：if (mutex.tryLock()) { ... }

────────────────────────────────────

【QMutexLocker 核心方法】

构造函数：QMutexLocker(QMutex *mutex)
作用：构造时自动调用 mutex->lock()
参数：mutex - 要管理的互斥锁
示例：QMutexLocker locker(&mutex);

析构函数：~QMutexLocker()
作用：析构时自动调用 mutex->unlock()
示例：// 自动调用，无需手动

────────────────────────────────────

【QReadWriteLock 核心方法】

方法：lockForRead()
作用：获取读锁（多个线程可以同时持有）
语法：void lockForRead()
参数：无
返回：无
示例：rwLock.lockForRead();

────────────────────────────────────

方法：lockForWrite()
作用：获取写锁（独占，其他线程不能读也不能写）
语法：void lockForWrite()
参数：无
返回：无
示例：rwLock.lockForWrite();

────────────────────────────────────
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
练习1：银行账户转账
要求：
- 创建两个账户，初始余额各 1000 元
- 创建多个线程，随机从账户A转账到账户B，或从B转到A
- 使用 QMutex 保护账户余额
- 确保总金额始终为 2000 元（不会丢失或增加）

提示：
- 转账需要同时锁住两个账户
- 注意死锁问题（固定加锁顺序）

────────────────────────────────────

练习2：生产者-消费者模式
要求：
- 创建一个共享队列（QQueue）
- 生产者线程：向队列添加数据
- 消费者线程：从队列取出数据
- 使用 QMutex 保护队列

提示：
- 队列为空时，消费者需要等待
- 可以使用 QWaitCondition 实现等待/唤醒

────────────────────────────────────

练习3：读写锁性能对比
要求：
- 创建一个共享数据结构（如 QVector）
- 使用 QMutex 实现读写保护
- 使用 QReadWriteLock 实现读写保护
- 对比两种方式的性能（读多写少场景）

提示：
- 创建 10 个读线程，1 个写线程
- 测量总执行时间

────────────────────────────────────
*/

// ============================================
// 主函数
// ============================================

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    qDebug() << "线程同步示例";
    qDebug() << "请查看代码中的详细注释和练习题";

    return 0;
}

#include "34_thread_sync.moc"
