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

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【表单验证设计模式】

是什么？
- 在提交数据前，逐项检查用户输入是否合法

核心策略：
1. 即时验证：输入时实时反馈（textChanged 信号）
2. 提交验证：点击提交时统一校验（本文件采用的方式）
3. 混合验证：两者结合

本项目验证规则：
- 姓名：非空，2-20 字符
- 年龄：18-100（QSpinBox 已限制，只需语义验证）
- 邮箱：包含 @ 和 .（简单格式检查）
- 电话：11 位
- 爱好：至少选一项
- 简介：50-200 字

【函数卡片：QString::trimmed()】

函数名：trimmed()
作用：去除字符串首尾的空白字符（空格、制表符、换行）
返回值：QString（处理后的新字符串，原字符串不变）

示例：
QString raw = "  张三  ";
QString clean = raw.trimmed();  // "张三"
// 验证前必须 trimmed，防止用户只输入空格通过验证

────────────────────────────────────────────────────────────

【函数卡片：QTextEdit::toPlainText()】

函数名：toPlainText()
作用：获取 QTextEdit 中的纯文本内容（去除 HTML 格式）
返回值：QString

对比：
- toPlainText()：纯文本，用于文字内容处理
- toHtml()：带 HTML 标签，用于富文本保存

示例：
QString bio = m_bioEdit->toPlainText().trimmed();
int len = bio.length();  // 字符数

────────────────────────────────────────────────────────────

【函数卡片：QString::arg()】

函数名：arg()
作用：格式化字符串，用实际值替换占位符 %1、%2...%9

语法：
QString QString::arg(const QString &a) const
QString QString::arg(int a) const
// 可链式调用

示例：
QString info = QString("姓名：%1，年龄：%2 岁").arg(name).arg(age);
// 链式替换，%1 → name，%2 → age
// 比 + 拼接更安全，不会有隐式类型转换问题
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
【错误1】验证前未 trimmed，空格输入通过验证

❌ 错误代码：
QString name = m_nameEdit->text();
if (name.isEmpty()) { ... }  // 用户输入 "   " 三个空格，isEmpty() 返回 false，通过验证！

✅ 正确代码：
QString name = m_nameEdit->text().trimmed();  // 先去首尾空格
if (name.isEmpty()) { ... }

预防措施：所有文本输入验证前必须调用 trimmed()。

────────────────────────────────────────────────────────────

【错误2】验证失败后忘记 return，导致多条错误提示连续弹出

❌ 错误代码：
void onSubmit()
{
    if (m_nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入姓名");
        // 忘记 return！继续执行下面的验证
    }
    if (m_emailEdit->text().isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入邮箱");
        // 用户连续收到多个弹框
    }
}

✅ 正确代码：
if (name.isEmpty()) {
    QMessageBox::warning(this, "错误", "请输入姓名");
    m_nameEdit->setFocus();
    return;  // 立即返回，只提示第一个错误
}

预防措施：每个验证失败分支后必须 return，一次只提示一个错误。

────────────────────────────────────────────────────────────

【错误3】电话号码验证只检查长度，未检查是否全为数字

❌ 错误代码：
if (phone.length() != 11) { ... }
// 通过验证：用户输入 "abc123456789"（长度11但含字母）

✅ 正确代码：
QRegularExpression re("^[0-9]{11}$");
if (!re.match(phone).hasMatch()) {
    QMessageBox::warning(this, "错误", "电话号码必须是11位数字");
    return;
}

预防措施：数字格式验证用正则，不只是检查长度。

────────────────────────────────────────────────────────────

【错误4】重置表单时漏掉某些控件

❌ 错误代码：
void onReset()
{
    m_nameEdit->clear();
    m_emailEdit->clear();
    // 忘记重置 m_cityCombo、m_hobby* 等
}
// 用户点重置，部分字段未清空

✅ 正确代码：
// 用列表统一管理所有需要重置的控件
void onReset()
{
    // 逐一重置所有字段
    m_nameEdit->clear();
    m_maleRadio->setChecked(true);
    m_ageSpinBox->setValue(25);
    m_cityCombo->setCurrentIndex(0);
    m_emailEdit->clear();
    m_phoneEdit->clear();
    for (QCheckBox *cb : {m_hobby1, m_hobby2, m_hobby3, m_hobby4, m_hobby5})
        cb->setChecked(false);
    m_bioEdit->clear();
    m_nameEdit->setFocus();
}

预防措施：添加新表单字段时，同步更新 onReset() 函数。

────────────────────────────────────────────────────────────

【错误5】QButtonGroup 的 checkedButton() 返回 nullptr 未检查

❌ 错误代码：
// 没有默认选中任何按钮时
QAbstractButton *btn = m_genderGroup->checkedButton();
QString gender = btn->text();  // btn 是 nullptr，崩溃！

✅ 正确代码：
QAbstractButton *btn = m_genderGroup->checkedButton();
QString gender = btn ? btn->text() : "未选择";

// 更好的做法：UI 初始化时设置默认选中
m_maleRadio->setChecked(true);  // 确保始终有默认值

预防措施：checkedButton() 返回值必须检查 nullptr；UI 初始化时设好默认选中项。

────────────────────────────────────────────────────────────

【错误6】用 + 拼接字符串，整数类型导致隐式转换问题

❌ 错误代码：
int age = m_ageSpinBox->value();
QString msg = "年龄：" + age;  // 编译错误：int 不能直接加 QString
// 或更危险：
QString msg = "年龄：" + QString(age);  // QString(int) 构造的是 Unicode 字符，不是数字字符串！

✅ 正确代码：
QString msg = "年龄：" + QString::number(age);  // 正确：数字转字符串
// 或用 arg()（推荐）：
QString msg = QString("年龄：%1 岁").arg(age);

预防措施：整数转字符串用 QString::number() 或 .arg()，不要用 QString(int)。
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
1. 即时验证增强
   为姓名输入框添加即时验证：
   - 连接 textChanged 信号
   - 输入不足 2 字符时，旁边显示红色提示文字"姓名至少 2 个字符"
   - 输入合法时，提示文字消失
   提示：在姓名行旁边加一个 QLabel，动态设置文字和颜色

2. 提交按钮状态联动
   实现"提交"按钮的智能启用/禁用：
   - 初始状态：禁用（submitBtn->setEnabled(false)）
   - 实时检测：姓名、邮箱、电话均非空时，启用提交按钮
   - 任意一项为空时，重新禁用
   提示：连接三个 QLineEdit 的 textChanged 信号，每次变化都检查三项

3. 字数统计
   为"个人简介" QTextEdit 添加字数统计：
   - 下方显示"当前字数：0 / 50-200"
   - 字数不足 50 时文字显示红色
   - 字数在 50-200 时显示绿色
   - 超过 200 时显示红色并阻止继续输入
   提示：连接 textChanged 信号，用 toPlainText().length() 获取字数

4. 数据持久化思考
   当前表单数据只在内存中，程序关闭后丢失。
   思考：如何将注册信息保存到文件？
   a. 用 QFile + QTextStream 保存为 CSV 格式，格式为：
      姓名,性别,年龄,城市,邮箱,电话,爱好,简介
   b. 如何处理简介中可能包含的逗号？
   （不需要写完整代码，描述思路即可）

5. 表单扩展设计
   思考以下需求该如何实现（不需要写代码）：
   - 添加"头像上传"功能：点击按钮选择图片文件，预览缩略图
   - 添加"密码"和"确认密码"字段：两次输入必须一致
   - 添加"省份-城市"二级联动下拉：选省份后，城市下拉自动更新
   对每项需求，描述会用到哪些 Qt 类和信号槽。
*/

#include "07_registration_form.moc"
