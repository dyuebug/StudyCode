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

#include <iostream>
#include <iomanip>  // 用于格式化输出
#include <string>
#include <limits>  // 用于输入验证
#include <cstdlib>  // 用于系统函数（如 system("pause")）

using namespace std;

int main() {
    system("chcp 65001");  // 设置控制台编码为 UTF-8，支持中文输出
    //1
    string name;
    int age;
    double height;

    cout << "请输入你的姓名：";
    cin >> name;  // 只能读取一个单词，遇到空格停止

    cout << "请输入你的年龄：";
    cin >> age;

    cout << "请输入你的身高（单位：米）：";
    cin >> height;

    cout << "\n你输入的信息如下：" << endl;
    cout << "姓名：" << name << endl;
    cout << "年龄：" << age << " 岁" << endl;
    cout << "身高：" << fixed << setprecision(2) << height << " 米" << endl;

    //2
    cout << "\n请输入你的完整地址：";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');  // 清空输入缓冲区
    string address;
    getline(cin, address);  // 读取整行输入，包括空格

    cout << "\n你输入的地址是：" << address << endl;
    //3
    cout << "\n" << string(60, '=') << endl;
    cout << "姓名" << setw(10) << "年龄" << setw(10) << "分数" << endl;
    cout << "张三" << setw(10) << 20 << setw(10) << 95 << endl;
    cout << "李四" << setw(10) << 22 << setw(10) << 88 << endl; 
    //4
    double num1, num2;
    cout << "\n请输入两个数字：";
    cin >> num1 >> num2;
    cout << "和：" << num1 + num2 << endl;
    cout << "差：" << num1 - num2 << endl;
    cout << "积：" << num1 * num2 << endl;
    cout << "商：" << fixed << setprecision(2) << num1 / num2 << endl;
    //5
    int integerInput;
    while (true) {
        cout << "\n请输入一个整数：";
        cin >> integerInput;

        if (cin.fail()) {
            cout << "输入错误，请输入一个整数！" << endl;
            cin.clear();  // 清除错误状态
            cin.ignore(numeric_limits<streamsize>::max(), '\n');  // 忽略错误输入
        } else {
            break;  // 输入正确，退出循环
        }
    }
    cout << "你输入的整数是：" << integerInput << endl;
    //6
    cout << "\n请输入3行文本：" << endl;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');  // 清空输入缓冲区
    string line1, line2, line3;
    getline(cin, line1);
    getline(cin, line2);
    getline(cin, line3);
    cout << "\n你输入的文本是：" << endl;
    cout << line1 << endl;
    cout << line2 << endl;
    cout << line3 << endl;
    //7
    bool flag1 = true;
    bool flag2 = false;
    cout << "\n布尔值输出：" << endl;
    cout << "默认输出（数字形式）：" << endl;
    cout << "flag1: " << flag1 << endl;  // 输出1
    cout << "flag2: " << flag2 << endl;  // 输出0
    cout << "\n使用 boolalpha 输出（文字形式）：" << endl;
    cout << boolalpha;  
    cout << "flag1: " << flag1 << endl;  // 输出true
    cout << "flag2: " << flag2 << endl;  // 输出false
    cout << "\n恢复默认输出（数字形式）：" << endl;
    cout << noboolalpha;
    cout << "flag1: " << flag1 << endl;  // 输出1
    cout << "flag2: " << flag2 << endl;  // 输出0
    return 0;
}