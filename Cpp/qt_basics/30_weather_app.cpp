/*
 * Qt 网络编程实战：天气查询应用
 * 日期：2026-06-08（第48天）
 * 难度：⭐⭐ 中级
 *
 * 调用 wttr.in API 获取天气数据（无需 API Key）
 * API：https://wttr.in/{城市}?format=j1
 *
 * 功能：
 * 1. 输入城市名查询天气
 * 2. 显示当前温度、天气描述、湿度、风速
 * 3. 显示未来3天天气预报
 * 4. 错误处理（城市不存在、网络错误）
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
#include <QPushButton>
#include <QProgressBar>
#include <QFrame>
#include <QScrollArea>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDateTime>
#include <QMap>

// 天气描述 → emoji 映射
static QString weatherEmoji(const QString &desc)
{
    QString d = desc.toLower();
    if (d.contains("sunny") || d.contains("clear"))    return "☀️";
    if (d.contains("partly cloudy"))                    return "⛅";
    if (d.contains("cloudy") || d.contains("overcast")) return "☁️";
    if (d.contains("rain") || d.contains("drizzle"))    return "🌧️";
    if (d.contains("thunder") || d.contains("storm"))   return "⛈️";
    if (d.contains("snow") || d.contains("blizzard"))   return "❄️";
    if (d.contains("fog") || d.contains("mist"))        return "🌫️";
    if (d.contains("wind"))                             return "💨";
    return "🌡️";
}

// 星期几中文
static QString weekdayCn(const QString &dateStr)
{
    QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");
    if (!date.isValid()) return dateStr;
    QStringList days = {"周一","周二","周三","周四","周五","周六","周日"};
    return days[date.dayOfWeek() - 1];
}

class WeatherApp : public QWidget
{
    Q_OBJECT

public:
    WeatherApp(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Qt 天气查询");
        setMinimumSize(480, 600);

        m_manager = new QNetworkAccessManager(this);

        // 主布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // ----------------------------------------
        // 顶部搜索栏
        // ----------------------------------------
        QWidget *searchBar = new QWidget();
        searchBar->setStyleSheet("background: #2c3e50;");
        QHBoxLayout *searchLayout = new QHBoxLayout(searchBar);
        searchLayout->setContentsMargins(16, 12, 16, 12);

        QLabel *appTitle = new QLabel("🌤 天气查询");
        appTitle->setStyleSheet("color:white;font-size:16px;font-weight:bold;");
        searchLayout->addWidget(appTitle);
        searchLayout->addStretch();

        m_cityEdit = new QLineEdit();
        m_cityEdit->setPlaceholderText("输入城市名（中文或英文）");
        m_cityEdit->setFixedWidth(200);
        m_cityEdit->setStyleSheet(
            "QLineEdit{background:rgba(255,255,255,0.15);color:white;"
            "border:1px solid rgba(255,255,255,0.3);border-radius:4px;padding:6px 10px;}"
            "QLineEdit::placeholder{color:rgba(255,255,255,0.5);}"
        );
        searchLayout->addWidget(m_cityEdit);

        m_searchBtn = new QPushButton("查询");
        m_searchBtn->setStyleSheet(
            "QPushButton{background:#3498db;color:white;border:none;"
            "border-radius:4px;padding:6px 16px;font-weight:bold;}"
            "QPushButton:hover{background:#2980b9;}"
            "QPushButton:disabled{background:#7f8c8d;}"
        );
        searchLayout->addWidget(m_searchBtn);
        mainLayout->addWidget(searchBar);

        // 进度条
        m_progress = new QProgressBar();
        m_progress->setRange(0, 0);
        m_progress->setVisible(false);
        m_progress->setFixedHeight(3);
        m_progress->setStyleSheet(
            "QProgressBar{border:none;background:#34495e;}"
            "QProgressBar::chunk{background:#3498db;}"
        );
        mainLayout->addWidget(m_progress);

        // ----------------------------------------
        // 内容区域（可滚动）
        // ----------------------------------------
        QScrollArea *scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("QScrollArea{border:none;background:#ecf0f1;}");

        m_contentWidget = new QWidget();
        m_contentWidget->setStyleSheet("background:#ecf0f1;");
        m_contentLayout = new QVBoxLayout(m_contentWidget);
        m_contentLayout->setContentsMargins(16, 16, 16, 16);
        m_contentLayout->setSpacing(12);

        // 初始提示
        m_hintLabel = new QLabel("输入城市名查询天气\n\n示例：北京、Shanghai、Tokyo");
        m_hintLabel->setAlignment(Qt::AlignCenter);
        m_hintLabel->setStyleSheet("color:#95a5a6;font-size:14px;padding:40px;");
        m_contentLayout->addWidget(m_hintLabel);
        m_contentLayout->addStretch();

        scroll->setWidget(m_contentWidget);
        mainLayout->addWidget(scroll);

        // 连接信号
        connect(m_searchBtn, &QPushButton::clicked, this, &WeatherApp::fetchWeather);
        connect(m_cityEdit, &QLineEdit::returnPressed, this, &WeatherApp::fetchWeather);
    }

private slots:

    void fetchWeather()
    {
        QString city = m_cityEdit->text().trimmed();
        if (city.isEmpty()) return;

        m_searchBtn->setEnabled(false);
        m_progress->setVisible(true);
        clearContent();
        m_hintLabel->setText("正在获取 " + city + " 的天气数据...");
        m_hintLabel->setVisible(true);

        // wttr.in API：返回 JSON 格式天气数据，无需 API Key
        QString url = QString("https://wttr.in/%1?format=j1").arg(
            QUrl::toPercentEncoding(city)
        );

        QNetworkRequest request(QUrl(url));
        request.setHeader(QNetworkRequest::UserAgentHeader, "Qt WeatherApp/1.0");
        request.setTransferTimeout(15000);

        QNetworkReply *reply = m_manager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, city]() {
            m_progress->setVisible(false);
            m_searchBtn->setEnabled(true);

            if (reply->error() != QNetworkReply::NoError) {
                showError("网络错误：" + reply->errorString());
                reply->deleteLater();
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);

            if (doc.isNull() || !doc.isObject()) {
                showError("数据解析失败，请检查城市名是否正确");
                reply->deleteLater();
                return;
            }

            displayWeather(doc.object(), city);
            reply->deleteLater();
        });
    }

    void displayWeather(const QJsonObject &json, const QString &city)
    {
        clearContent();
        m_hintLabel->setVisible(false);

        // ----------------------------------------
        // 解析当前天气
        // ----------------------------------------
        /*
        wttr.in JSON 结构：
        {
          "current_condition": [{
            "temp_C": "25",
            "FeelsLikeC": "27",
            "humidity": "60",
            "windspeedKmph": "15",
            "weatherDesc": [{"value": "Sunny"}],
            "observation_time": "12:00 PM"
          }],
          "weather": [{
            "date": "2024-06-08",
            "maxtempC": "30",
            "mintempC": "20",
            "hourly": [...]
          }]
        }
        */
        QJsonArray currentArr = json["current_condition"].toArray();
        if (currentArr.isEmpty()) {
            showError("未找到城市：" + city);
            return;
        }

        QJsonObject current = currentArr[0].toObject();
        QString tempC    = current["temp_C"].toString();
        QString feelsC   = current["FeelsLikeC"].toString();
        QString humidity = current["humidity"].toString();
        QString windKmph = current["windspeedKmph"].toString();
        QString desc     = current["weatherDesc"].toArray()[0].toObject()["value"].toString();
        QString obsTime  = current["observation_time"].toString();

        // ----------------------------------------
        // 当前天气卡片
        // ----------------------------------------
        QFrame *currentCard = makeCard();
        QVBoxLayout *cardLayout = new QVBoxLayout(currentCard);
        cardLayout->setSpacing(8);

        // 城市名 + 观测时间
        QLabel *cityLabel = new QLabel(city + "  " + obsTime);
        cityLabel->setStyleSheet("color:#7f8c8d;font-size:13px;");
        cardLayout->addWidget(cityLabel);

        // 天气 emoji + 温度
        QHBoxLayout *tempLayout = new QHBoxLayout();
        QLabel *emojiLabel = new QLabel(weatherEmoji(desc));
        emojiLabel->setStyleSheet("font-size:48px;");
        tempLayout->addWidget(emojiLabel);

        QVBoxLayout *tempInfoLayout = new QVBoxLayout();
        QLabel *tempLabel = new QLabel(tempC + "°C");
        tempLabel->setStyleSheet("font-size:48px;font-weight:bold;color:#2c3e50;");
        QLabel *descLabel = new QLabel(desc);
        descLabel->setStyleSheet("font-size:16px;color:#7f8c8d;");
        tempInfoLayout->addWidget(tempLabel);
        tempInfoLayout->addWidget(descLabel);
        tempLayout->addLayout(tempInfoLayout);
        tempLayout->addStretch();
        cardLayout->addLayout(tempLayout);

        // 详细信息
        QGridLayout *detailGrid = new QGridLayout();
        detailGrid->setSpacing(8);

        auto addDetail = [&](int row, int col, const QString &icon,
                             const QString &label, const QString &value) {
            QLabel *l = new QLabel(icon + " " + label);
            l->setStyleSheet("color:#7f8c8d;font-size:12px;");
            QLabel *v = new QLabel(value);
            v->setStyleSheet("color:#2c3e50;font-weight:bold;font-size:13px;");
            detailGrid->addWidget(l, row*2,   col);
            detailGrid->addWidget(v, row*2+1, col);
        };

        addDetail(0, 0, "🌡️", "体感温度", feelsC + "°C");
        addDetail(0, 1, "💧", "湿度",     humidity + "%");
        addDetail(0, 2, "💨", "风速",     windKmph + " km/h");

        cardLayout->addLayout(detailGrid);
        m_contentLayout->addWidget(currentCard);

        // ----------------------------------------
        // 未来3天预报
        // ----------------------------------------
        QLabel *forecastTitle = new QLabel("未来天气预报");
        forecastTitle->setStyleSheet(
            "font-size:14px;font-weight:bold;color:#2c3e50;padding:4px 0;"
        );
        m_contentLayout->addWidget(forecastTitle);

        QJsonArray weatherArr = json["weather"].toArray();
        for (int i = 0; i < qMin(3, weatherArr.size()); ++i) {
            QJsonObject day = weatherArr[i].toObject();
            QString date    = day["date"].toString();
            QString maxTemp = day["maxtempC"].toString();
            QString minTemp = day["mintempC"].toString();

            // 取当天中午的天气描述
            QJsonArray hourly = day["hourly"].toArray();
            QString dayDesc = "Unknown";
            if (!hourly.isEmpty()) {
                // hourly[4] 约为中午12点
                int idx = qMin(4, hourly.size() - 1);
                dayDesc = hourly[idx].toObject()["weatherDesc"]
                    .toArray()[0].toObject()["value"].toString();
            }

            QFrame *dayCard = makeCard();
            dayCard->setStyleSheet(
                "QFrame{background:white;border-radius:8px;"
                "border:1px solid #e0e0e0;padding:4px;}"
            );
            QHBoxLayout *dayLayout = new QHBoxLayout(dayCard);

            // 日期
            QString dayName = (i == 0) ? "今天" : (i == 1 ? "明天" : weekdayCn(date));
            QLabel *dateLabel = new QLabel(dayName + "\n" + date.mid(5));
            dateLabel->setStyleSheet("color:#7f8c8d;font-size:12px;");
            dateLabel->setFixedWidth(60);
            dayLayout->addWidget(dateLabel);

            // 天气图标
            QLabel *dayEmoji = new QLabel(weatherEmoji(dayDesc));
            dayEmoji->setStyleSheet("font-size:24px;");
            dayLayout->addWidget(dayEmoji);

            // 天气描述
            QLabel *dayDescLabel = new QLabel(dayDesc);
            dayDescLabel->setStyleSheet("color:#555;font-size:12px;");
            dayLayout->addWidget(dayDescLabel);
            dayLayout->addStretch();

            // 温度范围
            QLabel *tempRange = new QLabel(
                QString("<b>%1°</b> / %2°").arg(maxTemp).arg(minTemp)
            );
            tempRange->setStyleSheet("font-size:14px;color:#2c3e50;");
            dayLayout->addWidget(tempRange);

            m_contentLayout->addWidget(dayCard);
        }

        m_contentLayout->addStretch();
    }

private:

    void clearContent()
    {
        // 清除除 hintLabel 外的所有控件
        QLayoutItem *item;
        while ((item = m_contentLayout->takeAt(0)) != nullptr) {
            if (item->widget() && item->widget() != m_hintLabel) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        m_contentLayout->addWidget(m_hintLabel);
        m_contentLayout->addStretch();
    }

    void showError(const QString &msg)
    {
        m_hintLabel->setText("❌ " + msg);
        m_hintLabel->setStyleSheet("color:#e74c3c;font-size:14px;padding:40px;");
        m_hintLabel->setVisible(true);
    }

    QFrame *makeCard()
    {
        QFrame *card = new QFrame();
        card->setStyleSheet(
            "QFrame{background:white;border-radius:10px;"
            "border:1px solid #e0e0e0;padding:12px;}"
        );
        return card;
    }

    QNetworkAccessManager *m_manager;
    QLineEdit    *m_cityEdit;
    QPushButton  *m_searchBtn;
    QProgressBar *m_progress;
    QWidget      *m_contentWidget;
    QVBoxLayout  *m_contentLayout;
    QLabel       *m_hintLabel;
};

// ============================================
// 练习题
// ============================================
/*
1. 添加搜索历史：
   - 用 QStringList 记录最近5次搜索的城市
   - 在搜索框下方显示历史城市按钮，点击快速查询
   - 用 QSettings 持久化保存历史记录

2. 添加收藏城市功能：
   - 右上角"收藏"按钮，将当前城市加入收藏列表
   - 收藏列表显示在侧边栏，点击快速切换城市
   - 用 QSettings 持久化保存收藏列表

3. 添加小时预报：
   - 展开某天的预报，显示每3小时的天气
   - wttr.in 的 hourly 数组包含每3小时的数据
   - 用 QScrollArea 水平滚动显示
*/

// ============================================
// 函数卡片速查
// ============================================
/*
【函数卡片：QJsonArray::at()】

语法：QJsonValue QJsonArray::at(int i) const
作用：获取 JSON 数组中第 i 个元素

示例：
QJsonArray hourly = obj.value("hourly").toArray();
for (int i = 0; i < hourly.size(); i++) {
    QJsonObject h = hourly.at(i).toObject();
    QString time = h.value("time").toString();
}

────────────────────────────────────────────────────────────

【函数卡片：QUrl::toPercentEncoding()】

语法：static QByteArray QUrl::toPercentEncoding(const QString &input)
作用：将字符串中的特殊字符（空格、中文等）转为 URL 编码（%XX）

示例：
QString city = "北京";
QByteArray encoded = QUrl::toPercentEncoding(city);  // "%E5%8C%97%E4%BA%AC"
QUrl url("https://wttr.in/" + encoded + "?format=j1");
*/

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
【错误1】城市名含中文/空格未 URL 编码导致请求失败

❌ 错误代码：
QString city = "New York";
QUrl url("https://wttr.in/" + city + "?format=j1");
// URL 中有空格，请求失败或返回错误

✅ 正确代码：
QUrl url("https://wttr.in/" +
         QString(QUrl::toPercentEncoding(city)) + "?format=j1");

预防措施：用户输入的城市名拼入 URL 前，必须调用 toPercentEncoding()。

────────────────────────────────────────────────────────────

【错误2】JSON 路径层级假设错误导致数据为空

❌ 错误代码：
// 假设天气数据在 root["temp"]，实际在 root["current_condition"][0]["temp_C"]
QString temp = doc.object().value("temp").toString();  // 始终为空

✅ 正确代码：
// 先打印完整 JSON 确认结构
qDebug() << doc.toJson(QJsonDocument::Indented);

// 按实际路径访问：
auto current = doc.object().value("current_condition").toArray();
if (!current.isEmpty()) {
    QString temp = current.at(0).toObject().value("temp_C").toString();
}

预防措施：接入新 API 时，先 qDebug() 打印完整响应，再写解析代码。

────────────────────────────────────────────────────────────

【错误3】网络不可用时未显示友好提示

❌ 错误代码：
connect(reply, &QNetworkReply::finished, [=]() {
    QByteArray data = reply->readAll();
    reply->deleteLater();
    parseWeather(data);  // 网络错误时 data 为空，解析失败，界面无任何反馈
});

✅ 正确代码：
connect(reply, &QNetworkReply::finished, [=]() {
    if (reply->error() != QNetworkReply::NoError) {
        m_statusLabel->setText("网络错误：" + reply->errorString());
        reply->deleteLater();
        return;
    }
    QByteArray data = reply->readAll();
    reply->deleteLater();
    parseWeather(data);
});

预防措施：所有网络请求都要处理错误，向用户显示可读的错误信息。

────────────────────────────────────────────────────────────

【错误4】快速连续点击查询按钮，发起多个并发请求

❌ 错误代码：
// 用户快速点击"查询"3次，发出3个请求
// 响应顺序不确定，最终显示的可能不是最后一次查询的结果

✅ 正确代码：
void onSearch() {
    if (m_pendingReply) {
        m_pendingReply->abort();   // 取消之前未完成的请求
        m_pendingReply = nullptr;
    }
    m_pendingReply = m_manager->get(request);
    connect(m_pendingReply, &QNetworkReply::finished, this, [=]() {
        m_pendingReply = nullptr;
        // 处理响应...
    });
}

预防措施：发起新请求前，取消并清理上一个未完成的请求。

────────────────────────────────────────────────────────────

【错误5】将摄氏度字符串直接比较数字

❌ 错误代码：
QString temp = "25";
if (temp > "9") { ... }   // 字符串比较！"25" < "9"（字典序）

✅ 正确代码：
int tempInt = temp.toInt();
if (tempInt > 9) { ... }   // 数字比较

预防措施：从 JSON 取出的数字字符串，用 toInt() / toDouble() 转换后再比较。
*/

#include "30_weather_app.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    WeatherApp window;
    window.show();
    return app.exec();
}
