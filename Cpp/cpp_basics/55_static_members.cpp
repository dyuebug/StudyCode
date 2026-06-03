// ============================================
// 55_static_members.cpp - 超详细注释版
// 日期：第17天
// 难度：⭐⭐ 中级
// 主题：静态成员变量与静态成员函数
// ============================================

/*
【核心概念】

是什么？
- static 成员变量：属于类本身，所有对象共享同一份数据（不是每个对象各一份）
- static 成员函数：属于类本身，不依赖任何对象实例，没有 this 指针

为什么需要？
- 静态成员变量：跨对象共享数据（如：已创建的对象总数、全局配置、单例实例）
- 静态成员函数：工具函数/工厂函数，调用时不需要先创建对象

核心特点：
- 静态成员变量：类内声明，类外定义并初始化（C++17 可用 inline 省略类外定义）
- 静态成员函数：不能访问非静态成员（因为没有 this 指针）
- 调用方式：ClassName::staticMember 或 obj.staticMember（推荐前者，更清晰）
*/

#include <iostream>
#include <string>
#include <vector>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：static 成员变量】

声明（类内）：
    static 类型 变量名;            // 只声明，不分配内存

定义（类外，必须在类外初始化，否则链接错误）：
    类型 ClassName::变量名 = 初始值;

C++17 内联写法（类内直接初始化，不需要类外定义）：
    inline static 类型 变量名 = 初始值;

使用：
    ClassName::变量名          // 推荐
    对象.变量名                // 也可以，但不推荐（看起来像非静态）

────────────────────────────────────────────────────

【函数卡片：static 成员函数】

声明与定义：
    // 类内声明
    static 返回类型 函数名(参数);

    // 类外定义（不加 static 关键字！）
    返回类型 ClassName::函数名(参数) { ... }

限制：
    - 没有 this 指针
    - 只能访问 static 成员（不能访问非 static 的成员变量/函数）
    - 不能声明为 const（const 修饰的是 this，静态函数无 this）

调用：
    ClassName::函数名(参数)    // 推荐
    对象.函数名(参数)           // 也合法，但不推荐
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 静态成员变量：对象计数器 ----

class Robot {
private:
    string name;  // 每个 Robot 独有的名字（非静态）
    int    id;    // 每个 Robot 独有的 ID（非静态）

    // 静态成员变量：所有 Robot 对象共享
    // 记录已创建的 Robot 总数
    static int totalCount;   // 类内声明（不分配内存）
    static int nextId;       // 下一个可用 ID

public:
    Robot(const string& name) : name(name) {
        ++totalCount;        // 每创建一个对象，总数 +1
        id = nextId++;       // 分配唯一 ID，然后递增
        cout << "  [创建] " << this->name << " (ID=" << id << ")" << endl;
    }

    ~Robot() {
        --totalCount;        // 对象销毁时，总数 -1
        cout << "  [销毁] " << name << " (ID=" << id << ")" << endl;
    }

    // 静态成员函数：获取当前存活的对象数量
    // 不需要 this（不依赖具体对象），只访问静态成员 totalCount
    static int getCount() { return totalCount; }

    // 静态成员函数：重置计数器（工具函数）
    static void resetCounter() {
        // totalCount = 0;  // 不要这样做！已有对象的计数会乱
        cout << "  [警告] 计数器重置（仅演示用途）" << endl;
        nextId = 0;
    }

    void print() const {
        cout << "Robot: " << name << " (ID=" << id << ")" << endl;
    }
};

// 类外定义并初始化静态成员变量
// 格式：类型 ClassName::变量名 = 初始值;
int Robot::totalCount = 0;  // 初始没有 Robot 对象
int Robot::nextId = 1;      // ID 从 1 开始

void demoStaticVariable() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：静态成员变量（Robot 计数器）" << endl;
    cout << string(60, '=') << endl;

    // 通过类名访问静态成员（推荐方式）
    cout << "\n初始 Robot 数量：" << Robot::getCount() << endl;

    cout << "\n--- 创建 3 个 Robot ---" << endl;
    {
        Robot r1("Alpha");
        Robot r2("Beta");
        cout << "当前 Robot 数量：" << Robot::getCount() << endl;

        {
            Robot r3("Gamma");
            cout << "当前 Robot 数量：" << Robot::getCount() << endl;
        }  // r3 离开作用域，析构函数调用，totalCount--

        cout << "r3 销毁后：" << Robot::getCount() << endl;
    }  // r1, r2 离开作用域

    cout << "全部销毁后：" << Robot::getCount() << endl;
}

// ---- 2.2 静态成员函数：工厂函数模式 ----

class Color {
private:
    int r, g, b;  // RGB 分量，各 0-255

    // 私有构造函数：强制通过工厂函数创建
    Color(int r, int g, int b) : r(r), g(g), b(b) {}

public:
    // 静态工厂函数：创建预定义颜色
    // 好处：语义清晰（Color::red() 比 Color(255,0,0) 更可读）
    static Color red()   { return Color(255, 0, 0); }
    static Color green() { return Color(0, 255, 0); }
    static Color blue()  { return Color(0, 0, 255); }
    static Color white() { return Color(255, 255, 255); }
    static Color black() { return Color(0, 0, 0); }

    // 静态工厂函数：从十六进制字符串创建（简化版，只支持 "RRGGBB" 格式）
    static Color fromHex(int hex) {
        int r = (hex >> 16) & 0xFF;  // 高8位
        int g = (hex >>  8) & 0xFF;  // 中8位
        int b =  hex        & 0xFF;  // 低8位
        return Color(r, g, b);
    }

    // 混合两种颜色（取平均值）
    static Color mix(const Color& a, const Color& b) {
        return Color((a.r + b.r) / 2,
                     (a.g + b.g) / 2,
                     (a.b + b.b) / 2);
    }

    void print() const {
        cout << "Color(" << r << ", " << g << ", " << b << ")" << endl;
    }
};

void demoStaticFactory() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：静态工厂函数（Color 类）" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 预定义颜色 ---" << endl;
    Color red   = Color::red();     // 静态函数：不需要对象，直接通过类名调用
    Color green = Color::green();
    Color blue  = Color::blue();

    cout << "红色："; red.print();
    cout << "绿色："; green.print();
    cout << "蓝色："; blue.print();

    cout << "\n--- 从十六进制创建 ---" << endl;
    Color orange = Color::fromHex(0xFF8000);  // 橙色
    cout << "橙色："; orange.print();

    cout << "\n--- 混合颜色 ---" << endl;
    Color mixed = Color::mix(red, blue);  // 红+蓝=紫
    cout << "红+蓝=："; mixed.print();
}

// ---- 2.3 静态成员变量：单例模式（Singleton） ----

class Logger {
private:
    string logFile;
    int    logCount;

    // 私有构造函数：防止外部直接创建
    Logger() : logFile("app.log"), logCount(0) {
        cout << "  [Logger] 日志系统初始化，文件：" << logFile << endl;
    }

    // 静态实例指针：指向唯一的 Logger 对象
    static Logger* instance;

public:
    // 静态获取实例函数：确保全局只有一个 Logger
    static Logger* getInstance() {
        if (instance == nullptr) {
            instance = new Logger();  // 第一次调用时创建
        }
        return instance;  // 之后返回已有实例
    }

    // 记录日志
    void log(const string& msg) {
        ++logCount;
        cout << "[LOG #" << logCount << "] " << msg << endl;
    }

    int getLogCount() const { return logCount; }

    // 清理（实际项目中应在程序结束时调用）
    static void destroy() {
        delete instance;
        instance = nullptr;
    }
};

// 类外初始化静态指针（初始为 nullptr）
Logger* Logger::instance = nullptr;

void demoSingleton() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：静态成员实现单例模式（Logger）" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 获取 Logger 实例（第一次：触发创建）---" << endl;
    Logger* log1 = Logger::getInstance();
    log1->log("程序启动");
    log1->log("用户登录");

    cout << "\n--- 获取 Logger 实例（第二次：返回同一对象）---" << endl;
    Logger* log2 = Logger::getInstance();
    log2->log("数据处理完成");

    // 验证 log1 和 log2 指向同一个对象
    cout << "\nlog1 == log2: " << (log1 == log2 ? "true（同一对象）" : "false") << endl;
    cout << "总日志条数：" << log1->getLogCount() << endl;

    Logger::destroy();
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
🔍 深入理解：static 成员变量的内存布局

非静态成员变量：
    每个对象在堆/栈上各有一份
    sizeof(Robot) 包含非静态成员的大小

    Robot r1, r2, r3;
    r1.name → "Alpha"（r1 自己的）
    r2.name → "Beta" （r2 自己的，独立地址）

静态成员变量：
    存储在程序的数据段（类似全局变量）
    所有对象共享同一地址
    sizeof(Robot) 不包含静态成员的大小

    Robot::totalCount 只有一个地址
    r1 修改它，r2 读取到的也变了

🔍 深入理解：为什么静态成员函数不能访问非静态成员？

    非静态成员函数有隐含的 this 参数：
        void print()  →  void print(Robot* this)
        this->name 才能找到具体哪个对象的 name

    静态成员函数没有 this：
        static int getCount()  →  没有 Robot* this 参数
        不知道是哪个对象的 name，无法访问非静态成员

    结论：
        static 函数 → 只能访问 static 成员（有固定地址）
        非static 函数 → 可以访问所有成员（通过 this 找到对象）
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：静态成员变量只声明，忘记类外定义（链接错误）

   class Foo {
       static int count;    // ← 只声明
   };
   // 忘记写：int Foo::count = 0;
   // 编译通过，但链接时报 "undefined reference to Foo::count"

   ✅ 正确：必须在类外（且只在一个 .cpp 文件中）定义
   int Foo::count = 0;   // 类型 类名::变量名 = 初始值

   C++17 例外：inline static 可以在类内初始化
   class Foo {
       inline static int count = 0;  // C++17：无需类外定义
   };

------------------------------------------------------------

❌ 错误2：静态成员函数访问非静态成员

   class Counter {
       int value;            // 非静态
       static int total;     // 静态
   public:
       static void reset() {
           value = 0;        // ❌ 编译错误！静态函数无 this，不知道哪个对象的 value
           total = 0;        // ✅ 正确，total 是静态的，有固定地址
       }
   };

   ✅ 修复：非静态操作改为非静态函数
   void resetValue() { value = 0; }   // 非静态函数可以访问 value

------------------------------------------------------------

❌ 错误3：静态成员函数加 const 修饰

   static int getCount() const { ... }   // ❌ 编译错误！
   // const 的含义是"不修改 this 指向的对象"
   // 静态函数没有 this，const 毫无意义，编译器报错

   ✅ 静态成员函数不加 const：
   static int getCount() { return count; }

------------------------------------------------------------

❌ 错误4：混淆 static 局部变量与 static 成员变量

   class Foo {
   public:
       void func() {
           static int callCount = 0;  // ← 这是局部 static 变量！
           ++callCount;               // 跨函数调用保持值
       }
       // callCount 不是类的成员，无法通过 Foo::callCount 访问
   };

   Foo::callCount;   // ❌ 编译错误！callCount 是函数内局部变量

   ✅ 区分：
   - 类内 static 成员：属于类，所有对象共享
   - 函数内 static 局部：属于函数，跨调用保持值（不是类成员）

------------------------------------------------------------

❌ 错误5：在构造函数初始化列表中初始化静态成员

   class Foo {
       static int count;
   public:
       Foo() : count(0) {}  // ❌ 编译错误！静态成员不能在初始化列表中初始化
   };

   ✅ 正确：静态成员在类外初始化（或 C++17 inline 在类内）
   int Foo::count = 0;

------------------------------------------------------------

❌ 错误6：通过对象访问静态成员（语义误导）

   Robot r1("Alpha");
   r1.getCount();          // 合法，但容易误解为"r1 的 count"
   Robot::getCount();      // ✅ 推荐：明确表示访问类的静态成员

   ✅ 规范：始终用 ClassName:: 访问静态成员，不用对象.成员
)";
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void showFunctionCards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【静态成员变量】

类内声明：    static 类型 名字;
类外定义：    类型 ClassName::名字 = 初始值;
C++17内联：  inline static 类型 名字 = 初始值;   // 无需类外定义
访问：        ClassName::名字  （推荐）
              对象.名字        （合法但不推荐）

────────────────────────────────────────────────────

【静态成员函数】

声明：    static 返回类型 函数名(参数);
类外定义：返回类型 ClassName::函数名(参数) { ... }  // 不加 static
调用：    ClassName::函数名(参数)   （推荐）
          对象.函数名(参数)         （合法但不推荐）

限制：
  - 无 this 指针
  - 不能访问非 static 成员
  - 不能加 const 修饰

────────────────────────────────────────────────────

【常见 static 使用场景】

计数器：   static int count = 0;   // 跟踪创建的对象数量
单例：     static Type* instance;  // 指向唯一实例
工厂函数： static Type create(...)  // 不需要对象即可创建
配置常量： static const int MAX = 100;
)";
}

// ============================================
// 第六部分：练习题
// ============================================

void showExercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第六部分：练习题（独立思考）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现练习：为 Student 类添加静态功能

   class Student {
       string name;
       int    score;
       static int totalStudents;
       static double totalScore;
   public:
       Student(string name, int score);
       ~Student();

       // TODO：实现以下静态函数：
       // static int    getCount()    — 当前学生数
       // static double getAverage()  — 当前平均分
       // static void   printStats()  — 打印统计信息

       // 验证：
       // Student s1("Alice", 90), s2("Bob", 80), s3("Charlie", 70);
       // Student::printStats();  // 3人，平均分80
       // { Student s4("Dave", 100); Student::printStats(); }  // 4人，均分85
       // Student::printStats();  // s4 析构后，回到3人，均分80
   };

------------------------------------------------------------

2. 实现链式工厂：为 QueryBuilder 类实现静态工厂 + 链式调用

   class QueryBuilder {
       string table;
       string condition;
       int    limit;
   public:
       // 工厂函数：QueryBuilder q = QueryBuilder::from("users");
       static QueryBuilder from(const string& table);

       // 链式方法（返回 *this 引用）
       QueryBuilder& where(const string& cond);
       QueryBuilder& limitTo(int n);

       string build() const;   // 生成 SQL 字符串
   };

   // 期望用法：
   string sql = QueryBuilder::from("users")
                    .where("age > 18")
                    .limitTo(10)
                    .build();
   // 输出：SELECT * FROM users WHERE age > 18 LIMIT 10

------------------------------------------------------------

3. 思考题：以下哪些适合用 static？

   a) 记录程序启动时间（所有对象共享）
   b) 每个 Employee 对象的工资（每人不同）
   c) 公司名称（所有员工属于同一公司）
   d) 员工编号生成器（全局递增）
   e) 每个订单的创建时间（每单不同）

   判断原则：这个数据是"属于类"还是"属于某个对象实例"？
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << " 55_static_members.cpp - 静态成员变量与静态成员函数" << endl;
    cout << string(60, '=') << endl;

    demoStaticVariable();
    demoStaticFactory();
    demoSingleton();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. static 变量：属于类，所有对象共享，类外初始化" << endl;
    cout << "2. static 函数：无 this，只能访问 static 成员" << endl;
    cout << "3. 调用：ClassName::名字（推荐，不用对象.名字）" << endl;
    cout << "4. 常见用途：计数器、单例、工厂函数、全局配置" << endl;
    cout << "5. 静态函数不加 const（const 依赖 this）" << endl;

    return 0;
}
