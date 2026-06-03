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

#include "28_network_practice.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ApiPracticeDemo window;
    window.show();
    return app.exec();
}
