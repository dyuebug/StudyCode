/*
 * Qt 对话框综合实战 — 增强版注册表单
 * 日期：2026-06-01（第41天）
 * 难度：⭐⭐ 中级
 *
 * 在第39天的注册表单基础上，集成对话框：
 * 1. "选择头像"按钮 → QFileDialog 选择图片
 * 2. "选择主题色"按钮 → QColorDialog 选择颜色
 * 3. 提交前 → QMessageBox::question 确认
 * 4. 提交成功 → 自定义 QDialog 显示摘要
 *
 * 编译：qmake + make，或在 Qt Creator 中直接运行
 */

#include <QApplication>
#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QTextEdit>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>

// ============================================
// 注册成功摘要对话框（自定义 QDialog）
// ============================================
class RegistrationSummaryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegistrationSummaryDialog(const QString &summary, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("注册成功");
        setMinimumWidth(400);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(12);
        layout->setContentsMargins(20, 20, 20, 20);

        // 成功图标和标题
        QLabel *iconLabel = new QLabel("✅");
        iconLabel->setStyleSheet("font-size: 36px;");
        iconLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(iconLabel);

        QLabel *titleLabel = new QLabel("注册成功！");
        titleLabel->setStyleSheet(
            "font-size: 18px; font-weight: bold; color: #27ae60;"
        );
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);

        // 摘要信息（可滚动）
        QTextEdit *summaryEdit = new QTextEdit();
        summaryEdit->setReadOnly(true);
        summaryEdit->setPlainText(summary);
        summaryEdit->setMaximumHeight(200);
        summaryEdit->setStyleSheet(
            "QTextEdit { background: #f8f9fa; border: 1px solid #dee2e6; "
            "border-radius: 4px; padding: 8px; }"
        );
        layout->addWidget(summaryEdit);

        // 关闭按钮
        QPushButton *closeBtn = new QPushButton("关闭");
        closeBtn->setStyleSheet(
            "QPushButton { background: #27ae60; color: white; border: none; "
            "border-radius: 4px; padding: 8px 24px; font-size: 14px; }"
            "QPushButton:hover { background: #219a52; }"
        );
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        btnLayout->addWidget(closeBtn);
        layout->addLayout(btnLayout);
    }
};

// ============================================
// 增强版注册表单
// ============================================
class RegistrationForm : public QWidget
{
    Q_OBJECT

public:
    RegistrationForm(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("用户注册（集成对话框版）");
        resize(520, 700);

        // 使用滚动区域，防止内容过多时超出窗口
        QScrollArea *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);

        QWidget *content = new QWidget();
        QVBoxLayout *mainLayout = new QVBoxLayout(content);
        mainLayout->setSpacing(12);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        // ----------------------------------------
        // 标题
        // ----------------------------------------
        QLabel *titleLabel = new QLabel("用户注册");
        titleLabel->setStyleSheet(
            "font-size: 20px; font-weight: bold; color: #2c3e50; "
            "padding: 10px 0;"
        );
        titleLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(titleLabel);

        // ----------------------------------------
        // 头像选择区域（新增：使用 QFileDialog）
        // ----------------------------------------
        QGroupBox *avatarGroup = new QGroupBox("头像");
        QHBoxLayout *avatarLayout = new QHBoxLayout(avatarGroup);

        m_avatarLabel = new QLabel("未选择头像");
        m_avatarLabel->setFixedSize(80, 80);
        m_avatarLabel->setAlignment(Qt::AlignCenter);
        m_avatarLabel->setStyleSheet(
            "QLabel { border: 2px dashed #bdc3c7; border-radius: 40px; "
            "color: #95a5a6; font-size: 11px; }"
        );
        avatarLayout->addWidget(m_avatarLabel);

        QVBoxLayout *avatarBtnLayout = new QVBoxLayout();
        QPushButton *selectAvatarBtn = new QPushButton("选择头像图片");
        selectAvatarBtn->setStyleSheet(
            "QPushButton { background: #3498db; color: white; border: none; "
            "border-radius: 4px; padding: 6px 12px; }"
            "QPushButton:hover { background: #2980b9; }"
        );
        connect(selectAvatarBtn, &QPushButton::clicked, this, &RegistrationForm::selectAvatar);

        m_avatarPathLabel = new QLabel("未选择");
        m_avatarPathLabel->setStyleSheet("color: #7f8c8d; font-size: 11px;");
        m_avatarPathLabel->setWordWrap(true);

        avatarBtnLayout->addWidget(selectAvatarBtn);
        avatarBtnLayout->addWidget(m_avatarPathLabel);
        avatarBtnLayout->addStretch();
        avatarLayout->addLayout(avatarBtnLayout);

        mainLayout->addWidget(avatarGroup);

        // ----------------------------------------
        // 基本信息表单
        // ----------------------------------------
        QFormLayout *form = new QFormLayout();
        form->setSpacing(10);

        // 姓名
        m_nameEdit = new QLineEdit();
        m_nameEdit->setPlaceholderText("请输入真实姓名（2-20字符）");
        form->addRow("姓名 *：", m_nameEdit);

        // 性别
        QHBoxLayout *genderLayout = new QHBoxLayout();
        m_maleRadio = new QRadioButton("男");
        m_femaleRadio = new QRadioButton("女");
        m_maleRadio->setChecked(true);
        QButtonGroup *genderGroup = new QButtonGroup(this);
        genderGroup->addButton(m_maleRadio);
        genderGroup->addButton(m_femaleRadio);
        genderLayout->addWidget(m_maleRadio);
        genderLayout->addWidget(m_femaleRadio);
        genderLayout->addStretch();
        QWidget *genderWidget = new QWidget();
        genderWidget->setLayout(genderLayout);
        form->addRow("性别 *：", genderWidget);

        // 年龄
        m_ageSpinBox = new QSpinBox();
        m_ageSpinBox->setRange(18, 100);
        m_ageSpinBox->setValue(25);
        m_ageSpinBox->setSuffix(" 岁");
        form->addRow("年龄 *：", m_ageSpinBox);

        // 城市
        m_cityCombo = new QComboBox();
        m_cityCombo->addItems({"北京", "上海", "广州", "深圳", "杭州", "成都", "其他"});
        form->addRow("城市 *：", m_cityCombo);

        // 邮箱
        m_emailEdit = new QLineEdit();
        m_emailEdit->setPlaceholderText("example@email.com");
        form->addRow("邮箱 *：", m_emailEdit);

        // 电话
        m_phoneEdit = new QLineEdit();
        m_phoneEdit->setPlaceholderText("11位手机号码");
        form->addRow("电话 *：", m_phoneEdit);

        mainLayout->addLayout(form);

        // ----------------------------------------
        // 爱好选择
        // ----------------------------------------
        QGroupBox *hobbyGroup = new QGroupBox("爱好（至少选一项）*");
        QVBoxLayout *hobbyLayout = new QVBoxLayout(hobbyGroup);
        m_hobby1 = new QCheckBox("读书");
        m_hobby2 = new QCheckBox("运动");
        m_hobby3 = new QCheckBox("音乐");
        m_hobby4 = new QCheckBox("旅行");
        m_hobby5 = new QCheckBox("编程");
        hobbyLayout->addWidget(m_hobby1);
        hobbyLayout->addWidget(m_hobby2);
        hobbyLayout->addWidget(m_hobby3);
        hobbyLayout->addWidget(m_hobby4);
        hobbyLayout->addWidget(m_hobby5);
        mainLayout->addWidget(hobbyGroup);

        // ----------------------------------------
        // 简介
        // ----------------------------------------
        QLabel *bioLabel = new QLabel("个人简介（50-200字）*：");
        mainLayout->addWidget(bioLabel);
        m_bioEdit = new QTextEdit();
        m_bioEdit->setPlaceholderText("请介绍一下自己...");
        m_bioEdit->setMaximumHeight(100);
        mainLayout->addWidget(m_bioEdit);

        // ----------------------------------------
        // 主题色选择（新增：使用 QColorDialog）
        // ----------------------------------------
        QHBoxLayout *themeLayout = new QHBoxLayout();
        QLabel *themeLabel = new QLabel("个性主题色：");
        themeLayout->addWidget(themeLabel);

        m_themeColorBtn = new QPushButton("选择颜色");
        m_themeColorBtn->setFixedWidth(120);
        m_currentColor = QColor("#3498db");  // 默认蓝色
        updateThemeButton();
        connect(m_themeColorBtn, &QPushButton::clicked, this, &RegistrationForm::selectThemeColor);
        themeLayout->addWidget(m_themeColorBtn);
        themeLayout->addStretch();
        mainLayout->addLayout(themeLayout);

        // ----------------------------------------
        // 提交和重置按钮
        // ----------------------------------------
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();

        QPushButton *resetBtn = new QPushButton("重置");
        resetBtn->setStyleSheet(
            "QPushButton { background: #95a5a6; color: white; border: none; "
            "border-radius: 4px; padding: 8px 20px; }"
            "QPushButton:hover { background: #7f8c8d; }"
        );
        connect(resetBtn, &QPushButton::clicked, this, &RegistrationForm::onReset);

        QPushButton *submitBtn = new QPushButton("提交注册");
        submitBtn->setStyleSheet(
            "QPushButton { background: #27ae60; color: white; border: none; "
            "border-radius: 4px; padding: 8px 20px; font-weight: bold; }"
            "QPushButton:hover { background: #219a52; }"
        );
        connect(submitBtn, &QPushButton::clicked, this, &RegistrationForm::onSubmit);

        btnLayout->addWidget(resetBtn);
        btnLayout->addWidget(submitBtn);
        mainLayout->addLayout(btnLayout);
        mainLayout->addStretch();

        scroll->setWidget(content);

        QVBoxLayout *outerLayout = new QVBoxLayout(this);
        outerLayout->setContentsMargins(0, 0, 0, 0);
        outerLayout->addWidget(scroll);
    }

private slots:

    // ----------------------------------------
    // 选择头像（QFileDialog）
    // ----------------------------------------
    void selectAvatar()
    {
        QString file = QFileDialog::getOpenFileName(
            this,
            "选择头像图片",
            QDir::homePath(),
            "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)"
        );

        if (!file.isEmpty()) {
            // 显示缩略图
            QPixmap pixmap(file);
            if (!pixmap.isNull()) {
                m_avatarLabel->setPixmap(
                    pixmap.scaled(76, 76, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                );
                m_avatarLabel->setStyleSheet(
                    "QLabel { border: 2px solid #27ae60; border-radius: 40px; }"
                );
            }
            // 只显示文件名，不显示完整路径
            m_avatarPathLabel->setText(QFileInfo(file).fileName());
            m_avatarPath = file;
        }
    }

    // ----------------------------------------
    // 选择主题色（QColorDialog）
    // ----------------------------------------
    void selectThemeColor()
    {
        QColor color = QColorDialog::getColor(
            m_currentColor,
            this,
            "选择个性主题色"
        );

        if (color.isValid()) {
            m_currentColor = color;
            updateThemeButton();
        }
    }

    // ----------------------------------------
    // 提交注册（QMessageBox::question 确认）
    // ----------------------------------------
    void onSubmit()
    {
        // 第一步：验证表单
        QString error = validateForm();
        if (!error.isEmpty()) {
            QMessageBox::warning(this, "验证失败", error);
            return;
        }

        // 第二步：弹出确认对话框（QMessageBox::question）
        auto reply = QMessageBox::question(
            this,
            "确认提交",
            "确认提交注册信息吗？\n提交后将无法修改。",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );

        if (reply != QMessageBox::Yes) {
            return;
        }

        // 第三步：构建摘要信息
        QString summary = buildSummary();

        // 第四步：显示自定义成功对话框
        RegistrationSummaryDialog dialog(summary, this);
        dialog.exec();
    }

    // ----------------------------------------
    // 重置表单
    // ----------------------------------------
    void onReset()
    {
        // 重置前确认
        auto reply = QMessageBox::question(
            this,
            "确认重置",
            "确定要清空所有已填写的内容吗？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (reply != QMessageBox::Yes) return;

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
        m_avatarLabel->setText("未选择头像");
        m_avatarLabel->setPixmap(QPixmap());
        m_avatarLabel->setStyleSheet(
            "QLabel { border: 2px dashed #bdc3c7; border-radius: 40px; "
            "color: #95a5a6; font-size: 11px; }"
        );
        m_avatarPathLabel->setText("未选择");
        m_avatarPath.clear();
        m_currentColor = QColor("#3498db");
        updateThemeButton();
        m_nameEdit->setFocus();
    }

private:
    // 验证表单，返回错误信息（空字符串表示验证通过）
    QString validateForm()
    {
        QString name = m_nameEdit->text().trimmed();
        if (name.isEmpty() || name.length() < 2 || name.length() > 20)
            return "姓名长度应为 2-20 个字符！";

        QString email = m_emailEdit->text().trimmed();
        if (!email.contains("@") || !email.contains("."))
            return "邮箱格式不正确！";

        QString phone = m_phoneEdit->text().trimmed();
        if (phone.length() != 11 || !phone.toULongLong())
            return "电话号码应为 11 位数字！";

        QStringList hobbies = getSelectedHobbies();
        if (hobbies.isEmpty())
            return "请至少选择一个爱好！";

        QString bio = m_bioEdit->toPlainText().trimmed();
        if (bio.length() < 10)
            return "个人简介至少需要 10 个字符！";

        return "";  // 验证通过
    }

    // 获取已选爱好列表
    QStringList getSelectedHobbies()
    {
        QStringList hobbies;
        if (m_hobby1->isChecked()) hobbies << "读书";
        if (m_hobby2->isChecked()) hobbies << "运动";
        if (m_hobby3->isChecked()) hobbies << "音乐";
        if (m_hobby4->isChecked()) hobbies << "旅行";
        if (m_hobby5->isChecked()) hobbies << "编程";
        return hobbies;
    }

    // 构建注册摘要
    QString buildSummary()
    {
        QString gender = m_maleRadio->isChecked() ? "男" : "女";
        QStringList hobbies = getSelectedHobbies();

        return QString(
            "=== 注册信息摘要 ===\n\n"
            "姓名：%1\n"
            "性别：%2\n"
            "年龄：%3 岁\n"
            "城市：%4\n"
            "邮箱：%5\n"
            "电话：%6\n"
            "爱好：%7\n"
            "头像：%8\n"
            "主题色：%9\n\n"
            "个人简介：\n%10"
        )
        .arg(m_nameEdit->text().trimmed())
        .arg(gender)
        .arg(m_ageSpinBox->value())
        .arg(m_cityCombo->currentText())
        .arg(m_emailEdit->text().trimmed())
        .arg(m_phoneEdit->text().trimmed())
        .arg(hobbies.join("、"))
        .arg(m_avatarPath.isEmpty() ? "未设置" : QFileInfo(m_avatarPath).fileName())
        .arg(m_currentColor.name())
        .arg(m_bioEdit->toPlainText().trimmed());
    }

    // 更新主题色按钮样式
    void updateThemeButton()
    {
        // 根据背景色深浅决定文字颜色
        int brightness = (m_currentColor.red() * 299 +
                          m_currentColor.green() * 587 +
                          m_currentColor.blue() * 114) / 1000;
        QString textColor = brightness > 128 ? "#2c3e50" : "white";

        m_themeColorBtn->setStyleSheet(
            QString("QPushButton { background: %1; color: %2; border: none; "
                    "border-radius: 4px; padding: 6px 12px; }")
                .arg(m_currentColor.name())
                .arg(textColor)
        );
        m_themeColorBtn->setText(m_currentColor.name());
    }

    // 控件成员变量
    QLabel      *m_avatarLabel;
    QLabel      *m_avatarPathLabel;
    QString      m_avatarPath;
    QLineEdit   *m_nameEdit;
    QRadioButton *m_maleRadio;
    QRadioButton *m_femaleRadio;
    QSpinBox    *m_ageSpinBox;
    QComboBox   *m_cityCombo;
    QLineEdit   *m_emailEdit;
    QLineEdit   *m_phoneEdit;
    QCheckBox   *m_hobby1, *m_hobby2, *m_hobby3, *m_hobby4, *m_hobby5;
    QTextEdit   *m_bioEdit;
    QPushButton *m_themeColorBtn;
    QColor       m_currentColor;
};

#include "14_dialog_practice.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    RegistrationForm window;
    window.show();
    return app.exec();
}
