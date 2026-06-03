// ============================================
// RAII 与资源管理 - 超详细注释版
// 日期：2026-05-08（第22天）
// 难度：⭐⭐⭐ 中高级
// ============================================

/*
【核心概念】RAII（Resource Acquisition Is Initialization）

是什么？
- RAII 是一种资源管理技术，资源获取即初始化
- 利用对象的生命周期管理资源
- 构造函数获取资源，析构函数释放资源

为什么需要？
- 自动资源管理：无需手动释放
- 异常安全：即使抛出异常也能正确释放
- 简化代码：资源管理逻辑封装在类中

核心特点：
- 资源与对象生命周期绑定
- 离开作用域时自动释放
- 异常安全保证
*/

#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
using namespace std;

// ============================================
// 第一部分：RAII 核心概念 ⭐⭐⭐⭐⭐
// ============================================

void part1_concepts() {
    cout << string(60, '=') << endl;
    cout << "第一部分：RAII 核心概念" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【什么是 RAII？】

RAII = Resource Acquisition Is Initialization
资源获取即初始化

核心思想：
- 在对象构造时获取资源
- 在对象析构时释放资源
- 利用 C++ 的自动析构机制管理资源

【为什么需要 RAII？】

问题：手动管理资源容易出错

FILE* file = fopen("data.txt", "r");
// ... 使用文件
fclose(file);  // 忘记关闭 → 资源泄漏
               // 异常抛出 → 资源泄漏

解决：RAII 自动管理

class FileHandle {
    FILE* file;
public:
    FileHandle(const char* name) {
        file = fopen(name, "r");  // 构造时打开
    }
    ~FileHandle() {
        if (file) fclose(file);   // 析构时关闭
    }
};

{
    FileHandle fh("data.txt");
    // ... 使用文件
}  // 离开作用域，自动关闭文件

【RAII 的优势】

1. 自动管理：无需手动释放
2. 异常安全：即使抛出异常也能正确释放
3. 简化代码：资源管理逻辑封装在类中
4. 防止泄漏：编译器保证析构函数被调用

【RAII 管理的资源】

- 内存：new/delete → unique_ptr/shared_ptr
- 文件：fopen/fclose → fstream
- 锁：lock/unlock → lock_guard/unique_lock
- 网络连接：connect/disconnect → 自定义 RAII 类
- 数据库连接：open/close → 自定义 RAII 类
)" << endl;
}

// ============================================
// 第二部分：RAII 实践示例
// ============================================

// 示例1：文件资源管理
class FileRAII {
private:
    FILE* file;
    string filename;

public:
    // 构造函数：获取资源
    FileRAII(const string& name, const char* mode) : filename(name) {
        file = fopen(name.c_str(), mode);
        if (!file) {
            throw runtime_error("无法打开文件：" + name);
        }
        cout << "文件已打开：" << filename << endl;
    }

    // 析构函数：释放资源
    ~FileRAII() {
        if (file) {
            fclose(file);
            cout << "文件已关闭：" << filename << endl;
        }
    }

    // 禁止拷贝
    FileRAII(const FileRAII&) = delete;
    FileRAII& operator=(const FileRAII&) = delete;

    // 获取文件指针
    FILE* get() const { return file; }
};

void part2_file_example() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第二部分：文件资源管理示例" << endl;
    cout << string(60, '=') << endl;

    cout << "\n示例1：RAII 管理文件" << endl;
    cout << string(60, '-') << endl;

    try {
        FileRAII file("test.txt", "w");

        // 使用文件
        fprintf(file.get(), "Hello, RAII!\n");

        // 离开作用域时自动关闭文件
    } catch (const exception& e) {
        cout << "错误：" << e.what() << endl;
    }

    cout << "\n示例2：异常安全" << endl;
    cout << string(60, '-') << endl;

    try {
        FileRAII file("test.txt", "r");

        // 模拟异常
        throw runtime_error("发生异常");

        // 即使抛出异常，析构函数也会被调用
    } catch (const exception& e) {
        cout << "捕获异常：" << e.what() << endl;
        cout << "文件仍然被正确关闭" << endl;
    }
}

// 示例2：锁资源管理
class MutexRAII {
private:
    mutex& mtx;

public:
    // 构造函数：获取锁
    MutexRAII(mutex& m) : mtx(m) {
        mtx.lock();
        cout << "锁已获取" << endl;
    }

    // 析构函数：释放锁
    ~MutexRAII() {
        mtx.unlock();
        cout << "锁已释放" << endl;
    }

    // 禁止拷贝
    MutexRAII(const MutexRAII&) = delete;
    MutexRAII& operator=(const MutexRAII&) = delete;
};

void part3_mutex_example() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第三部分：锁资源管理示例" << endl;
    cout << string(60, '=') << endl;

    mutex mtx;

    cout << "\n示例1：RAII 管理锁" << endl;
    cout << string(60, '-') << endl;

    {
        MutexRAII lock(mtx);
        cout << "临界区代码" << endl;
        // 离开作用域时自动释放锁
    }

    cout << "\n示例2：标准库的 lock_guard" << endl;
    cout << string(60, '-') << endl;

    {
        lock_guard<mutex> lock(mtx);
        cout << "使用 lock_guard 保护临界区" << endl;
        // 离开作用域时自动释放锁
    }

    cout << "\n示例3：unique_lock（更灵活）" << endl;
    cout << string(60, '-') << endl;

    {
        unique_lock<mutex> lock(mtx);
        cout << "使用 unique_lock" << endl;

        // 可以手动解锁
        lock.unlock();
        cout << "手动解锁" << endl;

        // 可以重新加锁
        lock.lock();
        cout << "重新加锁" << endl;

        // 离开作用域时自动释放锁
    }
}

// 示例3：自定义资源管理类
class ArrayRAII {
private:
    int* data;
    size_t size;

public:
    // 构造函数：分配内存
    ArrayRAII(size_t n) : size(n) {
        data = new int[size];
        cout << "分配了 " << size << " 个 int 的内存" << endl;
    }

    // 析构函数：释放内存
    ~ArrayRAII() {
        delete[] data;
        cout << "释放了 " << size << " 个 int 的内存" << endl;
    }

    // 禁止拷贝
    ArrayRAII(const ArrayRAII&) = delete;
    ArrayRAII& operator=(const ArrayRAII&) = delete;

    // 允许移动
    ArrayRAII(ArrayRAII&& other) noexcept
        : data(other.data), size(other.size) {
        other.data = nullptr;
        other.size = 0;
    }

    // 访问元素
    int& operator[](size_t index) {
        return data[index];
    }

    size_t getSize() const { return size; }
};

void part4_custom_raii() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第四部分：自定义 RAII 类" << endl;
    cout << string(60, '=') << endl;

    cout << "\n示例：自定义数组管理类" << endl;
    cout << string(60, '-') << endl;

    {
        ArrayRAII arr(5);

        // 初始化数组
        for (size_t i = 0; i < arr.getSize(); i++) {
            arr[i] = i * 10;
        }

        // 打印数组
        cout << "数组元素：";
        for (size_t i = 0; i < arr.getSize(); i++) {
            cout << arr[i] << " ";
        }
        cout << endl;

        // 离开作用域时自动释放内存
    }
}

// ============================================
// 第五部分：RAII 与智能指针
// ============================================

void part5_smart_pointers() {
    cout << "\n" << string(60, '=') << endl;
    cout << "第五部分：RAII 与智能指针" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【智能指针是 RAII 的典型应用】

unique_ptr/shared_ptr 就是 RAII 的实现：

class unique_ptr {
    T* ptr;
public:
    unique_ptr(T* p) : ptr(p) {}  // 构造时获取资源
    ~unique_ptr() { delete ptr; }  // 析构时释放资源

    T& operator*() { return *ptr; }
    T* operator->() { return ptr; }
};

【RAII 的其他应用】

1. 智能指针：unique_ptr、shared_ptr
2. 容器：vector、string（自动管理内存）
3. 锁：lock_guard、unique_lock
4. 文件流：fstream（自动关闭文件）
5. 自定义资源管理类

【RAII 设计原则】

1. 构造函数获取资源
2. 析构函数释放资源
3. 禁止拷贝（或实现深拷贝）
4. 允许移动（转移所有权）
5. 提供访问接口
)" << endl;
}

// ============================================
// 第六部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

void part6_mistakes() {
    cout << "\n" << string(60, '=') << endl;
    cout << "常见错误和陷阱 ⭐⭐⭐⭐⭐" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【错误1：忘记禁止拷贝】

❌ 错误：
class FileHandle {
    FILE* file;
public:
    FileHandle(const char* name) { file = fopen(name, "r"); }
    ~FileHandle() { fclose(file); }
    // 没有禁止拷贝！
};

FileHandle f1("test.txt");
FileHandle f2 = f1;  // 浅拷贝，两个对象共享同一个 file
// 析构时会重复关闭文件 → 程序崩溃！

✅ 正确：
class FileHandle {
    FILE* file;
public:
    FileHandle(const char* name) { file = fopen(name, "r"); }
    ~FileHandle() { fclose(file); }

    // 禁止拷贝
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
};

────────────────────────────────────

【错误2：资源泄漏】

❌ 错误：
class Resource {
    int* data;
public:
    Resource() { data = new int[100]; }
    ~Resource() {}  // 忘记释放！
};

✅ 正确：
class Resource {
    int* data;
public:
    Resource() { data = new int[100]; }
    ~Resource() { delete[] data; }  // 释放资源
};

或者更好：
class Resource {
    unique_ptr<int[]> data;
public:
    Resource() : data(make_unique<int[]>(100)) {}
    // 自动释放，无需手动 delete
};

────────────────────────────────────

【错误3：异常不安全】

❌ 错误：
void func() {
    int* ptr = new int(10);
    someFunction();  // 可能抛出异常
    delete ptr;      // 异常时不会执行 → 内存泄漏
}

✅ 正确：
void func() {
    unique_ptr<int> ptr = make_unique<int>(10);
    someFunction();  // 即使抛出异常，ptr 也会自动释放
}

────────────────────────────────────

【错误4：手动管理资源】

❌ 不推荐：
FILE* file = fopen("test.txt", "r");
// ... 使用文件
fclose(file);

✅ 推荐：
{
    ifstream file("test.txt");
    // ... 使用文件
}  // 自动关闭

────────────────────────────────────

【错误5：在析构函数中抛出异常】

❌ 错误：
class Resource {
public:
    ~Resource() {
        throw runtime_error("错误");  // 析构函数不应抛出异常！
    }
};

原因：
- 析构函数在异常处理期间可能被调用
- 如果析构函数再抛出异常，程序会终止

✅ 正确：
class Resource {
public:
    ~Resource() noexcept {
        try {
            // 可能抛出异常的代码
        } catch (...) {
            // 捕获所有异常，不让它逃逸
        }
    }
};
)" << endl;
}

// ============================================
// 第七部分：RAII 最佳实践
// ============================================

void part7_best_practices() {
    cout << "\n" << string(60, '=') << endl;
    cout << "RAII 最佳实践" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
【最佳实践】

1. 优先使用标准库的 RAII 类
   - unique_ptr/shared_ptr 管理内存
   - fstream 管理文件
   - lock_guard/unique_lock 管理锁

2. 自定义 RAII 类时
   - 构造函数获取资源
   - 析构函数释放资源
   - 禁止拷贝（或实现深拷贝）
   - 允许移动（转移所有权）

3. 异常安全
   - 析构函数不应抛出异常
   - 使用 noexcept 标记析构函数

4. 资源管理
   - 一个类只管理一种资源
   - 遵循单一职责原则

5. 避免手动管理资源
   - 不要使用 new/delete
   - 不要使用 fopen/fclose
   - 不要手动 lock/unlock

【RAII 设计模式】

class ResourceManager {
private:
    Resource* resource;

public:
    // 构造：获取资源
    ResourceManager() {
        resource = acquireResource();
    }

    // 析构：释放资源
    ~ResourceManager() {
        releaseResource(resource);
    }

    // 禁止拷贝
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // 允许移动
    ResourceManager(ResourceManager&& other) noexcept {
        resource = other.resource;
        other.resource = nullptr;
    }

    // 访问接口
    Resource* get() const { return resource; }
};
)" << endl;
}

// ============================================
// 练习题
// ============================================

void exercises() {
    cout << "\n" << string(60, '=') << endl;
    cout << "练习题" << endl;
    cout << string(60, '=') << endl;

    cout << R"(
1. 实现一个 RAII 类管理动态数组
   要求：构造时分配，析构时释放，支持移动
   提示：禁止拷贝，实现移动构造和移动赋值

2. 实现一个 RAII 类管理文件
   要求：构造时打开，析构时关闭，提供读写接口
   提示：使用 fopen/fclose

3. 实现一个 RAII 类管理计时器
   要求：构造时开始计时，析构时输出耗时
   提示：使用 chrono 库

4. 实现一个 RAII 类管理临时目录
   要求：构造时创建目录，析构时删除目录
   提示：使用 filesystem 库（C++17）

5. 实现一个 RAII 类管理数据库连接
   要求：构造时连接，析构时断开，提供查询接口
   提示：模拟数据库连接

6. 实现一个 RAII 类管理作用域日志
   要求：构造时输出"进入"，析构时输出"离开"
   提示：记录函数名和时间戳

7. 实现一个 RAII 类管理资源池
   要求：构造时从池中获取，析构时归还池中
   提示：使用 shared_ptr 和自定义删除器
)" << endl;
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "RAII 与资源管理学习" << endl;
    cout << string(60, '=') << endl;

    part1_concepts();
    part2_file_example();
    part3_mutex_example();
    part4_custom_raii();
    part5_smart_pointers();
    part6_mistakes();
    part7_best_practices();
    exercises();

    cout << "\n" << string(60, '=') << endl;
    cout << "学习完成！" << endl;
    cout << string(60, '=') << endl;

    return 0;
}
