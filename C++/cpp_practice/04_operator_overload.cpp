// ============================================
// 04_operator_overload.cpp - 运算符重载练习
// 日期：2026-06-17（第4天）
// 内容：友元、运算符重载（+、+=、++、[]、<<、>>）
// ============================================

#include <iostream>
#include <cstring>
#include <cmath>

using std::cout;
using std::endl;
using std::ostream;
using std::istream;
using std::cin;

// ============================================
// 练习1：复数类（Complex）- 算术运算符
// ============================================

class Complex {
private:
    double _real;   // 实部
    double _imag;   // 虚部

public:
    // 构造函数
    Complex(double real = 0, double imag = 0)
    : _real(real), _imag(imag) {}

    // 友元函数重载 + 运算符（不修改操作数）
    friend Complex operator+(const Complex& lhs, const Complex& rhs) {
        return Complex(lhs._real + rhs._real, lhs._imag + rhs._imag);
    }

    // 友元函数重载 - 运算符
    friend Complex operator-(const Complex& lhs, const Complex& rhs) {
        return Complex(lhs._real - rhs._real, lhs._imag - rhs._imag);
    }

    // 友元函数重载 * 运算符
    // (a+bi) * (c+di) = (ac-bd) + (ad+bc)i
    friend Complex operator*(const Complex& lhs, const Complex& rhs) {
        return Complex(
            lhs._real * rhs._real - lhs._imag * rhs._imag,
            lhs._real * rhs._imag + lhs._imag * rhs._real
        );
    }

    // 成员函数重载 += 运算符（修改左操作数）
    Complex& operator+=(const Complex& rhs) {
        _real += rhs._real;
        _imag += rhs._imag;
        return *this;  // 返回自身引用
    }

    // 友元函数重载 == 运算符
    friend bool operator==(const Complex& lhs, const Complex& rhs) {
        return (lhs._real == rhs._real) && (lhs._imag == rhs._imag);
    }

    // 友元函数重载 != 运算符
    friend bool operator!=(const Complex& lhs, const Complex& rhs) {
        return !(lhs == rhs);
    }

    // 友元函数重载 << 运算符（输出流）⭐⭐⭐⭐⭐
    friend ostream& operator<<(ostream& os, const Complex& c) {
        os << c._real;
        if (c._imag >= 0) {
            os << "+" << c._imag << "i";
        } else {
            os << c._imag << "i";
        }
        return os;  // 返回流引用，支持链式调用
    }

    // 模长
    double modulus() const {
        return sqrt(_real * _real + _imag * _imag);
    }
};

void test_complex() {
    cout << "\n=== 练习1：复数类（Complex）===" << endl;

    Complex c1(3, 4);
    Complex c2(1, 2);

    cout << "c1 = " << c1 << endl;
    cout << "c2 = " << c2 << endl;

    // 测试算术运算符
    Complex c3 = c1 + c2;
    cout << "c1 + c2 = " << c3 << endl;

    Complex c4 = c1 - c2;
    cout << "c1 - c2 = " << c4 << endl;

    Complex c5 = c1 * c2;
    cout << "c1 * c2 = " << c5 << endl;

    // 测试 += 运算符
    c1 += c2;
    cout << "c1 += c2 后，c1 = " << c1 << endl;

    // 测试关系运算符
    Complex c6(4, 6);
    cout << "c1 == c6: " << (c1 == c6) << endl;
    cout << "c1 != c2: " << (c1 != c2) << endl;

    // 链式调用
    cout << "链式输出: c1 = " << c1 << ", c2 = " << c2 << endl;
}

// ============================================
// 练习2：计数器类（Counter）- 自增自减
// ============================================

class Counter {
private:
    int _count;

public:
    Counter(int count = 0) : _count(count) {}

    // 前置++：先自增，返回自身引用
    Counter& operator++() {
        ++_count;
        return *this;
    }

    // 后置++：先返回副本，再自增（int 是哑元参数）
    Counter operator++(int) {
        Counter tmp(*this);  // 保存当前值
        ++_count;            // 自增
        return tmp;          // 返回旧值
    }

    // 前置--
    Counter& operator--() {
        --_count;
        return *this;
    }

    // 后置--
    Counter operator--(int) {
        Counter tmp(*this);
        --_count;
        return tmp;
    }

    // 输出
    friend ostream& operator<<(ostream& os, const Counter& c) {
        os << "Counter: " << c._count;
        return os;
    }

    int getCount() const { return _count; }
};

void test_counter() {
    cout << "\n=== 练习2：计数器类（Counter）===" << endl;

    Counter c1(5);
    cout << "初始值: " << c1 << endl;

    // 测试前置++
    cout << "前置++: " << ++c1 << endl;  // 先自增，输出6
    cout << "结果: " << c1 << endl;      // 6

    // 测试后置++
    Counter c2(5);
    cout << "\n初始值: " << c2 << endl;
    cout << "后置++: " << c2++ << endl;  // 先输出5，再自增
    cout << "结果: " << c2 << endl;      // 6

    // 对比前置和后置
    cout << "\n对比前置和后置：" << endl;
    Counter c3(10);
    Counter c4 = ++c3;  // 前置：c3先自增，c4得到自增后的值
    cout << "前置++后，c3 = " << c3 << ", c4 = " << c4 << endl;

    Counter c5(10);
    Counter c6 = c5++;  // 后置：c5先复制，c6得到复制的旧值，c5再自增
    cout << "后置++后，c5 = " << c5 << ", c6 = " << c6 << endl;
}

// ============================================
// 练习3：字符串类（MyString）- 赋值和下标
// ============================================

class MyString {
private:
    char* _data;
    size_t _size;

public:
    // 构造函数
    MyString(const char* str = "") {
        _size = strlen(str);
        _data = new char[_size + 1];
        strcpy(_data, str);
    }

    // 析构函数
    ~MyString() {
        if (_data) {
            delete[] _data;
            _data = nullptr;
        }
    }

    // 拷贝构造函数
    MyString(const MyString& other) {
        _size = other._size;
        _data = new char[_size + 1];
        strcpy(_data, other._data);
    }

    // 赋值运算符重载（成员函数）⭐⭐⭐⭐⭐
    MyString& operator=(const MyString& rhs) {
        if (this != &rhs) {  // 防止自赋值
            // 释放旧资源
            delete[] _data;

            // 分配新资源
            _size = rhs._size;
            _data = new char[_size + 1];
            strcpy(_data, rhs._data);
        }
        return *this;
    }

    // += 运算符重载
    MyString& operator+=(const MyString& rhs) {
        size_t newSize = _size + rhs._size;
        char* newData = new char[newSize + 1];

        strcpy(newData, _data);
        strcat(newData, rhs._data);

        delete[] _data;
        _data = newData;
        _size = newSize;

        return *this;
    }

    // [] 运算符重载（成员函数）
    char& operator[](size_t idx) {
        if (idx < _size) {
            return _data[idx];
        } else {
            cout << "下标越界！" << endl;
            static char nullchar = '\0';
            return nullchar;
        }
    }

    // const 版本的 []
    const char& operator[](size_t idx) const {
        if (idx < _size) {
            return _data[idx];
        } else {
            static char nullchar = '\0';
            return nullchar;
        }
    }

    // == 运算符重载
    friend bool operator==(const MyString& lhs, const MyString& rhs) {
        return strcmp(lhs._data, rhs._data) == 0;
    }

    // != 运算符重载
    friend bool operator!=(const MyString& lhs, const MyString& rhs) {
        return !(lhs == rhs);
    }

    // << 运算符重载
    friend ostream& operator<<(ostream& os, const MyString& str) {
        os << str._data;
        return os;
    }

    const char* c_str() const { return _data; }
    size_t size() const { return _size; }
};

void test_mystring() {
    cout << "\n=== 练习3：字符串类（MyString）===" << endl;

    MyString s1("Hello");
    MyString s2("World");

    cout << "s1 = " << s1 << endl;
    cout << "s2 = " << s2 << endl;

    // 测试赋值运算符
    MyString s3;
    s3 = s1;
    cout << "s3 = s1 后，s3 = " << s3 << endl;

    // 测试 += 运算符
    s1 += MyString(" ");
    s1 += s2;
    cout << "s1 += \" \" + s2 后，s1 = " << s1 << endl;

    // 测试 [] 运算符
    cout << "\n测试下标访问：" << endl;
    cout << "s1[0] = " << s1[0] << endl;
    cout << "s1[6] = " << s1[6] << endl;

    // 修改字符
    s1[0] = 'h';
    cout << "修改 s1[0] = 'h' 后，s1 = " << s1 << endl;

    // 测试关系运算符
    MyString s4("Hello");
    MyString s5("Hello");
    cout << "\ns4 == s5: " << (s4 == s5) << endl;
    cout << "s4 != s1: " << (s4 != s1) << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "========================================" << endl;
    cout << "     运算符重载综合练习" << endl;
    cout << "========================================" << endl;

    // 练习1：复数类
    test_complex();

    // 练习2：计数器类
    test_counter();

    // 练习3：字符串类
    test_mystring();

    cout << "\n========================================" << endl;
    cout << "     所有练习完成！" << endl;
    cout << "========================================" << endl;

    return 0;
}

/*
编译运行：
g++ -o 04_operator_overload 04_operator_overload.cpp -std=c++17
./04_operator_overload

关键知识点验证：
1. 友元函数重载（+、-、*、==、!=、<<）
2. 成员函数重载（+=、++、--、[]、=）
3. 前置 vs 后置自增（返回值不同）
4. 运算符重载的本质是函数
5. 保持运算符的原有语义

核心理解：
- 运算符重载让自定义类型像内置类型一样使用
- 成员函数：左操作数是 this
- 友元函数：两个操作数都是参数
- 流操作符必须是友元函数
- 前置++返回引用，后置++返回副本
*/
