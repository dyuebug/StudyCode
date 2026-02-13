// ============================================
// 第一个 C++ 程序 - Hello World
// 日期：2026-02-13（第1天）
// 难度：⭐ 初级
// 参考资料：CppFiles/CppBase/1.C++与C.md
// ============================================

/*
【核心概念】C++ 程序的基本结构

是什么？
- 这是一个最简单的 C++ 程序，用于输出 "Hello, World!"

为什么需要？
- 验证 C++ 编译环境是否正确配置
- 理解 C++ 程序的基本结构
- 学习 C++ 的输入输出方式

核心特点：
- 使用 iostream 头文件进行输入输出
- 使用 std 命名空间中的 cout 和 endl
- main 函数是程序的入口
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
【#include <iostream>】

是什么？
- iostream 是 C++ 标准库的头文件
- 提供了输入输出流的功能（input/output stream）

为什么没有 .h 后缀？
- C++ 标准库的头文件不使用 .h 后缀
- 这是 C++ 的约定，与 C 语言的头文件区分

包含的主要内容：
- std::cout：标准输出流对象（console output）
- std::cin：标准输入流对象（console input）
- std::endl：换行符（end line）
*/
#include <iostream>

/*
【using 声明机制】⭐⭐⭐⭐⭐

是什么？
- using 声明机制用于引入命名空间中的特定实体
- 格式：using 命名空间::实体名;

为什么需要？
- std::cout 和 std::endl 都在 std 命名空间中
- 每次使用都写 std:: 比较繁琐
- using 声明可以简化代码

三种使用方式对比：
1. 作用域限定符：std::cout（每次都写，准确但繁琐）
2. using 编译指令：using namespace std;（引入所有实体，不推荐）
3. using 声明机制：using std::cout;（推荐，需要什么引入什么）⭐⭐⭐⭐⭐
*/
using std::cout;  // 引入 std 命名空间中的 cout
using std::endl;  // 引入 std 命名空间中的 endl

/*
【main 函数】

是什么？
- main 函数是 C++ 程序的入口
- 程序从 main 函数开始执行

函数签名：
int main()
- 返回类型：int（整数）
- 函数名：main
- 参数：无参数（或 int argc, char* argv[]）
- 返回值：0 表示程序正常结束

为什么返回 int？
- 操作系统需要知道程序是否正常结束
- 返回 0 表示成功，非 0 表示出错
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

int main()
{
    // --------------------------------------------
    // 2.1 输出 "Hello, World!"
    // --------------------------------------------

    /*
    【cout << "Hello, World!" << endl;】

    cout：标准输出流对象（console output）
    - 类型：std::ostream
    - 作用：向控制台输出内容

    <<：输出流运算符（左移运算符被重载）
    - 作用：将右侧的内容输出到左侧的流对象
    - 可以连续使用（链式调用）

    "Hello, World!"：字符串字面值
    - 类型：const char*（C 风格字符串）
    - 作用：要输出的内容

    endl：换行符（end line）
    - 作用：输出换行符并刷新缓冲区
    - 等价于 '\n' + flush

    执行流程：
    1. "Hello, World!" 被输出到 cout
    2. endl 输出换行符并刷新缓冲区
    3. 控制台显示：Hello, World!（并换行）
    */
    cout << "Hello, World!" << endl;

    // --------------------------------------------
    // 2.2 返回 0 表示程序正常结束
    // --------------------------------------------

    /*
    【return 0;】

    作用：
    - 返回 0 给操作系统
    - 表示程序正常结束

    约定：
    - 0 表示成功
    - 非 0 表示出错（不同的错误码表示不同的错误）
    */
    return 0;
}

// ============================================
// 🔍 深入理解：C++ 输出流机制（选学）
// ============================================

/*
==============================================
🔍 深入理解：C++ 输出流机制（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【为什么使用 cout 而不是 printf？】

C 语言的 printf：
- 优点：格式化输出灵活
- 缺点：类型不安全，容易出错

C++ 的 cout：
- 优点：类型安全，自动推断类型
- 缺点：格式化输出相对复杂

示例对比：
// C 语言
int x = 10;
printf("%d\n", x);  // 必须指定 %d，容易写错

// C++
int x = 10;
cout << x << endl;  // 自动推断类型，更安全

【endl 与 '\n' 的区别】

endl：
- 输出换行符
- 刷新缓冲区（flush）
- 性能稍慢

'\n'：
- 只输出换行符
- 不刷新缓冲区
- 性能稍快

推荐：
- 一般情况使用 endl（更安全）
- 性能敏感场景使用 '\n'

【输出流运算符 << 的重载】

<< 原本是左移运算符：
int x = 1 << 2;  // x = 4（二进制左移）

在 iostream 中被重载为输出流运算符：
cout << "Hello";  // 输出字符串

这是 C++ 运算符重载的典型应用。

==============================================
*/

// ============================================
// 第三部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
【常见错误1：忘记包含头文件】

❌ 错误代码：
// #include <iostream>  // 忘记包含头文件
using std::cout;
using std::endl;

int main() {
    cout << "Hello" << endl;  // 编译错误：cout 未定义
    return 0;
}

结果：编译错误，提示 cout 和 endl 未定义

原因：cout 和 endl 在 iostream 头文件中定义

✅ 正确代码：
#include <iostream>  // 必须包含头文件
using std::cout;
using std::endl;

int main() {
    cout << "Hello" << endl;  // 正确
    return 0;
}

记忆技巧：
- 使用任何标准库功能前，必须包含对应的头文件
- iostream = input/output stream（输入输出流）


【常见错误2：忘记 using 声明或作用域限定】

❌ 错误代码：
#include <iostream>
// 忘记 using 声明

int main() {
    cout << "Hello" << endl;  // 编译错误：cout 未定义
    return 0;
}

结果：编译错误，提示 cout 和 endl 未定义

原因：cout 和 endl 在 std 命名空间中，需要使用 std:: 或 using 声明

✅ 正确代码（方式1）：
#include <iostream>
using std::cout;
using std::endl;

int main() {
    cout << "Hello" << endl;  // 正确
    return 0;
}

✅ 正确代码（方式2）：
#include <iostream>

int main() {
    std::cout << "Hello" << std::endl;  // 正确
    return 0;
}

记忆技巧：
- 推荐使用 using 声明机制（方式1）
- 需要什么就声明什么，避免命名冲突


【常见错误3：使用 using namespace std;（不推荐）】

❌ 不推荐的代码：
#include <iostream>
using namespace std;  // 引入 std 中的所有实体

int main() {
    cout << "Hello" << endl;  // 可以工作，但不推荐
    return 0;
}

问题：
- 引入了 std 命名空间中的所有实体
- 可能导致命名冲突
- 不知道具体使用了哪些实体

✅ 推荐的代码：
#include <iostream>
using std::cout;  // 只引入需要的实体
using std::endl;

int main() {
    cout << "Hello" << endl;  // 推荐
    return 0;
}

记忆技巧：
- 初学阶段推荐使用 using 声明机制
- 需要什么就声明什么，清晰明确


【常见错误4：忘记 return 0;】

❌ 错误代码：
#include <iostream>
using std::cout;
using std::endl;

int main() {
    cout << "Hello" << endl;
    // 忘记 return 0;
}

结果：
- C++98：编译警告或错误
- C++11：可以省略 return 0;（编译器自动添加）

建议：
- 养成好习惯，始终写 return 0;
- 明确表示程序正常结束

✅ 正确代码：
#include <iostream>
using std::cout;
using std::endl;

int main() {
    cout << "Hello" << endl;
    return 0;  // 明确返回 0
}


【常见错误5：中文乱码问题】

❌ 可能出现乱码：
#include <iostream>
using std::cout;
using std::endl;

int main() {
    cout << "你好，世界！" << endl;  // 可能乱码
    return 0;
}

原因：
- 源文件编码与控制台编码不一致
- Windows 默认使用 GBK 编码
- Linux 默认使用 UTF-8 编码

解决方法：
1. 统一使用 UTF-8 编码保存源文件
2. Windows 控制台设置为 UTF-8：chcp 65001
3. 或者暂时使用英文输出

✅ 推荐代码（初学阶段）：
#include <iostream>
using std::cout;
using std::endl;

int main() {
    cout << "Hello, World!" << endl;  // 使用英文，避免乱码
    return 0;
}
*/

// ============================================
// 第四部分：编译和运行
// ============================================

/*
【编译命令】

基本编译：
g++ 00_hello_world.cpp

指定输出文件名：
g++ 00_hello_world.cpp -o hello

运行程序：
./hello（Linux/Mac）
hello.exe（Windows）

编译选项说明：
- g++：C++ 编译器
- -o：指定输出文件名（output）
- -std=c++11：指定 C++ 标准（可选）
- -Wall：显示所有警告（推荐）
- -g：生成调试信息（用于 gdb 调试）

推荐的编译命令：
g++ 00_hello_world.cpp -o hello -std=c++11 -Wall

【运行结果】

控制台输出：
Hello, World!

程序返回值：
0（表示成功）

验证返回值（Linux/Mac）：
./hello
echo $?  # 输出 0
*/

// ============================================
// 第五部分：练习题
// ============================================

/*
【练习1】修改输出内容
要求：修改程序，输出你的名字和学习目标
提示：使用多个 cout 语句或连续的 << 运算符
考察点：cout 和 << 的使用

【练习2】输出多行内容
要求：输出以下内容（每行一句）
  第1行：Hello, C++!
  第2行：I am learning C++.
  第3行：This is my first program.
提示：使用多个 cout << ... << endl; 语句
考察点：endl 的使用

【练习3】尝试不同的 using 方式
要求：分别使用以下三种方式编写程序
  方式1：using std::cout; using std::endl;
  方式2：std::cout << ... << std::endl;
  方式3：using namespace std;（了解即可）
提示：观察代码的可读性和安全性
考察点：命名空间的使用

【练习4】观察编译错误
要求：故意制造以下错误，观察编译器的错误信息
  错误1：删除 #include <iostream>
  错误2：删除 using 声明
  错误3：删除 return 0;
提示：学会阅读编译器的错误信息
考察点：理解编译错误的原因

【练习5】输出不同类型的数据
要求：输出以下内容
  整数：10
  浮点数：3.14
  字符：'A'
  字符串："Hello"
提示：cout 可以自动推断类型
考察点：cout 的类型安全特性
*/

// ============================================
// 学习总结
// ============================================

/*
【今日学习要点】

1. C++ 程序的基本结构
   - #include <iostream>：包含输入输出流头文件
   - using 声明：引入命名空间中的实体
   - main 函数：程序入口
   - return 0：程序正常结束

2. C++ 输入输出
   - cout：标准输出流对象
   - <<：输出流运算符
   - endl：换行符并刷新缓冲区

3. 命名空间的使用
   - 推荐使用 using 声明机制
   - 需要什么就声明什么
   - 避免使用 using namespace std;

4. 编译和运行
   - g++ 编译器
   - 基本编译命令
   - 运行程序

【下一步学习】

- 学习命名空间的详细用法
- 学习引用的概念和使用
- 学习函数重载

【学习建议】

- 多动手实践，修改代码观察结果
- 理解每一行代码的作用
- 养成良好的编码习惯（注释、缩进）
- 遇到错误不要慌，学会阅读错误信息
*/
