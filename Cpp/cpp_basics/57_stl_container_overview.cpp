// ============================================
// 57. STL 容器总览 - 超详细注释版
// 日期：2026-05-04（第18天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】

什么是 STL？
- Standard Template Library（标准模板库），C++ 自带的强大工具集

为什么需要 STL？
- 不用手写数组管理、不用手写链表、不用手写查找算法
- STL 提供了经过充分测试、高度优化的通用数据结构和算法

核心特点：
- 容器：存放数据的"盒子"（vector、list、map、set 等）
- 迭代器：访问容器元素的"指针"（后续详细学）
- 算法：对容器中元素进行操作的函数（排序、查找等）
- 函数对象：可以像函数一样使用的对象（后续详细学）

本文件重点：理解容器的分类和各自的特点，建立"选容器"的思维框架
*/

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <set>
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
    // 1.1 STL 是什么？为什么需要它？
    // --------------------------------------------

    cout << "1.1 STL 是什么？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【STL = Standard Template Library】

想象你去超市买东西：
- 不用 STL：你得自己种菜、养鸡、挤奶 —— 一切从头来
- 用 STL：超市已经帮你准备好了，你需要什么直接拿

C++ 中没有 STL 时：
- 想用一个能自动扩容的数组？自己写
- 想用一个能按键查找的字典？自己写
- 想对一组数据排序？自己写排序算法

有了 STL：
- vector：自动扩容的数组
- map：按键查找的字典
- sort()：一行代码排序

STL 三大组成部分：
1. 容器（Container）：存放数据的结构
2. 算法（Algorithm）：操作数据的函数
3. 迭代器（Iterator）：连接容器和算法的"桥梁"
)" << endl;

    // --------------------------------------------
    // 1.2 容器的分类 ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "1.2 容器的分类" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【容器分类 — 记忆框架】

          STL 容器
          /       \
    顺序容器      关联容器
    (按位置存)    (按键值存)
      |            /    \
    vector     有序      无序
    list      (红黑树)  (哈希表)
    deque      |          |
             map/set   unordered_map/unordered_set

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

顺序容器（Sequence Container）：
- 元素按存储位置排列，跟插入顺序有关
- 适合：需要按位置访问元素的场景
- 代表：vector、list、deque

关联容器（Associative Container）：
- 元素按"键"排序存储，跟插入顺序无关
- 适合：需要快速查找的场景
- 有序代表：map、set（内部用红黑树，元素自动排序）
- 无序代表：unordered_map、unordered_set（内部用哈希表）
)" << endl;

    // --------------------------------------------
    // 1.3 如何选择容器？⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "1.3 如何选择容器？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【选容器决策树】

你的数据需要什么操作？

→ 主要按位置访问（下标访问）？
   → vector ✅ 最常用

→ 主要在头部或中间频繁插入/删除？
   → list

→ 需要按"键"快速查找值？
   → map（字典：键→值）

→ 需要存储不重复的元素？
   → set（集合：自动去重）

→ 都不确定？
   → 先选 vector，90% 场景够用

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

记忆口诀：
- vector = 动态数组，用得最多
- list = 链表，插入删除快
- map = 字典，按键找值
- set = 集合，元素不重复
)" << endl;

    return 0;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

// 本节用最简单的代码让你"看到"四种容器的样子

int main_practice() {
    cout << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 vector：动态数组
    // --------------------------------------------

    cout << "2.1 vector — 动态数组" << endl;
    cout << string(60, '-') << endl;

    // 创建一个空的 int 类型 vector
    vector<int> nums;

    // push_back：在末尾添加元素
    nums.push_back(10);  // 现在 nums = {10}
    nums.push_back(20);  // 现在 nums = {10, 20}
    nums.push_back(30);  // 现在 nums = {10, 20, 30}

    // 用下标访问元素（和数组一样）
    cout << "第一个元素：" << nums[0] << endl;  // 输出 10
    cout << "第二个元素：" << nums[1] << endl;  // 输出 20

    // size：获取元素个数
    cout << "元素个数：" << nums.size() << endl;  // 输出 3

    // 遍历 vector（用下标循环，和数组一样）
    cout << "遍历：";
    for (int i = 0; i < nums.size(); i++) {
        cout << nums[i] << " ";  // 输出 10 20 30
    }
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 2.2 list：链表
    // --------------------------------------------

    cout << "2.2 list — 链表" << endl;
    cout << string(60, '-') << endl;

    // 创建一个 string 类型的 list
    list<string> names;

    // push_back：在末尾添加
    names.push_back("Alice");

    // push_front：在头部添加（vector 做不到！）
    names.push_front("Bob");

    // 现在 list 中：Bob → Alice

    // 遍历 list（不能用下标！只能用迭代器或范围 for）
    cout << "遍历：";
    for (const string& name : names) {
        cout << name << " ";  // 输出 Bob Alice
    }
    cout << endl;

    // size：获取元素个数
    cout << "元素个数：" << names.size() << endl;  // 输出 2

    cout << endl;

    // --------------------------------------------
    // 2.3 map：字典（键→值映射）
    // --------------------------------------------

    cout << "2.3 map — 字典" << endl;
    cout << string(60, '-') << endl;

    // 创建一个 键:string → 值:int 的 map
    map<string, int> ages;

    // 用 [] 操作符插入键值对
    ages["Alice"] = 25;  // 键 "Alice" 对应值 25
    ages["Bob"] = 30;    // 键 "Bob" 对应值 30

    // 用 [] 操作符按键查找值
    cout << "Alice 的年龄：" << ages["Alice"] << endl;  // 输出 25
    cout << "Bob 的年龄：" << ages["Bob"] << endl;      // 输出 30

    // 遍历 map（元素按键自动排序）
    cout << "遍历：";
    for (const auto& pair : ages) {
        // pair.first 是键，pair.second 是值
        cout << pair.first << "=" << pair.second << " ";
        // 输出 Alice=25 Bob=30（按键排序）
    }
    cout << endl;

    // count：检查某个键是否存在
    cout << "有 Alice 吗？" << (ages.count("Alice") ? "有" : "没有") << endl;  // 有
    cout << "有 Charlie 吗？" << (ages.count("Charlie") ? "有" : "没有") << endl;  // 没有

    cout << endl;

    // --------------------------------------------
    // 2.4 set：集合（不重复元素）
    // --------------------------------------------

    cout << "2.4 set — 集合" << endl;
    cout << string(60, '-') << endl;

    // 创建一个 int 类型的 set
    set<int> unique_nums;

    // insert：插入元素（重复的会被自动忽略）
    unique_nums.insert(10);
    unique_nums.insert(20);
    unique_nums.insert(10);  // 重复插入，被忽略
    unique_nums.insert(30);
    unique_nums.insert(20);  // 重复插入，被忽略

    // 遍历 set（元素自动排序）
    cout << "遍历：";
    for (int num : unique_nums) {
        cout << num << " ";  // 输出 10 20 30（已排序、无重复）
    }
    cout << endl;

    // size：获取元素个数
    cout << "元素个数：" << unique_nums.size() << endl;  // 输出 3（不是5）

    // count：检查某个值是否存在
    cout << "有 20 吗？" << (unique_nums.count(20) ? "有" : "没有") << endl;  // 有
    cout << "有 40 吗？" << (unique_nums.count(40) ? "有" : "没有") << endl;  // 没有

    return 0;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【🔍 深入理解1：为什么 vector 比数组好？】

C 数组 vs vector：

        C 数组               vector
大小：  固定，不能变         自动扩容
越界：  不检查，直接崩溃     at() 可以检查
传递：  退化为指针，丢信息   可以直接传对象
赋值：  不能直接赋值         可以直接赋值

vector 底层原理：
- 内部维护一个动态分配的数组
- 当空间不够时，重新分配一块更大的内存
- 把旧数据复制过去，释放旧内存
- 这就是"扩容"操作

扩容策略：
- 通常按 2 倍增长（capacity 从 1→2→4→8→16...）
- 扩容时有复制开销，但平摊下来每个操作 O(1)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【🔍 深入理解2：map 的底层 — 红黑树】

为什么 map 查找快？
- map 内部用红黑树（一种自平衡二叉搜索树）
- 查找时间：O(log n) — 即使 100 万个元素，最多 ~20 次比较
- 元素自动按键排序 — 遍历时是有序的

红黑树特点：
- 每个节点是红色或黑色
- 通过颜色规则保持树平衡
- 插入/删除时自动旋转调整

unordered_map vs map：
- unordered_map 用哈希表，查找 O(1)，但不排序
- map 用红黑树，查找 O(log n)，但自动排序
- 需要排序用 map，只需查找用 unordered_map
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

int main_errors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：用 [] 访问 vector 越界
    // --------------------------------------------
    cout << "4.1 错误1：用 [] 访问 vector 越界" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v = {1, 2, 3};
cout << v[5];  // 越界！[] 不检查边界，行为未定义

结果：可能崩溃，可能输出垃圾值，可能看起来正常

✅ 正确代码：
vector<int> v = {1, 2, 3};
if (v.size() > 5) {
    cout << v[5];
} else {
    cout << "索引越界！";
}

或者用 at()，它会检查边界：
cout << v.at(5);  // 抛出 out_of_range 异常

记忆技巧：
- [] = 快但危险（不检查）
- at() = 慢但安全（会检查）
- 遍历时用范围 for，避免手动管理下标
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：用 [] 访问 map 时无意插入
    // --------------------------------------------
    cout << "4.2 错误2：用 [] 访问 map 时无意插入" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
map<string, int> ages;
ages["Alice"] = 25;

// 想检查 Bob 是否存在
if (ages["Bob"] == 0) {  // 问题！这里会自动插入 "Bob"
    cout << "Bob 不存在" << endl;
}

// 现在 ages 里有两个元素：Alice 和 Bob！
cout << ages.size();  // 输出 2，而不是预期的 1

原因：map 的 [] 操作符在键不存在时会自动创建一个
       默认值的元素并插入

✅ 正确代码：
map<string, int> ages;
ages["Alice"] = 25;

// 用 count 或 find 检查键是否存在
if (ages.count("Bob") == 0) {
    cout << "Bob 不存在" << endl;
}

// 或用 find
auto it = ages.find("Bob");
if (it == ages.end()) {
    cout << "Bob 不存在" << endl;
}

记忆技巧：
- 检查键是否存在 → 用 count() 或 find()
- 读取值 → 确认键存在后再用 [] 或 at()
- [] 会自动插入 → 这是最大的陷阱！
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：误以为 list 可以用下标访问
    // --------------------------------------------
    cout << "4.3 错误3：误以为 list 可以用下标访问" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
list<int> lst = {10, 20, 30};
cout << lst[0];  // 编译错误！list 不支持 []

原因：list 是链表，每个节点只知道前一个和后一个节点
       要找第 0 个元素，必须从头开始走
       所以 list 不提供 [] 操作

✅ 正确代码：
list<int> lst = {10, 20, 30};

// 遍历用范围 for
for (int val : lst) {
    cout << val << " ";
}

// 如果真的需要按下标访问，就不该用 list，改用 vector
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：混淆 map 和 set 的插入方式
    // --------------------------------------------
    cout << "4.4 错误4：混淆 map 和 set 的插入方式" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
set<int> s;
s[0] = 10;  // 编译错误！set 没有 [] 操作符

原因：set 存的是"值"本身，不是"键值对"
       只有 map 才有"键→值"映射，才有 []

✅ 正确代码：
set<int> s;
s.insert(10);  // set 用 insert 插入值

map<string, int> m;
m["Alice"] = 25;  // map 用 [] 插入键值对
m.insert({"Bob", 30});  // 或用 insert
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：忘记 #include 对应的头文件
    // --------------------------------------------
    cout << "4.5 错误5：忘记 #include 对应的头文件" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
#include <iostream>
using namespace std;

int main() {
    vector<int> v;  // 编译错误！没有 #include <vector>
}

原因：每种容器都有对应的头文件，必须包含

✅ 正确代码：
#include <iostream>
#include <vector>     // vector 的头文件
#include <list>       // list 的头文件
#include <map>        // map 的头文件
#include <set>        // set 的头文件
using namespace std;

对应关系：
  容器          头文件
  vector    →  <vector>
  list      →  <list>
  map       →  <map>
  set       →  <set>
  string    →  <string>
  unordered_map → <unordered_map>
  unordered_set → <unordered_set>

记忆技巧：容器名去掉下划线部分就是头文件名
)" << endl;

    // --------------------------------------------
    // 4.6 错误6：vector 扩容时迭代器失效
    // --------------------------------------------
    cout << "4.6 错误6：vector 扩容时迭代器失效" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v = {1, 2, 3};
auto it = v.begin();  // it 指向第一个元素
v.push_back(4);       // 可能触发扩容
cout << *it;           // 未定义行为！it 可能已失效

原因：vector 扩容时会重新分配内存
       原来指向旧内存的迭代器就"失效"了
       解引用失效的迭代器是未定义行为

✅ 正确代码：
vector<int> v = {1, 2, 3};
v.push_back(4);
auto it = v.begin();  // push_back 之后再获取迭代器
cout << *it;          // 安全

记忆技巧：
- push_back 可能让 vector 扩容 → 迭代器失效
- 修改容器后，不要再用旧的迭代器
- 重新获取迭代器即可
)" << endl;

    // --------------------------------------------
    // 4.7 错误7：在遍历容器时修改容器
    // --------------------------------------------
    cout << "4.7 错误7：在遍历容器时修改容器" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v = {1, 2, 3, 4, 5};
for (int val : v) {
    if (val == 3) {
        v.push_back(6);  // 遍历中修改容器！未定义行为
    }
}

原因：遍历是基于迭代器的，修改容器可能改变内部结构
       导致迭代器失效或跳过元素

✅ 正确代码：
vector<int> v = {1, 2, 3, 4, 5};
// 先收集要添加的，遍历结束后再添加
vector<int> to_add;
for (int val : v) {
    if (val == 3) {
        to_add.push_back(6);
    }
}
for (int val : to_add) {
    v.push_back(val);
}

记忆技巧：
- 遍历 = 只读操作
- 想修改？先收集，遍历结束后再修改
)" << endl;

    return 0;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

int main_reference() {
    cout << R"(
============================================
函数卡片速查
============================================

【通用成员函数 — 所有容器都有】

size()
  语法：size_type size() const
  作用：返回容器中元素的个数
  返回：无符号整数（size_type）
  示例：cout << v.size();

empty()
  语法：bool empty() const
  作用：检查容器是否为空
  返回：true（空）/ false（非空）
  示例：if (v.empty()) cout << "空";

clear()
  语法：void clear()
  作用：移除所有元素
  返回：无
  示例：v.clear();

────────────────────────────────────

【vector 专属函数】

push_back(val)
  语法：void push_back(const T& val)
  作用：在末尾添加一个元素
  参数：val — 要添加的值
  示例：v.push_back(10);

pop_back()
  语法：void pop_back()
  作用：移除末尾元素（前提：不为空！）
  示例：v.pop_back();

at(index)
  语法：reference at(size_type index)
  作用：访问指定位置的元素（带边界检查）
  参数：index — 元素位置（从 0 开始）
  返回：元素的引用
  示例：cout << v.at(0);

capacity()
  语法：size_type capacity() const
  作用：返回当前已分配的存储空间大小
  示例：cout << v.capacity();

────────────────────────────────────

【list 专属函数】

push_front(val)
  语法：void push_front(const T& val)
  作用：在头部添加一个元素
  参数：val — 要添加的值
  示例：lst.push_front(10);

push_back(val)
  语法：void push_back(const T& val)
  作用：在末尾添加一个元素
  示例：lst.push_back(20);

pop_front()
  语法：void pop_front()
  作用：移除头部元素（前提：不为空！）

────────────────────────────────────

【map 专属函数】

operator[](key)
  语法：T& operator[](const Key& key)
  作用：访问或插入指定键的值
  注意：如果键不存在，会自动插入默认值！
  示例：m["Alice"] = 25;

insert(pair)
  语法：pair<iterator, bool> insert(const value_type& val)
  作用：插入键值对
  返回：插入位置迭代器 + 是否成功
  示例：m.insert({"Bob", 30});

count(key)
  语法：size_type count(const Key& key) const
  作用：检查键是否存在（返回 0 或 1）
  示例：if (m.count("Alice"))

find(key)
  语法：iterator find(const Key& key)
  作用：查找键，返回指向该元素的迭代器
  返回：找到返回迭代器，没找到返回 end()
  示例：auto it = m.find("Alice");

────────────────────────────────────

【set 专属函数】

insert(val)
  语法：pair<iterator, bool> insert(const T& val)
  作用：插入一个值（重复则忽略）
  返回：插入位置 + 是否成功
  示例：s.insert(10);

count(val)
  语法：size_type count(const T& val) const
  作用：检查值是否存在（返回 0 或 1）
  示例：if (s.count(10))

find(val)
  语法：iterator find(const T& val)
  作用：查找值，返回迭代器
  示例：auto it = s.find(10);
)" << endl;

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
1. 容器选择
   场景：你要写一个通讯录程序，需要按姓名查找电话号码
   要求：选择最合适的容器，并说明理由
   提示：思考"键→值"的关系

2. 容器选择
   场景：你要记录一学期所有考试成绩（允许重复分数）
   要求：选择最合适的容器，并说明理由
   提示：思考是否需要按键查找、是否需要下标访问

3. 代码纠错
   以下代码有 2 处错误，找出并修正：
   ```
   list<int> lst = {1, 2, 3};
   cout << lst[0] << endl;
   map<string, int> m;
   if (m["test"] > 0) { cout << "存在"; }
   ```

4. 思考题
   为什么 vector 的 push_back 操作有时会导致之前保存的
   迭代器失效，而 list 的 push_back 不会？
   提示：对比两种容器的内存结构

5. 代码编写
   创建一个 vector<string>，添加三种水果名称，
   然后遍历输出每种水果，最后输出总数。
   提示：push_back、范围 for、size()
)" << endl;

    return 0;
}

// 编译命令：g++ -std=c++11 -o 57_stl_container_overview 57_stl_container_overview.cpp
// 运行方式：./57_stl_container_overview 或 57_stl_container_overview.exe
// C++版本：C++11（初始化列表和范围 for 需要）
