// ============================================
// test_student_manager.cpp - 单元测试
// 功能：测试学生管理系统的核心功能
// ============================================

#include "StudentManager.h"
#include <cassert>
#include <iostream>
#include <fstream>
using namespace std;

// 测试计数器
int testsPassed = 0;
int testsFailed = 0;

// 测试辅助宏
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    cout << "运行测试: " << #name << "..."; \
    try { \
        test_##name(); \
        cout << " ✓ 通过" << endl; \
        testsPassed++; \
    } catch (const exception& e) { \
        cout << " ✗ 失败: " << e.what() << endl; \
        testsFailed++; \
    } \
} while(0)

#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        throw runtime_error("断言失败: " #condition); \
    } \
} while(0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))
#define ASSERT_EQUAL(a, b) ASSERT_TRUE((a) == (b))

// ============================================
// 测试用例
// ============================================

// 测试1：添加学生
TEST(AddStudent) {
    StudentManager manager;
    Student s("2021001", "张三", 20, "计算机");

    ASSERT_TRUE(manager.addStudent(s));
    ASSERT_EQUAL(manager.getStudentCount(), 1);

    // 测试重复学号
    ASSERT_FALSE(manager.addStudent(s));
    ASSERT_EQUAL(manager.getStudentCount(), 1);
}

// 测试2：删除学生
TEST(RemoveStudent) {
    StudentManager manager;
    Student s("2021001", "张三", 20, "计算机");

    manager.addStudent(s);
    ASSERT_EQUAL(manager.getStudentCount(), 1);

    ASSERT_TRUE(manager.removeStudent("2021001"));
    ASSERT_EQUAL(manager.getStudentCount(), 0);

    // 测试删除不存在的学生
    ASSERT_FALSE(manager.removeStudent("2021001"));
}

// 测试3：查找学生
TEST(FindStudent) {
    StudentManager manager;
    Student s("2021001", "张三", 20, "计算机");

    manager.addStudent(s);

    // 测试按学号查找
    const Student* found = manager.findStudentById("2021001");
    ASSERT_TRUE(found != nullptr);
    ASSERT_EQUAL(found->getName(), "张三");

    // 测试查找不存在的学生
    const Student* notFound = manager.findStudentById("9999999");
    ASSERT_TRUE(notFound == nullptr);
}

// 测试4：无效学号验证
TEST(InvalidStudentId) {
    StudentManager manager;

    // 学号长度不对
    Student s1("12345", "测试", 20, "计算机");
    ASSERT_FALSE(manager.addStudent(s1));

    // 学号包含字母
    Student s2("202100A", "测试", 20, "计算机");
    ASSERT_FALSE(manager.addStudent(s2));

    ASSERT_EQUAL(manager.getStudentCount(), 0);
}

// 测试5：无效年龄验证
TEST(InvalidAge) {
    StudentManager manager;

    // 年龄太小
    Student s1("2021001", "测试", 10, "计算机");
    ASSERT_FALSE(manager.addStudent(s1));

    // 年龄太大
    Student s2("2021002", "测试", 35, "计算机");
    ASSERT_FALSE(manager.addStudent(s2));

    // 有效年龄
    Student s3("2021003", "测试", 20, "计算机");
    ASSERT_TRUE(manager.addStudent(s3));

    ASSERT_EQUAL(manager.getStudentCount(), 1);
}

// 测试6：添加成绩
TEST(AddGrade) {
    StudentManager manager;
    Student s("2021001", "张三", 20, "计算机");
    manager.addStudent(s);

    Course c("CS101", "数据结构", 4, 95);
    ASSERT_TRUE(manager.addGrade("2021001", c));

    // 测试为不存在的学生添加成绩
    ASSERT_FALSE(manager.addGrade("9999999", c));
}

// 测试7：无效成绩验证
TEST(InvalidGrade) {
    StudentManager manager;
    Student s("2021001", "张三", 20, "计算机");
    manager.addStudent(s);

    // 学分无效
    Course c1("CS101", "数据结构", 0, 95);
    ASSERT_FALSE(manager.addGrade("2021001", c1));

    Course c2("CS102", "算法", 6, 95);
    ASSERT_FALSE(manager.addGrade("2021001", c2));

    // 成绩无效
    Course c3("CS103", "操作系统", 4, -10);
    ASSERT_FALSE(manager.addGrade("2021001", c3));

    Course c4("CS104", "数据库", 4, 105);
    ASSERT_FALSE(manager.addGrade("2021001", c4));
}

// 测试8：保存和加载数据
TEST(SaveAndLoad) {
    StudentManager manager;
    Student s1("2021001", "张三", 20, "计算机");
    Student s2("2021002", "李四", 21, "软件工程");

    manager.addStudent(s1);
    manager.addStudent(s2);

    Course c1("CS101", "数据结构", 4, 95);
    Course c2("MATH201", "高等数学", 5, 88);
    manager.addGrade("2021001", c1);
    manager.addGrade("2021001", c2);

    // 保存
    ASSERT_TRUE(manager.saveToFile("test_data.txt"));

    // 清空并重新加载
    manager.clear();
    ASSERT_EQUAL(manager.getStudentCount(), 0);

    ASSERT_TRUE(manager.loadFromFile("test_data.txt"));
    ASSERT_EQUAL(manager.getStudentCount(), 2);

    // 验证数据完整性
    const Student* loaded = manager.findStudentById("2021001");
    ASSERT_TRUE(loaded != nullptr);
    ASSERT_EQUAL(loaded->getName(), "张三");
    ASSERT_EQUAL(loaded->getCourseCount(), 2);
}

// 测试9：班级平均分
TEST(ClassAverage) {
    StudentManager manager;
    Student s1("2021001", "张三", 20, "计算机");
    Student s2("2021002", "李四", 21, "软件工程");

    manager.addStudent(s1);
    manager.addStudent(s2);

    Course c1("CS101", "数据结构", 4, 90);
    Course c2("CS102", "算法", 4, 80);
    manager.addGrade("2021001", c1);
    manager.addGrade("2021002", c2);

    double avg = manager.getClassAverage();
    ASSERT_EQUAL(avg, 85.0);
}

// 测试10：空数据处理
TEST(EmptyData) {
    StudentManager manager;

    ASSERT_EQUAL(manager.getStudentCount(), 0);
    ASSERT_TRUE(manager.isEmpty());

    // 空数据的平均分应该是0
    ASSERT_EQUAL(manager.getClassAverage(), 0.0);

    // 空数据的排名应该是空列表
    auto ranking = manager.getRanking();
    ASSERT_TRUE(ranking.empty());
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "=== 学生管理系统单元测试 ===" << endl;
    cout << endl;

    // 运行所有测试
    RUN_TEST(AddStudent);
    RUN_TEST(RemoveStudent);
    RUN_TEST(FindStudent);
    RUN_TEST(InvalidStudentId);
    RUN_TEST(InvalidAge);
    RUN_TEST(AddGrade);
    RUN_TEST(InvalidGrade);
    RUN_TEST(SaveAndLoad);
    RUN_TEST(ClassAverage);
    RUN_TEST(EmptyData);

    // 显示测试结果
    cout << endl;
    cout << string(50, '=') << endl;
    cout << "测试结果：" << endl;
    cout << "通过：" << testsPassed << " 个" << endl;
    cout << "失败：" << testsFailed << " 个" << endl;
    cout << "总计：" << (testsPassed + testsFailed) << " 个" << endl;
    cout << string(50, '=') << endl;

    // 清理测试文件
    remove("test_data.txt");

    return (testsFailed == 0) ? 0 : 1;
}
