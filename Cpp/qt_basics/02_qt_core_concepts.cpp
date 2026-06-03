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

// ============================================================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================================================

/*
【错误1】忘记添加 Q_OBJECT 宏

❌ 错误代码：
class MyObject : public QObject
{
    // 忘记写 Q_OBJECT！
public slots:
    void onClicked() {}
signals:
    void mySignal();
};

结果：信号槽连接编译通过，但运行时完全不工作，connect() 静默失败。

原因：Q_OBJECT 宏由 MOC 处理，没有它就没有元对象代码，信号槽机制无法运行。

✅ 正确代码：
class MyObject : public QObject
{
    Q_OBJECT  // 必须在类定义的第一行！
public slots:
    void onClicked() {}
signals:
    void mySignal();
};

预防措施：继承 QObject 的类，写完类名立即加 Q_OBJECT。

────────────────────────────────────────────────────────────

【错误2】在槽函数中实现信号

❌ 错误代码：
signals:
    void valueChanged(int v)  // 错误：给信号写了函数体
    {
        cout << v << endl;
    }

结果：编译错误，MOC 生成的代码与用户实现冲突。

原因：信号由 MOC 自动实现，用户只需声明，不能提供函数体。

✅ 正确代码：
signals:
    void valueChanged(int v);  // 只声明，不实现，无函数体

预防措施：signals: 区域只写声明，以分号结尾。

────────────────────────────────────────────────────────────

【错误3】手动 delete 已有父对象的子控件

❌ 错误代码：
QPushButton *btn = new QPushButton("OK", parentWidget);
// ... 某处
delete btn;  // 危险！parentWidget 析构时会再次 delete btn，导致双重释放

结果：程序崩溃（double free / heap corruption）。

原因：Qt 对象树中，父对象析构时会自动 delete 所有子对象。

✅ 正确代码：
QPushButton *btn = new QPushButton("OK", parentWidget);
// 不需要手动 delete，parentWidget 销毁时 btn 自动销毁
// 如果确实需要提前销毁，先断开父子关系：
btn->setParent(nullptr);
delete btn;

预防措施：有父对象的控件，不要手动 delete。

────────────────────────────────────────────────────────────

【错误4】connect() 返回值未检查导致连接失败无感知

❌ 错误代码：
QObject::connect(sender, &Sender::signal, receiver, &Receiver::slot);
// 不检查返回值，连接失败了也不知道

结果：信号发出后槽不执行，难以排查。

原因：connect() 连接失败时返回无效的 QMetaObject::Connection，但不抛异常。

✅ 正确代码（调试阶段）：
bool ok = QObject::connect(sender, &Sender::signal, receiver, &Receiver::slot);
Q_ASSERT(ok);  // 调试模式下，连接失败立即断言

// 或者用 Qt 5 新语法，编译期就能检查类型匹配：
QObject::connect(sender, &Sender::signal, receiver, &Receiver::slot);
// 新语法在信号/槽签名不匹配时，编译直接报错

预防措施：优先使用 Qt 5 函数指针语法，而非 Qt 4 的字符串宏语法。

────────────────────────────────────────────────────────────

【错误5】在没有 QApplication 的情况下使用 Qt 控件

❌ 错误代码：
int main()
{
    QPushButton btn("Hello");  // 错误：QApplication 还没创建
    btn.show();
    return 0;
}

结果：程序崩溃或行为未定义。

原因：Qt 的事件循环和控件系统依赖 QApplication 初始化。

✅ 正确代码：
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);  // 必须第一个创建
    QPushButton btn("Hello");
    btn.show();
    return app.exec();  // 启动事件循环
}

预防措施：main 函数第一行永远是创建 QApplication（或 QCoreApplication）。

────────────────────────────────────────────────────────────

【错误6】Lambda 捕获悬空指针

❌ 错误代码：
void setupConnection()
{
    QLabel *label = new QLabel("test", parent);
    QPushButton *btn = new QPushButton("click", parent);

    connect(btn, &QPushButton::clicked, [label]() {
        label->setText("clicked!");  // 如果 label 已被销毁，崩溃！
    });

    delete label;  // label 被提前销毁
    // 之后点击 btn，Lambda 访问已释放的 label
}

结果：访问已释放内存，程序崩溃。

✅ 正确代码：
// 方式1：使用第四个参数指定 context 对象，对象销毁时自动断开连接
connect(btn, &QPushButton::clicked, label, [label]() {
    label->setText("clicked!");
});
// label 销毁后，这个连接自动断开，Lambda 不会再被调用

预防措施：Lambda 槽函数总是提供 context 参数（第三个参数），与捕获的对象生命周期绑定。

────────────────────────────────────────────────────────────

【错误7】在子线程中直接操作 UI 控件

❌ 错误代码：
// 在工作线程中
void WorkerThread::run()
{
    // 计算完成后直接更新 UI
    m_label->setText("完成！");  // 崩溃或行为未定义！
}

结果：程序崩溃，或 UI 显示异常。

原因：Qt 的 UI 控件只能在主线程（GUI 线程）中操作。

✅ 正确代码：
// 工作线程发出信号
signals:
    void resultReady(const QString &text);

void WorkerThread::run()
{
    // 通过信号通知主线程更新 UI
    emit resultReady("完成！");
}

// 主线程的槽函数更新 UI
void MainWindow::onResultReady(const QString &text)
{
    m_label->setText(text);  // 在主线程中安全操作 UI
}

预防措施：子线程永远不直接操作 UI，只通过信号槽通知主线程。
*/

// ============================================================================
// 第五部分：函数卡片速查
// ============================================================================

/*
【函数卡片1：QObject::connect()】

语法：
QMetaObject::Connection QObject::connect(
    const QObject *sender,          // 信号发送者
    PointerToMemberFunction signal, // 信号（函数指针）
    const QObject *receiver,        // 信号接收者
    PointerToMemberFunction slot,   // 槽函数（函数指针）
    Qt::ConnectionType type = Qt::AutoConnection  // 连接类型（可选）
)

参数：
  - sender   (QObject*)：发送信号的对象
  - signal   (函数指针)：信号函数，格式 &ClassName::signalName
  - receiver (QObject*)：接收信号的对象（Lambda 版本可省略）
  - slot     (函数指针)：槽函数，格式 &ClassName::slotName 或 Lambda
  - type     (Qt::ConnectionType)：连接类型，默认 AutoConnection

返回值：QMetaObject::Connection，可用于 disconnect()；连接失败时返回无效对象

常用连接类型：
  - Qt::AutoConnection    默认，跨线程自动用队列连接
  - Qt::DirectConnection  直接调用，同步
  - Qt::QueuedConnection  队列连接，用于跨线程

示例：
// 普通槽函数
connect(btn, &QPushButton::clicked, this, &MainWindow::onClicked);

// Lambda 槽（带 context，防悬空）
connect(btn, &QPushButton::clicked, label, [label]() {
    label->setText("clicked");
});

────────────────────────────────────────────────────────────

【函数卡片2：QObject::disconnect()】

语法：
bool QObject::disconnect(const QMetaObject::Connection &connection)
bool QObject::disconnect(const QObject *sender, signal, const QObject *receiver, slot)

参数：
  - connection：connect() 返回的连接对象（推荐方式）

返回值：bool，断开成功返回 true

示例：
auto conn = connect(btn, &QPushButton::clicked, this, &MyClass::onClicked);
// 之后断开
disconnect(conn);

────────────────────────────────────────────────────────────

【函数卡片3：emit 关键字】

语法：
emit signalName(参数...);

作用：发出一个信号，触发所有已连接的槽函数

注意：emit 只是一个空宏（#define emit），主要起文档标注作用，不加也能编译

示例：
void Counter::increment()
{
    m_count++;
    emit countChanged(m_count);  // 发出信号，传递新值
}

────────────────────────────────────────────────────────────

【函数卡片4：Q_OBJECT 宏】

用途：在继承 QObject 的类中启用元对象系统
位置：必须放在类定义中第一个非注释行（私有区域）

class MyClass : public QObject
{
    Q_OBJECT  // ← 这里

public:
    explicit MyClass(QObject *parent = nullptr);
};

注意：
  - 使用 Q_OBJECT 的类必须有对应的 .cpp 文件
  - 修改后需要重新运行 qmake（Qt Creator 通常自动处理）
*/

// ============================================================================
// 第六部分：练习题
// ============================================================================

/*
1. 基础理解
   创建一个 Temperature 类（继承 QObject），包含：
   - 私有成员：double m_celsius（摄氏度）
   - 槽函数：setCelsius(double)，设置温度并发出信号
   - 信号：temperatureChanged(double celsius, double fahrenheit)
   要求：在 setCelsius 中，将摄氏度转换为华氏度（F = C * 9/5 + 32），一起通过信号发出
   提示：华氏度转换公式，emit 发出两个参数的信号

2. 连接练习
   在 Qt 项目中实现以下连接（只写 connect 代码）：
   - 点击 QPushButton，清空 QLineEdit 的文本
   - QLineEdit 文本变化时（textChanged 信号），更新 QLabel 显示当前字数
   - 两个 QSpinBox，让它们的值始终保持同步（A 变 B 也变，B 变 A 也变）
   提示：注意双向同步可能引起无限循环，思考如何避免

3. 对象树实践
   分析以下代码，指出哪些 delete 是必要的，哪些会导致问题：
   QWidget *win = new QWidget();
   QPushButton *btn1 = new QPushButton("A", win);
   QPushButton *btn2 = new QPushButton("B");
   btn2->setParent(win);
   QLabel *lbl = new QLabel("hello");

   a. delete win;    // 效果是什么？
   b. delete btn1;   // 安全吗？
   c. delete lbl;    // 需要吗？

4. Lambda 陷阱分析
   下面代码有什么潜在问题？如何修复？
   void MyClass::setup()
   {
       QPushButton *btn = new QPushButton("Go", this);
       QString *data = new QString("important");
       connect(btn, &QPushButton::clicked, [data]() {
           qDebug() << *data;
       });
   }

5. 综合设计
   设计一个简单的"温度转换器"信号槽系统（不需要实际运行，画出或写出类图）：
   - CelsiusInput 类：用户输入摄氏度，发出 celsiusChanged(double) 信号
   - FahrenheitDisplay 类：槽函数接收并显示对应华氏度
   - KelvinDisplay 类：槽函数接收并显示对应开尔文温度（K = C + 273.15）
   要求：画出三个类之间的信号槽连接关系
*/
