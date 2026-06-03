// ============================================
// 61. STL 常用算法 - 超详细注释版
// 日期：2026-05-04（第19天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】

什么是 STL 算法？
- <algorithm> 头文件提供的一系列通用函数模板
- 通过迭代器操作容器中的元素，不关心容器类型

为什么需要 STL 算法？
- 排序、查找、计数等操作太常见，不想每次手写
- STL 算法经过充分优化，比自己写的更高效
- 代码更简洁：sort(v.begin(), v.end()) 一行搞定

核心特点：
- 算法不直接操作容器，而是通过迭代器操作元素
- 同一个算法可用于不同容器（vector、list、数组...）
- 大部分算法需要 #include <algorithm>

本文件重点：掌握最常用的 7 个算法（sort、find、find_if、count、reverse、for_each、transform）
*/

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>    // STL 算法头文件
#include <numeric>      // accumulate 等数值算法
#include <functional>   // greater 等函数对象
using namespace std;

// 自定义函数，用于 find_if 和 for_each
bool isEven(int n) { return n % 2 == 0; }
bool isNegative(int n) { return n < 0; }
void printInt(int n) { cout << n << " "; }
int doubleIt(int n) { return n * 2; }

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 算法的工作方式
    // --------------------------------------------

    cout << "1.1 算法的工作方式" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【STL 算法的核心思想】

算法不认识容器，只认识迭代器！

  sort(v.begin(), v.end());
       ↑              ↑
       起始迭代器     结束迭代器

sort 不知道 v 是 vector 还是数组
它只拿到两个迭代器，然后对 [begin, end) 范围内的元素排序

通用公式：
  algorithm(container.begin(), container.end(), ...);

参数模式：
1. [begin, end) — 操作范围（左闭右开）
2. 可选参数 — 比较函数、自定义操作等

为什么这样设计？
- 同一个 sort 既能排 vector 又能排数组
- 不需要为每种容器写一个排序函数
- "算法 + 迭代器 = 通用"
)"" << endl;

    // --------------------------------------------
    // 1.2 重要算法函数卡片
    // --------------------------------------------

    cout << "1.2 重要算法函数卡片" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【函数卡片：sort()】

函数名：sort()
作用：对范围内元素排序（默认升序）
头文件：<algorithm>

完整语法：
void sort(RandomIt first, RandomIt last);
void sort(RandomIt first, RandomIt last, Compare comp);

参数详解：
1. first（必需）
   - 类型：随机访问迭代器
   - 作用：排序范围起始位置

2. last（必需）
   - 类型：随机访问迭代器
   - 作用：排序范围结束位置（不含）

3. comp（可选）
   - 类型：比较函数/函数对象
   - 作用：自定义排序规则
   - 示例：greater<int>() 降序

返回值：无（原地排序）

使用示例：
vector<int> v = {3, 1, 4, 1, 5};
sort(v.begin(), v.end());                // 升序：1 1 3 4 5
sort(v.begin(), v.end(), greater<int>()); // 降序：5 4 3 1 1

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：find()】

函数名：find()
作用：在范围内查找等于指定值的元素
头文件：<algorithm>

完整语法：
InputIt find(InputIt first, InputIt last, const T& value);

参数详解：
1. first, last — 搜索范围
2. value — 要查找的值

返回值：
- 找到：返回指向该元素的迭代器
- 没找到：返回 last（即 end()）

使用示例：
auto it = find(v.begin(), v.end(), 30);
if (it != v.end()) {
    cout << "找到：" << *it;  // 30
} else {
    cout << "没找到";
}

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：count()】

函数名：count()
作用：统计范围内等于指定值的元素个数
头文件：<algorithm>

完整语法：
difference_type count(InputIt first, InputIt last, const T& value);

参数：first, last — 范围，value — 要计数的值
返回值：元素个数（有符号整数类型）

使用示例：
int n = count(v.begin(), v.end(), 3);  // 值为 3 的元素有几个

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：reverse()】

函数名：reverse()
作用：反转范围内元素的顺序
头文件：<algorithm>

完整语法：
void reverse(BidirectionalIt first, BidirectionalIt last);

参数：first, last — 范围
返回值：无（原地反转）

使用示例：
reverse(v.begin(), v.end());  // 将 v 反转

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：find_if()】

函数名：find_if()
作用：查找第一个满足条件的元素
头文件：<algorithm>

完整语法：
InputIt find_if(InputIt first, InputIt last, Predicate pred);

参数：
1. first, last — 范围
2. pred — 判断函数（返回 bool 的函数/函数对象）

返回值：指向第一个满足 pred 的元素的迭代器，没找到返回 last

使用示例：
auto it = find_if(v.begin(), v.end(), isNegative);
// 找到第一个负数

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：for_each()】

函数名：for_each()
作用：对范围内每个元素执行指定操作
头文件：<algorithm>

完整语法：
Function for_each(InputIt first, InputIt last, Function f);

参数：
1. first, last — 范围
2. f — 对每个元素执行的操作函数

使用示例：
for_each(v.begin(), v.end(), printInt);  // 对每个元素调用 printInt

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【函数卡片：transform()】

函数名：transform()
作用：对范围内每个元素应用转换函数，结果写入目标范围
头文件：<algorithm>

完整语法：
OutputIt transform(InputIt first, InputIt last,
                   OutputIt d_first, UnaryOp op);

参数：
1. first, last — 输入范围
2. d_first — 输出起始位置
3. op — 转换函数

使用示例：
transform(v.begin(), v.end(), v.begin(), doubleIt);
// 对 v 中每个元素调用 doubleIt，结果写回 v
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
    // 2.1 sort — 排序
    // --------------------------------------------

    cout << "2.1 sort 排序" << endl;
    cout << string(60, '-') << endl;

    vector<int> nums = {5, 3, 8, 1, 9, 2, 7};

    // 默认升序排序
    sort(nums.begin(), nums.end());
    cout << "升序：";
    for (int n : nums) cout << n << " ";  // 1 2 3 5 7 8 9
    cout << endl;

    // 降序排序（用 greater<int>() 函数对象）
    sort(nums.begin(), nums.end(), greater<int>());
    cout << "降序：";
    for (int n : nums) cout << n << " ";  // 9 8 7 5 3 2 1
    cout << endl;

    // 部分排序（只排序前 4 个元素）
    vector<int> nums2 = {5, 3, 8, 1, 9, 2, 7};
    sort(nums2.begin(), nums2.begin() + 4);  // 只排序前4个
    cout << "部分排序前4个：";
    for (int n : nums2) cout << n << " ";  // 1 3 5 8 9 2 7
    cout << endl;

    // 排序 C 数组（迭代器 = 指针）
    int arr[] = {5, 3, 8, 1, 9};
    sort(arr, arr + 5);  // 对数组排序
    cout << "排序数组：";
    for (int n : arr) cout << n << " ";  // 1 3 5 8 9
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 2.2 find — 查找
    // --------------------------------------------

    cout << "2.2 find 查找" << endl;
    cout << string(60, '-') << endl;

    vector<int> data = {10, 20, 30, 40, 50};

    // 查找值为 30 的元素
    auto it = find(data.begin(), data.end(), 30);
    if (it != data.end()) {
        cout << "找到 30，位置：" << (it - data.begin()) << endl;  // 位置 2
        // it - data.begin() 是随机访问迭代器才有的，得到下标
    } else {
        cout << "没找到 30" << endl;
    }

    // 查找不存在的值
    auto it2 = find(data.begin(), data.end(), 99);
    if (it2 == data.end()) {
        cout << "没找到 99" << endl;
    }

    // 在 string 中查找字符
    string str = "Hello, World!";
    auto sit = find(str.begin(), str.end(), 'W');
    if (sit != str.end()) {
        cout << "找到 W，位置：" << (sit - str.begin()) << endl;  // 位置 7
    }

    cout << endl;

    // --------------------------------------------
    // 2.3 find_if — 条件查找
    // --------------------------------------------

    cout << "2.3 find_if 条件查找" << endl;
    cout << string(60, '-') << endl;

    vector<int> values = {3, -1, 7, -5, 2, -8};

    // 用自定义函数作为条件
    auto neg_it = find_if(values.begin(), values.end(), isNegative);
    if (neg_it != values.end()) {
        cout << "第一个负数：" << *neg_it << endl;  // -1
    }

    // 用 lambda 表达式（更灵活，不用提前定义函数）
    // lambda：[捕获](参数) { 函数体 }
    auto big_it = find_if(values.begin(), values.end(),
        [](int n) { return n > 5; }  // 找第一个大于 5 的
    );
    if (big_it != values.end()) {
        cout << "第一个 >5 的数：" << *big_it << endl;  // 7
    }

    cout << endl;

    // --------------------------------------------
    // 2.4 count — 计数
    // --------------------------------------------

    cout << "2.4 count 计数" << endl;
    cout << string(60, '-') << endl;

    vector<int> data2 = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};

    // 统计值为 5 的个数
    int fives = count(data2.begin(), data2.end(), 5);
    cout << "5 出现了 " << fives << " 次" << endl;  // 3

    // 统计值为 1 的个数
    int ones = count(data2.begin(), data2.end(), 1);
    cout << "1 出现了 " << ones << " 次" << endl;  // 2

    // count_if：统计满足条件的个数
    int evens = count_if(data2.begin(), data2.end(), isEven);
    cout << "偶数有 " << evens << " 个" << endl;  // 4 2 6 → 3个

    // 用 lambda 统计大于 3 的个数
    int bigs = count_if(data2.begin(), data2.end(),
        [](int n) { return n > 3; }
    );
    cout << "大于 3 的有 " << bigs << " 个" << endl;

    cout << endl;

    // --------------------------------------------
    // 2.5 reverse — 反转
    // --------------------------------------------

    cout << "2.5 reverse 反转" << endl;
    cout << string(60, '-') << endl;

    vector<int> seq = {1, 2, 3, 4, 5};

    reverse(seq.begin(), seq.end());
    cout << "反转后：";
    for (int n : seq) cout << n << " ";  // 5 4 3 2 1
    cout << endl;

    // 反转 string
    string greeting = "Hello";
    reverse(greeting.begin(), greeting.end());
    cout << "反转字符串：" << greeting << endl;  // olleH

    // 部分反转（只反转前 3 个）
    vector<int> seq2 = {1, 2, 3, 4, 5};
    reverse(seq2.begin(), seq2.begin() + 3);
    cout << "反转前3个：";
    for (int n : seq2) cout << n << " ";  // 3 2 1 4 5
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 2.6 for_each — 遍历操作
    // --------------------------------------------

    cout << "2.6 for_each 遍历操作" << endl;
    cout << string(60, '-') << endl;

    vector<int> nums3 = {1, 2, 3, 4, 5};

    // 用自定义函数
    cout << "for_each + 函数：";
    for_each(nums3.begin(), nums3.end(), printInt);  // 1 2 3 4 5
    cout << endl;

    // 用 lambda（更常见）
    cout << "for_each + lambda：";
    for_each(nums3.begin(), nums3.end(),
        [](int n) { cout << n * 2 << " "; }  // 2 4 6 8 10
    );
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 2.7 transform — 转换
    // --------------------------------------------

    cout << "2.7 transform 转换" << endl;
    cout << string(60, '-') << endl;

    vector<int> src = {1, 2, 3, 4, 5};

    // 原地转换：每个元素翻倍
    transform(src.begin(), src.end(), src.begin(), doubleIt);
    cout << "翻倍后：";
    for (int n : src) cout << n << " ";  // 2 4 6 8 10
    cout << endl;

    // 写入另一个容器
    vector<int> squares(src.size());  // 预分配空间
    transform(src.begin(), src.end(), squares.begin(),
        [](int n) { return n * n; }  // 每个元素平方
    );
    cout << "平方后：";
    for (int n : squares) cout << n << " ";  // 4 16 36 64 100
    cout << endl;

    // 字符串大写转换
    string lower = "hello";
    transform(lower.begin(), lower.end(), lower.begin(),
        [](char c) { return toupper(c); }  // 每个字符转大写
    );
    cout << "大写后：" << lower << endl;  // HELLO

    cout << endl;

    // --------------------------------------------
    // 2.8 其他实用算法快速概览
    // --------------------------------------------

    cout << "2.8 其他实用算法" << endl;
    cout << string(60, '-') << endl;

    vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

    // min_element / max_element：找最小/最大值
    auto min_it = min_element(v.begin(), v.end());
    auto max_it = max_element(v.begin(), v.end());
    cout << "最小值：" << *min_it << "，最大值：" << *max_it << endl;  // 1, 9

    // accumulate：累加（需要 #include <numeric>）
    int sum = accumulate(v.begin(), v.end(), 0);  // 初始值为 0
    cout << "总和：" << sum << endl;  // 31

    // sort + unique：去重（经典模式）
    vector<int> dup = {3, 1, 4, 1, 5, 3, 5};
    sort(dup.begin(), dup.end());  // 先排序
    auto last_unique = unique(dup.begin(), dup.end());  // 相邻重复移到末尾
    dup.erase(last_unique, dup.end());  // 删除末尾的多余元素
    cout << "sort+unique 去重：";
    for (int n : dup) cout << n << " ";  // 1 3 4 5
    cout << endl;

    return 0;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【🔍 深入理解1：sort 的底层 — IntroSort】

STL 的 sort 使用 IntroSort（内省排序）：
1. 开始用快速排序（平均 O(n log n)）
2. 如果递归太深，切换到堆排序（最坏 O(n log n)）
3. 子数组小时，切换到插入排序（常数因子更小）

所以 sort 的复杂度：
  最好：O(n log n)
  最坏：O(n log n)  ← 比纯快排好！纯快排最坏 O(n^2)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【🔍 深入理解2：为什么 list 不能用 std::sort】

list 的迭代器是"双向迭代器"，不支持 +n 和 []
std::sort 要求"随机访问迭代器"
所以 list 不能用 std::sort

解决方案：
1. list 自带的 sort() 成员函数
   lst.sort();  // OK，list 自己实现

2. 把 list 复制到 vector，排序后再复制回去
   vector<T> v(lst.begin(), lst.end());
   sort(v.begin(), v.end());
   lst = list<T>(v.begin(), v.end());

为什么 list 不支持随机访问？
- 链表节点不连续，不能通过 +n 直接跳转
- 只能沿指针一步步走

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【🔍 深入理解3：lambda 表达式初探】

lambda 是 C++11 引入的"匿名函数"：
  [捕获](参数) -> 返回类型 { 函数体 }

  []       — 不捕获任何外部变量
  [=]      — 按值捕获所有外部变量
  [&]      — 按引用捕获所有外部变量
  [x, &y]  — x 按值，y 按引用

示例：
  int threshold = 5;
  auto it = find_if(v.begin(), v.end(),
      [threshold](int n) { return n > threshold; }
  );
  // threshold 按值捕获，lambda 内部可以使用

lambda 是后续章节（第14周）的重点，这里先知道这种写法即可。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

int main_errors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：对 list 使用 std::sort
    // --------------------------------------------
    cout << "4.1 错误1：对 list 使用 std::sort" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
list<int> lst = {3, 1, 4, 1, 5};
sort(lst.begin(), lst.end());  // 编译错误！

原因：std::sort 要求随机访问迭代器
      list 的迭代器不支持 +n 和 [] 操作

✅ 正确代码：
list<int> lst = {3, 1, 4, 1, 5};
lst.sort();  // 用 list 自带的 sort() 成员函数

记忆技巧：
- vector/deque/string → std::sort()
- list/map/set → 自带 sort()（map/set 已自动排序）
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：忘记检查 find 的返回值
    // --------------------------------------------
    cout << "4.2 错误2：忘记检查 find 的返回值" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v = {1, 2, 3};
auto it = find(v.begin(), v.end(), 99);
cout << *it;  // 99 不存在！it == v.end()，解引用是未定义行为

✅ 正确代码：
auto it = find(v.begin(), v.end(), 99);
if (it != v.end()) {
    cout << *it;     // 找到了，安全访问
} else {
    cout << "没找到";  // 没找到，优雅处理
}

记忆技巧：
- find 返回的迭代器必须先检查是否 == end()
- 直接解引用可能是未定义行为
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：sort + unique 忘了 erase
    // --------------------------------------------
    cout << "4.3 错误3：sort + unique 忘了 erase" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v = {3, 1, 4, 1, 5, 3};
sort(v.begin(), v.end());
unique(v.begin(), v.end());  // unique 不删除元素！只是移到末尾
cout << v.size();  // 仍然是 6！没有变小

原因：unique 不改变容器大小
      它把重复元素移到末尾，返回"逻辑终点"的迭代器
      物理大小不变，需要手动 erase

✅ 正确代码：
vector<int> v = {3, 1, 4, 1, 5, 3};
sort(v.begin(), v.end());
auto last = unique(v.begin(), v.end());
v.erase(last, v.end());  // 删除末尾的多余元素
cout << v.size();  // 4

记忆技巧：
- sort + unique + erase = 去重三件套
- unique 不删除，只标记
- 必须 erase 才能真正移除
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：transform 目标容器空间不够
    // --------------------------------------------
    cout << "4.4 错误4：transform 目标容器空间不够" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> src = {1, 2, 3, 4, 5};
vector<int> dst;  // 空的！
transform(src.begin(), src.end(), dst.begin(), doubleIt);
// 未定义行为！dst 没有空间

原因：transform 向目标位置写入，但 dst 是空的
      没有元素可以被覆写

✅ 正确代码：
// 方法1：预先分配空间
vector<int> dst(src.size());  // 预分配与源一样大
transform(src.begin(), src.end(), dst.begin(), doubleIt);

// 方法2：用 back_inserter（后续详细学）
// transform(src.begin(), src.end(), back_inserter(dst), doubleIt);

记忆技巧：
- transform 写入目标 → 目标必须有足够空间
- 预分配用 vector<T>(size)
- 或用 back_inserter 自动扩容
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：忘记 #include <algorithm>
    // --------------------------------------------
    cout << "4.5 错误5：忘记 #include <algorithm>" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {3, 1, 2};
    sort(v.begin(), v.end());  // 可能编译错误！
}

原因：sort 等 STL 算法在 <algorithm> 中
      少数编译器可能通过其他头文件间接包含，但不可移植

✅ 正确代码：
#include <iostream>
#include <vector>
#include <algorithm>    // STL 算法
#include <numeric>      // accumulate 等数值算法
using namespace std;

常用头文件对应：
  <algorithm>  → sort, find, count, reverse, for_each, transform, unique...
  <numeric>    → accumulate, iota, inner_product...
  <functional> → greater, less, 函数对象...

记忆技巧：
- 用算法 → #include <algorithm>
- 用 accumulate → #include <numeric>
)" << endl;

    // --------------------------------------------
    // 4.6 错误6：accumulate 初始值类型不匹配
    // --------------------------------------------
    cout << "4.6 错误6：accumulate 初始值类型不匹配" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<double> v = {1.5, 2.5, 3.5};
double sum = accumulate(v.begin(), v.end(), 0);  // 结果是 int！7 而非 7.5

原因：accumulate 的返回类型由初始值决定
      初始值 0 是 int，所以整个计算用 int，丢失小数

✅ 正确代码：
double sum = accumulate(v.begin(), v.end(), 0.0);  // 初始值 0.0 是 double
// 或显式指定
double sum = accumulate(v.begin(), v.end(), 0.0);

记忆技巧：
- accumulate 的第三参数决定计算类型
- double 容器 → 用 0.0 不用 0
- long long 容器 → 用 0LL 不用 0
)" << endl;

    // --------------------------------------------
    // 4.7 错误7：用 count 检查存在性（低效）
    // --------------------------------------------
    cout << "4.7 错误7：用 count 检查存在性（低效）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 低效代码：
vector<int> v = {1, 2, 3, ..., 1000000};
if (count(v.begin(), v.end(), 42) > 0) {  // 遍历整个容器！
    cout << "找到了";
}

原因：count 会遍历整个范围，O(n)
      即使第一个元素就匹配，也要继续遍历

✅ 高效代码：
auto it = find(v.begin(), v.end(), 42);
if (it != v.end()) {  // 找到就停，平均更快
    cout << "找到了";
}

// 关联容器更高效：
set<int> s = {1, 2, 3, ..., 1000000};
if (s.count(42)) {  // O(log n)，比 find 快得多
    cout << "找到了";
}

记忆技巧：
- 只检查"是否存在" → find（找到就停）或 set::count（O(log n)）
- count 会遍历全部，适合统计个数，不适合判存
)" << endl;

    return 0;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

int main_reference() {
    cout << R"(
============================================
STL 算法函数卡片速查
============================================

【排序与重排】

sort(first, last)               — 排序（升序）
sort(first, last, comp)         — 排序（自定义规则）
stable_sort(first, last)        — 稳定排序（相等元素保持原顺序）
reverse(first, last)            — 反转
unique(first, last)             — 去重（相邻重复移末尾，需先排序）
random_shuffle(first, last)     — 随机打乱（C++14 起弃用）
shuffle(first, last, g)         — 随机打乱（C++11，需随机数引擎）
next_permutation(first, last)   — 下一个排列

【查找】

find(first, last, val)          — 查找等于 val 的元素
find_if(first, last, pred)      — 查找满足 pred 的元素
binary_search(first, last, val) — 二分查找（前提：已排序）

【计数】

count(first, last, val)         — 统计 val 出现次数
count_if(first, last, pred)     — 统计满足 pred 的元素数

【遍历与转换】

for_each(first, last, f)        — 对每个元素执行 f
transform(first, last, d, op)   — 对每个元素执行 op，写入 d

【极值】

min_element(first, last)        — 指向最小元素的迭代器
max_element(first, last)        — 指向最大元素的迭代器

【数值】（需 #include <numeric>）

accumulate(first, last, init)   — 累加（初始值 init）
accumulate(first, last, init, op) — 自定义累加操作

【修改】

fill(first, last, val)          — 填充相同值
copy(first, last, d_first)      — 复制到目标
remove(first, last, val)        — 移除等于 val 的元素（移末尾）
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
1. sort 练习
   给定 vector<int> = {9, 3, 7, 1, 5, 2, 8, 4, 6}
   a) 升序排序并输出
   b) 降序排序并输出
   c) 只排序前 5 个元素并输出全部
   提示：greater<int>()、begin()+5

2. find 与 find_if
   给定 vector<string> = {"apple", "banana", "cherry", "date", "elderberry"}
   a) 查找 "cherry"
   b) 查找第一个长度大于 5 的字符串
   提示：find + find_if + lambda

3. count 与 count_if
   给定 vector<int> = {3, 7, 2, 8, 1, 9, 4, 6, 5, 2, 8}
   a) 统计 2 出现的次数
   b) 统计偶数的个数
   c) 统计大于 5 的元素个数
   提示：count + count_if + lambda

4. 综合题：成绩分析
   给定 vector<double> = {85.5, 90.0, 78.5, 92.5, 88.0, 65.0, 95.0}
   a) 计算平均分（accumulate）
   b) 找出最高分和最低分（min_element/max_element）
   c) 按降序排序后输出
   d) 统计及格（>=60）的人数（count_if）
   提示：组合使用多个算法

5. 思考题
   为什么 unique 必须配合 sort 使用？
   如果不排序直接 unique 会怎样？
   提示：unique 只移除"相邻"的重复元素
)" << endl;

    return 0;
}

// 编译命令：g++ -std=c++11 -o 61_stl_algorithms 61_stl_algorithms.cpp
// 运行方式：./61_stl_algorithms 或 61_stl_algorithms.exe
// C++版本：C++11（lambda、auto、范围 for 需要）
