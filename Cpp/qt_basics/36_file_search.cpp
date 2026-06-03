// ============================================
// 多线程文件搜索工具 - 实战项目
// 日期：2026-05-31（第50天）
// 难度：⭐⭐⭐⭐ 高级
// ============================================

/*
【项目目标】
创建一个多线程文件搜索工具：
- 在多个目录中并发搜索文件
- 支持文件名模糊匹配
- 实时显示搜索结果
- 显示搜索进度
- 支持取消搜索

【技术要点】
- QtConcurrent::run() 并发搜索
- QFutureWatcher 监听任务完成
- QListWidget 显示结果
- 线程安全的结果收集
*/

#include <QApplication>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QDir>
#include <QDirIterator>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

// ============================================
// 搜索结果结构
// ============================================

struct SearchResult {
    QString filePath;    // 文件路径
    qint64 fileSize;     // 文件大小（字节）
    QString directory;   // 所在目录

    SearchResult() : fileSize(0) {}
    SearchResult(const QString &path, qint64 size, const QString &dir)
        : filePath(path), fileSize(size), directory(dir) {}
};

// ============================================
// 搜索函数
// ============================================

// 在指定目录中搜索文件
// 参数：directory - 搜索目录
//       pattern - 文件名模式（如 "*.txt"）
// 返回：搜索到的文件列表
QList<SearchResult> searchInDirectory(const QString &directory, const QString &pattern) {
    QList<SearchResult> results;

    qDebug() << "开始搜索目录：" << directory << "模式：" << pattern;

    // 使用 QDirIterator 递归遍历目录
    // 参数1：目录路径
    // 参数2：文件名过滤器
    // 参数3：过滤标志（只要文件，不要目录）
    // 参数4：迭代标志（递归子目录）
    QDirIterator it(directory,
                    QStringList() << pattern,
                    QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        // 检查是否被取消
        if (QThread::currentThread()->isInterruptionRequested()) {
            qDebug() << "搜索被取消：" << directory;
            break;
        }

        QString filePath = it.next();
        QFileInfo fileInfo(filePath);

        // 添加到结果列表
        results.append(SearchResult(
            filePath,
            fileInfo.size(),
            directory
        ));

        // 模拟耗时操作（实际搜索很快，这里故意减慢）
        QThread::msleep(10);
    }

    qDebug() << "完成搜索目录：" << directory << "找到" << results.size() << "个文件";

    return results;
}

// ============================================
// 主窗口
// ============================================

class FileSearchTool : public QWidget {
    Q_OBJECT

public:
    FileSearchTool(QWidget *parent = nullptr) : QWidget(parent) {
        setupUI();
        connectSignals();
    }

    ~FileSearchTool() {
        // 取消所有正在运行的任务
        cancelSearch();
    }

private:
    void setupUI() {
        // 目录选择
        QLabel *dirLabel = new QLabel("搜索目录：", this);
        dirListWidget = new QListWidget(this);
        dirListWidget->setMaximumHeight(100);

        QPushButton *addDirButton = new QPushButton("添加目录", this);
        QPushButton *removeDirButton = new QPushButton("移除目录", this);

        QHBoxLayout *dirButtonLayout = new QHBoxLayout();
        dirButtonLayout->addWidget(addDirButton);
        dirButtonLayout->addWidget(removeDirButton);

        // 搜索模式
        QLabel *patternLabel = new QLabel("文件名模式：", this);
        patternEdit = new QLineEdit(this);
        patternEdit->setText("*.txt");
        patternEdit->setPlaceholderText("如：*.txt, *.cpp, *report*");

        // 搜索按钮
        searchButton = new QPushButton("开始搜索", this);
        cancelButton = new QPushButton("取消搜索", this);
        cancelButton->setEnabled(false);

        QHBoxLayout *searchButtonLayout = new QHBoxLayout();
        searchButtonLayout->addWidget(searchButton);
        searchButtonLayout->addWidget(cancelButton);

        // 进度条
        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);

        statusLabel = new QLabel("状态：等待开始", this);

        // 结果列表
        QLabel *resultLabel = new QLabel("搜索结果：", this);
        resultListWidget = new QListWidget(this);

        // 布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(dirLabel);
        mainLayout->addWidget(dirListWidget);
        mainLayout->addLayout(dirButtonLayout);
        mainLayout->addWidget(patternLabel);
        mainLayout->addWidget(patternEdit);
        mainLayout->addLayout(searchButtonLayout);
        mainLayout->addWidget(progressBar);
        mainLayout->addWidget(statusLabel);
        mainLayout->addWidget(resultLabel);
        mainLayout->addWidget(resultListWidget);

        // 连接按钮
        connect(addDirButton, &QPushButton::clicked, this, &FileSearchTool::addDirectory);
        connect(removeDirButton, &QPushButton::clicked, this, &FileSearchTool::removeDirectory);
    }

    void connectSignals() {
        connect(searchButton, &QPushButton::clicked, this, &FileSearchTool::startSearch);
        connect(cancelButton, &QPushButton::clicked, this, &FileSearchTool::cancelSearch);
    }

private slots:
    // 添加搜索目录
    void addDirectory() {
        QString dir = QFileDialog::getExistingDirectory(this, "选择搜索目录");
        if (!dir.isEmpty()) {
            dirListWidget->addItem(dir);
        }
    }

    // 移除搜索目录
    void removeDirectory() {
        QListWidgetItem *item = dirListWidget->currentItem();
        if (item) {
            delete item;
        }
    }

    // 开始搜索
    void startSearch() {
        // 获取搜索目录列表
        QStringList directories;
        for (int i = 0; i < dirListWidget->count(); ++i) {
            directories.append(dirListWidget->item(i)->text());
        }

        if (directories.isEmpty()) {
            statusLabel->setText("状态：请先添加搜索目录");
            return;
        }

        // 获取搜索模式
        QString pattern = patternEdit->text().trimmed();
        if (pattern.isEmpty()) {
            statusLabel->setText("状态：请输入文件名模式");
            return;
        }

        // 清空结果
        resultListWidget->clear();
        totalResults = 0;

        // 更新 UI 状态
        searchButton->setEnabled(false);
        cancelButton->setEnabled(true);
        progressBar->setValue(0);
        progressBar->setRange(0, directories.size());
        statusLabel->setText(QString("状态：搜索中... (0/%1 目录完成)").arg(directories.size()));

        // ========================================
        // 多线程搜索 ⭐⭐⭐⭐⭐
        // ========================================

        // 清空之前的 Watcher 列表
        qDeleteAll(watchers);
        watchers.clear();

        completedCount = 0;
        totalDirectories = directories.size();

        // 为每个目录启动一个异步任务
        for (const QString &dir : directories) {
            // 创建 Watcher
            QFutureWatcher<QList<SearchResult>> *watcher =
                new QFutureWatcher<QList<SearchResult>>(this);

            // 连接完成信号
            connect(watcher, &QFutureWatcher<QList<SearchResult>>::finished,
                    this, &FileSearchTool::onSearchFinished);

            // 启动异步搜索任务
            QFuture<QList<SearchResult>> future =
                QtConcurrent::run(searchInDirectory, dir, pattern);

            // 监听任务
            watcher->setFuture(future);

            // 保存 Watcher（用于取消）
            watchers.append(watcher);
        }

        qDebug() << "启动了" << directories.size() << "个搜索任务";
    }

    // 取消搜索
    void cancelSearch() {
        qDebug() << "取消所有搜索任务";

        // 取消所有任务
        for (QFutureWatcher<QList<SearchResult>> *watcher : watchers) {
            watcher->future().cancel();
        }

        // 更新 UI 状态
        statusLabel->setText("状态：已取消");
        searchButton->setEnabled(true);
        cancelButton->setEnabled(false);
    }

    // 搜索完成（单个目录）
    void onSearchFinished() {
        // 获取发送信号的 Watcher
        QFutureWatcher<QList<SearchResult>> *watcher =
            qobject_cast<QFutureWatcher<QList<SearchResult>>*>(sender());

        if (!watcher) return;

        // 获取搜索结果
        QList<SearchResult> results = watcher->result();

        // 添加到结果列表
        for (const SearchResult &result : results) {
            QString displayText = QString("%1 (%2 字节) - %3")
                .arg(QFileInfo(result.filePath).fileName())
                .arg(result.fileSize)
                .arg(result.filePath);

            resultListWidget->addItem(displayText);
            totalResults++;
        }

        // 更新进度
        completedCount++;
        progressBar->setValue(completedCount);

        // 更新状态
        statusLabel->setText(QString("状态：搜索中... (%1/%2 目录完成，找到 %3 个文件)")
            .arg(completedCount)
            .arg(totalDirectories)
            .arg(totalResults));

        // 检查是否所有任务都完成
        if (completedCount >= totalDirectories) {
            onAllSearchFinished();
        }
    }

    // 所有搜索完成
    void onAllSearchFinished() {
        qDebug() << "所有搜索任务完成";

        // 更新 UI 状态
        statusLabel->setText(QString("状态：搜索完成！共找到 %1 个文件").arg(totalResults));
        searchButton->setEnabled(true);
        cancelButton->setEnabled(false);

        // 清理 Watchers
        qDeleteAll(watchers);
        watchers.clear();
    }

private:
    // UI 控件
    QListWidget *dirListWidget;
    QLineEdit *patternEdit;
    QPushButton *searchButton;
    QPushButton *cancelButton;
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QListWidget *resultListWidget;

    // 搜索状态
    QList<QFutureWatcher<QList<SearchResult>>*> watchers;
    int completedCount;
    int totalDirectories;
    int totalResults;
};

// ============================================
// 主函数
// ============================================

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    FileSearchTool tool;
    tool.setWindowTitle("多线程文件搜索工具");
    tool.resize(600, 500);
    tool.show();

    return app.exec();
}

#include "36_file_search.moc"

// ============================================
// 函数卡片速查
// ============================================
/*
【函数卡片：QtConcurrent::run()】

语法：QFuture<T> QtConcurrent::run(Function fn, Args... args)
作用：在线程池中异步执行函数，返回 QFuture 用于追踪结果
需要：QT += concurrent

示例：
QFuture<QStringList> future = QtConcurrent::run([=]() {
    QStringList results;
    // 耗时搜索...
    return results;
});

────────────────────────────────────────────────────────────

【函数卡片：QFutureWatcher】

作用：监听 QFuture 的状态变化，在主线程中发出信号

常用信号：
- finished()           → 任务完成
- resultReadyAt(int)   → 某个结果就绪（mapped 时用）
- progressValueChanged(int) → 进度变化

示例：
auto *watcher = new QFutureWatcher<QStringList>(this);
connect(watcher, &QFutureWatcher<QStringList>::finished, this, [=]() {
    auto results = watcher->result();
    // 更新 UI...
    watcher->deleteLater();
});
watcher->setFuture(future);

────────────────────────────────────────────────────────────

【函数卡片：QDirIterator】

作用：递归遍历目录，逐个获取文件路径

语法：
QDirIterator(const QString &path, const QStringList &nameFilters,
             QDir::Filters filters, QDirIterator::IteratorFlags flags)

常用标志：
- QDirIterator::Subdirectories  → 递归子目录
- QDir::Files                   → 只返回文件（过滤目录）

示例：
QDirIterator it("/home/user", {"*.cpp", "*.h"},
                QDir::Files, QDirIterator::Subdirectories);
while (it.hasNext()) {
    QString filePath = it.next();
    // 处理文件...
}
*/

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
【错误1】future.cancel() 后未等待线程结束就析构

❌ 错误代码：
void onCancel() {
    for (auto &f : m_futures) f.cancel();
    // 直接返回，Lambda 可能还在运行，访问已销毁的 this
}

✅ 正确代码：
void onCancel() {
    for (auto &f : m_futures) f.cancel();
    for (auto &f : m_futures) f.waitForFinished();  // 等待真正结束
}
// 或者在窗口关闭时：
void closeEvent(QCloseEvent *e) {
    for (auto &f : m_futures) { f.cancel(); f.waitForFinished(); }
    e->accept();
}

预防措施：cancel() 后调用 waitForFinished()，确保 Lambda 不再访问已释放的资源。

────────────────────────────────────────────────────────────

【错误2】QDirIterator 遍历时权限不足导致死循环或崩溃

❌ 错误代码：
QDirIterator it("C:/Windows/System32", QDir::Files, QDirIterator::Subdirectories);
while (it.hasNext()) {
    it.next();  // 某些系统目录权限不足，it.hasNext() 可能一直返回 true
}

✅ 正确代码：
QDirIterator it(dir, nameFilters, QDir::Files | QDir::Readable,
                QDirIterator::Subdirectories);
while (it.hasNext() && !m_cancelled) {  // 加取消检查
    QString path = it.next();
    if (!QFileInfo(path).isReadable()) continue;  // 跳过不可读文件
    // 处理...
}

预防措施：遍历系统目录时过滤 QDir::Readable，并加取消检查防止卡死。

────────────────────────────────────────────────────────────

【错误3】从工作线程直接向 QListWidget 添加条目

❌ 错误代码：
QtConcurrent::run([=]() {
    QDirIterator it(...);
    while (it.hasNext()) {
        m_resultList->addItem(it.next());  // 工作线程操作 UI！崩溃
    }
});

✅ 正确代码：
QtConcurrent::run([=]() {
    QStringList results;
    QDirIterator it(...);
    while (it.hasNext()) results << it.next();
    // 通过信号跨线程传递结果：
    emit searchPartialResult(results);
});
// 主线程槽：
connect(this, &FileSearch::searchPartialResult, this, [=](const QStringList &r) {
    for (auto &path : r) m_resultList->addItem(path);
});

预防措施：工作线程收集结果，通过信号传给主线程，由主线程更新 UI。

────────────────────────────────────────────────────────────

【错误4】通配符模式未转义特殊字符

❌ 错误代码：
QString pattern = userInput;  // 用户输入 "report (2026).txt"
// QDir 的 nameFilter 将括号视为特殊字符，匹配结果不符合预期

✅ 正确代码：
// QDir nameFilter 使用 shell 通配符（* ?），不是正则
// 只有 * 和 ? 是特殊字符，括号是普通字符，无需特殊处理
// 但如果使用 QRegularExpression 进行匹配，需要 escape：
QString escaped = QRegularExpression::escape(pattern);
QRegularExpression re(escaped.replace("\\*", ".*").replace("\\?", "."));

预防措施：明确区分"通配符模式"（QDir）和"正则表达式"（QRegularExpression），按需选择。

────────────────────────────────────────────────────────────

【错误5】多个 QFutureWatcher 共用一个 watcher 对象

❌ 错误代码：
QFutureWatcher<QStringList> m_watcher;  // 只有一个 watcher

// 搜索多个目录时：
for (auto &dir : dirs) {
    auto future = QtConcurrent::run([=]() { return searchDir(dir); });
    m_watcher.setFuture(future);  // 每次 setFuture 会取消前一个！
}

结果：只有最后一个目录被搜索，其余被取消。

✅ 正确代码：
// 每个 future 对应一个 watcher：
for (auto &dir : dirs) {
    auto future = QtConcurrent::run([=]() { return searchDir(dir); });
    auto *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [=]() {
        auto results = watcher->result();
        showResults(results);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
    m_futures << future;  // 保存用于取消
}

预防措施：N 个并发任务需要 N 个独立的 QFutureWatcher 实例。
*/

/*
==============================================
🎯 项目总结
==============================================

【实现的功能】
✅ 多目录并发搜索
✅ 文件名模式匹配（*.txt, *.cpp 等）
✅ 实时显示搜索结果
✅ 显示搜索进度
✅ 支持取消搜索
✅ UI 保持响应

【多线程要点】
1. QtConcurrent::run()：
   - 为每个目录启动一个异步任务
   - 自动在线程池中执行
   - 返回 QFuture<QList<SearchResult>>

2. QFutureWatcher：
   - 监听每个任务的完成事件
   - finished 信号在主线程中发射
   - 可以安全更新 UI

3. 取消机制：
   - 调用 future.cancel() 设置取消标志
   - 搜索函数检查 isInterruptionRequested()
   - 提前退出循环

4. 进度更新：
   - 每个任务完成时更新进度条
   - 显示已完成目录数和找到的文件数

【编译运行】
qmake -project "QT += widgets concurrent"
qmake
make
./36_file_search

【测试建议】
1. 添加多个目录（如 C:/Windows, C:/Program Files）
2. 输入搜索模式（如 *.txt）
3. 点击"开始搜索"，观察多个目录并发搜索
4. 搜索过程中尝试移动窗口（UI 不卡顿）
5. 点击"取消搜索"，观察搜索中断
6. 尝试不同的搜索模式（*.cpp, *report*, 等）

【扩展练习】
1. 添加文件内容搜索（搜索包含特定文本的文件）
2. 添加文件大小过滤（只搜索大于/小于指定大小的文件）
3. 添加修改时间过滤（只搜索最近修改的文件）
4. 导出搜索结果到文件
5. 添加搜索历史记录

【性能优化】
1. 使用 QThreadPool 限制并发线程数
2. 对大目录使用分块搜索
3. 缓存搜索结果
4. 使用数据库存储索引

==============================================
*/

// ============================================
// 练习题
// ============================================
/*
1. 文件内容搜索
   在当前只搜索文件名的基础上，添加"内容搜索"选项：
   - 勾选"搜索内容"复选框时，打开每个匹配的文件，检查是否包含关键词
   - 结果列表额外显示匹配行号和行内容
   提示：QFile + QTextStream 逐行读取，用 contains() 匹配

2. 文件大小过滤
   添加最小/最大文件大小过滤（单位 KB）：
   - 两个 QSpinBox 分别设置最小和最大大小
   - 用 QFileInfo::size() 获取文件大小（字节），与过滤条件比较
   提示：1 KB = 1024 字节

3. 搜索结果导出
   添加"导出结果"按钮：
   - 将当前搜索结果列表写入 CSV 文件
   - 格式：文件名,路径,大小(KB),修改时间
   提示：QFileInfo 提供 size() / lastModified() 等属性

4. 搜索历史记录
   在搜索框旁添加下拉历史（QComboBox）：
   - 每次搜索成功后，将搜索词添加到下拉列表
   - 最多保留 10 条历史
   - 用 QSettings 持久化保存
*/
