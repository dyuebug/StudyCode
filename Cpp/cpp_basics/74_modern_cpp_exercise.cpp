// ============================================
// 现代 C++ 综合练习 - 超详细注释版
// 日期：2026-05-26（第23天）
// 难度：⭐⭐⭐ 高级
// ============================================

/*
【综合练习目标】

本文件综合运用：
1. 移动语义与右值引用
2. auto 和范围 for 循环
3. 智能指针
4. Lambda 表达式
5. STL 容器和算法

通过实际项目加深理解。
*/

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <chrono>
using namespace std;

// ============================================
// 练习1：支持移动语义的动态字符串类
// ============================================

class MyString {
private:
    char* data;      // 字符数组指针
    size_t length;   // 字符串长度

public:
    // 构造函数
    MyString(const char* str = "") {
        length = strlen(str);
        data = new char[length + 1];
        strcpy(data, str);
        cout << "构造: \"" << data << "\"" << endl;
    }

    // 拷贝构造函数
    MyString(const MyString& other) {
        length = other.length;
        data = new char[length + 1];
        strcpy(data, other.data);
        cout << "拷贝构造: \"" << data << "\" (深拷贝)" << endl;
    }

    // 移动构造函数 ⭐⭐⭐⭐⭐
    MyString(MyString&& other) noexcept {
        data = other.data;
        length = other.length;
        other.data = nullptr;
        other.length = 0;
        cout << "移动构造: \"" << data << "\" (转移所有权)" << endl;
    }

    // 拷贝赋值运算符
    MyString& operator=(const MyString& other) {
        if (this != &other) {
            delete[] data;
            length = other.length;
            data = new char[length + 1];
            strcpy(data, other.data);
            cout << "拷贝赋值: \"" << data << "\" (深拷贝)" << endl;
        }
        return *this;
    }

    // 移动赋值运算符 ⭐⭐⭐⭐⭐
    MyString& operator=(MyString&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            length = other.length;
            other.data = nullptr;
            other.length = 0;
            cout << "移动赋值: \"" << data << "\" (转移所有权)" << endl;
        }
        return *this;
    }

    // 析构函数
    ~MyString() {
        if (data) {
            cout << "析构: \"" << data << "\"" << endl;
        } else {
            cout << "析构: (空字符串)" << endl;
        }
        delete[] data;
    }

    // 获取字符串
    const char* c_str() const { return data ? data : ""; }
    size_t size() const { return length; }
};

void exercise1_move_string() {
    cout << string(60, '=') << endl;
    cout << "练习1：支持移动语义的动态字符串类" << endl;
    cout << string(60, '=') << endl;

    cout << "\n【测试1：拷贝 vs 移动】" << endl;
    MyString s1("Hello");
    MyString s2 = s1;              // 拷贝构造
    MyString s3 = std::move(s1);   // 移动构造

    cout << "\n【测试2：容器中的移动】" << endl;
    vector<MyString> vec;
    vec.push_back(MyString("World"));  // 移动构造（临时对象）

    MyString s4("C++");
    vec.push_back(std::move(s4));      // 移动构造（显式移动）

    cout << "\n【测试3：返回值优化】" << endl;
    auto createString = []() -> MyString {
        return MyString("Lambda");  // RVO 或移动
    };
    MyString s5 = createString();
}

// ============================================
// 练习2：性能对比 - 拷贝 vs 移动
// ============================================

void exercise2_performance() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习2：性能对比 - 拷贝 vs 移动" << endl;
    cout << string(60, '=') << endl;

    const int SIZE = 1000000;  // 100万个元素

    // 测试1：拷贝性能
    cout << "\n【测试1：拷贝性能】" << endl;
    auto start = chrono::high_resolution_clock::now();

    vector<int> vec1(SIZE, 42);
    vector<int> vec2 = vec1;  // 拷贝构造：复制100万个元素

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    cout << "拷贝 " << SIZE << " 个元素耗时: " << duration.count() << " 微秒" << endl;

    // 测试2：移动性能
    cout << "\n【测试2：移动性能】" << endl;
    start = chrono::high_resolution_clock::now();

    vector<int> vec3(SIZE, 42);
    vector<int> vec4 = std::move(vec3);  // 移动构造：只转移指针

    end = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(end - start);
    cout << "移动 " << SIZE << " 个元素耗时: " << duration.count() << " 微秒" << endl;

    cout << "\n【结论】移动比拷贝快得多！" << endl;
}

// ============================================
// 练习3：现代 C++ 语法综合应用
// ============================================

void exercise3_modern_syntax() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习3：现代 C++ 语法综合应用" << endl;
    cout << string(60, '=') << endl;

    // 使用 auto 和初始化列表
    auto numbers = vector<int>{5, 2, 8, 1, 9, 3, 7};
    auto names = vector<string>{"Alice", "Bob", "Charlie", "David"};

    // 使用范围 for 和 const auto&
    cout << "\n【原始数据】" << endl;
    cout << "数字: ";
    for (const auto& num : numbers) {
        cout << num << " ";
    }
    cout << endl;

    cout << "姓名: ";
    for (const auto& name : names) {
        cout << name << " ";
    }
    cout << endl;

    // 使用 Lambda 表达式和 STL 算法
    cout << "\n【排序后】" << endl;
    sort(numbers.begin(), numbers.end());
    cout << "数字: ";
    for (const auto& num : numbers) {
        cout << num << " ";
    }
    cout << endl;

    // 使用 Lambda 自定义排序
    sort(names.begin(), names.end(), [](const string& a, const string& b) {
        return a.length() < b.length();  // 按长度排序
    });
    cout << "姓名（按长度）: ";
    for (const auto& name : names) {
        cout << name << " ";
    }
    cout << endl;

    // 使用 auto 和 Lambda 进行过滤
    cout << "\n【过滤：大于5的数字】" << endl;
    auto result = vector<int>{};
    copy_if(numbers.begin(), numbers.end(), back_inserter(result),
            [](int x) { return x > 5; });
    for (const auto& num : result) {
        cout << num << " ";
    }
    cout << endl;
}

// ============================================
// 练习4：智能指针 + 移动语义
// ============================================

class Resource {
private:
    string name;
    vector<int> data;

public:
    Resource(const string& n, size_t size) : name(n), data(size, 0) {
        cout << "创建资源: " << name << " (大小: " << size << ")" << endl;
    }

    ~Resource() {
        cout << "销毁资源: " << name << endl;
    }

    void process() {
        cout << "处理资源: " << name << endl;
    }

    const string& getName() const { return name; }
};

void exercise4_smart_pointers() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习4：智能指针 + 移动语义" << endl;
    cout << string(60, '=') << endl;

    // unique_ptr：独占所有权，只能移动
    cout << "\n【unique_ptr 移动】" << endl;
    auto res1 = make_unique<Resource>("Resource1", 100);
    res1->process();

    // 移动所有权
    auto res2 = std::move(res1);  // res1 变为 nullptr
    if (!res1) {
        cout << "res1 已被移动，现在是 nullptr" << endl;
    }
    res2->process();

    // 容器中的 unique_ptr
    cout << "\n【容器中的 unique_ptr】" << endl;
    auto resources = vector<unique_ptr<Resource>>{};
    resources.push_back(make_unique<Resource>("Res_A", 50));
    resources.push_back(make_unique<Resource>("Res_B", 75));

    // 使用范围 for 遍历（注意：unique_ptr 不能拷贝）
    for (const auto& res : resources) {
        res->process();
    }

    cout << "\n【作用域结束，自动释放资源】" << endl;
}

// ============================================
// 练习5：实战项目 - 学生成绩管理系统（现代 C++ 版）
// ============================================

class Student {
private:
    string name;
    vector<int> scores;

public:
    Student(string n, vector<int> s)
        : name(std::move(n)), scores(std::move(s)) {  // 移动参数
        cout << "创建学生: " << name << endl;
    }

    const string& getName() const { return name; }
    const vector<int>& getScores() const { return scores; }

    double getAverage() const {
        if (scores.empty()) return 0.0;
        int sum = 0;
        for (auto score : scores) {
            sum += score;
        }
        return static_cast<double>(sum) / scores.size();
    }
};

void exercise5_student_system() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习5：学生成绩管理系统（现代 C++ 版）" << endl;
    cout << string(60, '=') << endl;

    // 使用智能指针管理学生对象
    auto students = vector<unique_ptr<Student>>{};

    // 添加学生（使用移动语义）
    students.push_back(make_unique<Student>("Alice", vector<int>{90, 85, 92}));
    students.push_back(make_unique<Student>("Bob", vector<int>{78, 82, 88}));
    students.push_back(make_unique<Student>("Charlie", vector<int>{95, 98, 93}));

    // 使用范围 for 和 auto 遍历
    cout << "\n【学生成绩单】" << endl;
    for (const auto& student : students) {
        cout << student->getName() << ": ";
        for (auto score : student->getScores()) {
            cout << score << " ";
        }
        cout << "| 平均分: " << student->getAverage() << endl;
    }

    // 使用 Lambda 和算法查找最高平均分
    auto maxStudent = max_element(students.begin(), students.end(),
        [](const unique_ptr<Student>& a, const unique_ptr<Student>& b) {
            return a->getAverage() < b->getAverage();
        });

    cout << "\n【最高平均分】" << endl;
    cout << (*maxStudent)->getName() << ": "
         << (*maxStudent)->getAverage() << endl;

    // 使用 Lambda 过滤平均分大于85的学生
    cout << "\n【平均分 > 85 的学生】" << endl;
    for (const auto& student : students) {
        if (student->getAverage() > 85) {
            cout << student->getName() << ": "
                 << student->getAverage() << endl;
        }
    }
}


void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 " << endl;
    cout << string(60, '=') << endl;
    cout << R"(
[ERROR1] move 后继续使用源对象
   auto p = make_unique<Foo>(); auto q = move(p);
   p->use();  // 崩溃，p 是 nullptr
   正确：move 后不再使用源对象

[ERROR2] const 对象 move 退化为拷贝
   const vector<int> cv={1,2,3}; auto v2=move(cv); // 还是拷贝
   正确：move 只对非 const 对象有效

[ERROR3] range-for 中修改容器大小（迭代器失效）
   for(auto x:v){if(x>0)v.push_back(x*2);} // push_back 可能重分配
   正确：遍历前备份，遍历后插入

[ERROR4] auto 推导值类型复制大对象
   auto bigObj = getHeavy(); // 拷贝
   正确：auto& bigObj = ... 或 const auto& bigObj = ...

[ERROR5] unique_ptr 容器 push_back 忘记 move
   vec.push_back(p);       // 编译错误
   vec.push_back(move(p)); // 正确
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "============================================" << endl;
    cout << "现代 C++ 综合练习" << endl;
    cout << "============================================" << endl;

    exercise1_move_string();
    exercise2_performance();
    exercise3_modern_syntax();
    exercise4_smart_pointers();
    exercise5_student_system();

    cout << "\n" << string(60, '=') << endl;
    cout << "所有练习完成！" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【总结】

本次练习综合运用了：
✅ 移动语义：避免不必要的拷贝，提升性能
✅ 右值引用：实现移动构造和移动赋值
✅ auto：简化类型声明
✅ 范围 for：简化容器遍历
✅ 智能指针：自动内存管理
✅ Lambda 表达式：简化算法使用
✅ STL 算法：sort、max_element、copy_if

关键收获：
1. 移动语义可以大幅提升性能（100倍以上）
2. const auto& 是遍历容器的最佳选择
3. unique_ptr 只能移动，不能拷贝
4. 现代 C++ 让代码更简洁、更安全
5. 组合使用这些特性可以写出高效优雅的代码

最佳实践：
✅ 返回大对象时使用移动语义
✅ 容器中存储智能指针管理资源
✅ 使用 const auto& 遍历容器
✅ 使用 Lambda 简化算法调用
✅ 使用 make_unique/make_shared 创建智能指针

下一步：
→ 学习完美转发（perfect forwarding）
→ 学习 emplace 系列函数
→ 学习 constexpr 和编译期计算
→ 学习更多 C++17/20 新特性
)" << endl;

    return 0;
}







