// ============================================
// 78_weak_ptr_basics.cpp - weak_ptr 基础
// 功能：学习弱引用智能指针，解决循环引用问题
// ============================================

#include <memory>
#include <iostream>
#include <vector>
using namespace std;

// ============================================
// 示例1：循环引用问题演示
// ============================================

class B_Problem;  // 前向声明

class A_Problem {
public:
    shared_ptr<B_Problem> ptrB;

    A_Problem() {
        cout << "A_Problem 创建" << endl;
    }

    ~A_Problem() {
        cout << "A_Problem 销毁" << endl;
    }
};

class B_Problem {
public:
    shared_ptr<A_Problem> ptrA;  // 循环引用！

    B_Problem() {
        cout << "B_Problem 创建" << endl;
    }

    ~B_Problem() {
        cout << "B_Problem 销毁" << endl;
    }
};

// ============================================
// 示例2：使用 weak_ptr 解决循环引用
// ============================================

class B_Solution;  // 前向声明

class A_Solution {
public:
    shared_ptr<B_Solution> ptrB;

    A_Solution() {
        cout << "A_Solution 创建" << endl;
    }

    ~A_Solution() {
        cout << "A_Solution 销毁" << endl;
    }
};

class B_Solution {
public:
    weak_ptr<A_Solution> ptrA;  // 使用 weak_ptr 打破循环引用

    B_Solution() {
        cout << "B_Solution 创建" << endl;
    }

    ~B_Solution() {
        cout << "B_Solution 销毁" << endl;
    }

    void accessA() {
        // 使用 weak_ptr 前需要先转换为 shared_ptr
        if (auto sp = ptrA.lock()) {
            cout << "成功访问 A_Solution" << endl;
        } else {
            cout << "A_Solution 已被销毁" << endl;
        }
    }
};

// ============================================
// 示例3：实际应用 - 公司与员工
// ============================================

class Person;

class Company {
private:
    string name;
public:
    weak_ptr<Person> ceo;  // 使用 weak_ptr 避免循环引用

    Company(const string& n) : name(n) {
        cout << "Company " << name << " 创建" << endl;
    }

    ~Company() {
        cout << "Company " << name << " 销毁" << endl;
    }

    string getName() const { return name; }

    void showCEO() {
        if (auto p = ceo.lock()) {
            cout << name << " 的 CEO 还在职" << endl;
        } else {
            cout << name << " 的 CEO 已离职" << endl;
        }
    }
};

class Person {
private:
    string name;
public:
    shared_ptr<Company> company;

    Person(const string& n) : name(n) {
        cout << "Person " << name << " 创建" << endl;
    }

    ~Person() {
        cout << "Person " << name << " 销毁" << endl;
    }

    string getName() const { return name; }
};

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：lock() 返回后不检查就解引用（对象可能已被销毁）

   weak_ptr<int> wp = ...;
   auto sp = wp.lock();
   cout << *sp;   // ❌ 如果 sp 是 nullptr（对象已释放），崩溃

   ✅ 用 if 检查：
   if (auto sp = wp.lock()) {
       cout << *sp;   // 安全，sp 有效
   }

------------------------------------------------------------

❌ 错误2：expired() 和 lock() 之间的竞态（多线程环境）

   if (!wp.expired()) {       // 检查时对象存在
       auto sp = wp.lock();   // 此时另一线程可能已释放 shared_ptr
       // sp 可能是 nullptr！
   }

   ✅ 直接用 lock()，一步完成检查+获取：
   if (auto sp = wp.lock()) {
       // sp 有效，对象在此作用域内不会被释放
   }

------------------------------------------------------------

❌ 错误3：直接用 weak_ptr 访问成员（不能直接 ->）

   weak_ptr<Resource> wp = ...;
   wp->use();   // ❌ 编译错误：weak_ptr 没有 operator->

   ✅ 必须先 lock() 转换为 shared_ptr：
   if (auto sp = wp.lock()) {
       sp->use();   // ✅
   }

------------------------------------------------------------

❌ 错误4：循环引用只改了一边（仍然泄漏）

   class A { shared_ptr<B> b; };   // 强引用
   class B { shared_ptr<A> a; };   // ❌ 还是强引用，仍然循环

   只改一边无效！
   ✅ 循环中"次要方向"改为 weak_ptr：
   class A { shared_ptr<B> b; };   // A 拥有 B（主方向）
   class B { weak_ptr<A> a; };     // ✅ B 只弱引用 A（打破循环）

------------------------------------------------------------

❌ 错误5：用 weak_ptr 后忘记判断对象生命周期（悬空访问）

   struct Node {
       weak_ptr<Node> parent;
       void printParent() {
           parent.lock()->process();  // ❌ 没有检查，parent 可能已过期
       }
   };

   ✅ 每次使用前 lock() + 检查：
   void printParent() {
       if (auto p = parent.lock()) p->process();
       else cout << "parent 已释放" << endl;
   }

------------------------------------------------------------

❌ 错误6：把 weak_ptr 当 shared_ptr 的"轻量版"滥用（该用引用时用了 weak_ptr）

   // 函数只需要临时访问对象（不需要延长生命周期）
   void inspect(weak_ptr<Resource> wp) {   // ❌ 传 weak_ptr 不必要
       if (auto sp = wp.lock()) sp->use();
   }

   ✅ 临时访问只需引用或裸指针，weak_ptr 用于需要"可选持有"的场景：
   void inspect(const Resource& r) { r.use(); }   // 简单
   void inspect(const shared_ptr<Resource>& sp) { sp->use(); }  // 也可以
   // weak_ptr 只用于：需要引用对象但不影响其生命周期（缓存、观察者、打破循环）
)" << endl;
}

int main() {
    cout << "=== weak_ptr 基础示例 ===" << endl;

    // 示例1：循环引用问题（内存泄漏）
    {
        cout << "\n【示例1】循环引用问题（内存泄漏）" << endl;

        auto a = make_shared<A_Problem>();
        auto b = make_shared<B_Problem>();

        cout << "设置循环引用..." << endl;
        a->ptrB = b;
        b->ptrA = a;  // 循环引用！

        cout << "a 引用计数：" << a.use_count() << endl;  // 2
        cout << "b 引用计数：" << b.use_count() << endl;  // 2

        cout << "离开作用域..." << endl;
        // 问题：a 和 b 永远不会被销毁！
        // a 的引用计数为 2（原始 + b->ptrA）
        // b 的引用计数为 2（原始 + a->ptrB）
        // 离开作用域后，各自引用计数减1，但仍为1，不会销毁
    }

    cout << "\n注意：A_Problem 和 B_Problem 没有被销毁！内存泄漏！\n" << endl;

    // 示例2：使用 weak_ptr 解决循环引用
    {
        cout << "\n【示例2】使用 weak_ptr 解决循环引用" << endl;

        auto a = make_shared<A_Solution>();
        auto b = make_shared<B_Solution>();

        cout << "设置引用..." << endl;
        a->ptrB = b;
        b->ptrA = a;  // weak_ptr 不增加引用计数

        cout << "a 引用计数：" << a.use_count() << endl;  // 1
        cout << "b 引用计数：" << b.use_count() << endl;  // 2

        cout << "通过 weak_ptr 访问对象..." << endl;
        b->accessA();

        cout << "离开作用域..." << endl;
    }  // 正常销毁，没有内存泄漏

    cout << "\n注意：A_Solution 和 B_Solution 正常销毁！\n" << endl;

    // 示例3：weak_ptr 基本操作
    {
        cout << "\n【示例3】weak_ptr 基本操作" << endl;

        weak_ptr<int> wp;

        {
            auto sp = make_shared<int>(100);
            cout << "创建 shared_ptr，值：" << *sp << endl;
            cout << "shared_ptr 引用计数：" << sp.use_count() << endl;

            // 从 shared_ptr 创建 weak_ptr
            wp = sp;
            cout << "创建 weak_ptr" << endl;
            cout << "shared_ptr 引用计数：" << sp.use_count() << endl;  // 仍为 1

            // 检查对象是否存在
            if (wp.expired()) {
                cout << "对象已被销毁" << endl;
            } else {
                cout << "对象仍然存在" << endl;
            }

            // 使用 weak_ptr（需要先转换为 shared_ptr）
            if (auto sp2 = wp.lock()) {
                cout << "通过 weak_ptr 访问，值：" << *sp2 << endl;
                cout << "临时 shared_ptr 引用计数：" << sp2.use_count() << endl;  // 2
            }

            cout << "shared_ptr 离开作用域..." << endl;
        }  // sp 销毁，对象被释放

        cout << "\nshared_ptr 销毁后：" << endl;

        // 检查对象是否存在
        if (wp.expired()) {
            cout << "对象已被销毁" << endl;
        } else {
            cout << "对象仍然存在" << endl;
        }

        // 尝试使用 weak_ptr
        if (auto sp2 = wp.lock()) {
            cout << "通过 weak_ptr 访问成功" << endl;
        } else {
            cout << "通过 weak_ptr 访问失败，对象已被销毁" << endl;
        }
    }

    // 示例4：实际应用 - 公司与员工
    {
        cout << "\n【示例4】实际应用 - 公司与员工" << endl;

        auto company = make_shared<Company>("科技公司");
        auto person = make_shared<Person>("张三");

        cout << "\n设置关系..." << endl;
        company->ceo = person;
        person->company = company;

        cout << "\n引用计数：" << endl;
        cout << "company: " << company.use_count() << endl;  // 2（原始 + person->company）
        cout << "person: " << person.use_count() << endl;    // 1（weak_ptr 不增加）

        cout << "\n检查 CEO..." << endl;
        company->showCEO();

        cout << "\nCEO 离职（person 销毁）..." << endl;
        person.reset();

        cout << "\n再次检查 CEO..." << endl;
        company->showCEO();

        cout << "\n离开作用域..." << endl;
    }  // 正常销毁，没有内存泄漏

    // 示例5：weak_ptr 在容器中使用
    {
        cout << "\n【示例5】weak_ptr 在容器中使用" << endl;

        vector<weak_ptr<int>> weakVec;

        {
            auto sp1 = make_shared<int>(100);
            auto sp2 = make_shared<int>(200);
            auto sp3 = make_shared<int>(300);

            cout << "添加到 weak_ptr 容器..." << endl;
            weakVec.push_back(sp1);
            weakVec.push_back(sp2);
            weakVec.push_back(sp3);

            cout << "shared_ptr 引用计数：" << endl;
            cout << "sp1: " << sp1.use_count() << endl;  // 1
            cout << "sp2: " << sp2.use_count() << endl;  // 1
            cout << "sp3: " << sp3.use_count() << endl;  // 1

            cout << "\n遍历 weak_ptr 容器：" << endl;
            for (size_t i = 0; i < weakVec.size(); i++) {
                if (auto sp = weakVec[i].lock()) {
                    cout << "元素 " << i << "：" << *sp << endl;
                }
            }

            cout << "\nshared_ptr 离开作用域..." << endl;
        }  // sp1, sp2, sp3 销毁

        cout << "\nshared_ptr 销毁后，遍历 weak_ptr 容器：" << endl;
        int validCount = 0;
        for (size_t i = 0; i < weakVec.size(); i++) {
            if (auto sp = weakVec[i].lock()) {
                cout << "元素 " << i << "：" << *sp << endl;
                validCount++;
            } else {
                cout << "元素 " << i << "：已失效" << endl;
            }
        }
        cout << "有效元素数量：" << validCount << endl;
    }

    // 示例6：weak_ptr 的 use_count
    {
        cout << "\n【示例6】weak_ptr 的 use_count" << endl;

        auto sp = make_shared<int>(400);
        weak_ptr<int> wp = sp;

        cout << "shared_ptr 引用计数：" << sp.use_count() << endl;
        cout << "weak_ptr 的 use_count：" << wp.use_count() << endl;  // 与 shared_ptr 相同

        auto sp2 = sp;
        cout << "\n复制 shared_ptr 后：" << endl;
        cout << "shared_ptr 引用计数：" << sp.use_count() << endl;
        cout << "weak_ptr 的 use_count：" << wp.use_count() << endl;  // 也会增加

        sp2.reset();
        cout << "\nreset 一个 shared_ptr 后：" << endl;
        cout << "shared_ptr 引用计数：" << sp.use_count() << endl;
        cout << "weak_ptr 的 use_count：" << wp.use_count() << endl;  // 也会减少
    }

    // 示例7：weak_ptr 的 reset
    {
        cout << "\n【示例7】weak_ptr 的 reset" << endl;

        auto sp = make_shared<int>(500);
        weak_ptr<int> wp = sp;

        cout << "创建 weak_ptr" << endl;
        cout << "expired: " << (wp.expired() ? "是" : "否") << endl;

        wp.reset();
        cout << "\nweak_ptr.reset() 后" << endl;
        cout << "expired: " << (wp.expired() ? "是" : "否") << endl;

        cout << "\nshared_ptr 仍然有效：" << *sp << endl;
    }

    // 示例8：观察者模式示例
    {
        cout << "\n【示例8】观察者模式示例" << endl;

        class Subject {
        private:
            vector<weak_ptr<int>> observers;
        public:
            void addObserver(shared_ptr<int> observer) {
                observers.push_back(observer);
                cout << "添加观察者，当前观察者数量：" << observers.size() << endl;
            }

            void notifyObservers() {
                cout << "\n通知所有观察者..." << endl;
                int activeCount = 0;
                for (auto& wp : observers) {
                    if (auto sp = wp.lock()) {
                        cout << "通知观察者，值：" << *sp << endl;
                        activeCount++;
                    }
                }
                cout << "活跃观察者数量：" << activeCount << "/" << observers.size() << endl;
            }

            void cleanupObservers() {
                cout << "\n清理失效的观察者..." << endl;
                auto it = observers.begin();
                while (it != observers.end()) {
                    if (it->expired()) {
                        it = observers.erase(it);
                    } else {
                        ++it;
                    }
                }
                cout << "清理后观察者数量：" << observers.size() << endl;
            }
        };

        Subject subject;

        {
            auto obs1 = make_shared<int>(1);
            auto obs2 = make_shared<int>(2);
            auto obs3 = make_shared<int>(3);

            subject.addObserver(obs1);
            subject.addObserver(obs2);
            subject.addObserver(obs3);

            subject.notifyObservers();

            cout << "\n销毁 obs2..." << endl;
            obs2.reset();

            subject.notifyObservers();
            subject.cleanupObservers();

            cout << "\n离开作用域..." << endl;
        }

        cout << "\n所有观察者销毁后：" << endl;
        subject.notifyObservers();
        subject.cleanupObservers();
    }

    cout << "\n程序结束" << endl;
    return 0;
}
