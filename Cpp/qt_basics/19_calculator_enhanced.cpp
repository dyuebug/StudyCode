/*
 * Qt 综合扩展练习B：计算器 — 键盘支持 + 历史记录
 * 日期：2026-06-04（第44天）
 * 难度：⭐⭐ 中级
 *
 * 在第42天计算器基础上，新增：
 * - 键盘支持（keyPressEvent）：数字键/运算符/Enter/Backspace/Escape
 * - 历史记录（QListWidget）：每次按 = 追加一条算式记录
 * - 点击历史记录可将结果填入显示屏
 *
 * 核心知识点：
 * - keyPressEvent（键盘事件）
 * - QListWidget（列表控件）
 * - QSplitter（分割器，左右布局）
 *
 * 编译：qmake + make，或在 Qt Creator 中直接运行
 */

// ============================================
// 【核心概念】键盘事件
// ============================================
/*
重写 keyPressEvent(QKeyEvent *event) 可以捕获键盘输入。

常用键值：
  Qt::Key_0 ~ Qt::Key_9   — 数字键
  Qt::Key_Plus             — +
  Qt::Key_Minus            — -
  Qt::Key_Asterisk         — *（对应 ×）
  Qt::Key_Slash            — /（对应 ÷）
  Qt::Key_Return           — 回车（主键盘）
  Qt::Key_Enter            — 回车（小键盘）
  Qt::Key_Backspace        — 退格
  Qt::Key_Escape           — Esc（对应 C）
  Qt::Key_Period           — .（小数点）
  Qt::Key_Percent          — %

注意：
- 必须调用 setFocusPolicy(Qt::StrongFocus) 才能接收键盘事件
- 或者在主窗口上重写，而不是在子控件上
*/

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QKeyEvent>
#include <QFont>
#include <cmath>
#include <limits>

class CalculatorWithHistory : public QWidget
{
    Q_OBJECT

public:
    CalculatorWithHistory(QWidget *parent = nullptr) : QWidget(parent)
    {
        setWindowTitle("计算器（含键盘支持和历史记录）");
        setMinimumSize(500, 480);

        // 接收键盘事件
        setFocusPolicy(Qt::StrongFocus);

        // ----------------------------------------
        // 整体布局：左边计算器，右边历史记录
        // ----------------------------------------
        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // 左侧：计算器主体
        QWidget *calcWidget = new QWidget();
        calcWidget->setFixedWidth(320);
        QVBoxLayout *calcLayout = new QVBoxLayout(calcWidget);
        calcLayout->setSpacing(0);
        calcLayout->setContentsMargins(12, 12, 12, 12);

        // 显示屏
        m_display = new QLineEdit("0");
        m_display->setReadOnly(true);
        m_display->setAlignment(Qt::AlignRight);
        m_display->setFont(QFont("Arial", 28, QFont::Bold));
        m_display->setFixedHeight(80);
        m_display->setStyleSheet(
            "QLineEdit { background: #1a1a2e; color: #e0e0ff; "
            "border: none; border-radius: 8px; padding: 0 16px; }"
        );
        calcLayout->addWidget(m_display);
        calcLayout->addSpacing(8);

        // 按钮网格
        QGridLayout *grid = new QGridLayout();
        grid->setSpacing(8);

        struct BtnDef { QString text; int row, col, rowSpan, colSpan, type; };
        QList<BtnDef> buttons = {
            {"C",  0,0,1,1,3}, {"⌫",0,1,1,1,3}, {"%",0,2,1,1,1}, {"÷",0,3,1,1,1},
            {"7",  1,0,1,1,0}, {"8",1,1,1,1,0}, {"9",1,2,1,1,0}, {"×",1,3,1,1,1},
            {"4",  2,0,1,1,0}, {"5",2,1,1,1,0}, {"6",2,2,1,1,0}, {"-",2,3,1,1,1},
            {"1",  3,0,1,1,0}, {"2",3,1,1,1,0}, {"3",3,2,1,1,0}, {"+",3,3,1,1,1},
            {"0",  4,0,1,2,0}, {".",4,2,1,1,0}, {"=",4,3,1,1,2},
        };

        QString styleNum  = "QPushButton{background:#2d2d44;color:#cdd6f4;border:none;border-radius:8px;font-size:20px;font-weight:bold;}QPushButton:hover{background:#3d3d5c;}QPushButton:pressed{background:#1e1e30;}";
        QString styleOp   = "QPushButton{background:#3d5a80;color:#89b4fa;border:none;border-radius:8px;font-size:20px;font-weight:bold;}QPushButton:hover{background:#4d6a90;}QPushButton:pressed{background:#2d4a70;}";
        QString styleEq   = "QPushButton{background:#2d6a4f;color:#a6e3a1;border:none;border-radius:8px;font-size:22px;font-weight:bold;}QPushButton:hover{background:#3d7a5f;}QPushButton:pressed{background:#1d5a3f;}";
        QString styleFunc = "QPushButton{background:#5a2d2d;color:#f38ba8;border:none;border-radius:8px;font-size:18px;font-weight:bold;}QPushButton:hover{background:#6a3d3d;}QPushButton:pressed{background:#4a1d1d;}";

        for (const BtnDef &def : buttons) {
            QPushButton *btn = new QPushButton(def.text);
            btn->setFixedHeight(60);
            switch (def.type) {
                case 0: btn->setStyleSheet(styleNum);  break;
                case 1: btn->setStyleSheet(styleOp);   break;
                case 2: btn->setStyleSheet(styleEq);   break;
                case 3: btn->setStyleSheet(styleFunc); break;
            }
            QString text = def.text;
            connect(btn, &QPushButton::clicked, this, [this, text]() {
                onButtonClicked(text);
            });
            grid->addWidget(btn, def.row, def.col, def.rowSpan, def.colSpan);
        }
        calcLayout->addLayout(grid);

        // 键盘提示
        QLabel *hint = new QLabel("支持键盘输入 | Enter=等号 | Esc=清除");
        hint->setStyleSheet("color: #666; font-size: 11px; padding: 4px;");
        hint->setAlignment(Qt::AlignCenter);
        calcLayout->addWidget(hint);

        mainLayout->addWidget(calcWidget);

        // 右侧：历史记录
        QWidget *histWidget = new QWidget();
        histWidget->setMinimumWidth(180);
        QVBoxLayout *histLayout = new QVBoxLayout(histWidget);
        histLayout->setContentsMargins(8, 12, 12, 12);
        histLayout->setSpacing(6);

        QLabel *histTitle = new QLabel("历史记录");
        histTitle->setStyleSheet(
            "font-weight: bold; color: #cdd6f4; font-size: 14px; padding: 4px 0;"
        );
        histLayout->addWidget(histTitle);

        m_historyList = new QListWidget();
        m_historyList->setStyleSheet(R"(
            QListWidget {
                background: #1a1a2e;
                border: none;
                border-radius: 8px;
                color: #cdd6f4;
                font-size: 13px;
                padding: 4px;
            }
            QListWidget::item {
                padding: 6px 8px;
                border-radius: 4px;
            }
            QListWidget::item:hover { background: #2d2d44; }
            QListWidget::item:selected { background: #3d5a80; }
        )");
        m_historyList->setToolTip("点击历史记录可将结果填入显示屏");
        histLayout->addWidget(m_historyList);

        QPushButton *clearHistBtn = new QPushButton("清空历史");
        clearHistBtn->setStyleSheet(
            "QPushButton { background: #3d3d44; color: #888; border: none; "
            "border-radius: 4px; padding: 6px; font-size: 12px; }"
            "QPushButton:hover { background: #5a2d2d; color: #f38ba8; }"
        );
        connect(clearHistBtn, &QPushButton::clicked, m_historyList, &QListWidget::clear);
        histLayout->addWidget(clearHistBtn);

        mainLayout->addWidget(histWidget);

        // 点击历史记录，将结果填入显示屏
        connect(m_historyList, &QListWidget::itemClicked,
                this, &CalculatorWithHistory::onHistoryClicked);

        setStyleSheet("QWidget { background: #13131f; }");
    }

protected:
    // ============================================
    // 键盘事件处理
    // ============================================
    /*
    【函数卡片】keyPressEvent()

    作用：重写键盘按下事件
    语法：void keyPressEvent(QKeyEvent *event) override
    参数：
      event->key()  — 按下的键（Qt::Key_0 等）
      event->text() — 按键对应的字符（"0"、"+"等）

    注意：必须先调用 setFocusPolicy(Qt::StrongFocus)
    */
    void keyPressEvent(QKeyEvent *event) override
    {
        switch (event->key()) {
            // 数字键 0-9
            case Qt::Key_0: case Qt::Key_1: case Qt::Key_2:
            case Qt::Key_3: case Qt::Key_4: case Qt::Key_5:
            case Qt::Key_6: case Qt::Key_7: case Qt::Key_8:
            case Qt::Key_9:
                onButtonClicked(event->text());
                break;

            // 小数点
            case Qt::Key_Period:
                onButtonClicked(".");
                break;

            // 运算符
            case Qt::Key_Plus:
                onButtonClicked("+");
                break;
            case Qt::Key_Minus:
                onButtonClicked("-");
                break;
            case Qt::Key_Asterisk:
                onButtonClicked("×");
                break;
            case Qt::Key_Slash:
                onButtonClicked("÷");
                break;
            case Qt::Key_Percent:
                onButtonClicked("%");
                break;

            // 等号（回车键，主键盘和小键盘都支持）
            case Qt::Key_Return:
            case Qt::Key_Enter:
                onButtonClicked("=");
                break;

            // 退格
            case Qt::Key_Backspace:
                onButtonClicked("⌫");
                break;

            // Esc = 清除
            case Qt::Key_Escape:
                onButtonClicked("C");
                break;

            default:
                QWidget::keyPressEvent(event);  // 其他键交给父类处理
                break;
        }
    }

private slots:

    void onButtonClicked(const QString &text)
    {
        if (text >= "0" && text <= "9") onDigit(text);
        else if (text == ".")  onDot();
        else if (text == "C")  onClear();
        else if (text == "⌫") onBackspace();
        else if (text == "%")  onPercent();
        else if (text == "=")  onEquals();
        else                   onOperator(text);
    }

    void onHistoryClicked(QListWidgetItem *item)
    {
        // 从历史记录中提取结果（格式："算式 = 结果"）
        QString text = item->text();
        int eqPos = text.lastIndexOf("= ");
        if (eqPos >= 0) {
            QString result = text.mid(eqPos + 2);
            m_display->setText(result);
            m_firstNum = result.toDouble();
            m_operator = "";
            m_waitingSecond = false;
            m_justCalc = true;
        }
    }

    void onDigit(const QString &digit)
    {
        if (m_waitingSecond || m_justCalc) {
            m_display->setText(digit);
            m_waitingSecond = false;
            m_justCalc = false;
        } else {
            QString current = m_display->text();
            m_display->setText(current == "0" ? digit : current + digit);
        }
    }

    void onDot()
    {
        if (m_waitingSecond || m_justCalc) {
            m_display->setText("0.");
            m_waitingSecond = false;
            m_justCalc = false;
            return;
        }
        if (!m_display->text().contains("."))
            m_display->setText(m_display->text() + ".");
    }

    void onOperator(const QString &op)
    {
        double current = m_display->text().toDouble();
        if (!m_operator.isEmpty() && !m_waitingSecond) {
            double result = calculate(m_firstNum, current, m_operator);
            if (std::isnan(result)) return;
            m_firstNum = result;
            m_display->setText(formatNumber(result));
        } else {
            m_firstNum = current;
        }
        m_operator = op;
        m_waitingSecond = true;
        m_justCalc = false;
    }

    void onEquals()
    {
        if (m_operator.isEmpty()) return;

        double second = m_display->text().toDouble();
        double result = calculate(m_firstNum, second, m_operator);
        if (std::isnan(result)) return;

        // 构建历史记录条目
        QString entry = QString("%1 %2 %3 = %4")
            .arg(formatNumber(m_firstNum))
            .arg(m_operator)
            .arg(formatNumber(second))
            .arg(formatNumber(result));

        // 添加到历史记录列表顶部
        m_historyList->insertItem(0, entry);

        // 最多保留20条
        while (m_historyList->count() > 20)
            delete m_historyList->takeItem(m_historyList->count() - 1);

        m_display->setText(formatNumber(result));
        m_operator = "";
        m_waitingSecond = false;
        m_justCalc = true;
    }

    void onClear()
    {
        m_display->setText("0");
        m_firstNum = 0;
        m_operator = "";
        m_waitingSecond = false;
        m_justCalc = false;
    }

    void onBackspace()
    {
        if (m_waitingSecond || m_justCalc) return;
        QString current = m_display->text();
        if (current.length() <= 1 || (current.length() == 2 && current.startsWith("-")))
            m_display->setText("0");
        else
            m_display->setText(current.left(current.length() - 1));
    }

    void onPercent()
    {
        double value = m_display->text().toDouble() / 100.0;
        m_display->setText(formatNumber(value));
    }

private:

    double calculate(double a, double b, const QString &op)
    {
        if (op == "+") return a + b;
        if (op == "-") return a - b;
        if (op == "×") return a * b;
        if (op == "÷") {
            if (b == 0) {
                QMessageBox::warning(this, "错误", "除数不能为零！");
                return std::numeric_limits<double>::quiet_NaN();
            }
            return a / b;
        }
        return b;
    }

    QString formatNumber(double value)
    {
        if (value == std::floor(value) && std::abs(value) < 1e15)
            return QString::number(static_cast<long long>(value));
        return QString::number(value, 'g', 10);
    }

    QLineEdit   *m_display;
    QListWidget *m_historyList;
    double       m_firstNum = 0;
    QString      m_operator = "";
    bool         m_waitingSecond = false;
    bool         m_justCalc = false;
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================
/*
❌ 错误1：没有设置 setFocusPolicy，键盘事件不触发
    // 窗口没有焦点策略，keyPressEvent 永远不会被调用

✅ 正确：
    setFocusPolicy(Qt::StrongFocus);

❌ 错误2：小键盘回车（Key_Enter）和主键盘回车（Key_Return）混淆
    case Qt::Key_Return: onEquals(); break;
    // 小键盘的回车是 Key_Enter，不是 Key_Return！

✅ 正确：两个都处理
    case Qt::Key_Return:
    case Qt::Key_Enter:
        onEquals(); break;

❌ 错误3：历史记录无限增长
    m_historyList->insertItem(0, entry);
    // 没有限制条数，长时间使用后列表很长

✅ 正确：限制最大条数
    while (m_historyList->count() > 20)
        delete m_historyList->takeItem(m_historyList->count() - 1);

❌ 错误4：键盘 * 键对应 Key_Asterisk，不是 Key_Multiply
    case Qt::Key_Multiply: ...  // 这是小键盘的 * 键
    case Qt::Key_Asterisk: ...  // 这是主键盘的 * 键（Shift+8）

✅ 正确：两个都处理，或只处理最常用的
*/

// ============================================
// 练习题
// ============================================
/*
1. 添加小键盘支持：
   - Qt::Key_0 ~ Qt::Key_9 是主键盘数字键
   - Qt::Key_0 + Qt::KeypadModifier 是小键盘数字键
   - 提示：检查 event->modifiers() & Qt::KeypadModifier

2. 添加历史记录搜索：
   - 在历史记录上方添加一个 QLineEdit
   - 输入文字时过滤历史记录（隐藏不匹配的条目）
   - 提示：遍历 QListWidget 的所有条目，用 item->setHidden(bool)

3. 添加内存功能（M+、M-、MR、MC）：
   - M+：将当前显示值加到内存
   - M-：从内存中减去当前显示值
   - MR：将内存值填入显示屏
   - MC：清除内存
   - 提示：用 double m_memory = 0 存储内存值
*/

#include "19_calculator_enhanced.moc"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    CalculatorWithHistory window;
    window.show();
    return app.exec();
}
