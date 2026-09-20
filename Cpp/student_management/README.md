# 学生管理系统

## 项目状态

> 项目完成，工程化中。已有多文件实现和测试源码，但统一构建、测试隔离和完整行为验证仍需补充。

## 平台与构建

| 项目 | 内容 |
|---|---|
| 平台 | Windows MinGW、Linux/macOS 理论支持，需分别验证 |
| 标准 | C++17 |
| 构建 | 当前提供单命令构建；CMake/CTest 待补充 |
| 测试 | `test_student_manager.cpp`；历史记录为 10 个测试，需重新执行确认 |
| 数据 | 运行时会生成数据和备份文件，建议使用独立工作目录 |

## 功能

- 学生增删改查
- 课程成绩管理和统计
- 条件过滤、排名和优秀学生查询
- 文本持久化、CSV 批量导入和数据验证
- 单元测试示例

## 文件结构

```text
student_management/
├── Student.h/.cpp
├── Course.h/.cpp
├── StudentManager.h/.cpp
├── main.cpp
├── test_student_manager.cpp
├── README.md
├── DESIGN.md
├── CHANGELOG.md
├── PROJECT_SUMMARY.md
└── PROJECT_SHOWCASE.md
```

## 构建

```bash
g++ -std=c++17 -Wall -Wextra -o student_system Student.cpp StudentManager.cpp Course.cpp main.cpp
g++ -std=c++17 -Wall -Wextra -o test_student_manager Student.cpp StudentManager.cpp Course.cpp test_student_manager.cpp
```

运行测试前请在临时目录中执行，避免生成的数据文件污染源码目录。

## 后续工作

- 增加 CMake 和 CTest。
- 将测试文件写入临时目录。
- 补充自动备份、启动加载、CSV 异常输入测试。
- 移除头文件中的全局 `using namespace std`。
