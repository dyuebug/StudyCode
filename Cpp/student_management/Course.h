// ============================================
// Course.h - 课程类头文件
// 功能：封装课程信息，包括课程代码、名称、学分和成绩
// ============================================

#ifndef COURSE_H
#define COURSE_H

#include <string>
#include <iostream>
#include <iomanip>
using namespace std;

/*
【Course 类设计】

职责：
- 存储单门课程的信息
- 提供课程信息的访问和修改
- 提供课程信息的显示

设计原则：
- 单一职责：只负责课程信息的管理
- 封装：成员变量私有化
- 提供清晰的接口
*/

class Course {
private:
    string courseCode;    // 课程代码（如：CS101、MATH201）
    string courseName;    // 课程名称（如：数据结构、高等数学）
    int credits;          // 学分（1-5）
    double score;         // 成绩（0-100）

public:
    // --------------------------------------------
    // 构造函数
    // --------------------------------------------

    // 默认构造函数
    Course();

    // 带参数的构造函数
    // 参数：
    //   code - 课程代码
    //   name - 课程名称
    //   credits - 学分（1-5）
    //   score - 成绩（0-100）
    Course(const string& code, const string& name, int credits, double score);

    // --------------------------------------------
    // Getter 方法（const 成员函数，不修改对象）
    // --------------------------------------------

    // 获取课程代码
    string getCourseCode() const;

    // 获取课程名称
    string getCourseName() const;

    // 获取学分
    int getCredits() const;

    // 获取成绩
    double getScore() const;

    // --------------------------------------------
    // Setter 方法
    // --------------------------------------------

    // 设置成绩（带验证）
    // 参数：score - 成绩（0-100）
    // 返回：true 成功，false 失败（成绩无效）
    bool setScore(double score);

    // --------------------------------------------
    // 显示方法
    // --------------------------------------------

    // 显示详细信息（多行格式）
    void display() const;

    // 显示简要信息（一行格式，用于列表显示）
    void displayBrief() const;

    // --------------------------------------------
    // 辅助方法
    // --------------------------------------------

    // 判断是否及格（成绩 >= 60）
    bool isPassed() const;

    // 获取等级（A/B/C/D/F）
    char getGrade() const;
};

#endif // COURSE_H
