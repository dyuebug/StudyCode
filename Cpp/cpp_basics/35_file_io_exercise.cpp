// ============================================
// 文件操作练习 - 超详细注释版
// 日期：2026-03-16（第10天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】文件操作实战

本文件包含多个实用的文件操作案例：
1. 配置文件读取器
2. 日志记录系统
3. 文件复制工具
4. 文本统计工具
5. 简单的数据库（CSV格式）

通过这些实战案例，掌握文件操作的实际应用。
*/

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <vector>

// ============================================
// 案例1：配置文件读取器
// ============================================

/*
【配置文件格式】
key1=value1
key2=value2
# 这是注释
key3=value3
*/

// 读取配置文件，返回键值对映射
// 参数：filename - 配置文件名
// 返回：map<string, string> - 键值对映射
std::map<std::string, std::string> readConfig(const std::string& filename) {
    std::map<std::string, std::string> config;

    // 打开配置文件
    std::ifstream file(filename);

    // 检查文件是否打开成功
    if (!file.is_open()) {
        std::cout << "错误：无法打开配置文件 " << filename << std::endl;
        return config;  // 返回空映射
    }

    std::string line;

    // 逐行读取
    while (std::getline(file, line)) {
        // 跳过空行
        if (line.empty()) {
            continue;
        }

        // 跳过注释行（以#开头）
        if (line[0] == '#') {
            continue;
        }

        // 查找等号位置
        // find()：查找字符在字符串中的位置
        // 返回：位置索引，如果未找到返回 string::npos
        size_t pos = line.find('=');

        // 如果找到等号
        if (pos != std::string::npos) {
            // 提取键（等号左边）
            // substr(start, length)：提取子字符串
            std::string key = line.substr(0, pos);

            // 提取值（等号右边）
            // pos + 1：跳过等号
            std::string value = line.substr(pos + 1);

            // 存储到映射中
            config[key] = value;
        }
    }

    file.close();

    return config;
}

// ============================================
// 案例2：日志记录系统
// ============================================

/*
【日志级别】
INFO：一般信息
WARNING：警告信息
ERROR：错误信息
*/

// 写入日志到文件
// 参数：
//   level - 日志级别（INFO/WARNING/ERROR）
//   message - 日志消息
void writeLog(const std::string& level, const std::string& message) {
    // 使用追加模式打开日志文件
    // ios::app：追加模式，不清空原内容
    std::ofstream logFile("app.log", std::ios::app);

    if (!logFile.is_open()) {
        std::cout << "错误：无法打开日志文件" << std::endl;
        return;
    }

    // 写入日志
    // 格式：[级别] 消息
    logFile << "[" << level << "] " << message << std::endl;

    logFile.close();

    std::cout << "日志已记录：[" << level << "] " << message << std::endl;
}

// ============================================
// 案例3：文件复制工具
// ============================================

// 复制文件
// 参数：
//   source - 源文件名
//   dest - 目标文件名
// 返回：bool - 成功返回true，失败返回false
bool copyFile(const std::string& source, const std::string& dest) {
    // 打开源文件（读取模式）
    std::ifstream srcFile(source);

    if (!srcFile.is_open()) {
        std::cout << "错误：无法打开源文件 " << source << std::endl;
        return false;
    }

    // 打开目标文件（写入模式）
    std::ofstream destFile(dest);

    if (!destFile.is_open()) {
        std::cout << "错误：无法创建目标文件 " << dest << std::endl;
        srcFile.close();
        return false;
    }

    // 逐行复制
    std::string line;
    while (std::getline(srcFile, line)) {
        destFile << line << std::endl;
    }

    // 关闭文件
    srcFile.close();
    destFile.close();

    std::cout << "文件复制成功：" << source << " → " << dest << std::endl;

    return true;
}

// ============================================
// 案例4：文本统计工具
// ============================================

// 统计文件信息
// 参数：filename - 文件名
void analyzeFile(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cout << "错误：无法打开文件 " << filename << std::endl;
        return;
    }

    int lineCount = 0;      // 行数
    int wordCount = 0;      // 单词数
    int charCount = 0;      // 字符数

    std::string line;

    // 逐行读取
    while (std::getline(file, line)) {
        lineCount++;
        charCount += line.length();

        // 统计单词数
        // stringstream：字符串流，用于分割字符串
        std::stringstream ss(line);
        std::string word;

        // 逐词读取
        while (ss >> word) {
            wordCount++;
        }
    }

    file.close();

    // 输出统计结果
    std::cout << "\n文件统计：" << filename << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    std::cout << "行数：" << lineCount << std::endl;
    std::cout << "单词数：" << wordCount << std::endl;
    std::cout << "字符数：" << charCount << std::endl;
}

// ============================================
// 案例5：简单的学生数据库（CSV格式）
// ============================================

/*
【CSV格式】
姓名,年龄,成绩
张三,20,85
李四,21,90
*/

// 学生结构体
struct Student {
    std::string name;
    int age;
    double score;
};

// 保存学生数据到CSV文件
// 参数：
//   filename - 文件名
//   students - 学生列表
void saveStudents(const std::string& filename, const std::vector<Student>& students) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cout << "错误：无法创建文件 " << filename << std::endl;
        return;
    }

    // 写入表头
    file << "姓名,年龄,成绩" << std::endl;

    // 写入学生数据
    for (const Student& s : students) {
        file << s.name << "," << s.age << "," << s.score << std::endl;
    }

    file.close();

    std::cout << "学生数据已保存到 " << filename << std::endl;
}

// 从CSV文件读取学生数据
// 参数：filename - 文件名
// 返回：vector<Student> - 学生列表
std::vector<Student> loadStudents(const std::string& filename) {
    std::vector<Student> students;

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cout << "错误：无法打开文件 " << filename << std::endl;
        return students;
    }

    std::string line;

    // 跳过表头
    std::getline(file, line);

    // 逐行读取学生数据
    while (std::getline(file, line)) {
        // 使用stringstream分割CSV
        std::stringstream ss(line);
        std::string name, ageStr, scoreStr;

        // 读取姓名（逗号分隔）
        std::getline(ss, name, ',');

        // 读取年龄
        std::getline(ss, ageStr, ',');

        // 读取成绩
        std::getline(ss, scoreStr, ',');

        // 转换字符串到数字
        // stoi()：string to int
        // stod()：string to double
        int age = std::stoi(ageStr);
        double score = std::stod(scoreStr);

        // 创建学生对象并添加到列表
        Student s = {name, age, score};
        students.push_back(s);
    }

    file.close();

    return students;
}

// ============================================
// 主函数：演示所有案例
// ============================================

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "文件操作练习 - 实战案例" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    // --------------------------------------------
    // 案例1：配置文件读取器
    // --------------------------------------------

    std::cout << "\n【案例1：配置文件读取器】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // 创建配置文件
    std::ofstream configFile("config.txt");
    configFile << "# 应用配置文件" << std::endl;
    configFile << "app_name=MyApp" << std::endl;
    configFile << "version=1.0.0" << std::endl;
    configFile << "debug=true" << std::endl;
    configFile << "# 数据库配置" << std::endl;
    configFile << "db_host=localhost" << std::endl;
    configFile << "db_port=3306" << std::endl;
    configFile.close();

    // 读取配置
    std::map<std::string, std::string> config = readConfig("config.txt");

    // 显示配置
    std::cout << "配置内容：" << std::endl;
    for (const auto& pair : config) {
        std::cout << "  " << pair.first << " = " << pair.second << std::endl;
    }

    // --------------------------------------------
    // 案例2：日志记录系统
    // --------------------------------------------

    std::cout << "\n【案例2：日志记录系统】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    writeLog("INFO", "程序启动");
    writeLog("INFO", "加载配置文件");
    writeLog("WARNING", "内存使用率较高");
    writeLog("ERROR", "数据库连接失败");
    writeLog("INFO", "程序退出");

    // --------------------------------------------
    // 案例3：文件复制工具
    // --------------------------------------------

    std::cout << "\n【案例3：文件复制工具】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // 创建源文件
    std::ofstream srcFile("source.txt");
    srcFile << "这是第一行" << std::endl;
    srcFile << "这是第二行" << std::endl;
    srcFile << "这是第三行" << std::endl;
    srcFile.close();

    // 复制文件
    copyFile("source.txt", "destination.txt");

    // --------------------------------------------
    // 案例4：文本统计工具
    // --------------------------------------------

    std::cout << "\n【案例4：文本统计工具】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // 创建测试文件
    std::ofstream testFile("test.txt");
    testFile << "Hello World" << std::endl;
    testFile << "This is a test file" << std::endl;
    testFile << "It contains multiple lines" << std::endl;
    testFile.close();

    // 分析文件
    analyzeFile("test.txt");

    // --------------------------------------------
    // 案例5：学生数据库
    // --------------------------------------------

    std::cout << "\n【案例5：学生数据库】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    // 创建学生列表
    std::vector<Student> students = {
        {"张三", 20, 85.5},
        {"李四", 21, 90.0},
        {"王五", 19, 78.5},
        {"赵六", 22, 92.0}
    };

    // 保存到文件
    saveStudents("students.csv", students);

    // 从文件读取
    std::vector<Student> loadedStudents = loadStudents("students.csv");

    // 显示读取的数据
    std::cout << "\n读取的学生数据：" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    for (const Student& s : loadedStudents) {
        std::cout << "姓名：" << s.name
                  << ", 年龄：" << s.age
                  << ", 成绩：" << s.score << std::endl;
    }

    return 0;
}

// ============================================
// 练习题
// ============================================

/*
【练习题】

1. 实现一个简单的记事本程序
   要求：
   - 用户可以输入多行文本
   - 保存到文件 "notes.txt"
   - 可以读取并显示已保存的笔记
   提示：使用循环读取用户输入，直到输入"exit"

2. 实现一个单词计数器
   要求：
   - 读取文本文件
   - 统计每个单词出现的次数
   - 按出现次数排序输出
   提示：使用 map<string, int> 存储单词计数

3. 实现一个简单的通讯录
   要求：
   - 存储姓名、电话、邮箱
   - 支持添加、删除、查找联系人
   - 数据保存到CSV文件
   提示：使用 vector<Contact> 存储联系人

4. 实现文件加密/解密工具
   要求：
   - 读取文件内容
   - 对每个字符进行简单加密（如ASCII码+3）
   - 保存加密后的文件
   - 实现解密功能
   提示：逐字符读取和处理

5. 实现一个简单的日记系统
   要求：
   - 每天的日记保存为独立文件（如 2026-03-16.txt）
   - 支持查看历史日记
   - 支持搜索包含特定关键词的日记
   提示：使用日期作为文件名

6. 实现一个成绩管理系统
   要求：
   - 存储学生的多门课程成绩
   - 计算平均分、最高分、最低分
   - 生成成绩报告
   - 数据保存到文件
   提示：使用结构体和vector

7. 实现一个简单的数据备份工具
   要求：
   - 将指定目录下的所有.txt文件复制到backup目录
   - 在备份文件名后添加时间戳
   提示：需要遍历目录（可以先手动指定文件列表）
*/

// ============================================
// 编译和运行
// ============================================

/*
编译命令：
g++ -std=c++11 35_file_io_exercise.cpp -o 35_file_io_exercise

运行命令：
./35_file_io_exercise        (Linux/Mac)
35_file_io_exercise.exe      (Windows)

预期输出：
- 演示配置文件读取
- 演示日志记录
- 演示文件复制
- 演示文本统计
- 演示学生数据库

生成的文件：
- config.txt：配置文件
- app.log：日志文件
- source.txt：源文件
- destination.txt：复制的文件
- test.txt：测试文件
- students.csv：学生数据
*/


// ============================================
// 练习题
// ============================================

/*
【练习题】

1. 实现一个简单的记事本程序
   要求：
   - 用户可以输入多行文本
   - 保存到文件 notes.txt
   - 可以读取并显示文件内容
   提示：使用 getline() 读取用户输入，使用 ofstream 写入文件

2. 实现一个单词计数器
   要求：
   - 读取文本文件
   - 统计每个单词出现的次数
   - 按出现次数排序输出
   提示：使用 map<string, int> 存储单词和次数

3. 实现一个简单的通讯录
   要求：
   - 保存联系人信息（姓名、电话、邮箱）到CSV文件
   - 可以添加、删除、查找联系人
   - 可以显示所有联系人
   提示：参考学生数据库案例

4. 实现一个文件合并工具
   要求：
   - 将多个文本文件合并成一个文件
   - 每个文件之间用分隔线隔开
   提示：逐个读取源文件，写入目标文件

5. 实现一个简单的加密/解密工具
   要求：
   - 读取文件内容
   - 对每个字符进行简单加密（如ASCII码+3）
   - 保存加密后的文件
   - 可以解密文件
   提示：逐字符读取和处理

6. 实现一个日志分析工具
   要求：
   - 读取日志文件
   - 统计不同级别的日志数量（INFO、WARNING、ERROR）
   - 找出最常见的错误信息
   提示：使用字符串处理和map统计

7. 实现一个简单的数据备份工具
   要求：
   - 将指定目录下的所有.txt文件复制到backup目录
   - 文件名加上时间戳
   提示：需要遍历目录（可以手动指定文件列表）
*/

// ============================================
// 编译和运行
// ============================================

/*
编译命令：
g++ -std=c++11 35_file_io_exercise.cpp -o 35_file_io_exercise

运行命令：
./35_file_io_exercise        (Linux/Mac)
35_file_io_exercise.exe      (Windows)

预期输出：
- 演示配置文件读取
- 演示日志记录
- 演示文件复制
- 演示文本统计
- 演示学生数据库

生成的文件：
- config.txt：配置文件
- app.log：日志文件
- source.txt：源文件
- destination.txt：复制的文件
- test.txt：测试文件
- students.csv：学生数据
*/
