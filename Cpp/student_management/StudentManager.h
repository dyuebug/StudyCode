// ============================================
// StudentManager.h - 学生管理器类头文件
// 功能：管理所有学生，提供增删改查和成绩统计功能
// ============================================

#ifndef STUDENT_MANAGER_H
#define STUDENT_MANAGER_H

#include "Student.h"
#include <vector>
#include <string>
#include <functional>
#include <utility>
using namespace std;

/*
【StudentManager 类设计】

职责：
- 管理所有学生对象（使用 vector 存储）
- 提供增删改查功能
- 提供成绩管理功能
- 提供统计和查询功能

设计原则：
- 单一职责：只负责学生的管理
- 使用 STL 容器：vector 存储学生
- 使用 STL 算法：find_if 查找学生、sort 排序
- 使用 Lambda 表达式：简化查找和过滤逻辑
*/

class StudentManager {
private:
    vector<Student> students;   // 存储所有学生

    // 私有辅助方法：根据学号查找学生（返回迭代器）
    vector<Student>::iterator findById(const string& id);
    vector<Student>::const_iterator findById(const string& id) const;

    // 数据验证方法（新增）
    bool isValidStudentId(const string& id) const;    // 验证学号格式
    bool isValidAge(int age) const;                   // 验证年龄范围
    bool isValidScore(double score) const;            // 验证成绩范围
    bool isValidCredits(int credits) const;           // 验证学分范围

public:
    // --------------------------------------------
    // 构造函数
    // --------------------------------------------
    StudentManager();

    // --------------------------------------------
    // 增删改查功能
    // --------------------------------------------

    // 添加学生
    // 返回：true 成功，false 失败（学号重复）
    bool addStudent(const Student& student);

    // 删除学生（根据学号）
    // 返回：true 成功，false 失败（学生不存在）
    bool removeStudent(const string& id);

    // 修改学生信息
    // 返回：指向学生的指针（用于修改），nullptr 表示学生不存在
    Student* findStudentById(const string& id);

    // 查询学生（根据学号）- const 版本
    const Student* findStudentById(const string& id) const;

    // 查询学生（根据姓名）
    // 返回：符合条件的学生列表（可能有多个同名学生）
    vector<Student> findStudentsByName(const string& name) const;

    // --------------------------------------------
    // 显示功能
    // --------------------------------------------

    // 显示所有学生
    void displayAll() const;

    // 显示学生总数
    int getStudentCount() const;

    // 检查是否为空
    bool isEmpty() const;

    // --------------------------------------------
    // 清空功能
    // --------------------------------------------

    // 清空所有学生
    void clear();

    // --------------------------------------------
    // 成绩管理功能（新增）
    // --------------------------------------------

    // 为学生添加成绩
    // 参数：studentId - 学号，course - 课程对象
    // 返回：true 成功，false 失败（学生不存在）
    bool addGrade(const string& studentId, const Course& course);

    // 查询学生成绩
    // 参数：studentId - 学号
    void displayStudentGrades(const string& studentId) const;

    // --------------------------------------------
    // 统计功能（新增）
    // --------------------------------------------

    // 计算班级平均分
    // 返回：班级平均分（所有学生的平均分的平均值）
    double getClassAverage() const;

    // 获取成绩排名
    // 返回：pair<学号, 平均分> 的列表，按平均分降序排列
    vector<pair<string, double>> getRanking() const;

    // 查找优秀学生
    // 参数：threshold - 平均分阈值（默认90分）
    // 返回：平均分 >= threshold 的学生列表
    vector<Student> findExcellentStudents(double threshold = 90.0) const;

    // --------------------------------------------
    // 高级查询功能（新增）
    // --------------------------------------------

    // 按条件过滤学生
    // 参数：predicate - Lambda 表达式，返回 bool 的函数对象
    // 返回：符合条件的学生列表
    // 示例：filterStudents([](const Student& s) { return s.getAge() >= 20; })
    vector<Student> filterStudents(function<bool(const Student&)> predicate) const;

    // --------------------------------------------
    // 数据持久化功能（新增）
    // --------------------------------------------

    // 保存数据到文件
    // 参数：filename - 文件名
    // 返回：true 成功，false 失败
    bool saveToFile(const string& filename) const;

    // 从文件加载数据
    // 参数：filename - 文件名
    // 返回：true 成功，false 失败
    bool loadFromFile(const string& filename);

    // 自动备份
    // 返回：true 成功，false 失败
    bool autoBackup();

    // 批量导入学生（从CSV文件）
    // 参数：filename - CSV文件名
    // 返回：true 成功，false 失败
    bool importFromCSV(const string& filename);
};

#endif // STUDENT_MANAGER_H
