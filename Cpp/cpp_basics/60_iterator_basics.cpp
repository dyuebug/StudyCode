// ============================================
// 60. 迭代器基础 - 超详细注释版
// 日期：2026-05-04（第19天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】

什么是迭代器（Iterator）？
- 一种"泛化的指针"，用来遍历和访问容器中的元素
- 指针能做的事（*, ->, ++, --），迭代器都能做
- 但迭代器能适配所有容器，指针只能用于连续内存

为什么需要迭代器？
- 不同容器的内部结构不同（vector 连续，list 链式，map 树形）
- 迭代器提供了统一的访问方式，让算法不用关心容器内部结构
- 这就是"桥梁"作用：算法 ← 迭代器 → 容器

核心特点：
- 每种容器都提供 begin() 和 end() 获取迭代器
- 用 * 解引用访问元素，用 ++ 移动到下一个元素
- STL 算法通过迭代器操作容器，不直接操作容器本身

本文件重点：理解迭代器的概念、基本用法、与范围 for 的关系
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
    // 1.1 迭代器是什么？—— 用指针来理解
    // --------------------------------------------

    cout << "1.1 迭代器是什么？" << endl;
    cout << string(60, '-') << endl;

    // 先看指针的用法
    int arr[] = {10, 20, 30};
    int* ptr = arr;      // ptr 指向第一个元素
    cout << "*ptr = " << *ptr << endl;   // 解引用，输出 10
    ptr++;               // 移动到下一个元素
    cout << "*ptr = " << *ptr << endl;   // 输出 20

    cout << endl;

    // 迭代器的用法完全一样！
    vector<int> vec = {10, 20, 30};
    vector<int>::iterator it = vec.begin();  // it 指向第一个元素
    cout << "*it = " << *it << endl;          // 解引用，输出 10
    it++;                                      // 移动到下一个元素
    cout << "*it = " << *it << endl;          // 输出 20

    cout << R"(
【对比：指针 vs 迭代器】

操作          指针          迭代器
──────────────────────────────────────
指向首元素    ptr = arr     it = v.begin()
解引用        *ptr          *it
移动到下一个  ptr++         it++
比较          ptr != end    it != v.end()
访问成员      ptr->field   it->field

结论：迭代器就是"学会了适应不同容器的指针"
- 对 vector：迭代器内部就是指针
- 对 list：迭代器内部知道如何沿链表走
- 对 map：迭代器内部知道如何遍历红黑树

你用同样的 *it、it++ 操作，底层自动适配！
)"" << endl;

    // --------------------------------------------
    // 1.2 begin() 和 end() ⭐⭐⭐⭐⭐
    // --------------------------------------------

    cout << "1.2 begin() 和 end()" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【函数卡片：begin() 和 end()】

作用：获取容器的起始和末尾迭代器

begin()
  语法：iterator begin()
  作用：返回指向第一个元素的迭代器
  示例：auto it = v.begin();  // it 指向第一个元素

end()
  语法：iterator end()
  作用：返回指向"最后一个元素之后"的迭代器
  返回：不是最后一个元素！是"末尾之后"的位置

为什么 end 是"之后"？
  [10] [20] [30] [末尾之后]
   ^                  ^
 begin()            end()

  这样设计方便写循环：
  for (auto it = v.begin(); it != v.end(); it++) { ... }

  当 it == v.end() 时，表示已经遍历完所有元素

重要：
- end() 指向的位置不能解引用（*v.end() 是未定义行为！）
- 空容器：begin() == end()
)"" << endl;

    // 实际演示
    vector<int> v = {10, 20, 30, 40, 50};
    cout << "begin() 指向的元素：" << *v.begin() << endl;  // 10
    // *v.end() 不能用！

    // 用迭代器循环遍历
    cout << "迭代器遍历：";
    for (vector<int>::iterator it = v.begin(); it != v.end(); it++) {
        cout << *it << " ";  // 10 20 30 40 50
    }
    cout << endl;

    // 用 auto 简化（推荐！）
    cout << "auto 简化版：";
    for (auto it = v.begin(); it != v.end(); it++) {
        cout << *it << " ";  // 10 20 30 40 50
    }
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 1.3 迭代器类型⭐⭐⭐
    // --------------------------------------------

    cout << "1.3 迭代器类型" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【五种迭代器类型 — 从弱到强】

1. 输入迭代器（Input Iterator）
   - 只读，只能 ++，只能走一遍
   - 用途：istream_iterator

2. 输出迭代器（Output Iterator）
   - 只写，只能 ++，只能走一遍
   - 用途：ostream_iterator

3. 前向迭代器（Forward Iterator）
   - 可读写，只能 ++，可以多遍
   - 用途：forward_list

4. 双向迭代器（Bidirectional Iterator）
   - 可读写，能 ++ 和 --，可以多遍
   - 用途：list、map、set

5. 随机访问迭代器（Random Access Iterator）
   - 可读写，能 ++、--、+n、-n、[]，可以多遍
   - 最强大，和指针几乎一样
   - 用途：vector、deque、string

            能力对比
   ────────────────────────────────────────
   操作          输入  前向  双向  随机
   读 *it         ✅   ✅   ✅   ✅
   写 *it=       ✅   ✅   ✅   ✅
   ++it          ✅   ✅   ✅   ✅
   --it                    ✅   ✅
   it + n                        ✅
   it - n                        ✅
   it[n]                         ✅
   it1 - it2                     ✅

   这就是为什么：
   - vector 支持 [] 和 it+n（随机访问迭代器）
   - list 只支持 ++ 和 --（双向迭代器）
   - sort() 需要随机访问迭代器 → list 不能用 sort()
)"" << endl;

    // --------------------------------------------
    // 1.4 范围 for 与迭代器的关系
    // --------------------------------------------

    cout << "1.4 范围 for 与迭代器的关系" << endl;
    cout << string(60, '-') << endl;

    vector<string> names = {"Alice", "Bob", "Charlie"};

    // 范围 for（简单写法）
    cout << "范围 for：";
    for (const string& name : names) {
        cout << name << " ";
    }
    cout << endl;

    // 上面等价于下面的迭代器写法（编译器自动转换）
    cout << "迭代器版：";
    for (auto it = names.begin(); it != names.end(); it++) {
        cout << *it << " ";
    }
    cout << endl;

    cout << R"(
【结论】

范围 for 就是迭代器的语法糖！
编译器会把：
  for (const T& elem : container)
转换成：
  for (auto it = container.begin(); it != container.end(); it++)
      const T& elem = *it;

所以：
- 范围 for 能做的，迭代器都能做（反过来不一定）
- 需要更精细控制（删除元素、跳过元素等）时，用手动迭代器
- 简单遍历时，优先用范围 for（更简洁安全）
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
    // 2.1 vector 的迭代器操作
    // --------------------------------------------

    cout << "2.1 vector 的迭代器操作" << endl;
    cout << string(60, '-') << endl;

    vector<int> nums = {10, 20, 30, 40, 50};

    // 读写元素：通过迭代器修改值
    for (auto it = nums.begin(); it != nums.end(); it++) {
        *it += 5;  // 每个元素加 5
    }
    cout << "每个+5后：";
    for (int n : nums) cout << n << " ";  // 15 25 35 45 55
    cout << endl;

    // 随机访问：vector 迭代器支持 +n 和 [] 运算
    auto it = nums.begin();
    cout << "第3个元素（it+2）：" << *(it + 2) << endl;  // 35
    cout << "最后一个元素（it+4）：" << *(it + 4) << endl;  // 55

    // 距离计算：两个迭代器之间相差几个元素
    auto first = nums.begin();
    auto last = nums.end();
    cout << "元素个数（last - first）：" << last - first << endl;  // 5

    cout << endl;

    // --------------------------------------------
    // 2.2 list 的迭代器操作
    // --------------------------------------------

    cout << "2.2 list 的迭代器操作" << endl;
    cout << string(60, '-') << endl;

    list<string> words = {"hello", "world", "cpp"};

    // 遍历（和 vector 一样的写法！）
    cout << "遍历：";
    for (auto it = words.begin(); it != words.end(); it++) {
        cout << *it << " ";  // hello world cpp
    }
    cout << endl;

    // 双向移动（list 独有的 -- 操作）
    auto lit = words.end();
    lit--;  // 移到最后一个元素
    cout << "倒数第1个：" << *lit << endl;  // cpp
    lit--;  // 再往前一个
    cout << "倒数第2个：" << *lit << endl;  // world

    // list 迭代器不支持 +n（编译错误！）
    // auto err = words.begin() + 2;  // 编译错误
    // 如果需要移动多步，用 advance()
    auto it2 = words.begin();
    advance(it2, 2);  // 移动 2 步，等价于 it2++ 两次
    cout << "advance 后：" << *it2 << endl;  // cpp

    cout << endl;

    // --------------------------------------------
    // 2.3 map 的迭代器操作
    // --------------------------------------------

    cout << "2.3 map 的迭代器操作" << endl;
    cout << string(60, '-') << endl;

    map<string, int> ages = {{"Alice", 25}, {"Bob", 30}, {"Charlie", 28}};

    // 遍历 map（按键排序输出）
    cout << "遍历 map：" << endl;
    for (auto it = ages.begin(); it != ages.end(); it++) {
        // it->first 是键，it->second 是值
        cout << "  " << it->first << " = " << it->second << endl;
    }

    // 查找特定键
    auto found = ages.find("Bob");  // 返回指向该键值对的迭代器
    if (found != ages.end()) {
        cout << "找到 Bob：" << found->second << endl;  // 30
    }

    // 反向遍历（map 也支持双向迭代器）
    cout << "反向遍历：";
    for (auto it = ages.rbegin(); it != ages.rend(); it++) {
        cout << it->first << " ";
        // Charlie Bob Alice（按键从大到小）
    }
    cout << endl;

    cout << endl;

    // --------------------------------------------
    // 2.4 const 迭代器
    // --------------------------------------------

    cout << "2.4 const 迭代器" << endl;
    cout << string(60, '-') << endl;

    const vector<int> cv = {1, 2, 3, 4, 5};  // const 容器

    // const 容器只能用 const_iterator（只读）
    // vector<int>::const_iterator cit = cv.begin();  // 完整写法
    auto cit = cv.cbegin();  // 推荐：用 cbegin / cend
    cout << "const 遍历：";
    for (; cit != cv.cend(); cit++) {
        cout << *cit << " ";
        // *cit = 10;  // 编译错误！const 迭代器不能修改值
    }
    cout << endl;

    cout << R"(
【什么时候用 const 迭代器？】

- 容器是 const 的 → 只能用 const_iterator
- 只读遍历 → 推荐用 cbegin()/cend()（C++11）
- 需要修改 → 用 begin()/end()

记忆技巧：
  命名规则                含义
  begin() / end()        普通迭代器（可读写）
  cbegin() / cend()      const 迭代器（只读）
  rbegin() / rend()      反向迭代器
  crbegin() / crend()    const 反向迭代器（只读反向）
)"" << endl;

    return 0;
}

// ============================================
// 第三部分：深入理解（选学）🔍
// ============================================

/*
如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【🔍 深入理解1：迭代器的底层实现】

vector 的迭代器：
  内部就是原始指针！
  int* it = &v[0];   // 指向第一个元素
  it++;               // 指针 +1，移动到下一个元素
  *(it + 2);          // 指针偏移，直接跳转

  因为 vector 内存连续，所以指针可以直接跳转。
  这就是为什么 vector 迭代器是"随机访问"的。

list 的迭代器：
  内部是一个包装了节点指针的类
  class list_iterator {
      Node* current;    // 指向当前链表节点
      T& operator*() { return current->data; }
      iterator& operator++() {
          current = current->next;  // 沿着 next 指针走
          return *this;
      }
  };

  没有连续内存，只能沿着指针一步步走。
  所以 ++ 是 O(1)，但 +n 没有（必须走 n 步）。

map 的迭代器：
  类似 list，但沿着红黑树的中序遍历走
  operator++() 找到"中序后继节点"

  这就是为什么遍历 map 总是按键排序的。

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

【🔍 深入理解2：为什么 STL 算法不直接操作容器】

设计原则："算法 + 迭代器 = 通用"

sort(v.begin(), v.end());   // 排序 vector
sort(a, a + 5);             // 排序 C 数组！

同一个 sort 函数，既能排 vector，又能排数组。
因为 sort 只要求迭代器支持 ++, --, +n, 比较等操作。
它不关心底层是 vector 还是数组。

这就是迭代器的核心价值：
  把"如何遍历"这个细节隐藏起来
  让算法以统一的方式工作

STL 的设计哲学：
  算法不知道容器的类型，只知道迭代器的能力
  容器提供迭代器，算法使用迭代器
  两者通过迭代器协议（iterator protocol）连接
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

int main_errors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 4.1 错误1：解引用 end() 迭代器
    // --------------------------------------------
    cout << "4.1 错误1：解引用 end() 迭代器" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v = {1, 2, 3};
auto it = v.end();
cout << *it;  // 未定义行为！end() 指向最后一个之后

✅ 正确代码：
vector<int> v = {1, 2, 3};

// 要访问最后一个元素，用 back() 或 --end()
auto it = v.end();
it--;  // 先退回一步
cout << *it;  // 输出 3

// 或直接用
cout << v.back();  // 输出 3

记忆技巧：
- end() = "哨兵"，不是有效元素
- 解引用前，确保不是 end()
- 用 back()/front() 访问首尾，比手动操作迭代器更安全
)" << endl;

    // --------------------------------------------
    // 4.2 错误2：迭代器失效（vector 扩容）
    // --------------------------------------------
    cout << "4.2 错误2：迭代器失效（vector 扩容）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v = {1, 2, 3};
auto it = v.begin();
v.push_back(4);  // 可能扩容
cout << *it;     // it 可能已失效！未定义行为

✅ 正确代码：
vector<int> v = {1, 2, 3};
v.push_back(4);      // 先修改
auto it = v.begin(); // 修改后重新获取迭代器
cout << *it;          // 安全

记忆技巧：
- 修改 vector 后，之前的迭代器可能失效
- 修改后重新获取迭代器
- 如果不确定，就用下标访问或范围 for
)" << endl;

    // --------------------------------------------
    // 4.3 错误3：对 list 迭代器用 +n
    // --------------------------------------------
    cout << "4.3 错误3：对 list 迭代器用 +n" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
list<int> lst = {10, 20, 30};
auto it = lst.begin() + 2;  // 编译错误！list 迭代器不支持 +

✅ 正确代码：
list<int> lst = {10, 20, 30};
auto it = lst.begin();
advance(it, 2);    // 用 advance 移动多步
cout << *it;       // 输出 30

// 或用 next（C++11，不修改原迭代器）
auto it2 = next(lst.begin(), 2);  // 返回新迭代器
cout << *it2;      // 输出 30

记忆技巧：
- vector/string → 支持 +n（随机访问）
- list/map/set → 只能用 advance 或 next
- advance 修改原迭代器，next 返回新迭代器
)" << endl;

    // --------------------------------------------
    // 4.4 错误4：在迭代器循环中修改容器结构
    // --------------------------------------------
    cout << "4.4 错误4：在迭代器循环中修改容器结构" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v = {1, 2, 3, 4, 5};
for (auto it = v.begin(); it != v.end(); it++) {
    if (*it == 3) {
        v.erase(it);    // erase 后 it 失效！
        // it++ 是未定义行为
    }
}

✅ 正确代码：
for (auto it = v.begin(); it != v.end(); ) {
    if (*it == 3) {
        it = v.erase(it);  // erase 返回下一个有效迭代器
    } else {
        it++;               // 只在不删除时递增
    }
}

记忆技巧：
- erase 会使当前迭代器失效
- 必须接收 erase 的返回值作为新迭代器
- for 循环中，erase 时不要 it++
)" << endl;

    // --------------------------------------------
    // 4.5 错误5：混淆 iterator 和 const_iterator
    // --------------------------------------------
    cout << "4.5 错误5：混淆 iterator 和 const_iterator" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
const vector<int> cv = {1, 2, 3};
auto it = cv.begin();    // OK，但 it 是 const_iterator
*it = 10;                // 编译错误！不能修改 const 容器

或者：
void print(vector<int>& v) {
    auto it = v.begin();  // 可以修改，但不应该修改！
    *it = 10;             // 编译通过，但逻辑错误（只读函数不该改值）
}

✅ 正确代码：
// 只读函数用 cbegin/cend
void print(const vector<int>& v) {
    for (auto it = v.cbegin(); it != v.cend(); it++) {
        cout << *it << " ";  // 只读，安全
    }
}

记忆技巧：
- 只读遍历 → cbegin()/cend()
- 需要修改 → begin()/end()
- 函数参数能加 const 就加 const
)" << endl;

    // --------------------------------------------
    // 4.6 错误6：空容器的迭代器操作
    // --------------------------------------------
    cout << "4.6 错误6：空容器的迭代器操作" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v;        // 空容器
auto it = v.begin();  // it == v.end()
cout << *it;           // 未定义行为！没有元素可访问

✅ 正确代码：
vector<int> v;
if (!v.empty()) {
    auto it = v.begin();
    cout << *it;
}

// 或者更直接
if (v.size() > 0) {
    cout << v[0];
}

记忆技巧：
- 空容器：begin() == end()
- 使用迭代器前检查容器是否为空
- 下标访问同样需要检查
)" << endl;

    // --------------------------------------------
    // 4.7 错误7：用 != 比较不同容器的迭代器
    // --------------------------------------------
    cout << "4.7 错误7：比较不同容器的迭代器" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
❌ 错误代码：
vector<int> v1 = {1, 2, 3};
vector<int> v2 = {1, 2, 3};
auto it1 = v1.begin();
auto it2 = v2.begin();
if (it1 == it2) { ... }  // 未定义行为！不同容器的迭代器

✅ 正确代码：
// 只比较同一个容器内的迭代器
auto it1 = v1.begin();
auto end1 = v1.end();
if (it1 != end1) { ... }  // OK，同一个容器的迭代器

// 要比较元素值，解引用后比较
if (*v1.begin() == *v2.begin()) { ... }  // OK，比较值

记忆技巧：
- 迭代器只用于同一容器内的遍历和比较
- 跨容器比较 → 先解引用，再比值
)" << endl;

    return 0;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

int main_reference() {
    cout << R"(
============================================
迭代器函数卡片速查
============================================

【获取迭代器】

v.begin()    — 指向第一个元素
v.end()      — 指向最后一个之后
v.rbegin()   — 反向：指向最后一个元素
v.rend()     — 反向：指向第一个之前
v.cbegin()   — const 版 begin()
v.cend()     — const 版 end()

【迭代器操作】

*it          — 解引用，访问元素
it->member   — 访问元素成员（等价于 (*it).member）
it++         — 移动到下一个元素
it--         — 移动到上一个元素（双向以上）
it + n       — 前进 n 步（仅随机访问）
it - n       — 后退 n 步（仅随机访问）
it1 - it2    — 两个迭代器之间的距离

【辅助函数】（需要 #include <iterator>）

advance(it, n)          — 移动迭代器 n 步（任意类型）
next(it)               — 返回 it+1 的新迭代器
next(it, n)            — 返回 it+n 的新迭代器
prev(it)               — 返回 it-1 的新迭代器
prev(it, n)            — 返回 it-n 的新迭代器
distance(first, last)  — 两个迭代器之间的距离

【迭代器类型声明】

vector<int>::iterator              — 普通
vector<int>::const_iterator        — 只读
vector<int>::reverse_iterator      — 反向
vector<int>::const_reverse_iterator— 只读反向

// 实际中用 auto 代替，不需要写这么长：
auto it = v.begin();        // 编译器自动推导类型
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
   创建 vector<int> = {5, 3, 8, 1, 9, 2, 7}
   用迭代器完成：
   a) 遍历并输出所有元素
   b) 找到第一个大于 5 的元素并输出
   提示：遍历时用 *it > 5 判断

2. 代码纠错
   以下代码有 2 处错误，找出并修正：
   ```
   list<int> lst = {1, 2, 3};
   auto it = lst.begin() + 2;
   cout << *lst.end() << endl;
   ```

3. 思考题
   范围 for 和手动迭代器循环，什么时候必须用手动迭代器？
   提示：想想"遍历中需要删除元素"的情况

4. 反向遍历
   用反向迭代器（rbegin/rend）从后往前输出
   vector<string> = {"first", "second", "third", "fourth"}
   提示：rbegin() 指向最后一个元素，rend() 指向第一个之前

5. 综合练习
   创建 map<string, double> 存储商品名和价格，
   用迭代器遍历输出所有商品，然后找到价格最高的商品。
   提示：遍历比较 it->second 的大小
)" << endl;

    return 0;
}

// 编译命令：g++ -std=c++11 -o 60_iterator_basics 60_iterator_basics.cpp
// 运行方式：./60_iterator_basics 或 60_iterator_basics.exe
// C++版本：C++11（auto、范围 for、cbegin 需要）
