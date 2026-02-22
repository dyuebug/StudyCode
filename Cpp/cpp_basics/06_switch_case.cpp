// ============================================
// 06_switch_case.cpp - switch-case多分支选择详解
// 日期：2026-02-18（第2天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】switch-case多分支选择

是什么？
- 根据一个变量的值，从多个分支中选择一个执行
- 是if-else if-else的替代方案，适合处理离散值的多分支判断

为什么需要？
- 当需要根据一个变量的不同值执行不同代码时
- 比多个if-else if更简洁、更高效
- 代码可读性更好，结构更清晰

核心特点：
- 只能判断整数类型（int、char、枚举）和C++11后的字符串
- 每个case后需要break，否则会"穿透"到下一个case
- 可以有default分支，处理所有未匹配的情况
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
#include <string>
#include <cstdlib>
using namespace std;

int main() {
    system("chcp 65001");  // 设置UTF-8编码，支持中文

    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;
    cout << endl;

    // --------------------------------------------
    // 1.1 概念1：switch-case基本语法
    // --------------------------------------------

    cout << "1.1 switch-case基本语法" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【switch-case语法】

完整语法：
switch (表达式) {
    case 值1:
        // 当表达式等于值1时执行
        break;
    case 值2:
        // 当表达式等于值2时执行
        break;
    case 值3:
        // 当表达式等于值3时执行
        break;
    default:
        // 当表达式不等于任何case值时执行
        break;
}

执行流程：
1. 计算switch后面的表达式的值
2. 从上到下依次与每个case的值比较
3. 找到匹配的case，执行对应代码
4. 遇到break，跳出整个switch结构
5. 如果没有匹配的case，执行default块（如果有）

⚠️ 关键点：
- 表达式必须是整数类型（int、char、枚举）
- case后面的值必须是常量（不能是变量）
- 每个case后面建议加break
- default是可选的，但建议加上
)" << endl;

    cout << endl;

    // --------------------------------------------
    // 1.2 概念2：break语句的作用
    // --------------------------------------------

    cout << "1.2 break语句的作用" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【break语句】

是什么？
- break是跳出语句，用于立即退出switch结构
- 执行break后，程序跳到switch后面的代码继续执行

为什么需要？
- 防止"case穿透"（fall through）
- 确保只执行匹配的case，不执行后续case

没有break会怎样？
- 执行完当前case后，会继续执行下一个case
- 直到遇到break或switch结束

示例：
switch (x) {
    case 1:
        cout << "1" << endl;
        // 没有break，会继续执行case 2
    case 2:
        cout << "2" << endl;
        break;
    case 3:
        cout << "3" << endl;
        break;
}

如果x=1，输出：
1
2

⚠️ 注意：
- 大多数情况下，每个case都要加break
- 有时故意不加break，实现"穿透"效果（高级用法）
)" << endl;

    cout << endl;

    // --------------------------------------------
    // 1.3 概念3：default分支
    // --------------------------------------------

    cout << "1.3 default分支" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【default分支】

是什么？
- default是默认分支，当所有case都不匹配时执行
- 类似于if-else中的最后一个else

为什么需要？
- 处理未预期的输入
- 提供友好的错误提示
- 确保程序的健壮性

语法：
switch (x) {
    case 1:
        // ...
        break;
    case 2:
        // ...
        break;
    default:
        // 所有case都不匹配时执行
        break;
}

⚠️ 注意：
- default可以放在任何位置，但习惯放在最后
- default是可选的，但建议加上
- default后面的break可以省略（如果在最后）
)" << endl;

    cout << endl;

    // --------------------------------------------
    // 1.4 概念4：switch vs if-else
    // --------------------------------------------

    cout << "1.4 switch vs if-else" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【switch vs if-else对比】

┌─────────────────┬──────────────────┬──────────────────┐
│     特性        │     switch       │     if-else      │
├─────────────────┼──────────────────┼──────────────────┤
│ 判断类型        │ 只能判断相等     │ 可以判断范围     │
│ 支持的数据类型  │ 整数、字符、枚举 │ 任意类型         │
│ 可读性          │ 多分支时更清晰   │ 复杂条件更灵活   │
│ 性能            │ 多分支时更快     │ 少分支时差不多   │
│ 适用场景        │ 离散值的多分支   │ 范围判断、复杂条件│
└─────────────────┴──────────────────┴──────────────────┘

使用switch的场景：
✅ 根据菜单选项执行不同操作
✅ 根据星期几显示不同信息
✅ 根据月份计算天数
✅ 根据字符执行不同操作

使用if-else的场景：
✅ 判断分数范围（60-69、70-79...）
✅ 判断年龄段（0-18、18-60、60+）
✅ 复杂的逻辑条件（age > 18 && isMember）
✅ 浮点数比较

记忆技巧：
- 离散值（1、2、3...）用switch
- 范围值（>=60、<80...）用if-else
)" << endl;

    cout << endl;

    // ============================================
    // 第二部分：基础操作实践
    // ============================================

    cout << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;
    cout << endl;

    // --------------------------------------------
    // 2.1 示例1：基本switch-case
    // --------------------------------------------

    cout << "2.1 示例1：基本switch-case" << endl;
    cout << string(60, '-') << endl;

    // 声明变量：int类型，存储星期几（1-7）
    int day = 3;

    cout << "今天是星期" << day << endl;

    // switch语句：根据day的值输出对应的星期名称
    // 表达式：day（整数类型）
    switch (day) {
        case 1:
            // 当day等于1时执行
            cout << "星期一（Monday）" << endl;
            break;  // 跳出switch
        case 2:
            // 当day等于2时执行
            cout << "星期二（Tuesday）" << endl;
            break;
        case 3:
            // 当day等于3时执行
            cout << "星期三（Wednesday）" << endl;
            break;
        case 4:
            cout << "星期四（Thursday）" << endl;
            break;
        case 5:
            cout << "星期五（Friday）" << endl;
            break;
        case 6:
            cout << "星期六（Saturday）" << endl;
            break;
        case 7:
            cout << "星期日（Sunday）" << endl;
            break;
        default:
            // 当day不是1-7时执行
            cout << "无效的星期数！" << endl;
            break;
    }

    cout << endl;

    // --------------------------------------------
    // 2.2 示例2：使用char类型
    // --------------------------------------------

    cout << "2.2 示例2：使用char类型" << endl;
    cout << string(60, '-') << endl;

    // 声明变量：char类型，存储成绩等级
    char grade = 'B';

    cout << "你的成绩等级：" << grade << endl;

    // switch语句：根据等级输出评语
    // 表达式：grade（字符类型）
    switch (grade) {
        case 'A':
            // 当grade等于'A'时执行
            cout << "优秀！继续保持！" << endl;
            break;
        case 'B':
            // 当grade等于'B'时执行
            cout << "良好！再接再厉！" << endl;
            break;
        case 'C':
            cout << "中等，还需努力！" << endl;
            break;
        case 'D':
            cout << "及格，需要加油！" << endl;
            break;
        case 'F':
            cout << "不及格，要更加努力！" << endl;
            break;
        default:
            // 当grade不是A/B/C/D/F时执行
            cout << "无效的等级！" << endl;
            break;
    }

    cout << endl;

    // --------------------------------------------
    // 2.3 示例3：多个case共享代码（case穿透）
    // --------------------------------------------

    cout << "2.3 示例3：多个case共享代码（case穿透）" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【case穿透技巧】

使用场景：
- 多个case执行相同的代码
- 故意不加break，让代码"穿透"到下一个case

语法：
switch (x) {
    case 1:
    case 2:
    case 3:
        // case 1、2、3都执行这里
        break;
    case 4:
    case 5:
        // case 4、5都执行这里
        break;
}
)" << endl;

    // 声明变量：int类型，存储月份（1-12）
    int month = 4;

    cout << "月份：" << month << endl;

    // switch语句：判断月份属于哪个季节
    // 利用case穿透，多个月份共享代码
    switch (month) {
        case 12:
        case 1:
        case 2:
            // 12、1、2月都执行这里
            cout << "冬季（Winter）" << endl;
            break;
        case 3:
        case 4:
        case 5:
            // 3、4、5月都执行这里
            cout << "春季（Spring）" << endl;
            break;
        case 6:
        case 7:
        case 8:
            // 6、7、8月都执行这里
            cout << "夏季（Summer）" << endl;
            break;
        case 9:
        case 10:
        case 11:
            // 9、10、11月都执行这里
            cout << "秋季（Autumn）" << endl;
            break;
        default:
            cout << "无效的月份！" << endl;
            break;
    }

    cout << endl;

    // --------------------------------------------
    // 2.4 示例4：switch中声明变量
    // --------------------------------------------

    cout << "2.4 示例4：switch中声明变量" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【switch中声明变量】

⚠️ 注意事项：
- 不能在case标签后直接声明变量
- 需要用大括号{}创建作用域

错误写法：
case 1:
    int x = 10;  // 错误！
    break;

正确写法：
case 1: {
    int x = 10;  // 正确！
    cout << x << endl;
    break;
}
)" << endl;

    int option = 1;
    cout << "选项：" << option << endl;

    // switch语句：在case中声明变量
    switch (option) {
        case 1: {
            // 用大括号创建作用域
            // 在作用域内声明变量
            int result = 100;
            cout << "选项1，结果：" << result << endl;
            break;
        }
        case 2: {
            int result = 200;  // 不同作用域，可以重名
            cout << "选项2，结果：" << result << endl;
            break;
        }
        default:
            cout << "其他选项" << endl;
            break;
    }

    cout << endl;

    // --------------------------------------------
    // 2.5 示例5：简易计算器（switch版本）
    // --------------------------------------------

    cout << "2.5 示例5：简易计算器（switch版本）" << endl;
    cout << string(60, '-') << endl;

    // 声明变量
    double num1 = 10.0;
    double num2 = 5.0;
    char op = '+';

    cout << "计算：" << num1 << " " << op << " " << num2 << endl;

    // switch语句：根据运算符执行不同操作
    // 表达式：op（字符类型）
    switch (op) {
        case '+':
            // 加法
            cout << "结果：" << (num1 + num2) << endl;
            break;
        case '-':
            // 减法
            cout << "结果：" << (num1 - num2) << endl;
            break;
        case '*':
            // 乘法
            cout << "结果：" << (num1 * num2) << endl;
            break;
        case '/':
            // 除法，需要检查除零
            if (num2 != 0) {
                cout << "结果：" << (num1 / num2) << endl;
            } else {
                cout << "错误：除数不能为0！" << endl;
            }
            break;
        default:
            // 无效的运算符
            cout << "错误：无效的运算符！" << endl;
            break;
    }

    cout << endl;

    // --------------------------------------------
    // 2.6 示例6：菜单系统
    // --------------------------------------------

    cout << "2.6 示例6：菜单系统" << endl;
    cout << string(60, '-') << endl;

    // 显示菜单
    cout << "========== 主菜单 ==========" << endl;
    cout << "1. 新建文件" << endl;
    cout << "2. 打开文件" << endl;
    cout << "3. 保存文件" << endl;
    cout << "4. 退出程序" << endl;
    cout << "============================" << endl;

    // 模拟用户选择
    int choice = 2;
    cout << "你的选择：" << choice << endl;

    // switch语句：根据选择执行不同操作
    switch (choice) {
        case 1:
            cout << "执行：新建文件..." << endl;
            cout << "文件已创建！" << endl;
            break;
        case 2:
            cout << "执行：打开文件..." << endl;
            cout << "请输入文件名：" << endl;
            break;
        case 3:
            cout << "执行：保存文件..." << endl;
            cout << "文件已保存！" << endl;
            break;
        case 4:
            cout << "执行：退出程序..." << endl;
            cout << "再见！" << endl;
            break;
        default:
            cout << "错误：无效的选择！" << endl;
            cout << "请输入1-4之间的数字。" << endl;
            break;
    }

    cout << endl;

    // ============================================
    // 第三部分：深入理解（可选）
    // ============================================

    cout << string(60, '=') << endl;
    cout << "🔍 深入理解：switch-case（选学）" << endl;
    cout << string(60, '=') << endl;
    cout << "如果你想知道\"为什么\"，可以阅读这部分。" << endl;
    cout << "如果觉得难，可以跳过，不影响后续学习。" << endl;
    cout << string(60, '=') << endl;
    cout << endl;

    cout << R"(
【深入1：switch的底层实现】

编译器优化策略：
1. 跳转表（Jump Table）
   - 当case值连续且密集时使用
   - 时间复杂度：O(1)
   - 空间换时间

2. 二分查找（Binary Search）
   - 当case值稀疏时使用
   - 时间复杂度：O(log n)

3. if-else链
   - 当case很少时使用
   - 时间复杂度：O(n)

示例：
switch (x) {
    case 1: ...
    case 2: ...
    case 3: ...
}

编译器可能生成跳转表：
jump_table[x]();  // 直接跳转，O(1)

────────────────────────────────────

【深入2：为什么switch只支持整数类型？】

历史原因：
- switch源自C语言，设计时只考虑了整数
- 整数可以直接用作数组索引（跳转表）
- 字符本质上也是整数（ASCII码）

C++11后的改进：
- 支持枚举类型（本质也是整数）
- 不支持字符串（因为字符串比较复杂）

字符串的替代方案：
- 使用if-else
- 使用map<string, function>
- 使用哈希表

────────────────────────────────────

【深入3：case穿透的实际应用】

合理使用场景：
1. 多个值执行相同操作
   case 1:
   case 2:
   case 3:
       // 共同操作
       break;

2. 累积操作（少见）
   case 3:
       doSomething3();
       // 故意不break，继续执行case 2
   case 2:
       doSomething2();
       // 故意不break，继续执行case 1
   case 1:
       doSomething1();
       break;

⚠️ 注意：
- 故意穿透时，建议加注释说明
- 避免意外穿透（忘记break）

────────────────────────────────────

【深入4：switch vs if-else性能对比】

性能测试（100万次循环）：
- switch（连续case）：~10ms（跳转表）
- switch（稀疏case）：~20ms（二分查找）
- if-else（3个分支）：~15ms
- if-else（10个分支）：~50ms

结论：
- case少时（<5个），性能差不多
- case多时（>10个），switch更快
- 但现代CPU的分支预测很强，差距不大

建议：
- 优先考虑代码可读性
- 性能关键代码才需要优化
- 用性能分析工具测量，不要猜测
)" << endl;

    cout << endl;

    // ============================================
    // 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
    // ============================================

    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;
    cout << endl;

    cout << R"(
────────────────────────────────────
❌ 错误1：忘记break导致case穿透
────────────────────────────────────

错误代码：
int x = 1;
switch (x) {
    case 1:
        cout << "1" << endl;
        // 忘记break！
    case 2:
        cout << "2" << endl;
        break;
    case 3:
        cout << "3" << endl;
        break;
}

结果：
输出：
1
2

原因：
- case 1执行后，没有break
- 继续执行case 2的代码（case穿透）
- 这通常不是预期行为

✅ 正确代码：
int x = 1;
switch (x) {
    case 1:
        cout << "1" << endl;
        break;  // 加上break
    case 2:
        cout << "2" << endl;
        break;
    case 3:
        cout << "3" << endl;
        break;
}

结果：
输出：1

记忆技巧：
- 每个case后面都加break（除非故意穿透）
- 使用编译器警告选项（-Wimplicit-fallthrough）
- 故意穿透时，加注释说明：// fall through

────────────────────────────────────
❌ 错误2：case后面使用变量而非常量
────────────────────────────────────

错误代码：
int x = 1;
int y = 1;
switch (x) {
    case y:  // 错误！y是变量，不是常量
        cout << "匹配" << endl;
        break;
}

结果：
编译错误：case label does not reduce to an integer constant

原因：
- case后面必须是编译时常量
- 变量的值在运行时才确定
- 编译器无法生成跳转表

✅ 正确代码：
int x = 1;
const int y = 1;  // 使用const常量
switch (x) {
    case y:  // 正确！y是常量
        cout << "匹配" << endl;
        break;
}

或者直接使用字面量：
switch (x) {
    case 1:  // 直接使用常量
        cout << "匹配" << endl;
        break;
}

记忆技巧：
- case后面只能是常量（字面量或const变量）
- 不能是普通变量或表达式

────────────────────────────────────
❌ 错误3：在case中直接声明变量
────────────────────────────────────

错误代码：
int x = 1;
switch (x) {
    case 1:
        int result = 100;  // 错误！
        cout << result << endl;
        break;
    case 2:
        result = 200;  // 错误！result的作用域有问题
        cout << result << endl;
        break;
}

结果：
编译错误：crosses initialization of 'int result'

原因：
- switch的所有case共享同一个作用域
- case 1声明的变量，case 2也能看到
- 但case 2可能跳过case 1的初始化，导致未定义行为

✅ 正确代码：
int x = 1;
switch (x) {
    case 1: {  // 用大括号创建作用域
        int result = 100;
        cout << result << endl;
        break;
    }
    case 2: {  // 独立的作用域
        int result = 200;  // 可以重名
        cout << result << endl;
        break;
    }
}

记忆技巧：
- 在case中声明变量，必须用大括号{}
- 大括号创建独立作用域，避免变量冲突

────────────────────────────────────
❌ 错误4：switch中使用浮点数
────────────────────────────────────

错误代码：
double x = 1.5;
switch (x) {  // 错误！switch不支持浮点数
    case 1.5:
        cout << "1.5" << endl;
        break;
}

结果：
编译错误：switch quantity not an integer

原因：
- switch只支持整数类型（int、char、枚举）
- 浮点数不能用作switch表达式

✅ 正确代码：
使用if-else代替：
double x = 1.5;
if (x == 1.5) {
    cout << "1.5" << endl;
} else if (x == 2.5) {
    cout << "2.5" << endl;
}

或者转换为整数（如果合适）：
double x = 1.5;
int intX = (int)(x * 10);  // 转换为15
switch (intX) {
    case 15:  // 对应1.5
        cout << "1.5" << endl;
        break;
    case 25:  // 对应2.5
        cout << "2.5" << endl;
        break;
}

记忆技巧：
- switch只支持整数类型
- 浮点数用if-else

────────────────────────────────────
❌ 错误5：重复的case值
────────────────────────────────────

错误代码：
int x = 1;
switch (x) {
    case 1:
        cout << "第一个1" << endl;
        break;
    case 1:  // 错误！重复的case值
        cout << "第二个1" << endl;
        break;
}

结果：
编译错误：duplicate case value

原因：
- 每个case值必须唯一
- 否则编译器无法确定执行哪个分支

✅ 正确代码：
int x = 1;
switch (x) {
    case 1:
        cout << "1" << endl;
        break;
    case 2:  // 不同的值
        cout << "2" << endl;
        break;
}

记忆技巧：
- 每个case值必须唯一
- 编译器会检查，容易发现

────────────────────────────────────
❌ 错误6：default不在最后导致逻辑混乱
────────────────────────────────────

错误代码：
int x = 5;
switch (x) {
    case 1:
        cout << "1" << endl;
        break;
    default:
        cout << "其他" << endl;
        // 忘记break！
    case 2:
        cout << "2" << endl;
        break;
}

结果：
当x=5时，输出：
其他
2

原因：
- default可以放在任何位置
- 但如果忘记break，会穿透到下一个case
- 导致意外行为

✅ 正确代码：
int x = 5;
switch (x) {
    case 1:
        cout << "1" << endl;
        break;
    case 2:
        cout << "2" << endl;
        break;
    default:  // 习惯放在最后
        cout << "其他" << endl;
        break;  // 即使在最后，也建议加break
}

记忆技巧：
- default习惯放在最后
- 即使在最后，也加break（养成习惯）

────────────────────────────────────
❌ 错误7：switch表达式有副作用
────────────────────────────────────

错误代码：
int x = 0;
switch (x++) {  // 错误！表达式有副作用
    case 0:
        cout << "x=" << x << endl;  // x=1
        break;
    case 1:
        cout << "x=" << x << endl;
        break;
}

问题：
- switch (x++) 先计算x的值（0），再自增
- 匹配case 0，但此时x已经变成1
- 容易产生混淆

✅ 正确代码：
int x = 0;
int temp = x;  // 先保存值
x++;           // 再修改
switch (temp) {
    case 0:
        cout << "x=" << x << endl;
        break;
    case 1:
        cout << "x=" << x << endl;
        break;
}

或者：
int x = 0;
switch (x) {  // 不在switch中修改
    case 0:
        x++;  // 在case中修改
        cout << "x=" << x << endl;
        break;
}

记忆技巧：
- switch表达式不要有副作用（++、--、赋值等）
- 保持代码清晰，避免混淆
)" << endl;

    cout << endl;

    // ============================================
    // 第五部分：函数卡片速查
    // ============================================

    cout << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;
    cout << endl;

    cout << R"(
【语句1：switch-case】

语法：
switch (表达式) {
    case 常量1:
        // 代码
        break;
    case 常量2:
        // 代码
        break;
    default:
        // 代码
        break;
}

参数：
  - 表达式: 整数类型（int、char、枚举）
  - 常量: 编译时常量（字面量或const变量）

执行流程：
1. 计算表达式的值
2. 与每个case值比较
3. 匹配则执行对应代码
4. 遇到break跳出

示例：
int day = 1;
switch (day) {
    case 1:
        cout << "星期一" << endl;
        break;
    case 2:
        cout << "星期二" << endl;
        break;
    default:
        cout << "其他" << endl;
        break;
}

⚠️ 注意：
- 每个case后加break
- case值必须是常量
- 只支持整数类型

────────────────────────────────────

【语句2：break】

语法：
break;

作用：
- 立即退出switch结构
- 跳转到switch后面的代码

使用场景：
- 每个case后面（防止穿透）
- 循环中（提前退出循环，后续学习）

示例：
switch (x) {
    case 1:
        cout << "1" << endl;
        break;  // 退出switch
    case 2:
        cout << "2" << endl;
        break;
}

⚠️ 注意：
- 忘记break会导致case穿透
- 故意穿透时，加注释说明

────────────────────────────────────

【语句3：default】

语法：
default:
    // 代码
    break;

作用：
- 当所有case都不匹配时执行
- 类似if-else的最后一个else

位置：
- 可以放在任何位置
- 习惯放在最后

示例：
switch (x) {
    case 1:
        cout << "1" << endl;
        break;
    default:
        cout << "其他" << endl;
        break;
}

⚠️ 注意：
- default是可选的，但建议加上
- 即使在最后，也建议加break
)" << endl;

    cout << endl;

    // ============================================
    // 第六部分：练习题
    // ============================================

    cout << string(60, '=') << endl;
    cout << "第六部分：练习题" << endl;
    cout << string(60, '=') << endl;
    cout << endl;

    cout << R"(
请在 06_switch_case_exercise.cpp 中完成以下练习：

1. 星期查询
   要求：输入数字1-7，输出对应的星期名称
   提示：使用switch-case，每个case对应一个星期

2. 月份天数查询
   要求：输入月份（1-12），输出该月的天数
   提示：
   - 1、3、5、7、8、10、12月：31天
   - 4、6、9、11月：30天
   - 2月：28天（暂不考虑闰年）
   - 使用case穿透技巧

3. 简易计算器（完整版）
   要求：输入两个数和运算符（+、-、*、/、%），输出结果
   提示：
   - 使用switch判断运算符
   - 除法和取模要检查除零
   - 处理无效运算符

4. 成绩等级转换
   要求：输入等级字母（A/B/C/D/F），输出对应的分数范围
   提示：
   - A: 90-100
   - B: 80-89
   - C: 70-79
   - D: 60-69
   - F: 0-59

5. 季节查询
   要求：输入月份（1-12），输出对应的季节
   提示：
   - 春季：3、4、5月
   - 夏季：6、7、8月
   - 秋季：9、10、11月
   - 冬季：12、1、2月
   - 使用case穿透

6. 菜单系统（完整版）
   要求：实现一个简单的菜单系统
   提示：
   - 显示菜单选项
   - 输入选择
   - 根据选择执行不同操作
   - 处理无效输入

7. 字符类型判断
   要求：输入一个字符，判断是数字、大写字母、小写字母还是其他
   提示：
   - 数字：'0'-'9'
   - 大写字母：'A'-'Z'
   - 小写字母：'a'-'z'
   - 其他：default
   - 使用case穿透

8. 运算符优先级查询
   要求：输入运算符（+、-、*、/、%），输出其优先级
   提示：
   - 优先级1（最高）：*、/、%
   - 优先级2：+、-
   - 使用case穿透
)" << endl;

    cout << endl;
    cout << string(60, '=') << endl;
    cout << "06_switch_case.cpp 学习完成！" << endl;
    cout << "编译命令：g++ -o 06_switch_case 06_switch_case.cpp" << endl;
    cout << "运行命令：./06_switch_case 或 06_switch_case.exe" << endl;
    cout << string(60, '=') << endl;

    return 0;
}
