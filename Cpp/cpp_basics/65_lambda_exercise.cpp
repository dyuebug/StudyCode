// ============================================
// 65. Lambda 综合练习 - 超详细注释版
// 日期：2026-05-06（第20天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】

本文件是 Lambda 表达式的综合应用练习：
- Lambda 与算法配合（sort、find_if、count_if、for_each、transform）
- Lambda 捕获列表的实际应用
- Lambda 在实际场景中的使用

重点：通过实际案例掌握 Lambda 的使用技巧
思路：分析需求 → 选择算法 → 编写 Lambda → 组合实现
*/

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <numeric>
#include <functional>
using namespace std;

// 辅助函数：打印 vector
template<typename T>
void printVector(const vector<T>& v, const string& label = "") {
    if (!label.empty()) cout << label << "：";
    for (const T& val : v) cout << val << " ";
    cout << endl;
}

// ============================================
// 实战1：成绩分析系统（Lambda 版）
// ============================================

void exercise_grade_analysis() {
    cout << string(60, '=') << endl;
    cout << "实战1：成绩分析系统（Lambda 版）" << endl;
    cout << string(60, '=') << endl;

    // 数据
    vector<double> grades = {85.5, 90.0, 78.5, 92.5, 88.0,
                            65.0, 95.0, 72.5, 88.0, 55.0};

    printVector(grades, "原始成绩");

    // ------ 1. 计算平均分（lambda + accumulate） ------
    double sum = accumulate(grades.begin(), grades.end(), 0.0,
        [](double total, double grade) {
            return total + grade;
        }
    );
    double avg = sum / grades.size();
    cout << "平均分：" << avg << endl;

    // ------ 2. 最高分和最低分（lambda + min/max_element） ------
    auto max_it = max_element(grades.begin(), grades.end(),
        [](double a, double b) { return a < b; }
    );
    auto min_it = min_element(grades.begin(), grades.end(),
        [](double a, double b) { return a < b; }
    );
    cout << "最高分：" << *max_it << endl;
    cout << "最低分：" << *min_it << endl;

    // ------ 3. 按降序排序（lambda 自定义比较） ------
    sort(grades.begin(), grades.end(),
        [](double a, double b) { return a > b; }
    );
    printVector(grades, "降序排序");

    // ------ 4. 统计各等级人数（lambda + count_if） ------
    int excellent = count_if(grades.begin(), grades.end(),
        [](double g) { return g >= 90; }
    );
    int good = count_if(grades.begin(), grades.end(),
        [](double g) { return g >= 80 && g < 90; }
    );
    int pass = count_if(grades.begin(), grades.end(),
        [](double g) { return g >= 60 && g < 80; }
    );
    int fail = count_if(grades.begin(), grades.end(),
        [](double g) { return g < 60; }
    );

    cout << "优秀(>=90)：" << excellent << " 人" << endl;
    cout << "良好(80-89)：" << good << " 人" << endl;
    cout << "及格(60-79)：" << pass << " 人" << endl;
    cout << "不及格(<60)：" << fail << " 人" << endl;

    // ------ 5. 找到第一个不及格成绩（lambda + find_if） ------
    auto fail_it = find_if(grades.begin(), grades.end(),
        [](double g) { return g < 60; }
    );
    if (fail_it != grades.end()) {
        cout << "第一个不及格：" << *fail_it << endl;
    }

    // ------ 6. 成绩分段统计（lambda + map） ------
    map<string, int> levelCount;
    for_each(grades.begin(), grades.end(),
        [&levelCount](double g) {
            if (g >= 90) levelCount["优秀"]++;
            else if (g >= 80) levelCount["良好"]++;
            else if (g >= 60) levelCount["及格"]++;
            else levelCount["不及格"]++;
        }
    );
    cout << "\n分段统计：" << endl;
    for (const auto& p : levelCount) {
        cout << "  " << p.first << "：" << p.second << " 人" << endl;
    }

    cout << endl;
}

// ============================================
// 实战2：单词处理系统（Lambda 版）
// ============================================

void exercise_word_processing() {
    cout << string(60, '=') << endl;
    cout << "实战2：单词处理系统（Lambda 版）" << endl;
    cout << string(60, '=') << endl;

    vector<string> words = {
        "Hello", "World", "CPP", "STL", "Algorithm",
        "Lambda", "Function", "Object", "Template", "Vector"
    };

    printVector(words, "原始单词");

    // ------ 1. 转换为小写（lambda + transform） ------
    vector<string> lowerWords(words.size());
    transform(words.begin(), words.end(), lowerWords.begin(),
        [](string s) {
            transform(s.begin(), s.end(), s.begin(),
                [](char c) { return tolower(c); }
            );
            return s;
        }
    );
    printVector(lowerWords, "转小写");

    // ------ 2. 按长度排序（lambda + sort） ------
    vector<string> sortedByLength = words;
    sort(sortedByLength.begin(), sortedByLength.end(),
        [](const string& a, const string& b) {
            return a.size() < b.size();
        }
    );
    printVector(sortedByLength, "按长度排序");

    // ------ 3. 找最长单词（lambda + max_element） ------
    auto longest = max_element(words.begin(), words.end(),
        [](const string& a, const string& b) {
            return a.size() < b.size();
        }
    );
    cout << "最长单词：" << *longest << "（" << longest->size() << "个字符）" << endl;

    // ------ 4. 统计长度大于 5 的单词（lambda + count_if） ------
    int longWords = count_if(words.begin(), words.end(),
        [](const string& s) { return s.size() > 5; }
    );
    cout << "长度>5的单词：" << longWords << " 个" << endl;

    // ------ 5. 过滤出以 'S' 开头的单词（lambda + copy_if） ------
    vector<string> sWords;
    copy_if(words.begin(), words.end(), back_inserter(sWords),
        [](const string& s) { return s[0] == 'S'; }
    );
    printVector(sWords, "以S开头");

    // ------ 6. 词频统计（lambda + map） ------
    vector<string> allWords = {
        "hello", "world", "hello", "cpp", "stl",
        "hello", "world", "algorithm", "cpp", "stl"
    };

    map<string, int> wordCount;
    for_each(allWords.begin(), allWords.end(),
        [&wordCount](const string& w) { wordCount[w]++; }
    );
    cout << "\n词频统计：" << endl;
    for_each(wordCount.begin(), wordCount.end(),
        [](const pair<const string, int>& p) {
            cout << "  " << p.first << "：" << p.second << " 次" << endl;
        }
    );

    cout << endl;
}

// ============================================
// 实战3：数字处理系统（Lambda 版）
// ============================================

void exercise_number_processing() {
    cout << string(60, '=') << endl;
    cout << "实战3：数字处理系统（Lambda 版）" << endl;
    cout << string(60, '=') << endl;

    vector<int> numbers = {15, -3, 28, 7, -12, 5, 42, -8, 19, 3};

    printVector(numbers, "原始数据");

    // ------ 1. 分离正数和负数（lambda + copy_if） ------
    vector<int> positives, negatives;
    copy_if(numbers.begin(), numbers.end(), back_inserter(positives),
        [](int n) { return n > 0; }
    );
    copy_if(numbers.begin(), numbers.end(), back_inserter(negatives),
        [](int n) { return n < 0; }
    );
    printVector(positives, "正数");
    printVector(negatives, "负数");

    // ------ 2. 排序后去重（lambda + sort + unique） ------
    vector<int> dedup = numbers;
    sort(dedup.begin(), dedup.end(),
        [](int a, int b) { return a < b; }
    );
    dedup.erase(
        unique(dedup.begin(), dedup.end(),
            [](int a, int b) { return a == b; }
        ),
        dedup.end()
    );
    printVector(dedup, "排序去重");

    // ------ 3. 求和与平均值（lambda + accumulate） ------
    int total = accumulate(numbers.begin(), numbers.end(), 0,
        [](int sum, int n) { return sum + n; }
    );
    double average = (double)total / numbers.size();
    cout << "总和：" << total << "，平均值：" << average << endl;

    // ------ 4. 翻倍（lambda + transform） ------
    vector<int> doubled(numbers.size());
    transform(numbers.begin(), numbers.end(), doubled.begin(),
        [](int n) { return n * 2; }
    );
    printVector(doubled, "翻倍后");

    // ------ 5. 过滤并转换（组合 lambda） ------
    // 找出正数，然后求平方
    vector<int> posSquared;
    for_each(numbers.begin(), numbers.end(),
        [&posSquared](int n) {
            if (n > 0) {
                posSquared.push_back(n * n);
            }
        }
    );
    printVector(posSquared, "正数的平方");

    // ------ 6. 统计满足条件的个数（lambda + count_if） ------
    int divisibleBy3 = count_if(numbers.begin(), numbers.end(),
        [](int n) { return n % 3 == 0; }
    );
    cout << "能被3整除的数：" << divisibleBy3 << " 个" << endl;

    // ------ 7. 条件累加（lambda + accumulate） ------
    int posSum = accumulate(numbers.begin(), numbers.end(), 0,
        [](int sum, int n) {
            return n > 0 ? sum + n : sum;  // 只累加正数
        }
    );
    cout << "正数之和：" << posSum << endl;

    cout << endl;
}

// ============================================
// 实战4：学生管理系统（Lambda 版）
// ============================================

void exercise_student_management() {
    cout << string(60, '=') << endl;
    cout << "实战4：学生管理系统（Lambda 版）" << endl;
    cout << string(60, '=') << endl;

    struct Student {
        int id;
        string name;
        double score;
    };

    vector<Student> students = {
        {1001, "Alice", 85.5},
        {1002, "Bob", 92.0},
        {1003, "Charlie", 78.5},
        {1004, "Diana", 95.0},
        {1005, "Eve", 68.0},
        {1006, "Frank", 88.5}
    };

    // ------ 1. 按成绩降序排序（lambda + sort） ------
    sort(students.begin(), students.end(),
        [](const Student& a, const Student& b) {
            return a.score > b.score;
        }
    );
    cout << "按成绩降序：" << endl;
    for_each(students.begin(), students.end(),
        [](const Student& s) {
            cout << "  " << s.name << "：" << s.score << endl;
        }
    );

    // ------ 2. 找最高分学生（lambda + max_element） ------
    auto top = max_element(students.begin(), students.end(),
        [](const Student& a, const Student& b) {
            return a.score < b.score;
        }
    );
    cout << "\n最高分：" << top->name << "（" << top->score << "）" << endl;

    // ------ 3. 统计优秀学生（lambda + count_if） ------
    int excellent = count_if(students.begin(), students.end(),
        [](const Student& s) { return s.score >= 90; }
    );
    cout << "优秀学生(>=90)：" << excellent << " 人" << endl;

    // ------ 4. 查找特定学生（lambda + find_if） ------
    string targetName = "Charlie";
    auto found = find_if(students.begin(), students.end(),
        [&targetName](const Student& s) {
            return s.name == targetName;
        }
    );
    if (found != students.end()) {
        cout << "找到 " << targetName << "：" << found->score << endl;
    }

    // ------ 5. 计算平均分（lambda + accumulate） ------
    double totalScore = accumulate(students.begin(), students.end(), 0.0,
        [](double sum, const Student& s) {
            return sum + s.score;
        }
    );
    double avgScore = totalScore / students.size();
    cout << "平均分：" << avgScore << endl;

    // ------ 6. 按姓名排序（lambda + sort） ------
    sort(students.begin(), students.end(),
        [](const Student& a, const Student& b) {
            return a.name < b.name;
        }
    );
    cout << "\n按姓名排序：" << endl;
    for_each(students.begin(), students.end(),
        [](const Student& s) {
            cout << "  " << s.name << "：" << s.score << endl;
        }
    );

    // ------ 7. 过滤及格学生并排序（组合操作） ------
    vector<Student> passed;
    copy_if(students.begin(), students.end(), back_inserter(passed),
        [](const Student& s) { return s.score >= 60; }
    );
    sort(passed.begin(), passed.end(),
        [](const Student& a, const Student& b) {
            return a.score > b.score;
        }
    );
    cout << "\n及格学生（按成绩降序）：" << endl;
    for_each(passed.begin(), passed.end(),
        [](const Student& s) {
            cout << "  " << s.name << "：" << s.score << endl;
        }
    );

    cout << endl;
}

// ============================================
// 实战5：Lambda 高级用法
// ============================================

void exercise_advanced_lambda() {
    cout << string(60, '=') << endl;
    cout << "实战5：Lambda 高级用法" << endl;
    cout << string(60, '=') << endl;

    // ------ 1. Lambda 作为返回值 ------
    cout << "1. Lambda 作为返回值" << endl;
    cout << string(60, '-') << endl;

    // 创建一个"加法器"工厂函数
    auto createAdder = [](int base) {
        return [base](int x) { return base + x; };
    };

    auto add10 = createAdder(10);
    auto add100 = createAdder(100);
    cout << "add10(5) = " << add10(5) << endl;    // 15
    cout << "add100(5) = " << add100(5) << endl;  // 105

    // ------ 2. Lambda 与递归（使用 function） ------
    cout << "\n2. Lambda 与递归" << endl;
    cout << string(60, '-') << endl;

    // 阶乘
    function<int(int)> factorial = [&factorial](int n) {
        return n <= 1 ? 1 : n * factorial(n - 1);
    };
    cout << "5! = " << factorial(5) << endl;  // 120

    // 斐波那契
    function<int(int)> fibonacci = [&fibonacci](int n) {
        return n <= 1 ? n : fibonacci(n - 1) + fibonacci(n - 2);
    };
    cout << "fib(10) = " << fibonacci(10) << endl;  // 55

    // ------ 3. Lambda 与 STL 容器 ------
    cout << "\n3. Lambda 与 map" << endl;
    cout << string(60, '-') << endl;

    map<string, vector<int>> studentScores = {
        {"Alice", {85, 90, 78}},
        {"Bob", {92, 88, 95}},
        {"Charlie", {70, 65, 80}}
    };

    // 计算每个学生的平均分
    cout << "学生平均分：" << endl;
    for_each(studentScores.begin(), studentScores.end(),
        [](const pair<const string, vector<int>>& p) {
            double avg = accumulate(p.second.begin(), p.second.end(), 0.0)
                        / p.second.size();
            cout << "  " << p.first << "：" << avg << endl;
        }
    );

    // ------ 4. Lambda 与条件排序 ------
    cout << "\n4. 多条件排序" << endl;
    cout << string(60, '-') << endl;

    struct Employee {
        string name;
        int age;
        double salary;
    };

    vector<Employee> employees = {
        {"Alice", 30, 5000},
        {"Bob", 25, 4500},
        {"Charlie", 30, 5500},
        {"Diana", 28, 4800}
    };

    // 先按年龄升序，年龄相同按薪资降序
    sort(employees.begin(), employees.end(),
        [](const Employee& a, const Employee& b) {
            if (a.age != b.age) return a.age < b.age;
            return a.salary > b.salary;
        }
    );
    cout << "多条件排序（年龄升序，薪资降序）：" << endl;
    for_each(employees.begin(), employees.end(),
        [](const Employee& e) {
            cout << "  " << e.name << "：年龄" << e.age
                 << "，薪资" << e.salary << endl;
        }
    );

    // ------ 5. Lambda 与分组 ------
    cout << "\n5. 按条件分组" << endl;
    cout << string(60, '-') << endl;

    vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // 按奇偶分组
    map<string, vector<int>> groups;
    for_each(numbers.begin(), numbers.end(),
        [&groups](int n) {
            string key = (n % 2 == 0) ? "偶数" : "奇数";
            groups[key].push_back(n);
        }
    );
    cout << "奇偶分组：" << endl;
    for_each(groups.begin(), groups.end(),
        [](const pair<const string, vector<int>>& p) {
            cout << "  " << p.first << "：";
            for (int n : p.second) cout << n << " ";
            cout << endl;
        }
    );

    cout << endl;
}

// ============================================
// 第五部分：常见组合模式速查
// ============================================

int main_reference() {
    cout << R"(
============================================
Lambda + 算法 常见组合模式速查
============================================

【模式1：自定义排序】
sort(v.begin(), v.end(),
    [](const T& a, const T& b) { return a.field < b.field; }
);

【模式2：条件查找】
auto it = find_if(v.begin(), v.end(),
    [](const T& x) { return condition; }
);

【模式3：条件计数】
int n = count_if(v.begin(), v.end(),
    [](const T& x) { return condition; }
);

【模式4：条件累加】
T sum = accumulate(v.begin(), v.end(), init,
    [](T total, const T& x) {
        return condition ? total + x : total;
    }
);

【模式5：转换】
vector<U> result(v.size());
transform(v.begin(), v.end(), result.begin(),
    [](const T& x) { return transform_func(x); }
);

【模式6：过滤】
vector<T> filtered;
copy_if(v.begin(), v.end(), back_inserter(filtered),
    [](const T& x) { return condition; }
);

【模式7：遍历并修改外部变量】
int count = 0;
for_each(v.begin(), v.end(),
    [&count](const T& x) { if (condition) count++; }
);

【模式8：分组】
map<K, vector<T>> groups;
for_each(v.begin(), v.end(),
    [&groups](const T& x) {
        groups[key_func(x)].push_back(x);
    }
);

【模式9：工厂函数】
auto createFunc = [](参数) {
    return [捕获](参数) { return ...; };
};

【模式10：递归 Lambda】
function<返回类型(参数)> f = [&f](参数) {
    return base_case ? base : recursive_call(f(...));
};
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
1. 成绩统计
   给定 vector<double> scores = {78.5, 92.0, 65.5, 88.0, 95.5, 72.0, 83.5};
   a) 用 lambda + accumulate 计算平均分
   b) 用 lambda + count_if 统计各等级人数
   c) 用 lambda + find_if 找到第一个不及格成绩
   d) 用 lambda + sort 按降序排序
   提示：组合使用多个 lambda

2. 单词处理
   给定 vector<string> words = {"apple", "Banana", "CHERRY", "date", "Elderberry"}
   a) 用 lambda + transform 转换为小写
   b) 用 lambda + sort 按长度排序
   c) 用 lambda + copy_if 过滤出长度大于 4 的单词
   d) 用 lambda + count_if 统计以元音字母开头的单词
   提示：元音字母是 a, e, i, o, u

3. 数字处理
   给定 vector<int> nums = {3, -1, 7, -5, 2, -8, 4, 6, -3, 9}
   a) 用 lambda + copy_if 分离正数和负数
   b) 用 lambda + accumulate 只累加正数
   c) 用 lambda + transform 计算每个数的平方
   d) 用 lambda + count_if 统计能被 3 整除的个数
   提示：注意 lambda 的捕获列表

4. 学生管理
   定义 Student 结构体（name, score）
   给定 vector<Student> students
   a) 用 lambda + sort 按成绩降序排序
   b) 用 lambda + max_element 找最高分学生
   c) 用 lambda + for_each 打印所有学生
   d) 用 lambda + count_if 统计优秀学生（>=90）
   提示：lambda 参数用 const 引用

5. 高级练习：工厂函数
   编写一个 createMultiplier 工厂函数：
   auto createMultiplier = [](int factor) {
       return [factor](int x) { return factor * x; };
   };
   然后创建 doubleIt = createMultiplier(2)
             tripleIt = createMultiplier(3)
   测试它们的功能
   提示：lambda 返回 lambda

6. 高级练习：递归 Lambda
   用 lambda + function 实现斐波那契数列：
   function<int(int)> fib = [&fib](int n) {
       return n <= 1 ? n : fib(n-1) + fib(n-2);
   };
   测试 fib(10) 的结果
   提示：lambda 捕获自身需要 function 包装
)"" << endl;

    return 0;
}

// ============================================
// main 函数：运行所有实战
// ============================================

// 常见错误和陷阱
void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;
    cout << R"(
❌ 错误1：按引用捕获局部变量，lambda 逃出作用域（悬空引用）
   auto getAdder(int base) {
       return [&base](int x){ return base + x; };  // ❌ base 已析构
   }
   ✅ 按值捕获：[base](int x){ return base + x; }

❌ 错误2：在 const 成员函数中捕获 this 后修改成员（编译错误）
   void foo() const { auto f = [this](){ data = 1; }; }  // ❌
   ✅ 去掉 const 或不修改成员

❌ 错误3：sort 的比较 lambda 返回 bool 时写成 <=（违反严格弱序）
   sort(v.begin(), v.end(), [](int a, int b){ return a <= b; });  // ❌
   ✅ 严格小于：return a < b;

❌ 错误4：lambda 捕获 [=] 捕获了 this（隐式捕获指针，不是对象）
   class Foo { void bar() { auto f = [=](){ use(data); }; } };
   // [=] 在成员函数中捕获的是 this 指针，不是对象副本
   ✅ 明确写 [data]（C++14）或 [*this]（C++17）

❌ 错误5：mutable lambda 忘记加 mutable 关键字
   int cnt = 0;
   auto f = [cnt](){ ++cnt; };  // ❌ 编译错误：cnt 是 const 副本
   ✅ auto f = [cnt]() mutable { ++cnt; };
)" << endl;
}

int main() {
    exercise_grade_analysis();
    exercise_word_processing();
    exercise_number_processing();
    exercise_student_management();
    exercise_advanced_lambda();

    cout << string(60, '=') << endl;
    cout << "所有 Lambda 综合练习运行完毕！" << endl;
    cout << string(60, '=') << endl;

    return 0;
}

// 编译命令：g++ -std=c++11 -o 65_lambda_exercise 65_lambda_exercise.cpp
// 运行方式：./65_lambda_exercise 或 65_lambda_exercise.exe
// C++版本：C++11（lambda、function、auto 需要）
