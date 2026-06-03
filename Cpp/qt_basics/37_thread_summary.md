# Qt 多线程编程速查表 ⚡

> **阶段10总结** - 快速查找多线程编程常用代码模板

---

## 📋 目录

1. [核心类概览](#核心类概览)
2. [QThread 基础模板](#qthread-基础模板)
3. [QtConcurrent 模板](#qtconcurrent-模板)
4. [线程同步模板](#线程同步模板)
5. [常见错误和最佳实践](#常见错误和最佳实践)
6. [QThread vs QtConcurrent](#qthread-vs-qtconcurrent)

---

## 核心类概览

| 类名 | 作用 | 常用方法/信号 |
|------|------|---------------|
| **QThread** | 线程类 | `start()`, `quit()`, `wait()`, `isRunning()` |
| **QMutex** | 互斥锁 | `lock()`, `unlock()`, `tryLock()` |
| **QMutexLocker** | 自动锁管理 | 构造时加锁，析构时解锁 |
| **QReadWriteLock** | 读写锁 | `lockForRead()`, `lockForWrite()`, `unlock()` |
| **QtConcurrent** | 高级并发框架 | `run()`, `mapped()`, `filtered()` |
| **QFuture** | 异步任务结果 | `result()`, `isFinished()`, `cancel()` |
| **QFutureWatcher** | 监听任务完成 | `finished()`, `progressValueChanged()` |
| **QThreadPool** | 线程池 | `globalInstance()`, `maxThreadCount()` |

---

## QThread 基础模板

### 模板1：moveToThread 模式（推荐）⭐⭐⭐⭐⭐

```cpp
// 步骤1：创建工作对象
class Worker : public QObject {
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr) : QObject(parent) {}

signals:
    void finished(int result);
    void progress(int percent);

public slots:
    void doWork() {
        // 耗时任务
        for (int i = 0; i < 100; ++i) {
            // 执行任务
            emit progress(i);
        }
        emit finished(42);
    }
};

// 步骤2：在主窗口中使用
class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow() {
        // 创建工作对象（不设置父对象）
        worker = new Worker();

        // 创建线程对象
        thread = new QThread(this);

        // 将工作对象移动到线程
        worker->moveToThread(thread);

        // 连接信号槽
        connect(button, &QPushButton::clicked, worker, &Worker::doWork);
        connect(worker, &Worker::finished, this, &MainWindow::onFinished);
        connect(worker, &Worker::progress, this, &MainWindow::onProgress);
        connect(thread, &QThread::finished, worker, &QObject::deleteLater);

        // 启动线程
        thread->start();
    }

    ~MainWindow() {
        // 清理线程
        thread->quit();
        thread->wait();
    }

private:
    Worker *worker;
    QThread *thread;
};
```

### 模板2：取消机制

```cpp
class Worker : public QObject {
    Q_OBJECT
public:
    Worker() : shouldStop(false) {}

public slots:
    void doWork() {
        for (int i = 0; i < 1000; ++i) {
            // 检查取消标志
            if (shouldStop) {
                emit cancelled();
                return;
            }
            // 执行任务
        }
        emit finished();
    }

    void cancel() {
        shouldStop = true;
    }

signals:
    void finished();
    void cancelled();

private:
    bool shouldStop;
};
```

---

## QtConcurrent 模板

### 模板1：QtConcurrent::run() 基础用法 ⭐⭐⭐⭐⭐

```cpp
// 创建 Watcher
QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);

// 连接完成信号
connect(watcher, &QFutureWatcher<int>::finished, this, [watcher]() {
    int result = watcher->result();
    qDebug() << "结果：" << result;
    watcher->deleteLater();  // 清理 Watcher
});

// 启动异步任务
QFuture<int> future = QtConcurrent::run([]() {
    // 耗时计算
    QThread::sleep(2);
    return 42;
});

// 监听任务
watcher->setFuture(future);
```

### 模板2：QtConcurrent::run() 使用函数指针

```cpp
// 定义函数
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// 使用函数指针
QFuture<int> future = QtConcurrent::run(fibonacci, 40);
```

### 模板3：QtConcurrent::mapped() 并行映射

```cpp
// 映射函数
int square(int x) {
    return x * x;
}

// 输入数据
QVector<int> input = {1, 2, 3, 4, 5};

// 并行映射
QFuture<int> future = QtConcurrent::mapped(input, square);

// 监听完成
QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);
connect(watcher, &QFutureWatcher<int>::finished, this, [watcher]() {
    QList<int> results = watcher->future().results();
    // 处理结果
    watcher->deleteLater();
});
watcher->setFuture(future);
```

### 模板4：QtConcurrent::filtered() 并行过滤

```cpp
// 过滤函数
bool isEven(int x) {
    return x % 2 == 0;
}

// 输入数据
QVector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// 并行过滤
QFuture<int> future = QtConcurrent::filtered(input, isEven);

// 监听完成
QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);
connect(watcher, &QFutureWatcher<int>::finished, this, [watcher]() {
    QList<int> results = watcher->future().results();
    // 结果：2, 4, 6, 8, 10
    watcher->deleteLater();
});
watcher->setFuture(future);
```

---

## 线程同步模板

### 模板1：QMutex 互斥锁

```cpp
// 全局共享数据和互斥锁
QMutex mutex;
int sharedCounter = 0;

// 线程A
void threadA() {
    mutex.lock();
    sharedCounter++;
    mutex.unlock();
}

// 线程B
void threadB() {
    mutex.lock();
    sharedCounter++;
    mutex.unlock();
}
```

### 模板2：QMutexLocker 自动锁管理（推荐）⭐⭐⭐⭐⭐

```cpp
QMutex mutex;
int sharedCounter = 0;

void someFunction() {
    QMutexLocker locker(&mutex);  // 构造时自动 lock()
    sharedCounter++;
    // 函数结束时，locker 析构，自动 unlock()
}
```

### 模板3：QReadWriteLock 读写锁

```cpp
QReadWriteLock rwLock;
QVector<int> sharedData;

// 读线程（可以多个同时读）
void readData() {
    rwLock.lockForRead();
    int size = sharedData.size();
    rwLock.unlock();
}

// 写线程（独占访问）
void writeData() {
    rwLock.lockForWrite();
    sharedData.append(42);
    rwLock.unlock();
}
```

### 模板4：避免死锁 - 固定加锁顺序

```cpp
QMutex mutexA, mutexB;

// 所有线程都按相同顺序加锁
void safeFunction() {
    mutexA.lock();  // 先锁 A
    mutexB.lock();  // 再锁 B

    // 访问共享数据

    mutexB.unlock();
    mutexA.unlock();
}
```

---

## 常见错误和最佳实践

### ❌ 常见错误

#### 错误1：在主线程调用 result()
```cpp
// ❌ 错误：阻塞主线程
QFuture<int> future = QtConcurrent::run([]() {
    QThread::sleep(5);
    return 42;
});
int result = future.result();  // UI 卡死 5 秒

// ✅ 正确：使用 QFutureWatcher
QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);
connect(watcher, &QFutureWatcher<int>::finished, this, [watcher]() {
    int result = watcher->result();
    watcher->deleteLater();
});
watcher->setFuture(future);
```

#### 错误2：在工作线程中操作 UI
```cpp
// ❌ 错误：在工作线程操作 UI
QtConcurrent::run([this]() {
    label->setText("计算中...");  // 崩溃！
});

// ✅ 正确：通过信号槽通知主线程
QtConcurrent::run([this]() {
    QMetaObject::invokeMethod(this, [this]() {
        label->setText("计算中...");
    }, Qt::QueuedConnection);
});
```

#### 错误3：忘记释放锁
```cpp
// ❌ 错误：忘记 unlock()
mutex.lock();
counter++;
if (error) {
    return;  // 忘记 unlock()，死锁！
}
mutex.unlock();

// ✅ 正确：使用 QMutexLocker
QMutexLocker locker(&mutex);
counter++;
if (error) {
    return;  // locker 析构，自动 unlock()
}
```

#### 错误4：moveToThread 时设置了父对象
```cpp
// ❌ 错误：有父对象
Worker *worker = new Worker(this);
worker->moveToThread(thread);  // 失败！

// ✅ 正确：不设置父对象
Worker *worker = new Worker();
worker->moveToThread(thread);
connect(thread, &QThread::finished, worker, &QObject::deleteLater);
```

#### 错误5：忘记 deleteLater() 清理 Watcher
```cpp
// ❌ 错误：内存泄漏
connect(watcher, &QFutureWatcher<int>::finished, this, [watcher]() {
    int result = watcher->result();
    // 忘记 deleteLater()
});

// ✅ 正确：清理 Watcher
connect(watcher, &QFutureWatcher<int>::finished, this, [watcher]() {
    int result = watcher->result();
    watcher->deleteLater();
});
```

### ✅ 最佳实践

1. **永远使用 QMutexLocker**
   ```cpp
   QMutexLocker locker(&mutex);  // 自动管理锁
   ```

2. **永远使用 QFutureWatcher**
   ```cpp
   // 不要在主线程调用 result()
   watcher->setFuture(future);
   ```

3. **moveToThread 的对象不能有父对象**
   ```cpp
   Worker *worker = new Worker();  // 不设置父对象
   ```

4. **析构函数中清理线程**
   ```cpp
   ~MainWindow() {
       thread->quit();
       thread->wait();
   }
   ```

5. **在 finished 槽中调用 deleteLater()**
   ```cpp
   connect(watcher, &QFutureWatcher<int>::finished, [watcher]() {
       watcher->deleteLater();
   });
   ```

---

## QThread vs QtConcurrent

### 对比表

| 特性 | QThread | QtConcurrent |
|------|---------|--------------|
| **代码复杂度** | 高（需要手动管理） | 低（一行代码启动） |
| **线程管理** | 手动创建和销毁 | 自动线程池 |
| **适用场景** | 长期后台任务 | 短期计算任务 |
| **线程间通信** | 信号槽 | QFuture |
| **学习曲线** | 陡峭 | 平缓 |
| **灵活性** | 高（完全控制） | 中（自动管理） |
| **性能** | 可优化 | 自动优化 |

### 使用场景

**使用 QThread 的场景**：
- ✅ 长期运行的后台服务
- ✅ 需要复杂的线程间通信
- ✅ 需要精确控制线程生命周期
- ✅ 需要频繁与 UI 交互

**使用 QtConcurrent 的场景**：
- ✅ 批量数据处理（如图片处理、文件转换）
- ✅ 短期计算任务（如排序、搜索）
- ✅ 不需要频繁与 UI 交互
- ✅ 想要简化代码

---

## 🎯 快速参考

### QThread 基础（3步）
```cpp
Worker *worker = new Worker();
QThread *thread = new QThread(this);
worker->moveToThread(thread);
connect(button, &QPushButton::clicked, worker, &Worker::doWork);
thread->start();
```

### QtConcurrent::run()（3步）
```cpp
QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);
connect(watcher, &QFutureWatcher<int>::finished, [watcher]() { /* 处理 */ });
watcher->setFuture(QtConcurrent::run([]() { return 42; }));
```

### QMutex 保护共享数据（1步）
```cpp
QMutexLocker locker(&mutex);
sharedData++;
```

### QReadWriteLock 读写分离（2步）
```cpp
rwLock.lockForRead();   // 读
rwLock.lockForWrite();  // 写
```

---

## 📚 相关文件

- `32_thread_basics.cpp` - QThread 基础
- `33_thread_practice.cpp` - 多线程实战（斐波那契计算器）
- `34_thread_sync.cpp` - 线程同步基础
- `35_qtconcurrent.cpp` - QtConcurrent 高级并发
- `36_file_search.cpp` - 多线程文件搜索工具

---

**阶段10完成！接下来规划 Qt 综合项目** 🚀
