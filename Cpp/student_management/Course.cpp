// ============================================
// Course.cpp - 课程类实现文件
// 功能：实现课程类的所有方法
// ============================================

#include "Course.h"

// --------------------------------------------
// 构造函数实现
// --------------------------------------------

// 默认构造函数：初始化为空值
Course::Course()
    : courseCode(""), courseName(""), credits(0), score(0.0) {
}

// 带参数的构造函数：使用成员初始化列表
Course::Course(const string& code, const string& name, int credits, double score)
    : courseCode(code), courseName(name), credits(credits), score(score) {
    // 注意：数据验证应该在调用方（StudentManager::addGrade）中进行
    // 这里不做验证，假设传入的数据已经验证过
}

// --------------------------------------------
// Getter 方法实现
// --------------------------------------------

string Course::getCourseCode() const {
    return courseCode;
}

string Course::getCourseName() const {
    return courseName;
}

int Course::getCredits() const {
    return credits;
}

double Course::getScore() const {
    return score;
}

// --------------------------------------------
// Setter 方法实现
// --------------------------------------------

// 设置成绩（带验证）
bool Course::setScore(double score) {
    // 验证成绩范围
    if (score < 0 || score > 100) {
        cout << "错误：成绩必须在0-100之间！" << endl;
        return false;
    }

    this->score = score;
    return true;
}

// --------------------------------------------
// 显示方法实现
// --------------------------------------------

// 显示详细信息（多行格式）
void Course::display() const {
    cout << "课程代码：" << courseCode << endl;
    cout << "课程名称：" << courseName << endl;
    cout << "学分：" << credits << endl;
    cout << "成绩：" << fixed << setprecision(1) << score << endl;
    cout << "等级：" << getGrade() << endl;
    cout << "状态：" << (isPassed() ? "及格" : "不及格") << endl;
}

// 显示简要信息（一行格式，用于列表显示）
void Course::displayBrief() const {
    // 使用 setw 对齐输出
    cout << left                                    // 左对齐
         << setw(12) << courseCode                  // 课程代码占12个字符宽度
         << setw(20) << courseName                  // 课程名称占20个字符宽度
         << setw(8) << credits                      // 学分占8个字符宽度
         << setw(10) << fixed << setprecision(1) << score  // 成绩占10个字符宽度，保留1位小数
         << setw(8) << getGrade()                   // 等级占8个字符宽度
         << setw(10) << (isPassed() ? "及格" : "不及格")  // 状态占10个字符宽度
         << endl;
}

// --------------------------------------------
// 辅助方法实现
// --------------------------------------------

// 判断是否及格（成绩 >= 60）
bool Course::isPassed() const {
    return score >= 60.0;
}

// 获取等级（A/B/C/D/F）
char Course::getGrade() const {
    if (score >= 90) {
        return 'A';  // 优秀
    } else if (score >= 80) {
        return 'B';  // 良好
    } else if (score >= 70) {
        return 'C';  // 中等
    } else if (score >= 60) {
        return 'D';  // 及格
    } else {
        return 'F';  // 不及格
    }
}
