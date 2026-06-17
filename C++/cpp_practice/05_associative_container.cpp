// ============================================
// 05_associative_container.cpp - 关联式容器练习
// 日期：2026-06-18（第5天）
// 内容：set、map、迭代器
// ============================================

#include <iostream>
#include <set>
#include <map>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::set;
using std::map;
using std::string;
using std::pair;
using std::make_pair;
using std::vector;

// ============================================
// 练习1：set 基本操作
// ============================================

void test_set_basic() {
    cout << "\n=== 练习1：set 基本操作 ===" << endl;

    // 1. 创建 set（4种方式）
    set<int> s1;                        // 无参构造
    set<int> s2 = {3, 1, 4, 1, 5, 9};  // 初始化列表
    set<int> s3(s2);                    // 拷贝构造

    int arr[] = {2, 7, 1, 8};
    set<int> s4(arr, arr + 4);          // 迭代器构造

    // 2. 验证 set 的特征：唯一、有序
    cout << "s2 = {3, 1, 4, 1, 5, 9}，实际存储：" << endl;
    cout << "s2: ";
    for (const auto& elem : s2) {
        cout << elem << " ";
    }
    cout << endl;
    cout << "特征：自动去重（两个1只保留一个）、自动排序（升序）" << endl;

    // 3. 插入元素
    cout << "\n插入元素测试：" << endl;
    auto ret1 = s2.insert(6);
    if (ret1.second) {
        cout << "✅ 插入 6 成功，值为：" << *(ret1.first) << endl;
    }

    auto ret2 = s2.insert(3);  // 3 已存在
    if (!ret2.second) {
        cout << "❌ 插入 3 失败，元素已存在" << endl;
    }

    // 4. 查找元素
    cout << "\n查找元素测试：" << endl;

    // 方式1：find()
    auto it = s2.find(4);
    if (it != s2.end()) {
        cout << "✅ find(4)：找到元素 " << *it << endl;
    }

    it = s2.find(10);
    if (it == s2.end()) {
        cout << "❌ find(10)：未找到" << endl;
    }

    // 方式2：count()
    if (s2.count(5)) {
        cout << "✅ count(5)：元素存在" << endl;
    }
    if (!s2.count(10)) {
        cout << "❌ count(10)：元素不存在" << endl;
    }

    // 5. 删除元素
    cout << "\n删除前 s2 大小：" << s2.size() << endl;
    s2.erase(1);
    cout << "删除元素 1 后，s2 大小：" << s2.size() << endl;

    // 6. 遍历 set
    cout << "\n遍历 s2：";
    for (const auto& elem : s2) {
        cout << elem << " ";
    }
    cout << endl;
}

// ============================================
// 练习2：pair 的使用
// ============================================

void test_pair() {
    cout << "\n=== 练习2：pair 的使用 ===" << endl;

    // 创建 pair 的三种方式
    pair<int, string> p1(1, "hello");
    pair<int, string> p2 = {2, "world"};
    pair<int, string> p3 = make_pair(3, "c++");

    // 访问 pair 的成员
    cout << "p1: " << p1.first << " -> " << p1.second << endl;
    cout << "p2: " << p2.first << " -> " << p2.second << endl;
    cout << "p3: " << p3.first << " -> " << p3.second << endl;
}

// ============================================
// 练习3：map 基本操作
// ============================================

void test_map_basic() {
    cout << "\n=== 练习3：map 基本操作 ===" << endl;

    // 1. 创建 map
    map<int, string> m = {
        {1, "apple"},
        {2, "banana"},
        {3, "cherry"},
        pair<int, string>(4, "date"),
        make_pair(5, "elderberry"),
        {3, "duplicate"}  // key=3 重复，会被忽略
    };

    // 2. 验证 map 的特征：键唯一、按键排序
    cout << "map 内容（按 key 排序）：" << endl;
    for (const auto& p : m) {
        cout << p.first << " -> " << p.second << endl;
    }

    // 3. 插入元素
    cout << "\n插入元素测试：" << endl;
    auto ret1 = m.insert(make_pair(6, "fig"));
    if (ret1.second) {
        cout << "✅ 插入 {6, \"fig\"} 成功" << endl;
    }

    auto ret2 = m.insert({2, "new_banana"});
    if (!ret2.second) {
        cout << "❌ 插入 {2, \"new_banana\"} 失败，key=2 已存在" << endl;
        cout << "原有值：" << ret2.first->second << endl;
    }

    // 4. 查找元素
    cout << "\n查找元素测试：" << endl;

    // 方式1：find()
    auto it = m.find(3);
    if (it != m.end()) {
        cout << "✅ find(3)：" << it->first << " -> " << it->second << endl;
    }

    // 方式2：count()
    if (m.count(4)) {
        cout << "✅ count(4)：key 存在" << endl;
    }

    // 5. 下标操作 [] ⭐⭐⭐⭐⭐
    cout << "\n下标操作 [] 测试：" << endl;
    cout << "m[1] = " << m[1] << endl;  // 访问存在的 key

    m[2] = "BANANA";  // 修改存在的 key 的 value
    cout << "修改后 m[2] = " << m[2] << endl;

    cout << "访问不存在的 key：m[10] = \"" << m[10] << "\"" << endl;
    cout << "注意：m[10] 不存在，会自动插入 {10, \"\"}（默认值）" << endl;
    cout << "现在 m.size() = " << m.size() << endl;

    // 6. 删除元素
    m.erase(10);  // 删除刚才自动插入的
    cout << "删除 key=10 后，m.size() = " << m.size() << endl;

    // 7. 遍历 map
    cout << "\n遍历 map：" << endl;
    for (const auto& pair : m) {
        cout << pair.first << " -> " << pair.second << endl;
    }
}

// ============================================
// 练习4：实际应用 - 单词计数
// ============================================

void test_word_count() {
    cout << "\n=== 练习4：单词计数（map 应用）===" << endl;

    string text = "hello world hello c++ world hello";
    vector<string> words = {"hello", "world", "hello", "c++", "world", "hello"};

    // 使用 map 统计每个单词出现的次数
    map<string, int> word_count;

    for (const auto& word : words) {
        word_count[word]++;  // 如果 word 不存在，自动插入 {word, 0}，然后 ++
    }

    cout << "单词统计结果：" << endl;
    for (const auto& pair : word_count) {
        cout << pair.first << ": " << pair.second << " 次" << endl;
    }
}

// ============================================
// 练习5：实际应用 - 去重排序
// ============================================

void test_unique_sort() {
    cout << "\n=== 练习5：去重排序（set 应用）===" << endl;

    vector<int> nums = {5, 2, 8, 2, 9, 1, 5, 8, 3};

    cout << "原始数组：";
    for (int num : nums) {
        cout << num << " ";
    }
    cout << endl;

    // 使用 set 去重并排序
    set<int> unique_nums(nums.begin(), nums.end());

    cout << "去重排序后：";
    for (int num : unique_nums) {
        cout << num << " ";
    }
    cout << endl;
}

// ============================================
// 练习6：学生成绩管理（map 应用）
// ============================================

void test_student_grades() {
    cout << "\n=== 练习6：学生成绩管理（map 应用）===" << endl;

    // 学号 -> 成绩
    map<string, int> grades;

    // 添加学生成绩
    grades["2024001"] = 95;
    grades["2024002"] = 88;
    grades["2024003"] = 92;
    grades["2024004"] = 85;

    cout << "学生成绩表：" << endl;
    for (const auto& pair : grades) {
        cout << "学号 " << pair.first << ": " << pair.second << " 分" << endl;
    }

    // 查找学生成绩
    string id = "2024002";
    auto it = grades.find(id);
    if (it != grades.end()) {
        cout << "\n查询学号 " << id << " 的成绩：" << it->second << " 分" << endl;
    }

    // 修改学生成绩
    grades["2024002"] = 90;
    cout << "修改后学号 " << id << " 的成绩：" << grades[id] << " 分" << endl;

    // 计算平均分
    int total = 0;
    for (const auto& pair : grades) {
        total += pair.second;
    }
    double average = static_cast<double>(total) / grades.size();
    cout << "\n班级平均分：" << average << " 分" << endl;
}

// ============================================
// 练习7：迭代器的使用
// ============================================

void test_iterator() {
    cout << "\n=== 练习7：迭代器的使用 ===" << endl;

    set<int> s = {1, 3, 5, 7, 9};

    // 方式1：传统迭代器
    cout << "方式1：传统迭代器遍历 set：";
    for (auto it = s.begin(); it != s.end(); ++it) {
        cout << *it << " ";  // 解引用访问元素
    }
    cout << endl;

    // 方式2：C++11 范围 for（推荐）
    cout << "方式2：范围 for 遍历 set：";
    for (const auto& elem : s) {
        cout << elem << " ";
    }
    cout << endl;

    // map 的迭代器
    map<int, string> m = {{1, "one"}, {2, "two"}, {3, "three"}};

    cout << "\n遍历 map：" << endl;
    for (auto it = m.begin(); it != m.end(); ++it) {
        cout << it->first << " -> " << it->second << endl;
    }
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "========================================" << endl;
    cout << "     关联式容器综合练习" << endl;
    cout << "========================================" << endl;

    // 练习1：set 基本操作
    test_set_basic();

    // 练习2：pair 的使用
    test_pair();

    // 练习3：map 基本操作
    test_map_basic();

    // 练习4：单词计数
    test_word_count();

    // 练习5：去重排序
    test_unique_sort();

    // 练习6：学生成绩管理
    test_student_grades();

    // 练习7：迭代器的使用
    test_iterator();

    cout << "\n========================================" << endl;
    cout << "     所有练习完成！" << endl;
    cout << "========================================" << endl;

    return 0;
}

/*
编译运行：
g++ -o 05_associative_container 05_associative_container.cpp -std=c++17
./05_associative_container

关键知识点验证：
1. set：唯一、有序
2. map：键值对、按键排序
3. pair：存储两个值
4. 迭代器：遍历容器
5. 实际应用：单词计数、去重排序、学生成绩管理

核心理解：
- set 自动去重、自动排序
- map 的 [] 操作符：key 不存在会自动插入
- 迭代器是遍历容器的统一接口
- 底层都是红黑树，查找效率 O(log n)
*/
