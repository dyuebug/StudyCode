// ============================================
// Student.cpp - 学生类实现文件
// 功能：实现学生类的所有方法
// ============================================

#include "Student.h"
#include <iomanip>

// --------------------------------------------
// 构造函数实现
// --------------------------------------------

// 默认构造函数：初始化为空值
Student::Student()
    : id(""), name(""), age(0), major("") {
}

// 带参数的构造函数：使用成员初始化列表
Student::Student(const string& id, const string& name, int age, const string& major)
    : id(id), name(name), age(age), major(major) {
}

// --------------------------------------------
// Getter 方法实现
// --------------------------------------------

string Student::getId() const {
    return id;
}

string Student::getName() const {
    return name;
}

int Student::getAge() const {
    return age;
}

string Student::getMajor() const {
    return major;
}

// --------------------------------------------
// Setter 方法实现
// --------------------------------------------

void Student::setName(const string& name) {
    this->name = name;
}

void Student::setAge(int age) {
    // 简单的年龄验证
    if (age > 0 && age < 150) {
        this->age = age;
    } else {
        cout << "警告：年龄无效，未修改" << endl;
    }
}

void Student::setMajor(const string& major) {
    this->major = major;
}

// --------------------------------------------
// 课程管理方法实现（新增）
// --------------------------------------------

// 添加课程
bool Student::addCourse(const Course& course) {
    // 检查课程代码是否已存在
    if (findCourse(course.getCourseCode()) != nullptr) {
        cout << "错误：课程代码 " << course.getCourseCode() << " 已存在！" << endl;
        return false;
    }

    // 添加课程到 vector
    courses.push_back(course);
    cout << "成功为学生 " << name << " 添加课程：" << course.getCourseName() << endl;
    return true;
}

// 删除课程
bool Student::removeCourse(const string& courseCode) {
    // 使用 find_if + Lambda 查找课程
    auto it = find_if(courses.begin(), courses.end(),
        [&courseCode](const Course& c) {
            return c.getCourseCode() == courseCode;
        });

    // 检查是否找到
    if (it == courses.end()) {
        cout << "错误：课程代码 " << courseCode << " 不存在！" << endl;
        return false;
    }

    // 删除课程
    string courseName = it->getCourseName();  // 保存课程名用于提示
    courses.erase(it);
    cout << "成功删除课程：" << courseName << endl;
    return true;
}

// 查找课程（非 const 版本）
Course* Student::findCourse(const string& courseCode) {
    auto it = find_if(courses.begin(), courses.end(),
        [&courseCode](const Course& c) {
            return c.getCourseCode() == courseCode;
        });

    if (it != courses.end()) {
        return &(*it);  // 返回指向课程的指针
    }
    return nullptr;
}

// 查找课程（const 版本）
const Course* Student::findCourse(const string& courseCode) const {
    auto it = find_if(courses.begin(), courses.end(),
        [&courseCode](const Course& c) {
            return c.getCourseCode() == courseCode;
        });

    if (it != courses.end()) {
        return &(*it);
    }
    return nullptr;
}

// 显示所有课程
void Student::displayCourses() const {
    if (courses.empty()) {
        cout << "该学生暂无课程成绩。" << endl;
        return;
    }

    cout << string(80, '=') << endl;
    cout << "学生：" << name << "（" << id << "）的课程成绩" << endl;
    cout << string(80, '=') << endl;

    // 显示表头
    cout << left
         << setw(12) << "课程代码"
         << setw(20) << "课程名称"
         << setw(8) << "学分"
         << setw(10) << "成绩"
         << setw(8) << "等级"
         << setw(10) << "状态"
         << endl;
    cout << string(80, '-') << endl;

    // 显示所有课程
    for (const auto& course : courses) {
        course.displayBrief();
    }

    cout << string(80, '-') << endl;
    cout << "课程总数：" << courses.size() << endl;
    cout << "总学分：" << getTotalCredits() << endl;
    cout << "平均分：" << fixed << setprecision(2) << getAverageScore() << endl;
}

// 获取课程数量
int Student::getCourseCount() const {
    return courses.size();
}

// 计算平均分
double Student::getAverageScore() const {
    // 如果没有课程，返回0.0
    if (courses.empty()) {
        return 0.0;
    }

    // 累加所有课程成绩
    double sum = 0.0;
    for (const auto& course : courses) {
        sum += course.getScore();
    }

    // 返回平均分
    return sum / courses.size();
}

// 获取总学分
int Student::getTotalCredits() const {
    int total = 0;
    for (const auto& course : courses) {
        total += course.getCredits();
    }
    return total;
}

// 获取所有课程（用于数据持久化）
const vector<Course>& Student::getCourses() const {
    return courses;
}

// --------------------------------------------
// 显示信息方法实现
// --------------------------------------------

// 显示详细信息（多行格式）
void Student::display() const {
    cout << "学号：" << id << endl;
    cout << "姓名：" << name << endl;
    cout << "年龄：" << age << endl;
    cout << "专业：" << major << endl;
}

// 显示简要信息（一行格式，用于列表显示）
void Student::displayBrief() const {
    // 使用 setw 对齐输出
    cout << left                        // 左对齐
         << setw(12) << id              // 学号占12个字符宽度
         << setw(10) << name            // 姓名占10个字符宽度
         << setw(6) << age              // 年龄占6个字符宽度
         << setw(20) << major           // 专业占20个字符宽度
         << endl;
}
