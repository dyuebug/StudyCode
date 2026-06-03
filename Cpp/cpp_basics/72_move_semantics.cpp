// ============================================
// 移动语义与右值引用 - 超详细注释版
// 日期：2026-05-26（第23天）
// 难度：⭐⭐⭐ 高级
// ============================================

/*
【核心概念】移动语义与右值引用

是什么？
- 移动语义：转移资源所有权，而不是拷贝资源
- 右值引用：绑定到临时对象的引用类型（&&）

为什么需要？
- 避免不必要的深拷贝，大幅提升性能
- 充分利用临时对象即将销毁的特点

核心特点：
- 左值有名字可取地址，右值是临时对象
- 移动后源对象处于有效但未定义状态
- std::move 将左值转换为右值引用
*/

#include <iostream>
#include <vector>
#include <string>
#include <utility>  // std::move
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

void part1_concepts() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 左值与右值
    // --------------------------------------------

    cout << "\n1.1 左值（lvalue）与右值（rvalue）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【左值 lvalue】
- 有名字的对象，可以取地址
- 生命周期持续到作用域结束
- 可以出现在赋值语句左边或右边

示例：
int x = 10;     // x 是左值
int* p = &x;    // 可以取地址
x = 20;         // 可以赋值

【右值 rvalue】
- 临时对象、字面量、即将销毁的对象
- 不能取地址
- 只能出现在赋值语句右边

示例：
int y = 10;     // 10 是右值（字面量）
int z = x + y;  // x+y 是右值（临时结果）
// int* p = &10; // 错误！不能取右值的地址
)" << endl;

    // --------------------------------------------
    // 1.2 右值引用 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.2 右值引用（&&）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【右值引用】

语法：类型&& 变量名

作用：
- 绑定到右值（临时对象）
- 延长临时对象的生命周期
- 实现移动语义的基础

对比：
int& lref = x;      // 左值引用，绑定到左值
int&& rref = 10;    // 右值引用，绑定到右值
int&& rref2 = x+y;  // 右值引用，绑定到临时结果
)" << endl;

    // 示例：右值引用延长临时对象生命周期
    string&& rref = string("Hello");  // 绑定到临时 string 对象
    cout << "右值引用绑定的值: " << rref << endl;
    // 临时对象的生命周期延长到 rref 作用域结束

    // --------------------------------------------
    // 1.3 移动语义 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.3 移动语义（Move Semantics）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【移动语义】

是什么？
- 转移资源所有权，而不是拷贝资源
- 源对象的资源被"偷走"，转移给目标对象

为什么需要？
- 避免深拷贝的开销（复制大量数据）
- 充分利用临时对象即将销毁的特点

核心思想：
- 拷贝：复制一份新的资源（慢）
- 移动：转移资源指针（快）

移动后的状态：
- 源对象处于"有效但未定义"状态
- 可以安全析构，但不应再使用其值
)" << endl;

    // --------------------------------------------
    // 1.4 std::move 函数 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "\n1.4 std::move() 函数" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【函数卡片：std::move()】

作用：将左值转换为右值引用
头文件：<utility>

完整语法：
template<typename T>
typename remove_reference<T>::type&& move(T&& t) noexcept;

重要说明：
1. std::move 本身不移动任何东西！
2. 它只是类型转换：左值 → 右值引用
3. 真正的移动发生在移动构造/赋值函数中

使用场景：
- 明确表示"我不再需要这个对象的值"
- 将左值传递给接受右值引用的函数
- 优化性能，避免不必要的拷贝

示例：
string s1 = "Hello";
string s2 = std::move(s1);  // s1 的资源转移给 s2
// 此后 s1 处于有效但未定义状态，不应再使用
)" << endl;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

// 示例类：支持移动语义的动态数组
class MyArray {
private:
    int* data;      // 动态数组指针
    size_t size;    // 数组大小

public:
    // 构造函数
    MyArray(size_t n = 0) : data(nullptr), size(n) {
        if (n > 0) {
            data = new int[n];
            for (size_t i = 0; i < n; ++i) {
                data[i] = i;
            }
        }
        cout << "构造函数：创建大小为 " << size << " 的数组" << endl;
    }

    // 拷贝构造函数（深拷贝）
    MyArray(const MyArray& other) : data(nullptr), size(other.size) {
        if (size > 0) {
            data = new int[size];  // 分配新内存
            for (size_t i = 0; i < size; ++i) {
                data[i] = other.data[i];  // 复制每个元素
            }
        }
        cout << "拷贝构造：深拷贝 " << size << " 个元素（慢）" << endl;
    }

    // 移动构造函数 ⭐⭐⭐⭐⭐
    // 参数：右值引用 MyArray&&
    // 作用：转移资源所有权，不拷贝数据
    MyArray(MyArray&& other) noexcept : data(other.data), size(other.size) {
        // 1. 偷走源对象的资源指针
        // data = other.data;  // 已在初始化列表完成

        // 2. 将源对象置为安全状态（防止析构时释放资源）
        other.data = nullptr;
        other.size = 0;

        cout << "移动构造：转移 " << size << " 个元素的所有权（快）" << endl;
    }

    // 拷贝赋值运算符（深拷贝）
    MyArray& operator=(const MyArray& other) {
        if (this != &other) {  // 防止自赋值
            // 1. 释放旧资源
            delete[] data;

            // 2. 拷贝新资源
            size = other.size;
            if (size > 0) {
                data = new int[size];
                for (size_t i = 0; i < size; ++i) {
                    data[i] = other.data[i];
                }
            } else {
                data = nullptr;
            }
            cout << "拷贝赋值：深拷贝 " << size << " 个元素（慢）" << endl;
        }
        return *this;
    }

    // 移动赋值运算符 ⭐⭐⭐⭐⭐
    // 参数：右值引用 MyArray&&
    // 作用：转移资源所有权，不拷贝数据
    MyArray& operator=(MyArray&& other) noexcept {
        if (this != &other) {  // 防止自赋值
            // 1. 释放旧资源
            delete[] data;

            // 2. 偷走源对象的资源
            data = other.data;
            size = other.size;

            // 3. 将源对象置为安全状态
            other.data = nullptr;
            other.size = 0;

            cout << "移动赋值：转移 " << size << " 个元素的所有权（快）" << endl;
        }
        return *this;
    }

    // 析构函数
    ~MyArray() {
        delete[] data;
        cout << "析构函数：释放数组" << endl;
    }

    // 获取大小
    size_t getSize() const { return size; }

    // 打印数组
    void print() const {
        cout << "[";
        for (size_t i = 0; i < size; ++i) {
            cout << data[i];
            if (i < size - 1) cout << ", ";
        }
        cout << "]" << endl;
    }
};

void part2_practice() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 拷贝 vs 移动对比
    // --------------------------------------------

    cout << "\n2.1 拷贝 vs 移动对比" << endl;
    cout << string(60, '-') << endl;

    cout << "\n【拷贝构造】" << endl;
    MyArray arr1(5);  // 创建数组
    MyArray arr2 = arr1;  // 拷贝构造：深拷贝所有元素

    cout << "\n【移动构造】" << endl;
    MyArray arr3(5);  // 创建数组
    MyArray arr4 = std::move(arr3);  // 移动构造：转移所有权
    cout << "arr3 移动后大小: " << arr3.getSize() << " (资源已转移)" << endl;
    cout << "arr4 大小: " << arr4.getSize() << endl;

    // --------------------------------------------
    // 2.2 std::move 的使用
    // --------------------------------------------

    cout << "\n2.2 std::move 的使用" << endl;
    cout << string(60, '-') << endl;

    cout << "\n【拷贝赋值】" << endl;
    MyArray arr5(3);
    MyArray arr6(2);
    arr6 = arr5;  // 拷贝赋值：深拷贝

    cout << "\n【移动赋值】" << endl;
    MyArray arr7(3);
    MyArray arr8(2);
    arr8 = std::move(arr7);  // 移动赋值：转移所有权
    cout << "arr7 移动后大小: " << arr7.getSize() << " (资源已转移)" << endl;

    // --------------------------------------------
    // 2.3 返回值优化（RVO）
    // --------------------------------------------

    cout << "\n2.3 返回值优化" << endl;
    cout << string(60, '-') << endl;

    // 函数返回临时对象，编译器自动使用移动语义
    auto createArray = []() -> MyArray {
        MyArray temp(4);
        return temp;  // 返回时自动移动（或 RVO 优化）
    };

    cout << "\n调用返回 MyArray 的函数：" << endl;
    MyArray arr9 = createArray();  // 移动构造或 RVO
    cout << "arr9 大小: " << arr9.getSize() << endl;
}

/*
==============================================
🔍 深入理解：移动语义的底层原理（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【1. 为什么移动比拷贝快？】

拷贝构造（深拷贝）：
- 分配新内存：new int[size]
- 复制每个元素：循环 size 次
- 时间复杂度：O(n)

移动构造：
- 复制指针：data = other.data
- 置空源指针：other.data = nullptr
- 时间复杂度：O(1)

性能差异：
- 小对象（几个字节）：差异不大
- 大对象（MB/GB）：移动快几百倍甚至更多

【2. 移动后的状态】

"有效但未定义"是什么意思？
- 有效：对象仍然可以安全析构
- 未定义：对象的值不确定，不应再使用

示例：
string s1 = "Hello";
string s2 = std::move(s1);
// s1 现在可能是空字符串，也可能不是
// 唯一保证：s1 的析构函数可以安全调用

【3. noexcept 的重要性】

为什么移动函数要标记 noexcept？
- STL 容器（vector、deque）在扩容时：
  - 如果移动构造是 noexcept：使用移动
  - 否则：使用拷贝（保证异常安全）

示例：
vector<MyArray> vec;
vec.push_back(MyArray(1000));  // 如果移动构造是 noexcept，使用移动

【4. 五法则（Rule of Five）】

如果类管理资源，需要定义：
1. 析构函数
2. 拷贝构造函数
3. 拷贝赋值运算符
4. 移动构造函数
5. 移动赋值运算符

或者使用 = default / = delete 明确意图。

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void part4_common_mistakes() {
    cout << "\n" << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：移动后继续使用对象
    // --------------------------------------------

    cout << "\n4.1 错误1：移动后继续使用对象" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
string s1 = "Hello";
string s2 = std::move(s1);
cout << s1 << endl;  // 错误！s1 已被移动，值未定义

结果：可能输出空字符串，也可能崩溃

原因：移动后源对象处于"有效但未定义"状态

✅ 正确代码：
string s1 = "Hello";
string s2 = std::move(s1);
// 不再使用 s1，或者重新赋值
s1 = "World";  // 可以重新赋值
cout << s1 << endl;  // 现在可以使用

记忆技巧：
- std::move 后就当作对象"死了"
- 除非重新赋值，否则不要再使用
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：忘记将源对象置空
    // --------------------------------------------

    cout << "\n4.2 错误2：移动构造/赋值中忘记将源对象置空" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
MyArray(MyArray&& other) : data(other.data), size(other.size) {
    // 忘记将 other.data 置为 nullptr
}

结果：析构时 double free，程序崩溃

原因：
- 目标对象和源对象都持有同一个指针
- 两个对象析构时都会 delete[] data
- 第二次 delete 导致崩溃

✅ 正确代码：
MyArray(MyArray&& other) noexcept : data(other.data), size(other.size) {
    other.data = nullptr;  // 必须置空！
    other.size = 0;
}

记忆技巧：
- 移动 = 偷走资源 + 清空源对象
- 源对象必须处于可安全析构的状态
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：对 const 对象使用 std::move
    // --------------------------------------------

    cout << "\n4.3 错误3：对 const 对象使用 std::move" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
const string s1 = "Hello";
string s2 = std::move(s1);  // 实际调用拷贝构造！

结果：调用拷贝构造而不是移动构造

原因：
- const 对象不能被修改
- 移动构造需要修改源对象（置空）
- 编译器自动选择拷贝构造

✅ 正确代码：
string s1 = "Hello";  // 不要 const
string s2 = std::move(s1);  // 调用移动构造

记忆技巧：
- const 对象不能移动，只能拷贝
- 移动 = 修改源对象，const 禁止修改
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：移动赋值忘记检查自赋值
    // --------------------------------------------

    cout << "\n4.4 错误4：移动赋值忘记检查自赋值" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
MyArray& operator=(MyArray&& other) noexcept {
    delete[] data;  // 释放自己的资源
    data = other.data;  // 如果 other 就是 this，data 已被释放！
    other.data = nullptr;
    return *this;
}

结果：自赋值时访问已释放的内存

✅ 正确代码：
MyArray& operator=(MyArray&& other) noexcept {
    if (this != &other) {  // 检查自赋值
        delete[] data;
        data = other.data;
        other.data = nullptr;
    }
    return *this;
}

记忆技巧：
- 所有赋值运算符都要检查自赋值
- if (this != &other) 是标准写法
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：忘记标记 noexcept
    // --------------------------------------------

    cout << "\n4.5 错误5：移动函数忘记标记 noexcept" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
MyArray(MyArray&& other) {  // 没有 noexcept
    // ...
}

结果：STL 容器扩容时使用拷贝而不是移动

原因：
- vector 等容器扩容时需要异常安全保证
- 如果移动构造不是 noexcept，使用拷贝
- 性能损失巨大

✅ 正确代码：
MyArray(MyArray&& other) noexcept {  // 标记 noexcept
    // ...
}

记忆技巧：
- 移动构造和移动赋值必须标记 noexcept
- 否则 STL 容器不会使用移动语义
)" << endl;

    // --------------------------------------------
    // 4.6 错误6：返回局部变量的引用
    // --------------------------------------------

    cout << "\n4.6 错误6：返回局部变量的右值引用" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
MyArray&& createArray() {
    MyArray temp(10);
    return std::move(temp);  // 返回局部变量的引用！
}

结果：返回悬空引用，访问已销毁的对象

原因：
- temp 在函数结束时销毁
- 返回的引用指向已销毁的对象

✅ 正确代码：
MyArray createArray() {  // 按值返回
    MyArray temp(10);
    return temp;  // 编译器自动优化（RVO 或移动）
}

记忆技巧：
- 永远不要返回局部变量的引用
- 按值返回，编译器会自动优化
)" << endl;

    // --------------------------------------------
    // 4.7 错误7：过度使用 std::move
    // --------------------------------------------

    cout << "\n4.7 错误7：过度使用 std::move" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
MyArray createArray() {
    MyArray temp(10);
    return std::move(temp);  // 多余的 std::move
}

结果：可能阻止编译器的 RVO 优化

原因：
- 编译器会自动对返回值使用移动语义
- 显式 std::move 可能阻止 RVO（返回值优化）

✅ 正确代码：
MyArray createArray() {
    MyArray temp(10);
    return temp;  // 不需要 std::move
}

记忆技巧：
- 返回局部变量时不需要 std::move
- 编译器比你聪明，相信编译器
)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void part5_function_cards() {
    cout << "\n" << string(60, '=') << endl;
    cout << "函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【函数1：std::move()】

语法：template<typename T> T&& move(T&& t) noexcept;
头文件：<utility>
参数：
  - t (T&&): 要转换的对象（万能引用）
返回：右值引用

作用：将左值转换为右值引用，启用移动语义

示例：
string s1 = "Hello";
string s2 = std::move(s1);  // s1 的资源转移给 s2

────────────────────────────────────

【函数2：移动构造函数】

语法：ClassName(ClassName&& other) noexcept;
参数：
  - other (ClassName&&): 源对象的右值引用
返回：无（构造函数）

作用：转移资源所有权，避免深拷贝

实现要点：
1. 偷走源对象的资源指针
2. 将源对象置为安全状态（nullptr）
3. 标记 noexcept

示例：
MyArray(MyArray&& other) noexcept
    : data(other.data), size(other.size) {
    other.data = nullptr;
    other.size = 0;
}

────────────────────────────────────

【函数3：移动赋值运算符】

语法：ClassName& operator=(ClassName&& other) noexcept;
参数：
  - other (ClassName&&): 源对象的右值引用
返回：*this 的引用

作用：转移资源所有权，避免深拷贝

实现要点：
1. 检查自赋值
2. 释放旧资源
3. 偷走源对象的资源
4. 将源对象置为安全状态
5. 标记 noexcept

示例：
MyArray& operator=(MyArray&& other) noexcept {
    if (this != &other) {
        delete[] data;
        data = other.data;
        size = other.size;
        other.data = nullptr;
        other.size = 0;
    }
    return *this;
}
)" << endl;
}

// ============================================
// 第六部分：练习题
// ============================================

void part6_exercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现一个支持移动语义的 String 类
   要求：
   - 管理动态分配的字符数组
   - 实现拷贝构造、拷贝赋值
   - 实现移动构造、移动赋值
   - 所有函数都要有输出，观察调用情况
   提示：参考 MyArray 的实现

2. 分析以下代码的输出
   vector<MyArray> vec;
   vec.push_back(MyArray(100));
   vec.push_back(MyArray(200));
   vec.push_back(MyArray(300));
   提示：观察移动构造和拷贝构造的调用次数

3. 实现一个函数，接受 vector<string> 参数
   要求：
   - 提供两个版本：按值传递 vs 右值引用
   - 对比性能差异
   提示：使用 std::move 传递参数

4. 修复以下代码的问题
   class Buffer {
       char* data;
   public:
       Buffer(Buffer&& other) {
           data = other.data;
       }
   };
   提示：移动后源对象的状态

5. 什么时候应该使用 std::move？
   要求：列举至少 3 个使用场景和 3 个不应该使用的场景
   提示：考虑临时对象、返回值、容器操作

6. 实现一个 unique_ptr 的简化版本
   要求：
   - 只能移动，不能拷贝
   - 自动管理内存
   - 支持 -> 和 * 运算符
   提示：删除拷贝构造和拷贝赋值

7. 分析五法则（Rule of Five）
   要求：
   - 解释为什么需要五个函数
   - 什么情况下可以使用 = default
   - 什么情况下应该使用 = delete
   提示：考虑资源管理和异常安全
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "============================================" << endl;
    cout << "移动语义与右值引用 - 学习笔记" << endl;
    cout << "============================================" << endl;

    part1_concepts();
    part2_practice();
    part4_common_mistakes();
    part5_function_cards();
    part6_exercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "学习完成！" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【总结】

核心概念：
✅ 左值有名字可取地址，右值是临时对象
✅ 右值引用（&&）绑定到右值
✅ 移动语义转移资源所有权，避免深拷贝
✅ std::move 将左值转换为右值引用

实现要点：
✅ 移动构造：偷走资源 + 置空源对象
✅ 移动赋值：检查自赋值 + 释放旧资源 + 偷走资源
✅ 必须标记 noexcept
✅ 源对象必须处于可安全析构的状态

使用场景：
✅ 返回大对象时自动使用移动
✅ 容器操作（push_back、insert）
✅ 明确不再需要对象的值时使用 std::move

注意事项：
⚠️ 移动后不要再使用源对象
⚠️ const 对象不能移动
⚠️ 返回局部变量不需要 std::move
⚠️ 不要过度使用 std::move

下一步：
→ 学习现代 C++ 语法糖（auto、范围 for）
→ 学习完美转发
→ 学习 emplace 系列函数
)" << endl;

    return 0;
}

