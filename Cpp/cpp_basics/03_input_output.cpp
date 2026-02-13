// ============================================
// 03_input_output.cpp - 输入输出详解
// 日期：2026-02-12（第1天）
// 难度：⭐ 初级
// ============================================

/*
【核心概念】输入输出流

是什么？
- cout（console output）：控制台输出，用于显示信息
- cin（console input）：控制台输入，用于接收用户输入
- 流（stream）：数据的流动，像水流一样

为什么需要？
- 程序需要与用户交互（显示信息、接收输入）
- 输入输出是程序最基本的功能

核心特点：
- cout 使用 << 运算符（插入运算符）
- cin 使用 >> 运算符（提取运算符）
- 可以连续使用多个 << 或 >>
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
#include <iomanip>   // 用于格式化输出（setw, setprecision等）
#include <string>
using namespace std;

int main() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 输出流 cout 详解
    // --------------------------------------------

    cout << "\n1.1 输出流 cout 详解" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【cout - 控制台输出】

是什么？
- cout 是 console output 的缩写
- 是一个输出流对象，用于向控制台输出数据
- 定义在 <iostream> 头文件中

语法：
cout << 数据1 << 数据2 << ... << endl;

运算符：
- << 叫做"插入运算符"或"流插入运算符"
- 将右边的数据插入到左边的流中
- 可以连续使用多个 <<

换行方式：
1. endl：输出换行符并刷新缓冲区
2. \n：只输出换行符，不刷新缓冲区（更快）

示例：
cout << "Hello" << endl;           // 输出Hello并换行
cout << "Age: " << 20 << endl;     // 输出文字和数字
cout << "A" << "B" << "C" << endl; // 连续输出多个内容
)" << endl;

    // --------------------------------------------
    // 1.2 输入流 cin 详解
    // --------------------------------------------

    cout << "\n1.2 输入流 cin 详解" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【cin - 控制台输入】

是什么？
- cin 是 console input 的缩写
- 是一个输入流对象，用于从控制台读取数据
- 定义在 <iostream> 头文件中

语法：
cin >> 变量1 >> 变量2 >> ...;

运算符：
- >> 叫做"提取运算符"或"流提取运算符"
- 从左边的流中提取数据，存入右边的变量
- 可以连续使用多个 >>

工作原理：
1. cin 会等待用户输入
2. 用户输入数据后按回车
3. cin 根据变量类型读取相应的数据
4. 自动跳过空格、制表符、换行符

示例：
int age;
cin >> age;                    // 读取一个整数

double height;
cin >> height;                 // 读取一个浮点数

int a, b;
cin >> a >> b;                 // 连续读取两个整数
)" << endl;

    // ============================================
    // 第二部分：基础操作实践
    // ============================================

    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 2.1 cout 基本用法
    // --------------------------------------------

    cout << "\n2.1 cout 基本用法" << endl;
    cout << string(60, '-') << endl;

    // 输出字符串
    cout << "这是一个字符串" << endl;

    // 输出数字
    cout << "整数：" << 42 << endl;
    cout << "浮点数：" << 3.14159 << endl;

    // 输出变量
    int age = 20;
    double height = 1.75;
    cout << "年龄：" << age << endl;
    cout << "身高：" << height << " 米" << endl;

    // 连续输出多个内容
    cout << "姓名：" << "张三" << "，年龄：" << age << "，身高：" << height << endl;

    // 不换行输出
    cout << "第一部分 ";
    cout << "第二部分 ";
    cout << "第三部分" << endl;  // 最后换行

    // 使用 \n 换行（比 endl 更快）
    cout << "第一行\n";
    cout << "第二行\n";
    cout << "第三行\n";

    // --------------------------------------------
    // 2.2 cin 基本用法
    // --------------------------------------------

    cout << "\n2.2 cin 基本用法" << endl;
    cout << string(60, '-') << endl;

    // 读取整数
    int userAge;
    cout << "请输入你的年龄：";
    cin >> userAge;
    cout << "你输入的年龄是：" << userAge << endl;

    // 读取浮点数
    double userHeight;
    cout << "请输入你的身高（米）：";
    cin >> userHeight;
    cout << "你输入的身高是：" << userHeight << " 米" << endl;

    // 读取字符
    char grade;
    cout << "请输入你的成绩等级（A/B/C/D）：";
    cin >> grade;
    cout << "你的成绩等级是：" << grade << endl;

    // 连续读取多个值
    int num1, num2;
    cout << "请输入两个整数（用空格分隔）：";
    cin >> num1 >> num2;
    cout << "你输入的两个数是：" << num1 << " 和 " << num2 << endl;
    cout << "它们的和是：" << (num1 + num2) << endl;

    // --------------------------------------------
    // 2.3 读取字符串
    // --------------------------------------------

    cout << "\n2.3 读取字符串" << endl;
    cout << string(60, '-') << endl;

    // 方式1：使用 cin >>（只能读取一个单词，遇到空格停止）
    string firstName;
    cout << "请输入你的名字（单个单词）：";
    cin >> firstName;
    cout << "你的名字是：" << firstName << endl;

    // 清除输入缓冲区中的换行符
    cin.ignore();  // 忽略缓冲区中的一个字符（通常是换行符）

    // 方式2：使用 getline（可以读取包含空格的整行）
    string fullName;
    cout << "请输入你的全名（可以包含空格）：";
    getline(cin, fullName);  // 读取整行，包括空格
    cout << "你的全名是：" << fullName << endl;

    // --------------------------------------------
    // 2.4 格式化输出
    // --------------------------------------------

    cout << "\n2.4 格式化输出" << endl;
    cout << string(60, '-') << endl;

    // 设置输出宽度（setw）
    cout << "使用 setw 对齐输出：" << endl;
    cout << setw(10) << "姓名" << setw(10) << "年龄" << setw(10) << "分数" << endl;
    cout << setw(10) << "张三" << setw(10) << 20 << setw(10) << 95 << endl;
    cout << setw(10) << "李四" << setw(10) << 22 << setw(10) << 88 << endl;

    // 设置浮点数精度（setprecision）
    double pi = 3.14159265358979;
    cout << "\n设置浮点数精度：" << endl;
    cout << "默认：" << pi << endl;
    cout << "精度2：" << setprecision(2) << pi << endl;
    cout << "精度5：" << setprecision(5) << pi << endl;
    cout << "精度10：" << setprecision(10) << pi << endl;

    // fixed：固定小数点表示法
    cout << "\n使用 fixed 固定小数位数：" << endl;
    cout << fixed << setprecision(2);  // 固定显示2位小数
    cout << "圆周率：" << pi << endl;
    cout << "价格：" << 19.5 << endl;

    // 恢复默认设置
    cout.unsetf(ios::fixed);
    cout << setprecision(6);  // 恢复默认精度

    // 左对齐和右对齐
    cout << "\n对齐方式：" << endl;
    cout << left;  // 设置左对齐
    cout << setw(15) << "左对齐" << setw(10) << 123 << endl;
    cout << right;  // 设置右对齐
    cout << setw(15) << "右对齐" << setw(10) << 123 << endl;

    // 填充字符（setfill）
    cout << "\n使用填充字符：" << endl;
    cout << setfill('*');  // 设置填充字符为 *
    cout << setw(20) << 12345 << endl;
    cout << setfill(' ');  // 恢复默认填充字符（空格）

    // --------------------------------------------
    // 2.5 布尔值的输出
    // --------------------------------------------

    cout << "\n2.5 布尔值的输出" << endl;
    cout << string(60, '-') << endl;

    bool isStudent = true;
    bool hasLicense = false;

    // 默认输出（1和0）
    cout << "默认输出：" << endl;
    cout << "是否是学生：" << isStudent << endl;    // 输出1
    cout << "是否有驾照：" << hasLicense << endl;   // 输出0

    // 使用 boolalpha 输出 true/false
    cout << "\n使用 boolalpha：" << endl;
    cout << boolalpha;
    cout << "是否是学生：" << isStudent << endl;    // 输出true
    cout << "是否有驾照：" << hasLicense << endl;   // 输出false

    // 恢复默认（输出1和0）
    cout << noboolalpha;

    // ============================================
    // 第三部分：深入理解（可选）
    // ============================================

    cout << "\n" << string(60, '=') << endl;
    cout << "🔍 深入理解：输入输出流的工作原理（选学）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【深入内容】

1. 什么是流（Stream）？
   - 流是数据的序列，像水流一样流动
   - 输入流：数据从外部流入程序（键盘 → 程序）
   - 输出流：数据从程序流出到外部（程序 → 屏幕）

2. 缓冲区（Buffer）
   - 输入输出都有缓冲区，暂时存储数据
   - cout 的缓冲区：数据先存在缓冲区，满了或遇到 endl 才输出
   - cin 的缓冲区：用户输入的数据先存在缓冲区，程序再读取

3. endl vs \n 的区别
   - endl：输出换行符 + 刷新缓冲区（flush）
   - \n：只输出换行符，不刷新缓冲区
   - endl 更安全但稍慢，\n 更快但可能延迟输出
   - 建议：调试时用 endl，性能要求高时用 \n

4. cin >> 的工作原理
   - cin >> 会跳过前导空白字符（空格、制表符、换行符）
   - 读取数据直到遇到空白字符
   - 空白字符留在缓冲区中
   - 这就是为什么 cin >> 后需要 cin.ignore()

5. 为什么 cin >> 读取字符串会在空格处停止？
   - cin >> 把空格当作分隔符
   - 遇到空格就认为一个数据读取完成
   - 要读取包含空格的字符串，使用 getline()

6. 输入验证
   - cin 读取失败时会设置错误标志
   - 可以用 cin.fail() 检查是否读取失败
   - 用 cin.clear() 清除错误标志
   - 用 cin.ignore() 清空缓冲区

示例：输入验证
int number;
cout << "请输入一个整数：";
cin >> number;
if (cin.fail()) {
    cout << "输入错误！" << endl;
    cin.clear();  // 清除错误标志
    cin.ignore(1000, '\n');  // 清空缓冲区
}
)" << endl;

    return 0;
}

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
常见错误1：cin >> 后直接使用 getline
❌ 错误代码：
int age;
string name;
cout << "请输入年龄：";
cin >> age;
cout << "请输入姓名：";
getline(cin, name);  // 读取不到内容！

结果：getline 直接读取到换行符，name 为空

原因：cin >> age 后，换行符留在缓冲区中，getline 读取到换行符就停止了

✅ 正确代码：
int age;
string name;
cout << "请输入年龄：";
cin >> age;
cin.ignore();  // 忽略缓冲区中的换行符
cout << "请输入姓名：";
getline(cin, name);

记忆技巧：cin >> 后使用 getline 前，先调用 cin.ignore()

────────────────────────────────────

常见错误2：读取字符串时遇到空格停止
❌ 错误代码：
string fullName;
cout << "请输入全名：";
cin >> fullName;  // 输入 "Zhang San"，只读取到 "Zhang"

结果：只读取到第一个单词

原因：cin >> 遇到空格就停止读取

✅ 正确代码：
string fullName;
cout << "请输入全名：";
cin.ignore();  // 如果前面有 cin >>，需要先清除换行符
getline(cin, fullName);  // 读取整行，包括空格

记忆技巧：读取包含空格的字符串，使用 getline

────────────────────────────────────

常见错误3：输入类型不匹配
❌ 错误代码：
int age;
cout << "请输入年龄：";
cin >> age;  // 用户输入 "abc"

结果：cin 读取失败，age 的值不确定，程序可能出现异常行为

原因：cin >> 期望读取整数，但用户输入了字母

✅ 正确代码：
int age;
cout << "请输入年龄：";
cin >> age;
if (cin.fail()) {  // 检查是否读取失败
    cout << "输入错误，请输入整数！" << endl;
    cin.clear();  // 清除错误标志
    cin.ignore(1000, '\n');  // 清空缓冲区
}

记忆技巧：使用 cin.fail() 检查输入是否成功

────────────────────────────────────

常见错误4：忘记包含 <iomanip> 头文件
❌ 错误代码：
#include <iostream>
using namespace std;
int main() {
    cout << setw(10) << 123 << endl;  // 错误：setw 未定义
    return 0;
}

结果：编译错误，提示 'setw' was not declared

原因：setw、setprecision 等格式化函数定义在 <iomanip> 中

✅ 正确代码：
#include <iostream>
#include <iomanip>  // 包含格式化输出头文件
using namespace std;
int main() {
    cout << setw(10) << 123 << endl;
    return 0;
}

记忆技巧：使用格式化输出（setw、setprecision等）必须包含 <iomanip>

────────────────────────────────────

常见错误5：setw 只对下一个输出有效
❌ 错误代码：
cout << setw(10);  // 期望后面所有输出都宽度为10
cout << 123 << endl;
cout << 456 << endl;  // 实际上这里宽度不是10

结果：只有第一个输出（123）宽度为10，第二个（456）恢复默认

原因：setw 只对紧跟的下一个输出有效，不是持久设置

✅ 正确代码：
cout << setw(10) << 123 << endl;
cout << setw(10) << 456 << endl;  // 每次都要设置

记忆技巧：setw 是一次性的，每次输出都要重新设置

────────────────────────────────────

常见错误6：混淆 setprecision 的两种模式
❌ 错误代码：
double pi = 3.14159;
cout << setprecision(2) << pi << endl;  // 期望输出3.14

结果：可能输出 3.1（总共2位有效数字）

原因：默认模式下，setprecision 设置的是有效数字位数，不是小数位数

✅ 正确代码：
double pi = 3.14159;
cout << fixed << setprecision(2) << pi << endl;  // 输出3.14

说明：
- 不加 fixed：setprecision 设置有效数字位数
- 加 fixed：setprecision 设置小数位数

记忆技巧：要固定小数位数，使用 fixed + setprecision

────────────────────────────────────

常见错误7：输出布尔值时忘记 boolalpha
❌ 错误代码：
bool isTrue = true;
cout << "结果：" << isTrue << endl;  // 输出：结果：1

结果：输出1而不是true

原因：默认情况下，布尔值输出为1（true）或0（false）

✅ 正确代码：
bool isTrue = true;
cout << boolalpha;  // 设置布尔值以文字形式输出
cout << "结果：" << isTrue << endl;  // 输出：结果：true

记忆技巧：要输出 true/false 文字，使用 boolalpha
*/

// ============================================
// 第五部分：函数卡片速查
// ============================================

/*
【cout 输出】

语法：cout << 数据1 << 数据2 << ...;
作用：向控制台输出数据
返回：ostream& 类型（可以连续使用 <<）

示例：
cout << "Hello" << endl;
cout << "Age: " << 20 << endl;

────────────────────────────────────

【cin 输入】

语法：cin >> 变量1 >> 变量2 >> ...;
作用：从控制台读取数据到变量
返回：istream& 类型（可以连续使用 >>）

示例：
int age;
cin >> age;
cin >> a >> b >> c;  // 连续读取

────────────────────────────────────

【getline 读取整行】

语法：getline(cin, 字符串变量);
作用：读取整行输入，包括空格
返回：istream& 类型

示例：
string name;
getline(cin, name);

────────────────────────────────────

【cin.ignore 忽略字符】

语法：cin.ignore(数量, 终止字符);
作用：忽略输入缓冲区中的字符
参数：
  - 数量：要忽略的字符数（默认1）
  - 终止字符：遇到此字符停止（默认EOF）

示例：
cin.ignore();  // 忽略一个字符
cin.ignore(1000, '\n');  // 忽略直到换行符

────────────────────────────────────

【setw 设置宽度】

语法：setw(宽度)
作用：设置下一个输出的宽度
返回：格式化操作符
注意：只对下一个输出有效

示例：
cout << setw(10) << 123 << endl;

────────────────────────────────────

【setprecision 设置精度】

语法：setprecision(位数)
作用：设置浮点数的精度
返回：格式化操作符
注意：配合 fixed 使用可设置小数位数

示例：
cout << setprecision(2) << 3.14159 << endl;  // 有效数字
cout << fixed << setprecision(2) << 3.14159 << endl;  // 小数位数

────────────────────────────────────

【fixed 固定小数点】

语法：fixed
作用：使用固定小数点表示法
返回：格式化操作符

示例：
cout << fixed << setprecision(2) << 3.14159 << endl;  // 输出3.14

────────────────────────────────────

【boolalpha 布尔文字输出】

语法：boolalpha
作用：使布尔值输出为 true/false 而不是 1/0
返回：格式化操作符

示例：
cout << boolalpha << true << endl;  // 输出true
*/

// ============================================
// 练习题
// ============================================

/*
1. 基本输入输出
   要求：
   - 提示用户输入姓名、年龄、身高
   - 读取用户输入
   - 格式化输出所有信息
   提示：使用 cin >> 和 cout <<

2. 读取包含空格的字符串
   要求：
   - 提示用户输入完整地址（可能包含空格）
   - 使用 getline 读取
   - 输出地址
   提示：注意 cin.ignore()

3. 格式化输出表格
   要求：输出以下格式的表格
   姓名      年龄      分数
   张三        20        95
   李四        22        88
   提示：使用 setw 对齐

4. 计算器程序
   要求：
   - 提示用户输入两个数字
   - 计算并输出它们的和、差、积、商
   - 商保留2位小数
   提示：使用 fixed 和 setprecision

5. 输入验证
   要求：
   - 提示用户输入一个整数
   - 如果输入不是整数，提示错误并重新输入
   - 直到输入正确为止
   提示：使用 cin.fail()、cin.clear()、cin.ignore()

6. 多行输入
   要求：
   - 提示用户输入3行文本
   - 使用 getline 读取每一行
   - 输出所有行
   提示：注意第一次使用 getline 前可能需要 cin.ignore()

7. 布尔值输出
   要求：
   - 定义几个布尔变量
   - 分别以数字形式（1/0）和文字形式（true/false）输出
   提示：使用 boolalpha 和 noboolalpha
*/

// ============================================
// 编译和运行
// ============================================

/*
【编译命令】
g++ -Wall -std=c++11 -o input_output 03_input_output.cpp

【运行命令】
Windows: input_output.exe
Linux/Mac: ./input_output

【注意事项】
- 运行时需要手动输入数据
- 按照提示输入相应类型的数据
- 输入多个数据时用空格分隔
*/
