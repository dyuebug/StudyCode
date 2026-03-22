/*
 * ============================================================================
 * 文件名：30_struct_basics.cpp
 * 主题：结构体基础详解
 * 学习目标：
 *   1. 理解结构体的概念（自定义数据类型）
 *   2. 掌握结构体的定义和声明
 *   3. 学会访问结构体成员（.和->）
 *   4. 掌握结构体的初始化方式
 *   5. 学会使用结构体数组和指针
 *
 * 编译命令：g++ -std=c++11 -o 30_struct_basics 30_struct_basics.cpp
 * 运行命令：./30_struct_basics (Linux/Mac) 或 30_struct_basics.exe (Windows)
 * ============================================================================
 */

#include <iostream>
#include <string>
using namespace std;

// 定义学生结构体
struct Student {
    string name;
    int age;
    double score;
};

// 定义点结构体
struct Point {
    int x;
    int y;
};

// 定义矩形结构体（嵌套结构体）
struct Rectangle {
    Point topLeft;
    Point bottomRight;
};

// 函数声明
void printStudent(const Student& s);
void updateScore(Student& s, double newScore);
Student createStudent(const string& name, int age, double score);

int main() {
    cout << "=== 结构体基础详解 ===" << endl << endl;

    // ========================================
    // 1. 什么是结构体？
    // ========================================
    cout << "【1. 什么是结构体？】" << endl;
    cout << "结构体（struct）是用户自定义的数据类型" << endl;
    cout << "作用：将相关的数据组织在一起" << endl;
    cout << endl;

    cout << "为什么需要结构体？" << endl;
    cout << "  ✓ 组织相关数据（如学生的姓名、年龄、成绩）" << endl;
    cout << "  ✓ 提高代码可读性" << endl;
    cout << "  ✓ 便于数据管理" << endl;
    cout << "  ✓ 是面向对象编程的基础" << endl;
    cout << endl;

    // ========================================
    // 2. 结构体的定义
    // ========================================
    cout << "【2. 结构体的定义】" << endl;
    cout << "定义格式：" << endl;
    cout << "  struct 结构体名 {" << endl;
    cout << "      数据类型 成员1;" << endl;
    cout << "      数据类型 成员2;" << endl;
    cout << "      ..." << endl;
    cout << "  };" << endl;
    cout << endl;

    cout << "示例：Student结构体" << endl;
    cout << "  struct Student {" << endl;
    cout << "      string name;   // 姓名" << endl;
    cout << "      int age;       // 年龄" << endl;
    cout << "      double score;  // 成绩" << endl;
    cout << "  };" << endl;
    cout << endl;

    // ========================================
    // 3. 结构体变量的声明和初始化
    // ========================================
    cout << "【3. 结构体变量的声明和初始化】" << endl;

    // 方式1：先声明后赋值
    Student s1;
    s1.name = "Alice";
    s1.age = 20;
    s1.score = 95.5;
    cout << "方式1：先声明后赋值" << endl;
    cout << "  Student s1;" << endl;
    cout << "  s1.name = \"Alice\";" << endl;
    cout << "  s1.age = 20;" << endl;
    cout << "  s1.score = 95.5;" << endl;
    cout << "结果：" << s1.name << ", " << s1.age << "岁, " << s1.score << "分" << endl;
    cout << endl;

    // 方式2：声明时初始化
    Student s2 = {"Bob", 21, 88.0};
    cout << "方式2：声明时初始化" << endl;
    cout << "  Student s2 = {\"Bob\", 21, 88.0};" << endl;
    cout << "结果：" << s2.name << ", " << s2.age << "岁, " << s2.score << "分" << endl;
    cout << endl;

    // 方式3：C++11列表初始化
    Student s3{"Charlie", 19, 92.5};
    cout << "方式3：C++11列表初始化" << endl;
    cout << "  Student s3{\"Charlie\", 19, 92.5};" << endl;
    cout << "结果：" << s3.name << ", " << s3.age << "岁, " << s3.score << "分" << endl;
    cout << endl;

    // 方式4：部分初始化（未指定的成员为默认值）
    Student s4 = {"David"};
    cout << "方式4：部分初始化" << endl;
    cout << "  Student s4 = {\"David\"};" << endl;
    cout << "结果：" << s4.name << ", " << s4.age << "岁, " << s4.score << "分" << endl;
    cout << "未初始化的成员：age=0, score=0" << endl;
    cout << endl;

    // ========================================
    // 4. 访问结构体成员（.运算符）
    // ========================================
    cout << "【4. 访问结构体成员（.运算符）】" << endl;

    Student student;
    student.name = "Emma";
    student.age = 20;
    student.score = 90.0;

    cout << "访问成员：结构体变量.成员名" << endl;
    cout << "  student.name = " << student.name << endl;
    cout << "  student.age = " << student.age << endl;
    cout << "  student.score = " << student.score << endl;
    cout << endl;

    // 修改成员
    student.score = 95.0;
    cout << "修改成员：student.score = 95.0" << endl;
    cout << "修改后：" << student.score << endl;
    cout << endl;

    // ========================================
    // 5. 结构体指针（->运算符）
    // ========================================
    cout << "【5. 结构体指针（->运算符）】" << endl;

    Student s5 = {"Frank", 22, 87.5};
    Student* ptr = &s5;

    cout << "结构体指针：Student* ptr = &s5;" << endl;
    cout << endl;

    cout << "访问方式1：通过指针解引用" << endl;
    cout << "  (*ptr).name = " << (*ptr).name << endl;
    cout << "  (*ptr).age = " << (*ptr).age << endl;
    cout << endl;

    cout << "访问方式2：使用->运算符（推荐）" << endl;
    cout << "  ptr->name = " << ptr->name << endl;
    cout << "  ptr->age = " << ptr->age << endl;
    cout << "  ptr->score = " << ptr->score << endl;
    cout << endl;

    cout << "关键区别：" << endl;
    cout << "  普通结构体变量：用 . 访问成员" << endl;
    cout << "  结构体指针：用 -> 访问成员" << endl;
    cout << endl;

    // 通过指针修改成员
    ptr->score = 90.0;
    cout << "通过指针修改：ptr->score = 90.0" << endl;
    cout << "s5.score = " << s5.score << " (原变量也被修改)" << endl;
    cout << endl;

    // ========================================
    // 6. 结构体数组
    // ========================================
    cout << "【6. 结构体数组】" << endl;

    Student students[3] = {
        {"Grace", 20, 92.0},
        {"Henry", 21, 88.5},
        {"Ivy", 19, 95.0}
    };

    cout << "结构体数组：Student students[3] = {...};" << endl;
    cout << "学生列表：" << endl;
    for (int i = 0; i < 3; i++) {
        cout << (i + 1) << ". " << students[i].name
             << ", " << students[i].age << "岁"
             << ", " << students[i].score << "分" << endl;
    }
    cout << endl;

    // 查找最高分
    double maxScore = students[0].score;
    int maxIndex = 0;
    for (int i = 1; i < 3; i++) {
        if (students[i].score > maxScore) {
            maxScore = students[i].score;
            maxIndex = i;
        }
    }
    cout << "最高分学生：" << students[maxIndex].name
         << " (" << maxScore << "分)" << endl;
    cout << endl;

    // ========================================
    // 7. 结构体作为函数参数
    // ========================================
    cout << "【7. 结构体作为函数参数】" << endl;

    Student s6 = {"Jack", 20, 85.0};

    cout << "原始信息：" << endl;
    printStudent(s6);

    cout << "更新成绩：updateScore(s6, 90.0)" << endl;
    updateScore(s6, 90.0);

    cout << "更新后：" << endl;
    printStudent(s6);
    cout << endl;

    cout << "传递方式：" << endl;
    cout << "  值传递：void func(Student s)  → 拷贝，不影响原变量" << endl;
    cout << "  引用传递：void func(Student& s) → 不拷贝，可修改原变量" << endl;
    cout << "  const引用：void func(const Student& s) → 不拷贝，不可修改" << endl;
    cout << endl;

    // ========================================
    // 8. 结构体作为函数返回值
    // ========================================
    cout << "【8. 结构体作为函数返回值】" << endl;

    Student s7 = createStudent("Kate", 21, 93.0);
    cout << "创建学生：createStudent(\"Kate\", 21, 93.0)" << endl;
    printStudent(s7);
    cout << endl;

    // ========================================
    // 9. 嵌套结构体
    // ========================================
    cout << "【9. 嵌套结构体】" << endl;

    Rectangle rect;
    rect.topLeft = {0, 10};
    rect.bottomRight = {20, 0};

    cout << "矩形：" << endl;
    cout << "  左上角：(" << rect.topLeft.x << ", " << rect.topLeft.y << ")" << endl;
    cout << "  右下角：(" << rect.bottomRight.x << ", " << rect.bottomRight.y << ")" << endl;

    int width = rect.bottomRight.x - rect.topLeft.x;
    int height = rect.topLeft.y - rect.bottomRight.y;
    cout << "  宽度：" << width << endl;
    cout << "  高度：" << height << endl;
    cout << "  面积：" << (width * height) << endl;
    cout << endl;

    // ========================================
    // 10. 结构体的大小
    // ========================================
    cout << "【10. 结构体的大小】" << endl;

    cout << "sizeof(Student) = " << sizeof(Student) << " 字节" << endl;
    cout << "sizeof(Point) = " << sizeof(Point) << " 字节" << endl;
    cout << "sizeof(Rectangle) = " << sizeof(Rectangle) << " 字节" << endl;
    cout << endl;

    cout << "注意：结构体大小可能大于成员大小之和（内存对齐）" << endl;
    cout << endl;

    // ========================================
    // 11. 结构体的优势
    // ========================================
    cout << "【11. 结构体的优势】" << endl;
    cout << "✓ 组织相关数据，提高代码可读性" << endl;
    cout << "✓ 便于数据管理和传递" << endl;
    cout << "✓ 是面向对象编程的基础（类的前身）" << endl;
    cout << "✓ 可以包含不同类型的数据" << endl;
    cout << "✓ 可以嵌套使用" << endl;
    cout << endl;

    // ========================================
    // 12. 结构体 vs 类
    // ========================================
    cout << "【12. 结构体 vs 类（预告）】" << endl;
    cout << "在C++中，struct和class非常相似：" << endl;
    cout << "  struct：默认成员是public" << endl;
    cout << "  class：默认成员是private" << endl;
    cout << "后续会学习类（class），它是结构体的增强版" << endl;
    cout << endl;

    return 0;
}

// 函数实现
void printStudent(const Student& s) {
    cout << "  姓名：" << s.name
         << ", 年龄：" << s.age
         << ", 成绩：" << s.score << endl;
}

void updateScore(Student& s, double newScore) {
    s.score = newScore;
}

Student createStudent(const string& name, int age, double score) {
    Student s;
    s.name = name;
    s.age = age;
    s.score = score;
    return s;
}
