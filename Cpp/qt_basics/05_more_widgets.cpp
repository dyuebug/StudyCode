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

// __CONTINUE_HERE__
