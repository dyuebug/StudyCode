// ============================================
// StudentManager.cpp - 学生管理器类实现文件
// 功能：实现学生管理器的所有方法
// ============================================

#include "StudentManager.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>
#include <cctype>
using namespace std;

// --------------------------------------------
// 构造函数实现
// --------------------------------------------

StudentManager::StudentManager() {
    // vector 会自动初始化为空
}

// --------------------------------------------
// 私有辅助方法：根据学号查找学生
// --------------------------------------------

// 非 const 版本：返回可修改的迭代器
vector<Student>::iterator StudentManager::findById(const string& id) {
    // 使用 STL 算法 find_if + Lambda 表达式查找
    return find_if(students.begin(), students.end(),
        [&id](const Student& s) {
            return s.getId() == id;
        });
}

// const 版本：返回只读迭代器
vector<Student>::const_iterator StudentManager::findById(const string& id) const {
    return find_if(students.begin(), students.end(),
        [&id](const Student& s) {
            return s.getId() == id;
        });
}

// --------------------------------------------
// 数据验证方法实现（新增）
// --------------------------------------------

// 验证学号格式
bool StudentManager::isValidStudentId(const string& id) const {
    // 检查长度（必须是7位）
    if (id.length() != 7) {
        return false;
    }

    // 检查是否全是数字
    for (char c : id) {
        if (!isdigit(c)) {
            return false;
        }
    }

    return true;
}

// 验证年龄范围
bool StudentManager::isValidAge(int age) const {
    // 年龄必须在15-30之间
    return age >= 15 && age <= 30;
}

// 验证成绩范围
bool StudentManager::isValidScore(double score) const {
    // 成绩必须在0-100之间
    return score >= 0.0 && score <= 100.0;
}

// 验证学分范围
bool StudentManager::isValidCredits(int credits) const {
    // 学分必须在1-5之间
    return credits >= 1 && credits <= 5;
}

// --------------------------------------------
// 增删改查功能实现
// --------------------------------------------

// 添加学生
bool StudentManager::addStudent(const Student& student) {
    // 验证学号格式
    if (!isValidStudentId(student.getId())) {
        cout << "错误：学号格式无效！学号必须是7位数字（如：2021001）" << endl;
        return false;
    }

    // 验证年龄范围
    if (!isValidAge(student.getAge())) {
        cout << "错误：年龄无效！年龄必须在15-30岁之间" << endl;
        return false;
    }

    // 检查学号是否已存在
    if (findById(student.getId()) != students.end()) {
        cout << "错误：学号 " << student.getId() << " 已存在！" << endl;
        return false;
    }

    // 添加学生到 vector
    students.push_back(student);
    cout << "成功添加学生：" << student.getName() << endl;
    return true;
}

// 删除学生
bool StudentManager::removeStudent(const string& id) {
    // 查找学生
    auto it = findById(id);

    // 检查是否找到
    if (it == students.end()) {
        cout << "错误：学号 " << id << " 不存在！" << endl;
        return false;
    }

    // 删除学生
    string name = it->getName();  // 保存姓名用于提示
    students.erase(it);
    cout << "成功删除学生：" << name << endl;
    return true;
}

// 查询学生（根据学号）- 非 const 版本
Student* StudentManager::findStudentById(const string& id) {
    auto it = findById(id);
    if (it != students.end()) {
        return &(*it);  // 返回指向学生的指针
    }
    return nullptr;
}

// 查询学生（根据学号）- const 版本
const Student* StudentManager::findStudentById(const string& id) const {
    auto it = findById(id);
    if (it != students.end()) {
        return &(*it);
    }
    return nullptr;
}

// 查询学生（根据姓名）
vector<Student> StudentManager::findStudentsByName(const string& name) const {
    vector<Student> result;

    // 使用范围 for 循环遍历所有学生
    for (const auto& student : students) {
        if (student.getName() == name) {
            result.push_back(student);
        }
    }

    return result;
}

// --------------------------------------------
// 显示功能实现
// --------------------------------------------

// 显示所有学生
void StudentManager::displayAll() const {
    if (students.empty()) {
        cout << "当前没有学生信息。" << endl;
        return;
    }

    cout << string(60, '=') << endl;
    cout << "所有学生信息" << endl;
    cout << string(60, '=') << endl;

    // 显示表头
    cout << left
         << setw(12) << "学号"
         << setw(10) << "姓名"
         << setw(6) << "年龄"
         << setw(20) << "专业"
         << endl;
    cout << string(60, '-') << endl;

    // 显示所有学生（使用范围 for 循环）
    for (const auto& student : students) {
        student.displayBrief();
    }

    cout << string(60, '-') << endl;
    cout << "学生总数：" << students.size() << endl;
}

// 获取学生总数
int StudentManager::getStudentCount() const {
    return students.size();
}

// 检查是否为空
bool StudentManager::isEmpty() const {
    return students.empty();
}

// --------------------------------------------
// 清空功能实现
// --------------------------------------------

// 清空所有学生
void StudentManager::clear() {
    students.clear();
    cout << "已清空所有学生信息。" << endl;
}

// --------------------------------------------
// 成绩管理功能实现（新增）
// --------------------------------------------

// 为学生添加成绩
bool StudentManager::addGrade(const string& studentId, const Course& course) {
    // 验证学分范围
    if (!isValidCredits(course.getCredits())) {
        cout << "错误：学分无效！学分必须在1-5之间" << endl;
        return false;
    }

    // 验证成绩范围
    if (!isValidScore(course.getScore())) {
        cout << "错误：成绩无效！成绩必须在0-100之间" << endl;
        return false;
    }

    // 查找学生
    Student* student = findStudentById(studentId);

    if (student == nullptr) {
        cout << "错误：学号 " << studentId << " 不存在！" << endl;
        return false;
    }

    // 为学生添加课程
    return student->addCourse(course);
}

// 查询学生成绩
void StudentManager::displayStudentGrades(const string& studentId) const {
    // 查找学生
    const Student* student = findStudentById(studentId);

    if (student == nullptr) {
        cout << "错误：学号 " << studentId << " 不存在！" << endl;
        return;
    }

    // 显示学生的所有课程
    student->displayCourses();
}

// --------------------------------------------
// 统计功能实现（新增）
// --------------------------------------------

// 计算班级平均分
double StudentManager::getClassAverage() const {
    if (students.empty()) {
        return 0.0;
    }

    // 累加所有学生的平均分
    double sum = 0.0;
    int count = 0;

    for (const auto& student : students) {
        // 只统计有成绩的学生
        if (student.getCourseCount() > 0) {
            sum += student.getAverageScore();
            count++;
        }
    }

    // 如果没有学生有成绩，返回0.0
    if (count == 0) {
        return 0.0;
    }

    return sum / count;
}

// 获取成绩排名
vector<pair<string, double>> StudentManager::getRanking() const {
    vector<pair<string, double>> ranking;

    // 收集所有学生的学号和平均分
    for (const auto& student : students) {
        // 只统计有成绩的学生
        if (student.getCourseCount() > 0) {
            ranking.push_back({student.getId(), student.getAverageScore()});
        }
    }

    // 按平均分降序排序（使用 Lambda 表达式）
    sort(ranking.begin(), ranking.end(),
        [](const pair<string, double>& a, const pair<string, double>& b) {
            return a.second > b.second;  // 降序：a.second > b.second
        });

    return ranking;
}

// 查找优秀学生
vector<Student> StudentManager::findExcellentStudents(double threshold) const {
    vector<Student> result;

    // 遍历所有学生
    for (const auto& student : students) {
        // 筛选平均分 >= threshold 的学生
        if (student.getCourseCount() > 0 && student.getAverageScore() >= threshold) {
            result.push_back(student);
        }
    }

    return result;
}

// --------------------------------------------
// 高级查询功能实现（新增）
// --------------------------------------------

// 按条件过滤学生
vector<Student> StudentManager::filterStudents(function<bool(const Student&)> predicate) const {
    vector<Student> result;

    // 遍历所有学生，应用过滤条件
    for (const auto& student : students) {
        if (predicate(student)) {
            result.push_back(student);
        }
    }

    return result;
}

// --------------------------------------------
// 数据持久化功能实现（新增）
// --------------------------------------------

// 保存数据到文件
bool StudentManager::saveToFile(const string& filename) const {
    // 使用 ofstream 打开文件（写模式）
    ofstream file(filename);

    // 检查文件是否成功打开
    if (!file.is_open()) {
        cout << "错误：无法打开文件 " << filename << " 进行写入！" << endl;
        return false;
    }

    // 遍历所有学生，保存数据
    for (const auto& student : students) {
        // 写入学生标记
        file << "[STUDENT]" << endl;

        // 写入学生基本信息（使用 | 作为分隔符）
        file << student.getId() << "|"
             << student.getName() << "|"
             << student.getAge() << "|"
             << student.getMajor() << endl;

        // 写入课程标记
        file << "[COURSES]" << endl;

        // 获取学生的所有课程
        const vector<Course>& courses = student.getCourses();

        // 遍历并保存每门课程
        for (const auto& course : courses) {
            file << course.getCourseCode() << "|"
                 << course.getCourseName() << "|"
                 << course.getCredits() << "|"
                 << course.getScore() << endl;
        }

        // 写入课程结束标记
        file << "[END]" << endl;
    }

    // 关闭文件
    file.close();

    cout << "数据保存成功！文件：" << filename << endl;
    return true;
}

// 从文件加载数据
bool StudentManager::loadFromFile(const string& filename) {
    // 使用 ifstream 打开文件（读模式）
    ifstream file(filename);

    // 检查文件是否存在
    if (!file.is_open()) {
        cout << "错误：无法打开文件 " << filename << " 进行读取！" << endl;
        return false;
    }

    // 清空当前学生列表
    students.clear();

    string line;

    // 逐行读取文件
    while (getline(file, line)) {
        // 检查是否是学生标记
        if (line == "[STUDENT]") {
            // 读取学生基本信息
            if (!getline(file, line)) {
                cout << "错误：文件格式错误，缺少学生信息！" << endl;
                file.close();
                return false;
            }

            // 解析学生信息（使用 stringstream 分割字符串）
            stringstream ss(line);
            string id, name, ageStr, major;

            // 使用 | 作为分隔符读取
            getline(ss, id, '|');
            getline(ss, name, '|');
            getline(ss, ageStr, '|');
            getline(ss, major, '|');

            // 验证数据完整性
            if (id.empty() || name.empty() || ageStr.empty() || major.empty()) {
                cout << "错误：学生信息不完整！" << endl;
                file.close();
                return false;
            }

            // 转换年龄（字符串 -> 整数）
            int age;
            try {
                age = stoi(ageStr);
            } catch (const exception& e) {
                cout << "错误：年龄格式错误！" << endl;
                file.close();
                return false;
            }

            // 创建学生对象
            Student student(id, name, age, major);

            // 读取课程标记
            if (!getline(file, line) || line != "[COURSES]") {
                cout << "错误：文件格式错误，缺少课程标记！" << endl;
                file.close();
                return false;
            }

            // 读取课程信息（直到遇到 [END]）
            while (getline(file, line) && line != "[END]") {
                // 解析课程信息
                stringstream courseSS(line);
                string courseCode, courseName, creditsStr, scoreStr;

                getline(courseSS, courseCode, '|');
                getline(courseSS, courseName, '|');
                getline(courseSS, creditsStr, '|');
                getline(courseSS, scoreStr, '|');

                // 验证课程数据完整性
                if (!courseCode.empty() && !courseName.empty() &&
                    !creditsStr.empty() && !scoreStr.empty()) {
                    try {
                        int credits = stoi(creditsStr);
                        double score = stod(scoreStr);

                        // 创建课程对象并添加到学生
                        Course course(courseCode, courseName, credits, score);
                        student.addCourse(course);
                    } catch (const exception& e) {
                        cout << "警告：课程数据格式错误，跳过该课程。" << endl;
                    }
                }
            }

            // 添加学生到管理器
            students.push_back(student);
        }
    }

    // 关闭文件
    file.close();

    cout << "数据加载成功！共加载 " << students.size() << " 个学生。" << endl;
    return true;
}

// 自动备份
bool StudentManager::autoBackup() {
    // 生成带时间戳的备份文件名
    time_t now = time(nullptr);
    tm* ltm = localtime(&now);

    stringstream ss;
    ss << "backup_"
       << (1900 + ltm->tm_year)
       << setfill('0') << setw(2) << (1 + ltm->tm_mon)
       << setfill('0') << setw(2) << ltm->tm_mday
       << "_"
       << setfill('0') << setw(2) << ltm->tm_hour
       << setfill('0') << setw(2) << ltm->tm_min
       << setfill('0') << setw(2) << ltm->tm_sec
       << ".txt";

    string backupFilename = ss.str();

    // 调用 saveToFile 保存数据
    if (saveToFile(backupFilename)) {
        cout << "自动备份成功！备份文件：" << backupFilename << endl;
        return true;
    } else {
        cout << "自动备份失败！" << endl;
        return false;
    }
}

// 批量导入学生（从CSV文件）
bool StudentManager::importFromCSV(const string& filename) {
    // 使用 ifstream 打开文件
    ifstream file(filename);

    // 检查文件是否存在
    if (!file.is_open()) {
        cout << "错误：无法打开文件 " << filename << " 进行读取！" << endl;
        return false;
    }

    string line;
    int successCount = 0;  // 成功导入的数量
    int failCount = 0;     // 失败的数量
    int lineNumber = 0;    // 行号

    // 逐行读取文件
    while (getline(file, line)) {
        lineNumber++;

        // 跳过空行
        if (line.empty()) {
            continue;
        }

        // 跳过表头（第一行）
        if (lineNumber == 1 && line.find("学号") != string::npos) {
            continue;
        }

        // 解析CSV行（使用逗号分隔）
        stringstream ss(line);
        string id, name, ageStr, major;

        // 使用逗号作为分隔符读取
        getline(ss, id, ',');
        getline(ss, name, ',');
        getline(ss, ageStr, ',');
        getline(ss, major, ',');

        // 验证数据完整性
        if (id.empty() || name.empty() || ageStr.empty() || major.empty()) {
            cout << "警告：第 " << lineNumber << " 行数据不完整，跳过。" << endl;
            failCount++;
            continue;
        }

        // 转换年龄（字符串 -> 整数）
        int age;
        try {
            age = stoi(ageStr);
        } catch (const exception& e) {
            cout << "警告：第 " << lineNumber << " 行年龄格式错误，跳过。" << endl;
            failCount++;
            continue;
        }

        // 创建学生对象并添加
        Student student(id, name, age, major);
        if (addStudent(student)) {
            successCount++;
        } else {
            failCount++;
        }
    }

    // 关闭文件
    file.close();

    // 显示导入统计
    cout << "\n=== 导入完成 ===" << endl;
    cout << "成功导入：" << successCount << " 个学生" << endl;
    cout << "失败/跳过：" << failCount << " 个" << endl;
    cout << "总计：" << (successCount + failCount) << " 行数据" << endl;

    return successCount > 0;
}
