// ============================================
// 50_polymorphism_exercise.cpp - 超详细注释版
// 日期：第15天
// 难度：⭐⭐ 中级
// 主题：多态实战练习 - 图形管理系统
// ============================================

/*
【核心概念回顾】

本文件是多态的综合实战练习，融合以下知识点：
- 抽象类 + 纯虚函数（定义接口）
- 子类实现多态（各自实现 draw/area）
- 通过父类指针/引用统一处理（多态的典型用法）
- unique_ptr 自动内存管理（避免手动 delete）
- 对象切片（按值传递的危险）

学习目标：
- 理解"定义接口，统一调用"的多态设计模式
- 掌握用 vector<unique_ptr<T>> 管理多态对象集合
- 识别并避免对象切片问题
*/

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【多态的标准使用模式】

步骤1：定义抽象基类（接口）
    class Shape {
        virtual void draw() const = 0;
        virtual double area() const = 0;
        virtual ~Shape() {}
    };

步骤2：子类实现具体行为
    class Circle : public Shape { ... };
    class Rectangle : public Shape { ... };

步骤3：通过父类指针/引用统一处理
    void process(const Shape& s) {
        s.draw();    // 动态绑定：调用实际子类版本
    }

步骤4：用智能指针管理生命周期
    vector<unique_ptr<Shape>> shapes;
    shapes.push_back(make_unique<Circle>(5.0));

【函数卡片：make_unique / unique_ptr】

头文件：<memory>

make_unique<T>(args...)：
    - 在堆上创建 T 类型对象，传入构造参数 args
    - 返回 unique_ptr<T>（独占所有权的智能指针）
    - 替代 new T(args)，避免手动 delete

unique_ptr<T>：
    - 独占所有权：不能复制，只能 move
    - 离开作用域自动 delete
    - 通过 .get() 获取裸指针，通过 -> 访问成员

示例：
    auto p = make_unique<Circle>(3.0);    // 构造 Circle(3.0)
    p->draw();                             // 调用 Circle::draw()
    double a = p->area();                  // 调用 Circle::area()
    // 不需要 delete p，自动释放
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// ---- 2.1 抽象基类：定义图形的统一接口 ----

// Shape 是抽象类，定义所有图形必须实现的接口
// 纯虚函数强制子类实现，保证多态安全
class Shape {
public:
    // 纯虚：返回图形名称（每个图形名称不同）
    virtual string getName() const = 0;

    // 纯虚：绘制图形（每个图形绘制方式不同）
    virtual void draw() const = 0;

    // 纯虚：计算面积（每个图形计算公式不同）
    virtual double area() const = 0;

    // 多态基类析构函数必须 virtual
    virtual ~Shape() {}
};

// ---- 2.2 三个具体图形类 ----

// 圆形：继承 Shape，实现所有纯虚函数
class Circle : public Shape {
private:
    double radius;   // 半径

public:
    // 参数：r - 圆的半径
    explicit Circle(double r) : radius(r) {}

    string getName() const override { return "圆形"; }

    void draw() const override {
        cout << "绘制" << getName() << "，半径=" << radius << endl;
    }

    // 圆面积 = π × r²
    double area() const override {
        return 3.14159 * radius * radius;
    }
};

// 矩形：继承 Shape
class Rectangle : public Shape {
private:
    double width, height;   // 宽和高

public:
    Rectangle(double w, double h) : width(w), height(h) {}

    string getName() const override { return "矩形"; }

    void draw() const override {
        cout << "绘制" << getName() << "，宽=" << width
             << " 高=" << height << endl;
    }

    // 矩形面积 = 宽 × 高
    double area() const override { return width * height; }
};

// 三角形：继承 Shape
class Triangle : public Shape {
private:
    double base, height;   // 底边和高

public:
    Triangle(double b, double h) : base(b), height(h) {}

    string getName() const override { return "三角形"; }

    void draw() const override {
        cout << "绘制" << getName() << "，底=" << base
             << " 高=" << height << endl;
    }

    // 三角形面积 = 底 × 高 / 2
    double area() const override { return 0.5 * base * height; }
};

// ---- 2.3 统一处理函数 ----

// 函数1：渲染所有图形
// 参数：shapes - 包含多种 Shape 子类对象的集合
// 通过 const Shape& 引用，实现多态调用
void renderAll(const vector<unique_ptr<Shape>>& shapes) {
    cout << string(40, '-') << endl;
    cout << "渲染所有图形（共 " << shapes.size() << " 个）" << endl;
    cout << string(40, '-') << endl;

    // 遍历每个 unique_ptr，通过 -> 访问 Shape 接口
    for (const auto& shape : shapes) {
        cout << "[" << shape->getName() << "] ";
        shape->draw();              // 动态绑定：各自调用子类 draw()
        cout << "  面积：" << shape->area() << endl;  // 动态绑定
    }
}

// 函数2：计算总面积
double totalArea(const vector<unique_ptr<Shape>>& shapes) {
    double sum = 0.0;
    for (const auto& shape : shapes) {
        sum += shape->area();  // 多态调用各自的 area()
    }
    return sum;
}

// 函数3：找面积最大的图形
// 参数：shapes - 图形集合（非空）
// 返回：面积最大的图形的 const 引用
const Shape* findLargest(const vector<unique_ptr<Shape>>& shapes) {
    if (shapes.empty()) return nullptr;

    // max_element：找集合中的最大元素
    // 第三个参数是比较函数（lambda 表达式）
    // [](const auto& a, const auto& b) → 比较 a 和 b 的面积
    auto it = max_element(shapes.begin(), shapes.end(),
        [](const auto& a, const auto& b) {
            return a->area() < b->area();  // area 更小的算"更小"
        });

    return it->get();   // 返回裸指针（unique_ptr.get() 获取裸指针）
}

// 函数4：按面积升序排序（演示 sort + lambda）
void sortByArea(vector<unique_ptr<Shape>>& shapes) {
    sort(shapes.begin(), shapes.end(),
        [](const auto& a, const auto& b) {
            return a->area() < b->area();  // 升序
        });
}

void demoShapeSystem() {
    cout << string(60, '=') << endl;
    cout << "第二部分：图形管理系统多态实战" << endl;
    cout << string(60, '=') << endl;

    // 创建图形集合：用 unique_ptr 管理生命周期
    // make_unique<Circle>(3.0) 在堆上创建 Circle(3.0)，返回 unique_ptr<Circle>
    // 自动转换为 unique_ptr<Shape>（父类指针）
    vector<unique_ptr<Shape>> shapes;
    shapes.push_back(make_unique<Circle>(3.0));
    shapes.push_back(make_unique<Rectangle>(4.0, 5.0));
    shapes.push_back(make_unique<Triangle>(6.0, 2.0));
    shapes.push_back(make_unique<Circle>(1.5));
    shapes.push_back(make_unique<Rectangle>(3.0, 3.0));

    cout << "\n--- 渲染所有图形 ---" << endl;
    renderAll(shapes);

    cout << "\n--- 统计信息 ---" << endl;
    cout << "总面积：" << totalArea(shapes) << endl;

    const Shape* largest = findLargest(shapes);
    if (largest) {
        cout << "面积最大：" << largest->getName()
             << "，面积=" << largest->area() << endl;
    }

    cout << "\n--- 按面积升序排列后重新渲染 ---" << endl;
    sortByArea(shapes);
    renderAll(shapes);
    // 离开函数作用域：vector 析构，所有 unique_ptr 析构，自动 delete 所有对象
}

// ---- 2.4 对象切片演示 ----

// 按值传递：会切片（丢失子类信息），多态失效
void drawByValue(Shape shape) {  // ❌ 参数是值类型（抽象类不能这么写，这里用假设说明）
    // 实际上这行编译不了，因为 Shape 是抽象类
    // 假设 Shape 不是抽象类，子类对象赋值给 Shape 参数时会"切片"
    // shape.draw() 调用的是 Shape::draw()，不是子类版本！
}

// 按引用传递：保留子类信息，多态生效（正确做法）
void drawByRef(const Shape& shape) {
    shape.draw();   // 动态绑定，调用实际子类版本
    cout << "  （通过引用，多态正常工作）" << endl;
}

void demoSlicing() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：对象切片问题" << endl;
    cout << string(60, '=') << endl;

    Circle c(5.0);

    cout << "\n--- 通过 const Shape& 引用调用（正确，多态生效）---" << endl;
    drawByRef(c);  // 传入 Circle，通过引用保留多态

    cout << R"(
对象切片的危险（概念演示，不实际执行）：
    void bad(Shape s) { s.draw(); }  // 参数按值
    Circle c(5.0);
    bad(c);   // 将 Circle 复制给 Shape 参数
              // Circle 的 radius 等数据被"切掉"
              // s.draw() 调用 Shape::draw()（多态失效！）

规则：多态场景必须用指针（Shape*）或引用（const Shape&），绝对不能按值
)";
}

// ============================================
// 🔍 第三部分：深入理解（可选）
// ============================================

/*
【深入理解：为什么用 unique_ptr 而不是裸指针？】

裸指针方式（老方式，容易出问题）：
    vector<Shape*> shapes;
    shapes.push_back(new Circle(3.0));
    shapes.push_back(new Rectangle(4.0, 5.0));
    // 必须手动释放每个对象：
    for (auto* s : shapes) delete s;
    shapes.clear();
    // 如果中间抛出异常，delete 不会执行 → 内存泄漏！

unique_ptr 方式（现代 C++，推荐）：
    vector<unique_ptr<Shape>> shapes;
    shapes.push_back(make_unique<Circle>(3.0));
    // 离开作用域自动 delete，即使抛出异常也安全
    // 不需要手动 for-delete 循环

【深入理解：为什么 unique_ptr 不能复制？】

unique_ptr 的"独占所有权"语义：
    一块内存只能有一个 owner（拥有者）
    复制 unique_ptr 会产生两个 owner → 析构时 double free

因此：
    auto p1 = make_unique<Circle>(3.0);
    auto p2 = p1;      // ❌ 编译错误：不能复制
    auto p2 = move(p1); // ✅ 移动所有权，p1 变为空
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：按值传递基类对象（对象切片）

   void process(Shape s) { s.draw(); }   // ← 参数是值类型

   Circle c(5.0);
   process(c);   // c 被"切片"：只保留 Shape 部分，Circle 的 radius 丢失
                 // s.draw() 调用 Shape::draw()，多态失效！

   ✅ 正确：使用引用或指针
   void process(const Shape& s) { s.draw(); }   // 引用，多态正常

------------------------------------------------------------

❌ 错误2：用 vector<Shape> 存储多态对象

   vector<Shape> shapes;
   shapes.push_back(Circle(5.0));  // ❌ 切片！Circle 被转换为 Shape 对象存储
   shapes[0].draw();               // 调用 Shape::draw()，不是 Circle::draw()

   ✅ 正确：用 vector<unique_ptr<Shape>> 或 vector<Shape*>
   vector<unique_ptr<Shape>> shapes;
   shapes.push_back(make_unique<Circle>(5.0));  // 保留 Circle 类型
   shapes[0]->draw();  // 调用 Circle::draw()（多态）

------------------------------------------------------------

❌ 错误3：裸指针管理多态对象，忘记 delete（内存泄漏）

   vector<Shape*> shapes;
   shapes.push_back(new Circle(3.0));
   // ... 程序异常退出，或者忘记写 for-delete 循环
   // 所有 Circle 对象内存泄漏！

   ✅ 使用 unique_ptr 自动管理生命周期：
   vector<unique_ptr<Shape>> shapes;
   shapes.push_back(make_unique<Circle>(3.0));
   // 无需手动 delete

------------------------------------------------------------

❌ 错误4：通过 unique_ptr 调用时使用 . 而不是 ->

   auto p = make_unique<Circle>(3.0);
   p.draw();   // ❌ 编译错误：p 是 unique_ptr，不是 Circle
   p->draw();  // ✅ -> 解引用 unique_ptr 访问 Circle 的方法

------------------------------------------------------------

❌ 错误5：忘记在函数签名用 const 修饰（无法接受 const 对象）

   void process(Shape& s) { s.draw(); }  // ← 没有 const

   const Circle c(5.0);
   process(c);   // ❌ 编译错误：不能将 const 对象传给非 const 引用

   ✅ 正确：若函数不修改对象，参数加 const
   void process(const Shape& s) { s.draw(); }  // 可接受 const 和非 const 对象

------------------------------------------------------------

❌ 错误6：误以为 unique_ptr 可以直接复制到 vector

   auto p = make_unique<Circle>(3.0);
   vector<unique_ptr<Shape>> v;
   v.push_back(p);       // ❌ 编译错误：unique_ptr 不能复制
   v.push_back(move(p)); // ✅ 移动语义，p 变空，所有权转给 vector
   // 或者直接：
   v.push_back(make_unique<Circle>(3.0));  // ✅ 直接构造
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
【make_unique<T>(args...)】

头文件：<memory>（C++14）
语法：auto p = make_unique<T>(构造参数...);
返回：unique_ptr<T>（独占所有权智能指针）
作用：在堆上构造 T 对象，返回管理它的智能指针

示例：
    auto p = make_unique<Circle>(3.0);   // 等价于 new Circle(3.0)
    p->draw();                            // 通过 -> 访问成员
    // 不需要 delete，析构时自动释放

────────────────────────────────────────────────────

【max_element（STL 算法）】

头文件：<algorithm>
语法：max_element(first, last, comp)
参数：
    first, last - 迭代器范围（begin, end）
    comp        - 比较函数：comp(a,b) 返回 true 表示 a 比 b "小"
返回：指向最大元素的迭代器

示例：
    auto it = max_element(v.begin(), v.end(),
        [](const auto& a, const auto& b) {
            return a->area() < b->area();  // 比较面积
        });
    const Shape* largest = it->get();   // 获取裸指针

────────────────────────────────────────────────────

【多态对象集合标准模式】

// 正确模式（现代C++）：
vector<unique_ptr<Shape>> shapes;
shapes.push_back(make_unique<Circle>(3.0));
shapes.push_back(make_unique<Rectangle>(4.0, 5.0));

for (const auto& s : shapes) {
    s->draw();        // 多态调用
    s->area();        // 多态调用
}
// 离开作用域：自动析构全部对象

// 错误模式：
vector<Shape> shapes;        // ❌ 切片
vector<Shape*> shapes;       // ⚠️ 需要手动 delete（不推荐）
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
1. 扩展练习：在本文件的图形系统中新增 Square（正方形）类

   要求：
   - 继承 Shape
   - 只有 side（边长）一个参数
   - 面积 = side²，绘制输出 "绘制正方形，边长=X"
   - 加入 shapes 集合，验证 renderAll 和 totalArea 正常工作

   提示：Square 是否可以继承 Rectangle？如何设计？

------------------------------------------------------------

2. 统计练习：

   给 Shape 增加以下非纯虚方法（带默认实现）：
       virtual string category() const { return "平面图形"; }

   然后编写函数：
       int countByCategory(const vector<unique_ptr<Shape>>& shapes,
                           const string& cat);
   统计指定 category 的图形数量。

   验证：所有图形都返回 "平面图形"（默认实现）

------------------------------------------------------------

3. 排错练习：以下代码有什么问题？

   class Animal {
   public:
       virtual void speak() = 0;
       ~Animal() {}   // 注意
   };
   vector<Animal*> zoo;
   zoo.push_back(new Dog());
   zoo.push_back(new Cat());
   for (auto* a : zoo) {
       a->speak();
       delete a;  // 有没有问题？
   }

   提示：析构函数没有 virtual 时，delete 通过基类指针会发生什么

------------------------------------------------------------

4. 设计挑战：

   将图形系统改造为支持"分层管理"：
   - 新增 Group 类，继承 Shape
   - Group 内部有 vector<unique_ptr<Shape>> children
   - Group::draw() 递归绘制所有子图形
   - Group::area() 返回所有子图形面积之和

   这样一个 Group 可以包含其他 Group，形成树状结构

   提示：这是"组合模式"（Composite Pattern）的思想
)";
}

// ============================================
// main 函数
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "   50_polymorphism_exercise.cpp - 多态实战" << endl;
    cout << string(60, '=') << endl;

    demoShapeSystem();
    demoSlicing();
    showCommonErrors();
    showFunctionCards();
    showExercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "核心要点总结" << endl;
    cout << string(60, '=') << endl;
    cout << "1. 多态标准模式：抽象基类 + 子类实现 + 父类指针/引用统一调用" << endl;
    cout << "2. vector<unique_ptr<T>>：管理多态对象集合的推荐方式" << endl;
    cout << "3. make_unique<T>(args)：创建智能指针，无需手动 delete" << endl;
    cout << "4. 按值传递基类会切片，多态失效，必须用引用或指针" << endl;
    cout << "5. 多态基类析构必须 virtual，否则子类资源泄漏" << endl;

    return 0;
}
