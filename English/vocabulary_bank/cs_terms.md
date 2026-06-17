# 计算机专业术语库
**Last Updated**: 2026-06-16  
**Coverage**: 数据结构、算法、操作系统、计算机网络、数据库

---

## 📊 术语分类

- **DS (Data Structures)**: 数据结构
- **ALGO (Algorithms)**: 算法
- **OS (Operating Systems)**: 操作系统
- **NET (Computer Networks)**: 计算机网络
- **DB (Database)**: 数据库
- **SE (Software Engineering)**: 软件工程

---

## Week 0-1: 操作系统基础术语

### Day 1: Process Management (进程管理)

#### 1. process `/ˈprɑːses/`
- **CS义**: n. 进程 (an instance of a running program)
- **技术定义**: A process is an executing program with its own memory space and system resources.
- **考研对比**:
  - 📖 考研义: n. 过程，进程；v. 处理，加工
  - 💻 CS义: n. 进程（操作系统概念）
  - 🗣️ 日常义: n. 流程，步骤
- **核心搭配**:
  - process creation (进程创建)
  - process scheduling (进程调度)
  - process termination (进程终止)
- **例句**: "The operating system allocates CPU time to each process."

#### 2. thread `/θred/`
- **CS义**: n. 线程 (a lightweight unit of execution within a process)
- **技术定义**: A thread is the smallest unit of processing that can be scheduled by an OS. Multiple threads within a process share the same address space.
- **对比**: process vs thread
  - Process: 独立地址空间，资源隔离，创建开销大
  - Thread: 共享地址空间，轻量级，创建开销小
- **核心搭配**:
  - multi-threading (多线程)
  - thread synchronization (线程同步)
  - context switch (上下文切换)
- **例句**: "Multi-threaded programs can improve performance on multi-core systems."

#### 3. address space `/əˈdres speɪs/`
- **CS义**: 地址空间 (the range of memory addresses available to a process)
- **技术定义**: The set of all valid memory addresses that a process can use.
- **组成部分**:
  - **Text segment**: 代码段 (executable code, read-only)
  - **Data segment**: 数据段 (global/static variables)
  - **Stack**: 栈 (local variables, function calls)
  - **Heap**: 堆 (dynamically allocated memory)
- **例句**: "Each process has its own virtual address space."

#### 4. schedule `/ˈskedʒuːl/`
- **CS义**: v./n. 调度 (allocate CPU time to processes/threads)
- **技术定义**: The act of determining which process/thread should execute next.
- **调度算法**:
  - FCFS (First Come First Served): 先来先服务
  - SJF (Shortest Job First): 短作业优先
  - Round Robin: 时间片轮转
  - Priority Scheduling: 优先级调度
- **核心搭配**:
  - scheduler (n. 调度器)
  - scheduling algorithm (调度算法)
- **例句**: "The scheduler decides which process runs next."

#### 5. executable `/ˈeksɪkjuːtəbl/`
- **CS义**: adj. 可执行的；n. 可执行文件
- **词根**: execute(执行) + able(能够) → 能够执行的
- **文件扩展名**: .exe (Windows), .out (Linux), .app (macOS)
- **考研对比**:
  - 📖 考研义: execute v. 执行，处决
  - 💻 CS义: executable 可执行的（文件/代码）
- **例句**: "The compiler generates an executable binary file."

#### 6. terminate `/ˈtɜːrmɪneɪt/`
- **CS义**: v. 终止，结束（进程/程序）
- **技术定义**: To end the execution of a process.
- **核心搭配**:
  - graceful termination (正常终止)
  - forceful termination (强制终止)
  - signal termination (信号终止)
- **同义词**: kill, stop, exit
- **例句**: "The user can terminate a process by sending a SIGKILL signal."

#### 7. zombie (process) `/ˈzɑːmbi/`
- **CS义**: n. 僵尸进程 (a terminated process that hasn't been cleaned up)
- **技术定义**: A process that has completed execution but still has an entry in the process table, waiting for its parent to read its exit status.
- **产生原因**: 子进程终止后，父进程未调用 wait() 清理
- **危害**: 占用进程表空间，大量僵尸进程可能导致无法创建新进程
- **例句**: "A zombie process remains in the process table until its parent calls wait()."

#### 8. allocate `/ˈæləkeɪt/`
- **CS义**: v. 分配（内存/资源）
- **词根**: al-(向) + loc(地方) + ate → 分配到地方
- **核心搭配**:
  - allocate memory (分配内存)
  - dynamic allocation (动态分配)
  - static allocation (静态分配)
- **反义词**: deallocate (释放), free (释放)
- **例句**: "The OS allocates memory to each process when it starts."

#### 9. suspended `/səˈspendɪd/`
- **CS义**: adj. 挂起的，暂停的（进程状态）
- **技术定义**: A process state where execution is temporarily stopped.
- **核心搭配**:
  - suspend a process (挂起进程)
  - resume a process (恢复进程)
- **考研对比**:
  - 📖 考研义: suspend v. 暂停，中止，悬挂
  - 💻 CS义: suspended state 挂起状态
- **例句**: "A suspended process can be resumed later."

#### 10. kernel `/ˈkɜːrnl/`
- **CS义**: n. 内核（操作系统的核心部分）
- **技术定义**: The core component of an OS that manages system resources and hardware.
- **职责**:
  - Process management (进程管理)
  - Memory management (内存管理)
  - File system management (文件系统管理)
  - Device drivers (设备驱动)
- **例句**: "The Linux kernel is open-source and highly customizable."

---

## 📝 学习建议

1. **结合实践**: 边学术语边写代码，理解更深刻
2. **对比记忆**: 注意同一单词在考研/CS/日常语境下的不同含义
3. **技术定义优先**: CS术语必须掌握精确的技术定义
4. **408考频**: 标注⭐的术语是408统考高频考点

---

## 🔗 相关资源

- 定义来源：Linux Manual Pages, CSAPP教材, 408统考大纲
- 例句来源：技术文档、论文、Stack Overflow高赞回答
