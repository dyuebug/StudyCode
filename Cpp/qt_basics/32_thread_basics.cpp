// ============================================
// QThread 基础 - 超详细注释版
// 日期：2026-05-30（第49天）
// 难度：⭐⭐⭐ 中级
// ============================================

/*
【核心概念】Qt 多线程编程基础

是什么？
- QThread 是 Qt 提供的线程类，用于在后台执行耗时任务
- 多线程可以让 UI 保持响应，避免界面卡顿

为什么需要？
- 耗时操作（网络请求、文件读写、复杂计算）会阻塞 UI 线程
- 用户无法操作界面，体验很差
- 多线程可以将耗时任务放到后台执行

核心特点：
- Qt 的线程间通信通过信号槽实现（自动线程安全）
- moveToThread 是 Qt 推荐的多线程模式
- 永远不要在工作线程中直接操作 UI 控件
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
#include <QDebug>
#include <QTime>

/*
────────────────────────────────────────────
1.1 为什么需要多线程？
────────────────────────────────────────────

【问题场景】
假设你的应用需要执行一个耗时 5 秒的计算任务：

❌ 单线程（主线程）执行：
- 用户点击按钮 → 开始计算
- 计算过程中 UI 完全卡死（无法点击、无法移动窗口）
- 5 秒后计算完成，UI 恢复响应
- 用户体验：非常糟糕！

✅ 多线程（工作线程）执行：
- 用户点击按钮 → 启动工作线程
- 工作线程在后台计算，主线程继续处理 UI 事件
- 用户可以继续操作界面（点击其他按钮、移动窗口）
- 5 秒后工作线程完成，通过信号通知主线程
- 用户体验：流畅！

【Qt 的线程模型】
- 主线程（UI 线程）：负责处理 UI 事件、绘制界面
- 工作线程：负责执行耗时任务（计算、I/O、网络）
- 线程间通信：通过信号槽（Qt 自动处理线程安全）

【黄金规则】⭐⭐⭐⭐⭐
1. 永远不要在工作线程中直接操作 UI 控件
2. 工作线程通过信号通知主线程，由主线程更新 UI
3. 信号槽跨线程时，Qt 自动使用队列连接（线程安全）
*/

/*
────────────────────────────────────────────
1.2 QThread 基础概念
────────────────────────────────────────────

【QThread 类】
- 作用：表示一个线程
- 继承：QObject
- 核心方法：
  * start()：启动线程
  * quit()：请求线程退出
  * wait()：等待线程结束
  * isRunning()：检查线程是否运行中

【线程生命周期】
1. 创建：QThread *thread = new QThread();
2. 启动：thread->start();
3. 运行：线程执行任务
4. 退出：thread->quit();
5. 等待：thread->wait();
6. 销毁：thread->deleteLater();

【两种多线程模式】
1. 继承 QThread 重写 run()（不推荐）
2. moveToThread 模式（推荐）⭐⭐⭐⭐⭐
*/

/*
────────────────────────────────────────────
1.3 moveToThread 模式（推荐）⭐⭐⭐⭐⭐
────────────────────────────────────────────

【为什么推荐 moveToThread？】
- 更符合 Qt 的对象模型
- 更容易管理对象生命周期
- 更灵活（一个对象可以在多个线程间移动）
- 更容易测试和维护

【moveToThread 工作流程】
1. 创建工作对象（继承 QObject）
2. 创建线程对象（QThread）
3. 将工作对象移动到线程：worker->moveToThread(thread)
4. 连接信号槽：
   - 主线程信号 → 工作对象槽（触发任务）
   - 工作对象信号 → 主线程槽（返回结果）
5. 启动线程：thread->start()
6. 触发任务：emit startWork()

【关键点】
- 工作对象的槽函数在工作线程中执行
- 工作对象的信号在工作线程中发射
- 主线程的槽函数在主线程中执行
- Qt 自动处理跨线程信号槽（队列连接）
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 示例1：工作对象（Worker）
// --------------------------------------------

/*
【Worker 类】
- 作用：封装耗时任务的逻辑
- 继承：QObject（必须，才能使用信号槽）
- 特点：不继承 QThread，通过 moveToThread 移动到工作线程
*/

class Worker : public QObject {
    Q_OBJECT  // 必须：启用信号槽机制

public:
    // 构造函数：初始化工作对象
    // 参数：parent - 父对象（可选）
    explicit Worker(QObject *parent = nullptr) : QObject(parent) {}

signals:
    // 信号：任务完成
    // 参数：result - 计算结果
    // 说明：在工作线程中发射，主线程接收
    void finished(int result);

    // 信号：进度更新
    // 参数：percent - 完成百分比（0-100）
    // 说明：用于实时更新进度条
    void progress(int percent);

public slots:
    // 槽函数：执行耗时任务
    // 说明：此函数在工作线程中执行
    // 注意：不能直接操作 UI 控件！
    void doWork() {
        qDebug() << "工作线程 ID：" << QThread::currentThreadId();
        qDebug() << "开始计算...";

        int result = 0;

        // 模拟耗时计算（计算 1 到 1000000 的和）
        for (int i = 1; i <= 1000000; ++i) {
            result += i;

            // 每完成 10000 次，发射进度信号
            if (i % 10000 == 0) {
                int percent = (i * 100) / 1000000;
                emit progress(percent);  // 通知主线程更新进度

                // 模拟耗时操作
                QThread::msleep(1);  // 休眠 1 毫秒
            }
        }

        qDebug() << "计算完成！结果：" << result;

        // 发射完成信号，将结果传递给主线程
        emit finished(result);
    }
};

// --------------------------------------------
// 2.2 示例2：主窗口（使用 moveToThread）
// --------------------------------------------

class ThreadDemo : public QWidget {
    Q_OBJECT

public:
    ThreadDemo(QWidget *parent = nullptr) : QWidget(parent) {
        qDebug() << "主线程 ID：" << QThread::currentThreadId();

        // 创建 UI 控件
        startButton = new QPushButton("开始计算", this);
        statusLabel = new QLabel("状态：等待开始", this);
        resultLabel = new QLabel("结果：-", this);

        // 布局
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(startButton);
        layout->addWidget(statusLabel);
        layout->addWidget(resultLabel);

        // ========================================
        // 多线程设置（moveToThread 模式）⭐⭐⭐⭐⭐
        // ========================================

        // 步骤1：创建工作对象
        // 说明：Worker 对象封装了耗时任务的逻辑
        // 注意：不要设置 parent，否则无法移动到其他线程
        worker = new Worker();

        // 步骤2：创建线程对象
        // 说明：QThread 对象表示一个线程
        // 参数：this - 父对象（主窗口销毁时自动销毁线程）
        thread = new QThread(this);

        // 步骤3：将工作对象移动到线程
        // 说明：worker 的槽函数将在 thread 中执行
        // 关键：moveToThread 是 Qt 推荐的多线程模式
        worker->moveToThread(thread);

        // 步骤4：连接信号槽
        // 4.1 按钮点击 → 触发工作对象的槽函数
        // 说明：startButton 在主线程，worker->doWork() 在工作线程
        // Qt 自动使用队列连接（QueuedConnection），线程安全
        connect(startButton, &QPushButton::clicked, worker, &Worker::doWork);

        // 4.2 工作对象完成 → 更新 UI
        // 说明：worker->finished 在工作线程发射，updateResult 在主线程执行
        // Qt 自动处理跨线程信号槽
        connect(worker, &Worker::finished, this, &ThreadDemo::updateResult);

        // 4.3 工作对象进度 → 更新状态
        // 说明：实时更新进度信息
        connect(worker, &Worker::progress, this, &ThreadDemo::updateProgress);

        // 4.4 线程结束 → 清理资源
        // 说明：线程退出时自动删除工作对象
        connect(thread, &QThread::finished, worker, &QObject::deleteLater);

        // 步骤5：启动线程
        // 说明：线程启动后进入事件循环，等待信号触发任务
        // 注意：此时还没有执行任务，只是线程准备就绪
        thread->start();

        qDebug() << "线程已启动，等待任务...";
    }

    ~ThreadDemo() {
        // 清理线程
        // 步骤1：请求线程退出
        thread->quit();

        // 步骤2：等待线程结束
        thread->wait();

        qDebug() << "线程已清理";
    }

private slots:
    // 槽函数：更新结果
    // 参数：result - 计算结果
    // 说明：在主线程中执行，可以安全操作 UI
    void updateResult(int result) {
        qDebug() << "主线程接收结果：" << result;
        resultLabel->setText(QString("结果：%1").arg(result));
        statusLabel->setText("状态：计算完成！");
        startButton->setEnabled(true);
    }

    // 槽函数：更新进度
    // 参数：percent - 完成百分比
    // 说明：在主线程中执行，实时更新状态
    void updateProgress(int percent) {
        statusLabel->setText(QString("状态：计算中... %1%").arg(percent));

        // 第一次更新时禁用按钮
        if (percent == 0) {
            startButton->setEnabled(false);
        }
    }

private:
    QPushButton *startButton;
    QLabel *statusLabel;
    QLabel *resultLabel;

    Worker *worker;      // 工作对象
    QThread *thread;     // 线程对象
};

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
==============================================
🔍 深入理解：线程间信号槽的连接类型（选学）
==============================================

【Qt 的信号槽连接类型】
Qt 提供了 5 种连接类型（ConnectionType）：

1. AutoConnection（默认）⭐⭐⭐⭐⭐
   - Qt 自动选择连接类型
   - 同线程：DirectConnection
   - 跨线程：QueuedConnection
   - 推荐：99% 的情况使用默认即可

2. DirectConnection
   - 直接调用：信号发射后立即执行槽函数
   - 槽函数在发射信号的线程中执行
   - 风险：跨线程时不安全（可能同时访问共享数据）

3. QueuedConnection
   - 队列调用：信号发射后将槽函数放入接收者线程的事件队列
   - 槽函数在接收者线程中执行
   - 安全：跨线程时自动使用此类型

4. BlockingQueuedConnection
   - 阻塞队列调用：类似 QueuedConnection，但发射者线程会阻塞等待槽函数执行完成
   - 风险：可能导致死锁

5. UniqueConnection
   - 唯一连接：防止重复连接
   - 可以与其他类型组合使用（如 AutoConnection | UniqueConnection）

【示例：手动指定连接类型】
connect(worker, &Worker::finished, this, &ThreadDemo::updateResult, Qt::QueuedConnection);

【何时需要手动指定？】
- 99% 的情况使用默认（AutoConnection）即可
- 特殊场景：需要强制使用队列连接或直接连接

==============================================
*/

/*
==============================================
🔍 深入理解：moveToThread 的工作原理（选学）
==============================================

【QObject 的线程亲和性（Thread Affinity）】
- 每个 QObject 都有一个"所属线程"（thread affinity）
- 对象的槽函数在其所属线程中执行
- 对象的信号在发射信号的线程中发射

【moveToThread 做了什么？】
1. 改变对象的线程亲和性
2. 将对象的事件循环移动到目标线程
3. 对象的槽函数将在目标线程中执行

【示例】
Worker *worker = new Worker();  // 创建在主线程
worker->moveToThread(thread);   // 移动到工作线程

// 现在：
// - worker 对象本身仍在主线程创建
// - worker 的槽函数在工作线程中执行
// - worker 的信号在工作线程中发射

【注意事项】
1. 不能移动有父对象的对象
   Worker *worker = new Worker(this);  // ❌ 有父对象
   worker->moveToThread(thread);       // 失败！

2. 不能移动正在运行的对象
   worker->doWork();                   // 正在执行
   worker->moveToThread(thread);       // ❌ 失败！

3. 不能移动 UI 控件
   QPushButton *btn = new QPushButton();
   btn->moveToThread(thread);          // ❌ UI 控件只能在主线程

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
────────────────────────────────────────────
4.1 错误1：在工作线程中直接操作 UI
────────────────────────────────────────────

❌ 错误代码：
class Worker : public QObject {
public slots:
    void doWork() {
        // ❌ 错误：在工作线程中直接操作 UI
        label->setText("计算中...");  // 崩溃或未定义行为！
    }
private:
    QLabel *label;
};

结果：
- 程序可能崩溃
- UI 显示异常
- 未定义行为

原因：
- Qt 的 UI 控件不是线程安全的
- 只能在主线程中操作 UI

✅ 正确代码：
class Worker : public QObject {
    Q_OBJECT
signals:
    void statusChanged(const QString &status);

public slots:
    void doWork() {
        // ✅ 正确：通过信号通知主线程
        emit statusChanged("计算中...");
    }
};

// 主线程中连接信号
connect(worker, &Worker::statusChanged, label, &QLabel::setText);

记忆技巧：
- 工作线程只负责计算，不碰 UI
- 通过信号槽通知主线程更新 UI

────────────────────────────────────────────
4.2 错误2：moveToThread 时设置了父对象
────────────────────────────────────────────

❌ 错误代码：
Worker *worker = new Worker(this);  // 设置了父对象
worker->moveToThread(thread);       // 失败！

结果：
- moveToThread 失败
- 控制台警告：QObject::moveToThread: Cannot move objects with a parent

原因：
- 有父对象的对象不能移动到其他线程
- Qt 的对象树机制要求父子对象在同一线程

✅ 正确代码：
Worker *worker = new Worker();      // 不设置父对象
worker->moveToThread(thread);       // 成功

// 清理：连接线程结束信号
connect(thread, &QThread::finished, worker, &QObject::deleteLater);

记忆技巧：
- moveToThread 的对象不能有父对象
- 通过信号槽自动清理资源

────────────────────────────────────────────
4.3 错误3：忘记启动线程
────────────────────────────────────────────

❌ 错误代码：
QThread *thread = new QThread(this);
worker->moveToThread(thread);
// 忘记调用 thread->start()

connect(button, &QPushButton::clicked, worker, &Worker::doWork);

结果：
- 点击按钮后没有反应
- 任务不执行

原因：
- 线程没有启动，事件循环没有运行
- 信号无法触发槽函数

✅ 正确代码：
QThread *thread = new QThread(this);
worker->moveToThread(thread);
thread->start();  // ✅ 启动线程

connect(button, &QPushButton::clicked, worker, &Worker::doWork);

记忆技巧：
- moveToThread 后必须 start()
- 线程启动后才能处理信号

────────────────────────────────────────────
4.4 错误4：忘记清理线程
────────────────────────────────────────────

❌ 错误代码：
~ThreadDemo() {
    // 忘记清理线程
}

结果：
- 线程继续运行
- 可能导致崩溃（访问已销毁的对象）
- 内存泄漏

原因：
- 线程对象不会自动停止
- 需要手动请求退出并等待

✅ 正确代码：
~ThreadDemo() {
    thread->quit();   // 请求线程退出
    thread->wait();   // 等待线程结束
}

记忆技巧：
- 析构函数中必须 quit() + wait()
- 确保线程完全停止后再销毁对象

────────────────────────────────────────────
4.5 错误5：在工作线程中使用 QTimer
────────────────────────────────────────────

❌ 错误代码：
class Worker : public QObject {
public:
    Worker() {
        timer = new QTimer(this);  // 在主线程创建
        connect(timer, &QTimer::timeout, this, &Worker::doWork);
    }

    void start() {
        timer->start(1000);  // ❌ 定时器在主线程，槽函数在工作线程
    }
private:
    QTimer *timer;
};

结果：
- 定时器不工作
- 或者槽函数在错误的线程中执行

原因：
- QTimer 在创建它的线程中工作
- Worker 移动到工作线程后，timer 仍在主线程

✅ 正确代码：
class Worker : public QObject {
public slots:
    void start() {
        // ✅ 在工作线程中创建定时器
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &Worker::doWork);
        timer->start(1000);
    }
};

记忆技巧：
- 在工作线程的槽函数中创建 QTimer
- 确保定时器和槽函数在同一线程

────────────────────────────────────────────
*/

// ============================================
// 第五部分：函数卡片速查
// ============================================

/*
【QThread 核心方法】

方法：start()
作用：启动线程
语法：void start(Priority priority = InheritPriority)
参数：priority - 线程优先级（可选）
返回：无
示例：thread->start();

────────────────────────────────────

方法：quit()
作用：请求线程退出事件循环
语法：void quit()
参数：无
返回：无
示例：thread->quit();

────────────────────────────────────

方法：wait()
作用：阻塞当前线程，等待此线程结束
语法：bool wait(unsigned long time = ULONG_MAX)
参数：time - 最大等待时间（毫秒），默认无限等待
返回：true - 线程已结束，false - 超时
示例：thread->wait();

────────────────────────────────────

方法：isRunning()
作用：检查线程是否正在运行
语法：bool isRunning() const
参数：无
返回：true - 运行中，false - 未运行
示例：if (thread->isRunning()) { ... }

────────────────────────────────────

方法：currentThreadId()
作用：获取当前线程的 ID
语法：static Qt::HANDLE currentThreadId()
参数：无
返回：当前线程的 ID
示例：qDebug() << QThread::currentThreadId();

────────────────────────────────────

【QObject 线程相关方法】

方法：moveToThread()
作用：将对象移动到指定线程
语法：void moveToThread(QThread *targetThread)
参数：targetThread - 目标线程
返回：无
示例：worker->moveToThread(thread);

────────────────────────────────────

方法：thread()
作用：获取对象所属的线程
语法：QThread *thread() const
参数：无
返回：对象所属的线程
示例：QThread *t = worker->thread();

────────────────────────────────────
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
练习1：后台文件读取
要求：
- 创建一个工作对象，在后台读取大文件
- 读取过程中实时更新进度（已读取字节数 / 总字节数）
- 读取完成后显示文件内容（前 100 行）
- UI 在读取过程中保持响应

提示：
- 使用 QFile 读取文件
- 使用 QTextStream 逐行读取
- 每读取 1000 行发射一次进度信号

────────────────────────────────────

练习2：后台图片处理
要求：
- 创建一个工作对象，在后台对图片应用滤镜（如灰度化）
- 处理过程中实时更新进度
- 处理完成后显示结果图片
- 提供"取消"按钮中断处理

提示：
- 使用 QImage 加载和处理图片
- 逐像素处理，定期检查取消标志
- 使用 QLabel 显示图片

────────────────────────────────────

练习3：多个工作线程
要求：
- 创建 3 个工作线程，同时执行不同的计算任务
- 每个线程独立更新自己的进度条
- 所有线程完成后显示总结信息

提示：
- 创建 3 个 Worker 对象和 3 个 QThread 对象
- 每个 Worker 连接到对应的进度条
- 使用计数器跟踪完成的线程数量

────────────────────────────────────
*/

// ============================================
// 主函数
// ============================================

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ThreadDemo demo;
    demo.setWindowTitle("QThread 基础示例");
    demo.resize(400, 150);
    demo.show();

    return app.exec();
}

#include "32_thread_basics.moc"  // 必须：包含 MOC 生成的代码
