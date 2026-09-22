# 学生管理系统

## 项目状态

> 项目完成，工程化中。已有多文件实现、CMake/CTest 构建和 10 个测试；主程序仍有历史源码兼容性问题需要单独修复。

## 平台与构建

| 项目 | 内容 |
|---|---|
| 平台 | Windows MinGW、Linux/macOS 理论支持，需分别验证 |
| 标准 | C++17 |
| 构建 | CMake；MSVC 2022 Debug 已验证，MinGW/Linux 待验证 |
| 测试 | `student_management_unit_tests`；10 个测试，MSVC/CTest 已通过 |
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

推荐使用 CMake：

```bash
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

也可以使用单文件命令：

```bash
g++ -std=c++17 -Wall -Wextra -o student_system Student.cpp StudentManager.cpp Course.cpp main.cpp
g++ -std=c++17 -Wall -Wextra -o test_student_manager Student.cpp StudentManager.cpp Course.cpp test_student_manager.cpp
```

测试会接收由 CTest 传入的数据文件路径，避免依赖源码目录。运行手工编译的测试时，也建议传入独立临时路径。

## 后续工作

- 修复主程序在 MSVC 下的历史源码兼容性问题。
- 在 Linux/MinGW 上重新验证 CMake 构建。
- 补充自动备份、启动加载、CSV 异常输入测试。
- 移除头文件中的全局 `using namespace std`。
