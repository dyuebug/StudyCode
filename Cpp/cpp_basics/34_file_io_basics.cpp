// ============================================
// 文件读写基础 - 超详细注释版
// 日期：2026-03-16（第10天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】文件操作（File I/O）

是什么？
- 文件操作是指程序与磁盘文件之间的数据交互
- 包括读取文件内容、写入数据到文件、修改文件等

为什么需要？
- 数据持久化：程序关闭后数据不丢失
- 配置管理：读取配置文件
- 日志记录：记录程序运行日志
- 数据交换：与其他程序交换数据

核心特点：
- 使用文件流（fstream）进行操作
- 需要打开和关闭文件
- 需要错误处理
*/

#include <iostream>
#include <fstream>   // 文件流头文件
#include <string>

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

// --------------------------------------------
// 1.1 文件流的概念
// --------------------------------------------

/*
【文件流类型】

C++提供了三种文件流类：

1. ifstream（Input File Stream）
   - 用途：从文件读取数据（输入）
   - 继承自：istream
   - 常用操作：打开文件、读取数据、关闭文件

2. ofstream（Output File Stream）
   - 用途：向文件写入数据（输出）
   - 继承自：ostream
   - 常用操作：打开文件、写入数据、关闭文件

3. fstream（File Stream）
   - 用途：读写文件（输入+输出）
   - 继承自：iostream
   - 常用操作：打开文件、读写数据、关闭文件

【文件流与标准流的关系】

标准流          文件流
cin      →     ifstream（从文件读取）
cout     →     ofstream（向文件写入）
*/

// --------------------------------------------
// 1.2 文件打开模式
// --------------------------------------------

/*
【文件打开模式】

模式                  说明
ios::in              打开文件用于读取（默认ifstream）
ios::out             打开文件用于写入（默认ofstream，会清空原内容）
ios::app             追加模式（写入数据追加到文件末尾）
ios::ate             打开文件后定位到文件末尾
ios::trunc           如果文件存在，清空文件内容（默认ios::out）
ios::binary          二进制模式（默认是文本模式）

【组合使用】
可以使用 | 运算符组合多个模式：
- ios::out | ios::app        写入并追加
- ios::in | ios::out         读写模式
- ios::out | ios::binary     二进制写入
*/

// --------------------------------------------
// 1.3 文件操作流程
// --------------------------------------------

/*
【标准文件操作流程】

1. 创建文件流对象
   ifstream inFile;
   ofstream outFile;
   fstream file;

2. 打开文件
   inFile.open("input.txt");
   outFile.open("output.txt");

3. 检查文件是否打开成功
   if (!inFile.is_open()) {
       // 错误处理
   }

4. 读取或写入数据
   inFile >> data;
   outFile << data;

5. 关闭文件
   inFile.close();
   outFile.close();

【重要】：文件操作完成后必须关闭文件，释放资源
*/

// ============================================
// 第二部分：基础操作实践
// ============================================

int main() {
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "文件读写基础 - 实践演示" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // --------------------------------------------
    // 示例1：写入文件（ofstream）
    // --------------------------------------------
    
    std::cout << "\n【示例1：写入文件】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 创建输出文件流对象
    // ofstream：Output File Stream（输出文件流）
    std::ofstream outFile;
    
    // 打开文件用于写入
    // 参数：文件名（字符串）
    // 如果文件不存在，会自动创建
    // 如果文件存在，会清空原内容（默认ios::trunc模式）
    outFile.open("test_output.txt");
    
    // 检查文件是否打开成功
    // is_open()：返回bool值，true表示成功，false表示失败
    if (!outFile.is_open()) {
        std::cout << "错误：无法打开文件 test_output.txt" << std::endl;
        return 1;
    }
    
    std::cout << "文件 test_output.txt 打开成功" << std::endl;
    
    // 写入数据到文件
    // 使用 << 运算符，就像使用cout一样
    outFile << "Hello, File I/O!" << std::endl;
    outFile << "这是第二行" << std::endl;
    outFile << "数字：" << 42 << std::endl;
    outFile << "浮点数：" << 3.14159 << std::endl;
    
    std::cout << "数据已写入文件" << std::endl;
    
    // 关闭文件
    // 重要：文件操作完成后必须关闭，释放资源
    outFile.close();
    
    std::cout << "文件已关闭" << std::endl;
    
    // --------------------------------------------
    // 示例2：读取文件（ifstream）
    // --------------------------------------------
    
    std::cout << "\n【示例2：读取文件】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 创建输入文件流对象
    // ifstream：Input File Stream（输入文件流）
    std::ifstream inFile;
    
    // 打开文件用于读取
    inFile.open("test_output.txt");
    
    // 检查文件是否打开成功
    if (!inFile.is_open()) {
        std::cout << "错误：无法打开文件 test_output.txt" << std::endl;
        return 1;
    }
    
    std::cout << "文件 test_output.txt 打开成功" << std::endl;
    std::cout << "\n文件内容：" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 方法1：逐行读取（推荐）
    // getline()：读取一行文本
    // 参数1：输入流对象
    // 参数2：存储读取内容的string变量
    std::string line;
    while (std::getline(inFile, line)) {
        std::cout << line << std::endl;
    }
    
    std::cout << std::string(60, '-') << std::endl;
    
    // 关闭文件
    inFile.close();
    
    std::cout << "文件已关闭" << std::endl;
    
    // --------------------------------------------
    // 示例3：追加模式写入（ios::app）
    // --------------------------------------------
    
    std::cout << "\n【示例3：追加模式写入】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 使用追加模式打开文件
    // ios::app：append模式，数据追加到文件末尾，不清空原内容
    std::ofstream appendFile("test_output.txt", std::ios::app);
    
    if (!appendFile.is_open()) {
        std::cout << "错误：无法打开文件" << std::endl;
        return 1;
    }
    
    // 追加新内容
    appendFile << "这是追加的第一行" << std::endl;
    appendFile << "这是追加的第二行" << std::endl;
    
    std::cout << "数据已追加到文件" << std::endl;
    
    appendFile.close();
    
    // 再次读取文件，查看追加结果
    std::ifstream readFile("test_output.txt");
    
    if (readFile.is_open()) {
        std::cout << "\n追加后的文件内容：" << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        
        while (std::getline(readFile, line)) {
            std::cout << line << std::endl;
        }
        
        std::cout << std::string(60, '-') << std::endl;
        readFile.close();
    }
    
    // --------------------------------------------
    // 示例4：读写模式（fstream）
    // --------------------------------------------
    
    std::cout << "\n【示例4：读写模式】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 创建一个新文件用于读写演示
    std::ofstream createFile("readwrite.txt");
    createFile << "初始内容：行1" << std::endl;
    createFile << "初始内容：行2" << std::endl;
    createFile.close();
    
    // 使用fstream同时读写文件
    // ios::in | ios::out：读写模式
    std::fstream rwFile("readwrite.txt", std::ios::in | std::ios::out);
    
    if (!rwFile.is_open()) {
        std::cout << "错误：无法打开文件" << std::endl;
        return 1;
    }
    
    // 先读取内容
    std::cout << "原始内容：" << std::endl;
    while (std::getline(rwFile, line)) {
        std::cout << line << std::endl;
    }
    
    // 清除错误标志和EOF标志
    // clear()：清除流的错误状态
    rwFile.clear();
    
    // 移动到文件末尾
    // seekp()：设置写入位置
    // 参数1：偏移量
    // 参数2：基准位置（ios::beg开头，ios::end末尾，ios::cur当前）
    rwFile.seekp(0, std::ios::end);
    
    // 追加新内容
    rwFile << "追加内容：行3" << std::endl;
    
    std::cout << "\n已追加新内容" << std::endl;
    
    rwFile.close();
    
    // --------------------------------------------
    // 示例5：逐词读取
    // --------------------------------------------
    
    std::cout << "\n【示例5：逐词读取】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 创建测试文件
    std::ofstream wordFile("words.txt");
    wordFile << "apple banana cherry" << std::endl;
    wordFile << "dog elephant fox" << std::endl;
    wordFile.close();
    
    // 逐词读取
    std::ifstream wordInput("words.txt");
    
    if (wordInput.is_open()) {
        std::string word;
        std::cout << "逐词读取：" << std::endl;
        
        // 使用 >> 运算符逐词读取（以空格分隔）
        while (wordInput >> word) {
            std::cout << "读取到单词：" << word << std::endl;
        }
        
        wordInput.close();
    }
    
    // --------------------------------------------
    // 示例6：检查文件是否存在
    // --------------------------------------------
    
    std::cout << "\n【示例6：检查文件是否存在】" << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // 尝试打开一个不存在的文件
    std::ifstream checkFile("nonexistent.txt");
    
    if (checkFile.is_open()) {
        std::cout << "文件存在" << std::endl;
        checkFile.close();
    } else {
        std::cout << "文件不存在或无法打开" << std::endl;
    }
    
    // 检查已存在的文件
    std::ifstream checkFile2("test_output.txt");
    
    if (checkFile2.is_open()) {
        std::cout << "文件 test_output.txt 存在" << std::endl;
        checkFile2.close();
    } else {
        std::cout << "文件 test_output.txt 不存在" << std::endl;
    }
    
    return 0;
}

// ============================================
// 第三部分：深入理解（可选）
// ============================================

/*
==============================================
🔍 深入理解：文件流的底层原理（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【文件流的缓冲机制】

1. 为什么需要缓冲？
   - 磁盘I/O比内存操作慢得多（慢1000倍以上）
   - 频繁的小量读写会严重影响性能
   - 缓冲区可以批量处理数据，减少磁盘访问次数

2. 缓冲区如何工作？
   写入过程：
   - 数据先写入内存缓冲区
   - 缓冲区满了或调用flush()时，才真正写入磁盘
   - close()会自动flush缓冲区

   读取过程：
   - 一次性从磁盘读取一大块数据到缓冲区
   - 程序从缓冲区读取数据，速度快
   - 缓冲区空了，再从磁盘读取下一块

3. 手动刷新缓冲区
   - flush()：立即将缓冲区内容写入磁盘
   - endl：输出换行符并刷新缓冲区
   - '\n'：只输出换行符，不刷新缓冲区

【文件指针】

1. 读指针（get pointer）
   - 用于读取操作
   - tellg()：获取当前读指针位置
   - seekg()：设置读指针位置

2. 写指针（put pointer）
   - 用于写入操作
   - tellp()：获取当前写指针位置
   - seekp()：设置写指针位置

3. 指针位置
   - ios::beg：文件开头
   - ios::cur：当前位置
   - ios::end：文件末尾

示例：
file.seekg(0, ios::beg);  // 移动到文件开头
file.seekg(10, ios::cur); // 从当前位置向后移动10字节
file.seekg(-5, ios::end); // 从文件末尾向前移动5字节

【文本模式 vs 二进制模式】

1. 文本模式（默认）
   - 自动处理换行符转换
   - Windows：\r\n ↔ \n
   - 适合文本文件

2. 二进制模式（ios::binary）
   - 不做任何转换，原样读写
   - 适合图片、音频、视频等二进制文件
   - 使用read()和write()函数

【错误处理】

文件流有多个状态标志：
- good()：一切正常
- eof()：到达文件末尾
- fail()：操作失败（如类型不匹配）
- bad()：严重错误（如硬件故障）

示例：
if (file.fail()) {
    std::cout << "操作失败" << std::endl;
    file.clear();  // 清除错误标志
}

==============================================
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
【常见错误1：忘记检查文件是否打开成功】

❌ 错误代码：
std::ofstream file("output.txt");
file << "Hello";  // 如果文件打开失败，这里会静默失败
file.close();

结果：如果文件无法打开（如权限不足），数据不会写入，但程序不报错

原因：没有检查文件是否打开成功

✅ 正确代码：
std::ofstream file("output.txt");

if (!file.is_open()) {
    std::cout << "错误：无法打开文件" << std::endl;
    return 1;
}

file << "Hello";
file.close();

结果：如果文件打开失败，会提示错误信息

记忆技巧/预防措施：
- 打开文件后立即检查 is_open()
- 养成习惯：打开文件 → 检查 → 操作 → 关闭
- 使用RAII（构造函数打开，析构函数关闭）

────────────────────────────────────────────

【常见错误2：忘记关闭文件】

❌ 错误代码：
void writeFile() {
    std::ofstream file("output.txt");
    file << "Hello";
    // 忘记关闭文件
}

结果：
- 文件可能没有完全写入（缓冲区未刷新）
- 资源泄漏（文件句柄未释放）
- 其他程序可能无法访问该文件

原因：没有调用 close()

✅ 正确代码：
void writeFile() {
    std::ofstream file("output.txt");
    
    if (file.is_open()) {
        file << "Hello";
        file.close();  // 显式关闭文件
    }
}

结果：文件正确关闭，数据完全写入

记忆技巧/预防措施：
- 文件操作完成后立即关闭
- 使用作用域自动关闭（文件流对象销毁时自动关闭）
- 或使用 RAII 封装

────────────────────────────────────────────

【常见错误3：使用错误的打开模式】

❌ 错误代码：
// 想要追加内容，但使用了默认模式
std::ofstream file("log.txt");  // 默认会清空文件
file << "新日志";
file.close();

结果：原有日志被清空，只剩下"新日志"

原因：ofstream默认使用 ios::trunc 模式，会清空文件

✅ 正确代码：
// 使用追加模式
std::ofstream file("log.txt", std::ios::app);
file << "新日志" << std::endl;
file.close();

结果：新日志追加到文件末尾，原有内容保留

记忆技巧/预防措施：
- 追加内容：使用 ios::app
- 清空重写：使用默认模式或 ios::trunc
- 读写模式：使用 ios::in | ios::out
- 明确指定打开模式，不依赖默认行为

────────────────────────────────────────────

【常见错误4：读取文件后忘记重置流状态】

❌ 错误代码：
std::fstream file("data.txt", std::ios::in | std::ios::out);

// 读取到文件末尾
std::string line;
while (std::getline(file, line)) {
    std::cout << line << std::endl;
}

// 尝试写入，但失败了
file << "新内容";  // 写入失败！

结果：写入失败，因为流处于EOF状态

原因：读取到文件末尾后，流的EOF标志被设置，需要清除

✅ 正确代码：
std::fstream file("data.txt", std::ios::in | std::ios::out);

// 读取到文件末尾
std::string line;
while (std::getline(file, line)) {
    std::cout << line << std::endl;
}

// 清除错误标志和EOF标志
file.clear();

// 移动到文件末尾
file.seekp(0, std::ios::end);

// 写入成功
file << "新内容" << std::endl;

结果：写入成功

记忆技巧/预防措施：
- 读取后写入：先 clear()，再 seekp()
- 理解流的状态标志（eof、fail、bad）
- 使用 good() 检查流状态

────────────────────────────────────────────

【常见错误5：路径分隔符问题】

❌ 错误代码（Windows）：
std::ofstream file("C:\data\output.txt");  // 错误！\d 和 \o 是转义字符

结果：编译错误或文件路径错误

原因：反斜杠 \ 是转义字符

✅ 正确代码：
// 方法1：使用双反斜杠
std::ofstream file("C:\data\output.txt");

// 方法2：使用正斜杠（推荐，跨平台）
std::ofstream file("C:/data/output.txt");

// 方法3：使用原始字符串（C++11）
std::ofstream file(R"(C:\data\output.txt)");

结果：文件路径正确

记忆技巧/预防措施：
- 推荐使用正斜杠 /（跨平台）
- 或使用原始字符串 R"(...)"
- 避免使用单个反斜杠

────────────────────────────────────────────

【常见错误6：文件不存在时读取】

❌ 错误代码：
std::ifstream file("nonexistent.txt");
std::string content;
file >> content;  // 读取失败，但没有检查
std::cout << content << std::endl;  // 输出空字符串或垃圾值

结果：程序不报错，但读取的是无效数据

原因：文件不存在，但没有检查

✅ 正确代码：
std::ifstream file("nonexistent.txt");

if (!file.is_open()) {
    std::cout << "错误：文件不存在" << std::endl;
    return 1;
}

std::string content;
file >> content;
std::cout << content << std::endl;

结果：提示文件不存在，避免读取无效数据

记忆技巧/预防措施：
- 读取前检查文件是否存在
- 使用 is_open() 检查
- 提供友好的错误提示

────────────────────────────────────────────

【常见错误7：混淆 >> 和 getline()】

❌ 错误代码：
std::ifstream file("data.txt");
std::string line;

// 想读取整行，但使用了 >>
file >> line;  // 只读取到第一个空格

结果：只读取了第一个单词，不是整行

原因：>> 运算符以空格分隔，getline() 以换行符分隔

✅ 正确代码：
std::ifstream file("data.txt");
std::string line;

// 读取整行
std::getline(file, line);

结果：读取整行内容

记忆技巧/预防措施：
- 读取整行：使用 getline()
- 读取单词：使用 >>
- 理解两者的区别
*/

// ============================================
// 第五部分：函数卡片速查
// ============================================

/*
【文件流类】

1. ifstream（输入文件流）
   作用：从文件读取数据
   常用方法：open(), is_open(), close(), getline(), >>
   示例：
   std::ifstream file("input.txt");
   std::string line;
   std::getline(file, line);

2. ofstream（输出文件流）
   作用：向文件写入数据
   常用方法：open(), is_open(), close(), <<
   示例：
   std::ofstream file("output.txt");
   file << "Hello" << std::endl;

3. fstream（文件流）
   作用：读写文件
   常用方法：open(), is_open(), close(), getline(), <<, >>
   示例：
   std::fstream file("data.txt", std::ios::in | std::ios::out);

────────────────────────────────────────────

【常用成员函数】

1. open()
   语法：void open(const char* filename, ios::openmode mode)
   参数：
     - filename：文件名（字符串）
     - mode：打开模式（可选，默认根据流类型）
   作用：打开文件
   示例：file.open("data.txt", std::ios::app);

2. is_open()
   语法：bool is_open()
   返回：true表示文件打开成功，false表示失败
   作用：检查文件是否打开
   示例：if (file.is_open()) { ... }

3. close()
   语法：void close()
   作用：关闭文件
   示例：file.close();

4. getline()
   语法：istream& getline(istream& is, string& str)
   参数：
     - is：输入流对象
     - str：存储读取内容的string变量
   返回：输入流对象（用于判断是否读取成功）
   作用：读取一行文本
   示例：std::getline(file, line);

5. seekg() / seekp()
   语法：istream& seekg(streampos pos)
         ostream& seekp(streampos pos)
   参数：
     - pos：目标位置
     - offset：偏移量
     - dir：基准位置（ios::beg, ios::cur, ios::end）
   作用：移动读取/写入位置
   示例：
   file.seekg(0, std::ios::beg);  // 移动到文件开头
   file.seekp(0, std::ios::end);  // 移动到文件末尾

6. tellg() / tellp()
   语法：streampos tellg()
         streampos tellp()
   返回：当前读取/写入位置
   作用：获取当前位置
   示例：
   streampos pos = file.tellg();

7. good() / eof() / fail() / bad()
   语法：bool good() / eof() / fail() / bad()
   返回：流的状态
   作用：检查流状态
   示例：
   if (file.eof()) { std::cout << "到达文件末尾" << std::endl; }

8. clear()
   语法：void clear()
   作用：清除错误标志
   示例：file.clear();
*/

// ============================================
// 第六部分：练习题
// ============================================

/*
【练习题】

1. 创建一个程序，写入你的个人信息到文件 "profile.txt"
   要求：包含姓名、年龄、爱好
   提示：使用 ofstream

2. 读取 "profile.txt" 文件并显示内容
   要求：逐行读取并打印
   提示：使用 ifstream 和 getline()

3. 创建一个日志系统，每次运行程序时追加一条日志
   要求：日志格式 "[时间] 程序运行"
   提示：使用 ios::app 模式

4. 实现一个简单的配置文件读取器
   要求：读取 "config.txt"，格式为 "key=value"
   提示：使用 getline() 和字符串处理

5. 实现文件复制功能
   要求：将 "source.txt" 复制到 "dest.txt"
   提示：逐行读取源文件，写入目标文件

6. 统计文件的行数、单词数、字符数
   要求：读取文件并统计
   提示：使用 getline() 统计行数，>> 统计单词数

7. 实现一个简单的文本编辑器
   要求：读取文件，修改某一行，保存回文件
   提示：读取所有行到vector，修改后写回文件
*/

// ============================================
// 编译和运行
// ============================================

/*
编译命令：
g++ -std=c++11 34_file_io_basics.cpp -o 34_file_io_basics

运行命令：
./34_file_io_basics        (Linux/Mac)
34_file_io_basics.exe      (Windows)

预期输出：
- 创建并写入 test_output.txt
- 读取并显示文件内容
- 演示追加模式
- 演示读写模式
- 演示逐词读取
- 检查文件是否存在

生成的文件：
- test_output.txt
- readwrite.txt
- words.txt
*/
