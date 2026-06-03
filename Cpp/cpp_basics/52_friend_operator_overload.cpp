// ============================================
// 文件名：52_friend_operator_overload.cpp
// 日期：第17天
// 难度：中级
// 主题：友元函数运算符重载 + 流输出运算符
// ============================================

/*
【核心概念】

是什么？
- 将运算符重载函数声明为类的友元（friend），使其可以访问私有成员

为什么需要？
- 当运算符的左操作数不是本类对象时（如 cout << obj），成员函数无法胜任
- 友元非成员函数提供更对称的运算符语义（两侧操作数地位平等）

核心特点：
- friend 声明在类内，定义在类外
- 不属于类的成员函数，不通过对象调用
- 可访问类的所有私有成员
*/

#include <iostream>
#include <sstream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解
// ============================================

/*
【友元函数 vs 成员函数重载的区别】

成员函数重载：
  调用形式：a + b  -->  a.operator+(b)
  左操作数必须是本类对象，编译器将其绑定为 this
  适合：+=、-=、[]、() 等修改自身的运算符

友元非成员函数重载：
  调用形式：a + b  -->  operator+(a, b)
  两侧操作数地位平等，均作为显式参数传入
  适合：+、-、*、/、==、<<、>> 等对称运算符

【何时必须用友元（左操作数不是本类时）】

  cout << obj   -->   operator<<(cout, obj)
  左操作数是 ostream，不是本类，无法写成成员函数
  必须使用友元非成员函数

【函数卡片：operator<<】

  语法：ostream& operator<<(ostream& os, const ClassName& obj)
  参数1 os（ostream&）：输出流对象，不能加 const（需修改流状态）
  参数2 obj（const ClassName&）：被输出的对象，const 引用避免拷贝
  返回：ostream&，支持链式调用 cout << a << b

【函数卡片：operator>>】

  语法：istream& operator>>(istream& is, ClassName& obj)
  参数1 is（istream&）：输入流对象
  参数2 obj（ClassName&）：被赋值的对象，非 const（需要修改）
  返回：istream&，支持链式调用 cin >> a >> b

【函数卡片：非成员 operator+】

  语法：ClassName operator+(const ClassName& lhs, const ClassName& rhs)
  参数1 lhs：左操作数（left-hand side）
  参数2 rhs：右操作数（right-hand side）
  返回：新对象（值类型，非引用）
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

// --------------------------------------------
// 2.1 Fraction（分数）类 —— 友元运算符重载
// --------------------------------------------

class Fraction {
private:
    int numerator;   // 分子
    int denominator; // 分母

    // 辗转相除法求最大公约数（用于化简）
    int gcd(int a, int b) const {
        a = (a < 0) ? -a : a;
        b = (b < 0) ? -b : b;
        while (b != 0) { int t = b; b = a % b; a = t; }
        return a;
    }

    // 化简分数：保证分母为正，约分
    void reduce() {
        if (denominator < 0) { numerator = -numerator; denominator = -denominator; }
        int g = gcd(numerator, denominator);
        numerator /= g;
        denominator /= g;
    }

public:
    // 构造函数：默认值 0/1，构造时自动化简
    Fraction(int num = 0, int den = 1) : numerator(num), denominator(den) {
        reduce();
    }

    // 友元声明（放在 public 区，表示公开接口）
    // 声明为友元后，这些非成员函数可以访问 private 成员
    friend ostream& operator<<(ostream& os, const Fraction& f);
    friend istream& operator>>(istream& is, Fraction& f);
    friend Fraction operator+(const Fraction& lhs, const Fraction& rhs);
    friend bool     operator==(const Fraction& lhs, const Fraction& rhs);
};

// 友元函数定义（在类外，定义时不再写 friend 关键字）

// operator<<：输出 "分子/分母" 格式，如 "3/4"
// 参数 os：输出流（不加 const，需修改流内部状态）
// 参数 f：被输出的分数（const 引用，只读，不修改对象）
// 返回 ostream&：必须返回 os 自身，支持 cout << a << b 链式调用
ostream& operator<<(ostream& os, const Fraction& f) {
    if (f.denominator == 1) {
        os << f.numerator;                          // 整数直接输出，如 "3"
    } else {
        os << f.numerator << "/" << f.denominator;  // 分数形式，如 "3/4"
    }
    return os; // 返回流引用，支持链式：cout << f1 << " " << f2
}

// operator>>：从流中读取 "分子/分母" 格式（如 "3/4"）
// 参数 f：目标分数对象（非 const，需要修改它的值）
// 返回 istream&：支持 cin >> a >> b 链式输入
istream& operator>>(istream& is, Fraction& f) {
    char slash;                            // 读取中间的 '/' 字符
    is >> f.numerator >> slash >> f.denominator;
    f.reduce();                            // 读入后自动化简
    return is;
}

// operator+：分数加法（通分相加）
// 公式：a/b + c/d = (a*d + c*b) / (b*d)
// 返回新对象（值类型），构造函数会自动化简
Fraction operator+(const Fraction& lhs, const Fraction& rhs) {
    int newNum = lhs.numerator   * rhs.denominator   // 通分后的分子
               + rhs.numerator   * lhs.denominator;
    int newDen = lhs.denominator * rhs.denominator;  // 通分后的分母
    return Fraction(newNum, newDen);                  // 构造时自动化简
}

// operator==：化简后直接比较分子和分母即可
bool operator==(const Fraction& lhs, const Fraction& rhs) {
    return lhs.numerator == rhs.numerator &&
           lhs.denominator == rhs.denominator;
}

// --------------------------------------------
// 2.2 Complex（复数）类 —— 友元运算符重载
// --------------------------------------------

class Complex {
private:
    double real; // 实部
    double imag; // 虚部

public:
    // 构造函数：实部和虚部默认为 0
    Complex(double r = 0.0, double i = 0.0) : real(r), imag(i) {}

    // 友元声明
    friend ostream& operator<<(ostream& os, const Complex& c);
    friend istream& operator>>(istream& is, Complex& c);
    friend Complex  operator+(const Complex& lhs, const Complex& rhs);
    friend Complex  operator-(const Complex& lhs, const Complex& rhs);
    friend bool     operator==(const Complex& lhs, const Complex& rhs);
};

// operator<<：输出 "a+bi" 或 "a-bi" 格式
// 参数 os：输出流（不加 const，需修改流内部状态）
// 参数 c：被输出的复数（const 引用，只读）
// 返回 ostream&：支持链式调用
ostream& operator<<(ostream& os, const Complex& c) {
    os << c.real;                       // 先输出实部
    if (c.imag >= 0) {
        os << "+" << c.imag << "i";     // 虚部非负：加号 + 虚部 + i
    } else {
        os << c.imag << "i";            // 虚部为负：imag 本身带负号，直接输出
    }
    return os; // 返回流引用，支持链式调用
}

// operator>>：从流中读取实部和虚部（空格分隔）
// 参数 c：目标复数对象（非 const，需要写入）
istream& operator>>(istream& is, Complex& c) {
    is >> c.real >> c.imag; // 读取两个浮点数，空格分隔
    return is;
}

// operator+：实部加实部，虚部加虚部
// 返回新的 Complex 对象（值类型，不是引用）
Complex operator+(const Complex& lhs, const Complex& rhs) {
    return Complex(lhs.real + rhs.real,  // 实部相加
                   lhs.imag + rhs.imag); // 虚部相加
}

// operator-：实部减实部，虚部减虚部
Complex operator-(const Complex& lhs, const Complex& rhs) {
    return Complex(lhs.real - rhs.real,  // 实部相减
                   lhs.imag - rhs.imag); // 虚部相减
}

// operator==：实部和虚部都相等才相等
bool operator==(const Complex& lhs, const Complex& rhs) {
    return lhs.real == rhs.real && lhs.imag == rhs.imag;
}

// ============================================
// 主函数：演示所有友元运算符
// ============================================

int main() {
    cout << string(60, '=') << endl;
    cout << "第二部分：基础操作实践" << endl;
    cout << string(60, '=') << endl;

    // ----------------------------------------
    // 2.1 Fraction 演示
    // ----------------------------------------
    cout << "\n2.1 Fraction（分数）友元运算符重载" << endl;
    cout << string(60, '-') << endl;

    Fraction f1(1, 2); // 1/2
    Fraction f2(1, 3); // 1/3

    // 友元 operator<<：调用形式为 operator<<(cout, f1)
    // 不是成员函数调用，左操作数 cout 作为第一个显式参数传入
    cout << "f1 = " << f1 << endl;   // 输出：1/2
    cout << "f2 = " << f2 << endl;   // 输出：1/3

    // 友元 operator+：1/2 + 1/3 = 3/6 + 2/6 = 5/6
    Fraction f3 = f1 + f2;
    cout << "f1 + f2 = " << f3 << endl;

    // 链式输出：每次 << 返回 ostream&，供下一个 << 使用
    cout << f1 << " + " << f2 << " = " << f3 << endl;

    // 友元 operator==：2/4 化简后是 1/2，与 f1 相等
    Fraction f4(2, 4);
    cout << "f1(1/2) == f4(2/4): " << (f1 == f4 ? "true" : "false") << endl;

    // 友元 operator>>：从字符串流读入（模拟 cin 输入 "3/4"）
    Fraction f5;
    istringstream iss("3/4");
    iss >> f5;
    cout << "从流读入 f5 = " << f5 << endl; // 输出：3/4

    // ----------------------------------------
    // 2.2 Complex 演示
    // ----------------------------------------
    cout << "\n2.2 Complex（复数）友元运算符重载" << endl;
    cout << string(60, '-') << endl;

    Complex c1(3.0,  4.0); // 3+4i
    Complex c2(1.0, -2.0); // 1-2i

    cout << "c1 = " << c1 << endl;   // 输出：3+4i
    cout << "c2 = " << c2 << endl;   // 输出：1-2i

    // operator+：(3+1) + (4+(-2))i = 4+2i
    Complex c3 = c1 + c2;
    cout << "c1 + c2 = " << c3 << endl;

    // operator-：(3-1) + (4-(-2))i = 2+6i
    Complex c4 = c1 - c2;
    cout << "c1 - c2 = " << c4 << endl;

    // 链式输出
    cout << c1 << " + " << c2 << " = " << c3 << endl;

    // operator==
    Complex c5(3.0, 4.0);
    cout << "c1 == c5: " << (c1 == c5 ? "true" : "false") << endl;

    // operator>>：从字符串流读入（实部和虚部空格分隔）
    Complex c6;
    istringstream iss2("2.5 -1.5");
    iss2 >> c6;
    cout << "从流读入 c6 = " << c6 << endl; // 输出：2.5-1.5i

    // ============================================
    // 第三部分：深入理解（选学，可跳过，不影响后续学习）
    // ============================================
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：深入理解（选学）" << endl;
    cout << string(60, '=') << endl;

    cout << "\n"
            "[深入理解] 为什么 operator<< 不能是成员函数？\n\n"
            "  cout << c1  等价于  operator<<(cout, c1)\n"
            "  左操作数是 cout（类型 ostream），不是 Complex 对象。\n\n"
            "  若写成成员函数：ostream& Complex::operator<<(ostream& os)\n"
            "  调用方式变成：c1.operator<<(cout)  即  c1 << cout  （顺序颠倒！）\n\n"
            "  结论：凡是左操作数不是本类，必须用友元非成员函数。\n\n"
            "[深入理解] 为什么返回 ostream& 而不是 void？\n\n"
            "  cout << c1 << endl 拆解：\n"
            "    第1步：operator<<(cout, c1)  必须返回 cout（即 ostream&）\n"
            "    第2步：(返回的 ostream&) << endl  继续链式调用\n"
            "  如果返回 void，第2步的 << 找不到左操作数，编译报错。\n"
            "  规则：operator<< 和 operator>> 必须返回对应的流引用。\n"
         << endl;

    // ============================================
    // 第四部分：常见错误和陷阱
    // ============================================
    cout << string(60, '=') << endl;
    cout << "第四部分：常见错误和陷阱" << endl;
    cout << string(60, '=') << endl;

    cout
        << "\n" << string(60, '-') << "\n"
           "错误1：operator<< 写成成员函数\n"
        << string(60, '-') << "\n"
           "X 错误：\n"
           "  class Complex {\n"
           "      ostream& operator<<(ostream& os) { os << real; return os; }\n"
           "  };\n"
           "  // 使用变成：c1 << cout  （顺序颠倒！）\n\n"
           "V 正确：\n"
           "  friend ostream& operator<<(ostream& os, const Complex& c);\n"
           "  // 使用：cout << c1  V\n\n"
           "原因：cout << c1 的左操作数是 ostream，不是 Complex\n"
           "预防：左操作数不是本类时，必须用友元非成员函数\n"

        << "\n" << string(60, '-') << "\n"
           "错误2：忘记返回 ostream&，链式调用失效\n"
        << string(60, '-') << "\n"
           "X 错误：\n"
           "  void operator<<(ostream& os, const Complex& c) { os << c.real; }\n"
           "  // cout << c1 << endl;  // 报错！void 不能继续链接\n\n"
           "V 正确：\n"
           "  ostream& operator<<(ostream& os, const Complex& c) {\n"
           "      os << c.real << \"+\" << c.imag << \"i\";\n"
           "      return os;  // 必须返回 os\n"
           "  }\n\n"
           "预防：operator<< 和 operator>> 的返回类型始终是流引用\n"

        << "\n" << string(60, '-') << "\n"
           "错误3：friend 声明放在 private 区（概念混淆）\n"
        << string(60, '-') << "\n"
           "X 不推荐（能编译，但令人困惑）：\n"
           "  class Complex { private: friend ostream& operator<<(...); };\n\n"
           "V 推荐：\n"
           "  class Complex { public: friend ostream& operator<<(...); };\n\n"
           "原因：friend 不受访问控制影响，但惯例放 public 区提升可读性\n"

        << "\n" << string(60, '-') << "\n"
           "错误4：operator<< 参数顺序写反\n"
        << string(60, '-') << "\n"
           "X 错误：\n"
           "  ostream& operator<<(const Complex& c, ostream& os) { ... }\n"
           "  // cout << c1 -> operator<<(cout, c1) -> 参数不匹配！\n\n"
           "V 正确：\n"
           "  ostream& operator<<(ostream& os, const Complex& c) { ... }\n\n"
           "预防：记住「流在前，对象在后」，与书写 cout << obj 顺序一致\n"

        << "\n" << string(60, '-') << "\n"
           "错误5：给 ostream 参数加 const（编译报错）\n"
        << string(60, '-') << "\n"
           "X 错误：\n"
           "  ostream& operator<<(const ostream& os, const Complex& c) {\n"
           "      os << c.real;  // 报错！const ostream 不能调用非 const 的 <<\n"
           "      return os;     // 报错！const ref 不能转为非 const ref\n"
           "  }\n\n"
           "V 正确：\n"
           "  ostream& operator<<(ostream& os, const Complex& c) { ... }\n\n"
           "原因：ostream 的内部 << 是非 const 函数（会修改流状态）\n"
           "预防：ostream& 和 istream& 参数永远不加 const\n"

        << "\n" << string(60, '-') << "\n"
           "错误6：非成员 operator+ 忘记声明 friend\n"
        << string(60, '-') << "\n"
           "X 错误（类内无 friend 声明）：\n"
           "  Complex operator+(const Complex& l, const Complex& r) {\n"
           "      return Complex(l.real + r.real, ...); // 报错！real 是 private\n"
           "  }\n\n"
           "V 正确：\n"
           "  // 类内声明：\n"
           "  friend Complex operator+(const Complex& l, const Complex& r);\n"
           "  // 类外定义：\n"
           "  Complex operator+(const Complex& l, const Complex& r) {\n"
           "      return Complex(l.real + r.real, l.imag + r.imag);\n"
           "  }\n\n"
           "预防：需要访问 private 成员的非成员函数都必须在类内声明 friend\n"
        << endl;

    // ============================================
    // 第五部分：函数卡片速查
    // ============================================
    cout << string(60, '=') << endl;
    cout << "第五部分：函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << "\n"
            "【operator<<（流输出）】\n"
            "  语法：ostream& operator<<(ostream& os, const T& obj)\n"
            "  os  (ostream&)  ：输出流，不加 const\n"
            "  obj (const T&)  ：被输出对象\n"
            "  返回：ostream&（返回 os 本身，支持链式）\n"
            "  示例：cout << f1 << \" \" << f2;\n"
            "\n"
            "【operator>>（流输入）】\n"
            "  语法：istream& operator>>(istream& is, T& obj)\n"
            "  is  (istream&)  ：输入流，不加 const\n"
            "  obj (T&)        ：目标对象，非 const\n"
            "  返回：istream&（返回 is 本身，支持链式）\n"
            "  示例：cin >> f1 >> f2;\n"
            "\n"
            "【非成员 operator+】\n"
            "  语法：T operator+(const T& lhs, const T& rhs)\n"
            "  lhs (const T&)  ：左操作数\n"
            "  rhs (const T&)  ：右操作数\n"
            "  返回：新对象（值类型，不是引用）\n"
            "  示例：Fraction f3 = f1 + f2;\n"
            "\n"
            "【成员 vs 友元非成员的选择原则】\n"
            "  修改自身（+=、-=、[]、=）   -> 成员函数（必须）\n"
            "  左操作数是流（<<、>>）       -> 友元非成员（必须）\n"
            "  对称运算（+、-、==、!=）      -> 友元非成员（推荐）\n"
         << endl;

    // ============================================
    // 第六部分：练习题（不提供答案）
    // ============================================
    cout << string(60, '=') << endl;
    cout << "第六部分：练习题（独立完成，不提供答案）" << endl;
    cout << string(60, '=') << endl;

    cout << "\n"
            "1. 为 Fraction 类添加 operator- 友元非成员函数（分数减法）。\n"
            "   要求：结果自动化简，测试 1/2 - 1/3 = 1/6\n"
            "   提示：公式 a/b - c/d = (a*d - c*b) / (b*d)\n"
            "\n"
            "2. 为 Complex 类添加 operator* 友元非成员函数（复数乘法）。\n"
            "   要求：测试 (1+2i) * (3+4i) = -5+10i\n"
            "   提示：公式 (a+bi)(c+di) = (ac-bd) + (ad+bc)i\n"
            "\n"
            "3. 创建 Vector2D 类（二维向量，含 double x 和 double y）。\n"
            "   实现友元 operator<<（格式\"(x, y)\"）、operator>>、operator+、operator==\n"
            "   提示：友元声明和定义分离，注意输出格式统一\n"
            "\n"
            "4. 思考题：为什么 operator= 和 operator[] 不能用友元非成员函数重载？\n"
            "   提示：查阅 C++ 标准中规定只能作为成员函数的运算符有哪些\n"
         << endl;

    cout << string(60, '=') << endl;
    cout << "编译命令：g++ -std=c++11 -o 52 52_friend_operator_overload.cpp" << endl;
    cout << string(60, '=') << endl;

    return 0;
}
