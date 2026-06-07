// ============================================
// 05_shared_memory.cpp
// 日期：2026-06-07（第65天）
// 主题：Linux 共享内存（mmap / shm_open）
// 难度：⭐⭐ 中级
// 注意：需在 Linux/WSL 环境编译运行
// 编译：g++ -std=c++17 -o 05_shm 05_shared_memory.cpp -lrt -lpthread
// ============================================

/*
【核心概念】

是什么？
- 共享内存：多个进程映射同一块物理内存，直接读写，无需数据拷贝
- mmap()：将文件或匿名内存映射到进程地址空间
- shm_open()：POSIX共享内存，用于无亲缘关系的进程

为什么需要？
- 最快的IPC方式：无内核拷贝，带宽接近内存速度
- 文件映射：零拷贝读写大文件（数据库、视频处理常用）

核心特点：
- 共享内存本身不提供同步，需配合互斥锁/信号量
- mmap匿名映射只适用于 fork() 后的父子进程
- shm_open 创建的共享内存在 /dev/shm/ 下可见
*/

#include <iostream>
#include <string>
using namespace std;

void showConcepts() {
    cout << R"(
============================================================
共享内存核心概念
============================================================

【mmap() 函数卡片】

  #include <sys/mman.h>
  void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset);

  参数：
    addr   — 建议映射地址，通常传 nullptr（由内核选择）
    len    — 映射长度（字节）
    prot   — 保护标志：PROT_READ | PROT_WRITE | PROT_EXEC
    flags  — 映射类型（见下表）
    fd     — 文件描述符（匿名映射传 -1）
    offset — 文件偏移（匿名映射传 0）

  flags 核心选项：
    MAP_SHARED   — 修改对其他映射该区域的进程可见（共享映射）
    MAP_PRIVATE  — 写时复制，修改不影响其他进程/文件（私有映射）
    MAP_ANONYMOUS— 匿名映射（不关联文件，需配合fd=-1）

  返回：成功返回映射地址，失败返回 MAP_FAILED

  解除映射：munmap(addr, len)

【三种使用场景】

  1. 匿名共享映射（父子进程）：
     MAP_SHARED | MAP_ANONYMOUS, fd=-1

  2. 文件映射（零拷贝读写）：
     MAP_SHARED, fd=open(file)

  3. POSIX共享内存（无亲缘进程）：
     shm_open() → ftruncate() → mmap()

============================================================
)";
}

void showAnonymousMmap() {
    cout << R"(
------------------------------------------------------------
匿名共享内存：父子进程共享计数器
------------------------------------------------------------

// ---- 真实Linux代码 ----
#include <sys/mman.h>
#include <sys/wait.h>

int main() {
    // 创建共享内存（fork之前！）
    int* counter = (int*)mmap(
        nullptr, sizeof(int),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,  // ★ 共享+匿名
        -1, 0
    );
    *counter = 0;

    pid_t pid = fork();

    if (pid == 0) {
        // 子进程：递增计数器100次
        for (int i = 0; i < 100; ++i)
            (*counter)++;  // ⚠ 无同步，实际项目需加互斥锁
        exit(0);
    } else {
        // 父进程：也递增100次
        for (int i = 0; i < 100; ++i)
            (*counter)++;
        wait(nullptr);
        cout << "最终计数: " << *counter << endl;
        // 期望200，但因为竞争条件，实际可能更小
    }

    munmap(counter, sizeof(int));  // 解除映射
    return 0;
}
)";
}

void showFileMmap() {
    cout << R"(
------------------------------------------------------------
文件映射：零拷贝读写大文件
------------------------------------------------------------

// ---- 真实Linux代码 ----
#include <sys/mman.h>
#include <fcntl.h>

// 读文件（零拷贝）
int fd = open("bigfile.bin", O_RDONLY);
struct stat st; fstat(fd, &st);

char* data = (char*)mmap(nullptr, st.st_size,
    PROT_READ, MAP_SHARED, fd, 0);
close(fd);  // fd可以关闭，映射仍有效

// 直接操作内存，内核按需从磁盘加载页
cout << "文件前4字节: " << hex
     << (int)data[0] << (int)data[1]
     << (int)data[2] << (int)data[3] << endl;

munmap(data, st.st_size);

// 修改文件（零拷贝写）
fd = open("file.bin", O_RDWR);
char* p = (char*)mmap(nullptr, 4096,
    PROT_READ | PROT_WRITE,
    MAP_SHARED,  // ★ MAP_SHARED才会写回文件
    fd, 0);

p[0] = 0xFF;  // 直接修改，等价于写文件
msync(p, 4096, MS_SYNC);  // 强制刷新到磁盘
munmap(p, 4096);
close(fd);
)";
}

void showPosixShm() {
    cout << R"(
------------------------------------------------------------
POSIX 共享内存：无亲缘关系进程通信
------------------------------------------------------------

// ---- 进程A（创建者）----
#include <sys/mman.h>
#include <fcntl.h>

struct SharedData {
    pthread_mutex_t mutex;  // 进程间互斥锁（需设置PTHREAD_PROCESS_SHARED）
    int value;
    char message[64];
};

// 创建共享内存对象（/dev/shm/myshm 下可见）
int fd = shm_open("/myshm", O_CREAT | O_RDWR, 0666);
ftruncate(fd, sizeof(SharedData));  // 设置大小

SharedData* shm = (SharedData*)mmap(nullptr, sizeof(SharedData),
    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
close(fd);

// 初始化进程间互斥锁
pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);
pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);  // ★
pthread_mutex_init(&shm->mutex, &attr);

shm->value = 42;
strcpy(shm->message, "hello from A");

// ---- 进程B（访问者）----
int fd = shm_open("/myshm", O_RDWR, 0);
SharedData* shm = (SharedData*)mmap(nullptr, sizeof(SharedData),
    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

pthread_mutex_lock(&shm->mutex);
cout << "读取到: " << shm->value << " " << shm->message << endl;
shm->value++;
pthread_mutex_unlock(&shm->mutex);

// ---- 清理（进程A）----
shm_unlink("/myshm");  // 删除共享内存对象
)";
}

void showMistakes() {
    cout << R"(
============================================================
常见错误和陷阱 ⭐⭐⭐⭐⭐
============================================================

❌ 错误1：不对共享内存加锁导致数据竞争
  (*counter)++;  // ❌ 非原子操作，多进程并发时数据损坏
  // ✅ 用 pthread_mutex（PTHREAD_PROCESS_SHARED）或原子操作

❌ 错误2：mmap大小与文件大小不符
  mmap(nullptr, 8192, ..., fd, 0);  // ❌ 文件只有4096字节，访问后半段→SIGBUS
  // ✅ 用 fstat() 获取文件大小，或先 ftruncate() 扩展

❌ 错误3：fork后在子进程中munmap共享映射
  munmap(shared, size);  // ❌ 子进程unmap会影响父进程（MAP_SHARED）
  // ✅ MAP_SHARED的munmap只影响当前进程的映射，但要注意操作顺序

❌ 错误4：shm_open后忘记shm_unlink
  shm_open("/myshm", O_CREAT, ...);
  // 程序退出，/dev/shm/myshm 仍然存在，下次创建失败
  // ✅ 程序退出前调用 shm_unlink("/myshm")

❌ 错误5：用普通pthread_mutex（未设PROCESS_SHARED）做进程间同步
  pthread_mutex_init(&shm->mutex, nullptr);  // ❌ 默认只在同进程内有效
  // ✅ 必须设置 PTHREAD_PROCESS_SHARED 属性

============================================================
)";
}

int main() {
    cout << string(60, '=') << endl;
    cout << "05_shared_memory.cpp - Linux共享内存" << endl;
    cout << "（概念演示版，完整代码需在Linux/WSL中运行）" << endl;
    cout << string(60, '=') << endl;

    showConcepts();
    showAnonymousMmap();
    showFileMmap();
    showPosixShm();
    showMistakes();

    cout << R"(
============================================================
练习题
============================================================
1. 用 mmap 匿名映射实现父子进程共享一个结构体，包含计数器和字符串，
   用进程间互斥锁保护并发访问

2. 用文件映射实现一个简单的"内存数据库"：
   将结构体数组映射到文件，进程重启后数据仍然存在

3. 用 shm_open 实现生产者-消费者：
   进程A不断写数据到共享内存环形缓冲区，进程B读取并打印

4. 对比性能：分别用管道和mmap传输10MB数据，测量耗时
============================================================
)";
    return 0;
}
