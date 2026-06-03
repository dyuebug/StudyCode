/*
 * Qt 网络编程基础：QNetworkAccessManager + JSON 解析
 * 日期：2026-06-07（第47天）
 * 难度：⭐⭐ 中级
 *
 * 本程序演示：
 * 1. QNetworkAccessManager — 网络请求管理器
 * 2. QNetworkRequest — 封装请求（URL、Header）
 * 3. QNetworkReply — 异步响应对象
 * 4. GET 请求
 * 5. POST 请求（发送 JSON 数据）
 * 6. QJsonDocument — 解析 JSON 字符串
 * 7. QJsonObject / QJsonArray — 访问 JSON 数据
 * 8. 错误处理与超时
 *
 * 编译前必须在 .pro 文件添加：QT += network
 */

// ============================================
// 【核心概念】Qt 网络编程
// ============================================
/*
Qt 网络请求是异步的：
  发送请求 → 立即返回（不阻塞）→ 请求完成时发出 finished() 信号

核心类：
  QNetworkAccessManager：管理所有网络请求（一个应用通常只需一个）
  QNetworkRequest：封装请求信息（URL、Header、超时等）
  QNetworkReply：响应对象，包含状态码、响应头、响应体

异步模式（推荐）：
  reply = manager->get(request)
  connect(reply, &QNetworkReply::finished, this, [reply]() {
      // 在这里处理响应
      reply->deleteLater();  // 必须手动释放！
  })

⚠️ 重要：
  - reply->deleteLater() 必须调用，否则内存泄漏
  - 不要在主线程中同步等待网络请求（会冻结 UI）
  - .pro 文件必须添加 QT += network
*/

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QProgressBar>
#include <QTabWidget>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>

class NetworkBasicsDemo : public QWidget
{
    Q_OBJECT

public:
    NetworkBasicsDemo(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Qt 网络编程基础演示");
        resize(750, 600);

        // ----------------------------------------
        // 创建 QNetworkAccessManager
        // ----------------------------------------
        /*
        【函数卡片】QNetworkAccessManager

        作用：管理所有 HTTP/HTTPS 网络请求
        构造：QNetworkAccessManager(parent)
        说明：一个应用通常只需要一个实例（可以复用）

        主要方法：
          get(request)           — 发送 GET 请求
          post(request, data)    — 发送 POST 请求
          put(request, data)     — 发送 PUT 请求
          deleteResource(request)— 发送 DELETE 请求

        返回值：QNetworkReply*（异步，不阻塞）
        */
        m_manager = new QNetworkAccessManager(this);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(16, 16, 16, 16);
        mainLayout->setSpacing(10);

        QLabel *title = new QLabel("Qt 网络编程基础 — HTTP 请求与 JSON 解析");
        title->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50;");
        mainLayout->addWidget(title);

        QTabWidget *tabs = new QTabWidget();
        mainLayout->addWidget(tabs);

        // ----------------------------------------
        // Tab1：GET 请求
        // ----------------------------------------
        QWidget *getTab = new QWidget();
        QVBoxLayout *getLayout = new QVBoxLayout(getTab);

        QLabel *getDesc = new QLabel(
            "GET 请求：从服务器获取数据\n"
            "测试 URL：https://httpbin.org/get（返回请求信息的 JSON）"
        );
        getDesc->setStyleSheet("color:#555;background:#f8f9fa;padding:8px;border-radius:4px;");
        getLayout->addWidget(getDesc);

        QHBoxLayout *urlLayout = new QHBoxLayout();
        urlLayout->addWidget(new QLabel("URL："));
        m_getUrlEdit = new QLineEdit("https://httpbin.org/get?name=Qt&version=6");
        urlLayout->addWidget(m_getUrlEdit);
        QPushButton *getBtn = new QPushButton("发送 GET 请求");
        getBtn->setStyleSheet("QPushButton{background:#3498db;color:white;border:none;border-radius:4px;padding:8px 16px;}QPushButton:hover{background:#2980b9;}");
        urlLayout->addWidget(getBtn);
        getLayout->addLayout(urlLayout);

        m_getProgress = new QProgressBar();
        m_getProgress->setRange(0, 0);  // 不确定进度（循环动画）
        m_getProgress->setVisible(false);
        m_getProgress->setFixedHeight(6);
        m_getProgress->setStyleSheet("QProgressBar{border:none;background:#eee;border-radius:3px;}QProgressBar::chunk{background:#3498db;border-radius:3px;}");
        getLayout->addWidget(m_getProgress);

        m_getResult = new QTextEdit();
        m_getResult->setReadOnly(true);
        m_getResult->setFont(QFont("Consolas", 11));
        m_getResult->setStyleSheet("background:#1e1e2e;color:#cdd6f4;border:none;border-radius:6px;padding:8px;");
        getLayout->addWidget(m_getResult);

        connect(getBtn, &QPushButton::clicked, this, &NetworkBasicsDemo::sendGetRequest);
        tabs->addTab(getTab, "GET 请求");

        // ----------------------------------------
        // Tab2：POST 请求
        // ----------------------------------------
        QWidget *postTab = new QWidget();
        QVBoxLayout *postLayout = new QVBoxLayout(postTab);

        QLabel *postDesc = new QLabel(
            "POST 请求：向服务器发送数据\n"
            "测试 URL：https://httpbin.org/post（回显发送的数据）"
        );
        postDesc->setStyleSheet("color:#555;background:#f8f9fa;padding:8px;border-radius:4px;");
        postLayout->addWidget(postDesc);

        QFormLayout *postForm = new QFormLayout();
        m_postNameEdit  = new QLineEdit("张三");
        m_postEmailEdit = new QLineEdit("zhangsan@example.com");
        m_postAgeEdit   = new QLineEdit("25");
        postForm->addRow("姓名：", m_postNameEdit);
        postForm->addRow("邮箱：", m_postEmailEdit);
        postForm->addRow("年龄：", m_postAgeEdit);
        postLayout->addLayout(postForm);

        QPushButton *postBtn = new QPushButton("发送 POST 请求（JSON）");
        postBtn->setStyleSheet("QPushButton{background:#27ae60;color:white;border:none;border-radius:4px;padding:8px 16px;}QPushButton:hover{background:#219a52;}");
        postLayout->addWidget(postBtn);

        m_postProgress = new QProgressBar();
        m_postProgress->setRange(0, 0);
        m_postProgress->setVisible(false);
        m_postProgress->setFixedHeight(6);
        m_postProgress->setStyleSheet("QProgressBar{border:none;background:#eee;border-radius:3px;}QProgressBar::chunk{background:#27ae60;border-radius:3px;}");
        postLayout->addWidget(m_postProgress);

        m_postResult = new QTextEdit();
        m_postResult->setReadOnly(true);
        m_postResult->setFont(QFont("Consolas", 11));
        m_postResult->setStyleSheet("background:#1e1e2e;color:#cdd6f4;border:none;border-radius:6px;padding:8px;");
        postLayout->addWidget(m_postResult);

        connect(postBtn, &QPushButton::clicked, this, &NetworkBasicsDemo::sendPostRequest);
        tabs->addTab(postTab, "POST 请求");

        // ----------------------------------------
        // Tab3：JSON 解析演示
        // ----------------------------------------
        QWidget *jsonTab = new QWidget();
        QVBoxLayout *jsonLayout = new QVBoxLayout(jsonTab);

        QLabel *jsonDesc = new QLabel("JSON 解析演示：输入 JSON 字符串，解析并显示结构");
        jsonDesc->setStyleSheet("color:#555;background:#f8f9fa;padding:8px;border-radius:4px;");
        jsonLayout->addWidget(jsonDesc);

        m_jsonInput = new QTextEdit();
        m_jsonInput->setFont(QFont("Consolas", 11));
        m_jsonInput->setMaximumHeight(150);
        m_jsonInput->setPlainText(R"({
  "name": "张三",
  "age": 25,
  "scores": [85, 92, 78],
  "address": {
    "city": "北京",
    "district": "朝阳区"
  },
  "active": true
})");
        jsonLayout->addWidget(new QLabel("输入 JSON："));
        jsonLayout->addWidget(m_jsonInput);

        QPushButton *parseBtn = new QPushButton("解析 JSON");
        parseBtn->setStyleSheet("QPushButton{background:#e67e22;color:white;border:none;border-radius:4px;padding:8px 16px;}QPushButton:hover{background:#d35400;}");
        jsonLayout->addWidget(parseBtn);

        m_jsonResult = new QTextEdit();
        m_jsonResult->setReadOnly(true);
        m_jsonResult->setFont(QFont("Consolas", 11));
        m_jsonResult->setStyleSheet("background:#1e1e2e;color:#cdd6f4;border:none;border-radius:6px;padding:8px;");
        jsonLayout->addWidget(m_jsonResult);

        connect(parseBtn, &QPushButton::clicked, this, &NetworkBasicsDemo::parseJson);
        tabs->addTab(jsonTab, "JSON 解析");
    }

private slots:

    // ============================================
    // 发送 GET 请求
    // ============================================
    void sendGetRequest()
    {
        QString urlStr = m_getUrlEdit->text().trimmed();
        if (urlStr.isEmpty()) return;

        m_getResult->clear();
        m_getResult->append("正在发送 GET 请求...");
        m_getProgress->setVisible(true);

        // --------------------------------------------
        // 构建请求
        // --------------------------------------------
        /*
        【函数卡片】QNetworkRequest

        构造：QNetworkRequest(QUrl url)
        setHeader(header, value)：设置请求头
          QNetworkRequest::ContentTypeHeader — Content-Type
          QNetworkRequest::UserAgentHeader   — User-Agent
          QNetworkRequest::AuthorizationHeader — Authorization
        setAttribute(attr, value)：设置属性
          QNetworkRequest::RedirectPolicyAttribute — 重定向策略
        */
        QNetworkRequest request(QUrl(urlStr));
        request.setHeader(QNetworkRequest::UserAgentHeader, "Qt NetworkDemo/1.0");
        // 设置超时（Qt 5.15+）
        request.setTransferTimeout(10000);  // 10秒超时

        // 发送 GET 请求（异步，立即返回）
        QNetworkReply *reply = m_manager->get(request);

        // 连接完成信号
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            m_getProgress->setVisible(false);
            handleGetReply(reply);
        });

        // 连接下载进度信号（可选）
        connect(reply, &QNetworkReply::downloadProgress,
                this, [this](qint64 received, qint64 total) {
            if (total > 0) {
                m_getProgress->setRange(0, total);
                m_getProgress->setValue(received);
            }
        });
    }

    void handleGetReply(QNetworkReply *reply)
    {
        // --------------------------------------------
        // 处理响应
        // --------------------------------------------
        /*
        【函数卡片】QNetworkReply

        error()：错误类型（NoError = 成功）
        errorString()：错误描述
        attribute(QNetworkRequest::HttpStatusCodeAttribute)：HTTP 状态码
        readAll()：读取全部响应体（QByteArray）
        header(QNetworkRequest::ContentTypeHeader)：响应头
        deleteLater()：异步删除（必须调用！）
        */
        m_getResult->clear();

        if (reply->error() != QNetworkReply::NoError) {
            // 请求失败
            m_getResult->append("❌ 请求失败：" + reply->errorString());
            m_getResult->append("错误类型：" + QString::number(reply->error()));
        } else {
            // 请求成功
            int statusCode = reply->attribute(
                QNetworkRequest::HttpStatusCodeAttribute).toInt();
            m_getResult->append(QString("✅ 请求成功！HTTP 状态码：%1").arg(statusCode));
            m_getResult->append("");

            // 读取响应体
            QByteArray data = reply->readAll();
            m_getResult->append("响应内容：");
            m_getResult->append(QString::fromUtf8(data));

            // 尝试解析为 JSON
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull()) {
                m_getResult->append("\n--- 解析后的 JSON 结构 ---");
                m_getResult->append(doc.toJson(QJsonDocument::Indented));
            }
        }

        // ⭐ 必须调用 deleteLater()，否则内存泄漏！
        reply->deleteLater();
    }

    // ============================================
    // 发送 POST 请求
    // ============================================
    void sendPostRequest()
    {
        m_postResult->clear();
        m_postResult->append("正在发送 POST 请求...");
        m_postProgress->setVisible(true);

        // --------------------------------------------
        // 构建 JSON 请求体
        // --------------------------------------------
        /*
        【函数卡片】QJsonDocument（构建 JSON）

        QJsonObject：JSON 对象 {}
          obj["key"] = value;
          obj.insert("key", value);

        QJsonArray：JSON 数组 []
          arr.append(value);

        QJsonDocument：JSON 文档
          QJsonDocument(obj)：从对象创建
          doc.toJson()：转为字节数组
          doc.toJson(QJsonDocument::Compact)：紧凑格式
          doc.toJson(QJsonDocument::Indented)：缩进格式
        */
        QJsonObject body;
        body["name"]  = m_postNameEdit->text().trimmed();
        body["email"] = m_postEmailEdit->text().trimmed();
        body["age"]   = m_postAgeEdit->text().toInt();
        body["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonDocument doc(body);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        // 构建请求
        QNetworkRequest request(QUrl("https://httpbin.org/post"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setHeader(QNetworkRequest::UserAgentHeader, "Qt NetworkDemo/1.0");
        request.setTransferTimeout(10000);

        m_postResult->append("发送的 JSON 数据：");
        m_postResult->append(doc.toJson(QJsonDocument::Indented));
        m_postResult->append("---");

        // 发送 POST 请求
        QNetworkReply *reply = m_manager->post(request, jsonData);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            m_postProgress->setVisible(false);

            if (reply->error() != QNetworkReply::NoError) {
                m_postResult->append("❌ 请求失败：" + reply->errorString());
            } else {
                int code = reply->attribute(
                    QNetworkRequest::HttpStatusCodeAttribute).toInt();
                m_postResult->append(QString("✅ 请求成功！状态码：%1").arg(code));

                QByteArray data = reply->readAll();
                QJsonDocument respDoc = QJsonDocument::fromJson(data);
                if (!respDoc.isNull()) {
                    // 提取服务器回显的 json 字段
                    QJsonObject respObj = respDoc.object();
                    if (respObj.contains("json")) {
                        m_postResult->append("\n服务器收到的 JSON：");
                        m_postResult->append(
                            QJsonDocument(respObj["json"].toObject())
                                .toJson(QJsonDocument::Indented)
                        );
                    }
                }
            }
            reply->deleteLater();
        });
    }

    // ============================================
    // JSON 解析演示
    // ============================================
    void parseJson()
    {
        m_jsonResult->clear();
        QString jsonStr = m_jsonInput->toPlainText();

        // --------------------------------------------
        // QJsonDocument::fromJson — 解析 JSON 字符串
        // --------------------------------------------
        /*
        【函数卡片】QJsonDocument::fromJson()

        语法：
          QJsonDocument fromJson(const QByteArray &json,
                                 QJsonParseError *error = nullptr)

        返回值：QJsonDocument（解析失败时 isNull() 返回 true）

        检查类型：
          doc.isObject()  — 是否为 JSON 对象 {}
          doc.isArray()   — 是否为 JSON 数组 []
          doc.isNull()    — 是否解析失败
        */
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(
            jsonStr.toUtf8(), &parseError
        );

        if (doc.isNull()) {
            m_jsonResult->append("❌ JSON 解析失败：" + parseError.errorString());
            m_jsonResult->append(QString("错误位置：第 %1 个字符").arg(parseError.offset));
            return;
        }

        m_jsonResult->append("✅ JSON 解析成功！");
        m_jsonResult->append("");

        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            m_jsonResult->append("=== JSON 对象字段 ===");

            // 遍历所有键值对
            for (auto it = obj.begin(); it != obj.end(); ++it) {
                QString key = it.key();
                QJsonValue val = it.value();

                QString typeStr;
                QString valStr;

                switch (val.type()) {
                    case QJsonValue::String:
                        typeStr = "String";
                        valStr  = val.toString();
                        break;
                    case QJsonValue::Double:
                        typeStr = "Number";
                        valStr  = QString::number(val.toDouble());
                        break;
                    case QJsonValue::Bool:
                        typeStr = "Bool";
                        valStr  = val.toBool() ? "true" : "false";
                        break;
                    case QJsonValue::Array: {
                        typeStr = "Array";
                        QJsonArray arr = val.toArray();
                        QStringList items;
                        for (const QJsonValue &v : arr)
                            items << v.toVariant().toString();
                        valStr = "[" + items.join(", ") + "]";
                        break;
                    }
                    case QJsonValue::Object:
                        typeStr = "Object";
                        valStr  = QJsonDocument(val.toObject())
                                    .toJson(QJsonDocument::Compact);
                        break;
                    case QJsonValue::Null:
                        typeStr = "Null";
                        valStr  = "null";
                        break;
                    default:
                        typeStr = "Unknown";
                        break;
                }

                m_jsonResult->append(
                    QString("  %-12s [%s] = %s")
                        .arg(key).arg(typeStr).arg(valStr)
                );
            }

            // 演示具体字段访问
            m_jsonResult->append("\n=== 具体字段访问示例 ===");
            if (obj.contains("name"))
                m_jsonResult->append("name = " + obj["name"].toString());
            if (obj.contains("age"))
                m_jsonResult->append("age = " + QString::number(obj["age"].toInt()));
            if (obj.contains("scores")) {
                QJsonArray scores = obj["scores"].toArray();
                QStringList sl;
                for (const QJsonValue &v : scores)
                    sl << QString::number(v.toInt());
                m_jsonResult->append("scores = [" + sl.join(", ") + "]");
            }
            if (obj.contains("address")) {
                QJsonObject addr = obj["address"].toObject();
                m_jsonResult->append("address.city = " + addr["city"].toString());
            }
        }
    }

private:
    QNetworkAccessManager *m_manager;
    QLineEdit  *m_getUrlEdit;
    QTextEdit  *m_getResult;
    QProgressBar *m_getProgress;
    QLineEdit  *m_postNameEdit, *m_postEmailEdit, *m_postAgeEdit;
    QTextEdit  *m_postResult;
    QProgressBar *m_postProgress;
    QTextEdit  *m_jsonInput;
    QTextEdit  *m_jsonResult;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：忘记调用 reply->deleteLater()
    connect(reply, &QNetworkReply::finished, this, [reply]() {
        QByteArray data = reply->readAll();
        // 没有 deleteLater()！每次请求都泄漏内存
    });

✅ 正确：
    connect(reply, &QNetworkReply::finished, this, [reply]() {
        QByteArray data = reply->readAll();
        reply->deleteLater();  // 必须！
    });

❌ 错误2：在主线程同步等待网络请求（冻结 UI）
    QNetworkReply *reply = manager->get(request);
    while (!reply->isFinished()) {
        QApplication::processEvents();  // 危险！可能导致重入问题
    }

✅ 正确：使用信号槽异步处理
    connect(reply, &QNetworkReply::finished, this, &MyClass::onFinished);

❌ 错误3：不检查网络错误
    QByteArray data = reply->readAll();
    // 如果请求失败，data 可能为空或包含错误信息

✅ 正确：
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << reply->errorString();
        reply->deleteLater();
        return;
    }

❌ 错误4：JSON 解析不检查 isNull()
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();  // 如果解析失败，obj 是空的！

✅ 正确：
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) { return; }
    QJsonObject obj = doc.object();

❌ 错误5：忘记在 .pro 文件添加 QT += network
    编译报错：QNetworkAccessManager: No such file or directory
*/

// ============================================
// 练习题
// ============================================
/*
1. 添加请求头设置：
   - 允许用户输入自定义 Header（如 Authorization: Bearer token）
   - 提示：request.setRawHeader("Authorization", "Bearer " + token.toUtf8())

2. 添加请求历史记录：
   - 每次请求后，将 URL + 状态码 + 时间 记录到 QListWidget
   - 点击历史记录可重新发送该请求

3. 实现文件下载功能：
   - 输入文件 URL，点击下载
   - 用 QProgressBar 显示下载进度
   - 保存到本地文件（QFile + reply->readAll()）
   - 提示：connect(reply, &QNetworkReply::downloadProgress, ...)
*/

#include "27_network_basics.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    NetworkBasicsDemo window;
    window.show();
    return app.exec();
}
