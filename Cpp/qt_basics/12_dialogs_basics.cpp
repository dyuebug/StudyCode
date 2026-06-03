/*
 * Qt 标准对话框详解
 * 日期：2026-06-01（第41天）
 * 难度：⭐⭐ 中级
 *
 * 本程序演示：
 * 1. QMessageBox（消息对话框）- 四种类型
 * 2. QInputDialog（输入对话框）- 文本/数字/列表
 * 3. QFileDialog（文件对话框）- 打开/保存/目录
 * 4. QColorDialog（颜色对话框）
 * 5. QFontDialog（字体对话框）
 *
 * 编译：qmake + make，或在 Qt Creator 中直接运行
 */

// ============================================
// 【核心概念】Qt 对话框
// ============================================
/*
是什么？
- 对话框是弹出的临时窗口，用于与用户进行交互
- Qt 提供了一套标准对话框，覆盖最常见的交互场景

为什么需要？
- 提示信息、警告、错误 → QMessageBox
- 获取用户输入（文本、数字）→ QInputDialog
- 选择文件路径 → QFileDialog
- 选择颜色/字体 → QColorDialog / QFontDialog

核心特点：
- 静态方法调用，一行代码即可弹出
- 模态（阻塞）：用户必须关闭对话框才能继续操作
- 返回值告知用户的选择结果
*/

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QColorDialog>
#include <QFontDialog>
#include <QDir>

class DialogsDemo : public QWidget
{
    Q_OBJECT

public:
    DialogsDemo(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("Qt 标准对话框演示");
        resize(500, 600);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        // 结果显示标签
        m_resultLabel = new QLabel("点击按钮查看对话框效果");
        m_resultLabel->setWordWrap(true);
        m_resultLabel->setStyleSheet(
            "QLabel { background: #f0f0f0; border: 1px solid #ccc; "
            "padding: 10px; border-radius: 5px; min-height: 40px; }"
        );
        mainLayout->addWidget(m_resultLabel);

        // ----------------------------------------
        // 第一部分：QMessageBox
        // ----------------------------------------
        mainLayout->addWidget(makeTitle("第一部分：QMessageBox（消息对话框）"));

        QGridLayout *msgGrid = new QGridLayout();

        // information：普通信息提示
        QPushButton *btnInfo = new QPushButton("information（信息）");
        connect(btnInfo, &QPushButton::clicked, this, &DialogsDemo::showInformation);
        msgGrid->addWidget(btnInfo, 0, 0);

        // warning：警告提示
        QPushButton *btnWarn = new QPushButton("warning（警告）");
        connect(btnWarn, &QPushButton::clicked, this, &DialogsDemo::showWarning);
        msgGrid->addWidget(btnWarn, 0, 1);

        // critical：严重错误
        QPushButton *btnCrit = new QPushButton("critical（错误）");
        connect(btnCrit, &QPushButton::clicked, this, &DialogsDemo::showCritical);
        msgGrid->addWidget(btnCrit, 1, 0);

        // question：询问确认（有 Yes/No）
        QPushButton *btnQues = new QPushButton("question（询问）");
        connect(btnQues, &QPushButton::clicked, this, &DialogsDemo::showQuestion);
        msgGrid->addWidget(btnQues, 1, 1);

        mainLayout->addLayout(msgGrid);

        // ----------------------------------------
        // 第二部分：QInputDialog
        // ----------------------------------------
        mainLayout->addWidget(makeTitle("第二部分：QInputDialog（输入对话框）"));

        QGridLayout *inputGrid = new QGridLayout();

        QPushButton *btnText = new QPushButton("getText（输入文本）");
        connect(btnText, &QPushButton::clicked, this, &DialogsDemo::showGetText);
        inputGrid->addWidget(btnText, 0, 0);

        QPushButton *btnInt = new QPushButton("getInt（输入整数）");
        connect(btnInt, &QPushButton::clicked, this, &DialogsDemo::showGetInt);
        inputGrid->addWidget(btnInt, 0, 1);

        QPushButton *btnItem = new QPushButton("getItem（选择列表项）");
        connect(btnItem, &QPushButton::clicked, this, &DialogsDemo::showGetItem);
        inputGrid->addWidget(btnItem, 1, 0);

        QPushButton *btnDouble = new QPushButton("getDouble（输入小数）");
        connect(btnDouble, &QPushButton::clicked, this, &DialogsDemo::showGetDouble);
        inputGrid->addWidget(btnDouble, 1, 1);

        mainLayout->addLayout(inputGrid);

        // ----------------------------------------
        // 第三部分：QFileDialog
        // ----------------------------------------
        mainLayout->addWidget(makeTitle("第三部分：QFileDialog（文件对话框）"));

        QGridLayout *fileGrid = new QGridLayout();

        QPushButton *btnOpen = new QPushButton("getOpenFileName（打开文件）");
        connect(btnOpen, &QPushButton::clicked, this, &DialogsDemo::showOpenFile);
        fileGrid->addWidget(btnOpen, 0, 0);

        QPushButton *btnSave = new QPushButton("getSaveFileName（保存文件）");
        connect(btnSave, &QPushButton::clicked, this, &DialogsDemo::showSaveFile);
        fileGrid->addWidget(btnSave, 0, 1);

        QPushButton *btnDir = new QPushButton("getExistingDirectory（选目录）");
        connect(btnDir, &QPushButton::clicked, this, &DialogsDemo::showOpenDir);
        fileGrid->addWidget(btnDir, 1, 0);

        QPushButton *btnMulti = new QPushButton("getOpenFileNames（多文件）");
        connect(btnMulti, &QPushButton::clicked, this, &DialogsDemo::showOpenFiles);
        fileGrid->addWidget(btnMulti, 1, 1);

        mainLayout->addLayout(fileGrid);

        // ----------------------------------------
        // 第四部分：QColorDialog / QFontDialog
        // ----------------------------------------
        mainLayout->addWidget(makeTitle("第四部分：颜色 & 字体对话框"));

        QHBoxLayout *cfLayout = new QHBoxLayout();

        QPushButton *btnColor = new QPushButton("QColorDialog（选颜色）");
        connect(btnColor, &QPushButton::clicked, this, &DialogsDemo::showColorDialog);
        cfLayout->addWidget(btnColor);

        QPushButton *btnFont = new QPushButton("QFontDialog（选字体）");
        connect(btnFont, &QPushButton::clicked, this, &DialogsDemo::showFontDialog);
        cfLayout->addWidget(btnFont);

        mainLayout->addLayout(cfLayout);
        mainLayout->addStretch();
    }

private slots:

    // ============================================
    // 第一部分：QMessageBox
    // ============================================

    void showInformation()
    {
        // --------------------------------------------
        // QMessageBox::information — 信息提示
        // --------------------------------------------
        /*
        【函数卡片】QMessageBox::information()

        语法：
          QMessageBox::StandardButton information(
              QWidget *parent,    // 父窗口（居中显示用）
              const QString &title,   // 对话框标题
              const QString &text,    // 正文内容
              QMessageBox::StandardButtons buttons = Ok,  // 按钮（默认只有 OK）
              QMessageBox::StandardButton defaultButton = NoButton  // 默认焦点按钮
          )

        返回值：用户点击的按钮（QMessageBox::Ok / Yes / No 等）
        特点：蓝色 ℹ️ 图标，用于普通提示
        */
        QMessageBox::information(
            this,
            "操作成功",          // 标题
            "文件已保存！\n路径：C:/Users/user/document.txt"  // 内容
        );
        m_resultLabel->setText("用户关闭了 information 对话框");
    }

    void showWarning()
    {
        // --------------------------------------------
        // QMessageBox::warning — 警告提示
        // --------------------------------------------
        /*
        特点：黄色 ⚠️ 图标，用于提醒用户注意
        用途：操作有风险但不致命时使用
        */
        QMessageBox::warning(
            this,
            "警告",
            "磁盘空间不足！\n剩余空间：256 MB\n建议清理磁盘。"
        );
        m_resultLabel->setText("用户关闭了 warning 对话框");
    }

    void showCritical()
    {
        // --------------------------------------------
        // QMessageBox::critical — 严重错误
        // --------------------------------------------
        /*
        特点：红色 ❌ 图标，用于严重错误
        用途：操作失败、程序异常等严重情况
        */
        QMessageBox::critical(
            this,
            "错误",
            "无法连接到数据库！\n错误代码：1045\n请检查数据库配置。"
        );
        m_resultLabel->setText("用户关闭了 critical 对话框");
    }

    void showQuestion()
    {
        // --------------------------------------------
        // QMessageBox::question — 询问确认
        // --------------------------------------------
        /*
        特点：❓ 图标，提供 Yes/No 选择
        返回值：QMessageBox::Yes 或 QMessageBox::No
        用途：删除确认、退出确认等需要用户决策的场景

        ⭐ 重要：必须检查返回值才能知道用户的选择！
        */
        auto reply = QMessageBox::question(
            this,
            "确认删除",
            "确定要删除这个文件吗？\n此操作不可撤销！",
            QMessageBox::Yes | QMessageBox::No,  // 提供两个按钮
            QMessageBox::No                       // 默认焦点在 No（更安全）
        );

        // 根据用户选择执行不同操作
        if (reply == QMessageBox::Yes) {
            m_resultLabel->setText("用户点击了 Yes → 执行删除操作");
        } else {
            m_resultLabel->setText("用户点击了 No → 取消删除");
        }
    }

    // ============================================
    // 第二部分：QInputDialog
    // ============================================

    void showGetText()
    {
        // --------------------------------------------
        // QInputDialog::getText — 获取文本输入
        // --------------------------------------------
        /*
        【函数卡片】QInputDialog::getText()

        语法：
          QString getText(
              QWidget *parent,
              const QString &title,   // 对话框标题
              const QString &label,   // 输入框上方的提示文字
              QLineEdit::EchoMode mode = Normal,  // 显示模式（Normal/Password）
              const QString &text = QString(),    // 默认值
              bool *ok = nullptr      // 输出参数：用户是否点了 OK
          )

        返回值：用户输入的字符串（取消时返回空字符串）
        ⭐ 重要：必须检查 ok 参数，区分"输入空字符串"和"点了取消"
        */
        bool ok;
        QString name = QInputDialog::getText(
            this,
            "输入姓名",          // 标题
            "请输入您的姓名：",   // 提示文字
            QLineEdit::Normal,   // 普通显示（不隐藏）
            "张三",              // 默认值
            &ok                  // 接收用户是否点了 OK
        );

        if (ok && !name.isEmpty()) {
            m_resultLabel->setText("用户输入了姓名：" + name);
        } else if (ok) {
            m_resultLabel->setText("用户点了 OK 但没有输入内容");
        } else {
            m_resultLabel->setText("用户点了取消");
        }
    }

    void showGetInt()
    {
        // --------------------------------------------
        // QInputDialog::getInt — 获取整数输入
        // --------------------------------------------
        /*
        语法：
          int getInt(
              QWidget *parent,
              const QString &title,
              const QString &label,
              int value = 0,      // 默认值
              int min = -2147483647,  // 最小值
              int max = 2147483647,   // 最大值
              int step = 1,       // 步长
              bool *ok = nullptr
          )
        */
        bool ok;
        int age = QInputDialog::getInt(
            this,
            "输入年龄",
            "请输入您的年龄：",
            25,    // 默认值
            1,     // 最小值
            120,   // 最大值
            1,     // 步长
            &ok
        );

        if (ok) {
            m_resultLabel->setText(QString("用户输入了年龄：%1 岁").arg(age));
        } else {
            m_resultLabel->setText("用户取消了输入");
        }
    }

    void showGetItem()
    {
        // --------------------------------------------
        // QInputDialog::getItem — 从列表中选择
        // --------------------------------------------
        /*
        语法：
          QString getItem(
              QWidget *parent,
              const QString &title,
              const QString &label,
              const QStringList &items,  // 选项列表
              int current = 0,           // 默认选中项索引
              bool editable = true,      // 是否允许手动输入
              bool *ok = nullptr
          )
        */
        QStringList cities = {"北京", "上海", "广州", "深圳", "杭州", "成都"};
        bool ok;
        QString city = QInputDialog::getItem(
            this,
            "选择城市",
            "请选择您所在的城市：",
            cities,
            0,      // 默认选中第0项（北京）
            false,  // 不允许手动输入（只能从列表选）
            &ok
        );

        if (ok) {
            m_resultLabel->setText("用户选择了城市：" + city);
        } else {
            m_resultLabel->setText("用户取消了选择");
        }
    }

    void showGetDouble()
    {
        // --------------------------------------------
        // QInputDialog::getDouble — 获取浮点数输入
        // --------------------------------------------
        bool ok;
        double price = QInputDialog::getDouble(
            this,
            "输入价格",
            "请输入商品价格（元）：",
            99.99,   // 默认值
            0.0,     // 最小值
            99999.0, // 最大值
            2,       // 小数位数
            &ok
        );

        if (ok) {
            m_resultLabel->setText(QString("用户输入了价格：¥%1").arg(price, 0, 'f', 2));
        } else {
            m_resultLabel->setText("用户取消了输入");
        }
    }

    // ============================================
    // 第三部分：QFileDialog
    // ============================================

    void showOpenFile()
    {
        // --------------------------------------------
        // QFileDialog::getOpenFileName — 打开单个文件
        // --------------------------------------------
        /*
        【函数卡片】QFileDialog::getOpenFileName()

        语法：
          QString getOpenFileName(
              QWidget *parent,
              const QString &caption,   // 对话框标题
              const QString &dir,       // 初始目录
              const QString &filter,    // 文件过滤器
              QString *selectedFilter = nullptr,  // 输出：用户选择的过滤器
              Options options = Options()
          )

        返回值：用户选择的文件路径（取消时返回空字符串）

        过滤器格式：
          "图片文件 (*.png *.jpg *.bmp)"
          "文本文件 (*.txt);;所有文件 (*.*)"  // 多个过滤器用 ;; 分隔

        ⭐ 重要：用户取消时返回空字符串，必须检查 !file.isEmpty()
        */
        QString file = QFileDialog::getOpenFileName(
            this,
            "打开文件",                    // 标题
            QDir::homePath(),             // 初始目录（用户主目录）
            "图片文件 (*.png *.jpg *.bmp *.gif);;"  // 过滤器1
            "文本文件 (*.txt *.md);;"               // 过滤器2
            "所有文件 (*.*)"                         // 过滤器3
        );

        if (!file.isEmpty()) {
            m_resultLabel->setText("选择的文件：" + file);
        } else {
            m_resultLabel->setText("用户取消了文件选择");
        }
    }

    void showSaveFile()
    {
        // --------------------------------------------
        // QFileDialog::getSaveFileName — 保存文件
        // --------------------------------------------
        /*
        与 getOpenFileName 类似，但用于指定保存路径
        如果用户输入的文件名已存在，会自动提示是否覆盖
        */
        QString file = QFileDialog::getSaveFileName(
            this,
            "保存文件",
            QDir::homePath() + "/untitled.txt",  // 默认文件名
            "文本文件 (*.txt);;所有文件 (*.*)"
        );

        if (!file.isEmpty()) {
            m_resultLabel->setText("保存路径：" + file);
        } else {
            m_resultLabel->setText("用户取消了保存");
        }
    }

    void showOpenDir()
    {
        // --------------------------------------------
        // QFileDialog::getExistingDirectory — 选择目录
        // --------------------------------------------
        QString dir = QFileDialog::getExistingDirectory(
            this,
            "选择目录",
            QDir::homePath()
        );

        if (!dir.isEmpty()) {
            m_resultLabel->setText("选择的目录：" + dir);
        } else {
            m_resultLabel->setText("用户取消了目录选择");
        }
    }

    void showOpenFiles()
    {
        // --------------------------------------------
        // QFileDialog::getOpenFileNames — 选择多个文件
        // --------------------------------------------
        /*
        返回值：QStringList（文件路径列表）
        用途：批量导入文件等场景
        */
        QStringList files = QFileDialog::getOpenFileNames(
            this,
            "选择多个文件",
            QDir::homePath(),
            "所有文件 (*.*)"
        );

        if (!files.isEmpty()) {
            m_resultLabel->setText(
                QString("选择了 %1 个文件：\n%2")
                    .arg(files.size())
                    .arg(files.join("\n"))
            );
        } else {
            m_resultLabel->setText("用户取消了选择");
        }
    }

    // ============================================
    // 第四部分：QColorDialog / QFontDialog
    // ============================================

    void showColorDialog()
    {
        // --------------------------------------------
        // QColorDialog::getColor — 选择颜色
        // --------------------------------------------
        /*
        【函数卡片】QColorDialog::getColor()

        语法：
          QColor getColor(
              const QColor &initial = Qt::white,  // 初始颜色
              QWidget *parent = nullptr,
              const QString &title = QString(),
              ColorDialogOptions options = ColorDialogOptions()
          )

        返回值：QColor 对象（用户取消时返回无效颜色）
        ⭐ 重要：用 color.isValid() 检查用户是否取消
        */
        QColor color = QColorDialog::getColor(
            Qt::blue,   // 初始颜色
            this,
            "选择颜色"
        );

        if (color.isValid()) {
            // 用选中的颜色更新标签背景
            m_resultLabel->setStyleSheet(
                QString("QLabel { background: %1; border: 1px solid #ccc; "
                        "padding: 10px; border-radius: 5px; min-height: 40px; }")
                    .arg(color.name())
            );
            m_resultLabel->setText("选择的颜色：" + color.name());
        } else {
            m_resultLabel->setText("用户取消了颜色选择");
        }
    }

    void showFontDialog()
    {
        // --------------------------------------------
        // QFontDialog::getFont — 选择字体
        // --------------------------------------------
        /*
        【函数卡片】QFontDialog::getFont()

        语法：
          QFont getFont(
              bool *ok,               // 输出：用户是否点了 OK
              const QFont &initial,   // 初始字体
              QWidget *parent = nullptr,
              const QString &title = QString()
          )

        返回值：QFont 对象
        */
        bool ok;
        QFont font = QFontDialog::getFont(
            &ok,
            QFont("Arial", 12),  // 初始字体
            this,
            "选择字体"
        );

        if (ok) {
            m_resultLabel->setFont(font);
            m_resultLabel->setText(
                QString("选择的字体：%1，大小：%2pt")
                    .arg(font.family())
                    .arg(font.pointSize())
            );
        } else {
            m_resultLabel->setText("用户取消了字体选择");
        }
    }

private:
    QLabel *m_resultLabel;

    // 创建分组标题标签
    QLabel *makeTitle(const QString &text)
    {
        QLabel *label = new QLabel(text);
        label->setStyleSheet(
            "QLabel { font-weight: bold; color: #2c3e50; "
            "border-bottom: 2px solid #3498db; padding-bottom: 4px; }"
        );
        return label;
    }
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：不检查 question() 的返回值
    QMessageBox::question(this, "确认", "删除？");
    // 直接执行删除 ← 无论用户点 Yes 还是 No 都会删除！

✅ 正确：
    auto reply = QMessageBox::question(this, "确认", "删除？",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) { // 执行删除 }

❌ 错误2：不检查文件路径是否为空
    QString file = QFileDialog::getOpenFileName(...);
    QFile f(file);  // 用户取消时 file 为空，会崩溃！

✅ 正确：
    QString file = QFileDialog::getOpenFileName(...);
    if (!file.isEmpty()) { QFile f(file); ... }

❌ 错误3：不检查 QInputDialog 的 ok 参数
    QString name = QInputDialog::getText(..., &ok);
    // 直接使用 name ← 用户取消时 name 为空字符串，无法区分"取消"和"输入空"

✅ 正确：
    bool ok;
    QString name = QInputDialog::getText(..., &ok);
    if (ok) { // 用户点了 OK，使用 name }

❌ 错误4：不检查 QColor::isValid()
    QColor color = QColorDialog::getColor(...);
    widget->setStyleSheet("background: " + color.name());  // 取消时 color 无效！

✅ 正确：
    QColor color = QColorDialog::getColor(...);
    if (color.isValid()) { widget->setStyleSheet(...); }

❌ 错误5：文件过滤器格式错误
    "*.png, *.jpg"   ← 错误！逗号分隔不起作用
    "*.png *.jpg"    ← 正确！空格分隔多个扩展名
    "图片 (*.png *.jpg);;文本 (*.txt)"  ← 正确！;; 分隔多个过滤器
*/

// ============================================
// 练习题
// ============================================
/*
1. 创建一个"文本编辑器"界面：
   - 包含一个 QTextEdit（多行文本框）
   - "打开"按钮：用 QFileDialog 选择 .txt 文件，读取内容显示在 QTextEdit 中
   - "保存"按钮：用 QFileDialog 选择保存路径，将 QTextEdit 内容写入文件
   - 关闭前用 QMessageBox::question 询问是否保存
   提示：使用 QFile 和 QTextStream 读写文件

2. 创建一个"颜色主题选择器"：
   - 三个按钮：选择背景色、选择文字色、选择边框色
   - 每次选择后实时更新一个预览区域的样式
   - 底部显示当前三个颜色的十六进制值
   提示：用 QColorDialog::getColor() 获取颜色，用 setStyleSheet() 应用

3. 创建一个"批量文件处理"界面：
   - "选择文件"按钮：用 getOpenFileNames() 选择多个文件
   - 用 QListWidget 显示已选择的文件列表
   - "清空列表"按钮：弹出 question 确认后清空
   - 显示文件总数和总大小
   提示：用 QFileInfo 获取文件大小
*/

#include "12_dialogs_basics.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    DialogsDemo window;
    window.show();
    return app.exec();
}
