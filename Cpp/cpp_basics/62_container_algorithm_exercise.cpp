// ============================================
// 62. 容器与算法综合练习 - 超详细注释版
// 日期：2026-05-04（第19天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】

本文件是前面所学内容的综合应用：
- 容器：vector、map、set
- 算法：sort、find、count、accumulate、for_each、transform
- 迭代器：begin/end、范围 for

重点：学会组合使用工具解决实际问题
思路：先分析需求 → 选容器 → 选算法 → 组合实现
*/

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <numeric>
using namespace std;

// 辅助函数：打印 vector（复用，避免重复代码）
void printVector(const vector<int>& v, const string& label = "") {
    if (!label.empty()) cout << label << "：";
    for (int val : v) cout << val << " ";
    cout << endl;
}

void printVectorD(const vector<double>& v, const string& label = "") {
    if (!label.empty()) cout << label << "：";
    for (double val : v) cout << val << " ";
    cout << endl;
}

// ============================================
// 实战1：成绩分析系统
// ============================================

void exercise_grade_analysis() {
    cout << string(60, '=') << endl;
    cout << "实战1：成绩分析系统" << endl;
    cout << string(60, '=') << endl;

    // 数据：10 个学生的成绩
    vector<double> grades = {85.5, 90.0, 78.5, 92.5, 88.0,
                            65.0, 95.0, 72.5, 88.0, 55.0};

    printVectorD(grades, "原始成绩");

    // ------ 1. 计算平均分 ------
    // accumulate：从初始值 0.0 开始累加
    // 注意初始值 0.0（double），不是 0（int）
    double sum = accumulate(grades.begin(), grades.end(), 0.0);
    double avg = sum / grades.size();
    cout << "平均分：" << avg << endl;  // 82.05

    // ------ 2. 最高分和最低分 ------
    // min_element / max_element 返回迭代器，需要解引用
    double max_grade = *max_element(grades.begin(), grades.end());
    double min_grade = *min_element(grades.begin(), grades.end());
    cout << "最高分：" << max_grade << endl;  // 95
    cout << "最低分：" << min_grade << endl;  // 55

    // ------ 3. 按降序排序 ------
    sort(grades.begin(), grades.end(), greater<double>());
    printVectorD(grades, "降序排序");

    // ------ 4. 统计各等级人数 ------
    // count_if + lambda 条件判断
    int excellent = count_if(grades.begin(), grades.end(),
        [](double g) { return g >= 90; });           // 优秀 >= 90
    int good = count_if(grades.begin(), grades.end(),
        [](double g) { return g >= 80 && g < 90; });  // 良好 80-89
    int pass = count_if(grades.begin(), grades.end(),
        [](double g) { return g >= 60 && g < 80; });  // 及格 60-79
    int fail = count_if(grades.begin(), grades.end(),
        [](double g) { return g < 60; });              // 不及格 < 60

    cout << "优秀(>=90)：" << excellent << " 人" << endl;
    cout << "良好(80-89)：" << good << " 人" << endl;
    cout << "及格(60-79)：" << pass << " 人" << endl;
    cout << "不及格(<60)：" << fail << " 人" << endl;

    // ------ 5. 成绩翻倍（transform 演示） ------
    vector<double> doubled(grades.size());
    transform(grades.begin(), grades.end(), doubled.begin(),
        [](double g) { return g * 2; });
    printVectorD(doubled, "翻倍（纯练习）");

    cout << endl;
}

// ============================================
// 实战2：单词统计
// ============================================

void exercise_word_count() {
    cout << string(60, '=') << endl;
    cout << "实战2：单词统计" << endl;
    cout << string(60, '=') << endl;

    // 数据：一段文本（用 vector 模拟分词结果）
    vector<string> words = {
        "hello", "world", "hello", "cpp",
        "stl", "hello", "world", "algorithm",
        "cpp", "stl", "vector", "cpp"
    };

    cout << "原始单词（共 " << words.size() << " 个）：" << endl;
    for (const string& w : words) cout << w << " ";
    cout << endl;

    // ------ 1. 统计每个单词出现次数（map） ------
    map<string, int> word_count;
    for (const string& w : words) {
        word_count[w]++;  // 不存在自动插入 0，然后 +1
    }

    cout << "\n词频统计：" << endl;
    for (const auto& p : word_count) {
        cout << "  " << p.first << "：" << p.second << " 次" << endl;
    }

    // ------ 2. 去重（set） ------
    set<string> unique_words(words.begin(), words.end());
    cout << "\n不重复单词（共 " << unique_words.size() << " 个）：" << endl;
    for (const string& w : unique_words) cout << w << " ";
    cout << endl;

    // ------ 3. 找出现次数 >= 2 的单词 ------
    cout << "\n高频词（>=2次）：" << endl;
    for (const auto& p : word_count) {
        if (p.second >= 2) {
            cout << "  " << p.first << "：" << p.second << " 次" << endl;
        }
    }

    // ------ 4. 检查某个单词是否存在 ------
    string target = "algorithm";
    if (word_count.count(target)) {
        cout << "\n\"" << target << "\" 出现了 " << word_count[target] << " 次" << endl;
    } else {
        cout << "\n\"" << target << "\" 不存在" << endl;
    }

    // ------ 5. 排序单词（vector + sort） ------
    vector<string> sorted_words = words;  // 拷贝一份
    sort(sorted_words.begin(), sorted_words.end());
    cout << "\n排序后：";
    for (const string& w : sorted_words) cout << w << " ";
    cout << endl;

    // sort + unique + erase 去重（不同于 set 去重的方式）
    sort(sorted_words.begin(), sorted_words.end());
    auto last = unique(sorted_words.begin(), sorted_words.end());
    sorted_words.erase(last, sorted_words.end());
    cout << "sort+unique 去重：";
    for (const string& w : sorted_words) cout << w << " ";
    cout << endl;

    cout << endl;
}

// ============================================
// 实战3：数字处理
// ============================================

void exercise_number_processing() {
    cout << string(60, '=') << endl;
    cout << "实战3：数字处理" << endl;
    cout << string(60, '=') << endl;

    vector<int> numbers = {15, -3, 28, 7, -12, 5, 42, -8, 19, 3};

    printVector(numbers, "原始数据");

    // ------ 1. 分离正数和负数 ------
    vector<int> positives, negatives;

    // 用 copy_if 分别收集
    copy_if(numbers.begin(), numbers.end(),
            back_inserter(positives),
            [](int n) { return n > 0; });
    copy_if(numbers.begin(), numbers.end(),
            back_inserter(negatives),
            [](int n) { return n < 0; });

    printVector(positives, "正数");
    printVector(negatives, "负数");

    // ------ 2. 排序后去重 ------
    vector<int> dedup = numbers;
    sort(dedup.begin(), dedup.end());
    dedup.erase(unique(dedup.begin(), dedup.end()), dedup.end());
    printVector(dedup, "排序去重");

    // ------ 3. 找最大/最小 ------
    int max_val = *max_element(numbers.begin(), numbers.end());
    int min_val = *min_element(numbers.begin(), numbers.end());
    cout << "最大值：" << max_val << "，最小值：" << min_val << endl;

    // ------ 4. 求和与平均值 ------
    int total = accumulate(numbers.begin(), numbers.end(), 0);
    double average = (double)total / numbers.size();  // 强制 double 除法
    cout << "总和：" << total << "，平均值：" << average << endl;

    // ------ 5. 所有元素翻倍（transform） ------
    transform(numbers.begin(), numbers.end(), numbers.begin(),
        [](int n) { return n * 2; });
    printVector(numbers, "翻倍后");

    // ------ 6. 判断是否全为正数 ------
    bool all_pos = all_of(numbers.begin(), numbers.end(),
        [](int n) { return n > 0; });
    cout << "是否全为正数：" << (all_pos ? "是" : "否") << endl;

    // ------ 7. 是否存在负数 ------
    bool has_neg = any_of(numbers.begin(), numbers.end(),
        [](int n) { return n < 0; });
    cout << "是否有负数：" << (has_neg ? "是" : "否") << endl;

    // ------ 8. 是否全不为零 ------
    bool none_zero = none_of(numbers.begin(), numbers.end(),
        [](int n) { return n == 0; });
    cout << "是否全不为零：" << (none_zero ? "是" : "否") << endl;

    cout << endl;
}

// ============================================
// 实战4：简易学生管理
// ============================================

void exercise_student_management() {
    cout << string(60, '=') << endl;
    cout << "实战4：简易学生管理" << endl;
    cout << string(60, '=') << endl;

    // 用 map 存储：学号 → 姓名
    map<int, string> students;

    // ------ 1. 添加学生 ------
    students[1001] = "Alice";
    students[1002] = "Bob";
    students[1003] = "Charlie";
    students[1004] = "Diana";

    // ------ 2. 显示所有学生 ------
    cout << "学生列表：" << endl;
    for (const auto& p : students) {
        cout << "  学号：" << p.first << "，姓名：" << p.second << endl;
    }

    // ------ 3. 查找学生 ------
    int target_id = 1003;
    auto it = students.find(target_id);
    if (it != students.end()) {
        cout << "\n找到学号 " << target_id << "：" << it->second << endl;
    }

    // ------ 4. 用学号组成 set（演示 set 用法） ------
    set<int> ids;
    for (const auto& p : students) {
        ids.insert(p.first);
    }
    cout << "\n学号集合：";
    for (int id : ids) cout << id << " ";
    cout << endl;

    // ------ 5. 检查学号是否已存在 ------
    int new_id = 1002;
    if (ids.count(new_id)) {
        cout << "学号 " << new_id << " 已存在，不能重复添加" << endl;
    }

    // ------ 6. 用 vector 存储姓名并排序 ------
    vector<string> names;
    for (const auto& p : students) {
        names.push_back(p.second);
    }
    sort(names.begin(), names.end());
    cout << "按姓名排序：";
    for (const string& name : names) cout << name << " ";
    cout << endl;

    cout << endl;
}

// ============================================
// 第五部分：常见组合模式速查
// ============================================

int main_reference() {
    cout << R"(
============================================
容器 + 算法 常见组合模式速查
============================================

【模式1：排序 + 输出】
vector<int> v = {...};
sort(v.begin(), v.end());
for (int val : v) cout << val;

【模式2：查找 + 处理】
auto it = find(v.begin(), v.end(), target);
if (it != v.end()) { ... }

【模式3：条件统计】
int n = count_if(v.begin(), v.end(), [](int x){ return condition; });

【模式4：累加求平均】
double sum = accumulate(v.begin(), v.end(), 0.0);
double avg = sum / v.size();

【模式5：去重】
sort(v.begin(), v.end());
v.erase(unique(v.begin(), v.end()), v.end());

// 或用 set
set<T> s(v.begin(), v.end());
vector<T> clean(s.begin(), s.end());

【模式6：词频统计】
map<K, int> freq;
for (const K& key : data) freq[key]++;

【模式7：过滤】
vector<T> filtered;
copy_if(src.begin(), src.end(), back_inserter(filtered),
    [](const T& x){ return condition; });

【模式8：转换】
vector<T> result(src.size());
transform(src.begin(), src.end(), result.begin(), transform_func);

【模式9：极值】
T max_val = *max_element(v.begin(), v.end());
T min_val = *min_element(v.begin(), v.end());

【模式10：全部/任意/无】
all_of(first, last, pred)     — 是否全部满足
any_of(first, last, pred)     — 是否存在满足
none_of(first, last, pred)    — 是否全不满足
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
1. 综合练习：购物清单
   创建 vector<string> 存储购物清单：
   {"apple", "bread", "milk", "apple", "eggs", "bread", "cheese"}
   a) 用 map 统计每个商品出现次数
   b) 用 set 得去重后的商品列表
   c) 按字母排序输出所有商品
   提示：word_count 模式 + set 构造 + sort

2. 综合练习：温度分析
   给定 vector<double> = {23.5, 18.2, 30.1, 15.8, 28.7, 22.4, 35.2}
   a) 计算平均温度
   b) 找出最高温和最低温
   c) 统计高于 25 度的天数
   d) 按温度从高到低排序
   提示：accumulate + min/max_element + count_if + sort(greater)

3. 综合练习：字符串处理
   给定 vector<string> = {"Hello", "World", "CPP", "STL", "Algorithm"}
   a) 用 transform 将所有字符串转为小写（tolower）
   b) 找到长度最长的字符串
   c) 按字符串长度排序
   提示：transform + lambda + sort + lambda 自定义比较

4. 设计题
   你需要实现一个通讯录，支持：
   - 添加联系人（姓名+电话）
   - 按姓名查找电话
   - 删除联系人
   - 按姓名排序输出所有联系人
   问：用什么容器组合？为什么？
   提示：想想查找效率和排序需求
)" << endl;

    return 0;
}

// ============================================
// 第五部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：map 用 [] 访问不存在的键（自动插入 0）

   map<string, int> freq;
   freq["apple"]++;          // 正确：先插入 0 再 +1

   // 误用：只想查询，不想插入
   if (freq["banana"] > 0)   // ❌ 访问 "banana" 时自动插入了 freq["banana"]=0
       cout << "exists";

   // 结果：freq 里多了一个 "banana":0 的键值对

   ✅ 正确：查询用 count() 或 find()
   if (freq.count("banana"))    // count() 不会插入
       cout << "exists";
   if (freq.find("banana") != freq.end())  // find() 也不插入

------------------------------------------------------------

❌ 错误2：erase + unique 忘记 sort（unique 只消除相邻重复）

   vector<int> v = {3, 1, 2, 1, 3};
   // 直接 unique，不 sort
   auto last = unique(v.begin(), v.end());
   v.erase(last, v.end());
   // 结果：3 1 2 3（"1"只消了相邻的，"3"第一第二不相邻，保留了）

   ✅ 正确：sort 后再 unique
   sort(v.begin(), v.end());           // 先排序：1 1 2 3 3
   auto last = unique(v.begin(), v.end()); // 消相邻重复：1 2 3
   v.erase(last, v.end());

------------------------------------------------------------

❌ 错误3：accumulate 初始值类型错误导致整数溢出

   vector<double> grades = {85.5, 90.0, 78.5};
   double sum = accumulate(grades.begin(), grades.end(), 0);  // ❌ 初始值是 int 0
   // 内部计算：0 + 85.5 → 先把 85.5 截断为 85（int 运算），再加...

   ✅ 正确：初始值类型要匹配
   double sum = accumulate(grades.begin(), grades.end(), 0.0);  // 0.0 是 double

------------------------------------------------------------

❌ 错误4：在 range-for 中直接修改容器大小

   vector<int> v = {1, 2, 3, 4, 5};
   for (int x : v) {
       if (x == 3) v.push_back(6);  // ❌ 迭代中 push_back 可能触发 reallocation
   }                                 // 导致迭代器失效，行为未定义

   ✅ 正确：先收集要添加的元素，循环结束后再插入
   vector<int> toAdd;
   for (int x : v) {
       if (x == 3) toAdd.push_back(6);
   }
   v.insert(v.end(), toAdd.begin(), toAdd.end());

------------------------------------------------------------

❌ 错误5：max_element / min_element 对空容器解引用

   vector<int> v;  // 空容器
   int maxVal = *max_element(v.begin(), v.end());  // ❌ 解引用 end() 迭代器 → 崩溃

   ✅ 正确：先检查是否为空
   if (!v.empty()) {
       int maxVal = *max_element(v.begin(), v.end());
   }

------------------------------------------------------------

❌ 错误6：copy_if 目标容器未预留空间

   vector<int> src = {1, 2, 3, 4, 5};
   vector<int> dst;
   copy_if(src.begin(), src.end(), dst.begin(),  // ❌ dst 是空的，写入越界
       [](int x){ return x > 2; });

   ✅ 正确：用 back_inserter 自动扩展
   copy_if(src.begin(), src.end(), back_inserter(dst),
       [](int x){ return x > 2; });

------------------------------------------------------------

❌ 错误7：set 不能直接修改元素（所有元素是 const）

   set<int> s = {1, 2, 3};
   for (int& x : s) x = x * 2;  // ❌ 编译错误：set 元素是只读的

   ✅ 正确：删除旧值，插入新值
   set<int> newS;
   for (int x : s) newS.insert(x * 2);
   s = newS;
)" << endl;
}

// ============================================
// main 函数：运行所有实战
// ============================================

int main() {
    exercise_grade_analysis();
    exercise_word_count();
    exercise_number_processing();
    exercise_student_management();
    showCommonErrors();

    cout << string(60, '=') << endl;
    cout << "所有实战练习运行完毕！" << endl;
    cout << string(60, '=') << endl;

    return 0;
}

// 编译命令：g++ -std=c++11 -o 62_container_algorithm_exercise 62_container_algorithm_exercise.cpp
// 运行方式：./62_container_algorithm_exercise 或 62_container_algorithm_exercise.exe
// C++版本：C++11（lambda、auto、back_inserter、范围 for 需要）
