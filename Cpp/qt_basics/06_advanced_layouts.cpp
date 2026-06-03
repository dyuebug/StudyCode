/*
 * Qt 高级布局管理演示
 * 日期：2026-05-30（第39天）
 *
 * 本程序演示：
 * 1. QGridLayout（网格布局）
 * 2. QFormLayout（表单布局）
 * 3. 布局嵌套技巧
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

#include "06_advanced_layouts.moc"
