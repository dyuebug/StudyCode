// ============================================
// 练习题
// ============================================

/*
1. 修改程序，输出你的名字和年龄
   要求：使用 cout 输出 "我的名字是XXX，今年XX岁"
   提示：可以使用多个 << 连接输出

2. 输出一个简单的图案
   要求：使用多行 cout 输出以下图案：
   *****
   *   *
   *   *
   *****
   提示：每行一个 cout 语句

3. 尝试不使用 using namespace std
   要求：删除 using namespace std，修改代码使其能正常运行
   提示：需要在 cout 和 endl 前加 std::

4. 输出不同类型的数据
   要求：在一行中输出整数、浮点数、字符、字符串
   提示：使用多个 << 连接

5. 理解 endl 和 \n 的区别
   要求：分别使用 endl 和 \n 输出三行文字，观察效果
   提示：endl 会刷新缓冲区，\n 不会（初学阶段可以认为效果相同）
*/

#include <iostream>
using namespace std;
int main() {
    //1
    cout  << "my name is yanc, i am 25 years old" << endl;

    //2
    cout << "*****" << endl;
    cout << "*   *" << endl;    
    cout << "*   *" << endl;
    cout << "*****" << endl;

    //3
    std::cout << "Hello, without using namespace std!" << std::endl;

    //4
    cout << "Integer: " << 42 << ", Float: " << 3.14 << ", Char: " << 'A' << ", String: " << "Hello" << endl;

    //5
    cout << "Using endl:" << endl;
    cout << "Line 1" << endl;
    cout << "Line 2" << "\n";

    return 0;
}