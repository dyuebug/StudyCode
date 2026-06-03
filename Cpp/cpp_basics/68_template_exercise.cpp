// ============================================
// 模板综合练习 - 超详细注释版
// 日期：2026-05-07（第21天）
// 难度：⭐⭐⭐ 中高级
// ============================================

/*
【学习目标】

通过实战练习巩固模板知识：
1. 实现常用的泛型数据结构
2. 实现常用的泛型算法
3. 理解 STL 的设计思想
4. 掌握模板的实际应用

【练习内容】

- 实现通用容器（Array、Stack、Queue）
- 实现泛型算法（max、min、swap、find）
- 综合应用：学生管理系统（模板版）
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 练习1：实现通用数组（Array）
// ============================================

// 类模板：固定大小的数组
// T - 元素类型
// N - 数组大小（非类型模板参数）
template<typename T, int N>
class Array {
private:
    T data[N];  // 固定大小的数组

public:
    // 构造函数：初始化所有元素为默认值
    Array() {
        for (int i = 0; i < N; i++) {
            data[i] = T();  // T() 是默认值
        }
    }

    // 重载 [] 运算符：访问元素
    // 返回引用，可以修改元素
    T& operator[](int index) {
        return data[index];
    }

    // const 版本：只读访问
    const T& operator[](int index) const {
        return data[index];
    }

    // 获取数组大小
    int size() const {
        return N;
    }

    // 填充数组：所有元素设为同一个值
    void fill(const T& value) {
        for (int i = 0; i < N; i++) {
            data[i] = value;
        }
    }

    // 打印数组
    void print() const {
        cout << "[";
        for (int i = 0; i < N; i++) {
            cout << data[i];
            if (i < N - 1) cout << ", ";
        }
        cout << "]" << endl;
    }
};

void exercise1() {
    cout << string(60, '=') << endl;
    cout << "练习1：实现通用数组（Array）" << endl;
    cout << string(60, '=') << endl;

    // 创建 int 类型的数组，大小为 5
    Array<int, 5> intArray;
    intArray[0] = 10;
    intArray[1] = 20;
    intArray[2] = 30;
    intArray[3] = 40;
    intArray[4] = 50;

    cout << "Int Array: ";
    intArray.print();

    // 创建 double 类型的数组，大小为 3
    Array<double, 3> doubleArray;
    doubleArray.fill(3.14);

    cout << "Double Array (filled): ";
    doubleArray.print();

    // 创建 string 类型的数组，大小为 4
    Array<string, 4> strArray;
    strArray[0] = "Hello";
    strArray[1] = "World";
    strArray[2] = "C++";
    strArray[3] = "Template";

    cout << "String Array: ";
    strArray.print();
}

// ============================================
// 练习2：实现通用栈（Stack）
// ============================================

// 类模板：动态栈（使用动态数组）
template<typename T>
class Stack {
private:
    T* data;       // 动态数组
    int capacity;  // 容量
    int top;       // 栈顶索引

    // 扩容：当栈满时，容量翻倍
    void resize() {
        capacity *= 2;
        T* newData = new T[capacity];

        // 复制旧数据
        for (int i = 0; i < top; i++) {
            newData[i] = data[i];
        }

        // 释放旧内存
        delete[] data;
        data = newData;
    }

public:
    // 构造函数：初始化栈
    Stack(int initialCapacity = 10)
        : capacity(initialCapacity), top(0) {
        data = new T[capacity];
    }

    // 析构函数：释放内存
    ~Stack() {
        delete[] data;
    }

    // 拷贝构造函数：深拷贝
    Stack(const Stack& other)
        : capacity(other.capacity), top(other.top) {
        data = new T[capacity];
        for (int i = 0; i < top; i++) {
            data[i] = other.data[i];
        }
    }

    // 赋值运算符：深拷贝
    Stack& operator=(const Stack& other) {
        if (this != &other) {
            delete[] data;

            capacity = other.capacity;
            top = other.top;
            data = new T[capacity];

            for (int i = 0; i < top; i++) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }

    // 入栈
    void push(const T& value) {
        if (top >= capacity) {
            resize();  // 栈满时扩容
        }
        data[top++] = value;
    }

    // 出栈
    T pop() {
        if (isEmpty()) {
            throw runtime_error("Stack is empty!");
        }
        return data[--top];
    }

    // 查看栈顶元素
    T& peek() {
        if (isEmpty()) {
            throw runtime_error("Stack is empty!");
        }
        return data[top - 1];
    }

    // 判断栈是否为空
    bool isEmpty() const {
        return top == 0;
    }

    // 获取栈的大小
    int size() const {
        return top;
    }

    // 清空栈
    void clear() {
        top = 0;
    }
};

void exercise2() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习2：实现通用栈（Stack）" << endl;
    cout << string(60, '=') << endl;

    // 创建 int 类型的栈
    Stack<int> intStack;
    for (int i = 1; i <= 5; i++) {
        intStack.push(i * 10);
    }

    cout << "Int Stack (size: " << intStack.size() << "): ";
    while (!intStack.isEmpty()) {
        cout << intStack.pop() << " ";
    }
    cout << endl;

    // 创建 string 类型的栈
    Stack<string> strStack;
    strStack.push("First");
    strStack.push("Second");
    strStack.push("Third");

    cout << "String Stack: ";
    while (!strStack.isEmpty()) {
        cout << strStack.pop() << " ";
    }
    cout << endl;
}

// ============================================
// 练习3：实现泛型算法
// ============================================

// 函数模板：查找最大值
template<typename T>
T findMax(const T* arr, int size) {
    if (size <= 0) {
        throw invalid_argument("Array size must be positive!");
    }

    T maxVal = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > maxVal) {
            maxVal = arr[i];
        }
    }
    return maxVal;
}

// 函数模板：查找最小值
template<typename T>
T findMin(const T* arr, int size) {
    if (size <= 0) {
        throw invalid_argument("Array size must be positive!");
    }

    T minVal = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] < minVal) {
            minVal = arr[i];
        }
    }
    return minVal;
}

// 函数模板：查找元素
// 返回索引，找不到返回 -1
template<typename T>
int find(const T* arr, int size, const T& target) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == target) {
            return i;
        }
    }
    return -1;
}

// 函数模板：反转数组
template<typename T>
void reverse(T* arr, int size) {
    for (int i = 0; i < size / 2; i++) {
        T temp = arr[i];
        arr[i] = arr[size - 1 - i];
        arr[size - 1 - i] = temp;
    }
}

// 函数模板：打印数组
template<typename T>
void printArray(const T* arr, int size) {
    cout << "[";
    for (int i = 0; i < size; i++) {
        cout << arr[i];
        if (i < size - 1) cout << ", ";
    }
    cout << "]" << endl;
}

void exercise3() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习3：实现泛型算法" << endl;
    cout << string(60, '=') << endl;

    // 测试 int 数组
    int intArr[] = {5, 2, 8, 1, 9, 3};
    int intSize = sizeof(intArr) / sizeof(intArr[0]);

    cout << "Int Array: ";
    printArray(intArr, intSize);

    cout << "Max: " << findMax(intArr, intSize) << endl;
    cout << "Min: " << findMin(intArr, intSize) << endl;
    cout << "Find 8: index = " << find(intArr, intSize, 8) << endl;

    reverse(intArr, intSize);
    cout << "After reverse: ";
    printArray(intArr, intSize);

    // 测试 string 数组
    string strArr[] = {"apple", "banana", "cherry", "date"};
    int strSize = sizeof(strArr) / sizeof(strArr[0]);

    cout << "\nString Array: ";
    printArray(strArr, strSize);

    cout << "Max: " << findMax(strArr, strSize) << endl;
    cout << "Min: " << findMin(strArr, strSize) << endl;
    cout << "Find \"cherry\": index = " << find(strArr, strSize, string("cherry")) << endl;
}

// ============================================
// 练习4：综合应用 - 通用容器管理系统
// ============================================

// 类模板：通用容器管理器
template<typename T>
class Container {
private:
    T* items;      // 动态数组
    int capacity;  // 容量
    int count;     // 当前元素数量

    // 扩容
    void resize() {
        capacity *= 2;
        T* newItems = new T[capacity];

        for (int i = 0; i < count; i++) {
            newItems[i] = items[i];
        }

        delete[] items;
        items = newItems;
    }

public:
    // 构造函数
    Container(int initialCapacity = 10)
        : capacity(initialCapacity), count(0) {
        items = new T[capacity];
    }

    // 析构函数
    ~Container() {
        delete[] items;
    }

    // 添加元素
    void add(const T& item) {
        if (count >= capacity) {
            resize();
        }
        items[count++] = item;
    }

    // 删除元素（按索引）
    void remove(int index) {
        if (index < 0 || index >= count) {
            throw out_of_range("Index out of range!");
        }

        for (int i = index; i < count - 1; i++) {
            items[i] = items[i + 1];
        }
        count--;
    }

    // 获取元素
    T& get(int index) {
        if (index < 0 || index >= count) {
            throw out_of_range("Index out of range!");
        }
        return items[index];
    }

    // 查找元素
    int indexOf(const T& item) const {
        for (int i = 0; i < count; i++) {
            if (items[i] == item) {
                return i;
            }
        }
        return -1;
    }

    // 获取元素数量
    int size() const {
        return count;
    }

    // 判断是否为空
    bool isEmpty() const {
        return count == 0;
    }

    // 清空容器
    void clear() {
        count = 0;
    }

    // 打印所有元素
    void print() const {
        cout << "Container [" << count << " items]: ";
        for (int i = 0; i < count; i++) {
            cout << items[i];
            if (i < count - 1) cout << ", ";
        }
        cout << endl;
    }
};

// 学生类（用于测试）
class Student {
private:
    string name;
    int age;

public:
    Student() : name(""), age(0) {}
    Student(string n, int a) : name(n), age(a) {}

    string getName() const { return name; }
    int getAge() const { return age; }

    // 重载 == 运算符（用于查找）
    bool operator==(const Student& other) const {
        return name == other.name && age == other.age;
    }

    // 重载 << 运算符（用于打印）
    friend ostream& operator<<(ostream& os, const Student& s) {
        os << s.name << "(" << s.age << ")";
        return os;
    }
};

void exercise4() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习4：综合应用 - 通用容器管理系统" << endl;
    cout << string(60, '=') << endl;

    // 管理 int 类型
    Container<int> intContainer;
    intContainer.add(10);
    intContainer.add(20);
    intContainer.add(30);
    intContainer.print();

    // 管理 string 类型
    Container<string> strContainer;
    strContainer.add("Alice");
    strContainer.add("Bob");
    strContainer.add("Charlie");
    strContainer.print();

    // 管理 Student 类型
    Container<Student> studentContainer;
    studentContainer.add(Student("Alice", 20));
    studentContainer.add(Student("Bob", 22));
    studentContainer.add(Student("Charlie", 21));

    cout << "\nStudent Container:" << endl;
    studentContainer.print();

    // 查找学生
    Student target("Bob", 22);
    int index = studentContainer.indexOf(target);
    if (index != -1) {
        cout << "Found: " << studentContainer.get(index) << " at index " << index << endl;
    }

    // 删除学生
    studentContainer.remove(1);
    cout << "After removing Bob:" << endl;
    studentContainer.print();
}

// ============================================
// 主函数
// ============================================

// 常见错误和陷阱
void showCommonErrors() {
    cout << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;
    cout << R"(
❌ 错误1：模板定义放在 .cpp 文件（链接错误）
   // utils.h 只有声明，utils.cpp 有定义 → 其他文件无法实例化
   ✅ 模板定义必须放在头文件或使用处可见

❌ 错误2：类模板成员函数在类外定义时遗漏模板参数
   template<typename T>
   T Stack<T>::pop() { ... }    // ✅ 正确
   T Stack::pop() { ... }       // ❌ 缺少 <T>

❌ 错误3：模板参数推导失败（混合类型）
   myMax(10, 3.14);   // ❌ T 无法唯一确定：int vs double
   ✅ 显式指定：myMax<double>(10, 3.14)

❌ 错误4：类模板特化时遗漏 template<> 前缀
   void Stack<int>::push(int v) { ... }    // ❌ 缺少 template<>
   ✅ template<> void Stack<int>::push(int v) { ... }

❌ 错误5：模板中使用依赖类型时忘记 typename
   template<typename T>
   void func() { T::value_type x; }        // ❌ value_type 是依赖类型
   ✅ typename T::value_type x;
)" << endl;
}

int main() {
    cout << "模板综合练习" << endl;
    cout << string(60, '=') << endl;

    try {
        exercise1();
        exercise2();
        exercise3();
        exercise4();

        cout << "\n" << string(60, '=') << endl;
        cout << "所有练习完成！" << endl;
        cout << string(60, '=') << endl;

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}

/*
==============================================
🎯 学习总结
==============================================

通过这些练习，你应该掌握了：

1. 【类模板实现】
   - 固定大小数组（Array）
   - 动态栈（Stack）
   - 通用容器（Container）

2. 【函数模板实现】
   - 查找算法（findMax、findMin、find）
   - 数组操作（reverse、printArray）

3. 【模板应用】
   - 泛型数据结构
   - 泛型算法
   - 实际项目应用

4. 【关键技术】
   - 动态内存管理
   - 深拷贝与浅拷贝
   - 运算符重载
   - 异常处理

5. 【STL 设计思想】
   - 容器与算法分离
   - 类型参数化
   - 编译期多态

==============================================

🚀 下一步学习建议：

1. 学习智能指针（unique_ptr、shared_ptr）
2. 学习 STL 迭代器的实现原理
3. 学习模板元编程（高级主题）
4. 实践：用模板重构之前的项目

==============================================
*/
