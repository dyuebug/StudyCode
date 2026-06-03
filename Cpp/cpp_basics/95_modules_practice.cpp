// ============================================
// 95_modules_practice.cpp - C++20 Modules 实践应用
// 功能：通过实例理解模块的实际应用
// 注意：本文件以概念讲解为主，包含伪代码示例
// 编译：需要 C++20 支持，但本文件主要用于学习概念
// ============================================

#include <iostream>
#include <string>
#include <vector>
using namespace std;

// ============================================
// 第一部分：创建简单的数学模块
// ============================================

int main() {
    cout << "=== C++20 Modules 实践应用 ===" << endl;

    // --------------------------------------------
    // 示例1：简单的数学模块
    // --------------------------------------------

    cout << "\n【示例1】简单的数学模块" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【模块设计】

创建一个数学模块，提供基本的数学运算功能。

文件结构：
------------------------------------------------------------
math/
├── math.cppm          # 模块接口单元
└── math_impl.cpp      # 模块实现单元（可选）

【math.cppm - 模块接口单元】
------------------------------------------------------------
// 声明并导出模块
export module math;

// 导出函数
export int add(int a, int b) {
    return a + b;
}

export int subtract(int a, int b) {
    return a - b;
}

export int multiply(int a, int b) {
    return a * b;
}

export int divide(int a, int b) {
    if (b == 0) {
        throw runtime_error("Division by zero");
    }
    return a / b;
}

// 导出常量
export const double PI = 3.14159265358979323846;
export const double E = 2.71828182845904523536;

// 私有辅助函数（不导出）
int validate_input(int x) {
    return x >= 0 ? x : -x;  // 返回绝对值
}

【使用模块】
------------------------------------------------------------
// main.cpp
import math;  // 导入 math 模块

int main() {
    // 使用导出的函数
    int sum = add(10, 20);           // 30
    int diff = subtract(20, 10);     // 10
    int product = multiply(5, 6);    // 30
    int quotient = divide(20, 4);    // 5

    // 使用导出的常量
    double circle_area = PI * 10 * 10;  // 314.159...

    // 错误：不能使用私有函数
    // int x = validate_input(-5);  // 编译错误：validate_input 未导出

    return 0;
}

【关键点】
1. export module math; - 声明并导出模块
2. export 函数/常量 - 导出接口
3. 未 export 的内容是私有的
4. import math; - 导入模块
)" << endl;

    // --------------------------------------------
    // 示例2：导出类
    // --------------------------------------------

    cout << "\n【示例2】导出类" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【模块设计】

创建一个包含类的模块。

【calculator.cppm - 模块接口单元】
------------------------------------------------------------
export module calculator;

// 导出类
export class Calculator {
public:
    Calculator() : memory(0) {}

    // 基本运算
    int add(int a, int b) {
        return a + b;
    }

    int subtract(int a, int b) {
        return a - b;
    }

    // 内存操作
    void store(int value) {
        memory = value;
    }

    int recall() const {
        return memory;
    }

    void clear() {
        memory = 0;
    }

private:
    int memory;  // 私有成员变量
};

// 导出辅助函数
export int power(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

// 私有辅助类（不导出）
class InternalHelper {
    // 只在模块内部使用
};

【使用模块】
------------------------------------------------------------
// main.cpp
import calculator;

int main() {
    // 使用导出的类
    Calculator calc;

    int sum = calc.add(10, 20);      // 30
    int diff = calc.subtract(20, 10); // 10

    calc.store(100);
    int value = calc.recall();        // 100
    calc.clear();

    // 使用导出的函数
    int result = power(2, 10);        // 1024

    // 错误：不能使用私有类
    // InternalHelper helper;  // 编译错误：InternalHelper 未导出

    return 0;
}

【关键点】
1. export class - 导出整个类
2. 类的 public 成员对外可见
3. 类的 private 成员仍然是私有的
4. 未 export 的类是模块私有的
)" << endl;

    // --------------------------------------------
    // 示例3：模块分区（Module Partition）
    // --------------------------------------------

    cout << "\n【示例3】模块分区" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【模块分区的概念】

模块分区用于将大模块拆分为多个部分，便于组织和维护。

语法：
- export module 模块名:分区名;  // 声明分区
- import :分区名;                // 导入分区

【文件结构】
------------------------------------------------------------
math/
├── math.cppm              # 主模块接口
├── math-basic.cppm        # 基础运算分区
├── math-advanced.cppm     # 高级运算分区
└── math-constants.cppm    # 常量分区

【math-basic.cppm - 基础运算分区】
------------------------------------------------------------
export module math:basic;  // 声明分区

export int add(int a, int b) {
    return a + b;
}

export int subtract(int a, int b) {
    return a - b;
}

【math-advanced.cppm - 高级运算分区】
------------------------------------------------------------
export module math:advanced;  // 声明分区

export int power(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

export int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

【math-constants.cppm - 常量分区】
------------------------------------------------------------
export module math:constants;  // 声明分区

export const double PI = 3.14159265358979323846;
export const double E = 2.71828182845904523536;

【math.cppm - 主模块接口】
------------------------------------------------------------
export module math;  // 声明主模块

// 导入并重新导出分区
export import :basic;
export import :advanced;
export import :constants;

// 或者只导入，不重新导出
// import :basic;
// import :advanced;
// import :constants;

【使用模块】
------------------------------------------------------------
// main.cpp
import math;  // 导入主模块（包含所有分区）

int main() {
    // 使用基础运算
    int sum = add(10, 20);

    // 使用高级运算
    int pow_result = power(2, 10);
    int fact_result = factorial(5);

    // 使用常量
    double area = PI * 10 * 10;

    return 0;
}

【关键点】
1. export module 模块名:分区名; - 声明分区
2. export import :分区名; - 导入并重新导出分区
3. import :分区名; - 只导入分区（不重新导出）
4. 分区只能在模块内部使用，外部不能直接导入分区
)" << endl;

    // --------------------------------------------
    // 示例4：子模块（Submodule）
    // --------------------------------------------

    cout << "\n【示例4】子模块" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【子模块的概念】

子模块是独立的模块，但名称上有层次关系。

语法：
- export module 父模块.子模块;

【文件结构】
------------------------------------------------------------
math/
├── math.cppm           # 主模块
├── math.basic.cppm     # 子模块：基础运算
└── math.advanced.cppm  # 子模块：高级运算

【math.cppm - 主模块】
------------------------------------------------------------
export module math;

export int add(int a, int b) {
    return a + b;
}

【math.basic.cppm - 子模块】
------------------------------------------------------------
export module math.basic;  // 子模块

export int multiply(int a, int b) {
    return a * b;
}

export int divide(int a, int b) {
    if (b == 0) throw runtime_error("Division by zero");
    return a / b;
}

【math.advanced.cppm - 子模块】
------------------------------------------------------------
export module math.advanced;  // 子模块

export int power(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

【使用模块】
------------------------------------------------------------
// main.cpp
import math;           // 导入主模块
import math.basic;     // 导入子模块
import math.advanced;  // 导入子模块

int main() {
    // 使用主模块
    int sum = add(10, 20);

    // 使用子模块
    int product = multiply(5, 6);
    int pow_result = power(2, 10);

    return 0;
}

【关键点】
1. 子模块是独立的模块，不是分区
2. 子模块需要单独导入
3. 导入主模块不会自动导入子模块
4. 子模块可以有自己的分区
)" << endl;

    // --------------------------------------------
    // 示例5：模块的可见性控制
    // --------------------------------------------

    cout << "\n【示例5】模块的可见性控制" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【可见性规则】

模块提供了强大的封装性，默认情况下：
1. 未 export 的内容是模块私有的
2. export 的内容对外可见
3. 类的 private 成员仍然是私有的

【示例：可见性控制】
------------------------------------------------------------
export module utils;

// 1. 导出函数（公开）
export int public_function() {
    return 42;
}

// 2. 私有函数（模块内部使用）
int private_function() {
    return 100;
}

// 3. 导出类
export class PublicClass {
public:
    void public_method() {
        // 可以调用私有函数
        int x = private_function();
    }

private:
    int private_member;
};

// 4. 私有类（模块内部使用）
class PrivateClass {
    // 只在模块内部使用
};

// 5. 导出常量
export const int PUBLIC_CONSTANT = 100;

// 6. 私有常量
const int PRIVATE_CONSTANT = 200;

// 7. 导出类型别名
export using Number = int;

// 8. 私有类型别名
using InternalType = double;

【使用模块】
------------------------------------------------------------
// main.cpp
import utils;

int main() {
    // ✅ 可以使用导出的函数
    int x = public_function();

    // ❌ 不能使用私有函数
    // int y = private_function();  // 编译错误

    // ✅ 可以使用导出的类
    PublicClass obj;
    obj.public_method();

    // ❌ 不能使用私有类
    // PrivateClass obj2;  // 编译错误

    // ✅ 可以使用导出的常量
    int c1 = PUBLIC_CONSTANT;

    // ❌ 不能使用私有常量
    // int c2 = PRIVATE_CONSTANT;  // 编译错误

    // ✅ 可以使用导出的类型别名
    Number n = 42;

    // ❌ 不能使用私有类型别名
    // InternalType d = 3.14;  // 编译错误

    return 0;
}

【可见性总结】

┌──────────────┬────────────┬────────────┐
│   声明       │  是否导出  │  外部可见  │
├──────────────┼────────────┼────────────┤
│ export 函数  │ 是         │ 是         │
│ 普通函数     │ 否         │ 否         │
│ export 类    │ 是         │ 是         │
│ 普通类       │ 否         │ 否         │
│ export 常量  │ 是         │ 是         │
│ 普通常量     │ 否         │ 否         │
└──────────────┴────────────┴────────────┘

【封装性优势】
1. 模块默认私有，避免符号污染
2. 只导出必要的接口，隐藏实现细节
3. 修改私有实现不影响外部代码
4. 更好的代码组织和维护
)" << endl;

    // --------------------------------------------
    // 示例6：模块 vs 命名空间
    // --------------------------------------------

    cout << "\n【示例6】模块 vs 命名空间" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【模块 vs 命名空间对比】

模块和命名空间都用于组织代码，但有本质区别：

┌──────────────┬────────────────┬────────────────┐
│   特性       │   命名空间     │   模块         │
├──────────────┼────────────────┼────────────────┤
│ 作用域       │ 逻辑分组       │ 编译单元       │
│ 可见性       │ 全部公开       │ 默认私有       │
│ 编译         │ 每次都编译     │ 只编译一次     │
│ 依赖管理     │ 无             │ 清晰           │
│ 符号冲突     │ 容易           │ 不容易         │
└──────────────┴────────────────┴────────────────┘

【命名空间示例】
------------------------------------------------------------
// math.h
namespace math {
    int add(int a, int b);
    int subtract(int a, int b);
}

// main.cpp
#include "math.h"

int main() {
    int sum = math::add(10, 20);
    return 0;
}

特点：
- 所有内容都公开
- 每次包含都要重新编译
- 只是逻辑分组，不是编译单元

【模块示例】
------------------------------------------------------------
// math.cppm
export module math;

export int add(int a, int b) {
    return a + b;
}

int helper() {  // 私有函数
    return 42;
}

// main.cpp
import math;

int main() {
    int sum = add(10, 20);  // 直接使用，不需要命名空间
    // int x = helper();    // 编译错误：helper 未导出
    return 0;
}

特点：
- 默认私有，只有 export 的内容才公开
- 只编译一次，生成 BMI
- 是编译单元，有清晰的依赖关系

【结合使用】
------------------------------------------------------------
// 可以在模块中使用命名空间
export module math;

export namespace math {
    int add(int a, int b) {
        return a + b;
    }
}

// main.cpp
import math;

int main() {
    int sum = math::add(10, 20);
    return 0;
}

【建议】
1. 使用模块组织编译单元
2. 使用命名空间组织逻辑分组
3. 模块提供封装性，命名空间提供逻辑分组
4. 两者可以结合使用
)" << endl;

    cout << "\n程序结束" << endl;
    return 0;
}
