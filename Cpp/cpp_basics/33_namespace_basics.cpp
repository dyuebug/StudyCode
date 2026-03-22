// ============================================
// 命名空间基础 - 超详细注释版
// 日期：2026-03-16（第10天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】命名空间（Namespace）

是什么？
- 命名空间是一种将代码组织到逻辑分组中的机制
- 用于避免不同库或模块之间的命名冲突

为什么需要？
- 大型项目中，不同模块可能有同名的函数、类、变量
- 命名空间提供了作用域隔离，避免命名冲突
- 例如：std命名空间包含了C++标准库的所有内容

核心特点：
- 提供作用域隔离
- 可以嵌套使用
- 可以跨文件定义
*/

#include <iostream>
#include <string>

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

// --------------------------------------------
// 1.1 什么是命名空间？
// --------------------------------------------

/*
【命名空间的作用】

问题场景：
假设你在开发一个大型项目，使用了两个第三方库：
- 图形库：有一个函数 draw()
- 音频库：也有一个函数 draw()

如果没有命名空间，编译器无法区分这两个函数，会报错。

解决方案：
- 图形库：Graphics::draw()
- 音频库：Audio::draw()

通过命名空间，两个同名函数可以共存。
*/

// --------------------------------------------
// 1.2 定义命名空间
// --------------------------------------------

// 定义一个命名空间：MyNamespace
// 语法：namespace 命名空间名 { 代码 }
namespace MyNamespace {
    // 在命名空间内定义变量
    int value = 100;

    // 在命名空间内定义函数
    void display() {
        std::cout << "MyNamespace::display() 被调用" << std::endl;
        std::cout << "value = " << value << std::endl;
    }
}

// --------------------------------------------
// 1.3 使用命名空间
// --------------------------------------------

/*
【三种使用方式】

方式1：完全限定名（推荐）
- 语法：命名空间名::成员名
- 优点：明确、安全、不会冲突
- 缺点：代码稍长

方式2：using声明（推荐）
- 语法：using 命名空间名::成员名;
- 优点：只引入需要的成员，安全
- 缺点：需要逐个声明

方式3：using指令（谨慎使用）
- 语法：using namespace 命名空间名;
- 优点：代码简洁
- 缺点：可能引发命名冲突，不推荐在头文件中使用
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 示例1：定义和使用命名空间
// --------------------------------------------

// 定义命名空间：Company
namespace Company {
    std::string name = "TechCorp";
    int employeeCount = 500;
    
    void showInfo() {
        std::cout << "公司名称：" << name << std::endl;
        std::cout << "员工数量：" << employeeCount << std::endl;
    }
}

// 定义命名空间：School
namespace School {
    std::string name = "清华大学";  // 同名变量，但在不同命名空间
    int studentCount = 30000;
    
    void showInfo() {  // 同名函数，但在不同命名空间
        std::cout << "学校名称：" << name << std::endl;
        std::cout << "学生数量：" << studentCount << std::endl;
    }
}

// --------------------------------------------
// 2.2 示例2：嵌套命名空间
// --------------------------------------------

/*
【嵌套命名空间】

是什么？
- 命名空间可以嵌套定义，形成层次结构

为什么需要？
- 更细粒度的组织代码
- 例如：Project::Graphics::2D、Project::Graphics::3D
*/

// 传统嵌套语法
namespace Project {
    namespace Graphics {
        namespace TwoD {
            void draw() {
                std::cout << "绘制2D图形" << std::endl;
            }
        }
        
        namespace ThreeD {
            void draw() {
                std::cout << "绘制3D图形" << std::endl;
            }
        }
    }
}

// C++17简化语法（推荐）
namespace Project::Audio::Effects {
    void applyReverb() {
        std::cout << "应用混响效果" << std::endl;
    }
}

// --------------------------------------------
// 2.3 示例3：匿名命名空间
// --------------------------------------------

/*
【匿名命名空间】

是什么？
- 没有名字的命名空间
- 语法：namespace { 代码 }

作用：
- 限制变量/函数的作用域为当前文件
- 相当于C语言的static关键字
- 防止不同文件中的同名符号冲突

使用场景：
- 文件内部的辅助函数
- 不希望被其他文件访问的变量
*/

namespace {
    // 这个变量只在当前文件可见
    int internalCounter = 0;
    
    // 这个函数只在当前文件可见
    void internalHelper() {
        std::cout << "内部辅助函数，外部文件无法访问" << std::endl;
    }
}

// --------------------------------------------
// 2.4 示例4：using声明 vs using指令
// --------------------------------------------

// using声明：只引入特定成员
using Company::name;  // 只引入Company::name

// using指令：引入整个命名空间（谨慎使用）
// using namespace School;  // 引入School的所有成员

// ============================================
// 主函数：演示所有示例
// ============================================

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "命名空间基础 - 实践演示" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // --------------------------------------------
    // 示例1：使用完全限定名
    // --------------------------------------------
    
    std::cout << "\n【示例1：使用完全限定名】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 使用完全限定名访问MyNamespace的成员
    std::cout << "访问 MyNamespace::value = " << MyNamespace::value << std::endl;
    MyNamespace::display();
    
    // --------------------------------------------
    // 示例2：同名符号在不同命名空间
    // --------------------------------------------
    
    std::cout << "\n【示例2：同名符号在不同命名空间】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // Company和School都有name和showInfo()，但不冲突
    std::cout << "\n公司信息：" << std::endl;
    Company::showInfo();
    
    std::cout << "\n学校信息：" << std::endl;
    School::showInfo();
    
    // --------------------------------------------
    // 示例3：嵌套命名空间
    // --------------------------------------------
    
    std::cout << "\n【示例3：嵌套命名空间】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 访问嵌套命名空间的成员
    Project::Graphics::TwoD::draw();
    Project::Graphics::ThreeD::draw();
    Project::Audio::Effects::applyReverb();
    
    // --------------------------------------------
    // 示例4：匿名命名空间
    // --------------------------------------------
    
    std::cout << "\n【示例4：匿名命名空间】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 匿名命名空间的成员可以直接访问（无需前缀）
    internalCounter = 42;
    std::cout << "内部计数器：" << internalCounter << std::endl;
    internalHelper();
    
    // --------------------------------------------
    // 示例5：using声明
    // --------------------------------------------
    
    std::cout << "\n【示例5：using声明】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 因为前面有 using Company::name;
    // 所以可以直接使用 name（指向Company::name）
    std::cout << "使用using声明访问：" << name << std::endl;
    
    // 但其他成员仍需完全限定名
    std::cout << "员工数量：" << Company::employeeCount << std::endl;
    
    return 0;
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
==============================================
🔍 深入理解：命名空间的底层原理（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【编译器如何处理命名空间？】

1. 名称修饰（Name Mangling）
   - 编译器会将命名空间名编码到符号名中
   - 例如：MyNamespace::display() 
     → 编译后可能变成：_ZN11MyNamespace7displayEv
   - 这样不同命名空间的同名函数在链接时不会冲突

2. 作用域解析
   - 编译器使用作用域解析运算符（::）查找符号
   - 查找顺序：当前作用域 → 外层命名空间 → 全局命名空间

3. using指令的风险
   - using namespace std; 会引入std的所有符号（数千个）
   - 可能与你的代码冲突，例如：
     * 你定义了 count 变量
     * std也有 std::count 函数
     * 编译器可能无法区分，导致错误

【最佳实践】

1. 头文件中：
   - ✅ 使用完全限定名：std::cout
   - ✅ 使用using声明：using std::cout;
   - ❌ 避免using指令：using namespace std;

2. 源文件中：
   - ✅ 可以使用using指令，但要谨慎
   - ✅ 优先使用using声明

3. 全局命名空间：
   - ❌ 避免在全局命名空间定义大量符号
   - ✅ 将代码组织到命名空间中

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
【常见错误1：using namespace std 引发的冲突】

❌ 错误代码：
#include <algorithm>
using namespace std;  // 引入std的所有符号

int count = 10;  // 你定义的变量
// 编译错误！std::count 是一个函数，与你的变量冲突

结果：编译错误，符号冲突

原因：using namespace std 引入了数千个符号，容易冲突

✅ 正确代码：
#include <algorithm>

int count = 10;  // 你的变量
// std::count 仍然可以通过完全限定名访问

结果：编译成功，没有冲突

记忆技巧/预防措施：
- 头文件中禁止使用 using namespace
- 源文件中谨慎使用，优先使用 using 声明
- 大型项目中，始终使用完全限定名

────────────────────────────────────────────

【常见错误2：忘记命名空间前缀】

❌ 错误代码：
namespace Math {
    int add(int a, int b) { return a + b; }
}

int main() {
    int result = add(1, 2);  // 编译错误！找不到add函数
    return 0;
}

结果：编译错误，找不到add函数

原因：add函数在Math命名空间中，需要使用Math::add

✅ 正确代码：
namespace Math {
    int add(int a, int b) { return a + b; }
}

int main() {
    int result = Math::add(1, 2);  // 使用完全限定名
    return 0;
}

结果：编译成功

记忆技巧/预防措施：
- 记住命名空间的作用域规则
- 使用IDE的自动补全功能
- 编译错误时检查是否缺少命名空间前缀

────────────────────────────────────────────

【常见错误3：头文件中使用using namespace】

❌ 错误代码（在头文件中）：
// myheader.h
#ifndef MYHEADER_H
#define MYHEADER_H

using namespace std;  // ❌ 危险！污染全局命名空间

void myFunction();

#endif

结果：所有包含此头文件的源文件都被强制引入std命名空间

原因：头文件会被多个源文件包含，using namespace会影响所有文件

✅ 正确代码（在头文件中）：
// myheader.h
#ifndef MYHEADER_H
#define MYHEADER_H

#include <string>

void myFunction(const std::string& str);  // 使用完全限定名

#endif

结果：不污染全局命名空间，安全

记忆技巧/预防措施：
- 头文件中永远不要使用 using namespace
- 头文件中使用完全限定名
- 源文件中可以谨慎使用 using namespace

────────────────────────────────────────────

【常见错误4：命名空间拼写错误】

❌ 错误代码：
namespace MySpace {
    void func() {}
}

int main() {
    MySpce::func();  // 拼写错误：MySpce → MySpace
    return 0;
}

结果：编译错误，找不到MySpce命名空间

原因：命名空间名拼写错误

✅ 正确代码：
namespace MySpace {
    void func() {}
}

int main() {
    MySpace::func();  // 正确拼写
    return 0;
}

结果：编译成功

记忆技巧/预防措施：
- 使用IDE的自动补全功能
- 命名空间名要简洁易记
- 编译错误时仔细检查拼写

────────────────────────────────────────────

【常见错误5：嵌套命名空间访问错误】

❌ 错误代码：
namespace A {
    namespace B {
        void func() {}
    }
}

int main() {
    A::func();  // 错误！func在A::B中，不在A中
    return 0;
}

结果：编译错误，A中没有func函数

原因：func在嵌套命名空间A::B中，需要完整路径

✅ 正确代码：
namespace A {
    namespace B {
        void func() {}
    }
}

int main() {
    A::B::func();  // 使用完整路径
    return 0;
}

结果：编译成功

记忆技巧/预防措施：
- 嵌套命名空间需要完整路径
- 使用C++17简化语法：namespace A::B { }
- 使用using声明简化访问：using A::B::func;

────────────────────────────────────────────

【常见错误6：匿名命名空间的误用】

❌ 错误代码（在头文件中）：
// myheader.h
namespace {
    int internalVar = 10;  // ❌ 每个包含此头文件的源文件都有独立副本
}

结果：每个源文件都有独立的internalVar副本，可能导致链接错误

原因：匿名命名空间在每个翻译单元中都是独立的

✅ 正确代码（在源文件中）：
// myfile.cpp
namespace {
    int internalVar = 10;  // ✅ 只在当前源文件可见
}

结果：internalVar只在当前源文件可见，不会冲突

记忆技巧/预防措施：
- 匿名命名空间只在源文件中使用
- 头文件中不要使用匿名命名空间
- 匿名命名空间相当于static关键字

────────────────────────────────────────────

【常见错误7：using声明的作用域问题】

❌ 错误代码：
#include <iostream>

void func1() {
    using std::cout;
    cout << "func1" << std::endl;
}

void func2() {
    cout << "func2" << std::endl;  // 错误！cout在func1的作用域中
}

结果：编译错误，func2中找不到cout

原因：using声明的作用域仅限于声明所在的作用域

✅ 正确代码：
#include <iostream>

using std::cout;  // 在全局作用域声明

void func1() {
    cout << "func1" << std::endl;
}

void func2() {
    cout << "func2" << std::endl;  // 正确
}

结果：编译成功

记忆技巧/预防措施：
- using声明的作用域规则与变量相同
- 需要在多个函数中使用时，在全局作用域声明
- 或者在每个函数中单独声明
*/

// ============================================
// 第五部分：函数卡片速查
// ============================================

/*
【命名空间相关语法】

1. 定义命名空间
   语法：namespace 命名空间名 { 代码 }
   作用：创建一个命名空间
   示例：namespace MySpace { int x = 10; }

2. 访问命名空间成员（完全限定名）
   语法：命名空间名::成员名
   作用：访问命名空间中的成员
   示例：MySpace::x

3. using声明
   语法：using 命名空间名::成员名;
   作用：引入特定成员到当前作用域
   示例：using std::cout;

4. using指令
   语法：using namespace 命名空间名;
   作用：引入整个命名空间到当前作用域
   示例：using namespace std;
   注意：谨慎使用，可能引发命名冲突

5. 嵌套命名空间（C++17）
   语法：namespace A::B::C { 代码 }
   作用：简化嵌套命名空间的定义
   示例：namespace Project::Graphics::2D { }

6. 匿名命名空间
   语法：namespace { 代码 }
   作用：限制符号的作用域为当前文件
   示例：namespace { int internal = 0; }
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
【练习题】

1. 定义两个命名空间Game和Music，各自包含一个play()函数
   要求：在main函数中分别调用这两个函数
   提示：使用完全限定名

2. 创建嵌套命名空间App::UI::Button，定义一个click()函数
   要求：在main函数中调用click()函数
   提示：使用完整路径App::UI::Button::click()

3. 使用匿名命名空间定义一个辅助函数helper()
   要求：在main函数中调用helper()函数
   提示：匿名命名空间的成员可以直接访问

4. 定义命名空间Math，包含add()和subtract()函数
   要求：使用using声明只引入add()函数，subtract()仍需完全限定名
   提示：using Math::add;

5. 分析以下代码的问题并修复：
   namespace A {
       int value = 10;
   }
   
   int main() {
       using namespace A;
       int value = 20;  // 这里会有什么问题？
       cout << value << endl;
       return 0;
   }
   提示：考虑命名冲突

6. 创建一个命名空间Utils，包含多个工具函数
   要求：在头文件中声明，在源文件中实现
   提示：头文件中不要使用using namespace

7. 使用C++17简化语法定义嵌套命名空间Company::HR::Employee
   要求：定义一个showInfo()函数
   提示：namespace Company::HR::Employee { }
*/

// ============================================
// 编译和运行
// ============================================

/*
编译命令：
g++ -std=c++17 33_namespace_basics.cpp -o 33_namespace_basics

运行命令：
./33_namespace_basics        (Linux/Mac)
33_namespace_basics.exe      (Windows)

预期输出：
- 演示命名空间的定义和使用
- 演示嵌套命名空间
- 演示匿名命名空间
- 演示using声明
*/
