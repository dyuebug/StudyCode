/*
 * ============================================================================
 * 文件名：19_cstring_basics.cpp
 * 主题：C风格字符串详解
 * 学习目标：
 *   1. 理解字符数组与字符串的关系
 *   2. 掌握C风格字符串的特点（以'\0'结尾）
 *   3. 学会使用常用字符串函数（strlen、strcpy、strcmp、strcat）
 *   4. 理解字符串输入输出的方法
 *   5. 认识缓冲区溢出的危险性
 *
 * 编译命令：g++ -std=c++11 -o 19_cstring_basics 19_cstring_basics.cpp
 * 运行命令：./19_cstring_basics (Linux/Mac) 或 19_cstring_basics.exe (Windows)
 * ============================================================================
 */

#include <iostream>
#include <cstring>  // C风格字符串函数库
using namespace std;

int main() {
    cout << "=== C风格字符串详解 ===" << endl << endl;

    // ========================================
    // 1. 字符数组 vs 字符串
    // ========================================
    cout << "【1. 字符数组 vs 字符串】" << endl;

    // 普通字符数组
    char chars[5] = {'H', 'e', 'l', 'l', 'o'};
    cout << "普通字符数组：";
    for (int i = 0; i < 5; i++) {
        cout << chars[i];
    }
    cout << " (不是字符串，没有'\\0')" << endl;

    // C风格字符串（以'\0'结尾）
    char str[6] = {'H', 'e', 'l', 'l', 'o', '\0'};
    cout << "C风格字符串：" << str << " (以'\\0'结尾)" << endl;

    // 简化写法（自动添加'\0'）
    char str2[] = "World";  // 编译器自动添加'\0'，实际占6个字节
    cout << "简化写法：" << str2 << endl;
    cout << "str2实际长度：" << sizeof(str2) << " 字节（包含'\\0'）" << endl;
    cout << endl;

    // ========================================
    // 2. '\0'结束符的重要性
    // ========================================
    cout << "【2. '\\0'结束符的重要性】" << endl;
    char test1[10] = "Hello";  // 自动添加'\0'
    char test2[10];
    test2[0] = 'H';
    test2[1] = 'i';
    // 注意：test2没有'\0'，输出会出问题

    cout << "test1（有'\\0'）：" << test1 << endl;
    cout << "test1的strlen：" << strlen(test1) << endl;

    // 手动添加'\0'
    test2[2] = '\0';
    cout << "test2（手动添加'\\0'）：" << test2 << endl;
    cout << "test2的strlen：" << strlen(test2) << endl;
    cout << endl;

    // ========================================
    // 3. 字符串的声明和初始化
    // ========================================
    cout << "【3. 字符串的声明和初始化】" << endl;

    // 方式1：直接初始化
    char name1[] = "Alice";  // 自动计算大小（6字节）
    cout << "方式1：" << name1 << endl;

    // 方式2：指定大小
    char name2[20] = "Bob";  // 剩余空间自动填充'\0'
    cout << "方式2：" << name2 << endl;

    // 方式3：字符数组方式
    char name3[] = {'T', 'o', 'm', '\0'};
    cout << "方式3：" << name3 << endl;
    cout << endl;

    // ========================================
    // 4. 字符串输入输出
    // ========================================
    cout << "【4. 字符串输入输出】" << endl;

    // 输出：直接用cout
    char greeting[] = "Hello, C++!";
    cout << "输出字符串：" << greeting << endl;

    // 输入方式1：cin（遇到空格停止）
    cout << "请输入你的名字（不含空格）：";
    char name[50];
    cin >> name;
    cout << "你好，" << name << "！" << endl;

    // 清空输入缓冲区
    cin.ignore(100, '\n');

    // 输入方式2：cin.getline（可以包含空格）
    cout << "请输入一句话（可含空格）：";
    char sentence[100];
    cin.getline(sentence, 100);
    cout << "你说：" << sentence << endl;
    cout << endl;

    // ========================================
    // 5. 常用字符串函数
    // ========================================
    cout << "【5. 常用字符串函数】" << endl;

    // strlen - 获取字符串长度（不包括'\0'）
    char text[] = "Programming";
    cout << "strlen(\"" << text << "\") = " << strlen(text) << endl;
    cout << "sizeof(text) = " << sizeof(text) << " (包括'\\0')" << endl;
    cout << endl;

    // strcpy - 字符串复制
    char source[] = "Copy me";
    char dest[50];
    strcpy(dest, source);
    cout << "strcpy后：dest = \"" << dest << "\"" << endl;
    cout << endl;

    // strncpy - 安全的字符串复制（指定最大长度）
    char safe_dest[10];
    strncpy(safe_dest, "This is a very long string", 9);
    safe_dest[9] = '\0';  // 手动添加结束符
    cout << "strncpy后：safe_dest = \"" << safe_dest << "\"" << endl;
    cout << endl;

    // strcmp - 字符串比较
    char str_a[] = "apple";
    char str_b[] = "banana";
    char str_c[] = "apple";

    cout << "strcmp比较结果：" << endl;
    cout << "strcmp(\"apple\", \"banana\") = " << strcmp(str_a, str_b)
         << " (负数表示apple < banana)" << endl;
    cout << "strcmp(\"banana\", \"apple\") = " << strcmp(str_b, str_a)
         << " (正数表示banana > apple)" << endl;
    cout << "strcmp(\"apple\", \"apple\") = " << strcmp(str_a, str_c)
         << " (0表示相等)" << endl;
    cout << endl;

    // strcat - 字符串拼接
    char first[50] = "Hello, ";
    char second[] = "World!";
    strcat(first, second);
    cout << "strcat后：first = \"" << first << "\"" << endl;
    cout << endl;

    // strncat - 安全的字符串拼接
    char base[20] = "C++";
    strncat(base, " Programming", 5);  // 只拼接5个字符
    cout << "strncat后：base = \"" << base << "\"" << endl;
    cout << endl;

    // strchr - 查找字符
    char search_str[] = "Hello World";
    char* pos = strchr(search_str, 'W');
    if (pos != nullptr) {
        cout << "在\"" << search_str << "\"中找到'W'，位置："
             << (pos - search_str) << endl;
    }
    cout << endl;

    // strstr - 查找子串
    char main_str[] = "C++ is powerful";
    char* sub_pos = strstr(main_str, "powerful");
    if (sub_pos != nullptr) {
        cout << "在\"" << main_str << "\"中找到\"powerful\"" << endl;
        cout << "从该位置开始：" << sub_pos << endl;
    }
    cout << endl;

    // ========================================
    // 6. 字符串数组（字符串的数组）
    // ========================================
    cout << "【6. 字符串数组】" << endl;
    char fruits[][20] = {
        "Apple",
        "Banana",
        "Orange",
        "Grape"
    };

    cout << "水果列表：" << endl;
    for (int i = 0; i < 4; i++) {
        cout << (i + 1) << ". " << fruits[i] << endl;
    }
    cout << endl;

    // ========================================
    // 7. 实战案例：字符串处理
    // ========================================
    cout << "【7. 实战案例：字符串处理】" << endl;

    // 案例1：统计字符串中的字符类型
    char input[] = "Hello123World!";
    int letters = 0, digits = 0, others = 0;

    for (int i = 0; i < strlen(input); i++) {
        if ((input[i] >= 'a' && input[i] <= 'z') ||
            (input[i] >= 'A' && input[i] <= 'Z')) {
            letters++;
        } else if (input[i] >= '0' && input[i] <= '9') {
            digits++;
        } else {
            others++;
        }
    }

    cout << "字符串\"" << input << "\"统计：" << endl;
    cout << "  字母：" << letters << " 个" << endl;
    cout << "  数字：" << digits << " 个" << endl;
    cout << "  其他：" << others << " 个" << endl;
    cout << endl;

    // 案例2：字符串反转
    char original[] = "Reverse";
    int len = strlen(original);
    char reversed[50];

    for (int i = 0; i < len; i++) {
        reversed[i] = original[len - 1 - i];
    }
    reversed[len] = '\0';

    cout << "原字符串：" << original << endl;
    cout << "反转后：" << reversed << endl;
    cout << endl;

    // ========================================
    // 8. 常见错误与安全问题
    // ========================================
    cout << "【8. 常见错误与安全问题】" << endl;
    cout << "❌ 错误1：忘记'\\0'结束符" << endl;
    cout << "   char str[5] = {'H','e','l','l','o'}; // 没有'\\0'，输出会出错" << endl;
    cout << endl;

    cout << "❌ 错误2：数组越界（缓冲区溢出）" << endl;
    cout << "   char small[5];" << endl;
    cout << "   strcpy(small, \"This is too long\"); // 危险！会溢出" << endl;
    cout << "   解决：使用strncpy并手动添加'\\0'" << endl;
    cout << endl;

    cout << "❌ 错误3：未初始化就使用" << endl;
    cout << "   char str[10];" << endl;
    cout << "   cout << str; // 危险！内容未知" << endl;
    cout << endl;

    cout << "❌ 错误4：字符串比较用==" << endl;
    cout << "   if (str1 == str2) // 错误！比较的是地址" << endl;
    cout << "   应该用：strcmp(str1, str2) == 0" << endl;
    cout << endl;

    // ========================================
    // 9. 安全编程建议
    // ========================================
    cout << "【9. 安全编程建议】" << endl;
    cout << "✅ 1. 总是确保字符串以'\\0'结尾" << endl;
    cout << "✅ 2. 使用strncpy、strncat等安全函数" << endl;
    cout << "✅ 3. 检查数组边界，避免溢出" << endl;
    cout << "✅ 4. 初始化字符数组后再使用" << endl;
    cout << "✅ 5. 优先使用C++ string类（下一节）" << endl;
    cout << endl;

    // ========================================
    // 10. 总结
    // ========================================
    cout << "【10. 总结】" << endl;
    cout << "C风格字符串的特点：" << endl;
    cout << "  ✓ 本质是字符数组，以'\\0'结尾" << endl;
    cout << "  ✓ 需要手动管理内存和边界" << endl;
    cout << "  ✓ 提供丰富的字符串函数（cstring库）" << endl;
    cout << "  ✗ 容易出现缓冲区溢出等安全问题" << endl;
    cout << "  ✗ 使用不如C++ string类方便" << endl;
    cout << endl;
    cout << "下一节将学习更安全、更方便的C++ string类！" << endl;

    return 0;
}

/*
 * ============================================================================
 * 知识点总结
 * ============================================================================
 *
 * 1. C风格字符串 = 字符数组 + '\0'结束符
 *
 * 2. 常用函数：
 *    - strlen()  : 获取长度（不含'\0'）
 *    - strcpy()  : 复制字符串
 *    - strncpy() : 安全复制（指定长度）
 *    - strcmp()  : 比较字符串
 *    - strcat()  : 拼接字符串
 *    - strncat() : 安全拼接
 *    - strchr()  : 查找字符
 *    - strstr()  : 查找子串
 *
 * 3. 输入方法：
 *    - cin >> str        : 遇空格停止
 *    - cin.getline(str, n) : 可读取空格，最多n-1个字符
 *
 * 4. 安全要点：
 *    - 确保'\0'结束符存在
 *    - 避免缓冲区溢出
 *    - 使用安全函数（strncpy、strncat）
 *    - 字符串比较用strcmp，不用==
 *
 * 5. 为什么要学C风格字符串？
 *    - 理解字符串的底层实现
 *    - 很多C库和旧代码使用C风格字符串
 *    - 为学习指针打基础
 *
 * 6. 下一步：
 *    学习C++ string类，更安全、更方便！
 * ============================================================================
 */
