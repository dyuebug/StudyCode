/*
 * Qt 自定义对话框
 * 日期：2026-06-01（第41天）
 * 难度：⭐⭐ 中级
 *
 * 本程序演示：
 * 1. 继承 QDialog 创建自定义对话框
 * 2. 模态（exec）vs 非模态（show）
 * 3. 从对话框获取用户输入数据
 * 4. 对话框的标准按钮（QDialogButtonBox）
 *
 * 编译：qmake + make，或在 Qt Creator 中直接运行
 */

// ============================================
// 【核心概念】自定义 QDialog
// ============================================
/*
是什么？
- QDialog 是所有对话框的基类
- 继承 QDialog 可以创建任意复杂的自定义对话框

为什么需要？
- 标准对话框（QMessageBox 等）功能有限
- 实际项目中经常需要收集多个字段的信息
- 自定义对话框可以包含任意控件和布局

核心特点：
- exec()：模态显示，阻塞父窗口，返回 Accepted/Rejected
- show()：非模态显示，不阻塞，父窗口可继续操作
- accept() / reject()：关闭对话框并设置返回值
*/

#include <QApplication>
#include <QDialog>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QCheckBox>
#include <QTextEdit>

// ============================================
// 自定义对话框1：简单用户信息对话框
// ============================================
/*
【设计思路】
1. 继承 QDialog
2. 在构造函数中创建控件和布局
3. 提供 getter 方法让调用者获取用户输入
4. 用 QDialogButtonBox 提供标准的 OK/Cancel 按钮
*/
class UserInfoDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数：parent 参数让对话框居中显示在父窗口上
    explicit UserInfoDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("用户信息");
        setMinimumWidth(300);

        // 使用 QFormLayout 创建表单布局
        QFormLayout *form = new QFormLayout();

        // 姓名输入框
        m_nameEdit = new QLineEdit();
        m_nameEdit->setPlaceholderText("请输入姓名");
        form->addRow("姓名：", m_nameEdit);

        // 年龄输入框
        m_ageSpinBox = new QSpinBox();
        m_ageSpinBox->setRange(1, 120);
        m_ageSpinBox->setValue(25);
        form->addRow("年龄：", m_ageSpinBox);

        // 城市下拉框
        m_cityCombo = new QComboBox();
        m_cityCombo->addItems({"北京", "上海", "广州", "深圳", "其他"});
        form->addRow("城市：", m_cityCombo);

        // ----------------------------------------
        // QDialogButtonBox：标准对话框按钮
        // ----------------------------------------
        /*
        【函数卡片】QDialogButtonBox

        作用：提供标准的对话框按钮（OK、Cancel、Apply 等）
        优点：
          - 自动适配不同平台的按钮顺序（Windows: OK在左，macOS: OK在右）
          - 内置信号 accepted() 和 rejected()

        常用按钮：
          QDialogButtonBox::Ok       → 确定
          QDialogButtonBox::Cancel   → 取消
          QDialogButtonBox::Apply    → 应用
          QDialogButtonBox::Reset    → 重置
          QDialogButtonBox::Save     → 保存
          QDialogButtonBox::Discard  → 放弃
        */
        QDialogButtonBox *buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel
        );

        // 连接标准信号到 QDialog 的 accept()/reject() 槽
        // accept()：关闭对话框，exec() 返回 QDialog::Accepted（值为1）
        // reject()：关闭对话框，exec() 返回 QDialog::Rejected（值为0）
        connect(buttonBox, &QDialogButtonBox::accepted, this, &UserInfoDialog::onAccept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        // 组装布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(form);
        mainLayout->addWidget(buttonBox);
    }

    // ----------------------------------------
    // Getter 方法：让调用者获取用户输入
    // ----------------------------------------
    QString name() const { return m_nameEdit->text().trimmed(); }
    int age() const { return m_ageSpinBox->value(); }
    QString city() const { return m_cityCombo->currentText(); }

private slots:
    void onAccept()
    {
        // 在关闭前做验证
        if (m_nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "验证失败", "姓名不能为空！");
            m_nameEdit->setFocus();
            return;  // 不关闭对话框，让用户重新输入
        }
        accept();  // 验证通过，关闭对话框，返回 Accepted
    }

private:
    QLineEdit *m_nameEdit;
    QSpinBox  *m_ageSpinBox;
    QComboBox *m_cityCombo;
};

// ============================================
// 自定义对话框2：确认删除对话框（带详情）
// ============================================
class ConfirmDeleteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfirmDeleteDialog(const QString &itemName, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("确认删除");
        setMinimumWidth(350);

        QVBoxLayout *layout = new QVBoxLayout(this);

        // 警告图标 + 提示文字
        QLabel *iconLabel = new QLabel("⚠️");
        iconLabel->setStyleSheet("font-size: 32px;");
        iconLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(iconLabel);

        QLabel *msgLabel = new QLabel(
            QString("确定要删除 <b>%1</b> 吗？\n此操作不可撤销！").arg(itemName)
        );
        msgLabel->setWordWrap(true);
        msgLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(msgLabel);

        // 复选框：是否同时删除关联数据
        m_deleteRelated = new QCheckBox("同时删除关联数据");
        layout->addWidget(m_deleteRelated);

        // 自定义按钮（不用 QDialogButtonBox，手动创建）
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();

        QPushButton *cancelBtn = new QPushButton("取消");
        QPushButton *deleteBtn = new QPushButton("删除");
        deleteBtn->setStyleSheet(
            "QPushButton { background: #e74c3c; color: white; "
            "border: none; border-radius: 4px; padding: 6px 16px; }"
            "QPushButton:hover { background: #c0392b; }"
        );

        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        connect(deleteBtn, &QPushButton::clicked, this, &QDialog::accept);

        btnLayout->addWidget(cancelBtn);
        btnLayout->addWidget(deleteBtn);
        layout->addLayout(btnLayout);
    }

    // 获取用户是否勾选了"同时删除关联数据"
    bool shouldDeleteRelated() const { return m_deleteRelated->isChecked(); }

private:
    QCheckBox *m_deleteRelated;
};

// ============================================
// 主窗口：演示如何使用自定义对话框
// ============================================
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("自定义对话框演示");
        resize(500, 400);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(15);
        layout->setContentsMargins(20, 20, 20, 20);

        // 结果显示区域
        m_resultLabel = new QLabel("点击按钮查看效果");
        m_resultLabel->setWordWrap(true);
        m_resultLabel->setStyleSheet(
            "QLabel { background: #f8f9fa; border: 1px solid #dee2e6; "
            "padding: 12px; border-radius: 6px; min-height: 60px; }"
        );
        layout->addWidget(m_resultLabel);

        // ----------------------------------------
        // 演示1：模态对话框（exec）
        // ----------------------------------------
        QLabel *title1 = new QLabel("演示1：模态对话框（exec）");
        title1->setStyleSheet("font-weight: bold; color: #2c3e50;");
        layout->addWidget(title1);

        QLabel *desc1 = new QLabel(
            "exec() 会阻塞父窗口，直到用户关闭对话框才继续执行。\n"
            "适合：需要用户输入信息后才能继续的场景。"
        );
        desc1->setWordWrap(true);
        desc1->setStyleSheet("color: #666; font-size: 12px;");
        layout->addWidget(desc1);

        QPushButton *btnModal = new QPushButton("打开用户信息对话框（模态）");
        connect(btnModal, &QPushButton::clicked, this, &MainWindow::showModalDialog);
        layout->addWidget(btnModal);

        // ----------------------------------------
        // 演示2：确认删除对话框
        // ----------------------------------------
        QLabel *title2 = new QLabel("演示2：确认删除对话框");
        title2->setStyleSheet("font-weight: bold; color: #2c3e50;");
        layout->addWidget(title2);

        QPushButton *btnDelete = new QPushButton("删除文件（带确认对话框）");
        connect(btnDelete, &QPushButton::clicked, this, &MainWindow::showDeleteDialog);
        layout->addWidget(btnDelete);

        // ----------------------------------------
        // 演示3：非模态对话框（show）
        // ----------------------------------------
        QLabel *title3 = new QLabel("演示3：非模态对话框（show）");
        title3->setStyleSheet("font-weight: bold; color: #2c3e50;");
        layout->addWidget(title3);

        QLabel *desc3 = new QLabel(
            "show() 不阻塞父窗口，对话框和父窗口可以同时操作。\n"
            "适合：帮助文档、工具面板等辅助窗口。"
        );
        desc3->setWordWrap(true);
        desc3->setStyleSheet("color: #666; font-size: 12px;");
        layout->addWidget(desc3);

        QPushButton *btnNonModal = new QPushButton("打开帮助窗口（非模态）");
        connect(btnNonModal, &QPushButton::clicked, this, &MainWindow::showNonModalDialog);
        layout->addWidget(btnNonModal);

        layout->addStretch();
    }

private slots:

    void showModalDialog()
    {
        // ----------------------------------------
        // 模态对话框的标准用法
        // ----------------------------------------
        /*
        步骤：
        1. 创建对话框对象（栈上或堆上均可）
        2. 调用 exec() 显示并等待
        3. 检查返回值（Accepted / Rejected）
        4. 如果 Accepted，从对话框获取数据
        */
        UserInfoDialog dialog(this);  // 传入 this 作为父窗口，对话框居中显示

        // exec() 阻塞在这里，直到用户关闭对话框
        int result = dialog.exec();

        if (result == QDialog::Accepted) {
            // 用户点了 OK，获取输入数据
            QString info = QString("姓名：%1\n年龄：%2 岁\n城市：%3")
                .arg(dialog.name())
                .arg(dialog.age())
                .arg(dialog.city());
            m_resultLabel->setText("用户提交了信息：\n" + info);
        } else {
            // 用户点了 Cancel 或关闭了对话框
            m_resultLabel->setText("用户取消了输入");
        }
    }

    void showDeleteDialog()
    {
        ConfirmDeleteDialog dialog("重要文档.docx", this);

        if (dialog.exec() == QDialog::Accepted) {
            bool deleteRelated = dialog.shouldDeleteRelated();
            QString msg = "已删除文件：重要文档.docx";
            if (deleteRelated) {
                msg += "\n同时删除了关联数据";
            }
            m_resultLabel->setText(msg);
        } else {
            m_resultLabel->setText("取消删除");
        }
    }

    void showNonModalDialog()
    {
        // ----------------------------------------
        // 非模态对话框的标准用法
        // ----------------------------------------
        /*
        ⭐ 重要：非模态对话框必须用 new 在堆上创建！
        原因：show() 立即返回，如果在栈上创建，函数结束时对话框会被销毁。
        解决：设置 Qt::WA_DeleteOnClose 属性，关闭时自动释放内存。
        */
        QDialog *helpDialog = new QDialog(this);
        helpDialog->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动 delete
        helpDialog->setWindowTitle("帮助文档");
        helpDialog->resize(400, 300);

        QVBoxLayout *layout = new QVBoxLayout(helpDialog);
        QTextEdit *textEdit = new QTextEdit();
        textEdit->setReadOnly(true);
        textEdit->setPlainText(
            "Qt 对话框使用指南\n\n"
            "1. 模态对话框（exec）\n"
            "   - 阻塞父窗口\n"
            "   - 适合需要用户输入的场景\n\n"
            "2. 非模态对话框（show）\n"
            "   - 不阻塞父窗口\n"
            "   - 适合帮助文档、工具面板\n\n"
            "3. 自定义对话框\n"
            "   - 继承 QDialog\n"
            "   - 提供 getter 方法获取数据"
        );
        layout->addWidget(textEdit);

        helpDialog->show();  // 非阻塞，立即返回
        m_resultLabel->setText("帮助窗口已打开（非模态，可以同时操作主窗口）");
    }

private:
    QLabel *m_resultLabel;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：非模态对话框在栈上创建
    void showHelp() {
        QDialog dialog(this);  // 栈上创建
        dialog.show();         // show() 立即返回
    }  // ← 函数结束，dialog 被销毁，窗口一闪而过！

✅ 正确：堆上创建 + WA_DeleteOnClose
    QDialog *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

❌ 错误2：忘记检查 exec() 返回值
    UserInfoDialog dialog(this);
    dialog.exec();
    QString name = dialog.name();  // 用户可能点了 Cancel！

✅ 正确：
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.name();
    }

❌ 错误3：在 accept() 前忘记验证
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    // 直接关闭，没有验证用户输入！

✅ 正确：连接到自定义槽，验证后再调用 accept()
    connect(okBtn, &QPushButton::clicked, this, &MyDialog::onAccept);
    void onAccept() {
        if (m_edit->text().isEmpty()) { /* 提示错误 */ return; }
        accept();
    }

❌ 错误4：对话框没有设置父窗口
    QDialog *dialog = new QDialog();  // 没有父窗口
    dialog->exec();  // 对话框不会居中在父窗口上

✅ 正确：
    QDialog *dialog = new QDialog(this);  // 传入父窗口

❌ 错误5：在对话框析构后访问数据
    UserInfoDialog *dialog = new UserInfoDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();
    QString name = dialog->name();  // 如果设置了 WA_DeleteOnClose，exec() 后对象可能已删除！

✅ 正确：exec() 返回后对象仍然有效（WA_DeleteOnClose 在 close 事件时删除，exec() 返回时对象还在）
   但更安全的做法是在栈上创建模态对话框：
    UserInfoDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.name();  // 安全
    }
*/

// ============================================
// 练习题
// ============================================
/*
1. 创建一个"登录对话框"：
   - 包含用户名和密码输入框（密码用 QLineEdit::Password 模式）
   - 验证：用户名不能为空，密码至少6位
   - 提供"登录"和"取消"按钮
   - 登录成功后主窗口显示欢迎信息
   提示：密码模式：m_pwdEdit->setEchoMode(QLineEdit::Password)

2. 创建一个"设置对话框"（非模态）：
   - 包含字体大小、主题颜色、语言选择等设置项
   - 点击"应用"按钮时实时更新主窗口样式（不关闭对话框）
   - 点击"确定"保存设置并关闭
   - 点击"取消"恢复原设置并关闭
   提示：使用 QDialogButtonBox::Apply 信号

3. 创建一个"批量添加"对话框：
   - 用户可以输入多条记录（每次点"添加"追加一条）
   - 用 QListWidget 显示已添加的记录
   - 支持删除选中的记录
   - 点"确定"返回所有记录列表
   提示：用 QStringList 存储数据，提供 getData() 方法
*/

#include "13_custom_dialog.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
