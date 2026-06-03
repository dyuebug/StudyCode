/*
 * Qt 实战项目：记事本应用
 * 日期：2026-06-03（第43天）
 * 难度：⭐⭐ 中级
 *
 * 综合运用：
 * - QMainWindow（菜单栏、工具栏、状态栏）
 * - QAction（菜单项与工具栏按钮的统一抽象）
 * - QTextEdit（富文本编辑器）
 * - QFile + QTextStream（文件读写）
 * - QFileDialog（打开/保存对话框）
 * - closeEvent（关闭前确认）
 *
 * 编译：qmake + make，或在 Qt Creator 中直接运行
 */

// ============================================
// 【核心概念】QMainWindow 结构
// ============================================
/*
QMainWindow 是带完整 UI 框架的主窗口，结构如下：

  ┌─────────────────────────────────┐
  │  菜单栏（QMenuBar）              │
  ├─────────────────────────────────┤
  │  工具栏（QToolBar）              │
  ├─────────────────────────────────┤
  │                                 │
  │  中央控件（setCentralWidget）    │
  │                                 │
  ├─────────────────────────────────┤
  │  状态栏（QStatusBar）            │
  └─────────────────────────────────┘

与 QWidget 的区别：
- QWidget：空白窗口，自己管理所有布局
- QMainWindow：内置菜单栏/工具栏/状态栏框架，适合主窗口

【核心概念】QAction
- QAction 是"动作"的抽象，可以同时出现在菜单和工具栏
- 一个 QAction 对象 → 一个菜单项 + 一个工具栏按钮
- 修改 QAction 的状态（enabled/checked）会同步到所有位置
- 支持快捷键（setShortcut）、图标（setIcon）、提示（setToolTip）
*/

#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFontDialog>
#include <QColorDialog>
#include <QCloseEvent>
#include <QLabel>
#include <QDir>
#include <QFileInfo>

class Notepad : public QMainWindow
{
    Q_OBJECT

public:
    Notepad(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("记事本 - 新文件");
        resize(800, 600);

        // ----------------------------------------
        // 中央控件：QTextEdit
        // ----------------------------------------
        /*
        【函数卡片】setCentralWidget()

        作用：设置 QMainWindow 的中央内容区域
        语法：void setCentralWidget(QWidget *widget)
        说明：中央控件会自动填满菜单栏和状态栏之间的空间
        */
        m_editor = new QTextEdit(this);
        m_editor->setFont(QFont("Consolas", 12));
        m_editor->setPlaceholderText("开始输入...");
        setCentralWidget(m_editor);

        // ----------------------------------------
        // 创建所有 QAction
        // ----------------------------------------
        createActions();

        // ----------------------------------------
        // 创建菜单栏
        // ----------------------------------------
        createMenus();

        // ----------------------------------------
        // 创建工具栏
        // ----------------------------------------
        createToolBar();

        // ----------------------------------------
        // 创建状态栏
        // ----------------------------------------
        createStatusBar();

        // ----------------------------------------
        // 连接信号
        // ----------------------------------------

        // 文本改变时更新状态栏
        connect(m_editor, &QTextEdit::textChanged,
                this, &Notepad::updateStatusBar);

        // 文本改变时更新撤销/重做按钮状态
        connect(m_editor->document(), &QTextDocument::undoAvailable,
                m_undoAction, &QAction::setEnabled);
        connect(m_editor->document(), &QTextDocument::redoAvailable,
                m_redoAction, &QAction::setEnabled);

        // 初始状态：撤销/重做不可用
        m_undoAction->setEnabled(false);
        m_redoAction->setEnabled(false);

        // 应用样式
        applyStyle();
    }

protected:
    // ----------------------------------------
    // 关闭事件：关闭前检查未保存内容
    // ----------------------------------------
    /*
    【函数卡片】closeEvent()

    作用：重写关闭事件，在窗口关闭前执行自定义逻辑
    语法：void closeEvent(QCloseEvent *event) override
    参数：
      event->accept()  — 允许关闭
      event->ignore()  — 阻止关闭（用户点了"取消"）

    触发时机：用户点击窗口的 X 按钮，或调用 close()
    */
    void closeEvent(QCloseEvent *event) override
    {
        if (maybeSave()) {
            event->accept();   // 允许关闭
        } else {
            event->ignore();   // 用户点了取消，阻止关闭
        }
    }

private slots:

    // ============================================
    // 文件菜单操作
    // ============================================

    void newFile()
    {
        // 检查是否有未保存内容，如有则询问
        if (!maybeSave()) return;

        m_editor->clear();
        m_currentFile = "";
        setWindowTitle("记事本 - 新文件");
    }

    void openFile()
    {
        if (!maybeSave()) return;

        // QFileDialog 选择文件
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "打开文件",
            QDir::homePath(),
            "文本文件 (*.txt);;所有文件 (*.*)"
        );

        if (fileName.isEmpty()) return;

        loadFile(fileName);
    }

    bool saveFile()
    {
        // 已有文件名则直接保存，否则弹出另存为对话框
        if (m_currentFile.isEmpty()) {
            return saveFileAs();
        }
        return writeFile(m_currentFile);
    }

    bool saveFileAs()
    {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "另存为",
            m_currentFile.isEmpty()
                ? QDir::homePath() + "/untitled.txt"
                : m_currentFile,
            "文本文件 (*.txt);;所有文件 (*.*)"
        );

        if (fileName.isEmpty()) return false;
        return writeFile(fileName);
    }

    // ============================================
    // 格式菜单操作
    // ============================================

    void selectFont()
    {
        bool ok;
        QFont font = QFontDialog::getFont(&ok, m_editor->font(), this, "选择字体");
        if (ok) {
            m_editor->setFont(font);
        }
    }

    void selectTextColor()
    {
        QColor color = QColorDialog::getColor(
            m_editor->textColor(), this, "选择文字颜色"
        );
        if (color.isValid()) {
            m_editor->setTextColor(color);
        }
    }

    // ============================================
    // 帮助菜单
    // ============================================

    void showAbout()
    {
        QMessageBox::information(
            this,
            "关于记事本",
            "Qt 记事本 v1.0\n\n"
            "基于 Qt 框架开发的简单文本编辑器\n"
            "支持文件新建、打开、保存\n"
            "支持撤销、重做、剪切、复制、粘贴\n\n"
            "第43天学习项目"
        );
    }

    // ============================================
    // 状态栏更新
    // ============================================

    void updateStatusBar()
    {
        QString text = m_editor->toPlainText();
        int chars = text.length();
        int lines = m_editor->document()->lineCount();

        // 统计单词数（按空白字符分割）
        int words = text.split(QRegularExpression("\\s+"),
                               Qt::SkipEmptyParts).count();

        m_statusLabel->setText(
            QString("字符：%1  单词：%2  行数：%3")
                .arg(chars).arg(words).arg(lines)
        );
    }

private:

    // ============================================
    // 创建所有 QAction
    // ============================================
    /*
    【函数卡片】QAction

    作用：表示一个可触发的"动作"，可同时出现在菜单和工具栏
    构造：QAction(const QString &text, QObject *parent)
          QAction(const QIcon &icon, const QString &text, QObject *parent)

    常用方法：
      setShortcut(QKeySequence)  — 设置快捷键
      setToolTip(QString)        — 设置工具提示
      setEnabled(bool)           — 启用/禁用
      setCheckable(bool)         — 设为可勾选状态
      triggered()                — 点击时发出的信号
    */
    void createActions()
    {
        // ---- 文件菜单 ----
        m_newAction = new QAction("新建(&N)", this);
        m_newAction->setShortcut(QKeySequence::New);       // Ctrl+N
        m_newAction->setToolTip("新建文件");
        connect(m_newAction, &QAction::triggered, this, &Notepad::newFile);

        m_openAction = new QAction("打开(&O)...", this);
        m_openAction->setShortcut(QKeySequence::Open);     // Ctrl+O
        m_openAction->setToolTip("打开文件");
        connect(m_openAction, &QAction::triggered, this, &Notepad::openFile);

        m_saveAction = new QAction("保存(&S)", this);
        m_saveAction->setShortcut(QKeySequence::Save);     // Ctrl+S
        m_saveAction->setToolTip("保存文件");
        connect(m_saveAction, &QAction::triggered, this, &Notepad::saveFile);

        m_saveAsAction = new QAction("另存为(&A)...", this);
        m_saveAsAction->setShortcut(QKeySequence::SaveAs); // Ctrl+Shift+S
        connect(m_saveAsAction, &QAction::triggered, this, &Notepad::saveFileAs);

        m_exitAction = new QAction("退出(&Q)", this);
        m_exitAction->setShortcut(QKeySequence::Quit);     // Ctrl+Q
        connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

        // ---- 编辑菜单 ----
        // 撤销/重做直接连接到 QTextEdit 的内置槽
        m_undoAction = new QAction("撤销(&Z)", this);
        m_undoAction->setShortcut(QKeySequence::Undo);     // Ctrl+Z
        connect(m_undoAction, &QAction::triggered, m_editor, &QTextEdit::undo);

        m_redoAction = new QAction("重做(&Y)", this);
        m_redoAction->setShortcut(QKeySequence::Redo);     // Ctrl+Y
        connect(m_redoAction, &QAction::triggered, m_editor, &QTextEdit::redo);

        m_cutAction = new QAction("剪切(&X)", this);
        m_cutAction->setShortcut(QKeySequence::Cut);       // Ctrl+X
        connect(m_cutAction, &QAction::triggered, m_editor, &QTextEdit::cut);

        m_copyAction = new QAction("复制(&C)", this);
        m_copyAction->setShortcut(QKeySequence::Copy);     // Ctrl+C
        connect(m_copyAction, &QAction::triggered, m_editor, &QTextEdit::copy);

        m_pasteAction = new QAction("粘贴(&V)", this);
        m_pasteAction->setShortcut(QKeySequence::Paste);   // Ctrl+V
        connect(m_pasteAction, &QAction::triggered, m_editor, &QTextEdit::paste);

        m_selectAllAction = new QAction("全选(&A)", this);
        m_selectAllAction->setShortcut(QKeySequence::SelectAll); // Ctrl+A
        connect(m_selectAllAction, &QAction::triggered,
                m_editor, &QTextEdit::selectAll);

        // ---- 格式菜单 ----
        m_fontAction = new QAction("字体(&F)...", this);
        connect(m_fontAction, &QAction::triggered, this, &Notepad::selectFont);

        m_colorAction = new QAction("文字颜色(&C)...", this);
        connect(m_colorAction, &QAction::triggered, this, &Notepad::selectTextColor);

        // ---- 帮助菜单 ----
        m_aboutAction = new QAction("关于(&A)", this);
        connect(m_aboutAction, &QAction::triggered, this, &Notepad::showAbout);
    }

    // ============================================
    // 创建菜单栏
    // ============================================
    /*
    【函数卡片】QMenuBar / QMenu

    menuBar()         — 获取 QMainWindow 的菜单栏（自动创建）
    menuBar()->addMenu(title)  — 添加顶级菜单
    menu->addAction(action)    — 添加菜单项
    menu->addSeparator()       — 添加分隔线
    menu->addMenu(submenu)     — 添加子菜单
    */
    void createMenus()
    {
        // 文件菜单
        QMenu *fileMenu = menuBar()->addMenu("文件(&F)");
        fileMenu->addAction(m_newAction);
        fileMenu->addAction(m_openAction);
        fileMenu->addSeparator();
        fileMenu->addAction(m_saveAction);
        fileMenu->addAction(m_saveAsAction);
        fileMenu->addSeparator();
        fileMenu->addAction(m_exitAction);

        // 编辑菜单
        QMenu *editMenu = menuBar()->addMenu("编辑(&E)");
        editMenu->addAction(m_undoAction);
        editMenu->addAction(m_redoAction);
        editMenu->addSeparator();
        editMenu->addAction(m_cutAction);
        editMenu->addAction(m_copyAction);
        editMenu->addAction(m_pasteAction);
        editMenu->addSeparator();
        editMenu->addAction(m_selectAllAction);

        // 格式菜单
        QMenu *formatMenu = menuBar()->addMenu("格式(&O)");
        formatMenu->addAction(m_fontAction);
        formatMenu->addAction(m_colorAction);

        // 帮助菜单
        QMenu *helpMenu = menuBar()->addMenu("帮助(&H)");
        helpMenu->addAction(m_aboutAction);
    }

    // ============================================
    // 创建工具栏
    // ============================================
    /*
    【函数卡片】QToolBar

    addToolBar(title)          — 添加工具栏
    toolbar->addAction(action) — 添加工具栏按钮（复用 QAction）
    toolbar->addSeparator()    — 添加分隔线
    toolbar->setMovable(false) — 禁止拖动工具栏
    */
    void createToolBar()
    {
        QToolBar *toolbar = addToolBar("主工具栏");
        toolbar->setMovable(false);

        // 文件操作按钮（复用同一个 QAction，与菜单共享）
        toolbar->addAction(m_newAction);
        toolbar->addAction(m_openAction);
        toolbar->addAction(m_saveAction);
        toolbar->addSeparator();

        // 编辑操作按钮
        toolbar->addAction(m_undoAction);
        toolbar->addAction(m_redoAction);
        toolbar->addSeparator();
        toolbar->addAction(m_cutAction);
        toolbar->addAction(m_copyAction);
        toolbar->addAction(m_pasteAction);
    }

    // ============================================
    // 创建状态栏
    // ============================================
    void createStatusBar()
    {
        m_statusLabel = new QLabel("字符：0  单词：0  行数：1");
        statusBar()->addPermanentWidget(m_statusLabel);
        statusBar()->showMessage("就绪");
    }

    // ============================================
    // 文件读写辅助函数
    // ============================================

    void loadFile(const QString &fileName)
    {
        /*
        【函数卡片】QFile + QTextStream

        QFile：文件操作类
          open(QIODevice::ReadOnly)  — 只读打开
          open(QIODevice::WriteOnly) — 只写打开（覆盖）
          open(QIODevice::Text)      — 文本模式（自动处理换行符）

        QTextStream：文本流，方便读写字符串
          stream.readAll()           — 读取全部内容
          stream << text             — 写入文本
          stream.setEncoding(QStringConverter::Utf8) — 设置编码
        */
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "错误",
                "无法打开文件：" + file.errorString());
            return;
        }

        QTextStream in(&file);
        in.setEncoding(QStringConverter::Utf8);
        m_editor->setPlainText(in.readAll());
        file.close();

        m_currentFile = fileName;
        setWindowTitle("记事本 - " + QFileInfo(fileName).fileName());
        statusBar()->showMessage("已打开：" + fileName, 3000);
    }

    bool writeFile(const QString &fileName)
    {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "错误",
                "无法保存文件：" + file.errorString());
            return false;
        }

        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        out << m_editor->toPlainText();
        file.close();

        m_currentFile = fileName;
        setWindowTitle("记事本 - " + QFileInfo(fileName).fileName());

        // 清除"已修改"标记
        m_editor->document()->setModified(false);

        statusBar()->showMessage("已保存：" + fileName, 3000);
        return true;
    }

    // 检查是否有未保存内容，返回 true 表示可以继续操作
    bool maybeSave()
    {
        /*
        document()->isModified()：
        - true：文本自上次保存后有修改
        - false：未修改或刚保存

        三个选项：
        - Save（保存）：保存后继续
        - Discard（放弃）：不保存，直接继续
        - Cancel（取消）：中止当前操作
        */
        if (!m_editor->document()->isModified()) {
            return true;  // 没有未保存内容，直接继续
        }

        auto reply = QMessageBox::question(
            this,
            "未保存的更改",
            "文件已修改，是否保存？",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save
        );

        if (reply == QMessageBox::Save) {
            return saveFile();   // 保存成功返回 true，失败返回 false
        } else if (reply == QMessageBox::Cancel) {
            return false;        // 用户取消，阻止后续操作
        }
        return true;             // Discard：放弃修改，继续
    }

    void applyStyle()
    {
        setStyleSheet(R"(
            QMainWindow {
                background: #f5f5f5;
            }
            QMenuBar {
                background: #ffffff;
                border-bottom: 1px solid #e0e0e0;
                padding: 2px;
            }
            QMenuBar::item:selected {
                background: #e8f0fe;
                border-radius: 4px;
            }
            QMenu {
                background: #ffffff;
                border: 1px solid #e0e0e0;
                border-radius: 4px;
            }
            QMenu::item:selected {
                background: #e8f0fe;
            }
            QToolBar {
                background: #ffffff;
                border-bottom: 1px solid #e0e0e0;
                spacing: 4px;
                padding: 4px;
            }
            QToolBar QToolButton {
                border: none;
                border-radius: 4px;
                padding: 4px 8px;
                color: #333;
            }
            QToolBar QToolButton:hover {
                background: #e8f0fe;
            }
            QTextEdit {
                background: #ffffff;
                border: none;
                font-family: Consolas, monospace;
                font-size: 13px;
                color: #212121;
                padding: 8px;
            }
            QStatusBar {
                background: #f5f5f5;
                border-top: 1px solid #e0e0e0;
                color: #666;
                font-size: 12px;
            }
        )");
    }

    // ============================================
    // 成员变量
    // ============================================
    QTextEdit *m_editor;
    QString    m_currentFile;
    QLabel    *m_statusLabel;

    // 文件菜单 Actions
    QAction *m_newAction;
    QAction *m_openAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_exitAction;

    // 编辑菜单 Actions
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_selectAllAction;

    // 格式菜单 Actions
    QAction *m_fontAction;
    QAction *m_colorAction;

    // 帮助菜单 Actions
    QAction *m_aboutAction;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：忘记重写 closeEvent，直接关闭丢失数据
    // 没有 closeEvent，用户点 X 直接关闭，未保存内容丢失

✅ 正确：重写 closeEvent，调用 maybeSave()
    void closeEvent(QCloseEvent *event) override {
        if (maybeSave()) event->accept();
        else event->ignore();
    }

❌ 错误2：保存后忘记清除 isModified 标记
    out << m_editor->toPlainText();
    // 没有 setModified(false)，标题栏仍显示"已修改"

✅ 正确：
    m_editor->document()->setModified(false);

❌ 错误3：QAction 快捷键与系统冲突
    m_exitAction->setShortcut(Qt::CTRL | Qt::Key_Q);  // 手动设置
    // 不同平台快捷键不同（macOS 用 Cmd，Windows 用 Ctrl）

✅ 正确：使用 QKeySequence 标准序列，自动适配平台
    m_exitAction->setShortcut(QKeySequence::Quit);

❌ 错误4：文件编码问题，中文乱码
    QTextStream in(&file);
    // 没有设置编码，默认系统编码，中文可能乱码

✅ 正确：明确设置 UTF-8 编码
    in.setEncoding(QStringConverter::Utf8);

❌ 错误5：工具栏按钮没有图标，只显示文字
    toolbar->addAction(m_newAction);
    // QAction 没有设置图标，工具栏显示文字（较丑）

✅ 改进：设置图标（使用 Qt 内置图标或自定义图标）
    m_newAction->setIcon(QIcon::fromTheme("document-new",
        style()->standardIcon(QStyle::SP_FileIcon)));
*/

// ============================================
// 练习题
// ============================================
/*
1. 添加"查找和替换"功能：
   - 菜单：编辑 → 查找(Ctrl+F)、替换(Ctrl+H)
   - 创建自定义 QDialog，包含查找框、替换框、查找下一个、全部替换按钮
   - 用 QTextEdit::find() 实现查找
   提示：QTextEdit::find(text) 返回 bool，找到则高亮选中

2. 添加行号显示：
   - 在 QTextEdit 左侧显示行号
   - 行号随滚动同步
   提示：需要自定义 QWidget 作为行号区域，重写 paintEvent

3. 添加最近文件列表：
   - 文件菜单底部显示最近打开的5个文件
   - 用 QSettings 持久化保存文件路径列表
   - 点击最近文件直接打开
   提示：QSettings("MyApp", "Notepad") 读写注册表/配置文件
*/

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
【错误1】关闭时未提示保存，用户丢失内容

❌ 错误代码：
// 没有重写 closeEvent，直接关闭
// 用户写了大量文字，意外按了关闭，全部丢失

✅ 正确代码：
void closeEvent(QCloseEvent *event) override
{
    if (m_textEdit->document()->isModified()) {
        auto btn = QMessageBox::question(this, "保存", "内容已修改，是否保存？",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (btn == QMessageBox::Save)      { saveFile(); event->accept(); }
        else if (btn == QMessageBox::Discard) { event->accept(); }
        else                              { event->ignore(); }  // 取消，不关闭
    } else {
        event->accept();
    }
}

预防措施：有编辑功能的窗口必须重写 closeEvent 处理未保存内容。

────────────────────────────────────────────────────────────

【错误2】读取文件时未指定编码导致中文乱码

❌ 错误代码：
QFile file(path);
file.open(QIODevice::ReadOnly);
QTextStream in(&file);
m_textEdit->setPlainText(in.readAll());  // 默认编码，中文文件可能乱码

✅ 正确代码：
QFile file(path);
file.open(QIODevice::ReadOnly);
QTextStream in(&file);
in.setEncoding(QStringConverter::Utf8);  // Qt6
// Qt5: in.setCodec("UTF-8");
m_textEdit->setPlainText(in.readAll());

预防措施：文件读写时明确指定 UTF-8 编码。

────────────────────────────────────────────────────────────

【错误3】窗口标题未反映文件修改状态

❌ 错误代码：
// 文件被修改后，标题仍然显示原文件名，用户不知道有未保存的更改

✅ 正确代码：
connect(m_textEdit->document(), &QTextDocument::modificationChanged,
        this, [=](bool modified) {
    QString title = m_currentFile.isEmpty() ? "无标题" : QFileInfo(m_currentFile).fileName();
    setWindowTitle((modified ? "* " : "") + title + " - 记事本");
});

预防措施：连接 document()->modificationChanged 信号，在标题加 * 标记未保存状态。

────────────────────────────────────────────────────────────

【错误4】"另存为"后未更新当前文件路径

❌ 错误代码：
void saveAs() {
    QString path = QFileDialog::getSaveFileName(...);
    if (!path.isEmpty()) {
        writeFile(path);
        // 忘记更新 m_currentFile！
        // 之后按 Ctrl+S，还是保存到旧路径
    }
}

✅ 正确代码：
void saveAs() {
    QString path = QFileDialog::getSaveFileName(...);
    if (!path.isEmpty()) {
        m_currentFile = path;  // 更新当前文件路径
        writeFile(path);
        setWindowTitle(QFileInfo(path).fileName() + " - 记事本");
    }
}

预防措施："另存为"完成后，更新所有与当前文件路径相关的状态。

────────────────────────────────────────────────────────────

【错误5】QFile 打开失败未检查，导致读写空内容

❌ 错误代码：
QFile file(path);
file.open(QIODevice::ReadOnly);  // 文件不存在，open 失败，返回 false
QTextStream in(&file);
m_textEdit->setPlainText(in.readAll());  // 读到空字符串，清空了编辑器内容！

✅ 正确代码：
QFile file(path);
if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(this, "错误", "无法打开文件：" + file.errorString());
    return;
}
QTextStream in(&file);
m_textEdit->setPlainText(in.readAll());

预防措施：QFile::open() 返回 bool，必须检查，失败时向用户报告原因。
*/

#include "16_notepad.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Notepad window;
    window.show();
    return app.exec();
}
