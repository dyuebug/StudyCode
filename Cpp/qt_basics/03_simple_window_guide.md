# 简单的 Qt 窗口应用 - 问候程序

## 📝 项目说明

这是一个简单的 Qt 窗口应用，演示了：
- QWidget 基础
- 基本控件（QPushButton、QLabel、QLineEdit）
- 布局管理（QVBoxLayout）
- 信号与槽机制

## 🎯 功能描述

创建一个问候程序：
1. 用户在文本框中输入姓名
2. 点击"问候"按钮
3. 标签显示问候信息："你好，[姓名]！"

## 🛠️ 创建步骤

### 步骤1：创建项目

1. 打开 Qt Creator
2. 文件 -> 新建文件或项目
3. 选择 "Application (Qt)" -> "Qt Widgets Application"
4. 项目名称：GreetingApp
5. 选择 Kit：Desktop Qt 5.15.2 MinGW 64-bit
6. 基类：QWidget（不是 QMainWindow）
7. 完成

### 步骤2：项目结构

```
GreetingApp/
├── GreetingApp.pro      # 项目配置文件
├── main.cpp             # 主程序入口
├── widget.h             # 窗口头文件
├── widget.cpp           # 窗口实现
└── widget.ui            # 界面设计文件（可选）
```

### 步骤3：编写代码

#### main.cpp

```cpp
#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
    return a.exec();
}
```

#### widget.h

```cpp
#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void onGreetButtonClicked();  // 槽函数：处理按钮点击

private:
    // 控件指针
    QLineEdit *m_nameEdit;        // 文本框
    QPushButton *m_greetButton;   // 按钮
    QLabel *m_resultLabel;        // 标签
};

#endif // WIDGET_H
```

#### widget.cpp

```cpp
#include "widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    // 设置窗口标题和大小
    setWindowTitle("问候程序");
    resize(400, 200);

    // 创建控件
    QLabel *promptLabel = new QLabel("请输入你的姓名：");
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("在这里输入姓名");
    
    m_greetButton = new QPushButton("问候");
    m_resultLabel = new QLabel("等待输入...");
    m_resultLabel->setStyleSheet("QLabel { color: blue; font-size: 16px; }");

    // 创建布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(promptLabel);
    layout->addWidget(m_nameEdit);
    layout->addWidget(m_greetButton);
    layout->addWidget(m_resultLabel);
    layout->addStretch();  // 添加弹性空间

    // 连接信号和槽
    connect(m_greetButton, &QPushButton::clicked,
            this, &Widget::onGreetButtonClicked);
    
    // 按回车键也可以触发问候
    connect(m_nameEdit, &QLineEdit::returnPressed,
            this, &Widget::onGreetButtonClicked);
}

Widget::~Widget()
{
    // 控件会自动销毁（对象树机制）
}

void Widget::onGreetButtonClicked()
{
    // 获取输入的姓名
    QString name = m_nameEdit->text().trimmed();
    
    // 检查是否为空
    if (name.isEmpty()) {
        m_resultLabel->setText("请输入姓名！");
        m_resultLabel->setStyleSheet("QLabel { color: red; font-size: 16px; }");
    } else {
        // 显示问候信息
        QString greeting = QString("你好，%1！欢迎使用 Qt！").arg(name);
        m_resultLabel->setText(greeting);
        m_resultLabel->setStyleSheet("QLabel { color: green; font-size: 16px; }");
    }
}
```

### 步骤4：编译和运行

1. 点击左下角的"锤子"图标（构建）
2. 点击"绿色三角"图标（运行）
3. 在文本框中输入姓名，点击"问候"按钮

## 📚 代码详解

### 1. QWidget 基础

```cpp
Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("问候程序");  // 设置窗口标题
    resize(400, 200);            // 设置窗口大小
}
```

**说明**：
- `QWidget` 是所有 UI 控件的基类
- `setWindowTitle()` 设置窗口标题
- `resize()` 设置窗口大小

### 2. 创建控件

```cpp
QLabel *promptLabel = new QLabel("请输入你的姓名：");
m_nameEdit = new QLineEdit();
m_greetButton = new QPushButton("问候");
m_resultLabel = new QLabel("等待输入...");
```

**说明**：
- `QLabel`：标签控件，显示文本
- `QLineEdit`：文本框控件，接收用户输入
- `QPushButton`：按钮控件，响应点击事件

### 3. 布局管理

```cpp
QVBoxLayout *layout = new QVBoxLayout(this);
layout->addWidget(promptLabel);
layout->addWidget(m_nameEdit);
layout->addWidget(m_greetButton);
layout->addWidget(m_resultLabel);
layout->addStretch();
```

**说明**：
- `QVBoxLayout`：垂直布局，控件从上到下排列
- `addWidget()`：添加控件到布局
- `addStretch()`：添加弹性空间，控件靠上对齐

### 4. 信号与槽连接

```cpp
connect(m_greetButton, &QPushButton::clicked,
        this, &Widget::onGreetButtonClicked);

connect(m_nameEdit, &QLineEdit::returnPressed,
        this, &Widget::onGreetButtonClicked);
```

**说明**：
- 第一个 `connect`：按钮点击时调用 `onGreetButtonClicked()`
- 第二个 `connect`：按回车键时也调用 `onGreetButtonClicked()`
- 两个信号连接到同一个槽

### 5. 槽函数实现

```cpp
void Widget::onGreetButtonClicked()
{
    QString name = m_nameEdit->text().trimmed();
    
    if (name.isEmpty()) {
        m_resultLabel->setText("请输入姓名！");
        m_resultLabel->setStyleSheet("QLabel { color: red; }");
    } else {
        QString greeting = QString("你好，%1！").arg(name);
        m_resultLabel->setText(greeting);
        m_resultLabel->setStyleSheet("QLabel { color: green; }");
    }
}
```

**说明**：
- `text()`：获取文本框内容
- `trimmed()`：去除首尾空格
- `isEmpty()`：检查是否为空
- `setText()`：设置标签文本
- `setStyleSheet()`：设置样式（CSS 语法）
- `arg()`：字符串格式化

## 🎨 布局管理详解

### QVBoxLayout（垂直布局）

控件从上到下排列：

```
┌─────────────────┐
│  promptLabel    │
├─────────────────┤
│  nameEdit       │
├─────────────────┤
│  greetButton    │
├─────────────────┤
│  resultLabel    │
├─────────────────┤
│  (stretch)      │
└─────────────────┘
```

### QHBoxLayout（水平布局）

控件从左到右排列：

```cpp
QHBoxLayout *layout = new QHBoxLayout(this);
layout->addWidget(label1);
layout->addWidget(label2);
layout->addWidget(label3);
```

```
┌────────┬────────┬────────┐
│ label1 │ label2 │ label3 │
└────────┴────────┴────────┘
```

### 嵌套布局

```cpp
QVBoxLayout *mainLayout = new QVBoxLayout(this);

QHBoxLayout *topLayout = new QHBoxLayout();
topLayout->addWidget(label);
topLayout->addWidget(lineEdit);

mainLayout->addLayout(topLayout);
mainLayout->addWidget(button);
```

```
┌─────────────────────┐
│ label │ lineEdit    │
├─────────────────────┤
│      button         │
└─────────────────────┘
```

## 🎯 扩展练习

### 练习1：添加清空按钮

添加一个"清空"按钮，点击后清空文本框和结果标签。

**提示**：
```cpp
QPushButton *clearButton = new QPushButton("清空");
connect(clearButton, &QPushButton::clicked, [=]() {
    m_nameEdit->clear();
    m_resultLabel->setText("等待输入...");
});
```

### 练习2：添加计数功能

记录问候的次数，显示"这是第 X 次问候"。

**提示**：
```cpp
private:
    int m_greetCount = 0;

void Widget::onGreetButtonClicked()
{
    m_greetCount++;
    QString greeting = QString("你好，%1！这是第 %2 次问候。")
                       .arg(name).arg(m_greetCount);
}
```

### 练习3：添加时间问候

根据当前时间显示不同的问候语：
- 早上（6-12点）：早上好
- 下午（12-18点）：下午好
- 晚上（18-24点）：晚上好
- 深夜（0-6点）：夜深了

**提示**：
```cpp
#include <QTime>

QTime time = QTime::currentTime();
int hour = time.hour();

QString timeGreeting;
if (hour >= 6 && hour < 12) {
    timeGreeting = "早上好";
} else if (hour >= 12 && hour < 18) {
    timeGreeting = "下午好";
} else if (hour >= 18 && hour < 24) {
    timeGreeting = "晚上好";
} else {
    timeGreeting = "夜深了";
}

QString greeting = QString("%1，%2！").arg(timeGreeting).arg(name);
```

### 练习4：添加输入验证

限制姓名长度（2-20个字符），只允许中文和英文字母。

**提示**：
```cpp
#include <QRegularExpression>

QRegularExpression regex("^[\\u4e00-\\u9fa5a-zA-Z]{2,20}$");
if (!regex.match(name).hasMatch()) {
    m_resultLabel->setText("姓名格式不正确！");
    return;
}
```

## 📖 常用控件速查

### QLabel（标签）

```cpp
QLabel *label = new QLabel("文本");
label->setText("新文本");           // 设置文本
label->text();                      // 获取文本
label->setAlignment(Qt::AlignCenter); // 设置对齐方式
label->setStyleSheet("color: red;"); // 设置样式
```

### QLineEdit（文本框）

```cpp
QLineEdit *edit = new QLineEdit();
edit->setText("文本");              // 设置文本
edit->text();                       // 获取文本
edit->clear();                      // 清空
edit->setPlaceholderText("提示");   // 设置占位符
edit->setEchoMode(QLineEdit::Password); // 密码模式
```

### QPushButton（按钮）

```cpp
QPushButton *button = new QPushButton("按钮");
button->setText("新文本");          // 设置文本
button->setEnabled(false);          // 禁用按钮
button->setIcon(QIcon("icon.png")); // 设置图标
```

### 信号

```cpp
// QPushButton
clicked()                // 点击
pressed()                // 按下
released()               // 释放

// QLineEdit
textChanged(QString)     // 文本改变
returnPressed()          // 按回车键
editingFinished()        // 编辑完成
```

## 🎓 学习总结

### 核心知识点

1. **QWidget 基础**
   - 所有 UI 控件的基类
   - 设置窗口属性（标题、大小）

2. **基本控件**
   - QLabel：显示文本
   - QLineEdit：接收输入
   - QPushButton：响应点击

3. **布局管理**
   - QVBoxLayout：垂直布局
   - QHBoxLayout：水平布局
   - 可以嵌套使用

4. **信号与槽**
   - 连接控件信号和槽函数
   - 使用 Lambda 表达式
   - 多个信号连接同一个槽

5. **对象树**
   - 控件自动销毁
   - 不需要手动 delete

### 下一步学习

- 更多控件：QComboBox、QCheckBox、QRadioButton
- 更多布局：QGridLayout、QFormLayout
- 样式表：Qt Style Sheets（类似 CSS）
- 对话框：QMessageBox、QInputDialog、QFileDialog

---

**恭喜你完成了第一个 Qt 窗口应用！继续加油！** 🎉
