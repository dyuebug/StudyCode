// ============================================
// 03_iostream.cpp - 输入输出流练习
// 日期：2026-06-16（第3天）
// 内容：cin/cout、格式化输出、文件流
// ============================================

#include <iostream>
#include <iomanip>   // 格式化输出
#include <fstream>   // 文件流
#include <string>
#include <limits>

using std::cin;
using std::cout;
using std::endl;
using std::string;
using std::ifstream;
using std::ofstream;

// ============================================
// 练习1：cin/cout 基本使用
// ============================================

void test_basic_io() {
    cout << "\n=== 练习1：cin/cout 基本使用 ===" << endl;

    // 输出不同类型的数据
    int num = 42;
    double pi = 3.14159;
    string name = "Alice";

    cout << "整数：" << num << endl;
    cout << "浮点数：" << pi << endl;
    cout << "字符串：" << name << endl;

    // 链式调用
    cout << "姓名：" << name << ", 年龄：" << num << endl;

    // 输入数据
    cout << "\n请输入一个整数：";
    int input_num;
    cin >> input_num;
    cout << "你输入的整数是：" << input_num << endl;
}

// ============================================
// 练习2：流状态检查和恢复
// ============================================

void printStreamStatus(std::istream& is) {
    cout << "流状态：";
    cout << "good=" << is.good() << " ";
    cout << "bad=" << is.bad() << " ";
    cout << "fail=" << is.fail() << " ";
    cout << "eof=" << is.eof() << endl;
}

void test_stream_state() {
    cout << "\n=== 练习2：流状态检查 ===" << endl;

    cout << "初始流状态：" << endl;
    printStreamStatus(cin);

    cout << "\n请输入一个整数（尝试输入字母看看会发生什么）：";
    int num;
    cin >> num;

    if (cin.fail()) {
        cout << "❌ 输入失败！流进入 failbit 状态。" << endl;
        printStreamStatus(cin);

        // 恢复流状态
        cout << "\n恢复流状态..." << endl;
        cin.clear();  // 清除错误标志
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // 清空缓冲区

        cout << "恢复后的流状态：" << endl;
        printStreamStatus(cin);
    } else {
        cout << "✅ 输入成功！你输入的是：" << num << endl;
    }
}

// ============================================
// 练习3：格式化输出（iomanip）
// ============================================

void test_formatted_output() {
    cout << "\n=== 练习3：格式化输出 ===" << endl;

    // 1. setw() - 设置字段宽度
    cout << "\n1. 设置字段宽度（setw）：" << endl;
    cout << std::setw(10) << "Name" << std::setw(10) << "Age" << std::setw(10) << "Score" << endl;
    cout << std::setw(10) << "Alice" << std::setw(10) << 20 << std::setw(10) << 95 << endl;
    cout << std::setw(10) << "Bob" << std::setw(10) << 21 << std::setw(10) << 88 << endl;

    // 2. setfill() + setw() - 设置填充字符
    cout << "\n2. 设置填充字符（setfill）：" << endl;
    cout << std::setfill('*') << std::setw(20) << "Hello" << endl;
    cout << std::setfill(' ');  // 恢复默认填充字符

    // 3. left/right - 对齐方式
    cout << "\n3. 对齐方式（left/right）：" << endl;
    cout << std::left << std::setw(15) << "Left" << "|" << endl;
    cout << std::right << std::setw(15) << "Right" << "|" << endl;

    // 4. setprecision() - 设置浮点数精度
    cout << "\n4. 设置浮点数精度（setprecision）：" << endl;
    double pi = 3.14159265358979;
    cout << "默认：" << pi << endl;
    cout << std::fixed << std::setprecision(2) << "保留2位小数：" << pi << endl;
    cout << std::setprecision(5) << "保留5位小数：" << pi << endl;
    cout.unsetf(std::ios::fixed);  // 取消 fixed 模式

    // 5. hex/oct/dec - 进制转换
    cout << "\n5. 进制转换（hex/oct/dec）：" << endl;
    int num = 255;
    cout << "十进制：" << std::dec << num << endl;
    cout << "十六进制：" << std::hex << num << endl;
    cout << "八进制：" << std::oct << num << endl;
    cout << std::dec;  // 恢复十进制

    // 6. boolalpha - 布尔值输出
    cout << "\n6. 布尔值输出（boolalpha）：" << endl;
    bool flag = true;
    cout << "默认：" << flag << endl;
    cout << std::boolalpha << "boolalpha：" << flag << endl;
    cout << std::noboolalpha;  // 恢复默认
}

// ============================================
// 练习4：文件写入（ofstream）
// ============================================

void test_file_write() {
    cout << "\n=== 练习4：文件写入 ===" << endl;

    // 写入文本文件
    ofstream ofs("students.txt");

    if (!ofs.is_open()) {
        cout << "❌ 文件打开失败！" << endl;
        return;
    }

    // 写入学生信息
    ofs << "姓名\t年龄\t成绩" << endl;
    ofs << "张三\t20\t95" << endl;
    ofs << "李四\t21\t88" << endl;
    ofs << "王五\t19\t92" << endl;

    ofs.close();
    cout << "✅ 成功写入文件 students.txt" << endl;
}

// ============================================
// 练习5：文件读取（ifstream）
// ============================================

void test_file_read() {
    cout << "\n=== 练习5：文件读取 ===" << endl;

    ifstream ifs("students.txt");

    if (!ifs.is_open()) {
        cout << "❌ 文件打开失败！请先运行练习4创建文件。" << endl;
        return;
    }

    cout << "\n从文件读取的内容：" << endl;
    cout << "----------------------------------------" << endl;

    // 方法1：逐行读取（推荐）
    string line;
    while (getline(ifs, line)) {
        cout << line << endl;
    }

    cout << "----------------------------------------" << endl;

    ifs.close();
    cout << "✅ 成功读取文件" << endl;
}

// ============================================
// 练习6：文件追加模式
// ============================================

void test_file_append() {
    cout << "\n=== 练习6：文件追加模式 ===" << endl;

    // 以追加模式打开文件
    ofstream ofs("students.txt", std::ios::app);

    if (!ofs.is_open()) {
        cout << "❌ 文件打开失败！" << endl;
        return;
    }

    // 追加新学生
    ofs << "赵六\t22\t90" << endl;

    ofs.close();
    cout << "✅ 成功追加数据到文件" << endl;
}

// ============================================
// 练习7：综合练习 - 学生成绩管理
// ============================================

struct Student {
    string name;
    int age;
    double score;
};

void saveStudents(const Student students[], int count, const string& filename) {
    ofstream ofs(filename);

    if (!ofs.is_open()) {
        cout << "❌ 文件打开失败！" << endl;
        return;
    }

    // 写入表头
    ofs << std::left << std::setw(15) << "姓名"
        << std::setw(10) << "年龄"
        << std::setw(10) << "成绩" << endl;

    ofs << std::string(35, '-') << endl;

    // 写入学生数据
    for (int i = 0; i < count; i++) {
        ofs << std::left << std::setw(15) << students[i].name
            << std::setw(10) << students[i].age
            << std::fixed << std::setprecision(1)
            << std::setw(10) << students[i].score << endl;
    }

    ofs.close();
    cout << "✅ 学生信息已保存到 " << filename << endl;
}

void loadAndDisplayStudents(const string& filename) {
    ifstream ifs(filename);

    if (!ifs.is_open()) {
        cout << "❌ 文件打开失败！" << endl;
        return;
    }

    cout << "\n从文件加载的学生信息：" << endl;
    cout << "========================================" << endl;

    string line;
    while (getline(ifs, line)) {
        cout << line << endl;
    }

    cout << "========================================" << endl;

    ifs.close();
}

void test_student_management() {
    cout << "\n=== 练习7：学生成绩管理系统 ===" << endl;

    // 创建学生数据
    Student students[] = {
        {"Alice Wang", 20, 95.5},
        {"Bob Li", 21, 88.0},
        {"Charlie Zhang", 19, 92.5},
        {"Diana Liu", 20, 87.5},
        {"Eric Chen", 22, 90.0}
    };

    int count = sizeof(students) / sizeof(students[0]);

    // 保存到文件
    saveStudents(students, count, "student_records.txt");

    // 从文件加载并显示
    loadAndDisplayStudents("student_records.txt");
}

// ============================================
// 主函数
// ============================================

int main() {
    cout << "========================================" << endl;
    cout << "     C++ 输入输出流综合练习" << endl;
    cout << "========================================" << endl;

    // 练习1：基本输入输出
    // test_basic_io();  // 注释掉以避免交互

    // 练习2：流状态检查
    // test_stream_state();  // 注释掉以避免交互

    // 练习3：格式化输出
    test_formatted_output();

    // 练习4：文件写入
    test_file_write();

    // 练习5：文件读取
    test_file_read();

    // 练习6：文件追加
    test_file_append();

    // 验证追加后的文件内容
    cout << "\n追加后的文件内容：" << endl;
    test_file_read();

    // 练习7：学生成绩管理
    test_student_management();

    cout << "\n========================================" << endl;
    cout << "     所有练习完成！" << endl;
    cout << "========================================" << endl;

    return 0;
}

/*
编译运行：
g++ -o 03_iostream 03_iostream.cpp -std=c++17
./03_iostream

预期输出：
1. 格式化输出的各种效果
2. 创建 students.txt 文件
3. 读取 students.txt 文件内容
4. 追加数据到文件
5. 创建并显示 student_records.txt

关键知识点验证：
1. setw/setfill/left/right - 格式化输出
2. setprecision/fixed - 浮点数精度
3. hex/oct/dec - 进制转换
4. ofstream - 文件写入
5. ifstream - 文件读取
6. ios::app - 追加模式
7. getline - 按行读取
*/
