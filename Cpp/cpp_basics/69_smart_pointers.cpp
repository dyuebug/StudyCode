// ============================================
// 智能指针基础 - 超详细注释版
// 日期：2026-05-08（第22天）
// 难度：⭐⭐⭐ 中高级
// ============================================

/*
【核心概念】智能指针

是什么？
- 智能指针是自动管理动态内存的类模板，封装了裸指针
- 利用 RAII 原则，构造时分配内存，析构时自动释放
- C++11 引入，是现代 C++ 的标配

为什么需要？
- 避免内存泄漏：忘记 delete、异常导致跳过 delete
- 异常安全：即使抛出异常也能正确释放内存
- 简化代码：无需手动管理内存生命周期

核心特点：
- 自动内存管理：离开作用域时自动释放
- 所有权语义：明确谁负责释放内存
- 类型安全：编译期检查类型
*/

#include <iostream>
#include <memory>  // 智能指针头文件
#include <string>
#include <vector>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void part1_concepts() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【为什么需要智能指针？】

问题1：内存泄漏
int* ptr = new int(10);
// ... 使用 ptr
// 忘记 delete ptr;  → 内存泄漏！

问题2：异常安全
int* ptr = new int(10);
someFunction();  // 如果抛出异常
delete ptr;      // 这行代码不会执行 → 内存泄漏！

问题3：多次释放
int* ptr = new int(10);
delete ptr;
delete ptr;  // 重复释放 → 程序崩溃！

【智能指针解决方案】

unique_ptr<int> ptr = make_unique<int>(10);
// ... 使用 ptr
// 自动释放，无需手动 delete
// 异常安全，离开作用域自动释放
// 不会重复释放

【三种智能指针】

1. unique_ptr - 独占所有权
   - 一个资源只能有一个 unique_ptr 拥有
   - 不能拷贝，只能移动（std::move）
   - 性能最高，零开销
   - 默认选择

2. shared_ptr - 共享所有权
   - 多个 shared_ptr 可以共享同一个资源
   - 引用计数管理，最后一个释放时才释放资源
   - 有一定开销（引用计数）
   - 需要共享时使用

3. weak_ptr - 弱引用
   - 不拥有资源，不增加引用计数
   - 用于打破 shared_ptr 的循环引用
   - 必须配合 shared_ptr 使用
)" << endl;
}

// ============================================
// 第二部分：unique_ptr 详解 ⭐⭐⭐⭐⭐
// ============================================

void part2_unique_ptr() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：unique_ptr 详解" << endl;
    cout << string(60, '=') << endl;

    // 示例1：基本使用
    cout << "\n示例1：unique_ptr 基本使用" << endl;
    cout << string(60, '-') << endl;

    {
        // 创建 unique_ptr：推荐使用 make_unique
        unique_ptr<int> ptr1 = make_unique<int>(42);

        cout << "ptr1 指向的值：" << *ptr1 << endl;

        // 修改值
        *ptr1 = 100;
        cout << "修改后：" << *ptr1 << endl;

        // 离开作用域时自动释放内存
    }
    cout << "ptr1 已离开作用域，内存已自动释放" << endl;

    // 示例2：unique_ptr 不能拷贝
    cout << "\n示例2：unique_ptr 不能拷贝，只能移动" << endl;
    cout << string(60, '-') << endl;

    {
        unique_ptr<int> ptr1 = make_unique<int>(10);

        // unique_ptr<int> ptr2 = ptr1;  // 错误！不能拷贝

        // 可以移动所有权
        unique_ptr<int> ptr2 = std::move(ptr1);

        cout << "移动后：" << endl;
        cout << "ptr1 是否为空：" << (ptr1 == nullptr ? "是" : "否") << endl;
        cout << "ptr2 指向的值：" << *ptr2 << endl;
    }

    // 示例3：unique_ptr 管理数组
    cout << "\n示例3：unique_ptr 管理数组" << endl;
    cout << string(60, '-') << endl;

    {
        // 管理数组：使用 unique_ptr<T[]>
        unique_ptr<int[]> arr = make_unique<int[]>(5);

        // 初始化数组
        for (int i = 0; i < 5; i++) {
            arr[i] = i * 10;
        }

        // 访问数组元素
        cout << "数组元素：";
        for (int i = 0; i < 5; i++) {
            cout << arr[i] << " ";
        }
        cout << endl;

        // 自动释放数组内存
    }

    // 示例4：unique_ptr 管理自定义类
    cout << "\n示例4：unique_ptr 管理自定义类" << endl;
    cout << string(60, '-') << endl;

    class Person {
    public:
        string name;
        int age;

        Person(string n, int a) : name(n), age(a) {
            cout << "Person 构造：" << name << endl;
        }

        ~Person() {
            cout << "Person 析构：" << name << endl;
        }

        void introduce() {
            cout << "我是 " << name << "，" << age << " 岁" << endl;
        }
    };

    {
        unique_ptr<Person> person = make_unique<Person>("Alice", 25);

        // 使用 -> 访问成员
        person->introduce();

        // 离开作用域时自动调用析构函数
    }
    cout << "person 已离开作用域" << endl;

    // 示例5：unique_ptr 作为函数参数和返回值
    cout << "\n示例5：unique_ptr 作为函数参数和返回值" << endl;
    cout << string(60, '-') << endl;

    // 返回 unique_ptr
    auto createPerson = []() -> unique_ptr<Person> {
        return make_unique<Person>("Bob", 30);
    };

    // 接受 unique_ptr（移动语义）
    auto processPerson = [](unique_ptr<Person> p) {
        p->introduce();
        // p 离开作用域时自动释放
    };

    unique_ptr<Person> p = createPerson();
    processPerson(std::move(p));  // 移动所有权

    cout << "p 是否为空：" << (p == nullptr ? "是" : "否") << endl;
}

// ============================================
// 第三部分：shared_ptr 详解 ⭐⭐⭐⭐⭐
// ============================================

void part3_shared_ptr() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：shared_ptr 详解" << endl;
    cout << string(60, '=') << endl;

    // 示例1：基本使用
    cout << "\n示例1：shared_ptr 基本使用" << endl;
    cout << string(60, '-') << endl;

    {
        // 创建 shared_ptr：推荐使用 make_shared
        shared_ptr<int> ptr1 = make_shared<int>(42);

        cout << "ptr1 指向的值：" << *ptr1 << endl;
        cout << "引用计数：" << ptr1.use_count() << endl;

        // 可以拷贝，共享所有权
        shared_ptr<int> ptr2 = ptr1;

        cout << "\n拷贝后：" << endl;
        cout << "ptr1 引用计数：" << ptr1.use_count() << endl;
        cout << "ptr2 引用计数：" << ptr2.use_count() << endl;

        {
            shared_ptr<int> ptr3 = ptr1;
            cout << "\n再次拷贝后：" << endl;
            cout << "引用计数：" << ptr1.use_count() << endl;
        }

        cout << "\nptr3 离开作用域后：" << endl;
        cout << "引用计数：" << ptr1.use_count() << endl;

        // 最后一个 shared_ptr 离开作用域时才释放内存
    }
    cout << "所有 shared_ptr 都离开作用域，内存已释放" << endl;

    // 示例2：shared_ptr 在容器中使用
    cout << "\n示例2：shared_ptr 在容器中使用" << endl;
    cout << string(60, '-') << endl;

    class Resource {
    public:
        int id;

        Resource(int i) : id(i) {
            cout << "Resource " << id << " 创建" << endl;
        }

        ~Resource() {
            cout << "Resource " << id << " 销毁" << endl;
        }
    };

    {
        vector<shared_ptr<Resource>> resources;

        // 添加资源
        resources.push_back(make_shared<Resource>(1));
        resources.push_back(make_shared<Resource>(2));
        resources.push_back(make_shared<Resource>(3));

        cout << "\n容器中有 " << resources.size() << " 个资源" << endl;

        // 获取第一个资源的引用计数
        cout << "Resource 1 引用计数：" << resources[0].use_count() << endl;

        // 清空容器时自动释放所有资源
    }
    cout << "容器已清空，所有资源已释放" << endl;

    // 示例3：shared_ptr 的循环引用问题
    cout << "\n示例3：shared_ptr 的循环引用问题" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【循环引用问题】

class Node {
public:
    shared_ptr<Node> next;  // 指向下一个节点
    ~Node() { cout << "Node 销毁" << endl; }
};

shared_ptr<Node> node1 = make_shared<Node>();
shared_ptr<Node> node2 = make_shared<Node>();

node1->next = node2;  // node1 → node2
node2->next = node1;  // node2 → node1（循环引用）

// 问题：node1 和 node2 互相持有对方的 shared_ptr
// 引用计数永远不会降到 0，导致内存泄漏！

【解决方案：使用 weak_ptr】

class Node {
public:
    weak_ptr<Node> next;  // 使用 weak_ptr 打破循环
    ~Node() { cout << "Node 销毁" << endl; }
};
)" << endl;
}

// ============================================
// 第四部分：weak_ptr 详解 ⭐⭐⭐⭐⭐
// ============================================

void part4_weak_ptr() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：weak_ptr 详解" << endl;
    cout << string(60, '=') << endl;

    cout << "\n示例：weak_ptr 基本使用" << endl;
    cout << string(60, '-') << endl;

    {
        shared_ptr<int> sp = make_shared<int>(42);

        cout << "shared_ptr 引用计数：" << sp.use_count() << endl;

        // 创建 weak_ptr，不增加引用计数
        weak_ptr<int> wp = sp;

        cout << "创建 weak_ptr 后，引用计数：" << sp.use_count() << endl;

        // weak_ptr 不能直接访问对象，需要转换为 shared_ptr
        if (shared_ptr<int> temp = wp.lock()) {
            cout << "通过 weak_ptr 访问值：" << *temp << endl;
            cout << "临时 shared_ptr 引用计数：" << sp.use_count() << endl;
        }

        cout << "临时 shared_ptr 离开作用域后，引用计数：" << sp.use_count() << endl;
    }

    cout << R"(
【weak_ptr 的作用】

1. 打破循环引用
   - shared_ptr 互相持有会导致内存泄漏
   - 用 weak_ptr 替代其中一个，打破循环

2. 观察者模式
   - 观察者持有 weak_ptr，不影响对象生命周期
   - 对象销毁后，weak_ptr 自动失效

3. 缓存
   - 缓存持有 weak_ptr，不阻止对象释放
   - 对象存在时可以访问，不存在时返回空
)" << endl;
}

// ============================================
// 第五部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void part5_mistakes() {
    cout << "\n" << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【错误1：混用智能指针和裸指针】

❌ 错误：
int* raw = new int(10);
unique_ptr<int> ptr1(raw);
unique_ptr<int> ptr2(raw);  // 错误！两个智能指针管理同一块内存

结果：程序崩溃（重复释放）

✅ 正确：
unique_ptr<int> ptr = make_unique<int>(10);
// 不要从智能指针获取裸指针再创建新的智能指针

────────────────────────────────────

【错误2：忘记使用 make_unique/make_shared】

❌ 不推荐：
unique_ptr<int> ptr(new int(10));

✅ 推荐：
unique_ptr<int> ptr = make_unique<int>(10);

原因：
- make_unique/make_shared 更安全
- make_shared 性能更好（一次内存分配）
- 异常安全

────────────────────────────────────

【错误3：shared_ptr 循环引用】

❌ 错误：
class Node {
    shared_ptr<Node> next;  // 循环引用
};

✅ 正确：
class Node {
    weak_ptr<Node> next;  // 用 weak_ptr 打破循环
};

────────────────────────────────────

【错误4：在容器中存储裸指针】

❌ 错误：
vector<int*> vec;
vec.push_back(new int(10));
// 忘记释放 → 内存泄漏

✅ 正确：
vector<unique_ptr<int>> vec;
vec.push_back(make_unique<int>(10));
// 自动释放

────────────────────────────────────

【错误5：返回局部对象的智能指针】

❌ 错误：
unique_ptr<int> func() {
    int x = 10;
    return unique_ptr<int>(&x);  // 错误！x 是局部变量
}

✅ 正确：
unique_ptr<int> func() {
    return make_unique<int>(10);  // 动态分配
}
)" << endl;
}

// ============================================
// 第六部分：智能指针速查
// ============================================

void part6_reference() {
    cout << "\n" << string(60, '=') << endl;
    cout << "智能指针速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【unique_ptr】

创建：
auto ptr = make_unique<int>(10);

访问：
*ptr        // 解引用
ptr->func() // 访问成员

移动：
auto ptr2 = std::move(ptr);  // 移动所有权

释放：
ptr.reset();  // 手动释放
ptr = nullptr;

────────────────────────────────────

【shared_ptr】

创建：
auto ptr = make_shared<int>(10);

拷贝：
auto ptr2 = ptr;  // 共享所有权

引用计数：
ptr.use_count()  // 获取引用计数

释放：
ptr.reset();  // 减少引用计数

────────────────────────────────────

【weak_ptr】

创建：
shared_ptr<int> sp = make_shared<int>(10);
weak_ptr<int> wp = sp;

访问：
if (auto sp = wp.lock()) {
    // 使用 sp
}

检查：
wp.expired()  // 是否已失效
)" << endl;
}

// ============================================
// 练习题
// ============================================

void exercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 用 unique_ptr 实现一个简单的资源管理类
   要求：管理文件句柄，自动关闭文件
   提示：构造时打开文件，析构时关闭文件

2. 用 shared_ptr 实现一个共享资源池
   要求：多个对象共享同一个资源
   提示：使用 vector<shared_ptr<Resource>>

3. 用 weak_ptr 解决循环引用问题
   要求：实现双向链表，避免内存泄漏
   提示：一个方向用 shared_ptr，另一个方向用 weak_ptr

4. 实现一个智能指针工厂函数
   要求：根据类型创建不同的智能指针
   提示：使用模板和 make_unique/make_shared

5. 用智能指针重构一个使用裸指针的类
   要求：将所有 new/delete 替换为智能指针
   提示：成员变量用 unique_ptr，共享资源用 shared_ptr

6. 实现一个观察者模式
   要求：观察者持有 weak_ptr，不影响对象生命周期
   提示：主题销毁时，观察者的 weak_ptr 自动失效

7. 实现一个简单的缓存系统
   要求：缓存持有 weak_ptr，对象销毁后自动清理
   提示：定期检查 weak_ptr 是否失效
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "智能指针基础学习" << endl;
    cout << string(60, '=') << endl;

    part1_concepts();
    part2_unique_ptr();
    part3_shared_ptr();
    part4_weak_ptr();
    part5_mistakes();
    part6_reference();
    exercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "学习完成！" << endl;
    cout << string(60, '=') << endl;

    return 0;
}
