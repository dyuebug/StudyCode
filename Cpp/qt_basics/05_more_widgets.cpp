/*
 * Qt 更多控件详解
 * 日期：2026-05-30（第39天）
 * 难度：⭐⭐ 中级
 *
 * 学习目标：
 * 1. 掌握 QComboBox（下拉框）的使用
 * 2. 掌握 QCheckBox（复选框）的使用
 * 3. 掌握 QRadioButton（单选按钮）的使用
 * 4. 掌握 QSpinBox（数字输入框）的使用
 *
 * 编译说明：
 * 本文件需要在 Qt Creator 中创建项目后编译
 * 或使用 qmake + make 编译
 */

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QMessageBox>

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【Qt 控件分类】

是什么？
- Qt 提供了丰富的控件（Widgets）用于构建用户界面
- 每个控件都有特定的用途和交互方式

为什么需要？
- 不同的数据需要不同的输入方式
- 提供更好的用户体验
- 限制用户输入，减少错误

核心控件类型：
1. 输入控件：QLineEdit、QTextEdit、QSpinBox
2. 选择控件：QComboBox、QCheckBox、QRadioButton
3. 显示控件：QLabel、QProgressBar
4. 按钮控件：QPushButton、QToolButton
*/

// ============================================
// 1.1 QComboBox（下拉框）⭐⭐⭐⭐⭐
// ============================================

/*
【QComboBox - 下拉框】

是什么？
- 下拉选择框，用户从预定义的选项中选择一个

为什么需要？
- 节省界面空间
- 限制用户输入范围
- 提供清晰的选项列表

核心特点：
- 只能选择一个选项
- 可以添加、删除、清空选项
- 可以获取当前选中的选项
- 可以设置默认选项
*/

/*
【函数卡片：QComboBox::addItem()】

函数名：addItem()
作用：添加一个选项到下拉框
类型：成员函数

完整语法：
void addItem(const QString &text)

参数详解：
1. text（必需）
   - 类型：const QString&
   - 作用：选项的显示文本
   - 示例："北京"、"上海"、"广州"

返回值：无

使用示例：
// 基本用法
QComboBox *combo = new QComboBox();
combo->addItem("选项1");

// 推荐用法：批量添加
combo->addItems({"选项1", "选项2", "选项3"});
*/

/*
【函数卡片：QComboBox::currentText()】

函数名：currentText()
作用：获取当前选中的选项文本
类型：成员函数

完整语法：
QString currentText() const

参数详解：无参数

返回值：
- 返回当前选中选项的文本（QString类型）
- 如果没有选项，返回空字符串

使用示例：
// 基本用法
QString selected = combo->currentText();

// 推荐用法：配合判断
if (!combo->currentText().isEmpty()) {
    QString city = combo->currentText();
    // 处理选中的城市
}
*/

/*
【函数卡片：QComboBox::currentIndexChanged()】

信号名：currentIndexChanged(int index)
作用：当用户选择不同选项时发出信号
类型：信号

参数详解：
1. index（自动传递）
   - 类型：int
   - 作用：新选中选项的索引（从0开始）
   - 示例：0表示第一个选项，1表示第二个选项

使用示例：
// 连接信号和槽
connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [=](int index) {
    QString text = combo->itemText(index);
    qDebug() << "选中了：" << text;
});
*/

// ============================================
// 1.2 QCheckBox（复选框）⭐⭐⭐⭐
// ============================================

/*
【QCheckBox - 复选框】

是什么？
- 带勾选状态的复选框，可同时选中多个

为什么需要？
- 多项独立选择（如：爱好、权限、配置项）
- 每项互相独立，不互斥

核心特点：
- 三种状态：未勾选（Unchecked）、已勾选（Checked）、半选（PartiallyChecked）
- isChecked() 获取是否勾选
- stateChanged(int) 信号在状态改变时触发
*/

/*
【函数卡片：QCheckBox::isChecked()】

函数名：isChecked()
作用：返回复选框是否被勾选
类型：成员函数

完整语法：
bool isChecked() const

返回值：
- true：已勾选
- false：未勾选

使用示例：
QCheckBox *cb = new QCheckBox("同意条款");
if (cb->isChecked()) {
    // 用户同意了
}
*/

/*
【函数卡片：QCheckBox::stateChanged()】

信号名：stateChanged(int state)
作用：复选框状态改变时发出

参数：
- state (int)：Qt::Unchecked=0 / Qt::PartiallyChecked=1 / Qt::Checked=2

使用示例：
connect(checkBox, &QCheckBox::stateChanged, [=](int state) {
    bool checked = (state == Qt::Checked);
    submitBtn->setEnabled(checked);
});
*/

// ============================================
// 1.3 QRadioButton（单选按钮）⭐⭐⭐⭐
// ============================================

/*
【QRadioButton - 单选按钮】

是什么？
- 一组互斥的选择项，同一组内只能选一个

为什么需要？
- 互斥选项（性别、支付方式、优先级等）
- 所有选项可见，方便用户对比

核心特点：
- 同一个父控件下的 QRadioButton 自动互斥
- 需要分组时使用 QButtonGroup
- isChecked() 判断是否选中
*/

/*
【函数卡片：QButtonGroup::addButton()】

函数名：addButton()
作用：将按钮加入按钮组（用于手动分组）
类型：成员函数

完整语法：
void addButton(QAbstractButton *button, int id = -1)

参数：
- button (QAbstractButton*)：要加入的按钮
- id (int, 可选)：按钮的 ID，用于 checkedId() 识别

使用示例：
QButtonGroup *group = new QButtonGroup(this);
group->addButton(radioMale,   0);  // id=0 代表男
group->addButton(radioFemale, 1);  // id=1 代表女

int selected = group->checkedId();  // 获取选中按钮的 id
*/

// ============================================
// 1.4 QSpinBox（数字输入框）⭐⭐⭐⭐
// ============================================

/*
【QSpinBox - 整数数字输入框】

是什么？
- 带上下箭头的整数输入框

为什么需要？
- 限制数字范围，避免非法输入
- 提供直观的增减操作

核心特点：
- setRange(min, max) 设置范围
- setValue(n) / value() 设置/获取当前值
- valueChanged(int) 信号在值改变时触发
- QDoubleSpinBox 用于浮点数
*/

/*
【函数卡片：QSpinBox::setRange()】

函数名：setRange()
作用：设置数字输入框的最小值和最大值
类型：成员函数

完整语法：
void setRange(int minimum, int maximum)

参数：
- minimum (int)：最小值（含）
- maximum (int)：最大值（含）

使用示例：
QSpinBox *spin = new QSpinBox();
spin->setRange(1, 100);   // 1~100
spin->setValue(18);        // 默认值 18
spin->setSuffix(" 岁");    // 单位后缀
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

/*
以下为 Qt Creator 项目中的完整用法示例（伪代码）：

void WidgetDemo::setupWidgets()
{
    // ---- QComboBox 示例 ----
    QComboBox *cityCombo = new QComboBox(this);
    cityCombo->addItems({"北京", "上海", "广州", "深圳"});
    cityCombo->setCurrentIndex(0);  // 默认选第一项

    // 获取选中值
    QString city = cityCombo->currentText();  // "北京"

    // 监听变化
    connect(cityCombo, &QComboBox::currentTextChanged,
            this, [=](const QString &text) {
        statusLabel->setText("当前城市：" + text);
    });

    // ---- QCheckBox 示例 ----
    QCheckBox *cbMusic  = new QCheckBox("音乐", this);
    QCheckBox *cbSport  = new QCheckBox("运动", this);
    QCheckBox *cbReading = new QCheckBox("阅读", this);
    cbMusic->setChecked(true);  // 默认勾选

    // 读取勾选状态
    QStringList hobbies;
    if (cbMusic->isChecked())   hobbies << "音乐";
    if (cbSport->isChecked())   hobbies << "运动";
    if (cbReading->isChecked()) hobbies << "阅读";

    // ---- QRadioButton + QButtonGroup 示例 ----
    QRadioButton *radioMale   = new QRadioButton("男", this);
    QRadioButton *radioFemale = new QRadioButton("女", this);
    radioMale->setChecked(true);  // 默认选男

    QButtonGroup *genderGroup = new QButtonGroup(this);
    genderGroup->addButton(radioMale,   0);
    genderGroup->addButton(radioFemale, 1);

    // 获取选中性别
    int genderId = genderGroup->checkedId();  // 0=男 1=女

    // ---- QSpinBox 示例 ----
    QSpinBox *ageSpin = new QSpinBox(this);
    ageSpin->setRange(1, 120);
    ageSpin->setValue(18);
    ageSpin->setSuffix(" 岁");

    // 获取当前值
    int age = ageSpin->value();

    // 监听变化
    connect(ageSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [=](int val) {
        if (val < 18) {
            warningLabel->setText("未成年");
        } else {
            warningLabel->clear();
        }
    });
}
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
【错误1】QRadioButton 未分组导致全局互斥

❌ 错误代码：
// 想要两组独立的单选按钮
QRadioButton *rA1 = new QRadioButton("A1", this);
QRadioButton *rA2 = new QRadioButton("A2", this);
QRadioButton *rB1 = new QRadioButton("B1", this);
QRadioButton *rB2 = new QRadioButton("B2", this);
// 所有按钮都在同一个父控件 this 下，4个按钮全局互斥！

结果：本意是两组各自互斥，实际上四个按钮只能选一个。

✅ 正确代码：
// 用 QGroupBox 或 QButtonGroup 隔离分组
QGroupBox *groupA = new QGroupBox("组A", this);
QRadioButton *rA1 = new QRadioButton("A1", groupA);  // 父控件是 groupA
QRadioButton *rA2 = new QRadioButton("A2", groupA);

QGroupBox *groupB = new QGroupBox("组B", this);
QRadioButton *rB1 = new QRadioButton("B1", groupB);  // 父控件是 groupB
QRadioButton *rB2 = new QRadioButton("B2", groupB);
// 两组各自独立互斥

预防措施：多组单选按钮，必须用 QGroupBox 或 QButtonGroup 隔离。

────────────────────────────────────────────────────────────

【错误2】QComboBox 索引越界

❌ 错误代码：
QComboBox *combo = new QComboBox();
// 忘记添加选项，直接读取
int idx = combo->currentIndex();    // 返回 -1
QString text = combo->currentText(); // 返回空字符串，但后续代码未检查
doSomethingWith(text);  // 传入空字符串导致逻辑错误

✅ 正确代码：
if (combo->count() > 0 && combo->currentIndex() >= 0) {
    QString text = combo->currentText();
    doSomethingWith(text);
}

预防措施：使用 currentIndex() 前判断是否 >= 0；使用 count() 判断是否有选项。

────────────────────────────────────────────────────────────

【错误3】混用 currentIndexChanged(int) 的两种重载

❌ 错误代码：
// Qt 5 中 QComboBox 有两个同名信号：
// currentIndexChanged(int)   和
// currentIndexChanged(const QString &)
// 直接写函数指针会报歧义错误：
connect(combo, &QComboBox::currentIndexChanged, this, &MyClass::onChanged);
// 编译错误：overloaded signal

✅ 正确代码：
// 用 QOverload 消除歧义
connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [=](int index) { /* 用 index */ });

// Qt 6 已改为 currentIndexChanged(int)，无歧义，可直接写：
connect(combo, &QComboBox::currentIndexChanged, this, &MyClass::onChanged);

预防措施：Qt 5 项目中，连接有重载的信号时必须用 QOverload 指定版本。

────────────────────────────────────────────────────────────

【错误4】QSpinBox 未设置范围导致默认值超出预期

❌ 错误代码：
QSpinBox *spin = new QSpinBox();
// 忘记 setRange，默认范围是 0~99
spin->setValue(150);  // 会被截断为 99，不会报错！

结果：用户看到 99，以为输入了 150，数据静默截断。

✅ 正确代码：
QSpinBox *spin = new QSpinBox();
spin->setRange(0, 9999);   // 先设范围
spin->setValue(150);        // 再设值，150 在范围内，正常显示

预防措施：创建 QSpinBox 后立即调用 setRange()，再调用 setValue()。

────────────────────────────────────────────────────────────

【错误5】QCheckBox 的 stateChanged 信号参数类型误用

❌ 错误代码：
// stateChanged 传递的是 int，不是 bool
connect(cb, &QCheckBox::stateChanged, this, [=](bool checked) {
    // 编译可能通过（int 隐式转换 bool），但语义不准确
    // Qt::PartiallyChecked (1) 会被当成 true
});

✅ 正确代码：
connect(cb, &QCheckBox::stateChanged, this, [=](int state) {
    bool checked = (state == Qt::Checked);  // 明确判断
    btn->setEnabled(checked);
});

// 或者直接用 toggled(bool) 信号（只有 Checked/Unchecked 两态时推荐）：
connect(cb, &QCheckBox::toggled, this, [=](bool checked) {
    btn->setEnabled(checked);
});

预防措施：双态复选框用 toggled(bool)；三态复选框用 stateChanged(int) + 显式判断。

────────────────────────────────────────────────────────────

【错误6】addItems() 传入空列表不报错但索引变为 -1

❌ 错误代码：
QStringList items;  // 空列表（比如从数据库查询结果为空）
combo->addItems(items);
// combo->currentIndex() == -1
// 后续代码假设 index >= 0，产生逻辑错误

✅ 正确代码：
if (items.isEmpty()) {
    combo->setEnabled(false);
    combo->addItem("（无可用选项）");
} else {
    combo->addItems(items);
}

预防措施：动态填充 QComboBox 时，先检查数据源是否为空。

────────────────────────────────────────────────────────────

【错误7】QButtonGroup 未设置父对象导致内存泄漏

❌ 错误代码：
void MyWidget::setupUI()
{
    QButtonGroup *group = new QButtonGroup();  // 无父对象！
    group->addButton(radio1);
    group->addButton(radio2);
    // MyWidget 销毁时，group 不会自动销毁 → 内存泄漏
}

✅ 正确代码：
QButtonGroup *group = new QButtonGroup(this);  // this 作为父对象
// MyWidget 销毁时，group 自动销毁

预防措施：所有 QObject 子类实例化时，尽量传入父对象指针。
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
1. 城市选择器
   创建一个窗口，包含：
   - QComboBox，选项为"北京/上海/广州/深圳"
   - QLabel，实时显示"当前选择：XXX"
   要求：每次切换城市，Label 立即更新
   提示：使用 currentTextChanged 信号

2. 用户注册表单
   创建包含以下控件的注册表单：
   - QLineEdit：用户名
   - 性别单选（男/女），用 QButtonGroup 分组
   - 年龄 QSpinBox（范围 1~120）
   - 爱好多选（至少3项 QCheckBox）
   - QPushButton "提交"：点击后用 QMessageBox 显示所有填写内容
   提示：提交时遍历所有复选框的 isChecked()

3. 动态启用控制
   实现以下逻辑：
   - 一个 QCheckBox "启用高级选项"
   - 一组控件（QSpinBox + QComboBox），默认禁用（setEnabled(false)）
   - 勾选复选框时，那组控件变为可用；取消勾选时再次禁用
   提示：使用 toggled(bool) 信号连接控件的 setEnabled(bool) 槽

4. 计数范围验证
   创建两个 QSpinBox（最小值、最大值），要求：
   - "最小值" SpinBox 的最大限制始终 = "最大值" SpinBox 的当前值
   - "最大值" SpinBox 的最小限制始终 = "最小值" SpinBox 的当前值
   - 即：用户无法设置最小值 > 最大值
   提示：在 valueChanged 信号中动态调用对方的 setMinimum/setMaximum

5. 综合思考
   QComboBox 的 setEditable(true) 允许用户手动输入不在列表中的值。
   思考并回答：
   a. 此时 currentText() 返回什么？
   b. 如何验证用户输入的值是否在预设选项中？
   c. 如何在用户输入完成（按回车）时触发验证逻辑？
   （不需要写代码，文字描述思路即可）
*/
