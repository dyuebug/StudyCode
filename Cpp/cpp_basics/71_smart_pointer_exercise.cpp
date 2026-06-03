// ============================================
// 智能指针综合练习 - 超详细注释版
// 日期：2026-05-08（第22天）
// 难度：⭐⭐⭐ 中高级
// ============================================

/*
【学习目标】

通过实战练习巩固智能指针知识：
1. 用智能指针实现实际项目
2. 避免内存泄漏
3. 掌握智能指针最佳实践
4. 理解所有权语义

【练习内容】

- 实现学生管理系统（智能指针版）
- 实现简单的对象池
- 实现观察者模式
- 实现树形数据结构
*/

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;

// ============================================
// 练习1：学生管理系统（智能指针版）
// ============================================

class Student {
private:
    string name;
    int age;
    double score;

public:
    Student(string n, int a, double s)
        : name(n), age(a), score(s) {
        cout << "Student 创建：" << name << endl;
    }

    ~Student() {
        cout << "Student 销毁：" << name << endl;
    }

    string getName() const { return name; }
    int getAge() const { return age; }
    double getScore() const { return score; }

    void print() const {
        cout << "姓名：" << name
             << ", 年龄：" << age
             << ", 成绩：" << score << endl;
    }
};

class StudentManager {
private:
    // 使用 unique_ptr 管理学生对象
    vector<unique_ptr<Student>> students;

public:
    // 添加学生
    void addStudent(string name, int age, double score) {
        students.push_back(make_unique<Student>(name, age, score));
        cout << "添加学生：" << name << endl;
    }

    // 删除学生（按姓名）
    void removeStudent(const string& name) {
        auto it = remove_if(students.begin(), students.end(),
            [&name](const unique_ptr<Student>& s) {
                return s->getName() == name;
            });

        if (it != students.end()) {
            cout << "删除学生：" << name << endl;
            students.erase(it, students.end());
        } else {
            cout << "未找到学生：" << name << endl;
        }
    }

    // 查找学生
    Student* findStudent(const string& name) {
        auto it = find_if(students.begin(), students.end(),
            [&name](const unique_ptr<Student>& s) {
                return s->getName() == name;
            });

        return (it != students.end()) ? it->get() : nullptr;
    }

    // 打印所有学生
    void printAll() const {
        cout << "\n学生列表：" << endl;
        for (const auto& student : students) {
            student->print();
        }
    }

    // 获取学生数量
    size_t getCount() const {
        return students.size();
    }
};

void exercise1() {
    cout << string(60, '=') << endl;
    cout << "练习1：学生管理系统（智能指针版）" << endl;
    cout << string(60, '=') << endl;

    StudentManager manager;

    // 添加学生
    manager.addStudent("Alice", 20, 85.5);
    manager.addStudent("Bob", 22, 90.0);
    manager.addStudent("Charlie", 21, 78.5);

    // 打印所有学生
    manager.printAll();

    // 查找学生
    cout << "\n查找学生 Bob：" << endl;
    if (Student* s = manager.findStudent("Bob")) {
        s->print();
    }

    // 删除学生
    cout << "\n删除学生 Bob：" << endl;
    manager.removeStudent("Bob");

    // 再次打印
    manager.printAll();

    cout << "\n总学生数：" << manager.getCount() << endl;

    // 离开作用域时，所有学生对象自动释放
    cout << "\n离开作用域，自动释放所有学生：" << endl;
}

// ============================================
// 练习2：共享资源池（shared_ptr）
// ============================================

class Resource {
private:
    int id;
    string data;

public:
    Resource(int i, string d) : id(i), data(d) {
        cout << "Resource " << id << " 创建" << endl;
    }

    ~Resource() {
        cout << "Resource " << id << " 销毁" << endl;
    }

    int getId() const { return id; }
    string getData() const { return data; }

    void print() const {
        cout << "Resource " << id << ": " << data << endl;
    }
};

class ResourcePool {
private:
    // 使用 shared_ptr 管理共享资源
    map<int, shared_ptr<Resource>> resources;

public:
    // 添加资源
    void addResource(int id, const string& data) {
        resources[id] = make_shared<Resource>(id, data);
        cout << "添加资源 " << id << "，引用计数：" << resources[id].use_count() << endl;
    }

    // 获取资源（返回 shared_ptr，共享所有权）
    shared_ptr<Resource> getResource(int id) {
        auto it = resources.find(id);
        if (it != resources.end()) {
            cout << "获取资源 " << id << "，引用计数：" << it->second.use_count() << endl;
            return it->second;
        }
        return nullptr;
    }

    // 移除资源
    void removeResource(int id) {
        auto it = resources.find(id);
        if (it != resources.end()) {
            cout << "移除资源 " << id << "，当前引用计数：" << it->second.use_count() << endl;
            resources.erase(it);
        }
    }

    // 打印所有资源
    void printAll() const {
        cout << "\n资源池：" << endl;
        for (const auto& pair : resources) {
            pair.second->print();
            cout << "  引用计数：" << pair.second.use_count() << endl;
        }
    }
};

void exercise2() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习2：共享资源池（shared_ptr）" << endl;
    cout << string(60, '=') << endl;

    ResourcePool pool;

    // 添加资源
    pool.addResource(1, "数据1");
    pool.addResource(2, "数据2");
    pool.addResource(3, "数据3");

    pool.printAll();

    // 获取资源（共享所有权）
    cout << "\n获取资源：" << endl;
    {
        auto res1 = pool.getResource(1);
        auto res2 = pool.getResource(1);  // 再次获取同一个资源

        cout << "res1 和 res2 共享资源 1" << endl;
        pool.printAll();

        // res1 和 res2 离开作用域，引用计数减少
    }

    cout << "\nres1 和 res2 离开作用域后：" << endl;
    pool.printAll();

    // 移除资源
    cout << "\n移除资源 1：" << endl;
    pool.removeResource(1);

    pool.printAll();

    cout << "\n离开作用域，资源池自动清理：" << endl;
}

// ============================================
// 练习3：观察者模式（weak_ptr）
// ============================================

class Observer;

class Subject {
private:
    string name;
    // 使用 weak_ptr 存储观察者，不影响观察者的生命周期
    vector<weak_ptr<Observer>> observers;

public:
    Subject(string n) : name(n) {
        cout << "Subject 创建：" << name << endl;
    }

    ~Subject() {
        cout << "Subject 销毁：" << name << endl;
    }

    // 添加观察者
    void attach(shared_ptr<Observer> observer) {
        observers.push_back(observer);
        cout << "添加观察者" << endl;
    }

    // 通知所有观察者
    void notify(const string& message);

    string getName() const { return name; }
};

class Observer {
private:
    string name;

public:
    Observer(string n) : name(n) {
        cout << "Observer 创建：" << name << endl;
    }

    ~Observer() {
        cout << "Observer 销毁：" << name << endl;
    }

    void update(const string& message) {
        cout << "Observer " << name << " 收到消息：" << message << endl;
    }

    string getName() const { return name; }
};

void Subject::notify(const string& message) {
    cout << "\nSubject " << name << " 发送通知：" << message << endl;

    // 清理失效的观察者
    auto it = remove_if(observers.begin(), observers.end(),
        [](const weak_ptr<Observer>& wp) {
            return wp.expired();
        });
    observers.erase(it, observers.end());

    // 通知所有有效的观察者
    for (auto& wp : observers) {
        if (auto sp = wp.lock()) {
            sp->update(message);
        }
    }
}

void exercise3() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习3：观察者模式（weak_ptr）" << endl;
    cout << string(60, '=') << endl;

    auto subject = make_shared<Subject>("主题1");

    {
        auto observer1 = make_shared<Observer>("观察者1");
        auto observer2 = make_shared<Observer>("观察者2");

        subject->attach(observer1);
        subject->attach(observer2);

        subject->notify("消息1");

        cout << "\nobserver1 离开作用域：" << endl;
    }

    // observer1 和 observer2 已销毁
    cout << "\n观察者已销毁，再次发送通知：" << endl;
    subject->notify("消息2");
}

// ============================================
// 练习4：树形数据结构（shared_ptr + weak_ptr）
// ============================================

// 树节点类（使用 enable_shared_from_this）
class TreeNode : public enable_shared_from_this<TreeNode> {
private:
    int value;
    vector<shared_ptr<TreeNode>> children;  // 子节点（拥有所有权）
    weak_ptr<TreeNode> parent;              // 父节点（不拥有所有权，避免循环引用）

public:
    TreeNode(int v) : value(v) {
        cout << "TreeNode " << value << " 创建" << endl;
    }

    ~TreeNode() {
        cout << "TreeNode " << value << " 销毁" << endl;
    }

    int getValue() const { return value; }

    // 添加子节点
    void addChild(shared_ptr<TreeNode> child) {
        children.push_back(child);
        child->parent = shared_from_this();
    }

    // 获取父节点
    shared_ptr<TreeNode> getParent() {
        return parent.lock();
    }

    // 打印树
    void print(int depth = 0) const {
        for (int i = 0; i < depth; i++) {
            cout << "  ";
        }
        cout << "Node " << value << endl;

        for (const auto& child : children) {
            child->print(depth + 1);
        }
    }
};

void exercise4() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习4：树形数据结构（shared_ptr + weak_ptr）" << endl;
    cout << string(60, '=') << endl;

    // 创建树
    auto root = make_shared<TreeNode>(1);
    auto child1 = make_shared<TreeNode>(2);
    auto child2 = make_shared<TreeNode>(3);
    auto grandchild1 = make_shared<TreeNode>(4);
    auto grandchild2 = make_shared<TreeNode>(5);

    // 构建树结构
    root->addChild(child1);
    root->addChild(child2);
    child1->addChild(grandchild1);
    child1->addChild(grandchild2);

    // 打印树
    cout << "\n树结构：" << endl;
    root->print();

    // 获取父节点
    cout << "\ngrandchild1 的父节点：" << endl;
    if (auto parent = grandchild1->getParent()) {
        cout << "父节点值：" << parent->getValue() << endl;
    }

    cout << "\n离开作用域，树自动销毁：" << endl;
}

// ============================================
// 练习5：智能指针最佳实践总结
// ============================================

void exercise5() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习5：智能指针最佳实践总结" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【智能指针选择指南】

1. 默认使用 unique_ptr
   - 独占所有权
   - 零开销
   - 大多数情况的最佳选择

2. 需要共享时使用 shared_ptr
   - 多个所有者共享资源
   - 引用计数管理
   - 最后一个释放时才释放资源

3. 打破循环引用使用 weak_ptr
   - 不拥有资源
   - 不增加引用计数
   - 必须配合 shared_ptr 使用

【使用场景】

unique_ptr：
- 类的成员变量
- 函数返回值
- 容器中的元素
- 工厂函数

shared_ptr：
- 共享资源池
- 缓存系统
- 观察者模式（被观察者）
- 多线程共享数据

weak_ptr：
- 观察者模式（观察者）
- 缓存系统（不阻止释放）
- 打破循环引用（树、图）

【最佳实践】

1. 优先使用 make_unique/make_shared
2. 避免混用智能指针和裸指针
3. 不要从智能指针获取裸指针再创建新的智能指针
4. 使用 unique_ptr 作为函数参数时，传递引用或移动
5. 使用 shared_ptr 作为函数参数时，传递 const 引用
6. 避免 shared_ptr 的循环引用
7. 在类中使用智能指针管理资源
8. 禁止拷贝或实现深拷贝

【性能考虑】

unique_ptr：
- 零开销，和裸指针一样快
- 编译器优化后几乎没有性能损失

shared_ptr：
- 有引用计数开销
- 线程安全的引用计数（原子操作）
- 比 unique_ptr 慢，但比手动管理安全

weak_ptr：
- 需要转换为 shared_ptr 才能访问
- 有一定开销，但可以避免循环引用
)" << endl;
}

// ============================================
// 主函数
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;
    cout << R"(
❌ 错误1：用裸指针初始化两个 shared_ptr（两个控制块，双重释放）
   int* raw = new int(42);
   shared_ptr<int> p1(raw);
   shared_ptr<int> p2(raw);  // ❌ 两个独立控制块，各自 delete → 崩溃
   ✅ shared_ptr<int> p2 = p1;  // 共享同一控制块

❌ 错误2：unique_ptr 传值不 move（编译错误）
   void consume(unique_ptr<Foo> p) {}
   auto p = make_unique<Foo>();
   consume(p);        // ❌ unique_ptr 不可拷贝
   consume(move(p));  // ✅

❌ 错误3：shared_ptr 循环引用（内存泄漏）
   class A { shared_ptr<B> b; };
   class B { shared_ptr<A> a; };  // ❌ 循环引用，都不释放
   ✅ 次要方向改为 weak_ptr：class B { weak_ptr<A> a; };

❌ 错误4：weak_ptr 不 lock 直接访问成员（编译错误）
   weak_ptr<Foo> wp = sp;
   wp->use();          // ❌ weak_ptr 无 operator->
   wp.lock()->use();   // ✅ 先 lock 转为 shared_ptr

❌ 错误5：get() 得到裸指针后 delete（双重释放）
   auto p = make_shared<Foo>();
   delete p.get();   // ❌ p 析构时还会再 delete → 崩溃
   // 永远不要 delete 智能指针的 get() 结果
)" << endl;
}

int main() {
    cout << "智能指针综合练习" << endl;
    cout << string(60, '=') << endl;

    try {
        exercise1();
        exercise2();
        exercise3();
        exercise4();
        exercise5();

        cout << "\n" << string(60, '=') << endl;
        cout << "所有练习完成！" << endl;
        cout << string(60, '=') << endl;

    } catch (const exception& e) {
        cerr << "错误：" << e.what() << endl;
    }

    return 0;
}

/*
==============================================
🎯 学习总结
==============================================

通过这些练习，你应该掌握了：

1. 【unique_ptr 应用】
   - 学生管理系统
   - 独占所有权
   - 容器中使用

2. 【shared_ptr 应用】
   - 共享资源池
   - 引用计数管理
   - 多个所有者共享

3. 【weak_ptr 应用】
   - 观察者模式
   - 打破循环引用
   - 不影响对象生命周期

4. 【综合应用】
   - 树形数据结构
   - shared_ptr + weak_ptr 组合
   - 避免内存泄漏

5. 【最佳实践】
   - 智能指针选择指南
   - 使用场景
   - 性能考虑

==============================================

🚀 下一步学习建议：

1. 学习现代 C++ 特性（C++11/14/17/20）
2. 学习并发编程（线程、互斥锁、条件变量）
3. 学习设计模式（工厂、单例、观察者等）
4. 实践：用智能指针重构之前的项目

==============================================
*/
