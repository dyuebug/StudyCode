/*
 * Qt 用户注册表单 - 实战项目
 * 日期：2026-05-30（第39天）
 *
 * 功能：
 * 1. 完整的用户注册表单
 * 2. 表单验证
 * 3. 数据收集和显示
 * 4. 用户反馈
 */

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QPushButton>
#include <QButtonGroup>
#include <QMessageBox>
#include <QGroupBox>

class RegistrationForm : public QWidget
{
    Q_OBJECT

public:
    RegistrationForm(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("用户注册表单");
        resize(500, 600);

        // 创建主布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        // 标题
        QLabel *title = new QLabel("用户注册");
        title->setStyleSheet("QLabel { font-size: 24px; font-weight: bold; color: #2c3e50; }");
        title->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(title);

        mainLayout->addSpacing(20);

        // 创建表单布局
        QFormLayout *formLayout = new QFormLayout();
        formLayout->setLabelAlignment(Qt::AlignRight);
        formLayout->setSpacing(15);

        // 1. 姓名输入
        m_nameEdit = new QLineEdit();
        m_nameEdit->setPlaceholderText("请输入真实姓名");
        formLayout->addRow("姓名 *：", m_nameEdit);

        // 2. 性别选择
        QWidget *genderWidget = new QWidget();
        QHBoxLayout *genderLayout = new QHBoxLayout(genderWidget);
        genderLayout->setContentsMargins(0, 0, 0, 0);

        m_maleRadio = new QRadioButton("男");
        m_femaleRadio = new QRadioButton("女");
        m_maleRadio->setChecked(true);

        m_genderGroup = new QButtonGroup(this);
        m_genderGroup->addButton(m_maleRadio);
        m_genderGroup->addButton(m_femaleRadio);

        genderLayout->addWidget(m_maleRadio);
        genderLayout->addWidget(m_femaleRadio);
        genderLayout->addStretch();

        formLayout->addRow("性别：", genderWidget);

        // 3. 年龄输入
        m_ageSpinBox = new QSpinBox();
        m_ageSpinBox->setRange(18, 100);
        m_ageSpinBox->setValue(25);
        m_ageSpinBox->setSuffix(" 岁");
        formLayout->addRow("年龄 *：", m_ageSpinBox);

        // 4. 城市选择
        m_cityCombo = new QComboBox();
        m_cityCombo->addItems({"北京", "上海", "广州", "深圳", "杭州", "成都", "武汉", "西安"});
        formLayout->addRow("城市 *：", m_cityCombo);

        // 5. 邮箱输入
        m_emailEdit = new QLineEdit();
        m_emailEdit->setPlaceholderText("example@email.com");
        formLayout->addRow("邮箱 *：", m_emailEdit);

        // 6. 电话输入
        m_phoneEdit = new QLineEdit();
        m_phoneEdit->setPlaceholderText("13800138000");
        formLayout->addRow("电话 *：", m_phoneEdit);

        mainLayout->addLayout(formLayout);

        mainLayout->addSpacing(10);

        // 7. 爱好选择（使用 QGroupBox）
        QGroupBox *hobbyGroup = new QGroupBox("爱好 *（至少选择一项）");
        QVBoxLayout *hobbyLayout = new QVBoxLayout(hobbyGroup);

        m_hobby1 = new QCheckBox("读书");
        m_hobby2 = new QCheckBox("运动");
        m_hobby3 = new QCheckBox("旅游");
        m_hobby4 = new QCheckBox("音乐");
        m_hobby5 = new QCheckBox("编程");

        hobbyLayout->addWidget(m_hobby1);
        hobbyLayout->addWidget(m_hobby2);
        hobbyLayout->addWidget(m_hobby3);
        hobbyLayout->addWidget(m_hobby4);
        hobbyLayout->addWidget(m_hobby5);

        mainLayout->addWidget(hobbyGroup);

        mainLayout->addSpacing(10);

        // 8. 个人简介
        QLabel *bioLabel = new QLabel("个人简介 *：");
        mainLayout->addWidget(bioLabel);

        m_bioEdit = new QTextEdit();
        m_bioEdit->setPlaceholderText("请输入个人简介（50-200字）...");
        m_bioEdit->setMaximumHeight(100);
        mainLayout->addWidget(m_bioEdit);

        mainLayout->addSpacing(20);

        // 按钮布局
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();

        QPushButton *submitBtn = new QPushButton("提交注册");
        submitBtn->setStyleSheet("QPushButton { background-color: #3498db; color: white; padding: 10px 30px; font-size: 14px; }");

        QPushButton *resetBtn = new QPushButton("重置表单");
        resetBtn->setStyleSheet("QPushButton { background-color: #95a5a6; color: white; padding: 10px 30px; font-size: 14px; }");

        btnLayout->addWidget(submitBtn);
        btnLayout->addWidget(resetBtn);

        mainLayout->addLayout(btnLayout);

        // 连接信号和槽
        connect(submitBtn, &QPushButton::clicked, this, &RegistrationForm::onSubmit);
        connect(resetBtn, &QPushButton::clicked, this, &RegistrationForm::onReset);
    }

private slots:
    // 提交表单
    void onSubmit()
    {
        // 1. 验证姓名
        QString name = m_nameEdit->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(this, "验证失败", "请输入姓名！");
            m_nameEdit->setFocus();
            return;
        }

        if (name.length() < 2 || name.length() > 20) {
            QMessageBox::warning(this, "验证失败", "姓名长度必须在2-20个字符之间！");
            m_nameEdit->setFocus();
            return;
        }

        // 2. 验证年龄
        int age = m_ageSpinBox->value();
        if (age < 18 || age > 100) {
            QMessageBox::warning(this, "验证失败", "年龄必须在18-100之间！");
            return;
        }

        // 3. 验证邮箱
        QString email = m_emailEdit->text().trimmed();
        if (email.isEmpty()) {
            QMessageBox::warning(this, "验证失败", "请输入邮箱！");
            m_emailEdit->setFocus();
            return;
        }

        if (!email.contains("@") || !email.contains(".")) {
            QMessageBox::warning(this, "验证失败", "邮箱格式不正确！");
            m_emailEdit->setFocus();
            return;
        }

        // 4. 验证电话
        QString phone = m_phoneEdit->text().trimmed();
        if (phone.isEmpty()) {
            QMessageBox::warning(this, "验证失败", "请输入电话！");
            m_phoneEdit->setFocus();
            return;
        }

        if (phone.length() != 11) {
            QMessageBox::warning(this, "验证失败", "电话号码必须是11位！");
            m_phoneEdit->setFocus();
            return;
        }

        // 5. 验证爱好
        QStringList hobbies;
        if (m_hobby1->isChecked()) hobbies << "读书";
        if (m_hobby2->isChecked()) hobbies << "运动";
        if (m_hobby3->isChecked()) hobbies << "旅游";
        if (m_hobby4->isChecked()) hobbies << "音乐";
        if (m_hobby5->isChecked()) hobbies << "编程";

        if (hobbies.isEmpty()) {
            QMessageBox::warning(this, "验证失败", "请至少选择一个爱好！");
            return;
        }

        // 6. 验证个人简介
        QString bio = m_bioEdit->toPlainText().trimmed();
        if (bio.isEmpty()) {
            QMessageBox::warning(this, "验证失败", "请输入个人简介！");
            m_bioEdit->setFocus();
            return;
        }

        if (bio.length() < 50 || bio.length() > 200) {
            QMessageBox::warning(this, "验证失败", "个人简介长度必须在50-200字之间！");
            m_bioEdit->setFocus();
            return;
        }

        // 验证通过，收集数据
        QString gender = m_maleRadio->isChecked() ? "男" : "女";
        QString city = m_cityCombo->currentText();

        QString info = QString(
            "注册信息确认\n\n"
            "姓名：%1\n"
            "性别：%2\n"
            "年龄：%3 岁\n"
            "城市：%4\n"
            "邮箱：%5\n"
            "电话：%6\n"
            "爱好：%7\n"
            "简介：%8"
        ).arg(name).arg(gender).arg(age).arg(city)
         .arg(email).arg(phone).arg(hobbies.join("、")).arg(bio);

        QMessageBox::information(this, "注册成功", info);
    }

    // 重置表单
    void onReset()
    {
        m_nameEdit->clear();
        m_maleRadio->setChecked(true);
        m_ageSpinBox->setValue(25);
        m_cityCombo->setCurrentIndex(0);
        m_emailEdit->clear();
        m_phoneEdit->clear();
        m_hobby1->setChecked(false);
        m_hobby2->setChecked(false);
        m_hobby3->setChecked(false);
        m_hobby4->setChecked(false);
        m_hobby5->setChecked(false);
        m_bioEdit->clear();

        m_nameEdit->setFocus();
    }

private:
    QLineEdit *m_nameEdit;
    QRadioButton *m_maleRadio;
    QRadioButton *m_femaleRadio;
    QButtonGroup *m_genderGroup;
    QSpinBox *m_ageSpinBox;
    QComboBox *m_cityCombo;
    QLineEdit *m_emailEdit;
    QLineEdit *m_phoneEdit;
    QCheckBox *m_hobby1;
    QCheckBox *m_hobby2;
    QCheckBox *m_hobby3;
    QCheckBox *m_hobby4;
    QCheckBox *m_hobby5;
    QTextEdit *m_bioEdit;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    RegistrationForm form;
    form.show();

    return app.exec();
}

#include "07_registration_form.moc"
