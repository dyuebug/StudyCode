/*
 * ============================================================================
 * 文件名：20_string_class.cpp
 * 主题：C++ string类详解
 * 学习目标：
 *   1. 理解C++ string类的优势
 *   2. 掌握string的声明和初始化
 *   3. 学会使用string的常用操作（拼接、比较、查找、截取）
 *   4. 理解string与C风格字符串的转换
 *   5. 对比string类与C风格字符串的优缺点
 *
 * 编译命令：g++ -std=c++11 -o 20_string_class 20_string_class.cpp
 * 运行命令：./20_string_class (Linux/Mac) 或 20_string_class.exe (Windows)
 * ============================================================================
 */

#include <iostream>
#include <string>   // C++ string类
#include <cstring>  // C风格字符串函数（用于对比）
using namespace std;

int main() {
    cout << "=== C++ string类详解 ===" << endl << endl;

    // ========================================
    // 1. 为什么需要string类？
    // ========================================
    cout << "【1. 为什么需要string类？】" << endl;
    cout << "C风格字符串的问题：" << endl;
    cout << "  ✗ 需要手动管理内存大小" << endl;
    cout << "  ✗ 容易发生缓冲区溢出" << endl;
    cout << "  ✗ 操作复杂（需要使用函数）" << endl;
    cout << "  ✗ 不能直接用+拼接、==比较" << endl;
    cout << endl;

    cout << "C++ string类的优势：" << endl;
    cout << "  ✓ 自动管理内存" << endl;
    cout << "  ✓ 安全（不会溢出）" << endl;
    cout << "  ✓ 操作简单（支持运算符）" << endl;
    cout << "  ✓ 功能丰富（内置大量方法）" << endl;
    cout << endl;

    // ========================================
    // 2. string的声明和初始化
    // ========================================
    cout << "【2. string的声明和初始化】" << endl;

    // 方式1：直接初始化
    string str1 = "Hello";
    cout << "方式1：string str1 = \"Hello\" → " << str1 << endl;

    // 方式2：构造函数初始化
    string str2("World");
    cout << "方式2：string str2(\"World\") → " << str2 << endl;

    // 方式3：空字符串
    string str3;
    cout << "方式3：string str3（空字符串）→ \"" << str3 << "\"" << endl;

    // 方式4：重复字符
    string str4(5, '*');  // 5个'*'
    cout << "方式4：string str4(5, '*') → " << str4 << endl;

    // 方式5：从C风格字符串初始化
    char c_str[] = "C-Style";
    string str5(c_str);
    cout << "方式5：从C风格字符串 → " << str5 << endl;
    cout << endl;

    // ========================================
    // 3. string的基本操作
    // ========================================
    cout << "【3. string的基本操作】" << endl;

    string s = "C++";

    // 获取长度
    cout << "s.length() = " << s.length() << endl;
    cout << "s.size() = " << s.size() << " (与length()相同)" << endl;

    // 判断是否为空
    cout << "s.empty() = " << (s.empty() ? "true" : "false") << endl;

    string empty_str;
    cout << "empty_str.empty() = " << (empty_str.empty() ? "true" : "false") << endl;

    // 清空字符串
    string temp = "Temporary";
    cout << "清空前：temp = \"" << temp << "\"" << endl;
    temp.clear();
    cout << "清空后：temp = \"" << temp << "\"" << endl;
    cout << endl;

    // ========================================
    // 4. 字符串拼接（超级方便！）
    // ========================================
    cout << "【4. 字符串拼接】" << endl;

    string first = "Hello";
    string second = "World";

    // 方式1：使用+运算符
    string result1 = first + " " + second;
    cout << "使用+运算符：" << result1 << endl;

    // 方式2：使用+=运算符
    string result2 = first;
    result2 += " ";
    result2 += second;
    cout << "使用+=运算符：" << result2 << endl;

    // 方式3：使用append方法
    string result3 = first;
    result3.append(" ").append(second);
    cout << "使用append方法：" << result3 << endl;

    // 可以直接拼接字符串字面量
    string greeting = "Hello, " + string("C++") + "!";
    cout << "拼接字面量：" << greeting << endl;
    cout << endl;

    // ========================================
    // 5. 字符串比较（超级简单！）
    // ========================================
    cout << "【5. 字符串比较】" << endl;

    string a = "apple";
    string b = "banana";
    string c = "apple";

    // 直接使用==、!=、<、>、<=、>=
    cout << "a == c: " << (a == c ? "true" : "false") << endl;
    cout << "a != b: " << (a != b ? "true" : "false") << endl;
    cout << "a < b: " << (a < b ? "true" : "false") << " (字典序)" << endl;
    cout << "b > a: " << (b > a ? "true" : "false") << endl;

    // 使用compare方法（类似strcmp）
    cout << "a.compare(b) = " << a.compare(b) << " (负数表示a < b)" << endl;
    cout << "a.compare(c) = " << a.compare(c) << " (0表示相等)" << endl;
    cout << endl;

    // ========================================
    // 6. 字符串访问
    // ========================================
    cout << "【6. 字符串访问】" << endl;

    string text = "Programming";

    // 方式1：使用[]运算符
    cout << "text[0] = " << text[0] << endl;
    cout << "text[5] = " << text[5] << endl;

    // 方式2：使用at方法（带边界检查，更安全）
    cout << "text.at(0) = " << text.at(0) << endl;

    // 修改字符
    text[0] = 'p';
    cout << "修改后：" << text << endl;

    // 获取首尾字符
    cout << "首字符：text.front() = " << text.front() << endl;
    cout << "尾字符：text.back() = " << text.back() << endl;
    cout << endl;

    // ========================================
    // 7. 字符串查找
    // ========================================
    cout << "【7. 字符串查找】" << endl;

    string sentence = "C++ is powerful and C++ is fun";

    // 查找子串（返回位置，找不到返回string::npos）
    size_t pos1 = sentence.find("powerful");
    if (pos1 != string::npos) {
        cout << "找到\"powerful\"，位置：" << pos1 << endl;
    }

    // 查找字符
    size_t pos2 = sentence.find('i');
    cout << "第一个'i'的位置：" << pos2 << endl;

    // 从指定位置开始查找
    size_t pos3 = sentence.find("C++", 5);  // 从位置5开始找
    cout << "从位置5开始找\"C++\"：" << pos3 << endl;

    // 反向查找
    size_t pos4 = sentence.rfind("C++");
    cout << "最后一个\"C++\"的位置：" << pos4 << endl;

    // 查找任意字符
    size_t pos5 = sentence.find_first_of("aeiou");
    cout << "第一个元音字母位置：" << pos5 << " ('" << sentence[pos5] << "')" << endl;
    cout << endl;

    // ========================================
    // 8. 字符串截取
    // ========================================
    cout << "【8. 字符串截取】" << endl;

    string original = "Hello, World!";

    // substr(起始位置, 长度)
    string sub1 = original.substr(0, 5);  // 从位置0开始，取5个字符
    cout << "original.substr(0, 5) = \"" << sub1 << "\"" << endl;

    string sub2 = original.substr(7);  // 从位置7到末尾
    cout << "original.substr(7) = \"" << sub2 << "\"" << endl;

    string sub3 = original.substr(7, 5);
    cout << "original.substr(7, 5) = \"" << sub3 << "\"" << endl;
    cout << endl;

    // ========================================
    // 9. 字符串插入和删除
    // ========================================
    cout << "【9. 字符串插入和删除】" << endl;

    string base = "Hello World";
    cout << "原字符串：\"" << base << "\"" << endl;

    // 插入
    base.insert(5, ",");  // 在位置5插入","
    cout << "插入后：\"" << base << "\"" << endl;

    // 删除
    base.erase(5, 1);  // 从位置5开始删除1个字符
    cout << "删除后：\"" << base << "\"" << endl;

    // 替换
    base.replace(6, 5, "C++");  // 从位置6开始，替换5个字符为"C++"
    cout << "替换后：\"" << base << "\"" << endl;
    cout << endl;

    // ========================================
    // 10. 字符串与数字转换
    // ========================================
    cout << "【10. 字符串与数字转换】" << endl;

    // 数字转字符串
    int num = 123;
    string num_str = to_string(num);
    cout << "to_string(123) = \"" << num_str << "\"" << endl;

    double pi = 3.14159;
    string pi_str = to_string(pi);
    cout << "to_string(3.14159) = \"" << pi_str << "\"" << endl;

    // 字符串转数字
    string str_num = "456";
    int converted = stoi(str_num);  // string to int
    cout << "stoi(\"456\") = " << converted << endl;

    string str_double = "3.14";
    double converted_d = stod(str_double);  // string to double
    cout << "stod(\"3.14\") = " << converted_d << endl;
    cout << endl;

    // ========================================
    // 11. string与C风格字符串的转换
    // ========================================
    cout << "【11. string与C风格字符串的转换】" << endl;

    // string转C风格字符串
    string cpp_str = "C++ String";
    const char* c_style = cpp_str.c_str();
    cout << "string转C风格：" << c_style << endl;

    // C风格字符串转string
    char c_array[] = "C Array";
    string from_c(c_array);
    cout << "C风格转string：" << from_c << endl;
    cout << endl;

    // ========================================
    // 12. 实战案例：字符串处理
    // ========================================
    cout << "【12. 实战案例：字符串处理】" << endl;

    // 案例1：统计单词数量
    string paragraph = "C++ is a powerful programming language";
    int word_count = 1;  // 至少有一个单词
    for (char ch : paragraph) {
        if (ch == ' ') word_count++;
    }
    cout << "\"" << paragraph << "\"" << endl;
    cout << "单词数量：" << word_count << endl;
    cout << endl;

    // 案例2：反转字符串
    string to_reverse = "Hello";
    string reversed;
    for (int i = to_reverse.length() - 1; i >= 0; i--) {
        reversed += to_reverse[i];
    }
    cout << "原字符串：" << to_reverse << endl;
    cout << "反转后：" << reversed << endl;
    cout << endl;

    // 案例3：判断回文
    string palindrome = "madam";
    bool is_palindrome = true;
    int len = palindrome.length();
    for (int i = 0; i < len / 2; i++) {
        if (palindrome[i] != palindrome[len - 1 - i]) {
            is_palindrome = false;
            break;
        }
    }
    cout << "\"" << palindrome << "\" 是回文吗？"
         << (is_palindrome ? "是" : "否") << endl;
    cout << endl;

    // ========================================
    // 13. string vs C风格字符串对比
    // ========================================
    cout << "【13. string vs C风格字符串对比】" << endl;
    cout << "┌────────────────┬──────────────────┬──────────────────┐" << endl;
    cout << "│ 特性           │ C风格字符串      │ C++ string类     │" << endl;
    cout << "├────────────────┼──────────────────┼──────────────────┤" << endl;
    cout << "│ 内存管理       │ 手动             │ 自动             │" << endl;
    cout << "│ 安全性         │ 容易溢出         │ 安全             │" << endl;
    cout << "│ 拼接           │ strcat()         │ + 或 +=          │" << endl;
    cout << "│ 比较           │ strcmp()         │ == 或 !=         │" << endl;
    cout << "│ 长度           │ strlen()         │ .length()        │" << endl;
    cout << "│ 查找           │ strstr()         │ .find()          │" << endl;
    cout << "│ 截取           │ 复杂             │ .substr()        │" << endl;
    cout << "│ 性能           │ 稍快             │ 稍慢（可忽略）   │" << endl;
    cout << "│ 推荐使用       │ 底层/兼容C       │ 现代C++开发      │" << endl;
    cout << "└────────────────┴──────────────────┴──────────────────┘" << endl;
    cout << endl;

    // ========================================
    // 14. 总结与建议
    // ========================================
    cout << "【14. 总结与建议】" << endl;
    cout << "✓ 现代C++开发中，优先使用string类" << endl;
    cout << "✓ string更安全、更易用、功能更强大" << endl;
    cout << "✓ 只在需要与C代码交互时使用C风格字符串" << endl;
    cout << "✓ 使用c_str()方法可以方便地转换" << endl;
    cout << "✓ string的性能开销在大多数情况下可以忽略" << endl;
    cout << endl;

    cout << "=== 学习完成！===" << endl;

    return 0;
}

/*
 * ============================================================================
 * 知识点总结
 * ============================================================================
 *
 * 1. string类的优势：
 *    - 自动内存管理
 *    - 安全（不会溢出）
 *    - 支持运算符（+、==、<等）
 *    - 丰富的成员函数
 *
 * 2. 常用操作：
 *    - 拼接：+ 或 += 或 append()
 *    - 比较：== 或 != 或 < 等
 *    - 查找：find()、rfind()
 *    - 截取：substr()
 *    - 插入：insert()
 *    - 删除：erase()
 *    - 替换：replace()
 *
 * 3. 转换：
 *    - 数字转字符串：to_string()
 *    - 字符串转数字：stoi()、stod()等
 *    - string转C风格：c_str()
 *
 * 4. 最佳实践：
 *    - 现代C++优先使用string
 *    - 需要与C交互时使用c_str()
 *    - 使用at()而非[]进行安全访问
 *    - 使用范围for循环遍历字符
 *
 * ============================================================================
 */
