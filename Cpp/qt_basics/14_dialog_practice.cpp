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

// ============================================
// 函数卡片速查
// ============================================
/*
【函数卡片：QFileDialog::getOpenFileName()】

语法：
QString QFileDialog::getOpenFileName(
    QWidget *parent,           // 父窗口（对话框居中显示）
    const QString &caption,    // 对话框标题
    const QString &dir,        // 初始目录
    const QString &filter      // 文件过滤器
)

参数：
- parent  (QWidget*)：父窗口，传 this
- caption (QString)：标题栏文字
- dir     (QString)：初始目录，传 "" 用上次目录，传 QDir::homePath() 用主目录
- filter  (QString)：格式 "描述 (*.ext1 *.ext2);;描述2 (*.ext3)"

返回值：用户选中的文件路径（QString），取消时返回空字符串

示例：
QString path = QFileDialog::getOpenFileName(
    this, "选择图片", QDir::homePath(),
    "图片文件 (*.png *.jpg *.jpeg *.bmp);;所有文件 (*)"
);
if (!path.isEmpty()) { /* 使用 path */ }

────────────────────────────────────────────────────────────

【函数卡片：QColorDialog::getColor()】

语法：
QColor QColorDialog::getColor(
    const QColor &initial,     // 初始颜色
    QWidget *parent,           // 父窗口
    const QString &title       // 对话框标题（可选）
)

返回值：用户选中的 QColor；取消时返回无效颜色（isValid() == false）

示例：
QColor color = QColorDialog::getColor(m_currentColor, this, "选择主题色");
if (color.isValid()) {
    m_currentColor = color;
    updateButtonStyle(color);
}

────────────────────────────────────────────────────────────

【函数卡片：QPixmap::scaled()】

语法：
QPixmap scaled(int width, int height,
               Qt::AspectRatioMode aspectRatioMode = Qt::IgnoreAspectRatio,
               Qt::TransformationMode transformMode = Qt::FastTransformation) const

参数：
- width/height：目标尺寸
- aspectRatioMode：Qt::KeepAspectRatio 保持比例，Qt::IgnoreAspectRatio 拉伸
- transformMode：Qt::SmoothTransformation 平滑缩放（质量好）

示例：
QPixmap pix(avatarPath);
m_avatarLabel->setPixmap(
    pix.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation)
);
*/

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
【错误1】未检查 QFileDialog 返回值就使用路径

❌ 错误代码：
QString path = QFileDialog::getOpenFileName(this, "选择图片", "", "*.png");
QPixmap pix(path);  // 用户取消时 path 为空，QPixmap 加载失败，显示空白

✅ 正确代码：
QString path = QFileDialog::getOpenFileName(this, "选择图片", "", "*.png *.jpg");
if (!path.isEmpty()) {
    QPixmap pix(path);
    m_avatarLabel->setPixmap(pix.scaled(80, 80, Qt::KeepAspectRatio));
}

预防措施：文件对话框返回值必须先判断 isEmpty()。

────────────────────────────────────────────────────────────

【错误2】QColorDialog 取消后使用无效颜色

❌ 错误代码：
QColor color = QColorDialog::getColor(Qt::white, this);
widget->setStyleSheet("background: " + color.name());  // 取消时 color.name() 是 "#000000"（黑色）

✅ 正确代码：
QColor color = QColorDialog::getColor(m_currentColor, this);
if (color.isValid()) {  // 必须检查
    m_currentColor = color;
    applyColor(color);
}

预防措施：getColor() 后必须调用 isValid()。

────────────────────────────────────────────────────────────

【错误3】文件过滤器多扩展名用逗号分隔（无效）

❌ 错误代码：
QFileDialog::getOpenFileName(this, "选图", "", "*.png, *.jpg");
// 逗号被当作文件名的一部分，过滤器不生效

✅ 正确代码：
QFileDialog::getOpenFileName(this, "选图", "",
    "图片 (*.png *.jpg *.jpeg);;所有文件 (*)");
// 同类型扩展名用空格分隔；不同过滤器用 ;; 分隔

预防措施：记住过滤器规则：扩展名空格分隔，过滤器组 ;; 分隔。

────────────────────────────────────────────────────────────

【错误4】对 QDialog exec() 结果未判断就读取数据

❌ 错误代码：
UserInfoDialog dlg(this);
dlg.exec();
QString name = dlg.name();  // 用户点了"取消"，name 可能是空或默认值

✅ 正确代码：
UserInfoDialog dlg(this);
if (dlg.exec() == QDialog::Accepted) {
    QString name = dlg.name();  // 只有用户确认了才读取
}

预防措施：exec() 返回值必须与 QDialog::Accepted 比较。

────────────────────────────────────────────────────────────

【错误5】大图片未缩放直接赋给小 QLabel

❌ 错误代码：
QPixmap pix("large_photo.jpg");  // 3000x4000 像素的大图
m_avatarLabel->setPixmap(pix);   // label 只有 80x80，但图片撑大了 label

✅ 正确代码：
QPixmap pix("large_photo.jpg");
m_avatarLabel->setPixmap(
    pix.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation)
);
// 或者设置 label 不随图片缩放：
m_avatarLabel->setScaledContents(true);
m_avatarLabel->setFixedSize(80, 80);
m_avatarLabel->setPixmap(pix);

预防措施：显示用户选择的图片时，总是先 scaled() 到目标尺寸。
*/

// ============================================
// 练习题
// ============================================
/*
1. 头像裁剪功能
   选择图片后，弹出一个自定义 QDialog，用 QLabel 显示图片
   用户可以拖拽选择一个正方形区域，点"确定"后只保留该区域作为头像
   提示：重写 QLabel 的 mousePressEvent/mouseMoveEvent 绘制选择框

2. 多图上传预览
   将头像选择改为多张图片，用 QScrollArea + QGridLayout 显示缩略图网格
   每张图片右上角有"删除"按钮（小 QPushButton 悬浮在图片上）
   提示：每个缩略图用 QFrame 包装，用 QStackedLayout 叠放图片和删除按钮

3. 主题色实时预览
   选择主题色时，不用等用户点"确定"，颜色对话框实时预览
   QColorDialog::currentColorChanged 信号在用户拖动取色器时持续发出
   提示：connect(dialog, &QColorDialog::currentColorChanged, this, &Form::applyPreview)

4. 表单数据序列化
   点击"提交"后，将所有表单数据保存为 JSON 文件（用 QJsonDocument）
   格式：{"name":"张三","gender":"男","age":25,"city":"北京","hobbies":["读书","运动"]}
   提示：构建 QJsonObject，用 QJsonDocument::toJson() 写入文件
*/
