// ============================================
// 45_inheritance_basics.cpp - 超详细注释版
// 日期：第14天
// 难度：⭐⭐ 中级
// 主题：继承基础
// ============================================

/*
【核心概念】

是什么？
- 继承（Inheritance）：子类自动获得父类的成员变量和成员函数
- 语法：class Dog : public Animal { ... };
- 体现 is-a 关系（Dog is-a Animal）

为什么需要？
- 避免重复代码：共同属性/方法写在父类，子类只写自己特有的
- 建立类型层次：Dog 和 Cat 都是 Animal，可以统一处理
- 支持多态（配合 virtual）

核心特点：
- public 继承是最常用的，保持原有访问权限
- 子类可以重写（override）父类的方法
- protected 成员：子类内部可访问，类外不可访问
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【继承的访问权限规则】

父类成员权限    public继承      protected继承   private继承
public       → public         protected       private
protected    → protected      protected       private
private      → 不可访问        不可访问         不可访问

【is-a 关系】
Dog : public Animal  → Dog is-a Animal（正确，狗是动物）
Engine : private Car → Engine has-a Car（私有继承，不是 is-a）
常用的只有 public 继承，其他两种极少使用。

【函数卡片：父类构造函数调用】

语法（初始化列表中调用）：
    子类构造函数(参数) : 父类名(父类参数), 子类成员初始化 { }

示例：
    Dog(string name, int age, string breed)
        : Animal(name, age),    ← 调用父类构造函数，初始化继承的成员
          breed(breed) { }      ← 初始化子类自己的成员

注意：
    - 若不显式调用，编译器自动调用父类默认构造函数（无参）
    - 父类没有默认构造函数时，子类必须显式调用
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 基本继承语法 ----

// 基类（父类）：Animal
class Animal {
public:
    string name;   // 所有动物都有名字（公有成员，子类直接访问）
    int    age;    // 所有动物都有年龄

    // 父类构造函数：初始化公共属性
    // 参数：n - 动物名称，a - 年龄
    Animal(const string& n, int a) : name(n), age(a) {
        cout << "Animal 构造：" << name << endl;
    }

    // 所有动物都会吃东西（公共行为）
    void eat() const {
        cout << name << " 正在吃东西" << endl;
    }

    // 所有动物都会呼吸（公共行为）
    void breathe() const {
        cout << name << " 正在呼吸" << endl;
    }

    // 打印基本信息
    void info() const {
        cout << "[Animal] 名称：" << name << "，年龄：" << age << endl;
    }
};

// 派生类（子类）：Dog，public 继承 Animal
// 含义：Dog is-a Animal（狗是动物）
// 效果：Dog 自动拥有 Animal 的 name、age、eat()、breathe()、info()
class Dog : public Animal {
public:
    string breed;   // Dog 自己特有的属性：品种

    // 子类构造函数：必须调用父类构造函数初始化继承的成员
    // 参数：n - 名字，a - 年龄，b - 品种
    Dog(const string& n, int a, const string& b)
        : Animal(n, a),  // ← 必须！调用父类构造函数，初始化 name 和 age
          breed(b)        // 初始化 Dog 自己的 breed
    {
        cout << "Dog 构造：" << name << "（" << breed << "）" << endl;
    }

    // Dog 自己特有的方法：叫声
    void bark() const {
        cout << name << " 汪汪叫！" << endl;
    }

    // 重写父类的 info()（隐藏父类版本，添加 breed 信息）
    // 注意：这里没有用 virtual，是"隐藏"而不是"重写"
    void info() const {
        cout << "[Dog] 名称：" << name << "，年龄：" << age
             << "，品种：" << breed << endl;
    }
};

// 派生类：Cat，public 继承 Animal
class Cat : public Animal {
public:
    bool isIndoor;   // Cat 特有属性：是否室内猫

    // 参数：n - 名字，a - 年龄，indoor - 是否室内猫
    Cat(const string& n, int a, bool indoor)
        : Animal(n, a), isIndoor(indoor)
    {
        cout << "Cat 构造：" << name << endl;
    }

    // Cat 特有方法
    void meow() const {
        cout << name << " 喵喵叫！" << endl;
    }

    void info() const {
        cout << "[Cat] 名称：" << name << "，年龄：" << age
             << "，" << (isIndoor ? "室内猫" : "室外猫") << endl;
    }
};

void demoBasicInheritance() {
    cout << string(60, '=') << endl;
    cout << "第二部分 2.1：基本继承语法" << endl;
    cout << string(60, '=') << endl;

    // 构造顺序：先构造父类（Animal），再构造子类（Dog）
    cout << "\n--- 创建对象（注意构造顺序）---" << endl;
    Dog dog("旺财", 3, "金毛");
    Cat cat("咪咪", 2, true);

    cout << "\n--- 调用继承自父类的方法 ---" << endl;
    dog.eat();      // 继承自 Animal，不需要 Dog 重新定义
    dog.breathe();  // 继承自 Animal

    cout << "\n--- 调用子类特有方法 ---" << endl;
    dog.bark();     // Dog 自己的方法
    cat.meow();     // Cat 自己的方法

    cout << "\n--- 调用重写的 info() 方法 ---" << endl;
    dog.info();     // 调用 Dog 的 info()（隐藏了 Animal 的）
    cat.info();     // 调用 Cat 的 info()

    cout << "\n--- 直接访问继承的 public 成员 ---" << endl;
    cout << "dog.name = " << dog.name << endl;  // 继承自 Animal 的 name
    cout << "dog.age  = " << dog.age  << endl;  // 继承自 Animal 的 age
    cout << "dog.breed= " << dog.breed<< endl;  // Dog 自己的 breed
}

// ---- 2.2 访问权限：public / protected / private 继承 ----

class Base {
public:
    int pub = 1;       // public：任何地方都可访问
protected:
    int prot = 2;      // protected：类内 + 子类内可访问，类外不可
private:
    int priv = 3;      // private：只有类内可访问，子类也不可
public:
    void showBase() const {
        // 类内：三种权限都可访问
        cout << "Base: pub=" << pub << " prot=" << prot << " priv=" << priv << endl;
    }
};

// public 继承：父类 public→public，protected→protected，private→不可访问
class PublicDerived : public Base {
public:
    void show() const {
        // 子类内：pub（public）和 prot（protected）都可访问
        cout << "PublicDerived: pub=" << pub << " prot=" << prot << endl;
        // cout << priv;  // ❌ 编译错误：private 子类不可访问
    }
};

// protected 继承：父类 public→protected，protected→protected
class ProtectedDerived : protected Base {
public:
    void show() const {
        cout << "ProtectedDerived: pub=" << pub << " prot=" << prot << endl;
    }
};

// private 继承：父类所有成员→private（极少用，通常用组合代替）
class PrivateDerived : private Base {
public:
    void show() const {
        cout << "PrivateDerived: pub=" << pub << " prot=" << prot << endl;
    }
};

void demoAccessControl() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.2：三种继承方式的访问权限" << endl;
    cout << string(60, '=') << endl;

    PublicDerived pd;
    pd.show();
    cout << "类外访问 pd.pub = " << pd.pub << endl;  // ✅ public 继承后仍是 public
    // pd.prot  // ❌ 类外不可访问（protected）

    ProtectedDerived protd;
    protd.show();
    // protd.pub  // ❌ 类外不可访问（public 变成了 protected）

    PrivateDerived privd;
    privd.show();
    // privd.pub  // ❌ 类外不可访问（变成了 private）
}

// ---- 2.3 protected 成员的作用 ----

class Shape {
protected:
    // protected：子类内部可以直接访问 width 和 height
    // 如果是 private，子类就必须通过 getter 方法才能访问
    double width;
    double height;

public:
    Shape(double w, double h) : width(w), height(h) {}
    virtual double area() const { return 0; }
};

class RectShape : public Shape {
public:
    RectShape(double w, double h) : Shape(w, h) {}

    double area() const override {
        return width * height;  // 直接访问 protected 成员，简洁
    }

    void info() const {
        cout << "矩形：" << width << " × " << height
             << " = " << area() << endl;
    }
};

class TriangleShape : public Shape {
public:
    TriangleShape(double w, double h) : Shape(w, h) {}

    double area() const override {
        return 0.5 * width * height;
    }

    void info() const {
        cout << "三角形：底=" << width << " 高=" << height
             << " 面积=" << area() << endl;
    }
};

void demoProtected() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.3：protected 成员" << endl;
    cout << string(60, '=') << endl;

    RectShape    rect(5.0, 3.0);
    TriangleShape tri(4.0, 6.0);
    rect.info();
    tri.info();
    // rect.width   // ❌ 类外不可访问（protected）
    cout << "（rect.width 在类外不可访问，因为是 protected）" << endl;
}

// ---- 2.4 is-a 关系：子类对象可赋给父类指针/引用 ----

void demoIsA() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分 2.4：is-a 关系" << endl;
    cout << string(60, '=') << endl;

    Dog dog("旺财", 3, "金毛");
    Cat cat("咪咪", 2, true);

    // is-a 关系：Dog 和 Cat 都是 Animal，可以赋给 Animal 指针
    Animal* p1 = &dog;  // Dog is-a Animal（合法）
    Animal* p2 = &cat;  // Cat is-a Animal（合法）

    p1->eat();      // 调用的是 Animal::eat()（继承的）
    p2->eat();

    // 注意：通过父类指针调用 info()，调用的是 Animal::info()
    // 因为 info() 没有 virtual，是静态绑定（编译期决定）
    cout << "\n通过 Animal* 调用 info()（调用 Animal 版本）：" << endl;
    p1->info();  // ← 调用 Animal::info()，不是 Dog::info()！
    p2->info();  // ← 调用 Animal::info()，不是 Cat::info()！
    cout << "（要让父类指针调用子类版本，需要 virtual 关键字，见第48课）" << endl;
}

// ============================================
// 🔍 第三部分：深入理解（可选）
// ============================================

/*
【深入理解：继承时的内存布局】

    Animal 对象内存布局：
    [name(string)] [age(int)]

    Dog 对象内存布局：
    [name(string)] [age(int)] [breed(string)]
    ↑____继承自 Animal____↑   ↑__Dog 自己的__↑

Dog 对象内存中，父类成员在前面，子类成员在后面。
这就是为什么 Animal* 可以指向 Dog 对象：
前面的内存布局完全一致，通过 Animal* 访问时只看前面那部分。

【深入理解：为什么 public 继承最常用？】

public 继承体现 "is-a" 语义：Dog IS-A Animal
    → 可以在任何需要 Animal 的地方使用 Dog
    → 子类是父类的特化（specialization）

protected/private 继承体现 "implemented-in-terms-of" 语义
    → 仅仅是为了复用父类的实现，不体现 is-a 关系
    → 现代 C++ 中通常用组合（has-a）代替私有继承
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：子类构造函数没有调用父类构造函数（父类成员未初始化）

   class Dog : public Animal {
   public:
       Dog(string name, int age, string breed) {  // ← 没有 : Animal(name, age)
           this->breed = breed;
           // name 和 age 没有初始化！（或使用 Animal 的默认构造，若有的话）
       }
   };

   ✅ 正确：在初始化列表中调用父类构造函数
   Dog(string n, int a, string b)
       : Animal(n, a),   // ← 显式调用父类构造函数
         breed(b) { }

   记忆：父类成员只能由父类构造函数初始化，子类不能直接给 private 成员赋值

------------------------------------------------------------

❌ 错误2：试图访问父类的 private 成员（编译错误）

   class Animal {
   private:
       int age;  // private 成员
   };
   class Dog : public Animal {
   public:
       void showAge() {
           cout << age;  // ❌ 编译错误！private 成员子类不可访问
       }
   };

   ✅ 解决方案1：将 age 改为 protected（子类可访问）
   ✅ 解决方案2：在 Animal 中提供 public getAge() 方法

------------------------------------------------------------

❌ 错误3：忘记 public 关键字（默认是 private 继承）

   class Dog : Animal { ... };    // ← 默认 private 继承！
   class Dog : public Animal { ... };  // ← 正确：public 继承

   结果：private 继承时，Animal 的所有 public 方法在 Dog 外部不可用
   Animal* p = &dog;  // ❌ 编译错误（private 继承无 is-a 关系）

------------------------------------------------------------

❌ 错误4：通过父类指针调用子类特有方法（编译错误）

   Animal* p = &dog;
   p->bark();   // ❌ 编译错误！Animal 没有 bark() 方法
                // 编译器看到的是 Animal*，只知道 Animal 的成员

   ✅ 解决方案：向下转型（需确认对象确实是 Dog）
   Dog* dp = static_cast<Dog*>(p);
   dp->bark();  // ✅ 先转回 Dog*，再调用

------------------------------------------------------------

❌ 错误5：通过父类指针调用非 virtual 方法，期望调用子类版本

   Animal* p = new Dog("旺财", 3, "金毛");
   p->info();  // ← 调用 Animal::info()，不是 Dog::info()！
   // 因为 info() 没有 virtual，是静态绑定（编译期根据指针类型决定）

   ✅ 解决方案：给父类方法加 virtual（见第48课）
   virtual void info() const { ... }  // 父类中加 virtual

------------------------------------------------------------

❌ 错误6：构造/析构顺序错误理解

   Dog dog("旺财", 3, "金毛");

   错误理解：先构造 Dog，再构造 Animal

   ✅ 正确顺序：
   构造：先调用 Animal 构造函数 → 再执行 Dog 构造函数体
   析构：先执行 Dog 析构函数体 → 再调用 Animal 析构函数
   （析构顺序与构造顺序相反）

------------------------------------------------------------

❌ 错误7：用 is-a 关系的地方实际上是 has-a 关系

   ❌ class Car : public Engine { ... }  // Car is-a Engine？不对！
   ✅ class Car { Engine engine; ... }   // Car has-a Engine（组合）

   判断方法：说出来听听是否自然
   "狗是动物" → Dog : public Animal  ✅
   "汽车是引擎" → 不自然 → 应该用组合而非继承
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
【继承语法】

声明：class 子类名 : 继承方式 父类名 { ... };
示例：class Dog : public Animal { ... };

常用：public 继承（体现 is-a 关系）

────────────────────────────────────────────────────

【子类构造函数调用父类构造函数】

语法（初始化列表）：
    子类(参数列表) : 父类名(父类参数), 子类成员(初始值) { 函数体 }

示例：
    Dog(string n, int a, string b)
        : Animal(n, a),    // 调用 Animal(string, int)
          breed(b)         // 初始化 Dog 的 breed
    { cout << "Dog 构造" << endl; }

规则：
    - 若不写，编译器调用父类无参构造函数
    - 父类没有无参构造时，子类必须显式调用

────────────────────────────────────────────────────

【访问权限速查】

                public继承    protected继承   private继承
父类 public  →  public        protected       private
父类 protected→ protected     protected       private
父类 private →  不可访问       不可访问         不可访问

记忆：继承方式取"父类访问权限"和"继承方式"中更严格的那个
    (public 最宽松，private 最严格)

────────────────────────────────────────────────────

【继承 vs 组合选择】

is-a 关系（Dog is-a Animal）    → 使用继承 : public
has-a 关系（Car has-a Engine）  → 使用组合（成员变量）
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
1. 判断题：以下哪些是正确的 is-a 关系，应该用继承？

   a) 学生 is-a 人
   b) 汽车 is-a 轮子
   c) 经理 is-a 员工
   d) 银行账户 is-a 余额
   e) 正方形 is-a 矩形
   f) 飞机 is-a 交通工具

   提示：把句子读出来，判断是否语义正确

------------------------------------------------------------

2. 实现练习：设计一个 Vehicle（交通工具）继承体系

   基类 Vehicle：
   - 属性：brand（品牌）、speed（速度，int）
   - 方法：accelerate()（打印"加速到 X km/h"）、info()（打印基本信息）

   子类 Car（汽车）：
   - 额外属性：doors（车门数）
   - 额外方法：honk()（打印"嘟嘟！"）
   - 重写 info()，包含 doors 信息

   子类 Motorcycle（摩托车）：
   - 额外属性：hasSidecar（是否有边车）
   - 重写 info()

   要求：验证 Vehicle* 可以指向 Car 和 Motorcycle 对象

------------------------------------------------------------

3. 排错练习：以下代码有什么问题？

   class Person {
       string name;  // ← 注意：private
       int    age;
   public:
       Person(string n, int a) : name(n), age(a) {}
   };

   class Student : public Person {
   public:
       int studentId;
       Student(string n, int a, int id) {
           name = n;         // ← 这行有问题吗？
           age  = a;         // ← 这行有问题吗？
           studentId = id;
       }
       void show() {
           cout << name << " " << age << " " << studentId;  // ← 问题？
       }
   };

   提示：注意 Person 中 name 和 age 的访问权限

------------------------------------------------------------

4. 思考题：
   以下代码中，p->info() 调用的是哪个版本？为什么？

   Animal* p = new Dog("旺财", 3, "金毛");
   p->info();   // ← 调用 Animal::info() 还是 Dog::info()？

   如何修改才能让它调用 Dog::info()？
   提示：virtual 关键字的作用是什么？（见第48课）
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "      45_inheritance_basics.cpp - 继承基础" << endl;
    cout << string(60, '=') << endl;

    demoBasicInheritance();
    demoAccessControl();
    demoProtected();
    demoIsA();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 继承语法：class Dog : public Animal（最常用 public 继承）" << endl;
    cout << "2. 子类构造函数必须在初始化列表中调用父类构造函数" << endl;
    cout << "3. protected：子类内可访问，类外不可访问" << endl;
    cout << "4. is-a 关系：子类对象可赋给父类指针/引用" << endl;
    cout << "5. 非 virtual 方法：父类指针调用父类版本（静态绑定）" << endl;

    return 0;
}
