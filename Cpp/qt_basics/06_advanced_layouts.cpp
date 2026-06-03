/*
 * Qt 高级布局管理详解
 * 日期：2026-05-30（第39天）
 * 难度：⭐⭐ 中级
 *
 * 学习目标：
 * 1. 掌握 QGridLayout（网格布局）
 * 2. 掌握 QFormLayout（表单布局）
 * 3. 理解布局嵌套技巧
 */

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【Qt 布局管理器】

是什么？
- 布局管理器（Layout Manager）负责自动排列和调整控件的位置与大小
- 窗口大小改变时，布局管理器自动重新计算所有控件的位置

为什么需要？
- 避免手动计算绝对坐标（难以维护，换分辨率就乱）
- 自适应窗口大小变化
- 跨平台字体/DPI 差异自动适配

四种核心布局：
1. QHBoxLayout  — 水平排列（左→右）
2. QVBoxLayout  — 垂直排列（上→下）
3. QGridLayout  — 网格排列（行列定位），适合计算器、表格型界面
4. QFormLayout  — 标签+控件配对（左标签右输入），适合表单
*/

/*
【函数卡片：QGridLayout::addWidget()】

函数名：addWidget()（网格版）
作用：将控件添加到网格布局的指定位置，可跨行跨列

完整语法：
void addWidget(QWidget *widget, int row, int column,
               int rowSpan = 1, int columnSpan = 1,
               Qt::Alignment alignment = Qt::Alignment())

参数详解：
1. widget (QWidget*)：要添加的控件
2. row (int)：起始行（从 0 开始）
3. column (int)：起始列（从 0 开始）
4. rowSpan (int, 可选)：跨几行，默认 1
5. columnSpan (int, 可选)：跨几列，默认 1
6. alignment (Qt::Alignment, 可选)：控件在格子内的对齐方式

返回值：无

示例：
QGridLayout *grid = new QGridLayout(this);
grid->addWidget(display, 0, 0, 1, 4);  // 第0行第0列，跨1行4列（横跨整行）
grid->addWidget(btn,     1, 2);         // 第1行第2列，占1格
*/

/*
【函数卡片：QFormLayout::addRow()】

函数名：addRow()
作用：向表单布局添加一行（标签 + 控件）

完整语法（三种重载）：
void addRow(const QString &labelText, QWidget *field)
void addRow(QWidget *label, QWidget *field)
void addRow(QWidget *widget)  // 整行只有一个控件（无标签）

参数详解：
1. labelText (QString)：标签文字，自动创建 QLabel
2. field (QWidget*)：右侧的输入控件

返回值：无

示例：
QFormLayout *form = new QFormLayout();
form->addRow("姓名：", new QLineEdit());    // 自动创建标签
form->addRow("年龄：", new QSpinBox());
form->setLabelAlignment(Qt::AlignRight);   // 标签右对齐（推荐）
*/

/*
【函数卡片：QLayout::addStretch()】

函数名：addStretch()
作用：在布局中插入弹性空间，将控件推向一侧

完整语法：
void QBoxLayout::addStretch(int stretch = 0)

参数：
- stretch (int)：伸缩因子，默认 0（与其他 stretch 按比例分配空间）

使用场景：
// 按钮靠右对齐
QHBoxLayout *btnLayout = new QHBoxLayout();
btnLayout->addStretch();    // 左侧弹性空间，将按钮推向右边
btnLayout->addWidget(okBtn);
btnLayout->addWidget(cancelBtn);
*/

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QComboBox>

// ============================================
// 示例1：QGridLayout（网格布局）
// ============================================

class GridLayoutDemo : public QWidget
{
    Q_OBJECT

public:
    GridLayoutDemo(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("QGridLayout 演示 - 计算器布局");
        resize(300, 400);

        // 创建网格布局
        QGridLayout *gridLayout = new QGridLayout(this);

        // 显示屏（跨4列）
        QLineEdit *display = new QLineEdit("0");
        display->setReadOnly(true);
        display->setAlignment(Qt::AlignRight);
        display->setStyleSheet("QLineEdit { font-size: 24px; padding: 10px; }");
        gridLayout->addWidget(display, 0, 0, 1, 4);  // 第0行，第0列，跨1行4列

        // 按钮布局（4x4网格）
        QStringList buttons = {
            "7", "8", "9", "/",
            "4", "5", "6", "*",
            "1", "2", "3", "-",
            "0", ".", "=", "+"
        };

        int row = 1;
        int col = 0;
        for (const QString &text : buttons) {
            QPushButton *btn = new QPushButton(text);
            btn->setMinimumSize(60, 60);
            btn->setStyleSheet("QPushButton { font-size: 18px; }");
            gridLayout->addWidget(btn, row, col);

            col++;
            if (col == 4) {
                col = 0;
                row++;
            }
        }

        // 设置间距
        gridLayout->setSpacing(5);
        gridLayout->setContentsMargins(10, 10, 10, 10);
    }
};

// ============================================
// 示例2：QFormLayout（表单布局）
// ============================================

class FormLayoutDemo : public QWidget
{
    Q_OBJECT

public:
    FormLayoutDemo(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("QFormLayout 演示 - 用户信息表单");
        resize(400, 300);

        // 创建主布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        // 标题
        QLabel *title = new QLabel("用户信息表单");
        title->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; }");
        title->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(title);

        mainLayout->addSpacing(20);

        // 创建表单布局
        QFormLayout *formLayout = new QFormLayout();

        // 添加表单行
        QLineEdit *nameEdit = new QLineEdit();
        formLayout->addRow("姓名：", nameEdit);

        QSpinBox *ageSpinBox = new QSpinBox();
        ageSpinBox->setRange(18, 100);
        ageSpinBox->setValue(25);
        formLayout->addRow("年龄：", ageSpinBox);

        QComboBox *genderCombo = new QComboBox();
        genderCombo->addItems({"男", "女"});
        formLayout->addRow("性别：", genderCombo);

        QLineEdit *emailEdit = new QLineEdit();
        emailEdit->setPlaceholderText("example@email.com");
        formLayout->addRow("邮箱：", emailEdit);

        QLineEdit *phoneEdit = new QLineEdit();
        phoneEdit->setPlaceholderText("13800138000");
        formLayout->addRow("电话：", phoneEdit);

        QComboBox *cityCombo = new QComboBox();
        cityCombo->addItems({"北京", "上海", "广州", "深圳"});
        formLayout->addRow("城市：", cityCombo);

        QTextEdit *bioEdit = new QTextEdit();
        bioEdit->setMaximumHeight(80);
        bioEdit->setPlaceholderText("请输入个人简介...");
        formLayout->addRow("简介：", bioEdit);

        // 设置标签对齐方式
        formLayout->setLabelAlignment(Qt::AlignRight);

        mainLayout->addLayout(formLayout);

        mainLayout->addSpacing(20);

        // 按钮布局
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();

        QPushButton *submitBtn = new QPushButton("提交");
        QPushButton *resetBtn = new QPushButton("重置");

        btnLayout->addWidget(submitBtn);
        btnLayout->addWidget(resetBtn);

        mainLayout->addLayout(btnLayout);
    }
};

// ============================================
// 示例3：布局嵌套
// ============================================

class NestedLayoutDemo : public QWidget
{
    Q_OBJECT

public:
    NestedLayoutDemo(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("布局嵌套演示");
        resize(500, 400);

        // 主垂直布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        // 顶部：水平布局（标题 + 按钮）
        QHBoxLayout *topLayout = new QHBoxLayout();
        QLabel *title = new QLabel("嵌套布局示例");
        title->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; }");
        QPushButton *closeBtn = new QPushButton("关闭");
        closeBtn->setMaximumWidth(80);

        topLayout->addWidget(title);
        topLayout->addStretch();
        topLayout->addWidget(closeBtn);

        mainLayout->addLayout(topLayout);

        // 中间：网格布局（输入区域）
        QGridLayout *gridLayout = new QGridLayout();

        gridLayout->addWidget(new QLabel("用户名："), 0, 0);
        gridLayout->addWidget(new QLineEdit(), 0, 1);

        gridLayout->addWidget(new QLabel("密码："), 1, 0);
        QLineEdit *pwdEdit = new QLineEdit();
        pwdEdit->setEchoMode(QLineEdit::Password);
        gridLayout->addWidget(pwdEdit, 1, 1);

        gridLayout->addWidget(new QLabel("确认密码："), 2, 0);
        QLineEdit *confirmPwdEdit = new QLineEdit();
        confirmPwdEdit->setEchoMode(QLineEdit::Password);
        gridLayout->addWidget(confirmPwdEdit, 2, 1);

        mainLayout->addLayout(gridLayout);

        // 中间：表单布局（详细信息）
        QFormLayout *formLayout = new QFormLayout();
        formLayout->addRow("邮箱：", new QLineEdit());
        formLayout->addRow("电话：", new QLineEdit());

        mainLayout->addLayout(formLayout);

        mainLayout->addStretch();

        // 底部：水平布局（按钮组）
        QHBoxLayout *bottomLayout = new QHBoxLayout();
        bottomLayout->addStretch();
        bottomLayout->addWidget(new QPushButton("提交"));
        bottomLayout->addWidget(new QPushButton("取消"));

        mainLayout->addLayout(bottomLayout);

        // 连接关闭按钮
        connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    }
};

// ============================================
// 主程序：显示所有演示
// ============================================

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 创建并显示三个演示窗口
    GridLayoutDemo gridDemo;
    gridDemo.move(100, 100);
    gridDemo.show();

    FormLayoutDemo formDemo;
    formDemo.move(450, 100);
    formDemo.show();

    NestedLayoutDemo nestedDemo;
    nestedDemo.move(250, 550);
    nestedDemo.show();

    return app.exec();
}

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
【错误1】给已有布局的控件再设置布局

❌ 错误代码：
QWidget *widget = new QWidget();
QVBoxLayout *layout1 = new QVBoxLayout(widget);  // widget 已有布局
QHBoxLayout *layout2 = new QHBoxLayout(widget);  // 再次设置，Qt 打印警告

结果：Qt 输出警告 "QWidget::setLayout: Attempting to set QLayout ... on ..., which already has a layout"，第二个布局无效。

✅ 正确代码：
QWidget *widget = new QWidget();
QVBoxLayout *mainLayout = new QVBoxLayout(widget);  // 只设一个顶层布局

// 嵌套子布局时，用 addLayout()，不传父控件：
QHBoxLayout *subLayout = new QHBoxLayout();  // 不传 widget！
subLayout->addWidget(btn1);
subLayout->addWidget(btn2);
mainLayout->addLayout(subLayout);  // 嵌套进主布局

预防措施：顶层布局传父控件；子布局不传父控件，用 addLayout() 嵌套。

────────────────────────────────────────────────────────────

【错误2】QGridLayout 行列索引写错导致控件重叠

❌ 错误代码：
QGridLayout *grid = new QGridLayout(this);
grid->addWidget(labelA, 0, 0);
grid->addWidget(labelB, 0, 0);  // 同一格放了两个控件！

结果：两个控件叠在同一格，只显示最后添加的，前一个被遮住。Qt 不报错。

✅ 正确代码：
grid->addWidget(labelA, 0, 0);  // 第0行第0列
grid->addWidget(labelB, 0, 1);  // 第0行第1列

预防措施：addWidget 前画出行列草图，或用注释标注行列意图。

────────────────────────────────────────────────────────────

【错误3】addStretch() 位置错误导致控件排列不符合预期

❌ 错误代码：
QHBoxLayout *layout = new QHBoxLayout();
layout->addWidget(okBtn);
layout->addWidget(cancelBtn);
layout->addStretch();  // stretch 在最后 → 按钮靠左，右边是空白

// 想要按钮靠右，但 stretch 加在后面没有效果（已经靠左了）

✅ 正确代码：
QHBoxLayout *layout = new QHBoxLayout();
layout->addStretch();        // stretch 在前 → 将按钮推向右侧
layout->addWidget(okBtn);
layout->addWidget(cancelBtn);

预防措施：stretch 要加在想留空的那一侧，即"空间在左→按钮靠右"。

────────────────────────────────────────────────────────────

【错误4】QFormLayout 的标签对齐未设置，Windows/Mac 表现不一致

❌ 问题：
// 不同平台默认对齐方式不同：
// Windows：标签左对齐
// macOS：标签右对齐
// 导致同一份代码在不同平台看起来不一样

✅ 正确代码：
QFormLayout *form = new QFormLayout();
form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);  // 显式指定，跨平台一致

预防措施：表单布局总是显式调用 setLabelAlignment()，不依赖平台默认值。

────────────────────────────────────────────────────────────

【错误5】columnSpan 超出实际列数导致布局异常

❌ 错误代码：
QGridLayout *grid = new QGridLayout(this);
// 只添加了 3 列的控件
grid->addWidget(btn1, 0, 0);
grid->addWidget(btn2, 0, 1);
grid->addWidget(btn3, 0, 2);
// 试图让 display 跨 5 列（实际只有 3 列）
grid->addWidget(display, 1, 0, 1, 5);  // columnSpan=5，但只有 3 列

结果：display 只会跨实际存在的列数，不会报错，但布局可能与预期不符。

✅ 正确代码：
// 先确定网格的实际列数，再设置 span
int totalColumns = 4;
grid->addWidget(display, 0, 0, 1, totalColumns);  // span 与实际列数一致

预防措施：columnSpan 不要超过网格的实际列数；用常量统一管理列数。

────────────────────────────────────────────────────────────

【错误6】忘记将子布局添加到父布局（布局孤立）

❌ 错误代码：
QVBoxLayout *mainLayout = new QVBoxLayout(this);
QHBoxLayout *btnLayout = new QHBoxLayout();
btnLayout->addWidget(okBtn);
btnLayout->addWidget(cancelBtn);
// 忘记 mainLayout->addLayout(btnLayout)！

结果：按钮创建了，但不显示，btnLayout 是孤立的，没有挂到界面上。

✅ 正确代码：
mainLayout->addLayout(btnLayout);  // 必须添加到父布局

预防措施：创建子布局后立即写 addLayout()，不要留到后面忘记。
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
1. 计算器布局重现
   用 QGridLayout 实现一个 4×4 的计算器按钮区域：
   - 第 0 行：显示屏，横跨全部 4 列
   - 第 1-4 行：16 个按钮（7 8 9 / | 4 5 6 * | 1 2 3 - | 0 . = +）
   要求：每个按钮最小尺寸 60×60，按钮间距为 5px
   提示：用二维索引 row = i/4 + 1，col = i%4

2. 响应式表单
   用 QFormLayout 创建一个"个人资料"表单：
   - 至少包含：姓名（QLineEdit）、年龄（QSpinBox）、城市（QComboBox）、简介（QTextEdit）
   - 标签右对齐
   - 底部有"保存"和"取消"两个按钮，按钮靠右对齐
   提示：按钮行用 QHBoxLayout + addStretch() 实现靠右

3. 嵌套布局设计
   实现一个"登录窗口"，布局要求：
   - 顶部：居中显示标题"用户登录"（QLabel）
   - 中间：QFormLayout，包含用户名和密码输入
   - 底部：登录按钮（居中）+ 下方小字"忘记密码？"（居中）
   提示：顶部和底部用 QHBoxLayout + addStretch() 实现居中

4. 布局伸缩比例
   用 QHBoxLayout 实现三栏布局：
   - 左栏占 1 份宽度（导航菜单）
   - 中栏占 3 份宽度（主内容区）
   - 右栏占 1 份宽度（侧边栏）
   提示：使用 addWidget(widget, stretch) 的第二个参数设置伸缩比例

5. 思考题
   以下两种写法有什么区别？哪种更推荐？为什么？
   写法A：QVBoxLayout *layout = new QVBoxLayout(this);
   写法B：QVBoxLayout *layout = new QVBoxLayout();
           this->setLayout(layout);
*/

#include "06_advanced_layouts.moc"
