// ============================================
// Student.h - 学生类头文件
// 功能：定义学生类，存储学生基本信息和课程成绩
// ============================================

#ifndef STUDENT_H
#define STUDENT_H

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include "Course.h"
using namespace std;

/*
【Student 类设计】

职责：
- 存储学生的基本信息（学号、姓名、年龄、专业）
- 管理学生的课程和成绩
- 提供访问和修改学生信息的接口
- 提供显示学生信息的方法
- 提供成绩统计功能（平均分）

设计原则：
- 封装：成员变量私有，通过公有方法访问
- 单一职责：只负责学生信息和课程的存储和访问
*/

class Student {
private:
    string id;          // 学号（唯一标识）
    string name;        // 姓名
    int age;            // 年龄
    string major;       // 专业
    vector<Course> courses;  // 课程列表（新增）

public:
    // --------------------------------------------
    // 构造函数
    // --------------------------------------------

    // 默认构造函数
    Student();

    // 带参数的构造函数
    // 参数：学号、姓名、年龄、专业
    Student(const string& id, const string& name, int age, const string& major);

    // --------------------------------------------
    // Getter 方法（const 成员函数，不修改对象）
    // --------------------------------------------

    string getId() const;       // 获取学号
    string getName() const;     // 获取姓名
    int getAge() const;         // 获取年龄
    string getMajor() const;    // 获取专业

    // --------------------------------------------
    // Setter 方法（修改学生信息）
    // --------------------------------------------

    void setName(const string& name);       // 修改姓名
    void setAge(int age);                   // 修改年龄
    void setMajor(const string& major);     // 修改专业

    // 注意：学号不提供 setter，因为学号是唯一标识，不应该修改

    // --------------------------------------------
    // 课程管理方法（新增）
    // --------------------------------------------

    // 添加课程
    // 参数：course - 要添加的课程
    // 返回：true 成功，false 失败（课程代码重复）
    bool addCourse(const Course& course);

    // 删除课程
    // 参数：courseCode - 课程代码
    // 返回：true 成功，false 失败（课程不存在）
    bool removeCourse(const string& courseCode);

    // 查找课程（非 const 版本，可修改）
    // 参数：courseCode - 课程代码
    // 返回：指向课程的指针，nullptr 表示未找到
    Course* findCourse(const string& courseCode);

    // 查找课程（const 版本，只读）
    const Course* findCourse(const string& courseCode) const;

    // 显示所有课程
    void displayCourses() const;

    // 获取课程数量
    int getCourseCount() const;

    // 计算平均分
    // 返回：平均分（如果没有课程，返回0.0）
    double getAverageScore() const;

    // 获取总学分
    int getTotalCredits() const;

    // 获取所有课程（用于数据持久化）
    // 返回：课程列表的常量引用
    const vector<Course>& getCourses() const;

    // --------------------------------------------
    // 显示信息
    // --------------------------------------------

    // 显示学生详细信息
    void display() const;

    // 显示学生简要信息（一行）
    void displayBrief() const;
};

#endif // STUDENT_H
