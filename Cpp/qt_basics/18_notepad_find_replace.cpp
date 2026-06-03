/*
 * Qt 综合扩展练习A：记事本 — 查找替换功能
 * 日期：2026-06-04（第44天）
 * 难度：⭐⭐ 中级
 *
 * 在第43天记事本基础上，新增：
 * - 查找对话框（Ctrl+F）：查找下一个、查找上一个、高亮显示
 * - 替换对话框（Ctrl+H）：替换当前、全部替换
 *
 * 核心知识点：
 * - QTextEdit::find()
 * - 自定义非模态 QDialog
 * - QTextDocument::find()（全部替换）
 * - QTextCursor（文本光标操作）
 *
 * 编译：qmake + make，或在 Qt Creator 中直接运行
 */

// ============================================
// 【核心概念】QTextEdit::find() 与 QTextCursor
// ============================================
/*
QTextEdit::find(text, options)
  - 从当前光标位置向后查找
  - 找到则高亮选中，返回 true
  - 未找到返回 false
  - options：QTextDocument::FindBackward（向前）
              QTextDocument::FindCaseSensitively（区分大小写）
              QTextDocument::FindWholeWords（全词匹配）

QTextCursor：文本光标，用于程序化操作文本
  - editor->textCursor()：获取当前光标
  - cursor.selectedText()：获取选中文字
  - cursor.insertText(text)：在光标处插入文字（替换选中内容）
  - editor->setTextCursor(cursor)：设置光标位置
  - editor->moveCursor(QTextCursor::Start)：移到文档开头
*/

#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QMessageBox>
#include <QTextDocument>
#include <QTextCursor>
#include <QKeySequence>

// ============================================
// 查找替换对话框（非模态）
// ============================================
class FindReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindReplaceDialog(QTextEdit *editor, QWidget *parent = nullptr)
        : QDialog(parent), m_editor(editor)
    {
        setWindowTitle("查找和替换");
        setMinimumWidth(400);
        // 非模态：不阻塞主窗口
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        QGridLayout *grid = new QGridLayout(this);
        grid->setSpacing(8);
        grid->setContentsMargins(16, 16, 16, 16);

        // 查找行
        grid->addWidget(new QLabel("查找："), 0, 0);
        m_findEdit = new QLineEdit();
        m_findEdit->setPlaceholderText("输入要查找的文字");
        grid->addWidget(m_findEdit, 0, 1);

        // 替换行
        grid->addWidget(new QLabel("替换为："), 1, 0);
        m_replaceEdit = new QLineEdit();
        m_replaceEdit->setPlaceholderText("输入替换文字（留空则删除）");
        grid->addWidget(m_replaceEdit, 1, 1);

        // 选项
        m_caseSensitive = new QCheckBox("区分大小写");
        m_wholeWord     = new QCheckBox("全词匹配");
        QHBoxLayout *optLayout = new QHBoxLayout();
        optLayout->addWidget(m_caseSensitive);
        optLayout->addWidget(m_wholeWord);
        optLayout->addStretch();
        grid->addLayout(optLayout, 2, 0, 1, 2);

        // 按钮
        QHBoxLayout *btnLayout = new QHBoxLayout();

        m_findNextBtn = new QPushButton("查找下一个");
        m_findPrevBtn = new QPushButton("查找上一个");
        m_replaceBtn  = new QPushButton("替换");
        m_replaceAllBtn = new QPushButton("全部替换");
        QPushButton *closeBtn = new QPushButton("关闭");

        m_findNextBtn->setDefault(true);

        btnLayout->addWidget(m_findNextBtn);
        btnLayout->addWidget(m_findPrevBtn);
        btnLayout->addWidget(m_replaceBtn);
        btnLayout->addWidget(m_replaceAllBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(closeBtn);
        grid->addLayout(btnLayout, 3, 0, 1, 2);

        // 状态提示
        m_statusLabel = new QLabel("");
        m_statusLabel->setStyleSheet("color: #666; font-size: 12px;");
        grid->addWidget(m_statusLabel, 4, 0, 1, 2);

        // 连接信号
        connect(m_findNextBtn,   &QPushButton::clicked, this, &FindReplaceDialog::findNext);
        connect(m_findPrevBtn,   &QPushButton::clicked, this, &FindReplaceDialog::findPrev);
        connect(m_replaceBtn,    &QPushButton::clicked, this, &FindReplaceDialog::replaceCurrent);
        connect(m_replaceAllBtn, &QPushButton::clicked, this, &FindReplaceDialog::replaceAll);
        connect(closeBtn,        &QPushButton::clicked, this, &QDialog::close);

        // 输入框内容改变时清除状态提示
        connect(m_findEdit, &QLineEdit::textChanged, this, [this]() {
            m_statusLabel->clear();
        });
    }

    // 设置初始查找文字（从主窗口传入选中文字）
    void setFindText(const QString &text)
    {
        m_findEdit->setText(text);
        m_findEdit->selectAll();
        m_findEdit->setFocus();
    }

private slots:

    void findNext()
    {
        doFind(false);  // false = 向后查找
    }

    void findPrev()
    {
        doFind(true);   // true = 向前查找
    }

    void replaceCurrent()
    {
        // --------------------------------------------
        // 替换当前选中的匹配项
        // --------------------------------------------
        /*
        步骤：
        1. 检查当前选中文字是否就是要查找的文字
        2. 如果是，用替换文字替换选中内容
        3. 然后继续查找下一个
        */
        QString findText = m_findEdit->text();
        if (findText.isEmpty()) return;

        QTextCursor cursor = m_editor->textCursor();

        // 检查当前选中内容是否匹配
        bool matches = m_caseSensitive->isChecked()
            ? cursor.selectedText() == findText
            : cursor.selectedText().toLower() == findText.toLower();

        if (matches) {
            // 替换选中内容
            cursor.insertText(m_replaceEdit->text());
            m_editor->setTextCursor(cursor);
        }

        // 继续查找下一个
        doFind(false);
    }

    void replaceAll()
    {
        // --------------------------------------------
        // 全部替换
        // --------------------------------------------
        /*
        步骤：
        1. 从文档开头开始
        2. 循环查找，每找到一个就替换
        3. 统计替换次数
        */
        QString findText = m_findEdit->text();
        if (findText.isEmpty()) return;

        QString replaceText = m_replaceEdit->text();

        // 构建查找选项
        QTextDocument::FindFlags flags;
        if (m_caseSensitive->isChecked())
            flags |= QTextDocument::FindCaseSensitively;
        if (m_wholeWord->isChecked())
            flags |= QTextDocument::FindWholeWords;

        // 从文档开头开始查找
        QTextDocument *doc = m_editor->document();
        QTextCursor cursor(doc);  // 光标在文档开头
        int count = 0;

        // 开始一个撤销块，让所有替换操作可以一次性撤销
        cursor.beginEditBlock();

        while (true) {
            // QTextDocument::find() 从指定光标位置查找
            cursor = doc->find(findText, cursor, flags);
            if (cursor.isNull()) break;  // 没找到，结束

            cursor.insertText(replaceText);  // 替换
            count++;
        }

        cursor.endEditBlock();

        if (count > 0) {
            m_statusLabel->setText(QString("已替换 %1 处").arg(count));
            m_statusLabel->setStyleSheet("color: #27ae60; font-size: 12px;");
        } else {
            m_statusLabel->setText("未找到匹配内容");
            m_statusLabel->setStyleSheet("color: #e74c3c; font-size: 12px;");
        }
    }

private:

    void doFind(bool backward)
    {
        QString findText = m_findEdit->text();
        if (findText.isEmpty()) {
            m_statusLabel->setText("请输入查找内容");
            return;
        }

        // 构建查找选项
        QTextDocument::FindFlags flags;
        if (backward)
            flags |= QTextDocument::FindBackward;
        if (m_caseSensitive->isChecked())
            flags |= QTextDocument::FindCaseSensitively;
        if (m_wholeWord->isChecked())
            flags |= QTextDocument::FindWholeWords;

        // QTextEdit::find() 从当前光标位置查找，找到则高亮选中
        bool found = m_editor->find(findText, flags);

        if (found) {
            m_statusLabel->setText("已找到");
            m_statusLabel->setStyleSheet("color: #27ae60; font-size: 12px;");
            m_editor->setFocus();  // 焦点回到编辑器，让高亮可见
        } else {
            // 没找到：提示并回绕到文档另一端
            m_statusLabel->setText("已到达文档末尾，从头继续查找");
            m_statusLabel->setStyleSheet("color: #e67e22; font-size: 12px;");

            // 回绕：移到文档开头（或末尾），再查一次
            if (backward) {
                m_editor->moveCursor(QTextCursor::End);
            } else {
                m_editor->moveCursor(QTextCursor::Start);
            }
            m_editor->find(findText, flags);
        }
    }

    QTextEdit  *m_editor;
    QLineEdit  *m_findEdit;
    QLineEdit  *m_replaceEdit;
    QCheckBox  *m_caseSensitive;
    QCheckBox  *m_wholeWord;
    QPushButton *m_findNextBtn;
    QPushButton *m_findPrevBtn;
    QPushButton *m_replaceBtn;
    QPushButton *m_replaceAllBtn;
    QLabel     *m_statusLabel;
};

// ============================================
// 带查找替换功能的记事本
// ============================================
class NotepadWithFind : public QMainWindow
{
    Q_OBJECT

public:
    NotepadWithFind(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("记事本（含查找替换）");
        resize(800, 600);

        m_editor = new QTextEdit(this);
        m_editor->setFont(QFont("Consolas", 12));
        m_editor->setPlaceholderText("开始输入...\n\n提示：Ctrl+F 查找，Ctrl+H 替换");
        setCentralWidget(m_editor);

        // 预置一些示例文字
        m_editor->setPlainText(
            "Qt 是一个跨平台的 C++ 应用程序框架。\n"
            "Qt 提供了丰富的 GUI 控件和工具类。\n"
            "使用 Qt 可以快速开发桌面应用程序。\n"
            "Qt 的信号与槽机制是其核心特性之一。\n"
            "Qt Style Sheets 类似 CSS，用于美化界面。\n"
            "Qt 支持 Windows、macOS、Linux 等平台。"
        );

        createActions();
        createMenus();

        statusBar()->showMessage("就绪 | Ctrl+F 查找 | Ctrl+H 替换");
    }

private slots:

    void showFind()
    {
        // 获取当前选中文字作为默认查找内容
        QString selected = m_editor->textCursor().selectedText();

        if (!m_findDialog) {
            // 非模态：堆上创建，WA_DeleteOnClose 自动释放
            m_findDialog = new FindReplaceDialog(m_editor, this);
            m_findDialog->setAttribute(Qt::WA_DeleteOnClose);
            // 对话框关闭时将指针置空
            connect(m_findDialog, &QDialog::destroyed, this, [this]() {
                m_findDialog = nullptr;
            });
        }

        if (!selected.isEmpty()) {
            m_findDialog->setFindText(selected);
        }

        m_findDialog->show();
        m_findDialog->raise();      // 置于最前
        m_findDialog->activateWindow();
    }

    void showReplace()
    {
        showFind();  // 查找和替换共用同一个对话框
    }

private:

    void createActions()
    {
        m_findAction = new QAction("查找(&F)...", this);
        m_findAction->setShortcut(QKeySequence::Find);  // Ctrl+F
        connect(m_findAction, &QAction::triggered, this, &NotepadWithFind::showFind);

        m_replaceAction = new QAction("替换(&H)...", this);
        m_replaceAction->setShortcut(QKeySequence::Replace);  // Ctrl+H
        connect(m_replaceAction, &QAction::triggered, this, &NotepadWithFind::showReplace);

        m_undoAction = new QAction("撤销(&Z)", this);
        m_undoAction->setShortcut(QKeySequence::Undo);
        connect(m_undoAction, &QAction::triggered, m_editor, &QTextEdit::undo);

        m_redoAction = new QAction("重做(&Y)", this);
        m_redoAction->setShortcut(QKeySequence::Redo);
        connect(m_redoAction, &QAction::triggered, m_editor, &QTextEdit::redo);
    }

    void createMenus()
    {
        QMenu *editMenu = menuBar()->addMenu("编辑(&E)");
        editMenu->addAction(m_undoAction);
        editMenu->addAction(m_redoAction);
        editMenu->addSeparator();
        editMenu->addAction(m_findAction);
        editMenu->addAction(m_replaceAction);
    }

    QTextEdit         *m_editor;
    FindReplaceDialog *m_findDialog = nullptr;
    QAction *m_findAction;
    QAction *m_replaceAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：查找对话框用模态（exec）
    FindDialog dlg(editor, this);
    dlg.exec();  // 阻塞！用户无法同时操作编辑器

✅ 正确：非模态（show），堆上创建 + WA_DeleteOnClose

❌ 错误2：全部替换没有用 beginEditBlock/endEditBlock
    while (found) { cursor.insertText(replace); }
    // 每次替换都是独立的撤销步骤，需要按 N 次 Ctrl+Z

✅ 正确：
    cursor.beginEditBlock();
    while (...) { cursor.insertText(replace); }
    cursor.endEditBlock();
    // 所有替换合并为一个撤销步骤

❌ 错误3：find() 到达末尾后不回绕
    bool found = editor->find(text);
    if (!found) { showMessage("未找到"); }
    // 用户体验差，应该回绕到文档开头继续查找

✅ 正确：未找到时移到文档开头，再查一次

❌ 错误4：非模态对话框关闭后指针悬空
    m_findDialog->show();
    // 用户关闭对话框后，m_findDialog 指向已销毁的对象
    // 再次调用 showFind() 时访问悬空指针，崩溃！

✅ 正确：监听 destroyed 信号，关闭时将指针置 nullptr
    connect(m_findDialog, &QDialog::destroyed, this, [this]() {
        m_findDialog = nullptr;
    });
*/

// ============================================
// 练习题
// ============================================
/*
1. 添加"区分大小写"和"全词匹配"选项的实际效果验证：
   - 输入 "qt"，不勾选区分大小写，应该能找到 "Qt"
   - 输入 "Qt"，勾选全词匹配，不应该匹配 "QtCore"
   提示：QTextDocument::FindCaseSensitively / FindWholeWords

2. 添加查找结果计数：
   - 在状态栏显示"第 X 个，共 Y 个匹配"
   - 提示：先用 QTextDocument::find() 统计总数，再用 QTextEdit::find() 定位

3. 添加正则表达式查找：
   - 勾选"正则表达式"选项后，用 QRegularExpression 查找
   - 提示：QTextDocument::find(QRegularExpression(...), cursor, flags)
*/

#include "18_notepad_find_replace.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    NotepadWithFind window;
    window.show();
    return app.exec();
}
