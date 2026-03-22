/*
 * ============================================================================
 * 文件名：29_dynamic_memory_exercise.cpp
 * 主题：动态内存练习
 * 学习目标：
 *   1. 动态创建和释放对象
 *   2. 动态数组的实际应用
 *   3. 内存管理最佳实践
 *   4. 实战案例：动态学生管理系统
 *   5. 掌握RAII思想（资源获取即初始化）
 *
 * 编译命令：g++ -std=c++11 -o 29_dynamic_memory_exercise 29_dynamic_memory_exercise.cpp
 * 运行命令：./29_dynamic_memory_exercise (Linux/Mac) 或 29_dynamic_memory_exercise.exe (Windows)
 * ============================================================================
 */

#include <iostream>
#include <string>
#include <cstring>
using namespace std;

// 学生结构体
struct Student {
    string name;
    int age;
    double score;
};

// 函数声明
int* createArray(int size);
void fillArray(int* arr, int size);
void printArray(const int* arr, int size);
Student* createStudent(const string& name, int age, double score);
void printStudent(const Student* s);
int** createMatrix(int rows, int cols);
void fillMatrix(int** matrix, int rows, int cols);
void printMatrix(int** matrix, int rows, int cols);
void deleteMatrix(int** matrix, int rows);

int main() {
    cout << "=== 动态内存练习 ===" << endl << endl;

    // ========================================
    // 练习1：动态创建和使用数组
    // ========================================
    cout << "【练习1：动态创建和使用数组】" << endl;

    int size = 5;
    int* arr = createArray(size);
    fillArray(arr, size);

    cout << "数组元素：";
    printArray(arr, size);

    // 计算总和
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    cout << "总和：" << sum << endl;
    cout << "平均值：" << sum / (double)size << endl;

    delete[] arr;
    arr = nullptr;
    cout << "✓ 数组已释放" << endl;
    cout << endl;

    // ========================================
    // 练习2：动态创建结构体对象
    // ========================================
    cout << "【练习2：动态创建结构体对象】" << endl;

    Student* s1 = createStudent("Alice", 20, 95.5);
    Student* s2 = createStudent("Bob", 21, 88.0);
    Student* s3 = createStudent("Charlie", 19, 92.5);

    cout << "学生信息：" << endl;
    printStudent(s1);
    printStudent(s2);
    printStudent(s3);

    // 释放
    delete s1;
    delete s2;
    delete s3;
    s1 = s2 = s3 = nullptr;
    cout << "✓ 所有学生对象已释放" << endl;
    cout << endl;

    // ========================================
    // 练习3：动态学生数组
    // ========================================
    cout << "【练习3：动态学生数组】" << endl;

    int numStudents = 3;
    Student* students = new Student[numStudents];

    // 初始化
    students[0] = {"David", 20, 85.0};
    students[1] = {"Emma", 21, 90.5};
    students[2] = {"Frank", 19, 88.5};

    cout << "学生列表：" << endl;
    for (int i = 0; i < numStudents; i++) {
        cout << (i + 1) << ". ";
        printStudent(&students[i]);
    }

    // 查找最高分
    double maxScore = students[0].score;
    int maxIndex = 0;
    for (int i = 1; i < numStudents; i++) {
        if (students[i].score > maxScore) {
            maxScore = students[i].score;
            maxIndex = i;
        }
    }
    cout << "最高分学生：" << students[maxIndex].name
         << " (" << maxScore << "分)" << endl;

    delete[] students;
    students = nullptr;
    cout << "✓ 学生数组已释放" << endl;
    cout << endl;

    // ========================================
    // 练习4：动态二维数组（矩阵）
    // ========================================
    cout << "【练习4：动态二维数组（矩阵）】" << endl;

    int rows = 3, cols = 4;
    int** matrix = createMatrix(rows, cols);
    fillMatrix(matrix, rows, cols);

    cout << "矩阵 (" << rows << "x" << cols << ")：" << endl;
    printMatrix(matrix, rows, cols);

    // 计算每行总和
    cout << "每行总和：" << endl;
    for (int i = 0; i < rows; i++) {
        int rowSum = 0;
        for (int j = 0; j < cols; j++) {
            rowSum += matrix[i][j];
        }
        cout << "第" << (i + 1) << "行：" << rowSum << endl;
    }

    deleteMatrix(matrix, rows);
    matrix = nullptr;
    cout << "✓ 矩阵已释放" << endl;
    cout << endl;

    // ========================================
    // 练习5：动态字符串
    // ========================================
    cout << "【练习5：动态字符串】" << endl;

    const char* source = "Hello, Dynamic Memory!";
    int len = strlen(source);

    char* dynamicStr = new char[len + 1];  // +1 for '\0'
    strcpy(dynamicStr, source);

    cout << "动态字符串：" << dynamicStr << endl;
    cout << "长度：" << strlen(dynamicStr) << endl;

    // 修改字符串
    dynamicStr[0] = 'h';
    cout << "修改后：" << dynamicStr << endl;

    delete[] dynamicStr;
    dynamicStr = nullptr;
    cout << "✓ 字符串已释放" << endl;
    cout << endl;

    // ========================================
    // 练习6：动态扩展数组
    // ========================================
    cout << "【练习6：动态扩展数组】" << endl;

    int capacity = 3;
    int count = 0;
    int* data = new int[capacity];

    cout << "初始容量：" << capacity << endl;

    // 添加元素
    for (int i = 0; i < 5; i++) {
        if (count >= capacity) {
            // 扩容
            int newCapacity = capacity * 2;
            int* newData = new int[newCapacity];

            // 复制旧数据
            for (int j = 0; j < count; j++) {
                newData[j] = data[j];
            }

            // 释放旧数组
            delete[] data;

            // 更新指针和容量
            data = newData;
            capacity = newCapacity;

            cout << "扩容到：" << capacity << endl;
        }

        data[count++] = (i + 1) * 10;
    }

    cout << "最终元素：";
    for (int i = 0; i < count; i++) {
        cout << data[i] << " ";
    }
    cout << endl;
    cout << "最终容量：" << capacity << endl;

    delete[] data;
    data = nullptr;
    cout << "✓ 数组已释放" << endl;
    cout << endl;

    // ========================================
    // 练习7：内存管理最佳实践
    // ========================================
    cout << "【练习7：内存管理最佳实践】" << endl;

    cout << "✓ 最佳实践：" << endl;
    cout << "  1. 每个new必须对应一个delete" << endl;
    cout << "  2. new[]必须对应delete[]" << endl;
    cout << "  3. delete后立即设为nullptr" << endl;
    cout << "  4. 使用前检查指针是否为nullptr" << endl;
    cout << "  5. 避免内存泄漏：函数返回前释放" << endl;
    cout << "  6. 避免悬空指针：delete后不再使用" << endl;
    cout << "  7. 避免重复释放：只delete一次" << endl;
    cout << "  8. 使用智能指针（C++11后推荐）" << endl;
    cout << endl;

    // ========================================
    // 练习8：常见错误示例
    // ========================================
    cout << "【练习8：常见错误示例】" << endl;

    cout << "❌ 错误1：忘记释放" << endl;
    cout << "  int* p = new int(10);" << endl;
    cout << "  // 忘记 delete p;" << endl;
    cout << "  后果：内存泄漏" << endl;
    cout << endl;

    cout << "❌ 错误2：new/delete不匹配" << endl;
    cout << "  int* arr = new int[5];" << endl;
    cout << "  delete arr;  // 应该用 delete[]" << endl;
    cout << "  后果：未定义行为" << endl;
    cout << endl;

    cout << "❌ 错误3：重复释放" << endl;
    cout << "  int* p = new int(10);" << endl;
    cout << "  delete p;" << endl;
    cout << "  delete p;  // 重复释放" << endl;
    cout << "  后果：程序崩溃" << endl;
    cout << endl;

    cout << "❌ 错误4：使用已释放的内存" << endl;
    cout << "  int* p = new int(10);" << endl;
    cout << "  delete p;" << endl;
    cout << "  *p = 20;  // 悬空指针" << endl;
    cout << "  后果：未定义行为" << endl;
    cout << endl;

    // ========================================
    // 练习9：RAII思想预告
    // ========================================
    cout << "【练习9：RAII思想预告】" << endl;

    cout << "RAII = Resource Acquisition Is Initialization" << endl;
    cout << "资源获取即初始化" << endl;
    cout << endl;

    cout << "核心思想：" << endl;
    cout << "  ✓ 在构造函数中获取资源" << endl;
    cout << "  ✓ 在析构函数中释放资源" << endl;
    cout << "  ✓ 利用对象生命周期自动管理资源" << endl;
    cout << endl;

    cout << "C++11智能指针（后续学习）：" << endl;
    cout << "  unique_ptr - 独占所有权" << endl;
    cout << "  shared_ptr - 共享所有权" << endl;
    cout << "  weak_ptr - 弱引用" << endl;
    cout << "  自动管理内存，不需要手动delete" << endl;
    cout << endl;

    // ========================================
    // 练习10：总结
    // ========================================
    cout << "【练习10：总结】" << endl;

    cout << "动态内存管理的关键：" << endl;
    cout << "  1. 理解栈和堆的区别" << endl;
    cout << "  2. 掌握new/delete的正确使用" << endl;
    cout << "  3. 警惕内存泄漏和悬空指针" << endl;
    cout << "  4. 遵循最佳实践" << endl;
    cout << "  5. 后续学习智能指针" << endl;
    cout << endl;

    cout << "⚠️ 记住：" << endl;
    cout << "  动态内存是强大的工具，但也是危险的" << endl;
    cout << "  必须谨慎使用，严格管理" << endl;
    cout << "  现代C++推荐使用智能指针" << endl;
    cout << endl;

    return 0;
}

// ========================================
// 函数实现
// ========================================

int* createArray(int size) {
    return new int[size]();  // 值初始化为0
}

void fillArray(int* arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = (i + 1) * 10;
    }
}

void printArray(const int* arr, int size) {
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

Student* createStudent(const string& name, int age, double score) {
    Student* s = new Student;
    s->name = name;
    s->age = age;
    s->score = score;
    return s;
}

void printStudent(const Student* s) {
    cout << "姓名：" << s->name
         << ", 年龄：" << s->age
         << ", 成绩：" << s->score << endl;
}

int** createMatrix(int rows, int cols) {
    int** matrix = new int*[rows];
    for (int i = 0; i < rows; i++) {
        matrix[i] = new int[cols];
    }
    return matrix;
}

void fillMatrix(int** matrix, int rows, int cols) {
    int value = 1;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = value++;
        }
    }
}

void printMatrix(int** matrix, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cout << matrix[i][j] << "\t";
        }
        cout << endl;
    }
}

void deleteMatrix(int** matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        delete[] matrix[i];
    }
    delete[] matrix;
}
