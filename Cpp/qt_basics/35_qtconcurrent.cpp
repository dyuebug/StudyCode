// ============================================
// QtConcurrent 高级并发 - 超详细注释版
// 日期：2026-05-31（第50天）
// 难度：⭐⭐⭐ 中级
// ============================================

/*
【核心概念】QtConcurrent 高级并发

是什么？
- QtConcurrent 是 Qt 提供的高级并发框架
- 比 QThread 更简单，自动管理线程池
- 提供 map、filter、run 等高级函数

为什么需要？
- QThread 需要手动管理线程生命周期，代码复杂
- QtConcurrent 自动管理线程，代码简洁
- 适合大多数并发场景

核心特点：
- 自动线程池管理（无需手动创建线程）
- QFuture 表示异步任务的结果
- QFutureWatcher 监听任务完成
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QDebug>
#include <QTime>

/*
────────────────────────────────────────────
1.1 QtConcurrent 概述 ⭐⭐⭐⭐⭐
────────────────────────────────────────────

【QtConcurrent 是什么？】
QtConcurrent 是 Qt 提供的高级并发框架，简化多线程编程。

【核心函数】
1. QtConcurrent::run() - 在线程池中运行函数
2. QtConcurrent::map() - 并行映射（对每个元素应用函数）
3. QtConcurrent::filter() - 并行过滤（筛选满足条件的元素）
4. QtConcurrent::mapped() - 并行映射并返回新容器
5. QtConcurrent::filtered() - 并行过滤并返回新容器

【QThread vs QtConcurrent】
QThread：
- 需要手动创建线程对象
- 需要手动管理线程生命周期
- 需要手动连接信号槽
- 适合：长期运行的后台任务

QtConcurrent：
- 自动管理线程池
- 代码简洁（一行代码启动任务）
- 自动返回结果（QFuture）
- 适合：短期计算任务、批量处理

【何时使用 QtConcurrent？】
✅ 适合：
- 批量数据处理（如图片处理、文件转换）
- 短期计算任务（如排序、搜索）
- 不需要频繁与 UI 交互

❌ 不适合：
- 长期运行的后台服务
- 需要复杂的线程间通信
- 需要精确控制线程生命周期
*/

/*
────────────────────────────────────────────
1.2 QFuture 异步任务结果 ⭐⭐⭐⭐⭐
────────────────────────────────────────────

【QFuture 类】
- 作用：表示异步任务的结果
- 类似：JavaScript 的 Promise、C++ 的 std::future
- 核心方法：
  * result()：获取结果（阻塞等待任务完成）
  * isFinished()：检查任务是否完成
  * cancel()：取消任务
  * progressValue()：获取进度

【使用模式】
// 启动异步任务
QFuture<int> future = QtConcurrent::run([]() {
    // 耗时计算
    return 42;
});

// 等待结果
int result = future.result();  // 阻塞等待
qDebug() << "结果：" << result;

【注意】
- result() 会阻塞当前线程，不要在主线程调用
- 使用 QFutureWatcher 监听完成事件（不阻塞）
*/

/*
────────────────────────────────────────────
1.3 QFutureWatcher 监听任务完成 ⭐⭐⭐⭐⭐
────────────────────────────────────────────

【QFutureWatcher 类】
- 作用：监听 QFuture 的状态变化
- 优点：不阻塞主线程，通过信号通知
- 核心信号：
  * finished()：任务完成
  * progressValueChanged(int)：进度更新
  * resultReadyAt(int)：部分结果就绪

【使用模式】
// 创建 Watcher
QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);

// 连接信号
connect(watcher, &QFutureWatcher<int>::finished, this, [watcher]() {
    int result = watcher->result();
    qDebug() << "任务完成，结果：" << result;
});

// 启动任务并监听
QFuture<int> future = QtConcurrent::run([]() { return 42; });
watcher->setFuture(future);

【关键点】
- Watcher 的信号在主线程中发射（可以安全更新 UI）
- 一个 Watcher 可以监听多个 Future（依次监听）
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 示例1：QtConcurrent::run() 基础
// --------------------------------------------

// 耗时计算函数
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

class ConcurrentDemo : public QWidget {
    Q_OBJECT

public:
    ConcurrentDemo(QWidget *parent = nullptr) : QWidget(parent) {
        // 创建 UI
        QPushButton *runButton = new QPushButton("运行异步任务", this);
        resultLabel = new QLabel("结果：-", this);
        statusLabel = new QLabel("状态：等待", this);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(runButton);
        layout->addWidget(statusLabel);
        layout->addWidget(resultLabel);

        // 连接按钮
        connect(runButton, &QPushButton::clicked, this, &ConcurrentDemo::runTask);
    }

private slots:
    void runTask() {
        statusLabel->setText("状态：计算中...");
        resultLabel->setText("结果：计算中...");

        // ========================================
        // QtConcurrent::run() 基础用法 ⭐⭐⭐⭐⭐
        // ========================================

        // 方法1：直接使用 result()（阻塞，不推荐在主线程）
        // QFuture<int> future = QtConcurrent::run(fibonacci, 40);
        // int result = future.result();  // ❌ 阻塞主线程

        // 方法2：使用 QFutureWatcher（推荐）✅
        QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);

        // 连接完成信号
        connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher]() {
            int result = watcher->result();
            resultLabel->setText(QString("结果：%1").arg(result));
            statusLabel->setText("状态：完成");

            // 清理 watcher
            watcher->deleteLater();
        });

        // 启动异步任务
        // 参数1：函数指针
        // 参数2：函数参数
        QFuture<int> future = QtConcurrent::run(fibonacci, 35);

        // 监听任务
        watcher->setFuture(future);

        qDebug() << "任务已启动，主线程继续执行";
    }

private:
    QLabel *resultLabel;
    QLabel *statusLabel;
};

// --------------------------------------------
// 2.2 示例2：QtConcurrent::run() 使用 Lambda
// --------------------------------------------

class LambdaDemo : public QWidget {
    Q_OBJECT

public:
    LambdaDemo(QWidget *parent = nullptr) : QWidget(parent) {
        QPushButton *runButton = new QPushButton("运行 Lambda 任务", this);
        resultLabel = new QLabel("结果：-", this);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(runButton);
        layout->addWidget(resultLabel);

        connect(runButton, &QPushButton::clicked, this, &LambdaDemo::runTask);
    }

private slots:
    void runTask() {
        resultLabel->setText("结果：计算中...");

        // ========================================
        // 使用 Lambda 表达式 ⭐⭐⭐⭐⭐
        // ========================================

        QFutureWatcher<QString> *watcher = new QFutureWatcher<QString>(this);

        connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() {
            QString result = watcher->result();
            resultLabel->setText("结果：" + result);
            watcher->deleteLater();
        });

        // 使用 Lambda 表达式
        QFuture<QString> future = QtConcurrent::run([]() {
            // 模拟耗时操作
            QThread::sleep(2);

            // 返回结果
            return QString("Lambda 任务完成！");
        });

        watcher->setFuture(future);
    }

private:
    QLabel *resultLabel;
};

// --------------------------------------------
// 2.3 示例3：QtConcurrent::mapped() 并行映射
// --------------------------------------------

// 映射函数：计算平方
int square(int x) {
    QThread::msleep(100);  // 模拟耗时
    return x * x;
}

class MappedDemo : public QWidget {
    Q_OBJECT

public:
    MappedDemo(QWidget *parent = nullptr) : QWidget(parent) {
        QPushButton *runButton = new QPushButton("并行映射", this);
        resultLabel = new QLabel("结果：-", this);
        progressBar = new QProgressBar(this);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(runButton);
        layout->addWidget(progressBar);
        layout->addWidget(resultLabel);

        connect(runButton, &QPushButton::clicked, this, &MappedDemo::runTask);
    }

private slots:
    void runTask() {
        resultLabel->setText("结果：计算中...");

        // ========================================
        // QtConcurrent::mapped() 并行映射 ⭐⭐⭐⭐
        // ========================================

        // 输入数据
        QVector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // 创建 Watcher
        QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);

        // 连接进度信号
        connect(watcher, &QFutureWatcher<int>::progressValueChanged,
                this, [this](int value) {
            progressBar->setValue(value);
        });

        // 连接完成信号
        connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher]() {
            // 获取所有结果
            QList<int> results = watcher->future().results();

            QString resultText = "结果：";
            for (int r : results) {
                resultText += QString::number(r) + " ";
            }
            resultLabel->setText(resultText);

            watcher->deleteLater();
        });

        // 并行映射：对每个元素应用 square 函数
        // 自动在线程池中并行执行
        QFuture<int> future = QtConcurrent::mapped(input, square);

        // 设置进度条范围
        progressBar->setRange(0, input.size());

        watcher->setFuture(future);
    }

private:
    QLabel *resultLabel;
    QProgressBar *progressBar;
};

// --------------------------------------------
// 2.4 示例4：QtConcurrent::filtered() 并行过滤
// --------------------------------------------

// 过滤函数：保留偶数
bool isEven(int x) {
    return x % 2 == 0;
}

class FilteredDemo : public QWidget {
    Q_OBJECT

public:
    FilteredDemo(QWidget *parent = nullptr) : QWidget(parent) {
        QPushButton *runButton = new QPushButton("并行过滤", this);
        resultLabel = new QLabel("结果：-", this);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(runButton);
        layout->addWidget(resultLabel);

        connect(runButton, &QPushButton::clicked, this, &FilteredDemo::runTask);
    }

private slots:
    void runTask() {
        resultLabel->setText("结果：计算中...");

        // ========================================
        // QtConcurrent::filtered() 并行过滤 ⭐⭐⭐⭐
        // ========================================

        // 输入数据
        QVector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);

        connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher]() {
            // 获取过滤后的结果
            QList<int> results = watcher->future().results();

            QString resultText = "偶数：";
            for (int r : results) {
                resultText += QString::number(r) + " ";
            }
            resultLabel->setText(resultText);

            watcher->deleteLater();
        });

        // 并行过滤：保留满足条件的元素
        QFuture<int> future = QtConcurrent::filtered(input, isEven);

        watcher->setFuture(future);
    }

private:
    QLabel *resultLabel;
};

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
==============================================
🔍 深入理解：线程池（选学）
==============================================

【什么是线程池？】
线程池是一组预先创建的线程，用于执行任务。

【为什么需要线程池？】
- 创建线程有开销（时间、内存）
- 线程池复用线程，减少开销
- 自动管理线程数量（避免创建过多线程）

【Qt 的线程池】
- QThreadPool：Qt 的全局线程池
- 默认线程数：CPU 核心数
- QtConcurrent 自动使用全局线程池

【查看线程池信息】
QThreadPool *pool = QThreadPool::globalInstance();
qDebug() << "最大线程数：" << pool->maxThreadCount();
qDebug() << "活跃线程数：" << pool->activeThreadCount();

【调整线程池大小】
pool->setMaxThreadCount(8);  // 设置最大线程数

==============================================
*/

/*
==============================================
🔍 深入理解：QFuture 的取消机制（选学）
==============================================

【取消任务】
QFuture<int> future = QtConcurrent::run([]() {
    for (int i = 0; i < 1000; ++i) {
        // 检查是否被取消
        if (QThread::currentThread()->isInterruptionRequested()) {
            return -1;  // 提前退出
        }
        // 执行任务
    }
    return 42;
});

// 取消任务
future.cancel();

【注意】
- cancel() 只是设置取消标志
- 任务需要主动检查标志并退出
- 不是所有任务都支持取消

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
────────────────────────────────────────────
4.1 错误1：在主线程调用 result()
────────────────────────────────────────────

❌ 错误代码：
void onButtonClicked() {
    QFuture<int> future = QtConcurrent::run([]() {
        QThread::sleep(5);  // 耗时 5 秒
        return 42;
    });

    int result = future.result();  // ❌ 阻塞主线程 5 秒，UI 卡死
    label->setText(QString::number(result));
}

结果：
- UI 卡死 5 秒
- 用户无法操作

原因：
- result() 会阻塞当前线程等待任务完成

✅ 正确代码：
void onButtonClicked() {
    QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);

    connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher]() {
        int result = watcher->result();  // ✅ 在信号槽中获取结果
        label->setText(QString::number(result));
        watcher->deleteLater();
    });

    QFuture<int> future = QtConcurrent::run([]() {
        QThread::sleep(5);
        return 42;
    });

    watcher->setFuture(future);
}

记忆技巧：
- 永远不要在主线程调用 result()
- 使用 QFutureWatcher 监听完成事件

────────────────────────────────────────────
4.2 错误2：忘记 deleteLater() 清理 Watcher
────────────────────────────────────────────

❌ 错误代码：
void runTask() {
    QFutureWatcher<int> *watcher = new QFutureWatcher<int>(this);

    connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher]() {
        int result = watcher->result();
        // 忘记 deleteLater()
    });

    watcher->setFuture(QtConcurrent::run([]() { return 42; }));
}

结果：
- 内存泄漏（每次调用都创建新 Watcher）

原因：
- Watcher 不会自动销毁

✅ 正确代码：
connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher]() {
    int result = watcher->result();
    watcher->deleteLater();  // ✅ 清理 Watcher
});

记忆技巧：
- 在 finished 槽中调用 deleteLater()

────────────────────────────────────────────
4.3 错误3：在 QtConcurrent 任务中操作 UI
────────────────────────────────────────────

❌ 错误代码：
QtConcurrent::run([this]() {
    for (int i = 0; i < 100; ++i) {
        label->setText(QString::number(i));  // ❌ 在工作线程操作 UI
    }
});

结果：
- 程序崩溃或 UI 显示异常

原因：
- Qt 的 UI 控件不是线程安全的

✅ 正确代码：
QtConcurrent::run([this]() {
    for (int i = 0; i < 100; ++i) {
        // 通过信号通知主线程更新 UI
        QMetaObject::invokeMethod(this, [this, i]() {
            label->setText(QString::number(i));
        }, Qt::QueuedConnection);
    }
});

记忆技巧：
- 工作线程不能直接操作 UI
- 使用 QMetaObject::invokeMethod 或信号槽

────────────────────────────────────────────
4.4 错误4：捕获局部变量导致悬空引用
────────────────────────────────────────────

❌ 错误代码：
void someFunction() {
    int localVar = 42;

    QtConcurrent::run([&localVar]() {  // ❌ 引用捕获局部变量
        QThread::sleep(2);
        return localVar * 2;  // localVar 可能已被销毁
    });
}  // localVar 销毁

结果：
- 访问已销毁的变量，未定义行为

原因：
- Lambda 引用捕获局部变量
- 函数返回后局部变量销毁

✅ 正确代码：
void someFunction() {
    int localVar = 42;

    QtConcurrent::run([localVar]() {  // ✅ 值捕获
        QThread::sleep(2);
        return localVar * 2;
    });
}

记忆技巧：
- 使用值捕获 [=] 或 [localVar]
- 避免引用捕获 [&]

────────────────────────────────────────────
*/

// ============================================
// 第五部分：函数卡片速查
// ============================================

/*
【QtConcurrent 核心函数】

函数：QtConcurrent::run()
作用：在线程池中运行函数
语法：QFuture<T> run(Function function, Args... args)
参数：function - 要执行的函数，args - 函数参数
返回：QFuture<T> - 异步任务结果
示例：QFuture<int> f = QtConcurrent::run(fibonacci, 40);

────────────────────────────────────

函数：QtConcurrent::mapped()
作用：并行映射（对每个元素应用函数）
语法：QFuture<T> mapped(Sequence &sequence, MapFunction function)
参数：sequence - 输入序列，function - 映射函数
返回：QFuture<T> - 映射后的结果
示例：QFuture<int> f = QtConcurrent::mapped(list, square);

────────────────────────────────────

函数：QtConcurrent::filtered()
作用：并行过滤（筛选满足条件的元素）
语法：QFuture<T> filtered(Sequence &sequence, FilterFunction function)
参数：sequence - 输入序列，function - 过滤函数
返回：QFuture<T> - 过滤后的结果
示例：QFuture<int> f = QtConcurrent::filtered(list, isEven);

────────────────────────────────────

【QFuture 核心方法】

方法：result()
作用：获取结果（阻塞等待任务完成）
语法：T result() const
参数：无
返回：任务的结果
示例：int r = future.result();

────────────────────────────────────

方法：isFinished()
作用：检查任务是否完成
语法：bool isFinished() const
参数：无
返回：true - 已完成，false - 未完成
示例：if (future.isFinished()) { ... }

────────────────────────────────────

方法：cancel()
作用：取消任务
语法：void cancel()
参数：无
返回：无
示例：future.cancel();

────────────────────────────────────

【QFutureWatcher 核心信号】

信号：finished()
作用：任务完成时发射
语法：void finished()
示例：connect(watcher, &QFutureWatcher<int>::finished, ...);

────────────────────────────────────

信号：progressValueChanged(int)
作用：进度更新时发射
语法：void progressValueChanged(int progressValue)
参数：progressValue - 当前进度值
示例：connect(watcher, &QFutureWatcher<int>::progressValueChanged, ...);

────────────────────────────────────
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
练习1：并行图片处理
要求：
- 使用 QtConcurrent::mapped() 对多张图片应用滤镜
- 显示处理进度
- 处理完成后显示结果

提示：
- 使用 QImage 加载和处理图片
- 映射函数：applyFilter(const QString &path)
- 使用 QFutureWatcher 监听进度

────────────────────────────────────

练习2：并行文件搜索
要求：
- 使用 QtConcurrent::run() 在多个目录中搜索文件
- 实时显示搜索结果
- 支持取消搜索

提示：
- 每个目录启动一个任务
- 使用 QFutureWatcher 监听每个任务
- 检查取消标志

────────────────────────────────────

练习3：性能对比
要求：
- 对比串行处理和并行处理的性能
- 任务：计算 1000 个数的平方
- 测量执行时间

提示：
- 串行：使用 for 循环
- 并行：使用 QtConcurrent::mapped()
- 使用 QElapsedTimer 测量时间

────────────────────────────────────
*/

// ============================================
// 主函数
// ============================================

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 显示线程池信息
    QThreadPool *pool = QThreadPool::globalInstance();
    qDebug() << "线程池最大线程数：" << pool->maxThreadCount();
    qDebug() << "CPU 核心数：" << QThread::idealThreadCount();

    // 创建演示窗口
    ConcurrentDemo demo;
    demo.setWindowTitle("QtConcurrent 演示");
    demo.resize(400, 150);
    demo.show();

    return app.exec();
}

#include "35_qtconcurrent.moc"

/*
==============================================
🎯 QtConcurrent 总结
==============================================

【核心优势】
✅ 代码简洁（一行代码启动任务）
✅ 自动管理线程池
✅ 自动返回结果（QFuture）
✅ 适合批量处理

【使用场景】
✅ 批量数据处理
✅ 短期计算任务
✅ 不需要复杂线程间通信

【最佳实践】
1. 使用 QFutureWatcher 监听完成（不要调用 result()）
2. 在 finished 槽中调用 deleteLater() 清理 Watcher
3. 不要在任务中直接操作 UI
4. 使用值捕获避免悬空引用

【QThread vs QtConcurrent】
- QThread：长期后台任务、复杂通信
- QtConcurrent：短期计算任务、批量处理

==============================================
*/

