// ============================================
// 03_file_operations.cpp
// 日期：2026-06-07（第64天）
// 主题：POSIX 文件操作
// 难度：⭐⭐ 中级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 03_file 03_file_operations.cpp
// ============================================

/*
【核心概念】

是什么？
- POSIX 文件API：Linux/Unix 标准文件操作接口（比C标准库更底层）
- 文件描述符（fd）：内核用整数表示打开的文件/管道/socket
- 与C标准库对比：FILE* 是对 fd 的封装，增加了用户态缓冲

为什么需要？
- 嵌入式Linux开发必须掌握
- 网络编程（socket也是fd）、IPC（管道/FIFO）都用相同接口
- 更精细的控制：非阻塞IO、文件锁、零拷贝等

核心特点：
- fd 0/1/2 = stdin/stdout/stderr（固定）
- open() 总是分配当前最小可用的fd编号
- 所有IO操作（文件/管道/socket）使用相同的 read/write 接口
*/

#include <iostream>
#include <string>
using namespace std;

// ============================================
// 第一部分：核心概念详解
// ============================================

void showConcepts() {
    cout << R"(
============================================================
POSIX 文件操作 - 核心概念
============================================================

【文件描述符（File Descriptor）】

  内核为每个进程维护一张"文件描述符表"：
  ┌────┬─────────────────────────┐
  │ fd │ 指向的内核文件对象       │
  ├────┼─────────────────────────┤
  │  0 │ 标准输入 stdin          │
  │  1 │ 标准输出 stdout         │
  │  2 │ 标准错误 stderr         │
  │  3 │ open()后的第一个文件    │
  │  4 │ ...                     │
  └────┴─────────────────────────┘

  fd 特点：
  - 继承：fork()后子进程拥有父进程fd表的副本
  - 复用：close(fd)后，该编号可被下一次open()复用
  - 共享偏移：dup()/fork()后的fd共享文件偏移量

【open() 函数卡片】

  #include <fcntl.h>
  int open(const char* path, int flags, mode_t mode);

  常用flags（可用|组合）：
    O_RDONLY  — 只读
    O_WRONLY  — 只写
    O_RDWR    — 读写
    O_CREAT   — 不存在则创建（需要mode参数）
    O_TRUNC   — 清空文件内容
    O_APPEND  — 追加写入
    O_NONBLOCK— 非阻塞模式

  mode（创建时的权限，受umask影响）：
    0644 = rw-r--r--（常用文件权限）
    0755 = rwxr-xr-x（常用可执行权限）

  返回值：fd（>=0）成功，-1失败（查errno）

【read/write 函数卡片】

  ssize_t read(int fd, void* buf, size_t count);
  返回：实际读取字节数；0=EOF；-1=错误

  ssize_t write(int fd, const void* buf, size_t count);
  返回：实际写入字节数；-1=错误

  ⚠ 注意：read/write 可能读写少于 count 字节（短读/短写）
  ✅ 必须用循环确保读写完整数据！

【lseek() 函数卡片】

  off_t lseek(int fd, off_t offset, int whence);

  whence：
    SEEK_SET — 从文件开头偏移
    SEEK_CUR — 从当前位置偏移
    SEEK_END — 从文件末尾偏移

  返回：新的文件偏移量；-1=错误（管道/socket不支持lseek）

============================================================
)";
}

// ============================================
// 第二部分：代码示例
// ============================================

void showBasicFileOps() {
    cout << R"(
------------------------------------------------------------
基本文件读写
------------------------------------------------------------

// ---- 真实Linux代码 ----
#include <fcntl.h>
#include <unistd.h>

// 写文件
int fd = open("test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
if (fd < 0) { perror("open失败"); return 1; }

const char* data = "Hello, Linux File IO!\n";
ssize_t n = write(fd, data, strlen(data));
cout << "写入" << n << "字节" << endl;
close(fd);

// 读文件（完整读取循环）
fd = open("test.txt", O_RDONLY);
char buf[128];
ssize_t total = 0, ret;
while ((ret = read(fd, buf + total, sizeof(buf) - total)) > 0)
    total += ret;  // ★ 循环读，防止短读
buf[total] = '\0';
cout << "读取内容: " << buf;
close(fd);
)";
}

void showLseek() {
    cout << R"(
------------------------------------------------------------
lseek 文件定位
------------------------------------------------------------

int fd = open("test.txt", O_RDWR);

// 获取文件大小
off_t size = lseek(fd, 0, SEEK_END);
cout << "文件大小: " << size << "字节" << endl;

// 回到开头
lseek(fd, 0, SEEK_SET);

// 跳过前5字节读取
lseek(fd, 5, SEEK_SET);
char buf[64] = {};
read(fd, buf, sizeof(buf));
cout << "跳过5字节后: " << buf << endl;

// 稀疏文件：lseek跳过大量空间
lseek(fd, 1024*1024, SEEK_SET);  // 跳到1MB处
write(fd, "end", 3);  // 文件大小变为1MB+3，中间是"空洞"

close(fd);
)";
}

void showFcntl() {
    cout << R"(
------------------------------------------------------------
fcntl() 文件控制
------------------------------------------------------------

#include <fcntl.h>

// 1. 设置非阻塞模式
int flags = fcntl(fd, F_GETFL);          // 获取当前flags
fcntl(fd, F_SETFL, flags | O_NONBLOCK);  // 添加非阻塞标志

// 非阻塞读：没有数据时立即返回-1，errno=EAGAIN
ssize_t n = read(fd, buf, size);
if (n < 0 && errno == EAGAIN)
    cout << "暂无数据，稍后重试" << endl;

// 2. 设置 FD_CLOEXEC（exec后自动关闭）
fcntl(fd, F_SETFD, FD_CLOEXEC);
// 等价于 open(path, O_RDONLY | O_CLOEXEC)

// 3. 文件锁（建议性锁，非强制）
struct flock fl;
fl.l_type   = F_WRLCK;   // F_RDLCK读锁 / F_WRLCK写锁 / F_UNLCK解锁
fl.l_whence = SEEK_SET;
fl.l_start  = 0;
fl.l_len    = 0;          // 0=锁定整个文件

fcntl(fd, F_SETLKW, &fl); // F_SETLK非阻塞 / F_SETLKW阻塞等待
// ... 操作文件 ...
fl.l_type = F_UNLCK;
fcntl(fd, F_SETLK, &fl);  // 解锁
)";
}

void showPOSIXvsCStdlib() {
    cout << R"(
------------------------------------------------------------
POSIX API vs C标准库 FILE* 对比
------------------------------------------------------------

┌──────────────┬──────────────────┬─────────────────────┐
│              │ POSIX (fd)       │ C标准库 (FILE*)     │
├──────────────┼──────────────────┼─────────────────────┤
│ 打开         │ open()           │ fopen()             │
│ 读取         │ read()           │ fread()/fgets()     │
│ 写入         │ write()          │ fwrite()/fprintf()  │
│ 定位         │ lseek()          │ fseek()             │
│ 关闭         │ close()          │ fclose()            │
│ 缓冲         │ 无（内核缓冲）   │ 有（用户态缓冲）    │
│ 格式化       │ 需要自己处理     │ printf/scanf系列    │
│ 适用场景     │ 网络/IPC/底层    │ 普通文件读写        │
├──────────────┼──────────────────┼─────────────────────┤
│ 性能         │ 少量大块读写更快 │ 大量小块读写更快    │
└──────────────┴──────────────────┴─────────────────────┘

FILE* 底层就是对 fd 的封装：
  fileno(fp) → 获取FILE*对应的fd
  fdopen(fd, "r") → 把fd包装成FILE*
)";
}

// ============================================
// 第四部分：常见错误
// ============================================

void showMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

❌ 错误1：不处理短读（read返回值小于请求字节数）
  read(fd, buf, 100);  // ❌ 可能只读到50字节
  // ✅ 必须循环读直到返回0（EOF）或-1（错误）
  ssize_t total = 0, n;
  while (total < 100 && (n = read(fd, buf+total, 100-total)) > 0)
      total += n;

❌ 错误2：open()用O_CREAT但忘记mode参数
  open("file", O_WRONLY|O_CREAT);  // ❌ mode缺失，权限随机
  open("file", O_WRONLY|O_CREAT, 0644);  // ✅

❌ 错误3：不检查open()返回值就使用fd
  int fd = open("file", O_RDONLY);
  read(fd, buf, n);  // ❌ 若fd=-1，read读取fd=-1会失败
  // ✅ 每次open后必须检查 if(fd < 0) { perror(); ... }

❌ 错误4：close()失败未处理（NFS等网络文件系统）
  close(fd);  // ❌ 网络文件系统close可能失败，丢失最后的写入
  // ✅ if(close(fd) < 0) { perror("close"); }

❌ 错误5：混用FILE*和fd操作同一文件
  FILE* fp = fopen("file", "w");
  int fd = fileno(fp);
  write(fd, "A", 1);    // 直接写fd（绕过FILE*缓冲区）
  fprintf(fp, "B");     // 通过FILE*写（有缓冲区）
  fclose(fp);           // ❌ 可能"B"先于"A"写入（缓冲区刷新顺序）
  // ✅ 要么全用FILE*，要么全用fd，不要混用

============================================================
)";
}

int main() {
    cout << string(60, '=') << endl;
    cout << "03_file_operations.cpp - POSIX文件操作" << endl;
    cout << "（概念演示版，完整代码需在Linux/WSL中运行）" << endl;
    cout << string(60, '=') << endl;

    showConcepts();
    showBasicFileOps();
    showLseek();
    showFcntl();
    showPOSIXvsCStdlib();
    showMistakes();

    cout << R"(
============================================================
练习题
============================================================
1. 实现 cp 命令：用open/read/write把源文件复制到目标文件，
   注意循环读写处理大文件，复制完验证文件大小一致

2. 实现文件行数统计（类似 wc -l）：
   用read()读取原始字节，统计'\n'出现次数

3. 用文件锁实现"只允许一个实例运行"的守护进程：
   启动时尝试对 /var/run/myapp.pid 加写锁，失败则退出

4. 实现一个简单的日志轮转：
   写满1KB后关闭当前文件，重命名为.old，再创建新文件继续写
============================================================
)";
    return 0;
}
