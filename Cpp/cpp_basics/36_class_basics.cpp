// ============================================
// 类与对象基础 - 超详细注释版
// 日期：2026-03-17（第11天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】面向对象编程（OOP）

是什么？
- 面向对象编程是一种编程范式，将数据和操作数据的方法封装在一起

为什么需要？
- 更好地组织代码，提高代码复用性
- 更接近现实世界的思维方式
- 便于维护和扩展

核心特点：
- 封装：将数据和方法封装在类中
- 继承：子类继承父类的特性
- 多态：同一接口，不同实现
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
#include <string>
using namespace std;

int main() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 概念1：什么是类和对象
    // --------------------------------------------

    cout << "\n1.1 什么是类和对象？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【类（Class）】

是什么？
- 类是对象的模板/蓝图
- 类定义了对象的属性（成员变量）和行为（成员函数）

类比：
- 类就像建筑图纸，对象就像根据图纸建造的房子
- 类就像汽车设计图，对象就像生产出来的汽车

【对象（Object）】

是什么？
- 对象是类的实例
- 对象是具体存在的实体

关系：
- 类是抽象的，对象是具体的
- 一个类可以创建多个对象
- 每个对象都有自己独立的数据
)" << endl;

    // --------------------------------------------
    // 1.2 概念2：成员变量和成员函数
    // --------------------------------------------

    cout << "\n1.2 成员变量和成员函数" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【成员变量（Member Variables）】

是什么？
- 类中定义的变量，用于存储对象的数据
- 也叫属性（Attributes）或字段（Fields）

特点：
- 每个对象都有自己的成员变量副本
- 成员变量的值在不同对象中可以不同

【成员函数（Member Functions）】

是什么？
- 类中定义的函数，用于操作对象的数据
- 也叫方法（Methods）

特点：
- 成员函数可以访问对象的成员变量
- 成员函数定义了对象的行为
)" << endl;

    // --------------------------------------------
    // 1.3 重要概念：this指针 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.3 this指针详解" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【this指针】

是什么？
- this是一个隐式指针，指向当前对象
- 在成员函数中，this指向调用该函数的对象

为什么需要？
- 区分成员变量和参数（当名字相同时）
- 返回对象自身（链式调用）
- 在成员函数中明确表示"当前对象"

语法：
this->成员变量
this->成员函数()

注意：
- this是一个常量指针，不能修改this的值
- this只能在成员函数中使用
)" << endl;

    return 0;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

#include <iostream>
#include <string>
using namespace std;

// --------------------------------------------
// 2.1 示例1：定义一个简单的类
// --------------------------------------------

// 定义一个学生类
// class关键字：用于定义类
// Student：类名（通常首字母大写）
class Student {
public:  // 公有访问权限（后面会详细讲解）
    // 成员变量：存储学生的数据
    string name;    // 学生姓名
    int age;        // 学生年龄
    int score;      // 学生成绩

    // 成员函数：操作学生的数据
    // 功能：显示学生信息
    void display() {
        cout << "姓名：" << name << endl;
        cout << "年龄：" << age << endl;
        cout << "成绩：" << score << endl;
    }

    // 成员函数：判断是否及格
    // 返回：bool类型，true表示及格，false表示不及格
    bool isPassed() {
        return score >= 60;  // 60分及格
    }
};

int main() {
    cout << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 创建对象
    // --------------------------------------------

    cout << "\n2.1 创建对象" << endl;
    cout << string(60, '-') << endl;

    // 创建对象：类名 对象名;
    // 这里创建了一个Student类的对象，名为stu1
    Student stu1;

    // 访问成员变量：对象名.成员变量名
    // 使用点运算符(.)访问对象的成员
    stu1.name = "张三";
    stu1.age = 18;
    stu1.score = 85;

    cout << "学生1信息：" << endl;
    // 调用成员函数：对象名.成员函数名()
    stu1.display();

    // 调用成员函数判断是否及格
    if (stu1.isPassed()) {
        cout << "结果：及格" << endl;
    } else {
        cout << "结果：不及格" << endl;
    }

    // --------------------------------------------
    // 2.2 创建多个对象
    // --------------------------------------------

    cout << "\n2.2 创建多个对象" << endl;
    cout << string(60, '-') << endl;

    // 一个类可以创建多个对象
    // 每个对象都有自己独立的成员变量
    Student stu2;
    stu2.name = "李四";
    stu2.age = 19;
    stu2.score = 55;

    cout << "学生2信息：" << endl;
    stu2.display();

    if (stu2.isPassed()) {
        cout << "结果：及格" << endl;
    } else {
        cout << "结果：不及格" << endl;
    }

    // 验证：每个对象的数据是独立的
    cout << "\n验证对象独立性：" << endl;
    cout << "stu1的姓名：" << stu1.name << endl;  // 输出：张三
    cout << "stu2的姓名：" << stu2.name << endl;  // 输出：李四

    return 0;
}

// ============================================
// 第三部分：this指针实践
// ============================================

#include <iostream>
#include <string>
using namespace std;

// 定义一个矩形类，演示this指针的使用
class Rectangle {
public:
    double width;   // 宽度
    double height;  // 高度

    // 成员函数：设置尺寸
    // 参数名和成员变量名相同时，使用this指针区分
    void setSize(double width, double height) {
        // this->width：表示成员变量width
        // width：表示参数width
        this->width = width;
        this->height = height;
    }

    // 成员函数：计算面积
    double getArea() {
        // this->width 等价于 width（当没有歧义时）
        return this->width * this->height;
    }

    // 成员函数：显示信息
    void display() {
        cout << "宽度：" << this->width << endl;
        cout << "高度：" << this->height << endl;
        cout << "面积：" << this->getArea() << endl;
    }

    // 成员函数：返回对象自身（用于链式调用）
    Rectangle* scale(double factor) {
        this->width *= factor;
        this->height *= factor;
        return this;  // 返回当前对象的指针
    }
};

int main() {
    cout << string(60, '=') << endl;
    cout << "第三部分：this指针实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 3.1 使用this指针区分成员变量和参数
    // --------------------------------------------

    cout << "\n3.1 使用this指针" << endl;
    cout << string(60, '-') << endl;

    Rectangle rect;
    rect.setSize(10.0, 5.0);  // 设置宽度10，高度5

    cout << "矩形信息：" << endl;
    rect.display();

    // --------------------------------------------
    // 3.2 链式调用（返回this）
    // --------------------------------------------

    cout << "\n3.2 链式调用" << endl;
    cout << string(60, '-') << endl;

    Rectangle rect2;
    rect2.setSize(4.0, 3.0);

    cout << "原始矩形：" << endl;
    rect2.display();

    // 链式调用：scale返回this，可以继续调用
    rect2.scale(2.0)->scale(1.5);

    cout << "\n缩放后的矩形：" << endl;
    rect2.display();

    return 0;
}

/*
==============================================
🔍 深入理解：类的内存布局（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【类的内存布局】

1. 成员变量的存储
   - 每个对象都有自己的成员变量副本
   - 成员变量存储在对象的内存空间中
   - 对象的大小 = 所有成员变量的大小之和（可能有内存对齐）

2. 成员函数的存储
   - 成员函数只有一份，存储在代码段
   - 所有对象共享同一份成员函数代码
   - 成员函数通过this指针知道操作哪个对象

3. this指针的实现
   - 编译器会自动为成员函数添加一个隐藏的this参数
   - 调用成员函数时，编译器自动传入对象的地址
   - 例如：obj.func() 实际上是 func(&obj)

示例：
class MyClass {
    int x;
    void func() { x = 10; }
};

编译器实际处理为：
struct MyClass {
    int x;
};
void func(MyClass* this) { this->x = 10; }

调用：
MyClass obj;
obj.func();  // 实际上是 func(&obj)

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
#include <string>
using namespace std;

int main() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：忘记分号
    // --------------------------------------------

    cout << "\n4.1 错误1：类定义后忘记分号" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class Student {
    string name;
    int age;
}  // 忘记分号

结果：编译错误

原因：C++类定义后必须加分号

✅ 正确代码：
class Student {
    string name;
    int age;
};  // 必须有分号

记忆技巧：
- 类定义是一个完整的语句，需要分号结束
- 这是C++从C语言继承的语法规则
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：未初始化成员变量
    // --------------------------------------------

    cout << "\n4.2 错误2：未初始化成员变量" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class Student {
public:
    string name;
    int age;
};

Student stu;
cout << stu.age;  // 未初始化，输出随机值

结果：输出随机值或垃圾数据

原因：C++不会自动初始化成员变量

✅ 正确代码：
Student stu;
stu.age = 18;  // 先初始化
cout << stu.age;  // 输出18

或使用构造函数（后续会学习）

记忆技巧：
- 创建对象后立即初始化成员变量
- 或使用构造函数自动初始化
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：混淆类和对象
    // --------------------------------------------

    cout << "\n4.3 错误3：混淆类和对象" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class Student {
public:
    string name;
};

Student.name = "张三";  // 错误：试图访问类的成员

结果：编译错误

原因：类是模板，不能直接访问成员，必须通过对象

✅ 正确代码：
Student stu;  // 先创建对象
stu.name = "张三";  // 通过对象访问成员

记忆技巧：
- 类是图纸，对象是房子
- 不能在图纸上住人，必须先建房子
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：this指针使用错误
    // --------------------------------------------

    cout << "\n4.4 错误4：在非成员函数中使用this" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
void func() {
    this->x = 10;  // 错误：普通函数中没有this
}

结果：编译错误

原因：this只能在成员函数中使用

✅ 正确代码：
class MyClass {
public:
    int x;
    void func() {
        this->x = 10;  // 正确：成员函数中可以使用this
    }
};

记忆技巧：
- this表示"当前对象"
- 只有成员函数才有"当前对象"的概念
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：参数名和成员变量名相同但不用this
    // --------------------------------------------

    cout << "\n4.5 错误5：参数名和成员变量名相同但不用this" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
class Student {
public:
    string name;
    void setName(string name) {
        name = name;  // 错误：参数赋值给参数自己
    }
};

结果：成员变量name没有被修改

原因：name指的是参数，不是成员变量

✅ 正确代码：
void setName(string name) {
    this->name = name;  // 正确：this->name是成员变量
}

或者：
void setName(string n) {
    name = n;  // 参数名不同，不需要this
}

记忆技巧：
- 参数名和成员变量名相同时，必须用this区分
- 或者使用不同的参数名
)" << endl;

    return 0;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

#include <iostream>
using namespace std;

int main() {
    cout << "函数卡片速查" << endl;

    cout << R"(
【类定义语法】

语法：
class 类名 {
访问权限:
    成员变量;
    成员函数();
};  // 注意分号

示例：
class Student {
public:
    string name;
    void display() { cout << name; }
};

────────────────────────────────────

【创建对象】

语法：类名 对象名;

示例：
Student stu;

────────────────────────────────────

【访问成员】

语法：
对象名.成员变量名
对象名.成员函数名()

示例：
stu.name = "张三";
stu.display();

────────────────────────────────────

【this指针】

语法：
this->成员变量
this->成员函数()

作用：
- 指向当前对象
- 区分成员变量和参数
- 返回对象自身

示例：
void setName(string name) {
    this->name = name;
}
)" << endl;

    return 0;
}

// ============================================
// 练习题
// ============================================

#include <iostream>
using namespace std;

int main() {
    cout << "练习题" << endl;

    cout << R"(
1. 定义一个圆形类Circle
   要求：
   - 成员变量：半径radius
   - 成员函数：计算面积getArea()、计算周长getPerimeter()
   提示：圆面积 = π * r²，周长 = 2 * π * r，π取3.14159

2. 定义一个银行账户类BankAccount
   要求：
   - 成员变量：账户号accountNumber、余额balance
   - 成员函数：存款deposit()、取款withdraw()、查询余额getBalance()
   提示：取款时要检查余额是否足够

3. 定义一个时间类Time
   要求：
   - 成员变量：小时hour、分钟minute、秒second
   - 成员函数：设置时间setTime()、显示时间display()
   提示：使用this指针区分参数和成员变量

4. 修改练习2的BankAccount类
   要求：
   - 添加一个成员函数transfer()，实现向另一个账户转账
   - 返回this指针，实现链式调用
   提示：transfer返回BankAccount*类型

5. 思考题：为什么成员函数可以访问成员变量？
   提示：思考this指针的作用
)" << endl;

    return 0;
}
