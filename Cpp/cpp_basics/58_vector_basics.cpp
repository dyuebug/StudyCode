// ============================================
// 58. vector 基础 - 超详细注释版
// 日期：2026-05-04（第18天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】

什么是 vector？
- C++ STL 中最常用的容器，本质是"动态数组"
- 自动管理内存，自动扩容，使用完自动释放

为什么需要 vector？
- C 数组大小固定，不能扩容；vector 可以自动扩容
- C 数组不方便传参和赋值；vector 可以直接赋值和传参
- vector 提供了大量有用的成员函数

核心特点：
- 连续内存存储（和数组一样，支持下标访问）
- 末尾添加/删除元素效率高（O(1)）
- 中间插入/删除效率低（O(n)，需要移动后面的元素）
- 支持随机访问（可以用 [] 和 at() 按下标访问）

本文件重点：掌握 vector 的创建、添加、访问、遍历、常用操作
*/

#include <iostream>
#include <vector>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 vector 与数组的对比
    // --------------------------------------------

    cout << "1.1 vector 与数组的对比" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【对比：数组 vs vector】

特性            C 数组              vector
─────────────────────────────────────────────────
大小            固定，声明时确定     动态，自动扩容
内存            栈/静态区           堆（自动管理）
下标访问        支持                支持（[] 和 at()）
遍历            for 循环            范围 for + 下标循环
赋值            不支持              支持（v2 = v1）
传参            退化为指针          直接传对象
获取大小        需要额外变量        .size() 一键获取
添加元素        不可能              .push_back()
)"" << endl;

    // --------------------------------------------
    // 1.2 创建 vector 的几种方式
    // --------------------------------------------

    cout << "1.2 创建 vector 的几种方式" << endl;
    cout << string(60, '-') << endl;

    // 方式1：空 vector
    vector<int> v1;  // 空的 int 类型 vector，没有元素

    // 方式2：指定初始大小
    vector<int> v2(5);  // 5 个元素，每个默认值为 0
    // v2 = {0, 0, 0, 0, 0}

    // 方式3：指定大小和初始值
    vector<int> v3(5, 100);  // 5 个元素，每个值为 100
    // v3 = {100, 100, 100, 100, 100}

    // 方式4：用初始化列表（C++11）
    vector<int> v4 = {10, 20, 30, 40, 50};

    // 方式5：用另一个 vector 初始化（拷贝构造）
    vector<int> v5(v4);  // v5 是 v4 的拷贝

    // 打印每种方式的结果
    cout << "v1（空）：" << (v1.empty() ? "空" : "非空") << endl;
    cout << "v2（5个0）：";
    for (int val : v2) cout << val << " ";  // 0 0 0 0 0
    cout << endl;

    cout << "v3（5个100）：";
    for (int val : v3) cout << val << " ";  // 100 100 100 100 100
    cout << endl;

    cout << "v4（初始化列表）：";
    for (int val : v4) cout << val << " ";  // 10 20 30 40 50
    cout << endl;

    cout << "v5（拷贝v4）：";
    for (int val : v5) cout << val << " ";  // 10 20 30 40 50
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 1.3 重要函数讲解 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "1.3 重要函数讲解" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【函数卡片：push_back()】

函数名：push_back()
作用：在 vector 末尾添加一个元素
类型：vector 成员函数

完整语法：
void push_back(const T& value);

参数详解：
1. value（必需）
   - 类型：T（vector 元素类型的引用）
   - 作用：要添加到末尾的值
   - 示例：v.push_back(10)  // 添加整数 10

返回值：无

使用示例：
vector<int> v;
v.push_back(10);  // v = {10}
v.push_back(20);  // v = {20}  → v = {10, 20}
v.push_back(30);  // v = {30}  → v = {10, 20, 30}

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：size()】

函数名：size()
作用：返回 vector 中元素的个数
类型：vector 成员函数

完整语法：
size_type size() const;

参数：无

返回值：
- 类型：size_type（通常等价于 size_t，无符号整数）
- 含义：元素个数
- 示例：v 有 3 个元素，v.size() 返回 3

使用示例：
vector<int> v = {1, 2, 3};
cout << v.size();  // 输出 3

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：at()】

函数名：at()
作用：访问指定位置的元素（带边界检查）
类型：vector 成员函数

完整语法：
reference at(size_type pos);
const_reference at(size_type pos) const;

参数详解：
1. pos（必需）
   - 类型：size_type（无符号整数）
   - 作用：要访问的元素位置（从 0 开始）
   - 示例：v.at(0) 访问第一个元素

返回值：
- 类型：元素的引用
- 含义：指定位置的元素
- 越界时：抛出 out_of_range 异常

使用示例：
vector<int> v = {10, 20, 30};
cout << v.at(0);   // 输出 10
cout << v.at(2);   // 输出 30
// cout << v.at(5);  // 抛出异常！

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：pop_back()】

函数名：pop_back()
作用：移除 vector 末尾的元素
类型：vector 成员函数

完整语法：
void pop_back();

参数：无

返回值：无

注意：
- 调用前必须确保 vector 不为空！
- 空容器调用 pop_back() 是未定义行为

使用示例：
vector<int> v = {10, 20, 30};
v.pop_back();  // v = {10, 20}
v.pop_back();  // v = {10}
)"" << endl;

    return 0;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

int main_practice() {
    cout << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 添加和删除元素
    // --------------------------------------------

    cout << "2.1 添加和删除元素" << endl;
    cout << string(60, '-') << endl;

    // 创建空 vector
    vector<int> nums;
    cout << "初始状态：元素数 = " << nums.size() << endl;  // 0

    // push_back：末尾添加
    nums.push_back(10);  // nums = {10}
    nums.push_back(20);  // nums = {20}  → nums = {10, 20}
    nums.push_back(30);  // nums = {30}  → nums = {10, 20, 30}
    cout << "添加后：元素数 = " << nums.size() << endl;  // 3

    // pop_back：末尾删除
    nums.pop_back();  // 移除 30，nums = {10, 20}
    cout << "删除后：元素数 = " << nums.size() << endl;  // 2

    cout << endl;

    // --------------------------------------------
    // 2.2 访问元素
    // --------------------------------------------

    cout << "2.2 访问元素" << endl;
    cout << string(60, '-') << endl;

    vector<string> fruits = {"apple", "banana", "cherry"};

    // 方式1：用 [] 访问（不检查边界）
    cout << "第一个水果：" << fruits[0] << endl;  // apple
    cout << "第二个水果：" << fruits[1] << endl;  // banana

    // 方式2：用 at() 访问（检查边界，更安全）
    cout << "第三个水果：" << fruits.at(2) << endl;  // cherry

    // 方式3：front() 和 back() 访问首尾元素
    cout << "第一个：" << fruits.front() << endl;  // apple
    cout << "最后一个：" << fruits.back() << endl;  // cherry

    cout << endl;

    // --------------------------------------------
    // 2.3 遍历 vector
    // --------------------------------------------

    cout << "2.3 遍历 vector" << endl;
    cout << string(60, '-') << endl;

    vector<double> scores = {85.5, 90.0, 78.5, 92.5, 88.0};

    // 方式1：下标循环（和数组一样）
    cout << "方式1（下标循环）：";
    for (size_t i = 0; i < scores.size(); i++) {  // size_t 是 size() 返回的类型
        cout << scores[i] << " ";
    }
    cout << endl;

    // 方式2：范围 for（推荐！最简洁）
    cout << "方式2（范围 for）：";
    for (double score : scores) {
        cout << score << " ";
    }
    cout << endl;

    // 方式3：范围 for + 引用（需要修改元素时用）
    cout << "方式3（范围 for + 引用，每人加5分）：";
    for (double& score : scores) {  // 注意 & 引用，才能修改原值
        score += 5;
        cout << score << " ";
    }
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 2.4 其他常用操作
    // --------------------------------------------

    cout << "2.4 其他常用操作" << endl;
    cout << string(60, '-') << endl;

    vector<int> data = {5, 3, 8, 1, 9};

    // empty：检查是否为空
    cout << "是否为空：" << (data.empty() ? "是" : "否") << endl;  // 否

    // clear：清空所有元素
    vector<int> temp = {1, 2, 3};
    cout << "清空前：" << temp.size() << " 个元素" << endl;  // 3
    temp.clear();
    cout << "清空后：" << temp.size() << " 个元素" << endl;  // 0

    // resize：改变大小
    vector<int> v = {1, 2, 3};
    v.resize(5);  // 扩大到 5，新元素默认为 0
    cout << "resize(5) 后：";
    for (int val : v) cout << val << " ";  // 1 2 3 0 0
    cout << endl;

    v.resize(2);  // 缩小到 2，多余的元素被移除
    cout << "resize(2) 后：";
    for (int val : v) cout << val << " ";  // 1 2
    cout << endl;

    // 赋值操作（vector 独有的优势，数组做不到）
    vector<int> a = {1, 2, 3};
    vector<int> b;
    b = a;  // 直接赋值！b 现在也是 {1, 2, 3}
    cout << "赋值后 b：";
    for (int val : b) cout << val << " ";
    cout << endl;

    // 比较（按字典序逐元素比较）
    vector<int> x = {1, 2, 3};
    vector<int> y = {1, 2, 4};
    cout << "x == y？" << (x == y ? "是" : "否") << endl;   // 否
    cout << "x < y？" << (x < y ? "是" : "否") << endl;     // 是（3 < 4）

    return 0;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【🔍 深入理解1：vector 的内存模型】

vector 内部有 3 个关键值：

  size：     实际存储的元素个数
  capacity： 已分配的内存能容纳的元素个数
  data：     指向底层数组的指针

关系：size <= capacity

当你 push_back 时：
  if (size < capacity)：
    → 直接在末尾放置元素，size++
    → O(1) 操作

  if (size == capacity)：
    → 需要扩容！
    → 1. 分配新的更大内存（通常 2 倍）
    → 2. 把旧元素复制过去
    → 3. 释放旧内存
    → 4. 放置新元素
    → 这次操作 O(n)，但平摊 O(1)

示例：
  v = {10, 20}, capacity = 2
  v.push_back(30)  → 触发扩容
  → 新 capacity = 4
  → v = {10, 20, 30}, capacity = 4, size = 3
  → 还能再 push_back 一次不扩容

这就是为什么 vector "末尾添加"效率高：
大多数时候 O(1)，偶尔 O(n) 但平摊后还是 O(1)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【🔍 深入理解2：vector 中间插入为什么慢？】

vector 内存是连续的：
  [10][20][30][40][50]

如果在位置 2 插入 99：
  [10][20][99] ← 要把 30, 40, 50 都往后移一位
  [10][20][99][30][40][50]

移动 3 个元素 → O(n) 操作

这就是为什么：
- 末尾操作（push_back）快：O(1)
- 中间操作（insert）慢：O(n)
- 需要"频繁在中间插入"时，用 list 更合适
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

int main_errors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：用 size_t 做递减循环导致下溢
    // --------------------------------------------
    cout << "4.1 错误1：size_t 下溢" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v = {1, 2, 3};
for (size_t i = v.size() - 1; i >= 0; i--) {
    cout << v[i];  // 无限循环！
}

原因：size_t 是无符号类型，永远不会小于 0
      当 i = 0 时，i-- 变成一个极大的正数
      循环永远不停止

✅ 正确代码：
// 方法1：用 int 代替 size_t
for (int i = v.size() - 1; i >= 0; i--) {
    cout << v[i];
}

// 方法2：正向遍历 + 反向迭代器
for (auto it = v.rbegin(); it != v.rend(); it++) {
    cout << *it;
}

记忆技巧：
- size_t 不可用于递减到 0 的循环
- 需要反向遍历时，用反向迭代器或 int 类型
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：空 vector 调用 pop_back / back / front
    // --------------------------------------------
    cout << "4.2 错误2：空 vector 调用 pop_back / back / front" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v;      // 空的！
v.pop_back();       // 未定义行为！
cout << v.front();  // 未定义行为！
cout << v.back();   // 未定义行为！

原因：空 vector 没有元素，这些函数要求至少有 1 个元素

✅ 正确代码：
vector<int> v;
if (!v.empty()) {
    v.pop_back();
    cout << v.front();
    cout << v.back();
}

记忆技巧：
- 调用 pop_back / front / back 前先检查 empty()
- [] 和 at() 同理，要先确认下标有效
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：resize vs reserve 混淆
    // --------------------------------------------
    cout << "4.3 错误3：resize vs reserve 混淆" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v;
v.reserve(5);       // 只分配内存，不创建元素！
cout << v[0];       // 越界！size 仍然是 0

原因：reserve 只影响 capacity，不影响 size
      v.size() 仍然是 0，没有元素可以访问

✅ 正确区分：
  resize(n)   — 改变 size，创建/删除元素
  reserve(n)  — 改变 capacity，只预分配内存

vector<int> v;
v.resize(5);        // size = 5，有 5 个元素（值为 0）
cout << v[0];       // 安全，输出 0

何时用 reserve？
- 你知道大概要存多少元素
- 提前 reserve 可以避免多次扩容
- 纯粹的优化手段，不影响逻辑

记忆技巧：
- resize = "调整元素数量"（改 size）
- reserve = "预留空间"（改 capacity）
- reserve 后 size 不变，不能直接访问元素
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：范围 for 中修改容器结构
    // --------------------------------------------
    cout << "4.4 错误4：范围 for 中修改容器结构" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v = {1, 2, 3, 4, 5};
for (int val : v) {
    if (val == 3) {
        v.erase(v.begin() + 2);  // 遍历中删除元素！
    }
}

原因：删除元素后，后面的元素前移，迭代器失效
      可能跳过元素或崩溃

✅ 正确代码：
// 方法：用 erase 的返回值
for (auto it = v.begin(); it != v.end(); ) {
    if (*it == 3) {
        it = v.erase(it);  // erase 返回下一个有效迭代器
    } else {
        it++;  // 只在不删除时递增
    }
}

记忆技巧：
- 范围 for 只适合"只读遍历"或"修改值"
- 需要增删元素时，用手动迭代器循环
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：忘了 const 引用导致不必要的拷贝
    // --------------------------------------------
    cout << "4.5 错误5：忘了 const 引用导致不必要的拷贝" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 低效代码：
vector<string> names = {"Alice", "Bob", "Charlie"};
for (string name : names) {  // 每次循环都拷贝一个 string！
    cout << name << " ";
}

原因：范围 for 中不用引用，每次都会拷贝元素
      对于大型对象（如 string），拷贝开销大

✅ 高效代码：
for (const string& name : names) {  // const 引用，零拷贝
    cout << name << " ";
}

规则：
- 只读遍历 → const 引用（const T&）
- 需要修改 → 非常引用（T&）
- 小类型（int、double）→ 直接用值也可以，影响不大

记忆技巧：
- 遍历对象时，默认写 const 引用
- int/double 等小类型，直接用值也行
)" << endl;

    // --------------------------------------------
    // 4.6 错误6：vector 存不同类型的数据
    // --------------------------------------------
    cout << "4.6 错误6：vector 存不同类型的数据" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v;
v.push_back(10);
v.push_back("hello");  // 编译错误！类型不匹配

原因：vector<int> 只能存 int，不能混存其他类型
      vector 是强类型的，声明后类型就固定了

✅ 正确做法：
// 如果要存字符串，用 vector<string>
vector<string> v2;
v2.push_back("hello");

// 如果真的需要混合类型，后续会学习模板和继承
// 现在阶段：一个 vector 只存一种类型
)" << endl;

    // --------------------------------------------
    // 4.7 错误7：push_back 与 initializer_list 混淆
    // --------------------------------------------
    cout << "4.7 错误7：push_back 与 initializer_list 混淆" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<vector<int>> matrix;
matrix.push_back(1, 2, 3);  // 编译错误！push_back 只接受一个参数

原因：push_back 每次只能添加一个元素
      要添加多个元素，需要不同的方式

✅ 正确代码：
// 方法1：多次 push_back
vector<vector<int>> matrix;
matrix.push_back({1, 2, 3});  // push_back 一个 vector
matrix.push_back({4, 5, 6});

// 方法2：用初始化列表整体创建
vector<vector<int>> matrix2 = {
    {1, 2, 3},
    {4, 5, 6}
};

记忆技巧：
- push_back 一次只能加一个元素
- 要加一组，用花括号 {} 包裹
- 二维 vector = vector 的 vector
)" << endl;

    return 0;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

int main_reference() {
    cout << R"(
============================================
vector 函数卡片速查
============================================

【构造函数】

vector<T> v;              — 空向量
vector<T> v(n);            — n 个默认值元素
vector<T> v(n, val);       — n 个 val 值元素
vector<T> v = {a, b, c};   — 初始化列表
vector<T> v(other);        — 拷贝构造

【元素访问】

v[i]        — 访问第 i 个元素（不检查边界）
v.at(i)     — 访问第 i 个元素（检查边界）
v.front()   — 访问第一个元素
v.back()    — 访问最后一个元素

【容量相关】

v.size()       — 返回元素个数
v.empty()      — 是否为空
v.capacity()   — 已分配空间大小
v.reserve(n)   — 预留 n 个元素的空间
v.shrink_to_fit() — 释放多余空间

【修改操作】

v.push_back(val)  — 末尾添加元素
v.pop_back()      — 末尾删除元素
v.clear()         — 清空所有元素
v.resize(n)       — 改变大小为 n
v.resize(n, val)  — 改变大小为 n，新增元素用 val 填充
v.assign(n, val)  — 重新赋值为 n 个 val

【遍历方式】

// 方式1：下标循环
for (size_t i = 0; i < v.size(); i++) {
    cout << v[i];
}

// 方式2：范围 for（只读）
for (const T& elem : v) {
    cout << elem;
}

// 方式3：范围 for（修改）
for (T& elem : v) {
    elem *= 2;  // 修改原值
}
)"" << endl;

    return 0;
}

// ============================================
// 第六部分：练习题
// ============================================

int main_exercises() {
    cout << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 基础操作
   创建一个 vector<double>，添加 5 个成绩（85.5, 90.0, 78.5, 92.5, 88.0），
   然后计算平均分并输出。
   提示：遍历累加，除以 size()

2. 动态输入
   从键盘输入 5 个整数存入 vector，然后输出最大值和最小值。
   提示：声明一个临时变量，push_back 添加，遍历比较

3. 删除操作
   给定 vector<int> v = {10, 20, 30, 40, 50}，
   如何删除第三个元素（值为 30）？
   提示：想想如何用迭代器指定位置

4. 思考题
   如果你知道 vector 最终大约会有 1000 个元素，
   有什么方法可以优化性能？
   提示：想想扩容的开销和 reserve 的作用

5. 综合练习
   创建一个 vector<string> 存储待办事项列表，实现：
   - 添加一条待办
   - 显示所有待办
   - 删除最后一条（完成）
   - 检查是否还有未完成的事项
   提示：push_back、范围 for、pop_back、empty
)" << endl;

    return 0;
}

// 编译命令：g++ -std=c++11 -o 58_vector_basics 58_vector_basics.cpp
// 运行方式：./58_vector_basics 或 58_vector_basics.exe
// C++版本：C++11（初始化列表和范围 for 需要）
