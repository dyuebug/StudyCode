// ============================================
// 49_abstract_class.cpp - 超详细注释版
// 日期：第15天
// 难度：⭐⭐ 中级
// 主题：纯虚函数与抽象类
// ============================================

/*
【核心概念】

是什么？
- 纯虚函数：在虚函数声明末尾加 = 0，表示"只声明接口，不提供实现"
- 抽象类：含有至少一个纯虚函数的类
- 接口：抽象类在 C++ 中充当接口的角色

为什么需要？
- 有时父类没有合理的"默认实现"（Shape 怎么画？没有具体形状无法回答）
- 纯虚函数强制子类必须实现该功能，否则编译报错
- 建立规范：所有 Shape 子类都必须有 draw() 和 area()

核心特点：
- 含纯虚函数的类不能直接实例化（不能 new Shape()）
- 子类必须实现所有纯虚函数，否则子类也变成抽象类
- 纯虚函数可以有实现（罕见但合法），子类仍需 override
*/

#include <iostream>
#include <string>
#include <vector>
#include <memory>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【函数卡片：纯虚函数声明】

语法：
    virtual 返回类型 函数名(参数) [const] = 0;
    注意：= 0 不是等于零，是告诉编译器"此函数无默认实现，必须由子类实现"

示例：
    virtual void draw() const = 0;         // 纯虚函数（必须子类实现）
    virtual double area() const = 0;       // 纯虚函数

抽象类的规则：
    1. 含纯虚函数 → 抽象类 → 不能直接实例化
       AbstractBase obj;      // ❌ 编译错误
       AbstractBase* p = new AbstractBase(); // ❌ 编译错误
    2. 子类实现所有纯虚函数 → 可以实例化
    3. 子类没有实现所有纯虚函数 → 子类也是抽象类

【纯虚函数 vs 普通虚函数】

普通虚函数：
    virtual void speak() { cout << "动物叫"; }
    → 父类有默认实现，子类可以选择是否重写
    → 父类可以实例化

纯虚函数：
    virtual void speak() = 0;
    → 父类没有默认实现，子类必须实现
    → 父类不能实例化（抽象类）
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 基本抽象类和纯虚函数 ----

// Graphic 是抽象类：含纯虚函数，定义所有图形必须实现的接口
class Graphic {
public:
    // 纯虚函数：所有子类必须实现 draw()
    // 为什么是纯虚？因为 "一个图形" 如何画取决于具体形状，父类无法给出合理默认实现
    virtual void draw() const = 0;

    // 纯虚函数：所有子类必须实现 area()
    virtual double area() const = 0;

    // 多态基类析构函数仍然要是 virtual
    virtual ~Graphic() {}
};

// 下面这行会编译报错（注释掉展示）：
// Graphic g;  // ❌ 编译错误：Graphic 是抽象类，不能实例化

// Circle 实现了所有纯虚函数 → 可以实例化
class Circle : public Graphic {
private:
    double radius;   // 圆的半径

public:
    // 参数：r - 半径
    Circle(double r) : radius(r) {}

    // 实现父类纯虚函数 draw()
    // override 确保签名与父类一致
    void draw() const override {
        cout << "[Circle::draw] 画圆，半径=" << radius << endl;
    }

    // 实现父类纯虚函数 area()
    // 圆面积 = π × r²
    double area() const override {
        return 3.14159 * radius * radius;
    }
};

class RectGraphic : public Graphic {
private:
    double width, height;

public:
    RectGraphic(double w, double h) : width(w), height(h) {}

    void draw() const override {
        cout << "[RectGraphic::draw] 画矩形，" << width << "×" << height << endl;
    }

    double area() const override {
        return width * height;
    }
};

// 如果子类没有实现所有纯虚函数，它自己也变成抽象类：
// class IncompleteDerived : public Graphic {
//     void draw() const override { cout << "画了"; }
//     // 没有实现 area()
// };
// IncompleteDerived obj;  // ❌ 编译错误：IncompleteDerived 还是抽象类

void demoAbstractClass() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：抽象类基础" << endl;
    cout << string(60, '=') << endl;

    cout << "\nGraphic 是抽象类（含纯虚函数），不能实例化" << endl;
    cout << "Circle 和 RectGraphic 实现了所有纯虚函数，可以实例化" << endl;

    Circle      circle(3.0);
    RectGraphic rect(4.0, 5.0);

    circle.draw();
    cout << "圆面积：" << circle.area() << endl;

    rect.draw();
    cout << "矩形面积：" << rect.area() << endl;
}

// ---- 2.2 抽象类的核心价值：统一接口 ----

// 函数参数是父类引用，可以接受任何 Graphic 子类
// 调用方不需要知道具体类型，只关心"它是 Graphic"
void showGraphicInfo(const Graphic& g) {
    g.draw();                              // 动态绑定：调用实际子类的 draw()
    cout << "  面积：" << g.area() << endl; // 动态绑定
}

void demoUnifiedInterface() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：统一接口（多态的实际应用）" << endl;
    cout << string(60, '=') << endl;

    Circle      c1(2.0);
    Circle      c2(5.0);
    RectGraphic r1(3.0, 4.0);

    cout << "\n--- 统一函数处理不同图形 ---" << endl;
    showGraphicInfo(c1);
    showGraphicInfo(c2);
    showGraphicInfo(r1);

    cout << "\n--- 用父类指针数组管理多种图形 ---" << endl;
    // unique_ptr 自动管理内存（不需要手动 delete）
    vector<unique_ptr<Graphic>> graphics;
    graphics.push_back(make_unique<Circle>(3.0));
    graphics.push_back(make_unique<RectGraphic>(4.0, 5.0));
    graphics.push_back(make_unique<Circle>(1.5));

    double total = 0;
    for (const auto& g : graphics) {
        g->draw();
        total += g->area();
    }
    cout << "所有图形总面积：" << total << endl;
    cout << "（各自析构，内存自动释放）" << endl;
}

// ---- 2.3 普通 virtual 与纯 virtual 的使用场景对比 ----

// 有合理默认行为 → 用普通 virtual
class Logger {
public:
    // 有默认实现（打印到控制台）
    // 子类可以选择 override 来改变日志目标
    virtual void log(const string& msg) const {
        cout << "[Console] " << msg << endl;
    }
    virtual ~Logger() {}
};

class FileLogger : public Logger {
public:
    // override：改为输出到文件（这里模拟）
    void log(const string& msg) const override {
        cout << "[File] " << msg << endl;  // 实际应写到文件
    }
};

// 没有合理默认行为 → 用纯 virtual
class Serializable {
public:
    // "序列化"没有通用默认实现，不同类有不同格式
    // 强制每个子类自己实现
    virtual string serialize() const = 0;
    virtual void   deserialize(const string& data) = 0;
    virtual ~Serializable() {}
};

class Config : public Serializable {
private:
    string key, value;
public:
    Config(const string& k, const string& v) : key(k), value(v) {}

    string serialize() const override {
        return key + "=" + value;  // 简单的 key=value 格式
    }

    void deserialize(const string& data) override {
        size_t pos = data.find('=');
        if (pos != string::npos) {
            key   = data.substr(0, pos);
            value = data.substr(pos + 1);
        }
    }
};

void demoVirtualComparison() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：普通 virtual vs 纯 virtual 使用场景" << endl;
    cout << string(60, '=') << endl;

    cout << "\n--- 普通 virtual（有默认实现，子类可选 override）---" << endl;
    Logger     defaultLogger;
    FileLogger fileLogger;
    defaultLogger.log("系统启动");
    fileLogger.log("用户登录");

    cout << "\n--- 纯 virtual（无默认实现，子类必须 override）---" << endl;
    Config cfg("host", "localhost");
    cout << "序列化：" << cfg.serialize() << endl;
    cfg.deserialize("port=8080");
    cout << "反序列化后：" << cfg.serialize() << endl;
}

// ============================================
// 🔍 第三部分：深入理解（可选）
// ============================================

/*
【深入理解：纯虚函数可以有实现（罕见）】

即使是纯虚函数，也可以在类外提供实现：

class Shape {
public:
    virtual void draw() const = 0;  // 纯虚：子类必须 override
};

// 在类外提供实现（子类可以通过 Shape::draw() 显式调用）
void Shape::draw() const {
    cout << "Shape 通用绘制逻辑（子类可选择调用）";
}

class Circle : public Shape {
public:
    void draw() const override {
        Shape::draw();       // 显式调用父类实现（可选）
        cout << "Circle 特有逻辑";
    }
};

使用场景：
    为子类提供"可选的公共实现"，同时仍强制子类必须显式 override

【深入理解：接口设计原则（接口隔离）】

一个抽象类（接口）应该只包含一组相关的功能，
不应该把不相关的功能塞进同一个接口：

❌ 过大的接口：
    class IShape {
        virtual void draw() = 0;
        virtual double area() = 0;
        virtual void serialize() = 0;  // 序列化跟图形绘制不相关！
        virtual void sendOverNetwork() = 0;  // 网络传输更不相关！
    };

✅ 正确：拆分成多个小接口
    class IDrawable  { virtual void draw() = 0; };
    class IAreaable  { virtual double area() = 0; };
    class ISerializable { virtual string serialize() = 0; };

    class Circle : public IDrawable, public IAreaable {
        // Circle 只实现与它相关的接口
    };
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：试图直接实例化抽象类

   class IShape {
   public:
       virtual void draw() = 0;
   };
   IShape s;               // ❌ 编译错误：IShape 是抽象类
   IShape* p = new IShape();  // ❌ 编译错误

   ✅ 正确：只能通过子类实例化，用父类指针/引用引用它
   Circle c(5.0);
   IShape* p = &c;         // ✅ 父类指针指向子类对象

------------------------------------------------------------

❌ 错误2：子类没有实现所有纯虚函数（仍是抽象类）

   class IAnimal {
   public:
       virtual void eat() = 0;
       virtual void sleep() = 0;
   };
   class Dog : public IAnimal {
   public:
       void eat() override { cout << "Dog eats"; }
       // 忘记实现 sleep()！
   };
   Dog d;  // ❌ 编译错误：Dog 未实现 sleep()，仍是抽象类

   ✅ 正确：必须实现所有纯虚函数
   void sleep() override { cout << "Dog sleeps"; }

   记忆：继承抽象类时，检查所有 = 0 的函数是否都已实现

------------------------------------------------------------

❌ 错误3：混淆纯虚函数和普通虚函数的使用场景

   // 错误：用普通 virtual 表达"必须实现"的意图
   class Shape {
   public:
       virtual void draw() { /* 空实现 */ }  // ← 子类忘记实现也不报错！
   };

   // 正确：用纯虚函数明确要求子类必须实现
   class Shape {
   public:
       virtual void draw() = 0;  // ← 编译器强制检查
   };

------------------------------------------------------------

❌ 错误4：抽象类没有 virtual 析构函数

   class IAnimal {
   public:
       virtual void speak() = 0;
       ~IAnimal() {}   // ← 没有 virtual！
   };
   class Dog : public IAnimal {
       string* name;
   public:
       Dog() { name = new string("旺财"); }
       ~Dog() { delete name; }
       void speak() override { cout << "汪"; }
   };
   IAnimal* p = new Dog();
   delete p;  // 只调用 ~IAnimal()，~Dog() 没有被调用 → name 泄漏！

   ✅ 正确：抽象类（及所有多态基类）析构函数必须 virtual
   virtual ~IAnimal() {}

------------------------------------------------------------

❌ 错误5：误以为抽象类不能有构造函数/成员变量

   class Shape {
   protected:
       string color;  // ✅ 抽象类可以有成员变量
   public:
       Shape(const string& c) : color(c) {}  // ✅ 可以有构造函数
       virtual double area() = 0;            // 纯虚函数
   };
   // 子类可以在初始化列表中调用父类构造函数
   class Circle : public Shape {
   public:
       Circle(double r, const string& c) : Shape(c), radius(r) {}
       double area() override { return 3.14 * radius * radius; }
   private:
       double radius;
   };

------------------------------------------------------------

❌ 错误6：按值存储抽象类对象（切片 + 编译错误）

   void processShape(Shape shape) { ... }  // ❌ 参数按值，抽象类不能实例化

   // 即使不是抽象类，按值传递会切片，多态失效
   ✅ 正确：使用引用或指针
   void processShape(const Shape& shape) { ... }   // 引用
   void processShape(const Shape* shape) { ... }   // 指针
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
【纯虚函数语法】

声明：virtual 返回类型 函数名(参数) [const] = 0;
位置：父类（抽象类）的成员函数声明中

示例：
    virtual void draw() const = 0;        // 最常见形式
    virtual double area() const = 0;      // 带 const
    virtual string serialize() = 0;       // 不带 const

────────────────────────────────────────────────────

【抽象类规则速查】

含纯虚函数的类 = 抽象类：
    ✅ 可以有成员变量
    ✅ 可以有构造函数（供子类调用）
    ✅ 可以有普通虚函数
    ✅ 可以有非虚成员函数
    ❌ 不能直接实例化
    ❌ 不能作为函数参数值类型（可以用引用/指针）

────────────────────────────────────────────────────

【普通 virtual vs 纯 virtual 选择】

场景                               选择
有合理的通用默认实现              → 普通 virtual
没有合理的默认实现（必须子类定义）→ 纯 virtual = 0
需要强制子类实现某功能           → 纯 virtual = 0

────────────────────────────────────────────────────

【make_unique / make_shared 与多态】

// 用智能指针管理多态对象（推荐）
vector<unique_ptr<Shape>> shapes;
shapes.push_back(make_unique<Circle>(3.0));    // 不需要手动 delete
shapes.push_back(make_unique<Rectangle>(4,5));

for (const auto& s : shapes) {
    s->draw();   // 多态调用
}
// 离开作用域：自动析构所有对象
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
1. 判断题：以下代码哪行会编译错误？说明原因。

   class IFly {
   public:
       virtual void fly() = 0;
       virtual ~IFly() {}
   };
   class Bird : public IFly {
   public:
       void fly() override { cout << "鸟飞"; }
   };
   class Penguin : public IFly {
       // 没有实现 fly()
   };

   IFly  f;           // ← ①
   Bird  b;           // ← ②
   Penguin p;         // ← ③
   IFly* pf = &b;     // ← ④
   pf->fly();         // ← ⑤

------------------------------------------------------------

2. 实现练习：设计一个支付系统接口

   抽象类 IPayment（接口）：
   - virtual bool process(double amount) = 0  （处理付款，返回是否成功）
   - virtual string name() const = 0          （支付方式名称）

   子类：
   - CreditCard(string cardNo)：打印 "信用卡 xxxx 付款 ¥amount"
   - Alipay(string account)：打印 "支付宝 account 付款 ¥amount"
   - Cash：打印 "现金付款 ¥amount"

   函数：
   void checkout(IPayment& payment, double amount)  // 统一结算接口

   验证多态调用是否正确。

------------------------------------------------------------

3. 设计题：
   一个游戏里有多种敌人（Enemy），它们都需要：
   - 移动（move）：不同敌人移动方式不同
   - 攻击（attack）：不同敌人攻击方式不同
   - HP 属性和受伤（takeDamage）：所有敌人通用

   请设计这个类体系：
   - 哪些方法应该是纯虚函数？
   - 哪些应该是普通虚函数？
   - HP 和 takeDamage 应该放在哪里？

------------------------------------------------------------

4. 思考题：
   C++ 用抽象类模拟接口，Java 有专门的 interface 关键字。
   两者有什么本质区别？
   C++ 的抽象类比 Java 接口多了什么？少了什么？
   提示：考虑成员变量、多重继承、访问权限
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "     49_abstract_class.cpp - 纯虚函数与抽象类" << endl;
    cout << string(60, '=') << endl;

    demoAbstractClass();
    demoUnifiedInterface();
    demoVirtualComparison();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 纯虚函数语法：virtual 返回类型 函数名() = 0;" << endl;
    cout << "2. 含纯虚函数 → 抽象类 → 不能直接实例化" << endl;
    cout << "3. 子类必须实现所有纯虚函数，否则子类也是抽象类" << endl;
    cout << "4. 抽象类的价值：定义统一接口，强制子类实现" << endl;
    cout << "5. 抽象类析构函数必须是 virtual" << endl;

    return 0;
}
