/*
 * Qt 核心概念详解
 *
 * 学习目标：
 * 1. 理解 QObject 基类
 * 2. 掌握信号与槽机制
 * 3. 了解 Qt 元对象系统
 * 4. 理解对象树和内存管理
 *
 * 编译说明：
 * 本文件是概念讲解文件，包含伪代码和示例
 * 实际的 Qt 项目需要在 Qt Creator 中创建
 */

#include <iostream>
using namespace std;

// ============================================================================
// 第一部分：QObject 基类
// ============================================================================

/*
 * QObject 是什么？
 *
 * QObject 是 Qt 对象模型的核心，所有 Qt 类的基类。
 *
 * 核心功能：
 * 1. 信号与槽机制
 * 2. 对象树（父子关系）
 * 3. 属性系统
 * 4. 事件处理
 * 5. 对象名称和查找
 */

// 示例：QObject 的基本使用（伪代码）
/*
class MyObject : public QObject
{
    Q_OBJECT  // 必须有这个宏！启用元对象系统

public:
    MyObject(QObject *parent = nullptr) : QObject(parent)
    {
        // 构造函数
    }

    ~MyObject()
    {
        // 析构函数
        // 注意：子对象会自动销毁，不需要手动 delete
    }
};
*/

// ============================================================================
// 第二部分：对象树和内存管理
// ============================================================================

/*
 * Qt 的对象树机制
 *
 * 核心思想：父对象销毁时，自动销毁所有子对象
 *
 * 优势：
 * 1. 简化内存管理
 * 2. 避免内存泄漏
 * 3. 符合对象生命周期
 *
 * 示例：
 *
 * QObject *parent = new QObject();
 * QObject *child1 = new QObject(parent);  // child1 的父对象是 parent
 * QObject *child2 = new QObject(parent);  // child2 的父对象是 parent
 *
 * delete parent;  // parent、child1、child2 都会被销毁
 *
 * 注意：
 * - 只需要 delete 父对象
 * - 子对象会自动销毁
 * - 不要手动 delete 子对象（除非你知道自己在做什么）
 */

// 对象树示例（伪代码）
/*
void objectTreeExample()
{
    // 创建父对象
    QWidget *window = new QWidget();
    window->setWindowTitle("对象树示例");

    // 创建子对象（按钮）
    QPushButton *button1 = new QPushButton("按钮1", window);
    QPushButton *button2 = new QPushButton("按钮2", window);
    QPushButton *button3 = new QPushButton("按钮3", window);

    // 对象树结构：
    // window (父对象)
    //   ├── button1 (子对象)
    //   ├── button2 (子对象)
    //   └── button3 (子对象)

    window->show();

    // 当 window 被销毁时，button1、button2、button3 会自动销毁
    // 不需要手动 delete button1, button2, button3
}
*/

// ============================================================================
// 第三部分：信号与槽机制（Signal & Slot）
// ============================================================================

/*
 * 什么是信号与槽？
 *
 * 信号（Signal）：对象发出的事件通知
 * 槽（Slot）：响应信号的函数
 *
 * 核心思想：
 * - 信号：当某个事件发生时，对象发出信号
 * - 槽：接收信号并执行相应的操作
 * - 连接：使用 connect() 函数连接信号和槽
 *
 * 优势：
 * 1. 松耦合：信号发送者不需要知道接收者
 * 2. 类型安全：编译期检查
 * 3. 灵活：一个信号可以连接多个槽，一个槽可以响应多个信号
 */

// 信号与槽示例（伪代码）
/*
class Counter : public QObject
{
    Q_OBJECT

public:
    Counter() : m_value(0) {}

    int value() const { return m_value; }

public slots:  // 槽函数
    void setValue(int value)
    {
        if (m_value != value) {
            m_value = value;
            emit valueChanged(value);  // 发出信号
        }
    }

signals:  // 信号声明
    void valueChanged(int newValue);  // 只声明，不实现

private:
    int m_value;
};

// 使用示例
void signalSlotExample()
{
    Counter a, b;

    // 连接信号和槽
    // 当 a 的值改变时，b 的值也会改变
    QObject::connect(&a, &Counter::valueChanged,
                     &b, &Counter::setValue);

    a.setValue(10);  // a 的值变为 10
    // 自动触发：b.setValue(10)，b 的值也变为 10

    cout << "a = " << a.value() << endl;  // 输出：a = 10
    cout << "b = " << b.value() << endl;  // 输出：b = 10
}
*/

// ============================================================================
// 第四部分：信号与槽的连接方式
// ============================================================================

/*
 * Qt 5 的连接方式（推荐）
 *
 * 语法：
 * connect(sender, &SenderClass::signalName,
 *         receiver, &ReceiverClass::slotName);
 *
 * 优势：
 * 1. 编译期检查：信号和槽的类型必须匹配
 * 2. 支持 Lambda 表达式
 * 3. 更安全
 */

// 连接方式示例（伪代码）
/*
void connectExample()
{
    QPushButton *button = new QPushButton("点击我");
    QLabel *label = new QLabel("未点击");

    // 方式1：连接到槽函数
    QObject::connect(button, &QPushButton::clicked,
                     label, &QLabel::clear);

    // 方式2：连接到 Lambda 表达式（推荐）
    QObject::connect(button, &QPushButton::clicked, [=]() {
        label->setText("按钮被点击了！");
    });

    // 方式3：连接到成员函数
    QObject::connect(button, &QPushButton::clicked,
                     this, &MyClass::onButtonClicked);
}
*/

// ============================================================================
// 第五部分：自定义信号和槽
// ============================================================================

/*
 * 如何定义自己的信号和槽？
 *
 * 步骤：
 * 1. 继承 QObject
 * 2. 添加 Q_OBJECT 宏
 * 3. 在 signals: 区域声明信号
 * 4. 在 public slots: 区域声明槽函数
 * 5. 使用 emit 发出信号
 */

// 自定义信号和槽示例（伪代码）
/*
class Student : public QObject
{
    Q_OBJECT

public:
    Student(const QString &name) : m_name(name), m_score(0) {}

    QString name() const { return m_name; }
    int score() const { return m_score; }

public slots:
    // 槽函数：设置分数
    void setScore(int score)
    {
        if (m_score != score) {
            m_score = score;
            emit scoreChanged(m_name, score);  // 发出信号

            // 根据分数发出不同的信号
            if (score >= 90) {
                emit excellent();  // 优秀
            } else if (score >= 60) {
                emit passed();     // 及格
            } else {
                emit failed();     // 不及格
            }
        }
    }

signals:
    // 信号：分数改变
    void scoreChanged(const QString &name, int newScore);

    // 信号：优秀
    void excellent();

    // 信号：及格
    void passed();

    // 信号：不及格
    void failed();

private:
    QString m_name;
    int m_score;
};

class Teacher : public QObject
{
    Q_OBJECT

public slots:
    // 槽函数：处理学生分数改变
    void onScoreChanged(const QString &name, int score)
    {
        cout << name << " 的分数是：" << score << endl;
    }

    // 槽函数：处理优秀
    void onExcellent()
    {
        cout << "恭喜！成绩优秀！" << endl;
    }

    // 槽函数：处理及格
    void onPassed()
    {
        cout << "及格了，继续努力！" << endl;
    }

    // 槽函数：处理不及格
    void onFailed()
    {
        cout << "不及格，需要补考！" << endl;
    }
};

// 使用示例
void customSignalSlotExample()
{
    Student student("张三");
    Teacher teacher;

    // 连接信号和槽
    QObject::connect(&student, &Student::scoreChanged,
                     &teacher, &Teacher::onScoreChanged);
    QObject::connect(&student, &Student::excellent,
                     &teacher, &Teacher::onExcellent);
    QObject::connect(&student, &Student::passed,
                     &teacher, &Teacher::onPassed);
    QObject::connect(&student, &Student::failed,
                     &teacher, &Teacher::onFailed);

    // 设置分数
    student.setScore(95);  // 触发 scoreChanged 和 excellent
    student.setScore(75);  // 触发 scoreChanged 和 passed
    student.setScore(50);  // 触发 scoreChanged 和 failed
}
*/

// ============================================================================
// 第六部分：Qt 元对象系统（Meta-Object System）
// ============================================================================

/*
 * 什么是元对象系统？
 *
 * Qt 的元对象系统提供了以下功能：
 * 1. 信号与槽机制
 * 2. 运行时类型信息（RTTI）
 * 3. 动态属性系统
 * 4. 对象反射
 *
 * 核心组件：
 * 1. Q_OBJECT 宏：启用元对象系统
 * 2. MOC（Meta-Object Compiler）：元对象编译器
 * 3. QMetaObject：元对象类
 */

// Q_OBJECT 宏的作用
/*
 * Q_OBJECT 宏会展开为：
 * 1. 元对象相关的代码
 * 2. 信号与槽的实现
 * 3. 运行时类型信息
 *
 * 注意：
 * - 必须放在类的私有区域（private:）
 * - 必须在类定义的开头
 * - 使用了 Q_OBJECT 的类必须有自己的 .cpp 文件
 */

// MOC（Meta-Object Compiler）
/*
 * MOC 是什么？
 *
 * MOC 是 Qt 的元对象编译器，它会：
 * 1. 扫描头文件，查找 Q_OBJECT 宏
 * 2. 生成元对象代码（moc_*.cpp 文件）
 * 3. 实现信号与槽机制
 *
 * 工作流程：
 * 1. 编写 .h 文件（包含 Q_OBJECT）
 * 2. MOC 处理 .h 文件，生成 moc_*.cpp
 * 3. 编译器编译 .cpp 和 moc_*.cpp
 * 4. 链接生成可执行文件
 *
 * 注意：
 * - Qt Creator 会自动运行 MOC
 * - 不需要手动运行 MOC
 */

// ============================================================================
// 第七部分：信号与槽的高级特性
// ============================================================================

/*
 * 1. 一个信号连接多个槽
 */
/*
void oneSignalMultipleSlots()
{
    QPushButton *button = new QPushButton("点击我");
    QLabel *label1 = new QLabel("标签1");
    QLabel *label2 = new QLabel("标签2");

    // 一个信号连接多个槽
    QObject::connect(button, &QPushButton::clicked, [=]() {
        label1->setText("按钮被点击了！");
    });

    QObject::connect(button, &QPushButton::clicked, [=]() {
        label2->setText("按钮被点击了！");
    });

    // 点击按钮时，两个标签都会更新
}
*/

/*
 * 2. 多个信号连接一个槽
 */
/*
void multipleSignalsOneSlot()
{
    QPushButton *button1 = new QPushButton("按钮1");
    QPushButton *button2 = new QPushButton("按钮2");
    QLabel *label = new QLabel("未点击");

    // 多个信号连接一个槽
    auto updateLabel = [=]() {
        label->setText("有按钮被点击了！");
    };

    QObject::connect(button1, &QPushButton::clicked, updateLabel);
    QObject::connect(button2, &QPushButton::clicked, updateLabel);

    // 点击任意按钮，标签都会更新
}
*/

/*
 * 3. 信号连接信号
 */
/*
void signalToSignal()
{
    QPushButton *button = new QPushButton("点击我");
    QLabel *label = new QLabel("未点击");

    // 自定义对象
    MyObject *obj = new MyObject();

    // 信号连接信号
    QObject::connect(button, &QPushButton::clicked,
                     obj, &MyObject::mySignal);

    // 信号连接槽
    QObject::connect(obj, &MyObject::mySignal, [=]() {
        label->setText("按钮被点击了！");
    });

    // 点击按钮 -> 触发 obj 的信号 -> 更新标签
}
*/

/*
 * 4. 断开连接
 */
/*
void disconnectExample()
{
    QPushButton *button = new QPushButton("点击我");
    QLabel *label = new QLabel("未点击");

    // 连接信号和槽
    QMetaObject::Connection conn = QObject::connect(
        button, &QPushButton::clicked, [=]() {
            label->setText("按钮被点击了！");
        });

    // 断开连接
    QObject::disconnect(conn);

    // 现在点击按钮不会更新标签
}
*/

// ============================================================================
// 第八部分：实战示例 - 简单的计数器
// ============================================================================

/*
class Counter : public QObject
{
    Q_OBJECT

public:
    Counter() : m_count(0) {}

    int count() const { return m_count; }

public slots:
    void increment()
    {
        m_count++;
        emit countChanged(m_count);
    }

    void decrement()
    {
        m_count--;
        emit countChanged(m_count);
    }

    void reset()
    {
        m_count = 0;
        emit countChanged(m_count);
    }

signals:
    void countChanged(int newCount);

private:
    int m_count;
};

class CounterWindow : public QWidget
{
    Q_OBJECT

public:
    CounterWindow()
    {
        // 创建控件
        m_label = new QLabel("计数：0");
        m_incrementButton = new QPushButton("+1");
        m_decrementButton = new QPushButton("-1");
        m_resetButton = new QPushButton("重置");

        // 创建布局
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(m_label);
        layout->addWidget(m_incrementButton);
        layout->addWidget(m_decrementButton);
        layout->addWidget(m_resetButton);

        // 创建计数器
        m_counter = new Counter(this);

        // 连接信号和槽
        connect(m_incrementButton, &QPushButton::clicked,
                m_counter, &Counter::increment);
        connect(m_decrementButton, &QPushButton::clicked,
                m_counter, &Counter::decrement);
        connect(m_resetButton, &QPushButton::clicked,
                m_counter, &Counter::reset);
        connect(m_counter, &Counter::countChanged,
                this, &CounterWindow::onCountChanged);
    }

private slots:
    void onCountChanged(int count)
    {
        m_label->setText(QString("计数：%1").arg(count));
    }

private:
    Counter *m_counter;
    QLabel *m_label;
    QPushButton *m_incrementButton;
    QPushButton *m_decrementButton;
    QPushButton *m_resetButton;
};
*/

// ============================================================================
// 主函数（概念讲解，不实际运行）
// ============================================================================

int main()
{
    cout << "=== Qt 核心概念详解 ===" << endl;
    cout << endl;

    cout << "1. QObject 基类" << endl;
    cout << "   - Qt 对象模型的核心" << endl;
    cout << "   - 提供信号与槽、对象树、属性系统等功能" << endl;
    cout << endl;

    cout << "2. 对象树和内存管理" << endl;
    cout << "   - 父对象销毁时，自动销毁所有子对象" << endl;
    cout << "   - 简化内存管理，避免内存泄漏" << endl;
    cout << endl;

    cout << "3. 信号与槽机制" << endl;
    cout << "   - 信号：对象发出的事件通知" << endl;
    cout << "   - 槽：响应信号的函数" << endl;
    cout << "   - 连接：使用 connect() 函数连接信号和槽" << endl;
    cout << endl;

    cout << "4. 信号与槽的优势" << endl;
    cout << "   - 松耦合：信号发送者不需要知道接收者" << endl;
    cout << "   - 类型安全：编译期检查" << endl;
    cout << "   - 灵活：一对多、多对一、信号连信号" << endl;
    cout << endl;

    cout << "5. Qt 元对象系统" << endl;
    cout << "   - Q_OBJECT 宏：启用元对象系统" << endl;
    cout << "   - MOC：元对象编译器" << endl;
    cout << "   - 提供运行时类型信息和反射" << endl;
    cout << endl;

    cout << "6. 关键要点" << endl;
    cout << "   - 继承 QObject 的类必须添加 Q_OBJECT 宏" << endl;
    cout << "   - 信号只声明，不实现" << endl;
    cout << "   - 使用 emit 发出信号" << endl;
    cout << "   - 槽函数可以是普通成员函数" << endl;
    cout << "   - 推荐使用 Qt 5 的连接语法" << endl;
    cout << endl;

    cout << "下一步：创建实际的 Qt 项目，实践信号与槽机制！" << endl;

    return 0;
}

/*
 * 学习总结：
 *
 * 1. QObject 是 Qt 对象模型的基础
 *    - 提供信号与槽、对象树、属性系统等核心功能
 *
 * 2. 对象树简化了内存管理
 *    - 父对象销毁时，自动销毁所有子对象
 *    - 避免内存泄漏
 *
 * 3. 信号与槽是 Qt 的核心机制
 *    - 信号：事件通知
 *    - 槽：事件处理
 *    - 连接：connect() 函数
 *
 * 4. 信号与槽的特点
 *    - 松耦合、类型安全、灵活
 *    - 一对多、多对一、信号连信号
 *
 * 5. Q_OBJECT 宏是必须的
 *    - 启用元对象系统
 *    - 支持信号与槽
 *
 * 6. MOC 自动处理元对象代码
 *    - Qt Creator 会自动运行
 *    - 不需要手动干预
 *
 * 下一步：
 * - 创建实际的 Qt 项目
 * - 实践信号与槽机制
 * - 学习 Qt 基础控件
 */
