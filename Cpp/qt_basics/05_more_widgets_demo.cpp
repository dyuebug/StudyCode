/*
 * Qt 更多控件演示程序
 * 日期：2026-05-30（第39天）
 *
 * 本程序演示：
 * 1. QComboBox（下拉框）
 * 2. QCheckBox（复选框）
 * 3. QRadioButton（单选按钮）
 * 4. QSpinBox（数字输入框）
 */

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QMessageBox>

class WidgetDemo : public QWidget
{
    Q_OBJECT

public:
    WidgetDemo(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Qt 控件演示");
        resize(400, 400);

        // 创建主布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        // 1. QComboBox 演示
        mainLayout->addWidget(new QLabel("=== QComboBox（下拉框）==="));

        m_cityCombo = new QComboBox();
        m_cityCombo->addItems({"北京", "上海", "广州", "深圳", "杭州"});
        m_cityCombo->setCurrentIndex(0);  // 设置默认选项

        m_cityLabel = new QLabel("当前选择：北京");

        mainLayout->addWidget(new QLabel("选择城市："));
        mainLayout->addWidget(m_cityCombo);
        mainLayout->addWidget(m_cityLabel);

        // 连接信号：当选项改变时更新标签
        connect(m_cityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &WidgetDemo::onCityChanged);

        mainLayout->addSpacing(20);

        // 2. QCheckBox 演示
        mainLayout->addWidget(new QLabel("=== QCheckBox（复选框）==="));
        mainLayout->addWidget(new QLabel("选择爱好（可多选）："));

        m_hobby1 = new QCheckBox("读书");
        m_hobby2 = new QCheckBox("运动");
        m_hobby3 = new QCheckBox("旅游");

        mainLayout->addWidget(m_hobby1);
        mainLayout->addWidget(m_hobby2);
        mainLayout->addWidget(m_hobby3);

        m_hobbyLabel = new QLabel("已选择：无");
        mainLayout->addWidget(m_hobbyLabel);

        // 连接信号：当复选框状态改变时更新标签
        connect(m_hobby1, &QCheckBox::stateChanged, this, &WidgetDemo::onHobbyChanged);
        connect(m_hobby2, &QCheckBox::stateChanged, this, &WidgetDemo::onHobbyChanged);
        connect(m_hobby3, &QCheckBox::stateChanged, this, &WidgetDemo::onHobbyChanged);

        mainLayout->addSpacing(20);

        // 3. QRadioButton 演示
        mainLayout->addWidget(new QLabel("=== QRadioButton（单选按钮）==="));
        mainLayout->addWidget(new QLabel("选择性别（单选）："));

        m_maleRadio = new QRadioButton("男");
        m_femaleRadio = new QRadioButton("女");
        m_maleRadio->setChecked(true);  // 设置默认选中

        // 创建按钮组（确保互斥）
        m_genderGroup = new QButtonGroup(this);
        m_genderGroup->addButton(m_maleRadio);
        m_genderGroup->addButton(m_femaleRadio);

        mainLayout->addWidget(m_maleRadio);
        mainLayout->addWidget(m_femaleRadio);

        m_genderLabel = new QLabel("当前选择：男");
        mainLayout->addWidget(m_genderLabel);

        // 连接信号
        connect(m_maleRadio, &QRadioButton::toggled, this, &WidgetDemo::onGenderChanged);

        mainLayout->addSpacing(20);

        // 4. QSpinBox 演示
        mainLayout->addWidget(new QLabel("=== QSpinBox（数字输入框）==="));
        mainLayout->addWidget(new QLabel("选择年龄："));

        m_ageSpinBox = new QSpinBox();
        m_ageSpinBox->setRange(18, 100);  // 设置范围
        m_ageSpinBox->setValue(25);       // 设置默认值
        m_ageSpinBox->setSingleStep(1);   // 设置步长

        mainLayout->addWidget(m_ageSpinBox);

        m_ageLabel = new QLabel("当前年龄：25");
        mainLayout->addWidget(m_ageLabel);

        // 连接信号
        connect(m_ageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &WidgetDemo::onAgeChanged);

        mainLayout->addSpacing(20);

        // 提交按钮
        QPushButton *submitBtn = new QPushButton("提交");
        mainLayout->addWidget(submitBtn);
        connect(submitBtn, &QPushButton::clicked, this, &WidgetDemo::onSubmit);

        mainLayout->addStretch();
    }

private slots:
    // 城市选择改变
    void onCityChanged(int index)
    {
        QString city = m_cityCombo->currentText();
        m_cityLabel->setText("当前选择：" + city);
    }

    // 爱好选择改变
    void onHobbyChanged()
    {
        QStringList hobbies;
        if (m_hobby1->isChecked()) hobbies << "读书";
        if (m_hobby2->isChecked()) hobbies << "运动";
        if (m_hobby3->isChecked()) hobbies << "旅游";

        if (hobbies.isEmpty()) {
            m_hobbyLabel->setText("已选择：无");
        } else {
            m_hobbyLabel->setText("已选择：" + hobbies.join("、"));
        }
    }

    // 性别选择改变
    void onGenderChanged(bool checked)
    {
        if (checked) {
            m_genderLabel->setText("当前选择：男");
        } else {
            m_genderLabel->setText("当前选择：女");
        }
    }

    // 年龄改变
    void onAgeChanged(int value)
    {
        m_ageLabel->setText("当前年龄：" + QString::number(value));
    }

    // 提交按钮点击
    void onSubmit()
    {
        QString city = m_cityCombo->currentText();

        QStringList hobbies;
        if (m_hobby1->isChecked()) hobbies << "读书";
        if (m_hobby2->isChecked()) hobbies << "运动";
        if (m_hobby3->isChecked()) hobbies << "旅游";

        QString gender = m_maleRadio->isChecked() ? "男" : "女";
        int age = m_ageSpinBox->value();

        QString info = QString("城市：%1\n爱好：%2\n性别：%3\n年龄：%4")
                       .arg(city)
                       .arg(hobbies.isEmpty() ? "无" : hobbies.join("、"))
                       .arg(gender)
                       .arg(age);

        QMessageBox::information(this, "提交信息", info);
    }

private:
    // QComboBox
    QComboBox *m_cityCombo;
    QLabel *m_cityLabel;

    // QCheckBox
    QCheckBox *m_hobby1;
    QCheckBox *m_hobby2;
    QCheckBox *m_hobby3;
    QLabel *m_hobbyLabel;

    // QRadioButton
    QRadioButton *m_maleRadio;
    QRadioButton *m_femaleRadio;
    QButtonGroup *m_genderGroup;
    QLabel *m_genderLabel;

    // QSpinBox
    QSpinBox *m_ageSpinBox;
    QLabel *m_ageLabel;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    WidgetDemo demo;
    demo.show();

    return app.exec();
}

#include "05_more_widgets_demo.moc"
