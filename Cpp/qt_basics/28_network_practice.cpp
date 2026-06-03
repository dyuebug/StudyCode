/*
 * Qt 网络编程实战：调用公开 API
 * 日期：2026-06-07（第47天）
 * 难度：⭐⭐ 中级
 *
 * 调用两个公开 API：
 * 1. httpbin.org/uuid — 生成随机 UUID
 * 2. open.er-api.com/v6/latest/CNY — 人民币汇率
 * 3. api.ipify.org?format=json — 获取本机公网 IP
 *
 * 编译前必须在 .pro 文件添加：QT += network
 */

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QGroupBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDateTime>

class ApiPracticeDemo : public QWidget
{
    Q_OBJECT

public:
    ApiPracticeDemo(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Qt 网络编程实战 — 公开 API 调用");
        resize(700, 580);

        m_manager = new QNetworkAccessManager(this);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(16, 16, 16, 16);
        mainLayout->setSpacing(12);

        QLabel *title = new QLabel("Qt 网络编程实战 — 调用公开 API");
        title->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50;");
        mainLayout->addWidget(title);

        // ----------------------------------------
        // API 1：获取本机公网 IP
        // ----------------------------------------
        QGroupBox *ipGroup = new QGroupBox("API 1：获取本机公网 IP（api.ipify.org）");
        QHBoxLayout *ipLayout = new QHBoxLayout(ipGroup);

        m_ipLabel = new QLabel("点击按钮获取...");
        m_ipLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #3498db;");
        ipLayout->addWidget(m_ipLabel);
        ipLayout->addStretch();

        QPushButton *ipBtn = new QPushButton("获取我的 IP");
        ipBtn->setStyleSheet("QPushButton{background:#3498db;color:white;border:none;border-radius:4px;padding:8px 16px;}QPushButton:hover{background:#2980b9;}");
        ipLayout->addWidget(ipBtn);
        connect(ipBtn, &QPushButton::clicked, this, &ApiPracticeDemo::fetchMyIp);
        mainLayout->addWidget(ipGroup);

        // ----------------------------------------
        // API 2：生成随机 UUID
        // ----------------------------------------
        QGroupBox *uuidGroup = new QGroupBox("API 2：生成随机 UUID（httpbin.org/uuid）");
        QVBoxLayout *uuidLayout = new QVBoxLayout(uuidGroup);

        QHBoxLayout *uuidBtnLayout = new QHBoxLayout();
        m_uuidLabel = new QLabel("点击按钮生成...");
        m_uuidLabel->setStyleSheet("font-family: Consolas; font-size: 14px; color: #27ae60;");
        uuidBtnLayout->addWidget(m_uuidLabel);
        uuidBtnLayout->addStretch();

        QPushButton *uuidBtn = new QPushButton("生成 UUID");
        uuidBtn->setStyleSheet("QPushButton{background:#27ae60;color:white;border:none;border-radius:4px;padding:8px 16px;}QPushButton:hover{background:#219a52;}");
        uuidBtnLayout->addWidget(uuidBtn);
        uuidLayout->addLayout(uuidBtnLayout);

        m_uuidHistory = new QTextEdit();
        m_uuidHistory->setReadOnly(true);
        m_uuidHistory->setMaximumHeight(80);
        m_uuidHistory->setFont(QFont("Consolas", 10));
        m_uuidHistory->setStyleSheet("background:#f8f9fa;border:1px solid #dee2e6;border-radius:4px;");
        m_uuidHistory->setPlaceholderText("历史记录...");
        uuidLayout->addWidget(m_uuidHistory);

        connect(uuidBtn, &QPushButton::clicked, this, &ApiPracticeDemo::fetchUuid);
        mainLayout->addWidget(uuidGroup);

        // ----------------------------------------
        // API 3：人民币汇率
        // ----------------------------------------
        QGroupBox *rateGroup = new QGroupBox("API 3：人民币汇率（open.er-api.com）");
        QVBoxLayout *rateLayout = new QVBoxLayout(rateGroup);

        QPushButton *rateBtn = new QPushButton("获取最新汇率");
        rateBtn->setStyleSheet("QPushButton{background:#e67e22;color:white;border:none;border-radius:4px;padding:8px 16px;}QPushButton:hover{background:#d35400;}");
        rateLayout->addWidget(rateBtn);

        m_rateProgress = new QProgressBar();
        m_rateProgress->setRange(0, 0);
        m_rateProgress->setVisible(false);
        m_rateProgress->setFixedHeight(4);
        m_rateProgress->setStyleSheet("QProgressBar{border:none;background:#eee;border-radius:2px;}QProgressBar::chunk{background:#e67e22;border-radius:2px;}");
        rateLayout->addWidget(m_rateProgress);

        // 汇率显示网格
        m_rateGrid = new QGridLayout();
        m_rateGrid->setSpacing(8);
        QStringList currencies = {"USD", "EUR", "JPY", "GBP", "HKD", "KRW"};
        QStringList names      = {"美元", "欧元", "日元", "英镑", "港币", "韩元"};
        for (int i = 0; i < currencies.size(); ++i) {
            QLabel *nameLabel = new QLabel(names[i] + "（" + currencies[i] + "）");
            nameLabel->setStyleSheet("color:#495057;");
            QLabel *rateLabel = new QLabel("--");
            rateLabel->setStyleSheet("font-weight:bold;color:#e67e22;font-size:14px;");
            rateLabel->setAlignment(Qt::AlignRight);
            m_rateGrid->addWidget(nameLabel, i/3, (i%3)*2);
            m_rateGrid->addWidget(rateLabel, i/3, (i%3)*2+1);
            m_rateLabels[currencies[i]] = rateLabel;
        }
        rateLayout->addLayout(m_rateGrid);

        m_rateUpdateLabel = new QLabel("最后更新：--");
        m_rateUpdateLabel->setStyleSheet("color:#999;font-size:11px;");
        rateLayout->addWidget(m_rateUpdateLabel);

        connect(rateBtn, &QPushButton::clicked, this, &ApiPracticeDemo::fetchExchangeRates);
        mainLayout->addWidget(rateGroup);

        // ----------------------------------------
        // 请求日志
        // ----------------------------------------
        QLabel *logTitle = new QLabel("请求日志：");
        logTitle->setStyleSheet("font-weight:bold;color:#495057;");
        mainLayout->addWidget(logTitle);

        m_log = new QTextEdit();
        m_log->setReadOnly(true);
        m_log->setFont(QFont("Consolas", 10));
        m_log->setMaximumHeight(100);
        m_log->setStyleSheet("background:#1e1e2e;color:#cdd6f4;border:none;border-radius:6px;padding:6px;");
        mainLayout->addWidget(m_log);
    }

private slots:

    // ----------------------------------------
    // API 1：获取本机公网 IP
    // ----------------------------------------
    void fetchMyIp()
    {
        log("→ GET https://api.ipify.org?format=json");
        m_ipLabel->setText("获取中...");

        QNetworkRequest req(QUrl("https://api.ipify.org?format=json"));
        req.setTransferTimeout(8000);
        QNetworkReply *reply = m_manager->get(req);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                QString ip = doc.object()["ip"].toString();
                m_ipLabel->setText("🌐 " + ip);
                log("← 公网 IP：" + ip);
            } else {
                m_ipLabel->setText("获取失败");
                log("← 错误：" + reply->errorString());
            }
            reply->deleteLater();
        });
    }

    // ----------------------------------------
    // API 2：生成随机 UUID
    // ----------------------------------------
    void fetchUuid()
    {
        log("→ GET https://httpbin.org/uuid");

        QNetworkRequest req(QUrl("https://httpbin.org/uuid"));
        req.setTransferTimeout(8000);
        QNetworkReply *reply = m_manager->get(req);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                QString uuid = doc.object()["uuid"].toString();
                m_uuidLabel->setText(uuid);
                // 追加到历史记录
                QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
                m_uuidHistory->append(time + "  " + uuid);
                log("← UUID：" + uuid);
            } else {
                m_uuidLabel->setText("生成失败");
                log("← 错误：" + reply->errorString());
            }
            reply->deleteLater();
        });
    }

    // ----------------------------------------
    // API 3：人民币汇率
    // ----------------------------------------
    void fetchExchangeRates()
    {
        log("→ GET https://open.er-api.com/v6/latest/CNY");
        m_rateProgress->setVisible(true);

        // 重置显示
        for (auto &label : m_rateLabels)
            label->setText("获取中...");

        QNetworkRequest req(QUrl("https://open.er-api.com/v6/latest/CNY"));
        req.setTransferTimeout(10000);
        QNetworkReply *reply = m_manager->get(req);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            m_rateProgress->setVisible(false);

            if (reply->error() != QNetworkReply::NoError) {
                log("← 错误：" + reply->errorString());
                for (auto &label : m_rateLabels)
                    label->setText("获取失败");
                reply->deleteLater();
                return;
            }

            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();

            // 检查 API 返回状态
            if (obj["result"].toString() != "success") {
                log("← API 返回错误");
                reply->deleteLater();
                return;
            }

            // 获取汇率数据
            QJsonObject rates = obj["rates"].toObject();
            QStringList currencies = {"USD", "EUR", "JPY", "GBP", "HKD", "KRW"};

            for (const QString &currency : currencies) {
                if (rates.contains(currency) && m_rateLabels.contains(currency)) {
                    double rate = rates[currency].toDouble();
                    // CNY 为基准，rate 表示 1 CNY = rate 外币
                    // 显示 1 外币 = ? CNY
                    double cnyPerUnit = 1.0 / rate;
                    m_rateLabels[currency]->setText(
                        QString("1 %1 = ¥%2").arg(currency).arg(cnyPerUnit, 0, 'f', 4)
                    );
                }
            }

            // 更新时间
            QString updateTime = obj["time_last_update_utc"].toString();
            m_rateUpdateLabel->setText("最后更新：" + updateTime);
            log("← 汇率数据获取成功");

            reply->deleteLater();
        });
    }

private:
    void log(const QString &msg)
    {
        QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
        m_log->append("[" + time + "] " + msg);
    }

    QNetworkAccessManager *m_manager;

    // API 1
    QLabel *m_ipLabel;

    // API 2
    QLabel    *m_uuidLabel;
    QTextEdit *m_uuidHistory;

    // API 3
    QProgressBar *m_rateProgress;
    QGridLayout  *m_rateGrid;
    QMap<QString, QLabel*> m_rateLabels;
    QLabel *m_rateUpdateLabel;

    // 日志
    QTextEdit *m_log;
};

// ============================================
// 函数卡片速查
// ============================================
/*
【函数卡片：QJsonObject::value()】

语法：QJsonValue QJsonObject::value(const QString &key) const
作用：从 JSON 对象中取指定键的值
返回：QJsonValue（若键不存在，返回 QJsonValue::Undefined）

类型转换：
- .toString()    → QString
- .toInt()       → int
- .toDouble()    → double
- .toBool()      → bool
- .toArray()     → QJsonArray
- .toObject()    → QJsonObject

示例：
QJsonObject obj = doc.object();
QString ip   = obj.value("ip").toString();
int     code = obj.value("code").toInt();
// 安全写法（提供默认值）：
QString city = obj.value("city").toString("未知");

────────────────────────────────────────────────────────────

【函数卡片：QNetworkReply::error()】

语法：QNetworkReply::NetworkError error() const
作用：获取网络请求的错误类型
返回：枚举值，NoError=0 表示成功

示例：
connect(reply, &QNetworkReply::finished, this, [=]() {
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "请求失败：" << reply->errorString();
        reply->deleteLater();
        return;
    }
    QByteArray data = reply->readAll();
    reply->deleteLater();
    // 处理 data...
});
*/

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
【错误1】reply 未调用 deleteLater() 导致内存泄漏

❌ 错误代码：
connect(reply, &QNetworkReply::finished, [=]() {
    QByteArray data = reply->readAll();
    // 忘记 reply->deleteLater()！每次请求泄漏一个 reply 对象
});

✅ 正确代码：
connect(reply, &QNetworkReply::finished, [=]() {
    QByteArray data = reply->readAll();
    reply->deleteLater();  // 必须！
    // 处理 data...
});

预防措施：finished 槽函数末尾永远调用 reply->deleteLater()。

────────────────────────────────────────────────────────────

【错误2】未检查 reply->error() 就解析响应体

❌ 错误代码：
connect(reply, &QNetworkReply::finished, [=]() {
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    // 网络错误时 data 可能是错误页面 HTML，解析 JSON 失败

✅ 正确代码：
connect(reply, &QNetworkReply::finished, [=]() {
    if (reply->error() != QNetworkReply::NoError) {
        showError(reply->errorString());
        reply->deleteLater();
        return;
    }
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    reply->deleteLater();
});

预防措施：先检查 error()，再读取 readAll()。

────────────────────────────────────────────────────────────

【错误3】JSON 嵌套层级访问未检查类型

❌ 错误代码：
QJsonObject root = doc.object();
// 假设 "data" 是对象，直接转：
QString name = root["data"]["name"].toString();
// 如果 "data" 不存在或不是对象，root["data"] 返回 Undefined
// .toObject() 返回空对象，再访问 "name" 同样 Undefined → toString() 返回 ""
// 没有崩溃，但得到空字符串，难以排查

✅ 正确代码：
QJsonValue dataVal = root.value("data");
if (!dataVal.isObject()) {
    qDebug() << "data 字段缺失或类型错误";
    return;
}
QString name = dataVal.toObject().value("name").toString();

预防措施：访问嵌套 JSON 时，逐层用 isObject()/isArray() 检查类型。

────────────────────────────────────────────────────────────

【错误4】在 UI 线程直接做耗时 JSON 解析阻塞界面

❌ 错误代码：
connect(reply, &QNetworkReply::finished, [=]() {
    QByteArray data = reply->readAll();
    // 解析一个 10MB 的 JSON 文件，UI 卡顿几秒
    auto doc = QJsonDocument::fromJson(data);
    updateUI(doc);
});

✅ 正确代码（数据量大时）：
// 用 QtConcurrent 或 QThread 在后台解析
connect(reply, &QNetworkReply::finished, [=]() {
    QByteArray data = reply->readAll();
    reply->deleteLater();
    QtConcurrent::run([=]() {
        auto doc = QJsonDocument::fromJson(data);
        QMetaObject::invokeMethod(this, [=]() { updateUI(doc); });
    });
});

预防措施：响应体超过 1MB 时，JSON 解析放到后台线程。

────────────────────────────────────────────────────────────

【错误5】并发多个请求时 Lambda 捕获 reply 变量被覆盖

❌ 错误代码：
for (const auto &url : urls) {
    QNetworkReply *reply = m_manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, [=]() {
        qDebug() << reply->url();  // 每次循环 reply 被重新赋值
        // Lambda 捕获的是 reply 的当前值（值捕获），实际上没问题
        // 但如果是引用捕获 [&] 就有问题！
    });
}

// 危险写法（[&] 捕获）：
QNetworkReply *reply;
for (const auto &url : urls) {
    reply = m_manager->get(...);
    connect(reply, &QNetworkReply::finished, [&]() {  // 引用捕获！
        // 所有 Lambda 都引用同一个 reply 变量，最终值是最后一个 reply
        reply->readAll();  // 错误！
    });
}

✅ 正确代码：
// 总是用值捕获 [=] 或明确捕获特定变量
for (const auto &url : urls) {
    auto *reply = m_manager->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, reply, [reply]() {  // 明确捕获
        qDebug() << reply->url();
        reply->deleteLater();
    });
}

预防措施：网络请求回调 Lambda 永远用值捕获，不用引用捕获。
*/

// ============================================
// 练习题
// ============================================
/*
1. 添加请求超时机制
   - 发起请求后，启动一个 3 秒 QTimer
   - 3 秒内收到响应，停止 Timer
   - 3 秒超时，调用 reply->abort() 并显示"请求超时"
   提示：QTimer::singleShot(3000, reply, [reply]() { reply->abort(); })

2. 请求重试机制
   - 请求失败时（非用户取消），最多重试 3 次
   - 每次重试前等待 1 秒
   - 3 次均失败后显示错误
   提示：用计数变量 + QTimer::singleShot() 实现延迟重试

3. 缓存上次成功的响应
   - 请求成功后，将响应数据存入 QMap<QUrl, QByteArray>
   - 再次请求同一 URL 时，先显示缓存数据，再发起新请求更新
   - 提示：在 finished 回调中先更新缓存，再更新 UI

4. 思考题
   以下场景各应该用 GET 还是 POST？为什么？
   a. 查询天气（传入城市名）
   b. 用户登录（传入用户名+密码）
   c. 获取随机 UUID
   d. 搜索图片（传入关键词）
*/

#include "28_network_practice.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ApiPracticeDemo window;
    window.show();
    return app.exec();
}
