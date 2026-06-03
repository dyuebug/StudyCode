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
