// ============================================
// 函数返回值 - 超详细注释版
// 日期：2026-03-09（第3天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】函数返回值是什么？

是什么？
- 返回值是函数执行后返回给调用者的结果
- 通过return语句返回

为什么需要？
- 让函数可以计算并返回结果
- 让函数的结果可以被其他代码使用
- 提高函数的实用性

核心特点？
- 返回类型：函数定义时指定返回什么类型的数据
- return语句：返回具体的值
- void：表示不返回任何值
*/

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

#include <iostream>
#include <string>
using namespace std;

void conceptExplanation() {
    cout << string(60, '=') << endl;
    cout << "第一部分：核心概念详解 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 1.1 什么是返回值？
    // --------------------------------------------

    cout << "\n1.1 什么是返回值？" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【返回值的本质】

想象你去ATM取钱：
- 你输入密码和金额（参数）
- ATM处理你的请求（函数体）
- ATM吐出钞票（返回值）

函数返回值就是这样：
- 函数接收输入（参数）
- 函数进行处理（函数体）
- 函数返回结果（返回值）

【有返回值 vs 无返回值】

// 有返回值：计算并返回结果
int add(int a, int b) {
    return a + b;  // 返回计算结果
}

// 无返回值：只执行操作，不返回结果
void printHello() {
    cout << "Hello!" << endl;  // 只打印，不返回
}

使用方式：

int result = add(3, 5);  // 接收返回值
cout << result << endl;   // 输出8

printHello();  // 直接调用，没有返回值可接收
)" << endl;

    // --------------------------------------------
    // 1.2 return语句的作用
    // --------------------------------------------

    cout << "\n1.2 return语句的作用" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【return的两个作用】

1. 返回值
   - 将计算结果返回给调用者
   - return后面跟要返回的值

2. 结束函数
   - 执行return后，函数立即结束
   - return后面的代码不会执行

【示例】

int calculate(int x) {
    if (x < 0) {
        return -1;  // 提前返回
    }

    int result = x * 2;
    return result;  // 正常返回

    cout << "这行不会执行" << endl;  // return后的代码不执行
}

【return的位置】

// 可以有多个return
int getSign(int num) {
    if (num > 0) {
        return 1;   // 正数返回1
    } else if (num < 0) {
        return -1;  // 负数返回-1
    } else {
        return 0;   // 零返回0
    }
}

// 但每次只会执行一个return
int x = getSign(5);   // 执行第一个return，返回1
int y = getSign(-3);  // 执行第二个return，返回-1
int z = getSign(0);   // 执行第三个return，返回0
)" << endl;

    // --------------------------------------------
    // 1.3 返回类型详解
    // --------------------------------------------

    cout << "\n1.3 返回类型详解" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【常见返回类型】

1. int - 返回整数
   int getAge() {
       return 20;
   }

2. double - 返回小数
   double getPrice() {
       return 99.99;
   }

3. string - 返回字符串
   string getName() {
       return "张三";
   }

4. bool - 返回布尔值
   bool isAdult(int age) {
       return age >= 18;
   }

5. void - 不返回任何值
   void printInfo() {
       cout << "信息" << endl;
       // 可以写 return; 或不写return
   }

【返回类型必须匹配】

int add(int a, int b) {
    return a + b;  // ✅ 返回int，匹配
}

int wrong() {
    return "Hello";  // ❌ 返回string，不匹配int
}

double calculate() {
    return 10;  // ✅ 返回int，自动转换为double
}
)" << endl;

    // --------------------------------------------
    // 1.4 void函数详解
    // --------------------------------------------

    cout << "\n1.4 void函数详解" << endl;
    cout << string(60, '-') << endl;

    cout << R"(
【void的含义】

void = 空的、无效的
void函数 = 不返回任何值的函数

【void函数的特点】

1. 不需要return语句
   void sayHello() {
       cout << "Hello!" << endl;
       // 函数结束时自动返回
   }

2. 可以使用空return提前结束
   void printPositive(int num) {
       if (num <= 0) {
           return;  // 提前结束，不执行后面的代码
       }
       cout << num << endl;
   }

3. 不能接收返回值
   void test() {
       cout << "Test" << endl;
   }

   int x = test();  // ❌ 错误：void函数没有返回值

【什么时候用void？】

- 只需要执行操作，不需要返回结果
- 例如：打印信息、修改全局变量、文件操作等

示例：
void printMenu() {
    cout << "1. 开始游戏" << endl;
    cout << "2. 退出游戏" << endl;
}
)" << endl;
}

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 返回整数
// --------------------------------------------

// 函数定义：计算两个数的和
// 返回类型：int（整数）
// 参数：int a, int b
// 返回值：a + b的结果
int add(int a, int b) {
    int sum = a + b;
    return sum;  // 返回计算结果
}

// 函数定义：获取绝对值
// 返回类型：int
// 参数：int num
// 返回值：num的绝对值
int getAbsolute(int num) {
    if (num < 0) {
        return -num;  // 负数返回相反数
    } else {
        return num;   // 非负数直接返回
    }
}

// --------------------------------------------
// 2.2 返回小数
// --------------------------------------------

// 函数定义：计算平均值
// 返回类型：double（小数）
// 参数：int a, int b, int c
// 返回值：三个数的平均值
double getAverage(int a, int b, int c) {
    // 注意：要转换为double才能得到小数结果
    double sum = a + b + c;
    return sum / 3.0;
}

// 函数定义：计算圆的面积
// 返回类型：double
// 参数：double radius - 半径
// 返回值：圆的面积
double getCircleArea(double radius) {
    const double PI = 3.14159;
    return PI * radius * radius;
}

// --------------------------------------------
// 2.3 返回字符串
// --------------------------------------------

// 函数定义：获取问候语
// 返回类型：string（字符串）
// 参数：string name
// 返回值：包含名字的问候语
string getGreeting(string name) {
    return "你好，" + name + "！";
}

// 函数定义：根据分数获取等级
// 返回类型：string
// 参数：int score
// 返回值：等级字符串
string getGrade(int score) {
    if (score >= 90) {
        return "优秀";
    } else if (score >= 80) {
        return "良好";
    } else if (score >= 60) {
        return "及格";
    } else {
        return "不及格";
    }
}

// --------------------------------------------
// 2.4 返回布尔值
// --------------------------------------------

// 函数定义：判断是否为偶数
// 返回类型：bool（布尔值）
// 参数：int num
// 返回值：true表示偶数，false表示奇数
bool isEven(int num) {
    return num % 2 == 0;
}

// 函数定义：判断是否在范围内
// 返回类型：bool
// 参数：int num - 要判断的数
//       int min - 最小值
//       int max - 最大值
// 返回值：true表示在范围内，false表示不在
bool isInRange(int num, int min, int max) {
    return num >= min && num <= max;
}

// --------------------------------------------
// 2.5 void函数
// --------------------------------------------

// 函数定义：打印分隔线
// 返回类型：void（无返回值）
// 参数：无
void printSeparator() {
    cout << string(60, '-') << endl;
}

// 函数定义：打印数字（只打印正数）
// 返回类型：void
// 参数：int num
void printPositiveNumber(int num) {
    if (num <= 0) {
        cout << "错误：数字必须为正数" << endl;
        return;  // 提前结束函数
    }

    cout << "数字：" << num << endl;
}

void testFunctions() {
    cout << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // --------------------------------------------
    // 测试1：返回整数
    // --------------------------------------------

    cout << "\n2.1 返回整数的函数" << endl;
    printSeparator();

    // 接收返回值
    int sum = add(10, 20);
    cout << "10 + 20 = " << sum << endl;

    // 直接使用返回值
    cout << "5 + 8 = " << add(5, 8) << endl;

    // 在表达式中使用
    int total = add(3, 4) + add(5, 6);
    cout << "3+4 + 5+6 = " << total << endl;

    cout << endl;

    // 测试绝对值函数
    cout << "|-5| = " << getAbsolute(-5) << endl;
    cout << "|10| = " << getAbsolute(10) << endl;

    cout << "\n说明：" << endl;
    cout << "- 返回值可以赋值给变量" << endl;
    cout << "- 返回值可以直接打印" << endl;
    cout << "- 返回值可以用在表达式中" << endl;

    // --------------------------------------------
    // 测试2：返回小数
    // --------------------------------------------

    cout << "\n2.2 返回小数的函数" << endl;
    printSeparator();

    // 计算平均值
    double avg = getAverage(80, 90, 85);
    cout << "80, 90, 85的平均值：" << avg << endl;

    // 计算圆的面积
    double area = getCircleArea(5.0);
    cout << "半径为5的圆的面积：" << area << endl;

    cout << "\n说明：" << endl;
    cout << "- double类型可以存储小数" << endl;
    cout << "- 计算时注意类型转换" << endl;

    // --------------------------------------------
    // 测试3：返回字符串
    // --------------------------------------------

    cout << "\n2.3 返回字符串的函数" << endl;
    printSeparator();

    // 获取问候语
    string greeting = getGreeting("小明");
    cout << greeting << endl;

    // 获取等级
    cout << "95分的等级：" << getGrade(95) << endl;
    cout << "75分的等级：" << getGrade(75) << endl;
    cout << "50分的等级：" << getGrade(50) << endl;

    cout << "\n说明：" << endl;
    cout << "- string类型可以返回文本" << endl;
    cout << "- 可以在函数内拼接字符串后返回" << endl;

    // --------------------------------------------
    // 测试4：返回布尔值
    // --------------------------------------------

    cout << "\n2.4 返回布尔值的函数" << endl;
    printSeparator();

    // 判断奇偶
    cout << "10是偶数吗？" << (isEven(10) ? "是" : "否") << endl;
    cout << "7是偶数吗？" << (isEven(7) ? "是" : "否") << endl;

    // 判断范围
    cout << "15在10-20之间吗？" << (isInRange(15, 10, 20) ? "是" : "否") << endl;
    cout << "25在10-20之间吗？" << (isInRange(25, 10, 20) ? "是" : "否") << endl;

    // 在if语句中使用
    if (isEven(8)) {
        cout << "8是偶数" << endl;
    }

    cout << "\n说明：" << endl;
    cout << "- bool类型返回true或false" << endl;
    cout << "- 常用于条件判断" << endl;
    cout << "- 可以用三元运算符转换为文本" << endl;

    // --------------------------------------------
    // 测试5：void函数
    // --------------------------------------------

    cout << "\n2.5 void函数" << endl;
    printSeparator();

    // 调用void函数
    printSeparator();
    cout << "这是一条分隔线" << endl;
    printSeparator();

    cout << endl;

    // 测试提前返回
    printPositiveNumber(10);   // 正常打印
    printPositiveNumber(-5);   // 提前返回，打印错误信息

    cout << "\n说明：" << endl;
    cout << "- void函数不返回值" << endl;
    cout << "- 可以使用return提前结束" << endl;
    cout << "- 适合只执行操作的场景" << endl;
}

/*
==============================================
🔍 深入理解：返回值的内存机制（选学）
==============================================

如果你想知道"返回值在内存中是如何传递的"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【返回值的传递过程】

int add(int a, int b) {
    int sum = a + b;
    return sum;
}

int main() {
    int result = add(3, 5);
    return 0;
}

内存过程：

1. 调用add(3, 5)
   - 在栈上为add函数分配空间
   - 创建形参a=3, b=5
   - 创建局部变量sum

2. 计算sum = 3 + 5 = 8
   - sum存储在add函数的栈帧中

3. 执行return sum
   - 将sum的值（8）复制到一个临时位置
   - 这个临时位置叫"返回值寄存器"

4. 函数结束
   - 销毁add函数的栈帧
   - sum变量被销毁
   - 但返回值（8）已经保存在寄存器中

5. 赋值给result
   - 将寄存器中的值（8）复制给result
   - result存储在main函数的栈帧中

【为什么不能返回局部变量的地址？】

❌ 错误示例：

int* getPointer() {
    int x = 10;
    return &x;  // 危险！返回局部变量的地址
}

int main() {
    int* ptr = getPointer();
    cout << *ptr << endl;  // 未定义行为！
    return 0;
}

问题：
- x是局部变量，函数结束后被销毁
- 返回的地址指向已经被销毁的内存
- 访问这个地址是未定义行为

正确做法：
- 返回值本身（值传递）
- 返回动态分配的内存地址
- 使用引用返回（后续学习）

【返回值优化（RVO）】

现代编译器会优化返回值的复制：

string getName() {
    string name = "张三";
    return name;  // 看起来会复制
}

int main() {
    string result = getName();  // 实际上可能不复制
    return 0;
}

编译器优化：
- 直接在result的位置构造name
- 避免不必要的复制
- 这叫做"返回值优化"（Return Value Optimization）

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void commonMistakes() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
// --------------------------------------------
// 4.1 错误1：返回类型不匹配
// --------------------------------------------

❌ 错误代码：

int calculate() {
    return "Hello";  // 错误：返回string，但函数声明返回int
}

结果：编译错误 - cannot convert 'const char*' to 'int'

原因：return的值类型必须和函数声明的返回类型匹配

✅ 正确代码：

int calculate() {
    return 42;  // 正确：返回int
}

string getText() {
    return "Hello";  // 正确：返回string
}

记忆技巧：
- 函数声明什么类型，就return什么类型
- 编译器会检查类型匹配

// --------------------------------------------
// 4.2 错误2：非void函数没有return
// --------------------------------------------

❌ 错误代码：

int add(int a, int b) {
    int sum = a + b;
    // 忘记写return
}

结果：编译警告或错误 - control reaches end of non-void function

原因：声明返回int，但没有return语句

✅ 正确代码：

int add(int a, int b) {
    int sum = a + b;
    return sum;  // 必须有return
}

记忆技巧：
- 非void函数必须有return
- 所有执行路径都要有return

// --------------------------------------------
// 4.3 错误3：return后面还有代码
// --------------------------------------------

❌ 错误代码：

int calculate(int x) {
    return x * 2;
    cout << "这行不会执行" << endl;  // 永远不会执行
    return x * 3;  // 永远不会执行
}

结果：编译警告 - unreachable code

原因：return会立即结束函数，后面的代码不会执行

✅ 正确代码：

int calculate(int x) {
    cout << "计算中..." << endl;  // 先执行其他代码
    return x * 2;  // 最后return
}

记忆技巧：
- return是函数的终点
- return后面不要写代码

// --------------------------------------------
// 4.4 错误4：void函数返回值
// --------------------------------------------

❌ 错误代码：

void printInfo() {
    cout << "信息" << endl;
    return 100;  // 错误：void函数不能返回值
}

结果：编译错误 - return-statement with a value, in function returning 'void'

原因：void表示不返回任何值

✅ 正确代码：

void printInfo() {
    cout << "信息" << endl;
    return;  // 可以写空return，或不写
}

int getInfo() {
    return 100;  // 如果需要返回值，改为非void
}

记忆技巧：
- void = 无返回值
- 需要返回值就不要用void

// --------------------------------------------
// 4.5 错误5：忘记接收返回值
// --------------------------------------------

❌ 错误代码（不是语法错误，但逻辑错误）：

int add(int a, int b) {
    return a + b;
}

int main() {
    add(3, 5);  // 计算了，但没有使用返回值
    // 返回值被丢弃了
    return 0;
}

结果：程序运行正常，但返回值被浪费了

原因：没有接收或使用返回值

✅ 正确代码：

int main() {
    int result = add(3, 5);  // 接收返回值
    cout << result << endl;   // 使用返回值
    return 0;
}

或者：

int main() {
    cout << add(3, 5) << endl;  // 直接使用返回值
    return 0;
}

记忆技巧：
- 有返回值的函数，要么接收，要么直接使用
- 不要让返回值白白浪费

// --------------------------------------------
// 4.6 错误6：所有路径都要有return
// --------------------------------------------

❌ 错误代码：

int getSign(int num) {
    if (num > 0) {
        return 1;
    } else if (num < 0) {
        return -1;
    }
    // 如果num==0，没有return！
}

结果：编译警告 - control reaches end of non-void function

原因：num==0时，没有return语句

✅ 正确代码：

int getSign(int num) {
    if (num > 0) {
        return 1;
    } else if (num < 0) {
        return -1;
    } else {
        return 0;  // 确保所有情况都有return
    }
}

或者：

int getSign(int num) {
    if (num > 0) {
        return 1;
    } else if (num < 0) {
        return -1;
    }
    return 0;  // 默认返回
}

记忆技巧：
- 检查所有if-else分支
- 确保每个分支都有return
- 或者在最后加一个默认return

// --------------------------------------------
// 4.7 错误7：整数除法返回小数
// --------------------------------------------

❌ 错误代码：

double getAverage(int a, int b) {
    return (a + b) / 2;  // 整数除法，结果是整数
}

int main() {
    cout << getAverage(5, 6) << endl;  // 输出5，不是5.5
    return 0;
}

结果：输出5（整数），不是5.5（小数）

原因：两个int相除，结果是int，小数部分被截断

✅ 正确代码：

double getAverage(int a, int b) {
    return (a + b) / 2.0;  // 除以2.0，得到double结果
}

或者：

double getAverage(int a, int b) {
    return (double)(a + b) / 2;  // 强制转换为double
}

记忆技巧：
- 整数除法得整数
- 要得小数，至少一个操作数要是小数
- 或者强制类型转换

)" << endl;
}

// ============================================
// 第五部分：函数卡片速查
// ============================================

void functionCards() {
    cout << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【返回整数】

语法：int 函数名(参数列表) { return 整数值; }
示例：
int add(int a, int b) {
    return a + b;
}

────────────────────────────────────

【返回小数】

语法：double 函数名(参数列表) { return 小数值; }
示例：
double getAverage(int a, int b) {
    return (a + b) / 2.0;
}

────────────────────────────────────

【返回字符串】

语法：string 函数名(参数列表) { return 字符串; }
示例：
string getGreeting(string name) {
    return "Hello, " + name;
}

────────────────────────────────────

【返回布尔值】

语法：bool 函数名(参数列表) { return 布尔表达式; }
示例：
bool isEven(int num) {
    return num % 2 == 0;
}

────────────────────────────────────

【无返回值】

语法：void 函数名(参数列表) { 执行操作; }
示例：
void printInfo() {
    cout << "信息" << endl;
}

────────────────────────────────────

【提前返回】

语法：在函数中间使用return结束函数
示例：
int calculate(int x) {
    if (x < 0) {
        return -1;  // 提前返回
    }
    return x * 2;
}

)" << endl;
}

// ============================================
// 第六部分：练习题
// ============================================

void exercises() {
    cout << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 编写函数 int getMax(int a, int b)
   要求：返回两个数中的最大值
   提示：使用if-else判断，返回较大的数

2. 编写函数 bool isPositive(int num)
   要求：判断一个数是否为正数
   提示：返回 num > 0 的结果

3. 编写函数 string getDayName(int day)
   要求：根据数字1-7返回星期几（1=星期一，7=星期日）
   提示：使用if-else或switch-case

4. 编写函数 double calculateBMI(double weight, double height)
   要求：计算BMI指数（体重/身高²）
   提示：weight单位kg，height单位m

5. 编写函数 int countDigits(int num)
   要求：计算一个整数有几位数
   提示：使用循环，每次除以10，直到num为0

)" << endl;
}

// ============================================
// 主程序：运行所有示例
// ============================================

int main() {
    // 运行所有测试
    conceptExplanation();
    cout << "\n";
    testFunctions();
    cout << "\n";
    commonMistakes();
    cout << "\n";
    functionCards();
    cout << "\n";
    exercises();

    return 0;
}
