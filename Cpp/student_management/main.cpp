// ============================================
// main.cpp - 学生管理系统主程序
// 功能：提供菜单系统，处理用户交互
// ============================================

#include "StudentManager.h"
#include <iostream>
#include <limits>
#include <cstdlib>
#include <climits>
#include <cfloat>
using namespace std;

// --------------------------------------------
// 函数声明
// --------------------------------------------

void clearScreen();                     // 清屏
void pauseScreen();                     // 暂停
void displayMenu();                     // 显示菜单
void addStudentMenu(StudentManager& manager);       // 添加学生菜单
void removeStudentMenu(StudentManager& manager);    // 删除学生菜单
void updateStudentMenu(StudentManager& manager);    // 修改学生菜单
void findStudentMenu(StudentManager& manager);      // 查询学生菜单
void addGradeMenu(StudentManager& manager);         // 添加成绩菜单（新增）
void queryGradeMenu(StudentManager& manager);       // 查询成绩菜单（新增）
void statisticsMenu(StudentManager& manager);       // 统计信息菜单（新增）
void advancedQueryMenu(StudentManager& manager);    // 高级查询菜单（新增）
void saveDataMenu(StudentManager& manager);         // 保存数据菜单（新增）
void loadDataMenu(StudentManager& manager);         // 加载数据菜单（新增）
void importCSVMenu(StudentManager& manager);        // 批量导入菜单（新增）
void showHelp();                                    // 显示帮助（新增）
void clearInput();                                  // 清空输入缓冲区

// 通用输入验证函数（新增）
bool getIntInput(int& value, const string& prompt, int min = INT_MIN, int max = INT_MAX);
bool getDoubleInput(double& value, const string& prompt, double min = -DBL_MAX, double max = DBL_MAX);

// --------------------------------------------
// 主函数
// --------------------------------------------

int main() {
    StudentManager manager;  // 创建学生管理器
    int choice;              // 用户选择

    cout << "欢迎使用学生管理系统！" << endl;

    // 程序启动时自动加载数据
    if (manager.loadFromFile("students.txt")) {
        cout << "成功加载学生数据！" << endl;
    } else {
        cout << "未找到数据文件，从空白开始。" << endl;
    }
    cout << endl;

    // 主循环：显示菜单，处理用户选择
    while (true) {
        displayMenu();
        cout << "请选择操作：";
        cin >> choice;

        // 处理输入错误
        if (cin.fail()) {
            clearInput();
            cout << "输入错误，请输入数字！" << endl;
            continue;
        }

        // 根据选择执行相应操作
        switch (choice) {
            case 1:
                addStudentMenu(manager);
                pauseScreen();
                break;
            case 2:
                removeStudentMenu(manager);
                pauseScreen();
                break;
            case 3:
                updateStudentMenu(manager);
                pauseScreen();
                break;
            case 4:
                findStudentMenu(manager);
                pauseScreen();
                break;
            case 5:
                manager.displayAll();
                pauseScreen();
                break;
            case 6:
                addGradeMenu(manager);
                pauseScreen();
                break;
            case 7:
                queryGradeMenu(manager);
                pauseScreen();
                break;
            case 8:
                statisticsMenu(manager);
                pauseScreen();
                break;
            case 9:
                advancedQueryMenu(manager);
                pauseScreen();
                break;
            case 10:
                saveDataMenu(manager);
                pauseScreen();
                break;
            case 11:
                loadDataMenu(manager);
                pauseScreen();
                break;
            case 12:
                importCSVMenu(manager);
                pauseScreen();
                break;
            case 13:
                showHelp();
                pauseScreen();
                break;
            case 0:
                cout << "是否保存数据？(y/n): ";
                char saveChoice;
                cin >> saveChoice;

                if (saveChoice == 'y' || saveChoice == 'Y') {
                    manager.saveToFile("students.txt");
                    cout << "数据已保存。" << endl;
                }

                cout << "感谢使用学生管理系统，再见！" << endl;
                return 0;
            default:
                cout << "无效的选择，请重新输入！" << endl;
        }

        cout << endl;  // 空行分隔
    }

    return 0;
}

// --------------------------------------------
// 清屏功能
// --------------------------------------------

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

// --------------------------------------------
// 暂停功能
// --------------------------------------------

void pauseScreen() {
    cout << "\n按回车键继续...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

// --------------------------------------------
// 显示菜单
// --------------------------------------------

void displayMenu() {
    cout << string(50, '=') << endl;
    cout << "       学生管理系统 v1.0" << endl;
    cout << string(50, '=') << endl;
    cout << "1. 添加学生" << endl;
    cout << "2. 删除学生" << endl;
    cout << "3. 修改学生信息" << endl;
    cout << "4. 查询学生" << endl;
    cout << "5. 显示所有学生" << endl;
    cout << "6. 添加成绩" << endl;
    cout << "7. 查询成绩" << endl;
    cout << "8. 统计信息" << endl;
    cout << "9. 高级查询" << endl;
    cout << "10. 保存数据" << endl;
    cout << "11. 加载数据" << endl;
    cout << "12. 批量导入（CSV）" << endl;
    cout << "13. 帮助" << endl;
    cout << "0. 退出系统" << endl;
    cout << string(50, '=') << endl;
}

// --------------------------------------------
// 添加学生菜单
// --------------------------------------------

void addStudentMenu(StudentManager& manager) {
    string id, name, major;
    int age;

    cout << "\n--- 添加学生 ---" << endl;

    // 输入学号
    cout << "请输入学号（7位数字，如：2021001）：";
    cin >> id;

    // 输入姓名
    cout << "请输入姓名：";
    cin >> name;

    // 输入年龄
    cout << "请输入年龄（15-30岁）：";
    cin >> age;

    // 处理输入错误
    if (cin.fail()) {
        clearInput();
        cout << "年龄输入错误！" << endl;
        return;
    }

    // 输入专业
    cout << "请输入专业：";
    cin.ignore();  // 忽略前面的换行符
    getline(cin, major);  // 读取一行（专业可能包含空格）

    // 创建学生对象并添加（验证在 addStudent 中进行）
    Student student(id, name, age, major);
    manager.addStudent(student);
}

// --------------------------------------------
// 删除学生菜单
// --------------------------------------------

void removeStudentMenu(StudentManager& manager) {
    string id;

    cout << "\n--- 删除学生 ---" << endl;

    // 输入学号
    cout << "请输入要删除的学号：";
    cin >> id;

    // 先查找学生，显示信息
    const Student* student = manager.findStudentById(id);
    if (student == nullptr) {
        cout << "错误：学号 " << id << " 不存在！" << endl;
        return;
    }

    // 显示学生信息
    cout << "\n找到学生：" << endl;
    student->display();

    // 确认删除
    cout << "\n确认删除该学生？(y/n): ";
    char confirm;
    cin >> confirm;

    if (confirm == 'y' || confirm == 'Y') {
        // 删除学生
        manager.removeStudent(id);
    } else {
        cout << "取消删除。" << endl;
    }
}

// --------------------------------------------
// 修改学生菜单
// --------------------------------------------

void updateStudentMenu(StudentManager& manager) {
    string id;

    cout << "\n--- 修改学生信息 ---" << endl;

    // 输入学号
    cout << "请输入要修改的学号：";
    cin >> id;

    // 查找学生
    Student* student = manager.findStudentById(id);

    if (student == nullptr) {
        cout << "错误：学号 " << id << " 不存在！" << endl;
        return;
    }

    // 显示当前信息
    cout << "\n当前学生信息：" << endl;
    student->display();

    // 选择要修改的属性
    cout << "\n请选择要修改的属性：" << endl;
    cout << "1. 姓名" << endl;
    cout << "2. 年龄" << endl;
    cout << "3. 专业" << endl;
    cout << "0. 取消修改" << endl;
    cout << "请选择：";

    int choice;
    cin >> choice;

    // 处理输入错误
    if (cin.fail()) {
        clearInput();
        cout << "输入错误！" << endl;
        return;
    }

    // 根据选择修改相应属性
    switch (choice) {
        case 1: {
            string name;
            cout << "请输入新姓名：";
            cin >> name;
            student->setName(name);
            cout << "姓名修改成功！" << endl;
            break;
        }
        case 2: {
            int age;
            cout << "请输入新年龄：";
            cin >> age;
            if (cin.fail()) {
                clearInput();
                cout << "年龄输入错误！" << endl;
                return;
            }
            student->setAge(age);
            cout << "年龄修改成功！" << endl;
            break;
        }
        case 3: {
            string major;
            cout << "请输入新专业：";
            cin.ignore();
            getline(cin, major);
            student->setMajor(major);
            cout << "专业修改成功！" << endl;
            break;
        }
        case 0:
            cout << "取消修改。" << endl;
            break;
        default:
            cout << "无效的选择！" << endl;
    }
}

// --------------------------------------------
// 查询学生菜单
// --------------------------------------------

void findStudentMenu(StudentManager& manager) {
    cout << "\n--- 查询学生 ---" << endl;
    cout << "1. 按学号查询" << endl;
    cout << "2. 按姓名查询" << endl;
    cout << "请选择：";

    int choice;
    cin >> choice;

    // 处理输入错误
    if (cin.fail()) {
        clearInput();
        cout << "输入错误！" << endl;
        return;
    }

    switch (choice) {
        case 1: {
            // 按学号查询
            string id;
            cout << "请输入学号：";
            cin >> id;

            const Student* student = manager.findStudentById(id);
            if (student != nullptr) {
                cout << "\n找到学生：" << endl;
                cout << string(40, '-') << endl;
                student->display();
                cout << string(40, '-') << endl;
            } else {
                cout << "未找到学号为 " << id << " 的学生。" << endl;
            }
            break;
        }
        case 2: {
            // 按姓名查询
            string name;
            cout << "请输入姓名：";
            cin >> name;

            vector<Student> results = manager.findStudentsByName(name);
            if (results.empty()) {
                cout << "未找到姓名为 " << name << " 的学生。" << endl;
            } else {
                cout << "\n找到 " << results.size() << " 个学生：" << endl;
                cout << string(60, '-') << endl;
                cout << left
                     << setw(12) << "学号"
                     << setw(10) << "姓名"
                     << setw(6) << "年龄"
                     << setw(20) << "专业"
                     << endl;
                cout << string(60, '-') << endl;
                for (const auto& student : results) {
                    student.displayBrief();
                }
                cout << string(60, '-') << endl;
            }
            break;
        }
        default:
            cout << "无效的选择！" << endl;
    }
}

// --------------------------------------------
// 清空输入缓冲区
// --------------------------------------------

void clearInput() {
    cin.clear();  // 清除错误标志
    cin.ignore(numeric_limits<streamsize>::max(), '\n');  // 忽略错误输入
}

// --------------------------------------------
// 通用输入验证函数（新增）
// --------------------------------------------

// 获取整数输入（带范围验证）
bool getIntInput(int& value, const string& prompt, int min, int max) {
    cout << prompt;
    cin >> value;

    if (cin.fail()) {
        clearInput();
        cout << "错误：输入格式无效！" << endl;
        return false;
    }

    if (value < min || value > max) {
        cout << "错误：输入必须在 " << min << " 到 " << max << " 之间！" << endl;
        return false;
    }

    return true;
}

// 获取浮点数输入（带范围验证）
bool getDoubleInput(double& value, const string& prompt, double min, double max) {
    cout << prompt;
    cin >> value;

    if (cin.fail()) {
        clearInput();
        cout << "错误：输入格式无效！" << endl;
        return false;
    }

    if (value < min || value > max) {
        cout << "错误：输入必须在 " << min << " 到 " << max << " 之间！" << endl;
        return false;
    }

    return true;
}

// --------------------------------------------
// 添加成绩菜单（新增）
// --------------------------------------------

void addGradeMenu(StudentManager& manager) {
    string studentId, courseCode, courseName;
    int credits;
    double score;

    cout << "\n--- 添加成绩 ---" << endl;

    // 输入学号
    cout << "请输入学号：";
    cin >> studentId;

    // 输入课程代码
    cout << "请输入课程代码（如：CS101）：";
    cin >> courseCode;

    // 输入课程名称
    cout << "请输入课程名称：";
    cin.ignore();
    getline(cin, courseName);

    // 输入学分（使用通用验证函数）
    if (!getIntInput(credits, "请输入学分（1-5）：", 1, 5)) {
        return;
    }

    // 输入成绩（使用通用验证函数）
    if (!getDoubleInput(score, "请输入成绩（0-100）：", 0.0, 100.0)) {
        return;
    }

    // 创建课程对象并添加（验证在 addGrade 中进行）
    Course course(courseCode, courseName, credits, score);
    manager.addGrade(studentId, course);
}

// --------------------------------------------
// 查询成绩菜单（新增）
// --------------------------------------------

void queryGradeMenu(StudentManager& manager) {
    string studentId;

    cout << "\n--- 查询成绩 ---" << endl;

    // 输入学号
    cout << "请输入学号：";
    cin >> studentId;

    // 显示学生成绩
    manager.displayStudentGrades(studentId);
}

// --------------------------------------------
// 统计信息菜单（新增）
// --------------------------------------------

void statisticsMenu(StudentManager& manager) {
    cout << "\n--- 统计信息 ---" << endl;
    cout << "1. 班级平均分" << endl;
    cout << "2. 成绩排名" << endl;
    cout << "3. 优秀学生（平均分>=90）" << endl;

    int choice;
    if (!getIntInput(choice, "请选择：", 1, 3)) {
        return;
    }

    switch (choice) {
        case 1: {
            // 班级平均分
            double avg = manager.getClassAverage();
            cout << "\n班级平均分：" << fixed << setprecision(2) << avg << endl;
            break;
        }
        case 2: {
            // 成绩排名
            auto ranking = manager.getRanking();

            if (ranking.empty()) {
                cout << "暂无成绩数据。" << endl;
                break;
            }

            cout << "\n成绩排名（前10名）：" << endl;
            cout << string(50, '=') << endl;
            cout << left
                 << setw(10) << "排名"
                 << setw(15) << "学号"
                 << setw(15) << "平均分"
                 << endl;
            cout << string(50, '-') << endl;

            int rank = 1;
            for (size_t i = 0; i < ranking.size() && i < 10; i++) {
                cout << left
                     << setw(10) << rank++
                     << setw(15) << ranking[i].first
                     << setw(15) << fixed << setprecision(2) << ranking[i].second
                     << endl;
            }
            cout << string(50, '=') << endl;
            break;
        }
        case 3: {
            // 优秀学生
            auto excellentStudents = manager.findExcellentStudents(90.0);

            if (excellentStudents.empty()) {
                cout << "暂无优秀学生（平均分>=90）。" << endl;
                break;
            }

            cout << "\n优秀学生名单（平均分>=90）：" << endl;
            cout << string(70, '=') << endl;
            cout << left
                 << setw(12) << "学号"
                 << setw(10) << "姓名"
                 << setw(6) << "年龄"
                 << setw(20) << "专业"
                 << setw(12) << "平均分"
                 << endl;
            cout << string(70, '-') << endl;

            for (const auto& student : excellentStudents) {
                cout << left
                     << setw(12) << student.getId()
                     << setw(10) << student.getName()
                     << setw(6) << student.getAge()
                     << setw(20) << student.getMajor()
                     << setw(12) << fixed << setprecision(2) << student.getAverageScore()
                     << endl;
            }
            cout << string(70, '=') << endl;
            cout << "优秀学生总数：" << excellentStudents.size() << endl;
            break;
        }
        default:
            cout << "无效的选择！" << endl;
    }
}

// --------------------------------------------
// 高级查询菜单（新增）
// --------------------------------------------

void advancedQueryMenu(StudentManager& manager) {
    cout << "\n--- 高级查询 ---" << endl;
    cout << "1. 按年龄范围查询" << endl;
    cout << "2. 按专业查询" << endl;
    cout << "3. 按平均分范围查询" << endl;
    cout << "请选择：";

    int choice;
    cin >> choice;

    if (cin.fail()) {
        clearInput();
        cout << "输入错误！" << endl;
        return;
    }

    vector<Student> results;

    switch (choice) {
        case 1: {
            // 按年龄范围查询
            int minAge, maxAge;
            cout << "请输入最小年龄：";
            cin >> minAge;
            cout << "请输入最大年龄：";
            cin >> maxAge;

            if (cin.fail()) {
                clearInput();
                cout << "输入错误！" << endl;
                return;
            }

            // 使用 Lambda 表达式过滤
            results = manager.filterStudents(
                [minAge, maxAge](const Student& s) {
                    return s.getAge() >= minAge && s.getAge() <= maxAge;
                });
            break;
        }
        case 2: {
            // 按专业查询
            string major;
            cout << "请输入专业：";
            cin.ignore();
            getline(cin, major);

            // 使用 Lambda 表达式过滤
            results = manager.filterStudents(
                [&major](const Student& s) {
                    return s.getMajor() == major;
                });
            break;
        }
        case 3: {
            // 按平均分范围查询
            double minScore, maxScore;
            cout << "请输入最低平均分：";
            cin >> minScore;
            cout << "请输入最高平均分：";
            cin >> maxScore;

            if (cin.fail()) {
                clearInput();
                cout << "输入错误！" << endl;
                return;
            }

            // 使用 Lambda 表达式过滤
            results = manager.filterStudents(
                [minScore, maxScore](const Student& s) {
                    return s.getCourseCount() > 0 &&
                           s.getAverageScore() >= minScore &&
                           s.getAverageScore() <= maxScore;
                });
            break;
        }
        default:
            cout << "无效的选择！" << endl;
            return;
    }

    // 显示查询结果
    if (results.empty()) {
        cout << "未找到符合条件的学生。" << endl;
    } else {
        cout << "\n找到 " << results.size() << " 个学生：" << endl;
        cout << string(70, '=') << endl;
        cout << left
             << setw(12) << "学号"
             << setw(10) << "姓名"
             << setw(6) << "年龄"
             << setw(20) << "专业"
             << setw(12) << "平均分"
             << endl;
        cout << string(70, '-') << endl;

        for (const auto& student : results) {
            cout << left
                 << setw(12) << student.getId()
                 << setw(10) << student.getName()
                 << setw(6) << student.getAge()
                 << setw(20) << student.getMajor()
                 << setw(12) << fixed << setprecision(2)
                 << (student.getCourseCount() > 0 ? student.getAverageScore() : 0.0)
                 << endl;
        }
        cout << string(70, '=') << endl;
    }
}

// --------------------------------------------
// 保存数据菜单（新增）
// --------------------------------------------

void saveDataMenu(StudentManager& manager) {
    cout << "\n--- 保存数据 ---" << endl;

    if (manager.saveToFile("students.txt")) {
        cout << "数据保存成功！" << endl;
    } else {
        cout << "数据保存失败！" << endl;
    }
}

// --------------------------------------------
// 加载数据菜单（新增）
// --------------------------------------------

void loadDataMenu(StudentManager& manager) {
    cout << "\n--- 加载数据 ---" << endl;
    cout << "警告：加载数据将覆盖当前所有数据！" << endl;
    cout << "是否继续？(y/n): ";

    char choice;
    cin >> choice;

    if (choice == 'y' || choice == 'Y') {
        if (manager.loadFromFile("students.txt")) {
            cout << "数据加载成功！" << endl;
        } else {
            cout << "数据加载失败！" << endl;
        }
    } else {
        cout << "取消加载。" << endl;
    }
}

// --------------------------------------------
// 批量导入菜单（新增）
// --------------------------------------------

void importCSVMenu(StudentManager& manager) {
    string filename;

    cout << "\n--- 批量导入（CSV格式） ---" << endl;
    cout << "CSV文件格式示例：" << endl;
    cout << "学号,姓名,年龄,专业" << endl;
    cout << "2021001,张三,20,计算机科学" << endl;
    cout << "2021002,李四,21,软件工程" << endl;
    cout << endl;

    cout << "请输入CSV文件名：";
    cin >> filename;

    if (manager.importFromCSV(filename)) {
        cout << "\n批量导入成功！" << endl;
    } else {
        cout << "\n批量导入失败或无数据导入！" << endl;
    }
}

// --------------------------------------------
// 显示帮助（新增）
// --------------------------------------------

void showHelp() {
    cout << "\n" << string(60, '=') << endl;
    cout << "       学生管理系统 v1.0 - 使用帮助" << endl;
    cout << string(60, '=') << endl;

    cout << "\n【基本操作】" << endl;
    cout << string(60, '-') << endl;
    cout << "1. 添加学生" << endl;
    cout << "   - 学号格式：7位数字（如：2021001）" << endl;
    cout << "   - 年龄范围：15-30岁" << endl;
    cout << "   - 专业可以包含空格" << endl;
    cout << endl;

    cout << "2. 删除学生" << endl;
    cout << "   - 输入学号即可删除" << endl;
    cout << "   - 删除前会显示学生信息并要求确认" << endl;
    cout << endl;

    cout << "3. 修改学生信息" << endl;
    cout << "   - 可以修改姓名、年龄、专业" << endl;
    cout << "   - 学号不可修改（唯一标识）" << endl;
    cout << endl;

    cout << "4. 查询学生" << endl;
    cout << "   - 按学号查询：精确查找" << endl;
    cout << "   - 按姓名查询：可能返回多个结果" << endl;
    cout << endl;

    cout << "\n【成绩管理】" << endl;
    cout << string(60, '-') << endl;
    cout << "6. 添加成绩" << endl;
    cout << "   - 学分范围：1-5" << endl;
    cout << "   - 成绩范围：0-100" << endl;
    cout << "   - 课程代码不能重复" << endl;
    cout << endl;

    cout << "7. 查询成绩" << endl;
    cout << "   - 显示学生的所有课程和成绩" << endl;
    cout << "   - 自动计算平均分和总学分" << endl;
    cout << endl;

    cout << "8. 统计信息" << endl;
    cout << "   - 班级平均分：所有学生的平均分" << endl;
    cout << "   - 成绩排名：显示前10名" << endl;
    cout << "   - 优秀学生：平均分>=90的学生" << endl;
    cout << endl;

    cout << "9. 高级查询" << endl;
    cout << "   - 按年龄范围查询" << endl;
    cout << "   - 按专业查询" << endl;
    cout << "   - 按平均分范围查询" << endl;
    cout << endl;

    cout << "\n【数据管理】" << endl;
    cout << string(60, '-') << endl;
    cout << "10. 保存数据" << endl;
    cout << "    - 手动保存所有数据到 students.txt" << endl;
    cout << endl;

    cout << "11. 加载数据" << endl;
    cout << "    - 从 students.txt 加载数据" << endl;
    cout << "    - 会覆盖当前数据（有确认提示）" << endl;
    cout << endl;

    cout << "12. 批量导入（CSV）" << endl;
    cout << "    - 从CSV文件批量导入学生" << endl;
    cout << "    - CSV格式：学号,姓名,年龄,专业" << endl;
    cout << "    - 自动验证数据并统计导入结果" << endl;
    cout << endl;

    cout << "\n【数据文件说明】" << endl;
    cout << string(60, '-') << endl;
    cout << "- students.txt：主数据文件（自动生成）" << endl;
    cout << "- backup_*.txt：自动备份文件（带时间戳）" << endl;
    cout << "- 程序启动时自动加载 students.txt" << endl;
    cout << "- 退出时提示是否保存数据" << endl;
    cout << endl;

    cout << "\n【常见问题】" << endl;
    cout << string(60, '-') << endl;
    cout << "Q: 学号格式错误怎么办？" << endl;
    cout << "A: 学号必须是7位数字，如：2021001" << endl;
    cout << endl;

    cout << "Q: 如何备份数据？" << endl;
    cout << "A: 使用【保存数据】功能，或程序会自动备份" << endl;
    cout << endl;

    cout << "Q: 数据丢失了怎么办？" << endl;
    cout << "A: 查看 backup_*.txt 备份文件，使用【加载数据】恢复" << endl;
    cout << endl;

    cout << "Q: CSV导入失败怎么办？" << endl;
    cout << "A: 检查CSV文件格式，确保使用逗号分隔，数据完整" << endl;
    cout << endl;

    cout << string(60, '=') << endl;
    cout << "提示：每个操作后会暂停，按回车键继续" << endl;
    cout << string(60, '=') << endl;
}
