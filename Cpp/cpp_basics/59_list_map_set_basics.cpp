// ============================================
// 59. list、map、set 入门 - 超详细注释版
// 日期：2026-05-04（第18天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】

今天学习三种容器的入门用法：
- list：链表，擅长头部和中间的插入/删除
- map：字典，按键快速查找值
- set：集合，自动去重和排序

本文件重点：理解每种容器的"适用场景"和"基本操作"
不要求记住所有接口，先建立"什么场景用什么容器"的思维
*/

#include <iostream>
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
    // 1.1 list — 链表
    // --------------------------------------------

    cout << "1.1 list — 链表是什么？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【list 核心概念】

是什么？
- 双向链表，每个节点存储数据 + 两个指针（指向前/后节点）

为什么需要 list？
- vector 在中间插入/删除很慢（要移动后面所有元素）
- list 在任意位置插入/删除都很快（O(1)，只改指针）

核心特点：
  优点                          缺点
  ─────────────────────────────────────────
  任意位置插入/删除快 O(1)      不支持下标访问
  头部操作方便（push_front）    访问第 n 个元素慢 O(n)
  插入/删除不会使迭代器失效     内存开销大（每个节点额外两个指针）
  有序合并操作（merge/splice）  随机访问效率低

适用场景：
- 频繁在头部插入/删除
- 频繁在中间插入/删除（已知位置）
- 不需要随机访问（按下标访问）

形象比喻：
  vector = 一排连座的长椅，中间加人很麻烦
  list = 一队手拉手的人，中间加人只需松手重连
)"" << endl;

    // --------------------------------------------
    // 1.2 map — 字典
    // --------------------------------------------

    cout << "1.2 map — 字典是什么？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【map 核心概念】

是什么？
- 键值对（key-value）的集合，内部按键自动排序
- 类似现实中的字典：查"apple"找到"苹果"

为什么需要 map？
- vector 按位置存，查找只能遍历（O(n)）
- map 按键存，查找只需 O(log n)
- 100万个元素，查找最多约 20 次比较

核心特点：
  优点                          缺点
  ─────────────────────────────────────────
  按键查找快 O(log n)           内存开销比 vector 大
  键自动排序                    插入/删除比 vector 末尾操作慢
  [] 语法直观易用               [] 在键不存在时会自动插入
  一个键只对应一个值            不适合存重复键

适用场景：
- 需要按"名称/编号"快速查找对应信息
- 需要维护键的有序性
- 统计频次（键=元素，值=出现次数）

形象比喻：
  map = 新华字典，查字找释义
  vector = 编号列表，只能按编号翻
)"" << endl;

    // --------------------------------------------
    // 1.3 set — 集合
    // --------------------------------------------

    cout << "1.3 set — 集合是什么？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【set 核心概念】

是什么？
- 元素的集合，自动去重和排序
- 可以理解为"只有键没有值"的 map

为什么需要 set？
- 需要存储不重复的元素
- 需要快速判断某个值是否存在

核心特点：
  优点                          缺点
  ─────────────────────────────────────────
  自动去重                      不允许重复值
  自动排序                      不能修改已插入的值
  查找快 O(log n)               不支持下标访问
  插入/删除方便                 只存值，没有"值对应值"

适用场景：
- 去重（如去除重复单词）
- 检查成员是否存在
- 维护有序不重复的集合

形象比喻：
  set = 名册，每人只能登记一次，自动按拼音排序
  map = 花名册，每人登记且带联系方式
)"" << endl;

    // --------------------------------------------
    // 1.4 三种容器 vs vector 对比总结
    // --------------------------------------------

    cout << "1.4 四种容器对比总结" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
特性          vector     list       map           set
──────────────────────────────────────────────────────────
存储方式      连续内存    离散节点    红黑树节点    红黑树节点
下标访问      O(1) 支持   不支持     不支持        不支持
按键查找      不支持      不支持     O(log n)      O(log n)
末尾添加      O(1)       O(1)       O(log n)      O(log n)
头部添加      O(n)       O(1)       N/A           N/A
中间插入      O(n)       O(1)*      O(log n)      O(log n)
自动排序      否          否          是            是
自动去重      否          否          是(键)        是(值)

* list 中间插入 O(1) 的前提：已经知道插入位置的迭代器

一句话选型：
- 大多数场景 → vector
- 频繁头/中间插入删除 → list
- 按键查找 → map
- 去重/判存 → set
)"" << endl;

    // --------------------------------------------
    // 1.5 重要函数讲解
    // --------------------------------------------

    cout << "1.5 重要函数讲解" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【函数卡片：list::push_front()】

函数名：push_front()
作用：在链表头部添加一个元素
类型：list 成员函数

完整语法：
void push_front(const T& value);

参数：
  - value（必需）：要添加的值
  - 类型：T（list 元素类型的引用）

返回值：无

示例：
list<int> lst;
lst.push_front(10);  // lst = {10}
lst.push_front(20);  // lst = {20, 10}（20 在头部）

注意：vector 没有 push_front()，需要用 insert() 替代

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：map::operator[]】

函数名：operator[]
作用：按键访问或插入值
类型：map 成员函数

完整语法：
T& operator[](const Key& key);

参数：
  - key（必需）：要访问/插入的键

返回值：对应键的值的引用

注意：
  - 如果键已存在 → 返回对应值的引用
  - 如果键不存在 → 自动插入一个默认值，再返回引用
  - 这是 map 最大的陷阱！

示例：
map<string, int> ages;
ages["Alice"] = 25;   // 插入键值对
cout << ages["Alice"]; // 输出 25
cout << ages["Bob"];   // 自动插入 Bob=0，输出 0！

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：set::insert()】

函数名：insert()
作用：插入一个元素（重复则忽略）
类型：set 成员函数

完整语法：
pair<iterator, bool> insert(const T& value);

参数：
  - value（必需）：要插入的值

返回值：
  - pair<iterator, bool>
  - first：指向元素的迭代器
  - second：是否插入成功（true=新插入，false=已存在）

示例：
set<int> s;
auto result = s.insert(10);
cout << result.second;  // true（新插入）
result = s.insert(10);
cout << result.second;  // false（已存在）
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

    // ===========================================
    // list 操作实践
    // ===========================================

    cout << "\n========== list 操作实践 ==========\n" << endl;

    // --------------------------------------------
    // 2.1 list：创建和基本操作
    // --------------------------------------------

    cout << "2.1 list 创建和基本操作" << endl;
    cout << string(60, '-') << endl;

    // 创建
    list<int> lst = {20, 30, 40};

    // push_front：头部添加（vector 做不到！）
    lst.push_front(10);  // lst = {10, 20, 30, 40}

    // push_back：末尾添加
    lst.push_back(50);   // lst = {10, 20, 30, 40, 50}

    // 遍历（只能用范围 for 或迭代器，不能用下标）
    cout << "遍历：";
    for (int val : lst) {
        cout << val << " ";  // 10 20 30 40 50
    }
    cout << endl;

    // size 和 empty
    cout << "大小：" << lst.size() << endl;    // 5
    cout << "是否为空：" << (lst.empty() ? "是" : "否") << endl;  // 否

    // pop_front：头部删除
    lst.pop_front();  // lst = {20, 30, 40, 50}
    cout << "头部删除后：";
    for (int val : lst) cout << val << " ";  // 20 30 40 50
    cout << endl;

    // pop_back：末尾删除
    lst.pop_back();  // lst = {20, 30, 40}
    cout << "末尾删除后：";
    for (int val : lst) cout << val << " ";  // 20 30 40
    cout << endl;

    // sort：排序（list 自带 sort，不需要用 STL 算法）
    list<int> unsorted = {30, 10, 20, 50, 40};
    unsorted.sort();
    cout << "排序后：";
    for (int val : unsorted) cout << val << " ";  // 10 20 30 40 50
    cout << endl;

    // reverse：反转
    unsorted.reverse();
    cout << "反转后：";
    for (int val : unsorted) cout << val << " ";  // 50 40 30 20 10
    cout << endl;

    cout << endl;

    // ===========================================
    // map 操作实践
    // ===========================================

    cout << "========== map 操作实践 ==========\n" << endl;

    // --------------------------------------------
    // 2.2 map：创建和基本操作
    // --------------------------------------------

    cout << "2.2 map 创建和基本操作" << endl;
    cout << string(60, '-') << endl;

    // 创建
    map<string, int> ages;

    // 用 [] 插入键值对
    ages["Alice"] = 25;  // 键 "Alice"，值 25
    ages["Bob"] = 30;    // 键 "Bob"，值 30
    ages["Charlie"] = 28; // 键 "Charlie"，值 28

    // 用 [] 查找值
    cout << "Alice 年龄：" << ages["Alice"] << endl;  // 25

    // 遍历（按键自动排序！）
    cout << "遍历（自动按键排序）：";
    for (const auto& pair : ages) {
        cout << pair.first << "=" << pair.second << " ";
        // Alice=25 Bob=30 Charlie=28
    }
    cout << endl;

    // count：检查键是否存在
    cout << "Alice 存在？" << (ages.count("Alice") ? "是" : "否") << endl;    // 是
    cout << "David 存在？" << (ages.count("David") ? "是" : "否") << endl;    // 否

    // find：查找键，返回迭代器
    auto it = ages.find("Bob");
    if (it != ages.end()) {
        cout << "找到 Bob：" << it->second << endl;  // 30
        // it->first 是键，it->second 是值
    }

    // 修改值（用 [] 直接赋值）
    ages["Alice"] = 26;  // 修改 Alice 的年龄
    cout << "修改后 Alice：" << ages["Alice"] << endl;  // 26

    // erase：按键删除
    ages.erase("Charlie");
    cout << "删除 Charlie 后大小：" << ages.size() << endl;  // 2

    cout << endl;

    // --------------------------------------------
    // 2.3 map 实战：词频统计
    // --------------------------------------------

    cout << "2.3 map 实战：词频统计" << endl;
    cout << string(60, '-') << endl;

    // 用 map 统计每个单词出现的次数
    vector<string> words = {"apple", "banana", "apple", "cherry",
                            "banana", "apple", "date", "cherry"};

    map<string, int> word_count;

    // 遍历所有单词
    for (const string& word : words) {
        // 键 = 单词，值 = 出现次数
        word_count[word]++;  // 不存在时自动插入 0，然后 +1
    }

    // 输出统计结果（按键排序）
    cout << "词频统计：" << endl;
    for (const auto& pair : word_count) {
        cout << "  " << pair.first << ": " << pair.second << " 次" << endl;
    }
    // apple: 3 次, banana: 2 次, cherry: 2 次, date: 1 次

    cout << endl;

    // ===========================================
    // set 操作实践
    // ===========================================

    cout << "========== set 操作实践 ==========\n" << endl;

    // --------------------------------------------
    // 2.4 set：创建和基本操作
    // --------------------------------------------

    cout << "2.4 set 创建和基本操作" << endl;
    cout << string(60, '-') << endl;

    // 创建
    set<int> s;

    // insert：插入元素（重复自动忽略）
    s.insert(30);
    s.insert(10);
    s.insert(20);
    s.insert(30);  // 重复！被忽略
    s.insert(10);  // 重复！被忽略
    s.insert(40);

    // 遍历（自动排序 + 无重复）
    cout << "遍历（自动排序、无重复）：";
    for (int val : s) {
        cout << val << " ";  // 10 20 30 40
    }
    cout << endl;

    // count：检查值是否存在
    cout << "20 存在？" << (s.count(20) ? "是" : "否") << endl;  // 是
    cout << "50 存在？" << (s.count(50) ? "是" : "否") << endl;  // 否

    // find：查找值，返回迭代器
    auto sit = s.find(30);
    if (sit != s.end()) {
        cout << "找到：" << *sit << endl;  // 30
    }

    // erase：删除元素
    s.erase(20);
    cout << "删除 20 后：";
    for (int val : s) cout << val << " ";  // 10 30 40
    cout << endl;

    // size
    cout << "大小：" << s.size() << endl;  // 3

    cout << endl;

    // --------------------------------------------
    // 2.5 set 实战：去重
    // --------------------------------------------

    cout << "2.5 set 实战：去重" << endl;
    cout << string(60, '-') << endl;

    // 有一组数据包含重复值
    vector<int> data = {5, 3, 8, 3, 5, 1, 8, 2, 5, 3};

    // 用 set 去重
    set<int> unique_data(data.begin(), data.end());
    // 用 data 的迭代器范围初始化 set，自动去重 + 排序

    cout << "原数据大小：" << data.size() << endl;       // 10
    cout << "去重后大小：" << unique_data.size() << endl; // 5

    cout << "去重结果（已排序）：";
    for (int val : unique_data) {
        cout << val << " ";  // 1 2 3 5 8
    }
    cout << endl;

    // 把去重结果转回 vector
    vector<int> unique_vec(unique_data.begin(), unique_data.end());
    cout << "转回 vector：";
    for (int val : unique_vec) {
        cout << val << " ";  // 1 2 3 5 8
    }
    cout << endl;

    return 0;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【🔍 深入理解1：list 内部结构 — 双向链表】

list 的每个节点结构：
  ┌──────────────────────────────┐
  │ prev指针 │ 数据 │ next指针   │
  └──────────────────────────────┘

节点间通过指针相连：
  nullptr ← [prev|10|next] ⇄ [prev|20|next] ⇄ [prev|30|next] → nullptr

为什么 list 不能用下标访问？
- 要找第 3 个元素，必须从头开始一个个走
- 走 3 步 = O(3)，走 n 步 = O(n)
- 所以 list 根本不提供 [] 操作

为什么 list 插入/删除快？
- 在任意位置插入，只需：
  1. 新建节点
  2. 修改前后节点的指针指向新节点
  不需要移动其他元素，O(1)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【🔍 深入理解2：map 和 set 底层 — 红黑树】

map 和 set 内部用红黑树（Red-Black Tree）实现：

红黑树特点：
1. 每个节点是红色或黑色
2. 根节点是黑色
3. 红色节点的子节点必须是黑色
4. 从任一节点到其子树所有叶子的路径，黑色节点数相同

这些规则保证了树的"大致平衡"：
- 最长路径不超过最短路径的 2 倍
- 查找、插入、删除都是 O(log n)

为什么遍历 map/set 是有序的？
- 红黑树是二叉搜索树的变体
- 中序遍历二叉搜索树 = 有序输出
- map/set 的迭代器就是按中序遍历的

unordered_map / unordered_set vs map / set：
  容器              底层       查找效率    元素有序
  map/set          红黑树     O(log n)    是
  unordered_*      哈希表     O(1)*       否

  * 哈希冲突时可能退化到 O(n)
  * C++11 引入，需要 #include <unordered_map> / <unordered_set>

如果不需要有序，优先用 unordered 版本（更快）
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

int main_errors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：用 [] 检查 map 中键是否存在
    // --------------------------------------------
    cout << "4.1 错误1：用 [] 检查 map 中键是否存在" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
map<string, int> ages;
ages["Alice"] = 25;

// 想检查 Bob 是否存在
if (ages["Bob"] != 0) {  // 问题！[] 会自动插入 Bob=0
    cout << "Bob 存在";
}
// 现在 ages 有两个元素了！

✅ 正确代码：
// 用 count 检查
if (ages.count("Bob") > 0) {
    cout << "Bob 存在";
}

// 或用 find
if (ages.find("Bob") != ages.end()) {
    cout << "Bob 存在";
}

记忆技巧：
- [] = 读/写操作，会自动插入
- count = 存在性检查，不会影响容器
- find = 精确定位，也不影响容器
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：list 用下标访问
    // --------------------------------------------
    cout << "4.2 错误2：list 用下标访问" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
list<int> lst = {10, 20, 30};
cout << lst[0];  // 编译错误！

原因：list 是链表，不支持随机访问

✅ 正确代码：
// 遍历
for (int val : lst) { cout << val; }

// 如果需要第 n 个元素，用 advance
auto it = lst.begin();
advance(it, 2);     // it 现在指向第 3 个元素
cout << *it;         // 输出 30

// 但 advance 是 O(n) 的！这说明你不该用 list 做随机访问

记忆技巧：
- 需要 [] 访问 → 用 vector
- 用 list → 只用遍历和头部/末尾操作
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：set 中修改元素
    // --------------------------------------------
    cout << "4.3 错误3：set 中修改元素" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
set<int> s = {10, 20, 30};
auto it = s.find(20);
*it = 25;  // 编译错误！set 的迭代器是 const 的

原因：set 依赖元素值来维持排序
      修改值可能破坏排序结构
      所以 set 不允许通过迭代器修改值

✅ 正确代码：
set<int> s = {10, 20, 30};

// 要"修改"一个元素：先删除旧的，再插入新的
s.erase(20);    // 删除 20
s.insert(25);    // 插入 25
// s = {10, 25, 30}

记忆技巧：
- set 的元素不可修改
- 需要"改值" = 先删后插
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：map 的 [] 导致默认值插入
    // --------------------------------------------
    cout << "4.4 错误4：map 的 [] 导致默认值插入" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
map<string, int> scores;
scores["Alice"] = 90;

// 只是想知道 Alice 的分数
int alice_score = scores["Alice"];  // OK，Alice 存在

// 想看 Bob 的分数，但 Bob 不存在
int bob_score = scores["Bob"];  // 问题！自动插入 Bob=0
cout << scores.size();  // 输出 2！本应只有 Alice

✅ 正确代码：
// 安全读取方式1：用 at()
try {
    int bob_score = scores.at("Bob");  // 抛出 out_of_range
} catch (const out_of_range& e) {
    cout << "Bob 不存在" << endl;
}

// 安全读取方式2：先检查
if (scores.count("Bob")) {
    int bob_score = scores["Bob"];  // 确认存在后再用 []
}

记忆技巧：
- 赋值用 [] → OK（插入或修改）
- 读取用 at() 或先 count → 安全
- [] 读取 → 危险（可能意外插入）
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：erase 后使用失效的迭代器
    // --------------------------------------------
    cout << "4.5 错误5：erase 后使用失效的迭代器" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
set<int> s = {10, 20, 30, 40, 50};
for (auto it = s.begin(); it != s.end(); it++) {
    if (*it == 30) {
        s.erase(it);   // it 失效了！
        // 再 it++ 就是未定义行为
    }
}

✅ 正确代码：
// 方法1：用 erase 的返回值（C++11）
for (auto it = s.begin(); it != s.end(); ) {
    if (*it == 30) {
        it = s.erase(it);  // erase 返回下一个有效迭代器
    } else {
        it++;
    }
}

// 方法2：直接按键删除（更简单）
s.erase(30);  // 如果 30 存在就删除

记忆技巧：
- erase 会使当前迭代器失效
- 必须用 erase 返回的迭代器继续遍历
- 关联容器（map/set）可以直接按值/键删除
)" << endl;

    // --------------------------------------------
    // 4.6 错误6：忽略 map 的自动排序特性
    // --------------------------------------------
    cout << "4.6 错误6：忽略 map 的自动排序特性" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码（逻辑错误）：
map<int, string> tasks;
tasks[3] = "写代码";    // 按优先级 3
tasks[1] = "看文档";    // 按优先级 1
tasks[2] = "开会";      // 按优先级 2

// 以为会按插入顺序输出
for (const auto& p : tasks) {
    cout << p.second << endl;
}
// 实际输出：看文档(1) → 开会(2) → 写代码(3)
// 被自动排序了！

原因：map 按键自动排序，不保留插入顺序

✅ 如果需要保留插入顺序：
- 可以用 vector<pair<Key, Value>> 替代
- 或用 C++11 的第三方库（如 boost::container::flat_map）

记忆技巧：
- map = 有序字典（按键排序）
- 需要保留插入顺序 → 不要用 map
)" << endl;

    // --------------------------------------------
    // 4.7 错误7：混用容器构造函数
    // --------------------------------------------
    cout << "4.7 错误7：混用容器构造函数" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
// 想创建一个包含 5 个空字符串的 vector
vector<string> v(5, "");    // OK，5 个空字符串

// 想创建一个包含 "hi", "hi", "hi" 的 vector
vector<string> v2(3, "hi");  // OK，3 个 "hi"

// 但对 map/set 不适用：
map<string, int> m(5);  // 编译错误！map 没有这种构造函数

✅ 正确代码：
// map 只能默认构造或从其他 map 构造
map<string, int> m;                    // 空的 map
map<string, int> m2 = m;              // 拷贝构造
map<string, int> m3 = {{"a",1},{"b",2}}; // 初始化列表

// set 同理
set<int> s;                      // 空的 set
set<int> s2 = {1, 2, 3};        // 初始化列表

记忆技巧：
- vector 支持大小/值构造：vector<T>(n, val)
- map/set 只支持默认构造和初始化列表
- 每种容器的构造方式不同，别套用
)" << endl;

    return 0;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

int main_reference() {
    cout << R"(
============================================
list / map / set 函数卡片速查
============================================

【list 函数】

构造：list<T> l;  或  list<T> l = {a, b, c};

push_front(val)  — 头部添加元素
push_back(val)   — 末尾添加元素
pop_front()      — 头部删除元素
pop_back()       — 末尾删除元素
front()          — 访问首元素
back()           — 访问末元素
size()           — 元素个数
empty()          — 是否为空
clear()          — 清空
sort()           — 排序（list 专有，不需要 std::sort）
reverse()        — 反转

────────────────────────────────────

【map 函数】

构造：map<K, V> m;  或  map<K, V> m = {{k1,v1},{k2,v2}};

m[key]           — 访问/插入键值对（注意自动插入！）
m.at(key)        — 访问键值对（不存在则抛异常）
m.insert({k,v})  — 插入键值对
m.count(key)     — 键是否存在（0 或 1）
m.find(key)      — 查找键，返回迭代器
m.erase(key)     — 按键删除
m.size()         — 元素个数
m.empty()        — 是否为空
m.clear()        — 清空

遍历：
for (const auto& p : m) {
    p.first  // 键
    p.second // 值
}

────────────────────────────────────

【set 函数】

构造：set<T> s;  或  set<T> s = {a, b, c};

s.insert(val)    — 插入值（重复忽略）
s.count(val)     — 值是否存在（0 或 1）
s.find(val)      — 查找值，返回迭代器
s.erase(val)     — 按值删除
s.size()         — 元素个数
s.empty()        — 是否为空
s.clear()        — 清空

遍历：
for (int val : s) {
    cout << val;  // 自动排序输出
}

去重技巧：
vector<T> v = {...};
set<T> s(v.begin(), v.end());     // vector → set（去重+排序）
vector<T> clean(s.begin(), s.end()); // set → vector（转回）
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
1. 容器选择
   以下场景分别应该选择哪个容器？
   a) 存储班级 50 名学生的成绩，需要按学号查询
   b) 管理一个先进先出的任务队列（需要在头部删除、末尾添加）
   c) 记录 100 个英文单词各自出现的次数
   d) 存储一组不重复的用户 ID
   提示：回顾"选容器决策树"

2. list 操作
   创建一个 list<string>，完成以下操作：
   a) 在头部添加 "First"
   b) 在末尾添加 "Last"
   c) 添加 "Middle"（插入到 First 和 Last 之间）
   d) 输出所有元素
   提示：push_front、push_back、insert

3. map 实战
   用 map 实现一个简单的通讯录：
   a) 添加 3 个人的姓名和电话
   b) 按姓名查找某个人的电话
   c) 修改某个人的电话
   d) 删除一个人
   e) 输出所有联系人
   提示：string 类型的 key 和 value，count 检查存在

4. set 实战
   给定一个 vector<int> = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5}
   a) 用 set 去重后输出
   b) 检查数字 7 是否存在
   c) 输出去重后的元素个数
   提示：用迭代器范围构造 set

5. 综合思考
   为什么list 自带 sort() 函数，而 vector 没有？
   （提示：std::sort 要求随机访问迭代器，list 的迭代器不是随机访问的）

6. 综合思考
   如果你要写一个学生管理系统，每个学生有学号、姓名、成绩，
   你会用什么容器来存储？为什么？
   （提示：想想查找方式是按学号还是按位置）
)" << endl;

    return 0;
}

// 编译命令：g++ -std=c++11 -o 59_list_map_set_basics 59_list_map_set_basics.cpp
// 运行方式：./59_list_map_set_basics 或 59_list_map_set_basics.exe
// C++版本：C++11（初始化列表、范围 for、auto 需要）
