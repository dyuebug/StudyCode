/*
1. 声明并初始化变量
   要求：声明以下变量并赋予合适的初始值
   - 你的年龄（整数）
   - 你的身高（浮点数，单位：米）
   - 你的姓名首字母（字符）
   - 你是否是学生（布尔值）
   - 你的完整姓名（字符串）
   提示：选择合适的数据类型

2. 计算圆的面积和周长
   要求：
   - 定义常量 PI = 3.14159
   - 定义变量 radius（半径）= 5.0
   - 计算面积（area = PI * radius * radius）
   - 计算周长（circumference = 2 * PI * radius）
   - 输出结果
   提示：使用 const 定义常量

3. 类型转换练习
   要求：
   - 定义 int a = 10, b = 3
   - 计算 a / b 的整数结果
   - 计算 a / b 的浮点数结果（保留小数）
   - 输出两个结果
   提示：使用类型转换

4. 变量交换
   要求：
   - 定义 int x = 5, y = 10
   - 不使用第三个变量，交换 x 和 y 的值
   - 输出交换后的结果
   提示：使用算术运算（x = x + y; y = x - y; x = x - y;）

5. 温度转换
   要求：
   - 定义摄氏温度 celsius = 25.0
   - 转换为华氏温度（fahrenheit = celsius * 9.0 / 5.0 + 32.0）
   - 输出结果
   提示：注意使用浮点数运算

6. 字符ASCII码
   要求：
   - 定义字符变量 ch = 'A'
   - 输出该字符
   - 输出该字符的ASCII码值
   - 输出下一个字符（'B'）
   提示：使用 (int)ch 获取ASCII码，ch + 1 获取下一个字符

7. 变量命名练习
   要求：判断以下变量名是否合法，如果不合法说明原因
   - student_age
   - 2ndPlace
   - my-name
   - _count
   - int
   - MAX_SIZE
   提示：回顾变量命名规则
*/
#include <iostream>
#include <string>    // 使用 string 类型需要包含这个头文件
#include <cstdlib>   // 包含 system() 函数
using namespace std;
int main() {
    system("chcp 65001");  // 设置控制台为UTF-8编码，解决中文乱码问题
    //1
    int age = 25;                // 你的年龄
    double height = 1.75;        // 你的身高（米）
    char initial = 'Y';          // 你的姓名首字母
    bool isStudent = false;       // 你是否是学生
    string fullName = "Yan Chen"; // 你的完整姓名

    //2
    const double PI = 3.14159;   // 圆周率常量
    double radius = 5.0;         // 圆的半径
    double area = PI * radius * radius;
    double circumference = 2 * PI * radius;
    cout << "圆的面积：" << area << endl;
    cout << "圆的周长：" << circumference << endl;

    //3
    int a = 10, b = 3;
    int intResult = a / b;  // 整数结果
    double floatResult = (double)a / b;  // 浮点数结果
    cout << "整数结果：" << intResult << endl;

    //4
    int x = 5, y = 10;
    x = x + y;  // x现在是15
    y = x - y;  // y现在是5
    x = x - y;  // x现在是10
    cout << "交换后的x：" << x << ", y：" << y << endl;

    //5
    double celsius = 25.0;
    double fahrenheit = celsius * 9.0 / 5.0 + 32.0;
    cout << "华氏温度：" << fahrenheit << endl;

    //6
    char ch = 'A';
    cout << "字符：" << ch << endl;
    cout << "ASCII码值：" << (int)ch << endl;
    cout << "下一个字符：" << (char)(ch + 1) << endl;

    //7
    // student_age - 合法
    // 2ndPlace - 不合法，变量名不能以数字开头
    // my-name - 不合法，变量名不能包含连字符
    // _count - 合法，虽然不推荐以_开头，但在某些情况下是允许的
    // int - 不合法，int是C++的关键字
    // MAX_SIZE - 合法，通常用大写字母表示常量
    return 0;
}