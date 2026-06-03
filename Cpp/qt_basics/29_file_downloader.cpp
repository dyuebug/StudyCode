/*
 * Qt 网络编程进阶：文件下载器
 * 日期：2026-06-08（第48天）
 * 难度：⭐⭐ 中级
 *
 * 本程序演示：
 * 1. downloadProgress 信号（实时进度）
 * 2. QProgressBar 显示下载进度
 * 3. 下载速度计算
 * 4. 取消下载（reply->abort()）
 * 5. QFile 保存下载内容到本地
 * 6. QFileDialog 选择保存路径
 *
 * 编译前必须在 .pro 文件添加：QT += network
 */

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QFileDialog>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QDateTime>
#include <QUrl>

class FileDownloader : public QWidget
{
    Q_OBJECT

public:
    FileDownloader(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Qt 文件下载器");
        resize(600, 450);

        m_manager = new QNetworkAccessManager(this);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(16, 16, 16, 16);
        layout->setSpacing(10);

        // 标题
        QLabel *title = new QLabel("Qt 文件下载器");
        title->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50;");
        layout->addWidget(title);

        // URL 输入
        QLabel *urlLabel = new QLabel("下载 URL：");
        layout->addWidget(urlLabel);

        QHBoxLayout *urlLayout = new QHBoxLayout();
        m_urlEdit = new QLineEdit();
        m_urlEdit->setPlaceholderText("输入文件下载地址...");
        // 预置一个测试 URL（Qt 官网的小文件）
        m_urlEdit->setText("https://www.qt.io/hubfs/Qt-logo-neon-small.png");
        urlLayout->addWidget(m_urlEdit);
        layout->addLayout(urlLayout);

        // 保存路径
        QHBoxLayout *saveLayout = new QHBoxLayout();
        saveLayout->addWidget(new QLabel("保存到："));
        m_savePathEdit = new QLineEdit();
        m_savePathEdit->setText(QDir::homePath() + "/downloaded_file");
        m_savePathEdit->setReadOnly(true);
        saveLayout->addWidget(m_savePathEdit);
        QPushButton *browseBtn = new QPushButton("浏览...");
        browseBtn->setFixedWidth(70);
        connect(browseBtn, &QPushButton::clicked, this, &FileDownloader::chooseSavePath);
        saveLayout->addWidget(browseBtn);
        layout->addLayout(saveLayout);

        // 操作按钮
        QHBoxLayout *btnLayout = new QHBoxLayout();
        m_downloadBtn = new QPushButton("▶ 开始下载");
        m_downloadBtn->setStyleSheet(
            "QPushButton{background:#27ae60;color:white;border:none;"
            "border-radius:4px;padding:8px 20px;font-weight:bold;}"
            "QPushButton:hover{background:#219a52;}"
            "QPushButton:disabled{background:#95a5a6;}"
        );
        m_cancelBtn = new QPushButton("■ 取消");
        m_cancelBtn->setEnabled(false);
        m_cancelBtn->setStyleSheet(
            "QPushButton{background:#e74c3c;color:white;border:none;"
            "border-radius:4px;padding:8px 20px;}"
            "QPushButton:hover{background:#c0392b;}"
            "QPushButton:disabled{background:#bdc3c7;}"
        );
        btnLayout->addWidget(m_downloadBtn);
        btnLayout->addWidget(m_cancelBtn);
        btnLayout->addStretch();
        layout->addLayout(btnLayout);

        // 进度条
        m_progressBar = new QProgressBar();
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        m_progressBar->setTextVisible(true);
        m_progressBar->setStyleSheet(
            "QProgressBar{border:none;background:#ecf0f1;border-radius:6px;height:20px;}"
            "QProgressBar::chunk{background:#27ae60;border-radius:6px;}"
        );
        layout->addWidget(m_progressBar);

        // 状态信息
        QHBoxLayout *statusLayout = new QHBoxLayout();
        m_statusLabel = new QLabel("等待下载...");
        m_statusLabel->setStyleSheet("color:#555;");
        m_speedLabel = new QLabel("");
        m_speedLabel->setStyleSheet("color:#3498db;font-weight:bold;");
        m_sizeLabel = new QLabel("");
        m_sizeLabel->setStyleSheet("color:#7f8c8d;");
        statusLayout->addWidget(m_statusLabel);
        statusLayout->addStretch();
        statusLayout->addWidget(m_speedLabel);
        statusLayout->addWidget(m_sizeLabel);
        layout->addLayout(statusLayout);

        // 日志
        QLabel *logLabel = new QLabel("下载日志：");
        logLabel->setStyleSheet("font-weight:bold;color:#495057;");
        layout->addWidget(logLabel);

        m_log = new QTextEdit();
        m_log->setReadOnly(true);
        m_log->setFont(QFont("Consolas", 10));
        m_log->setStyleSheet(
            "background:#1e1e2e;color:#cdd6f4;border:none;border-radius:6px;padding:8px;"
        );
        layout->addWidget(m_log);

        // 速度计算定时器（每秒更新一次）
        m_speedTimer = new QTimer(this);
        m_speedTimer->setInterval(1000);
        connect(m_speedTimer, &QTimer::timeout, this, &FileDownloader::updateSpeed);

        // 连接按钮
        connect(m_downloadBtn, &QPushButton::clicked, this, &FileDownloader::startDownload);
        connect(m_cancelBtn,   &QPushButton::clicked, this, &FileDownloader::cancelDownload);
    }

private slots:

    void chooseSavePath()
    {
        QString path = QFileDialog::getSaveFileName(
            this, "选择保存路径", m_savePathEdit->text()
        );
        if (!path.isEmpty()) {
            m_savePathEdit->setText(path);
        }
    }

    void startDownload()
    {
        QString urlStr = m_urlEdit->text().trimmed();
        if (urlStr.isEmpty()) {
            log("❌ 请输入下载 URL");
            return;
        }

        QUrl url(urlStr);
        if (!url.isValid()) {
            log("❌ URL 格式不正确");
            return;
        }

        // 根据 URL 自动设置文件名
        QString fileName = QFileInfo(url.path()).fileName();
        if (!fileName.isEmpty()) {
            QString savePath = QDir::homePath() + "/" + fileName;
            m_savePathEdit->setText(savePath);
        }

        // 重置状态
        m_progressBar->setValue(0);
        m_receivedBytes = 0;
        m_lastBytes = 0;
        m_startTime = QDateTime::currentMSecsSinceEpoch();

        // 更新 UI 状态
        m_downloadBtn->setEnabled(false);
        m_cancelBtn->setEnabled(true);
        m_statusLabel->setText("连接中...");
        m_speedLabel->setText("");
        m_sizeLabel->setText("");

        log("→ 开始下载：" + urlStr);
        log("  保存到：" + m_savePathEdit->text());

        // 构建请求
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::UserAgentHeader, "Qt FileDownloader/1.0");
        request.setTransferTimeout(30000);  // 30秒超时

        // 发送 GET 请求
        m_currentReply = m_manager->get(request);

        // ----------------------------------------
        // downloadProgress 信号：实时下载进度
        // ----------------------------------------
        /*
        【函数卡片】QNetworkReply::downloadProgress

        信号：void downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
        参数：
          bytesReceived：已接收字节数
          bytesTotal：总字节数（-1 表示未知）
        触发时机：每收到一批数据时触发
        */
        connect(m_currentReply, &QNetworkReply::downloadProgress,
                this, &FileDownloader::onDownloadProgress);

        // 请求完成
        connect(m_currentReply, &QNetworkReply::finished,
                this, &FileDownloader::onDownloadFinished);

        // 启动速度计算定时器
        m_speedTimer->start();
    }

    void onDownloadProgress(qint64 received, qint64 total)
    {
        m_receivedBytes = received;

        if (total > 0) {
            // 已知总大小
            int percent = static_cast<int>(received * 100 / total);
            m_progressBar->setValue(percent);
            m_sizeLabel->setText(
                QString("%1 KB / %2 KB")
                    .arg(received / 1024)
                    .arg(total / 1024)
            );
            m_statusLabel->setText(QString("下载中... %1%").arg(percent));
        } else {
            // 未知总大小（流式下载）
            m_progressBar->setRange(0, 0);  // 不确定进度动画
            m_sizeLabel->setText(QString("已下载 %1 KB").arg(received / 1024));
            m_statusLabel->setText("下载中...");
        }
    }

    void updateSpeed()
    {
        // 计算下载速度（字节/秒）
        qint64 delta = m_receivedBytes - m_lastBytes;
        m_lastBytes = m_receivedBytes;

        if (delta > 0) {
            QString speedStr;
            if (delta >= 1024 * 1024) {
                speedStr = QString("%1 MB/s").arg(delta / 1024.0 / 1024.0, 0, 'f', 1);
            } else if (delta >= 1024) {
                speedStr = QString("%1 KB/s").arg(delta / 1024.0, 0, 'f', 1);
            } else {
                speedStr = QString("%1 B/s").arg(delta);
            }
            m_speedLabel->setText(speedStr);
        }
    }

    void onDownloadFinished()
    {
        m_speedTimer->stop();
        m_progressBar->setRange(0, 100);

        if (m_currentReply->error() == QNetworkReply::OperationCanceledError) {
            // 用户取消
            m_progressBar->setValue(0);
            m_statusLabel->setText("已取消");
            m_speedLabel->setText("");
            log("⚠️ 下载已取消");
        } else if (m_currentReply->error() != QNetworkReply::NoError) {
            // 下载失败
            m_progressBar->setValue(0);
            m_statusLabel->setText("下载失败");
            log("❌ 下载失败：" + m_currentReply->errorString());
        } else {
            // 下载成功，保存文件
            QByteArray data = m_currentReply->readAll();
            QString savePath = m_savePathEdit->text();

            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();

                m_progressBar->setValue(100);
                m_statusLabel->setText("✅ 下载完成");
                m_speedLabel->setText("");

                // 计算总耗时
                qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_startTime;
                log(QString("✅ 下载完成！"));
                log(QString("  文件大小：%1 KB").arg(data.size() / 1024));
                log(QString("  保存路径：%1").arg(savePath));
                log(QString("  耗时：%1 秒").arg(elapsed / 1000.0, 0, 'f', 1));
            } else {
                log("❌ 文件保存失败：" + file.errorString());
                m_statusLabel->setText("保存失败");
            }
        }

        // 恢复按钮状态
        m_downloadBtn->setEnabled(true);
        m_cancelBtn->setEnabled(false);
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    void cancelDownload()
    {
        if (m_currentReply) {
            // reply->abort() 取消正在进行的请求
            // 会触发 finished() 信号，error() 返回 OperationCanceledError
            m_currentReply->abort();
        }
    }

private:
    void log(const QString &msg)
    {
        QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
        m_log->append("[" + time + "] " + msg);
    }

    QNetworkAccessManager *m_manager;
    QNetworkReply         *m_currentReply = nullptr;
    QTimer                *m_speedTimer;

    QLineEdit    *m_urlEdit;
    QLineEdit    *m_savePathEdit;
    QPushButton  *m_downloadBtn;
    QPushButton  *m_cancelBtn;
    QProgressBar *m_progressBar;
    QLabel       *m_statusLabel;
    QLabel       *m_speedLabel;
    QLabel       *m_sizeLabel;
    QTextEdit    *m_log;

    qint64 m_receivedBytes = 0;
    qint64 m_lastBytes = 0;
    qint64 m_startTime = 0;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：用 reply->readAll() 读取大文件（内存溢出）
    // 对于大文件，readAll() 会把整个文件加载到内存！
    QByteArray data = reply->readAll();  // 1GB 文件 = 1GB 内存！

✅ 正确：边下载边写入文件（流式写入）
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        m_file.write(reply->readAll());  // 每次只写入当前批次的数据
    });

❌ 错误2：取消下载后没有处理 OperationCanceledError
    connect(reply, &QNetworkReply::finished, this, [reply]() {
        QByteArray data = reply->readAll();  // 取消后 data 为空，但没有检查！
        saveFile(data);  // 保存了空文件！
    });

✅ 正确：检查错误类型
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        // 用户取消，不保存文件
        return;
    }

❌ 错误3：进度条范围没有重置
    m_progressBar->setRange(0, 0);  // 设置为不确定进度
    // 下载完成后忘记重置回 (0, 100)，进度条一直转圈

✅ 正确：完成后重置
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(100);

❌ 错误4：多次点击下载按钮，创建多个 reply
    void startDownload() {
        m_reply = manager->get(request);  // 第二次点击又创建一个！
    }

✅ 正确：下载时禁用按钮，或先取消上一个
    m_downloadBtn->setEnabled(false);
*/

// ============================================
// 练习题
// ============================================
/*
1. 实现流式写入（适合大文件）：
   - 在 startDownload() 中打开文件
   - 连接 readyRead 信号，每次有数据就写入文件
   - 在 finished() 中关闭文件
   提示：
     m_file.open(QIODevice::WriteOnly);
     connect(reply, &QNetworkReply::readyRead, [this, reply]() {
         m_file.write(reply->readAll());
     });

2. 添加下载队列：
   - 支持同时添加多个 URL
   - 用 QQueue<QString> 存储待下载列表
   - 一个下载完成后自动开始下一个
   提示：在 onDownloadFinished() 中检查队列是否为空

3. 添加断点续传（HTTP Range）：
   - 如果文件已部分下载，从断点继续
   - 提示：request.setRawHeader("Range", "bytes=已下载字节数-")
   - 服务器返回 206 Partial Content 表示支持断点续传
*/

#include "29_file_downloader.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    FileDownloader window;
    window.show();
    return app.exec();
}
