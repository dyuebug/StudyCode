// ============================================
// 多线程实战：后台计算斐波那契数列
// 日期：2026-05-30（第49天）
// 难度：⭐⭐⭐ 中级
// ============================================

/*
【项目目标】
创建一个应用，在后台计算斐波那契数列的第 N 项：
- 用户输入 N（如 45）
- 点击"开始计算"按钮，启动后台计算
- 计算过程中显示进度条和状态
- UI 保持响应（可以移动窗口、点击其他按钮）
- 提供"取消"按钮中断计算
- 计算完成后显示结果

【技术要点】
- moveToThread 模式
- 线程间信号槽通信
- 进度更新
- 取消机制
- UI 线程安全
*/

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThread>
#include <QDebug>

// ============================================
// 工作对象：斐波那契计算器
// ============================================

class FibonacciWorker : public QObject {
    Q_OBJECT

public:
    explicit FibonacciWorker(QObject *parent = nullptr)
        : QObject(parent), shouldStop(false) {}

signals:
    // 信号：计算完成
    // 参数：result - 计算结果
    void finished(qint64 result);

    // 信号：进度更新
    // 参数：current - 当前计算到第几项
    //       total - 总共需要计算的项数
    void progress(int current, int total);

    // 信号：计算被取消
    void cancelled();

public slots:
    // 槽函数：开始计算
    // 参数：n - 计算斐波那契数列的第 n 项
    // 说明：此函数在工作线程中执行
    void calculate(int n) {
        qDebug() << "工作线程开始计算 Fib(" << n << ")";

        // 重置取消标志
        shouldStop = false;

        // 使用迭代法计算（避免递归栈溢出）
        // Fib(0) = 0, Fib(1) = 1
        // Fib(n) = Fib(n-1) + Fib(n-2)

        if (n == 0) {
            emit finished(0);
            return;
        }
        if (n == 1) {
            emit finished(1);
            return;
        }

        qint64 prev = 0;  // Fib(0)
        qint64 curr = 1;  // Fib(1)
        qint64 next = 0;

        // 从第 2 项开始计算到第 n 项
        for (int i = 2; i <= n; ++i) {
            // 检查是否需要取消
            if (shouldStop) {
                qDebug() << "计算被取消";
                emit cancelled();
                return;
            }

            // 计算下一项
            next = prev + curr;
            prev = curr;
            curr = next;

            // 每计算 100 项更新一次进度
            if (i % 100 == 0) {
                emit progress(i, n);
            }

            // 模拟耗时操作（实际计算很快，这里故意减慢）
            QThread::msleep(10);  // 休眠 10 毫秒
        }

        // 发射最终进度
        emit progress(n, n);

        qDebug() << "计算完成：Fib(" << n << ") =" << curr;
        emit finished(curr);
    }

    // 槽函数：取消计算
    // 说明：设置取消标志，工作线程会检查并停止
    void cancel() {
        qDebug() << "收到取消请求";
        shouldStop = true;
    }

private:
    bool shouldStop;  // 取消标志（原子操作更安全，这里简化）
};

// ============================================
// 主窗口
// ============================================

class FibonacciCalculator : public QWidget {
    Q_OBJECT

public:
    FibonacciCalculator(QWidget *parent = nullptr) : QWidget(parent) {
        // 创建 UI 控件
        QLabel *inputLabel = new QLabel("计算斐波那契数列的第 N 项：", this);

        inputEdit = new QLineEdit(this);
        inputEdit->setText("45");  // 默认值
        inputEdit->setPlaceholderText("输入一个正整数（如 45）");

        startButton = new QPushButton("开始计算", this);
        cancelButton = new QPushButton("取消", this);
        cancelButton->setEnabled(false);  // 初始禁用

        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);

        statusLabel = new QLabel("状态：等待开始", this);
        resultLabel = new QLabel("结果：-", this);

        // 布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(inputLabel);
        mainLayout->addWidget(inputEdit);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(startButton);
        buttonLayout->addWidget(cancelButton);
        mainLayout->addLayout(buttonLayout);

        mainLayout->addWidget(progressBar);
        mainLayout->addWidget(statusLabel);
        mainLayout->addWidget(resultLabel);

        // ========================================
        // 多线程设置
        // ========================================

        // 创建工作对象（不设置父对象）
        worker = new FibonacciWorker();

        // 创建线程对象
        thread = new QThread(this);

        // 将工作对象移动到线程
        worker->moveToThread(thread);

        // 连接信号槽
        // 1. 开始按钮 → 触发计算
        connect(startButton, &QPushButton::clicked, this, &FibonacciCalculator::onStartClicked);

        // 2. 取消按钮 → 取消计算
        connect(cancelButton, &QPushButton::clicked, worker, &FibonacciWorker::cancel);

        // 3. 工作对象完成 → 更新结果
        connect(worker, &FibonacciWorker::finished, this, &FibonacciCalculator::onFinished);

        // 4. 工作对象进度 → 更新进度条
        connect(worker, &FibonacciWorker::progress, this, &FibonacciCalculator::onProgress);

        // 5. 工作对象取消 → 更新状态
        connect(worker, &FibonacciWorker::cancelled, this, &FibonacciCalculator::onCancelled);

        // 6. 线程结束 → 清理工作对象
        connect(thread, &QThread::finished, worker, &QObject::deleteLater);

        // 启动线程
        thread->start();

        qDebug() << "应用启动，线程已准备就绪";
    }

    ~FibonacciCalculator() {
        // 清理线程
        thread->quit();
        thread->wait();
        qDebug() << "线程已清理";
    }

private slots:
    // 槽函数：开始按钮点击
    void onStartClicked() {
        // 获取输入
        bool ok;
        int n = inputEdit->text().toInt(&ok);

        // 验证输入
        if (!ok || n < 0) {
            statusLabel->setText("状态：输入无效！请输入正整数");
            return;
        }

        if (n > 100) {
            statusLabel->setText("状态：数值过大！请输入 0-100 之间的数");
            return;
        }

        // 更新 UI 状态
        startButton->setEnabled(false);
        cancelButton->setEnabled(true);
        inputEdit->setEnabled(false);
        progressBar->setValue(0);
        statusLabel->setText("状态：计算中...");
        resultLabel->setText("结果：计算中...");

        // 触发工作对象的计算槽函数
        // 注意：使用 QMetaObject::invokeMethod 传递参数
        QMetaObject::invokeMethod(worker, "calculate", Qt::QueuedConnection, Q_ARG(int, n));

        qDebug() << "触发计算任务：Fib(" << n << ")";
    }

    // 槽函数：计算完成
    // 参数：result - 计算结果
    void onFinished(qint64 result) {
        qDebug() << "主线程接收结果：" << result;

        // 更新 UI
        resultLabel->setText(QString("结果：%1").arg(result));
        statusLabel->setText("状态：计算完成！");
        progressBar->setValue(100);

        // 恢复按钮状态
        startButton->setEnabled(true);
        cancelButton->setEnabled(false);
        inputEdit->setEnabled(true);
    }

    // 槽函数：进度更新
    // 参数：current - 当前进度
    //       total - 总进度
    void onProgress(int current, int total) {
        int percent = (current * 100) / total;
        progressBar->setValue(percent);
        statusLabel->setText(QString("状态：计算中... %1/%2 (%3%)")
                             .arg(current).arg(total).arg(percent));
    }

    // 槽函数：计算被取消
    void onCancelled() {
        qDebug() << "主线程收到取消通知";

        // 更新 UI
        statusLabel->setText("状态：已取消");
        resultLabel->setText("结果：已取消");

        // 恢复按钮状态
        startButton->setEnabled(true);
        cancelButton->setEnabled(false);
        inputEdit->setEnabled(true);
    }

private:
    QLineEdit *inputEdit;
    QPushButton *startButton;
    QPushButton *cancelButton;
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QLabel *resultLabel;

    FibonacciWorker *worker;
    QThread *thread;
};

// ============================================
// 主函数
// ============================================

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    FibonacciCalculator calculator;
    calculator.setWindowTitle("斐波那契计算器（多线程版）");
    calculator.resize(400, 250);
    calculator.show();

    return app.exec();
}

// ============================================
// 函数卡片速查
// ============================================
/*
【函数卡片：QThread::start()】

语法：void QThread::start(Priority priority = InheritPriority)
作用：启动线程，调用 run() 方法（moveToThread 模式下执行事件循环）
参数：priority — 线程优先级，通常用默认值

常用配合方法：
- quit()   → 请求线程事件循环退出
- wait()   → 等待线程完全结束（阻塞调用方）
- isRunning() → 检查线程是否在运行

示例（moveToThread 模式）：
auto *worker = new FibWorker();
auto *thread = new QThread(this);
worker->moveToThread(thread);
connect(thread, &QThread::started,   worker, &FibWorker::calculate);
connect(worker, &FibWorker::finished, thread, &QThread::quit);
connect(thread, &QThread::finished,  worker, &QObject::deleteLater);
thread->start();

────────────────────────────────────────────────────────────

【函数卡片：QProgressBar::setValue()】

语法：void setValue(int value)
作用：设置进度条当前值（需配合 setRange 设置范围）

示例：
QProgressBar *bar = new QProgressBar();
bar->setRange(0, 100);   // 0%~100%
bar->setValue(0);         // 初始 0%
// 工作线程进度更新时：
emit progressUpdated(50);  // 通过信号跨线程更新
// 主线程槽：
connect(worker, &Worker::progressUpdated, bar, &QProgressBar::setValue);
*/

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
【错误1】在工作线程中直接操作 UI 控件

❌ 错误代码：
void FibWorker::calculate()
{
    for (int i = 0; i <= n; i++) {
        m_progressBar->setValue(i);  // 直接操作 UI！线程不安全，可能崩溃
    }
}

✅ 正确代码：
void FibWorker::calculate()
{
    for (int i = 0; i <= n; i++) {
        emit progressUpdated(i);  // 通过信号，Qt 自动用队列连接跨线程
    }
}
// 主线程的槽函数更新 UI
connect(worker, &FibWorker::progressUpdated, progressBar, &QProgressBar::setValue);

预防措施：工作线程永远只发信号，永远不直接访问 UI 对象。

────────────────────────────────────────────────────────────

【错误2】线程未结束就 delete worker

❌ 错误代码：
thread->quit();
delete worker;   // quit() 只是请求退出，线程可能还在运行！

✅ 正确代码：
thread->quit();
thread->wait();      // 等待线程真正结束
delete worker;       // 安全删除

// 或者用信号自动管理：
connect(thread, &QThread::finished, worker, &QObject::deleteLater);
connect(thread, &QThread::finished, thread, &QObject::deleteLater);

预防措施：quit() 后必须 wait()，或用 finished 信号触发 deleteLater()。

────────────────────────────────────────────────────────────

【错误3】取消标志未使用 atomic 导致数据竞争

❌ 错误代码：
class FibWorker {
    bool m_cancelled = false;  // 普通 bool，多线程访问不安全
public:
    void cancel() { m_cancelled = true; }  // 主线程写
    void run() {
        while (!m_cancelled) { ... }  // 工作线程读
    }
};

✅ 正确代码：
#include <atomic>
class FibWorker {
    std::atomic<bool> m_cancelled{false};  // 原子变量
public:
    void cancel() { m_cancelled = true; }
    void run() { while (!m_cancelled) { ... } }
};

预防措施：多线程共享的简单标志变量使用 std::atomic<bool>。

────────────────────────────────────────────────────────────

【错误4】没有设置 moveToThread 就启动线程

❌ 错误代码：
auto *worker = new FibWorker();
auto *thread = new QThread();
// 忘记 worker->moveToThread(thread)！
connect(thread, &QThread::started, worker, &FibWorker::calculate);
thread->start();
// worker 仍在主线程，calculate() 在主线程执行，UI 卡死

✅ 正确代码：
worker->moveToThread(thread);  // 必须！将 worker 迁移到工作线程
connect(thread, &QThread::started, worker, &FibWorker::calculate);
thread->start();

预防措施：moveToThread 在 start() 之前调用。

────────────────────────────────────────────────────────────

【错误5】窗口关闭时线程仍在运行导致崩溃

❌ 错误代码：
// 用户关闭窗口，MainWindow 析构
// 工作线程还在运行，访问已销毁的对象 → 崩溃

✅ 正确代码：
// 在 MainWindow 析构或 closeEvent 中：
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_thread->isRunning()) {
        m_worker->cancel();     // 请求停止
        m_thread->quit();
        m_thread->wait(3000);   // 最多等 3 秒
    }
    event->accept();
}

预防措施：重写 closeEvent，关闭前确保工作线程已停止。
*/

#include "33_thread_practice.moc"

/*
==============================================
🎯 项目总结
==============================================

【实现的功能】
✅ 后台计算斐波那契数列
✅ 实时进度更新
✅ UI 保持响应（可以移动窗口）
✅ 取消机制
✅ 输入验证

【多线程要点】
1. moveToThread 模式：
   - 工作对象不设置父对象
   - moveToThread 后启动线程
   - 通过信号槽触发任务

2. 线程间通信：
   - 主线程 → 工作线程：QMetaObject::invokeMethod
   - 工作线程 → 主线程：emit 信号
   - Qt 自动处理线程安全

3. 取消机制：
   - 使用布尔标志（shouldStop）
   - 工作线程定期检查标志
   - 主线程通过信号设置标志

4. UI 线程安全：
   - 工作线程不直接操作 UI
   - 通过信号通知主线程更新 UI
   - 主线程的槽函数更新 UI

【编译运行】
qmake -project "QT += widgets"
qmake
make
./33_thread_practice

【测试建议】
1. 输入 45，观察计算过程和进度更新
2. 计算过程中尝试移动窗口（UI 不卡顿）
3. 点击"取消"按钮，观察计算中断
4. 输入无效值（如 -1、abc），观察错误提示
5. 输入大值（如 100），观察长时间计算

【扩展练习】
1. 添加"暂停/恢复"功能
2. 支持计算多个数值（批量计算）
3. 显示计算耗时
4. 保存计算历史记录
5. 使用 QFutureWatcher 简化代码

==============================================
*/

// ============================================
// 练习题
// ============================================
/*
1. 添加"暂停/恢复"功能
   - 在取消按钮旁添加"暂停"按钮
   - 工作线程检查一个 m_paused 标志，为 true 时调用 QThread::msleep(100) 并循环等待
   - 恢复时将 m_paused 改回 false
   提示：用 std::atomic<bool> m_paused；注意暂停期间仍要检查取消标志

2. 批量计算（并发多个斐波那契）
   - 允许用户输入多个数值（逗号分隔）
   - 为每个数值启动一个独立工作线程
   - 每个线程完成后追加结果到结果列表
   提示：用 QList<QThread*> 管理多个线程；注意线程数上限

3. 计算耗时统计
   - 记录每次计算的开始和结束时间
   - 计算完成后在结果旁显示"耗时：XXX ms"
   提示：QElapsedTimer timer; timer.start(); ... timer.elapsed()

4. 使用 QFutureWatcher 简化代码
   - 将当前 QThread + Worker 的实现替换为 QtConcurrent::run()
   - 取消时调用 future.cancel()
   - 对比两种方式的代码量和可读性差异
*/
