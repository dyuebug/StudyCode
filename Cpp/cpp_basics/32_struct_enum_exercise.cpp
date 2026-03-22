/*
 * ============================================================================
 * 文件名：32_struct_enum_exercise.cpp
 * 主题：结构体与枚举综合练习
 * 学习目标：
 *   1. 用结构体管理复杂数据
 *   2. 用枚举表示状态和类型
 *   3. 结构体嵌套枚举
 *   4. 实战案例：学生管理系统
 *   5. 掌握结构体和枚举的组合使用
 *
 * 编译命令：g++ -std=c++11 -o 32_struct_enum_exercise 32_struct_enum_exercise.cpp
 * 运行命令：./32_struct_enum_exercise (Linux/Mac) 或 32_struct_enum_exercise.exe (Windows)
 * ============================================================================
 */

#include <iostream>
#include <string>
using namespace std;

// 枚举：性别
enum class Gender {
    MALE,
    FEMALE,
    OTHER
};

// 枚举：学生状态
enum class StudentStatus {
    ACTIVE,      // 在读
    GRADUATED,   // 已毕业
    SUSPENDED,   // 休学
    DROPPED      // 退学
};

// 枚举：成绩等级
enum class Grade {
    A,  // 90-100
    B,  // 80-89
    C,  // 70-79
    D,  // 60-69
    F   // 0-59
};

// 结构体：学生信息
struct Student {
    int id;
    string name;
    Gender gender;
    int age;
    double score;
    StudentStatus status;
};

// 枚举：任务优先级
enum class Priority {
    LOW,
    MEDIUM,
    HIGH,
    URGENT
};

// 枚举：任务状态
enum class TaskStatus {
    TODO,
    IN_PROGRESS,
    COMPLETED,
    CANCELLED
};

// 结构体：任务
struct Task {
    int id;
    string title;
    string description;
    Priority priority;
    TaskStatus status;
};

// 函数声明
string getGenderName(Gender g);
string getStudentStatusName(StudentStatus s);
Grade calculateGrade(double score);
string getGradeName(Grade g);
void printStudent(const Student& s);
string getPriorityName(Priority p);
string getTaskStatusName(TaskStatus s);
void printTask(const Task& t);

int main() {
    cout << "=== 结构体与枚举综合练习 ===" << endl << endl;

    // ========================================
    // 练习1：学生信息管理
    // ========================================
    cout << "【练习1：学生信息管理】" << endl;

    Student s1 = {
        1001,
        "Alice",
        Gender::FEMALE,
        20,
        95.5,
        StudentStatus::ACTIVE
    };

    Student s2 = {
        1002,
        "Bob",
        Gender::MALE,
        21,
        88.0,
        StudentStatus::ACTIVE
    };

    Student s3 = {
        1003,
        "Charlie",
        Gender::MALE,
        22,
        76.5,
        StudentStatus::GRADUATED
    };

    cout << "学生列表：" << endl;
    printStudent(s1);
    printStudent(s2);
    printStudent(s3);
    cout << endl;

    // ========================================
    // 练习2：成绩等级评定
    // ========================================
    cout << "【练习2：成绩等级评定】" << endl;

    double scores[] = {95.5, 88.0, 76.5, 62.0, 45.0};
    int numScores = 5;

    cout << "成绩等级：" << endl;
    for (int i = 0; i < numScores; i++) {
        Grade grade = calculateGrade(scores[i]);
        cout << "分数 " << scores[i] << " → 等级 " << getGradeName(grade) << endl;
    }
    cout << endl;

    // ========================================
    // 练习3：学生数组管理
    // ========================================
    cout << "【练习3：学生数组管理】" << endl;

    Student students[5] = {
        {1001, "Alice", Gender::FEMALE, 20, 95.5, StudentStatus::ACTIVE},
        {1002, "Bob", Gender::MALE, 21, 88.0, StudentStatus::ACTIVE},
        {1003, "Charlie", Gender::MALE, 22, 76.5, StudentStatus::GRADUATED},
        {1004, "Diana", Gender::FEMALE, 19, 92.0, StudentStatus::ACTIVE},
        {1005, "Eve", Gender::FEMALE, 20, 85.5, StudentStatus::SUSPENDED}
    };

    // 统计在读学生
    int activeCount = 0;
    for (int i = 0; i < 5; i++) {
        if (students[i].status == StudentStatus::ACTIVE) {
            activeCount++;
        }
    }
    cout << "在读学生数量：" << activeCount << endl;

    // 统计男女比例
    int maleCount = 0, femaleCount = 0;
    for (int i = 0; i < 5; i++) {
        if (students[i].gender == Gender::MALE) {
            maleCount++;
        } else if (students[i].gender == Gender::FEMALE) {
            femaleCount++;
        }
    }
    cout << "男生：" << maleCount << "人，女生：" << femaleCount << "人" << endl;

    // 计算平均分
    double totalScore = 0;
    for (int i = 0; i < 5; i++) {
        totalScore += students[i].score;
    }
    cout << "平均分：" << totalScore / 5 << endl;

    // 查找最高分学生
    int maxIndex = 0;
    for (int i = 1; i < 5; i++) {
        if (students[i].score > students[maxIndex].score) {
            maxIndex = i;
        }
    }
    cout << "最高分学生：" << students[maxIndex].name
         << " (" << students[maxIndex].score << "分)" << endl;
    cout << endl;

    // ========================================
    // 练习4：任务管理系统
    // ========================================
    cout << "【练习4：任务管理系统】" << endl;

    Task tasks[4] = {
        {1, "完成作业", "完成C++作业", Priority::HIGH, TaskStatus::IN_PROGRESS},
        {2, "复习考试", "准备期末考试", Priority::URGENT, TaskStatus::TODO},
        {3, "看电影", "周末看新电影", Priority::LOW, TaskStatus::TODO},
        {4, "买书", "购买C++教材", Priority::MEDIUM, TaskStatus::COMPLETED}
    };

    cout << "任务列表：" << endl;
    for (int i = 0; i < 4; i++) {
        printTask(tasks[i]);
    }
    cout << endl;

    // 统计各状态任务数量
    int todoCount = 0, inProgressCount = 0, completedCount = 0;
    for (int i = 0; i < 4; i++) {
        switch (tasks[i].status) {
            case TaskStatus::TODO:
                todoCount++;
                break;
            case TaskStatus::IN_PROGRESS:
                inProgressCount++;
                break;
            case TaskStatus::COMPLETED:
                completedCount++;
                break;
            default:
                break;
        }
    }
    cout << "任务统计：" << endl;
    cout << "  待办：" << todoCount << endl;
    cout << "  进行中：" << inProgressCount << endl;
    cout << "  已完成：" << completedCount << endl;
    cout << endl;

    // 查找紧急任务
    cout << "紧急任务：" << endl;
    for (int i = 0; i < 4; i++) {
        if (tasks[i].priority == Priority::URGENT) {
            cout << "  - " << tasks[i].title << endl;
        }
    }
    cout << endl;

    // ========================================
    // 练习5：动态学生管理
    // ========================================
    cout << "【练习5：动态学生管理】" << endl;

    int numStudents = 3;
    Student* dynamicStudents = new Student[numStudents];

    // 初始化
    dynamicStudents[0] = {2001, "Frank", Gender::MALE, 20, 90.0, StudentStatus::ACTIVE};
    dynamicStudents[1] = {2002, "Grace", Gender::FEMALE, 21, 87.5, StudentStatus::ACTIVE};
    dynamicStudents[2] = {2003, "Henry", Gender::MALE, 19, 92.5, StudentStatus::ACTIVE};

    cout << "动态学生列表：" << endl;
    for (int i = 0; i < numStudents; i++) {
        printStudent(dynamicStudents[i]);
    }

    // 释放内存
    delete[] dynamicStudents;
    dynamicStudents = nullptr;
    cout << "✓ 内存已释放" << endl;
    cout << endl;

    // ========================================
    // 练习6：结构体指针与枚举
    // ========================================
    cout << "【练习6：结构体指针与枚举】" << endl;

    Student* pStudent = new Student{
        3001,
        "Ivy",
        Gender::FEMALE,
        20,
        88.5,
        StudentStatus::ACTIVE
    };

    cout << "通过指针访问：" << endl;
    cout << "  ID：" << pStudent->id << endl;
    cout << "  姓名：" << pStudent->name << endl;
    cout << "  性别：" << getGenderName(pStudent->gender) << endl;
    cout << "  年龄：" << pStudent->age << endl;
    cout << "  成绩：" << pStudent->score << endl;
    cout << "  状态：" << getStudentStatusName(pStudent->status) << endl;

    // 修改状态
    pStudent->status = StudentStatus::GRADUATED;
    cout << "修改状态后：" << getStudentStatusName(pStudent->status) << endl;

    delete pStudent;
    pStudent = nullptr;
    cout << "✓ 内存已释放" << endl;
    cout << endl;

    // ========================================
    // 练习7：结构体与枚举的最佳实践
    // ========================================
    cout << "【练习7：结构体与枚举的最佳实践】" << endl;

    cout << "✓ 最佳实践：" << endl;
    cout << "  1. 用枚举替代魔法数字，提高可读性" << endl;
    cout << "  2. 用结构体组织相关数据，提高可维护性" << endl;
    cout << "  3. 优先使用enum class（强类型枚举）" << endl;
    cout << "  4. 结构体成员命名要清晰" << endl;
    cout << "  5. 为枚举提供转字符串函数" << endl;
    cout << "  6. 结构体传参用const引用（高效且安全）" << endl;
    cout << endl;

    // ========================================
    // 练习8：实战案例总结
    // ========================================
    cout << "【练习8：实战案例总结】" << endl;

    cout << "结构体的应用场景：" << endl;
    cout << "  ✓ 学生信息管理" << endl;
    cout << "  ✓ 任务管理系统" << endl;
    cout << "  ✓ 图形坐标（Point、Rectangle）" << endl;
    cout << "  ✓ 日期时间（Date、Time）" << endl;
    cout << "  ✓ 数据库记录" << endl;
    cout << endl;

    cout << "枚举的应用场景：" << endl;
    cout << "  ✓ 状态表示（任务状态、学生状态）" << endl;
    cout << "  ✓ 类型分类（性别、优先级）" << endl;
    cout << "  ✓ 等级评定（成绩等级）" << endl;
    cout << "  ✓ 方向控制（上下左右）" << endl;
    cout << "  ✓ 错误码定义" << endl;
    cout << endl;

    return 0;
}

// 函数实现
string getGenderName(Gender g) {
    switch (g) {
        case Gender::MALE: return "男";
        case Gender::FEMALE: return "女";
        case Gender::OTHER: return "其他";
        default: return "未知";
    }
}

string getStudentStatusName(StudentStatus s) {
    switch (s) {
        case StudentStatus::ACTIVE: return "在读";
        case StudentStatus::GRADUATED: return "已毕业";
        case StudentStatus::SUSPENDED: return "休学";
        case StudentStatus::DROPPED: return "退学";
        default: return "未知";
    }
}

Grade calculateGrade(double score) {
    if (score >= 90) return Grade::A;
    if (score >= 80) return Grade::B;
    if (score >= 70) return Grade::C;
    if (score >= 60) return Grade::D;
    return Grade::F;
}

string getGradeName(Grade g) {
    switch (g) {
        case Grade::A: return "A (优秀)";
        case Grade::B: return "B (良好)";
        case Grade::C: return "C (中等)";
        case Grade::D: return "D (及格)";
        case Grade::F: return "F (不及格)";
        default: return "未知";
    }
}

void printStudent(const Student& s) {
    cout << "  [" << s.id << "] " << s.name
         << " | " << getGenderName(s.gender)
         << " | " << s.age << "岁"
         << " | " << s.score << "分"
         << " | " << getStudentStatusName(s.status) << endl;
}

string getPriorityName(Priority p) {
    switch (p) {
        case Priority::LOW: return "低";
        case Priority::MEDIUM: return "中";
        case Priority::HIGH: return "高";
        case Priority::URGENT: return "紧急";
        default: return "未知";
    }
}

string getTaskStatusName(TaskStatus s) {
    switch (s) {
        case TaskStatus::TODO: return "待办";
        case TaskStatus::IN_PROGRESS: return "进行中";
        case TaskStatus::COMPLETED: return "已完成";
        case TaskStatus::CANCELLED: return "已取消";
        default: return "未知";
    }
}

void printTask(const Task& t) {
    cout << "  [" << t.id << "] " << t.title
         << " | 优先级：" << getPriorityName(t.priority)
         << " | 状态：" << getTaskStatusName(t.status) << endl;
}
