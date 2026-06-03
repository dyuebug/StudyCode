// ============================================
// 81_move_class_implementation.cpp - 实现支持移动语义的类
// 功能：实现完整的五法则、noexcept、自我赋值检查
// ============================================

#include <iostream>
#include <cstring>
#include <algorithm>
#include <vector>
using namespace std;

// ============================================
// String 类：完整的五法则实现
// ============================================

class String {
private:
    char* data;
    size_t length;

public:
    // 构造函数
    String(const char* str = "") {
        length = strlen(str);
        data = new char[length + 1];
        strcpy(data, str);
        cout << "构造：\"" << data << "\"" << endl;
    }

    // 析构函数
    ~String() {
        if (data) {
            cout << "析构：\"" << data << "\"" << endl;
            delete[] data;
        }
    }

    // 拷贝构造函数
    String(const String& other) {
        length = other.length;
        data = new char[length + 1];
        strcpy(data, other.data);
        cout << "拷贝构造：\"" << data << "\"" << endl;
    }

    // 拷贝赋值运算符
    String& operator=(const String& other) {
        if (this != &other) {  // 自我赋值检查
            delete[] data;
            length = other.length;
            data = new char[length + 1];
            strcpy(data, other.data);
            cout << "拷贝赋值：\"" << data << "\"" << endl;
        }
        return *this;
    }

    // 移动构造函数
    String(String&& other) noexcept
        : data(other.data), length(other.length) {
        other.data = nullptr;
        other.length = 0;
        cout << "移动构造：\"" << data << "\"" << endl;
    }

    // 移动赋值运算符
    String& operator=(String&& other) noexcept {
        if (this != &other) {  // 自我赋值检查
            delete[] data;
            data = other.data;
            length = other.length;
            other.data = nullptr;
            other.length = 0;
            cout << "移动赋值：\"" << data << "\"" << endl;
        }
        return *this;
    }

    // 辅助方法
    const char* c_str() const { return data ? data : ""; }
    size_t size() const { return length; }
    bool empty() const { return length == 0; }

    // 重载 + 运算符
    String operator+(const String& other) const {
        String result;
        result.length = length + other.length;
        result.data = new char[result.length + 1];
        strcpy(result.data, data);
        strcat(result.data, other.data);
        return result;
    }
};

// ============================================
// Array 类：动态数组的五法则实现
// ============================================

template<typename T>
class Array {
private:
    T* data;
    size_t capacity;
    size_t count;

public:
    // 构造函数
    Array(size_t cap = 10) : capacity(cap), count(0) {
        data = new T[capacity];
        cout << "Array 构造，容量：" << capacity << endl;
    }

    // 析构函数
    ~Array() {
        delete[] data;
        cout << "Array 析构，容量：" << capacity << endl;
    }

    // 拷贝构造函数
    Array(const Array& other)
        : capacity(other.capacity), count(other.count) {
        data = new T[capacity];
        for (size_t i = 0; i < count; i++) {
            data[i] = other.data[i];
        }
        cout << "Array 拷贝构造，容量：" << capacity << endl;
    }

    // 拷贝赋值运算符
    Array& operator=(const Array& other) {
        if (this != &other) {
            delete[] data;
            capacity = other.capacity;
            count = other.count;
            data = new T[capacity];
            for (size_t i = 0; i < count; i++) {
                data[i] = other.data[i];
            }
            cout << "Array 拷贝赋值，容量：" << capacity << endl;
        }
        return *this;
    }

    // 移动构造函数
    Array(Array&& other) noexcept
        : data(other.data), capacity(other.capacity), count(other.count) {
        other.data = nullptr;
        other.capacity = 0;
        other.count = 0;
        cout << "Array 移动构造" << endl;
    }

    // 移动赋值运算符
    Array& operator=(Array&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            capacity = other.capacity;
            count = other.count;
            other.data = nullptr;
            other.capacity = 0;
            other.count = 0;
            cout << "Array 移动赋值" << endl;
        }
        return *this;
    }

    // 添加元素
    void push_back(const T& value) {
        if (count >= capacity) {
            resize(capacity * 2);
        }
        data[count++] = value;
    }

    // 获取元素
    T& operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }

    size_t size() const { return count; }
    size_t getCapacity() const { return capacity; }

private:
    void resize(size_t newCapacity) {
        T* newData = new T[newCapacity];
        for (size_t i = 0; i < count; i++) {
            newData[i] = data[i];
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }
};

// ============================================
// 工厂函数
// ============================================

String createString(const char* str) {
    cout << "工厂函数创建 String..." << endl;
    return String(str);  // RVO
}

template<typename T>
Array<T> createArray(size_t capacity) {
    cout << "工厂函数创建 Array..." << endl;
    return Array<T>(capacity);  // RVO
}

// ============================================
// 常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
❌ 错误1：移动构造忘记清空源对象指针（双重释放）

   String(String&& other) noexcept : data(other.data), length(other.length) {
       // ❌ 忘记：other.data = nullptr;
   }
   // other 析构时：delete other.data（与 this->data 同地址）
   // 双重释放 → 崩溃

   ✅ 必须清空源对象：
   other.data = nullptr;
   other.length = 0;

------------------------------------------------------------

❌ 错误2：析构函数没有检查 nullptr

   ~String() {
       cout << "析构：\"" << data << "\"" << endl;  // ❌ data 可能是 nullptr
       delete[] data;  // delete nullptr 本身安全，但打印会崩溃
   }

   ✅ 先检查：
   if (data) {
       cout << "析构：\"" << data << "\"" << endl;
       delete[] data;
   }

------------------------------------------------------------

❌ 错误3：拷贝赋值没有自赋值检查（自赋值导致数据丢失）

   String& operator=(const String& other) {
       delete[] data;       // ❌ 如果 this == &other，释放了自己的 data
       data = new char[other.length + 1];
       strcpy(data, other.data);  // other.data 已被 delete，是野指针
       return *this;
   }

   ✅ 先检查：
   if (this == &other) return *this;

------------------------------------------------------------

❌ 错误4：Array 的 resize 用拷贝而非移动（性能差）

   void resize(size_t newCap) {
       T* newData = new T[newCap];
       for (size_t i = 0; i < count; i++) {
           newData[i] = data[i];   // ❌ 拷贝赋值，T 可能有昂贵的拷贝
       }
   }

   ✅ 如果 T 支持移动，用 move：
   for (size_t i = 0; i < count; i++) {
       newData[i] = move(data[i]);  // 移动，避免深拷贝
   }
   // 或者：使用 std::move（算法版本，移动范围）
   std::move(data, data + count, newData);

------------------------------------------------------------

❌ 错误5：只实现了移动，没有实现拷贝（三法则不完整）

   class Foo {
       int* ptr;
   public:
       Foo(int n) { ptr = new int[n]; }
       ~Foo() { delete[] ptr; }
       Foo(Foo&& o) noexcept { ptr = o.ptr; o.ptr = nullptr; }
       // ❌ 没有拷贝构造 → 编译器会生成浅拷贝版本
       // Foo a(5); Foo b = a; → b.ptr == a.ptr → 双重释放
   };

   ✅ 有析构/拷贝/移动任意一个 → 实现完整五法则

------------------------------------------------------------

❌ 错误6：返回局部变量时显式 move（阻止 NRVO）

   String createString() {
       String s("hello");
       return move(s);   // ❌ move 阻止了编译器的 NRVO 优化
   }

   ✅ 直接 return，编译器自动选择 NRVO 或移动：
   String createString() {
       String s("hello");
       return s;   // 编译器 NRVO 或隐式移动
   }
)" << endl;
}

// ============================================
// 函数卡片速查
// ============================================

void showFunctionCards() {
    cout << string(60, '=') << endl;
    cout << "函数卡片速查" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【五法则标准结构】

class MyClass {
    T* ptr;
public:
    MyClass(size_t n) : ptr(new T[n]) {}           // 1. 构造
    ~MyClass() { delete[] ptr; }                    // 2. 析构

    MyClass(const MyClass& o) {                     // 3. 拷贝构造（深拷贝）
        ptr = new T[...]; copy(o.ptr, ...);
    }
    MyClass& operator=(const MyClass& o) {          // 4. 拷贝赋值
        if (this == &o) return *this;               //    自赋值检查
        delete[] ptr; ptr = new T[...]; copy(...); return *this;
    }
    MyClass(MyClass&& o) noexcept                   // 5. 移动构造
        : ptr(o.ptr) { o.ptr = nullptr; }
    MyClass& operator=(MyClass&& o) noexcept {      // 6. 移动赋值
        if (this == &o) return *this;               //    自赋值检查
        delete[] ptr; ptr = o.ptr; o.ptr = nullptr; return *this;
    }
};

【检查清单】
  □ 析构：delete[] ptr（不是 delete ptr）
  □ 移动后清空源指针：o.ptr = nullptr
  □ 移动构造/赋值标记 noexcept
  □ 拷贝/移动赋值都有自赋值检查
  □ 返回 *this 引用（赋值运算符）
)" << endl;
}

// ============================================
// 练习题
// ============================================

void showExercises() {
    cout << string(60, '=') << endl;
    cout << "练习题（独立思考）" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 为 String 类添加 operator+ 的移动版本：

   String operator+(String&& other) const;   // 右侧是临时对象时的优化版本

   提示：如果右侧是右值，可以直接在 other.data 前面拼接，避免额外分配

2. 扩展 Array 类：

   a) 添加 insert(size_t pos, const T& val) — 在指定位置插入元素
   b) 添加 erase(size_t pos) — 删除指定位置元素
   c) 添加 emplace_back(T&& val) — 移动插入（与 push_back 的区别）

3. 思考题：以下代码打印什么？（分析每一步）

   String a("hello");
   String b = a;
   String c = move(b);
   String d;
   d = c;
   d = move(c);
   cout << a.c_str() << b.c_str() << c.c_str() << d.c_str();
)" << endl;
}

int main() {
    cout << "=== 移动语义类实现示例 ===" << endl;

    // 示例1：String 类基本操作
    {
        cout << "\n【示例1】String 类基本操作" << endl;
        cout << string(60, '-') << endl;

        String s1("Hello");
        String s2 = s1;              // 拷贝构造
        String s3 = move(s1);        // 移动构造

        cout << "\n检查状态：" << endl;
        cout << "s1: \"" << s1.c_str() << "\"" << endl;
        cout << "s2: \"" << s2.c_str() << "\"" << endl;
        cout << "s3: \"" << s3.c_str() << "\"" << endl;

        cout << "\n离开作用域..." << endl;
    }

    // 示例2：String 类赋值操作
    {
        cout << "\n【示例2】String 类赋值操作" << endl;
        cout << string(60, '-') << endl;

        String s1("World");
        String s2("C++");

        cout << "\n拷贝赋值：" << endl;
        s2 = s1;                     // 拷贝赋值

        String s3("Move");
        cout << "\n移动赋值：" << endl;
        s3 = move(s1);               // 移动赋值

        cout << "\n检查状态：" << endl;
        cout << "s1: \"" << s1.c_str() << "\"" << endl;
        cout << "s2: \"" << s2.c_str() << "\"" << endl;
        cout << "s3: \"" << s3.c_str() << "\"" << endl;

        cout << "\n离开作用域..." << endl;
    }

    // 示例3：String 类函数返回
    {
        cout << "\n【示例3】String 类函数返回" << endl;
        cout << string(60, '-') << endl;

        String s = createString("Factory");
        cout << "返回的字符串：\"" << s.c_str() << "\"" << endl;

        cout << "\n离开作用域..." << endl;
    }

    // 示例4：String 类在容器中使用
    {
        cout << "\n【示例4】String 类在容器中使用" << endl;
        cout << string(60, '-') << endl;

        vector<String> vec;
        vec.reserve(3);

        cout << "\n添加临时对象：" << endl;
        vec.push_back(String("String1"));

        cout << "\n添加左值：" << endl;
        String s("String2");
        vec.push_back(s);

        cout << "\n添加移动：" << endl;
        vec.push_back(move(s));

        cout << "\n遍历容器：" << endl;
        for (size_t i = 0; i < vec.size(); i++) {
            cout << "vec[" << i << "]: \"" << vec[i].c_str() << "\"" << endl;
        }

        cout << "\n离开作用域..." << endl;
    }

    // 示例5：String 类运算符重载
    {
        cout << "\n【示例5】String 类运算符重载" << endl;
        cout << string(60, '-') << endl;

        String s1("Hello");
        String s2(" World");

        cout << "\n字符串拼接：" << endl;
        String s3 = s1 + s2;
        cout << "结果：\"" << s3.c_str() << "\"" << endl;

        cout << "\n离开作用域..." << endl;
    }

    // 示例6：Array 类基本操作
    {
        cout << "\n【示例6】Array 类基本操作" << endl;
        cout << string(60, '-') << endl;

        Array<int> arr1(5);
        arr1.push_back(10);
        arr1.push_back(20);
        arr1.push_back(30);

        cout << "\n拷贝构造：" << endl;
        Array<int> arr2 = arr1;

        cout << "\n移动构造：" << endl;
        Array<int> arr3 = move(arr1);

        cout << "\n检查状态：" << endl;
        cout << "arr1 大小：" << arr1.size() << endl;
        cout << "arr2 大小：" << arr2.size() << endl;
        cout << "arr3 大小：" << arr3.size() << endl;

        cout << "\narr3 的元素：";
        for (size_t i = 0; i < arr3.size(); i++) {
            cout << arr3[i] << " ";
        }
        cout << endl;

        cout << "\n离开作用域..." << endl;
    }

    // 示例7：Array 类赋值操作
    {
        cout << "\n【示例7】Array 类赋值操作" << endl;
        cout << string(60, '-') << endl;

        Array<int> arr1(5);
        arr1.push_back(100);
        arr1.push_back(200);

        Array<int> arr2(3);
        arr2.push_back(1);

        cout << "\n拷贝赋值：" << endl;
        arr2 = arr1;

        Array<int> arr3(2);
        cout << "\n移动赋值：" << endl;
        arr3 = move(arr1);

        cout << "\n检查状态：" << endl;
        cout << "arr1 大小：" << arr1.size() << endl;
        cout << "arr2 大小：" << arr2.size() << endl;
        cout << "arr3 大小：" << arr3.size() << endl;

        cout << "\n离开作用域..." << endl;
    }

    // 示例8：自我赋值检查
    {
        cout << "\n【示例8】自我赋值检查" << endl;
        cout << string(60, '-') << endl;

        String s("Test");

        cout << "\n自我拷贝赋值：" << endl;
        s = s;  // 应该什么都不做

        cout << "\n自我移动赋值：" << endl;
        s = move(s);  // 应该什么都不做

        cout << "字符串仍然有效：\"" << s.c_str() << "\"" << endl;

        cout << "\n离开作用域..." << endl;
    }

    // 示例9：noexcept 的重要性
    {
        cout << "\n【示例9】noexcept 的重要性" << endl;
        cout << string(60, '-') << endl;

        cout << "\n移动操作标记为 noexcept 的好处：" << endl;
        cout << "1. STL 容器会优先使用移动而非拷贝" << endl;
        cout << "2. 提高性能" << endl;
        cout << "3. 保证异常安全" << endl;

        cout << "\n检查 String 的移动构造是否 noexcept：" << endl;
        cout << "noexcept(String(declval<String&&>())): "
             << noexcept(String(declval<String&&>())) << endl;

        cout << "\nvector 扩容时会使用移动：" << endl;
        vector<String> vec;
        vec.push_back(String("A"));
        vec.push_back(String("B"));  // 可能触发扩容，使用移动
        vec.push_back(String("C"));  // 可能触发扩容，使用移动

        cout << "\n离开作用域..." << endl;
    }

    // 示例10：性能对比
    {
        cout << "\n【示例10】性能对比" << endl;
        cout << string(60, '-') << endl;

        const int COUNT = 3;

        cout << "\n创建 String 数组（拷贝）：" << endl;
        vector<String> vec1;
        vec1.reserve(COUNT);
        for (int i = 0; i < COUNT; i++) {
            String s("String");
            vec1.push_back(s);  // 拷贝
        }

        cout << "\n创建 String 数组（移动）：" << endl;
        vector<String> vec2;
        vec2.reserve(COUNT);
        for (int i = 0; i < COUNT; i++) {
            String s("String");
            vec2.push_back(move(s));  // 移动
        }

        cout << "\n创建 String 数组（直接构造）：" << endl;
        vector<String> vec3;
        vec3.reserve(COUNT);
        for (int i = 0; i < COUNT; i++) {
            vec3.push_back(String("String"));  // 临时对象，自动移动
        }

        cout << "\n性能：直接构造 ≈ 移动 > 拷贝" << endl;

        cout << "\n离开作用域..." << endl;
    }

    cout << "\n程序结束" << endl;
    return 0;
}
