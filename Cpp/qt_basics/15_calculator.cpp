/*
 * Qt 实战项目：简单计算器
 * 日期：2026-06-02（第42天）
 * 难度：⭐⭐ 中级
 *
 * 综合运用：
 * - QGridLayout（按钮布局）
 * - Qt Style Sheets（深色主题）
 * - 信号与槽（Lambda 捕获）
 * - 状态机（计算器逻辑）
 *
 * 编译：qmake + make，或在 Qt Creator 中直接运行
 */

// ============================================
// 【核心概念】计算器状态机
// ============================================
/*
计算器在任意时刻处于以下状态之一：

  状态A：输入第一个数
    - 初始状态，或按下 C 后
    - 数字键：追加到显示屏
    - 运算符键：保存第一个数，切换到状态B

  状态B：输入第二个数
    - 按下运算符后进入
    - 数字键：清空显示屏，开始输入第二个数
    - 运算符键：先计算当前结果，再保存新运算符（连续运算）
    - 等号键：计算结果，切换到状态C

  状态C：显示结果
    - 按下 = 后进入
    - 数字键：清空，开始输入新的第一个数（回到状态A）
    - 运算符键：以当前结果为第一个数，继续运算（切换到状态B）

关键成员变量：
  m_firstNum       — 第一个操作数
  m_operator       — 当前运算符（"+"、"-"、"×"、"÷"）
  m_waitingSecond  — true 表示正在等待输入第二个数（状态B）
  m_justCalc       — true 表示刚按了 =（状态C）
*/

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFont>
#include <cmath>

class Calculator : public QWidget
{
    Q_OBJECT

public:
    Calculator(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("计算器");
        setFixedSize(320, 480);  // 固定大小，不允许缩放

        // ----------------------------------------
        // 整体布局
        // ----------------------------------------
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(12, 12, 12, 12);

        // ----------------------------------------
        // 显示屏
        // ----------------------------------------
        /*
        用 QLineEdit 作为显示屏：
        - setReadOnly(true)：只读，用户不能直接输入
        - setAlignment(Qt::AlignRight)：数字右对齐（符合计算器习惯）
        - 大字体，让数字清晰可见
        */
        m_display = new QLineEdit("0");
        m_display->setReadOnly(true);
        m_display->setAlignment(Qt::AlignRight);
        m_display->setFont(QFont("Arial", 28, QFont::Bold));
        m_display->setFixedHeight(80);
        m_display->setStyleSheet(
            "QLineEdit {"
            "  background: #1a1a2e;"
            "  color: #e0e0ff;"
            "  border: none;"
            "  border-radius: 8px;"
            "  padding: 0 16px;"
            "}"
        );
        mainLayout->addWidget(m_display);
        mainLayout->addSpacing(8);

        // ----------------------------------------
        // 按钮网格（QGridLayout）
        // ----------------------------------------
        /*
        布局示意：
          行0: [C]   [⌫]  [%]   [÷]
          行1: [7]   [8]  [9]   [×]
          行2: [4]   [5]  [6]   [-]
          行3: [1]   [2]  [3]   [+]
          行4: [0(跨2列)]  [.]  [=]
        */
        QGridLayout *grid = new QGridLayout();
        grid->setSpacing(8);

        // 按钮定义：{文字, 行, 列, 跨行数, 跨列数, 类型}
        // 类型：0=数字, 1=运算符, 2=等号, 3=功能键
        struct BtnDef {
            QString text;
            int row, col, rowSpan, colSpan;
            int type;  // 0数字 1运算符 2等号 3功能
        };

        QList<BtnDef> buttons = {
            // 第0行：功能键 + 运算符
            {"C",  0, 0, 1, 1, 3},
            {"⌫", 0, 1, 1, 1, 3},
            {"%",  0, 2, 1, 1, 1},
            {"÷",  0, 3, 1, 1, 1},
            // 第1行
            {"7",  1, 0, 1, 1, 0},
            {"8",  1, 1, 1, 1, 0},
            {"9",  1, 2, 1, 1, 0},
            {"×",  1, 3, 1, 1, 1},
            // 第2行
            {"4",  2, 0, 1, 1, 0},
            {"5",  2, 1, 1, 1, 0},
            {"6",  2, 2, 1, 1, 0},
            {"-",  2, 3, 1, 1, 1},
            // 第3行
            {"1",  3, 0, 1, 1, 0},
            {"2",  3, 1, 1, 1, 0},
            {"3",  3, 2, 1, 1, 0},
            {"+",  3, 3, 1, 1, 1},
            // 第4行：0 跨2列
            {"0",  4, 0, 1, 2, 0},
            {".",  4, 2, 1, 1, 0},
            {"=",  4, 3, 1, 1, 2},
        };

        // 各类型按钮的样式
        // 用 R"(...)" 原始字符串避免转义
        QString styleNum = R"(
            QPushButton {
                background: #2d2d44;
                color: #cdd6f4;
                border: none;
                border-radius: 8px;
                font-size: 20px;
                font-weight: bold;
            }
            QPushButton:hover  { background: #3d3d5c; }
            QPushButton:pressed { background: #1e1e30; }
        )";

        QString styleOp = R"(
            QPushButton {
                background: #3d5a80;
                color: #89b4fa;
                border: none;
                border-radius: 8px;
                font-size: 20px;
                font-weight: bold;
            }
            QPushButton:hover  { background: #4d6a90; }
            QPushButton:pressed { background: #2d4a70; }
        )";

        QString styleEq = R"(
            QPushButton {
                background: #2d6a4f;
                color: #a6e3a1;
                border: none;
                border-radius: 8px;
                font-size: 22px;
                font-weight: bold;
            }
            QPushButton:hover  { background: #3d7a5f; }
            QPushButton:pressed { background: #1d5a3f; }
        )";

        QString styleFunc = R"(
            QPushButton {
                background: #5a2d2d;
                color: #f38ba8;
                border: none;
                border-radius: 8px;
                font-size: 18px;
                font-weight: bold;
            }
            QPushButton:hover  { background: #6a3d3d; }
            QPushButton:pressed { background: #4a1d1d; }
        )";

        // 创建所有按钮并添加到网格
        for (const BtnDef &def : buttons) {
            QPushButton *btn = new QPushButton(def.text);
            btn->setFixedHeight(60);

            // 根据类型设置样式
            switch (def.type) {
                case 0: btn->setStyleSheet(styleNum);  break;
                case 1: btn->setStyleSheet(styleOp);   break;
                case 2: btn->setStyleSheet(styleEq);   break;
                case 3: btn->setStyleSheet(styleFunc); break;
            }

            // 用 Lambda 捕获按钮文字，连接点击信号
            // [this, text = def.text] 捕获 this 指针和按钮文字
            QString text = def.text;
            connect(btn, &QPushButton::clicked, this, [this, text]() {
                onButtonClicked(text);
            });

            grid->addWidget(btn, def.row, def.col, def.rowSpan, def.colSpan);
        }

        mainLayout->addLayout(grid);

        // 设置窗口背景色
        setStyleSheet("QWidget { background: #13131f; }");
    }

private slots:

    // ============================================
    // 按钮点击统一处理入口
    // ============================================
    void onButtonClicked(const QString &text)
    {
        if (text >= "0" && text <= "9") {
            onDigit(text);
        } else if (text == ".") {
            onDot();
        } else if (text == "C") {
            onClear();
        } else if (text == "⌫") {
            onBackspace();
        } else if (text == "%" ) {
            onPercent();
        } else if (text == "=" ) {
            onEquals();
        } else {
            // +、-、×、÷
            onOperator(text);
        }
    }

    // ----------------------------------------
    // 数字键：0-9
    // ----------------------------------------
    void onDigit(const QString &digit)
    {
        if (m_waitingSecond || m_justCalc) {
            // 状态B/C：清空显示屏，开始输入新数字
            m_display->setText(digit);
            m_waitingSecond = false;
            m_justCalc = false;
        } else {
            // 状态A：追加数字
            QString current = m_display->text();
            if (current == "0") {
                m_display->setText(digit);  // 替换前导零
            } else {
                m_display->setText(current + digit);
            }
        }
    }

    // ----------------------------------------
    // 小数点键
    // ----------------------------------------
    void onDot()
    {
        if (m_waitingSecond || m_justCalc) {
            // 开始输入新数字，以 "0." 开头
            m_display->setText("0.");
            m_waitingSecond = false;
            m_justCalc = false;
            return;
        }

        // 同一个数只能有一个小数点
        if (!m_display->text().contains(".")) {
            m_display->setText(m_display->text() + ".");
        }
    }

    // ----------------------------------------
    // 运算符键：+、-、×、÷
    // ----------------------------------------
    void onOperator(const QString &op)
    {
        double current = m_display->text().toDouble();

        if (!m_operator.isEmpty() && !m_waitingSecond) {
            // 连续运算：先计算上一步的结果
            // 例如：1 + 2 × → 先算 1+2=3，再以3为第一个数
            double result = calculate(m_firstNum, current, m_operator);
            if (std::isnan(result)) return;  // 除零，已在 calculate 中处理
            m_firstNum = result;
            m_display->setText(formatNumber(result));
        } else {
            m_firstNum = current;
        }

        m_operator = op;
        m_waitingSecond = true;
        m_justCalc = false;
    }

    // ----------------------------------------
    // 等号键
    // ----------------------------------------
    void onEquals()
    {
        if (m_operator.isEmpty()) return;  // 没有运算符，不做任何事

        double second = m_display->text().toDouble();
        double result = calculate(m_firstNum, second, m_operator);
        if (std::isnan(result)) return;

        m_display->setText(formatNumber(result));
        m_operator = "";
        m_waitingSecond = false;
        m_justCalc = true;  // 标记刚按了 =
    }

    // ----------------------------------------
    // 清除键（C）：重置所有状态
    // ----------------------------------------
    void onClear()
    {
        m_display->setText("0");
        m_firstNum = 0;
        m_operator = "";
        m_waitingSecond = false;
        m_justCalc = false;
    }

    // ----------------------------------------
    // 退格键（⌫）：删除最后一位
    // ----------------------------------------
    void onBackspace()
    {
        if (m_waitingSecond || m_justCalc) return;  // 等待输入时不退格

        QString current = m_display->text();
        if (current.length() <= 1 || (current.length() == 2 && current.startsWith("-"))) {
            m_display->setText("0");  // 只剩一位时清零
        } else {
            m_display->setText(current.left(current.length() - 1));
        }
    }

    // ----------------------------------------
    // 百分比键（%）：当前数字除以100
    // ----------------------------------------
    void onPercent()
    {
        double value = m_display->text().toDouble() / 100.0;
        m_display->setText(formatNumber(value));
    }

private:

    // ============================================
    // 核心计算函数
    // ============================================
    double calculate(double a, double b, const QString &op)
    {
        if (op == "+") return a + b;
        if (op == "-") return a - b;
        if (op == "×") return a * b;
        if (op == "÷") {
            // 除零保护
            if (b == 0) {
                QMessageBox::warning(this, "错误", "除数不能为零！");
                return std::numeric_limits<double>::quiet_NaN();
            }
            return a / b;
        }
        return b;
    }

    // ============================================
    // 数字格式化：去掉多余的小数零
    // ============================================
    /*
    例如：
      3.0      → "3"
      3.14     → "3.14"
      3.10     → "3.1"
      1234567  → "1234567"
    */
    QString formatNumber(double value)
    {
        // 检查是否为整数
        if (value == std::floor(value) && std::abs(value) < 1e15) {
            return QString::number(static_cast<long long>(value));
        }
        // 浮点数：最多保留10位有效数字，去掉尾部零
        QString s = QString::number(value, 'g', 10);
        return s;
    }

    // ============================================
    // 成员变量（计算器状态）
    // ============================================
    QLineEdit *m_display;       // 显示屏
    double     m_firstNum = 0;  // 第一个操作数
    QString    m_operator = ""; // 当前运算符
    bool m_waitingSecond = false; // 是否等待输入第二个数
    bool m_justCalc = false;      // 是否刚按了 =
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：没有处理连续运算
    按下 1 + 2 + 时，第二个 + 应该先算出 1+2=3
    再以 3 为第一个数继续运算。
    如果不处理，第二个 + 会覆盖第一个 +，导致结果错误。

✅ 正确：在 onOperator() 中检查 !m_waitingSecond，先计算再更新

❌ 错误2：小数点重复输入
    用户可能连续按两次 "."，导致 "3.." 这样的非法数字。

✅ 正确：用 contains(".") 检查，已有小数点则忽略

❌ 错误3：没有除零保护
    直接计算 a / b，当 b == 0 时程序崩溃或产生 inf。

✅ 正确：在 calculate() 中检查 b == 0，弹出警告并返回 NaN

❌ 错误4：Lambda 捕获问题
    for (auto &def : buttons) {
        connect(btn, ..., [this, def.text]() { ... });  // 错误！引用捕获循环变量
    }
    循环结束后 def 已销毁，Lambda 访问悬空引用。

✅ 正确：值捕获，在循环内先复制到局部变量
    QString text = def.text;  // 复制到局部变量
    connect(btn, ..., [this, text]() { ... });  // 值捕获

❌ 错误5：显示屏直接用 QLabel
    QLabel 不支持文字溢出时自动缩小字体，长数字会超出边界。

✅ 正确：用 QLineEdit（只读），或在 QLabel 上设置 setScaledContents

❌ 错误6：整数结果显示为 "3.0"
    QString::number(3.0) 可能输出 "3"，但某些情况下会输出 "3.0"。

✅ 正确：用 formatNumber() 函数统一处理，整数去掉小数部分
*/

// ============================================
// 练习题
// ============================================
/*
1. 添加键盘支持：
   - 按数字键 0-9 等同于点击对应按钮
   - 按 Enter 等同于点击 =
   - 按 Escape 等同于点击 C
   - 按 Backspace 等同于点击 ⌫
   提示：重写 keyPressEvent(QKeyEvent *event)

2. 添加历史记录功能：
   - 每次按 = 后，将算式和结果记录到 QListWidget
   - 点击历史记录中的某条，将结果填入显示屏
   提示：用 QString 拼接算式，如 "1 + 2 = 3"

3. 添加科学计算器功能（扩展）：
   - 添加 sin、cos、tan、sqrt、x² 按钮
   - 用 <cmath> 中的函数实现
   - 三角函数注意角度/弧度转换（Qt 默认弧度）
   提示：M_PI 是圆周率，角度转弧度：angle * M_PI / 180
*/

#include "15_calculator.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Calculator window;
    window.show();
    return app.exec();
}
