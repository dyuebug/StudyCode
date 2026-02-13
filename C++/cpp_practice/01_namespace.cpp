// ============================================
// 命名空间（Namespace）- 超详细注释版
// 日期：2026-02-13（第1天）
// 难度：⭐⭐ 中级
// 参考资料：CppFiles/CppBase/1.C++与C.md（命名空间部分）
// ============================================

/*
【核心概念】命名空间（Namespace）

是什么？
- 命名空间是程序员命名的内存区域
- 用于将全局实体分组，避免命名冲突

为什么需要？
- 大型项目中，不同模块可能使用相同的名字
- C 语言只能通过命名约定避免冲突（如 hw_cpp_tom_num）
- C++ 提供命名空间机制，更优雅地解决命名冲突

核心特点：
- 可以包含变量、函数、类等实体
- 可以嵌套使用
- 可以跨文件定义
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>

/*
【命名空间的定义】

语法：
namespace 命名空间名 {
    // 实体（变量、函数、类等）
}

作用：
- 将相关的实体组织在一起
- 避免与其他命名空间中的同名实体冲突

命名空间中可以包含：
- 变量、常量
- 函数
- 类、结构体
- 其他命名空间（嵌套）
*/

// --------------------------------------------
// 1.1 定义命名空间
// --------------------------------------------

// 定义命名空间 wd
namespace wd {
    // 变量：存储在命名空间中
    int number = 100;

    // 函数：存储在命名空间中
    void display() {
        std::cout << "wd::display()" << std::endl;
    }
}

// 定义命名空间 cpp
namespace cpp {
    // 同名变量：与 wd::number 不冲突
    int number = 200;

    // 同名函数：与 wd::display() 不冲突
    void display() {
        std::cout << "cpp::display()" << std::endl;
    }
}

// --------------------------------------------
// 1.2 命名空间的三种使用方式 ⭐⭐⭐⭐⭐
// --------------------------------------------

/*
【方式1：作用域限定符 ::】

语法：命名空间名::实体名

优点：
- 准确，明确指定使用哪个命名空间的实体
- 不会产生命名冲突

缺点：
- 繁琐，每次使用都要写命名空间名

适用场景：
- 偶尔使用某个命名空间的实体
- 需要明确区分不同命名空间的同名实体
*/

void test_scope_operator() {
    std::cout << "\n=== 方式1：作用域限定符 :: ===" << std::endl;

    // 使用 wd 命名空间的 number
    std::cout << "wd::number = " << wd::number << std::endl;

    // 使用 cpp 命名空间的 number
    std::cout << "cpp::number = " << cpp::number << std::endl;

    // 调用 wd 命名空间的 display()
    wd::display();

    // 调用 cpp 命名空间的 display()
    cpp::display();
}

/*
【方式2：using 编译指令】

语法：using namespace 命名空间名;

优点：
- 简洁，不需要每次写命名空间名
- 一次性引入命名空间中的所有实体

缺点：
- 引入了所有实体，可能导致命名冲突
- 不清楚具体使用了哪些实体
- 不推荐在全局作用域使用

适用场景：
- 局部作用域中使用
- 确定不会产生命名冲突
*/

void test_using_directive() {
    std::cout << "\n=== 方式2：using 编译指令 ===" << std::endl;

    // 在局部作用域中使用 using 编译指令
    using namespace wd;

    // 直接使用 number 和 display()
    // 编译器会在 wd 命名空间中查找
    std::cout << "number = " << number << std::endl;
    display();

    // 注意：如果此时再使用 using namespace cpp;
    // 会导致 number 和 display 产生二义性
}

/*
【方式3：using 声明机制】⭐⭐⭐⭐⭐（推荐）

语法：using 命名空间名::实体名;

优点：
- 需要什么就声明什么，清晰明确
- 不会引入不需要的实体
- 避免命名冲突

缺点：
- 需要逐个声明

适用场景：
- 推荐在初学阶段使用
- 适合大多数场景
*/

void test_using_declaration() {
    std::cout << "\n=== 方式3：using 声明机制（推荐）===" << std::endl;

    // 只声明需要的实体
    using wd::number;
    using wd::display;

    // 直接使用声明的实体
    std::cout << "number = " << number << std::endl;
    display();

    // 如果需要使用 cpp 命名空间的实体
    // 可以使用作用域限定符
    std::cout << "cpp::number = " << cpp::number << std::endl;
    cpp::display();
}

// --------------------------------------------
// 1.3 命名空间的嵌套使用
// --------------------------------------------

/*
【嵌套命名空间】

是什么？
- 命名空间中可以定义另一个命名空间
- 类似于文件夹中可以包含子文件夹

为什么需要？
- 更细粒度的组织代码
- 避免命名冲突

如何访问？
- 使用多层作用域限定符：外层::内层::实体名
*/

namespace company {
    namespace project1 {
        int version = 1;
        void info() {
            std::cout << "Project 1, Version " << version << std::endl;
        }
    }

    namespace project2 {
        int version = 2;
        void info() {
            std::cout << "Project 2, Version " << version << std::endl;
        }
    }
}

void test_nested_namespace() {
    std::cout << "\n=== 命名空间的嵌套使用 ===" << std::endl;

    // 方式1：使用作用域限定符
    std::cout << "Project1 version: " << company::project1::version << std::endl;
    company::project1::info();

    std::cout << "Project2 version: " << company::project2::version << std::endl;
    company::project2::info();

    // 方式2：using 声明
    using company::project1::version;
    using company::project1::info;

    std::cout << "version = " << version << std::endl;
    info();
}

// ============================================
// 第二部分：基础操作实践
// ============================================

/*
【std 命名空间】

是什么？
- std 是 C++ 标准库的命名空间（standard）
- 包含了所有标准库的实体

常用实体：
- std::cout：标准输出流
- std::cin：标准输入流
- std::endl：换行符
- std::string：字符串类
- std::vector：动态数组
- 等等...

推荐使用方式：
- 使用 using 声明机制
- 需要什么就声明什么
*/

// 使用 using 声明机制（推荐）
using std::cout;
using std::endl;

void test_std_namespace() {
    cout << "\n=== std 命名空间的使用 ===" << endl;

    // 直接使用 cout 和 endl
    cout << "Hello, C++!" << endl;

    // 其他 std 实体仍需使用 std::
    std::string name = "Alice";
    cout << "Name: " << name << endl;
}

// ============================================
// 🔍 深入理解：命名空间的设计原理（选学）
// ============================================

/*
==============================================
🔍 深入理解：命名空间的设计原理（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【为什么 C++ 需要命名空间？】

C 语言的问题：
- 所有全局实体都在同一个全局作用域
- 大型项目中容易产生命名冲突
- 只能通过命名约定避免（如 prefix_module_name）

C++ 的解决方案：
- 引入命名空间机制
- 将全局作用域划分为多个命名空间
- 每个命名空间是一个独立的作用域

【命名空间的实现原理】

编译器处理：
- 编译器会为每个命名空间中的实体生成唯一的符号名
- 例如：wd::display() 可能被编译为 _ZN2wd7displayEv
- 这样即使不同命名空间有同名实体，符号名也不同

链接器处理：
- 链接器根据符号名进行链接
- 不同命名空间的同名实体有不同的符号名
- 因此不会产生链接冲突

【命名空间与作用域】

命名空间是一种作用域：
- 全局作用域：全局变量、全局函数
- 命名空间作用域：命名空间中的实体
- 类作用域：类的成员
- 局部作用域：函数内的局部变量

作用域的查找顺序：
1. 当前作用域
2. 外层作用域
3. 命名空间作用域
4. 全局作用域

【using 声明的作用域】

using 声明的作用域：
- 从 using 声明开始
- 到 using 所在作用域结束

示例：
void func() {
    using std::cout;
    cout << "Hello";  // OK
}
// cout 在这里不可用

【命名空间的最佳实践】

1. 提倡在命名空间中定义实体
2. 不要在头文件中使用 using 编译指令
3. 优先使用 using 声明机制
4. 将 using 声明放在局部作用域

==============================================
*/

// ============================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
【常见错误1：using 编译指令导致命名冲突】

❌ 错误代码：
namespace wd {
    int number = 100;
}

namespace cpp {
    int number = 200;
}

void test() {
    using namespace wd;
    using namespace cpp;
    cout << number << endl;  // 错误：二义性
}

结果：编译错误，number 有二义性

原因：
- using namespace wd; 引入了 wd::number
- using namespace cpp; 引入了 cpp::number
- 编译器不知道使用哪个 number

✅ 正确代码（方式1）：
void test() {
    using namespace wd;
    cout << number << endl;  // 使用 wd::number
    cout << cpp::number << endl;  // 明确使用 cpp::number
}

✅ 正确代码（方式2）：
void test() {
    using wd::number;
    cout << number << endl;  // 使用 wd::number
    cout << cpp::number << endl;  // 明确使用 cpp::number
}

记忆技巧：
- 避免在同一作用域使用多个 using namespace
- 推荐使用 using 声明机制


【常见错误2：全局作用域使用 using namespace】

❌ 不推荐的代码：
#include <iostream>
using namespace std;  // 全局作用域，不推荐

int main() {
    cout << "Hello" << endl;
    return 0;
}

问题：
- 引入了 std 中的所有实体（数百个）
- 可能与自定义的实体冲突
- 不清楚具体使用了哪些实体

✅ 推荐的代码：
#include <iostream>
using std::cout;  // 只引入需要的实体
using std::endl;

int main() {
    cout << "Hello" << endl;
    return 0;
}

记忆技巧：
- 将 using 声明放在局部作用域
- 或者使用 using 声明机制代替 using 编译指令


【常见错误3：命名空间与全局实体冲突】

❌ 错误代码：
int number = 100;  // 全局变量

namespace wd {
    int number = 200;  // 命名空间变量
}

void test() {
    using namespace wd;
    cout << number << endl;  // 错误：二义性
}

结果：编译错误，number 有二义性

原因：
- 全局变量 number
- 命名空间变量 wd::number
- using namespace wd; 引入了 wd::number
- 编译器不知道使用哪个 number

✅ 正确代码：
int number = 100;

namespace wd {
    int number = 200;
}

void test() {
    using wd::number;  // 使用 wd::number
    cout << number << endl;  // 输出 200（局部屏蔽全局）
    cout << ::number << endl;  // 输出 100（明确使用全局）
}

记忆技巧：
- 避免命名空间与全局实体同名
- 使用 :: 访问全局实体（::number）


【常见错误4：忘记命名空间作用域】

❌ 错误代码：
namespace wd {
    int number = 100;
}

void test() {
    cout << number << endl;  // 错误：number 未定义
}

结果：编译错误，number 未定义

原因：
- number 在 wd 命名空间中
- 没有使用 using 声明或作用域限定符

✅ 正确代码（方式1）：
void test() {
    cout << wd::number << endl;  // 使用作用域限定符
}

✅ 正确代码（方式2）：
void test() {
    using wd::number;
    cout << number << endl;  // 使用 using 声明
}

记忆技巧：
- 命名空间中的实体必须通过命名空间访问
- 使用作用域限定符或 using 声明


【常见错误5：在头文件中使用 using 编译指令】

❌ 错误代码（头文件 myheader.h）：
#ifndef MYHEADER_H
#define MYHEADER_H

#include <iostream>
using namespace std;  // 不推荐

void myFunction();

#endif

问题：
- 头文件被包含时，using namespace std; 也会生效
- 影响包含该头文件的所有源文件
- 可能导致命名冲突

✅ 正确代码（头文件 myheader.h）：
#ifndef MYHEADER_H
#define MYHEADER_H

#include <iostream>

void myFunction();

#endif

✅ 正确代码（源文件 myheader.cpp）：
#include "myheader.h"
using std::cout;  // 在源文件中使用 using 声明
using std::endl;

void myFunction() {
    cout << "Hello" << endl;
}

记忆技巧：
- 不要在头文件中使用 using 编译指令
- 在源文件中使用 using 声明
*/

// ============================================
// 第四部分：实战示例
// ============================================

// 模拟一个简单的项目结构
namespace math {
    int add(int a, int b) {
        return a + b;
    }

    int subtract(int a, int b) {
        return a - b;
    }
}

namespace string_utils {
    void print(const char* str) {
        std::cout << "String: " << str << std::endl;
    }

    int length(const char* str) {
        int len = 0;
        while (str[len] != '\0') {
            len++;
        }
        return len;
    }
}

void test_practical_example() {
    cout << "\n=== 实战示例 ===" << endl;

    // 使用 math 命名空间
    using math::add;
    using math::subtract;

    int x = 10, y = 5;
    cout << "x + y = " << add(x, y) << endl;
    cout << "x - y = " << subtract(x, y) << endl;

    // 使用 string_utils 命名空间
    using string_utils::print;
    using string_utils::length;

    const char* str = "Hello";
    print(str);
    cout << "Length: " << length(str) << endl;
}

// ============================================
// 第五部分：练习题
// ============================================

/*
【练习1】定义自己的命名空间
要求：定义一个命名空间 student，包含以下内容
  - 变量 name（字符串）
  - 变量 age（整数）
  - 函数 printInfo()（输出姓名和年龄）
提示：参考 wd 命名空间的定义
考察点：命名空间的定义和使用

【练习2】使用三种方式访问命名空间
要求：使用三种方式访问 student 命名空间的实体
  - 方式1：作用域限定符
  - 方式2：using 编译指令
  - 方式3：using 声明机制
提示：在不同的函数中使用不同的方式
考察点：命名空间的三种使用方式

【练习3】解决命名冲突
要求：定义两个命名空间 team1 和 team2
  - 都包含变量 score
  - 都包含函数 printScore()
  - 在 main 函数中分别访问两个命名空间的实体
提示：使用作用域限定符或 using 声明
考察点：命名空间避免命名冲突

【练习4】嵌套命名空间
要求：定义嵌套命名空间 school::class1 和 school::class2
  - 每个班级包含变量 studentCount
  - 每个班级包含函数 printInfo()
  - 在 main 函数中访问两个班级的信息
提示：使用多层作用域限定符
考察点：嵌套命名空间的使用

【练习5】std 命名空间的使用
要求：编写一个程序，使用 std 命名空间的以下实体
  - std::cout
  - std::endl
  - std::string（后续会学习）
提示：使用 using 声明机制
考察点：std 命名空间的使用
*/

// ============================================
// 主函数
// ============================================

int main() {
    cout << "=== 命名空间（Namespace）学习 ===" << endl;

    // 测试三种使用方式
    test_scope_operator();
    test_using_directive();
    test_using_declaration();

    // 测试嵌套命名空间
    test_nested_namespace();

    // 测试 std 命名空间
    test_std_namespace();

    // 实战示例
    test_practical_example();

    return 0;
}

// ============================================
// 学习总结
// ============================================

/*
【今日学习要点】

1. 命名空间的作用
   - 避免命名冲突
   - 组织代码
   - 版本控制

2. 命名空间的定义
   - namespace 命名空间名 { ... }
   - 可以包含变量、函数、类等实体
   - 可以嵌套使用

3. 命名空间的三种使用方式
   - 作用域限定符 ::（准确但繁琐）
   - using 编译指令（简洁但不推荐）
   - using 声明机制（推荐）⭐⭐⭐⭐⭐

4. 最佳实践
   - 推荐使用 using 声明机制
   - 将 using 声明放在局部作用域
   - 不要在头文件中使用 using 编译指令
   - 避免命名空间与全局实体同名

【下一步学习】

- 学习引用的概念和使用
- 学习函数重载
- 学习 const 关键字

【学习建议】

- 命名空间是 C++ 的重要特性，务必掌握
- 推荐使用 using 声明机制，清晰明确
- 多动手实践，理解三种使用方式的区别
- 养成良好的编码习惯，避免命名冲突
*/
