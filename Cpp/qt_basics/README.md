# qt_basics

## 定位

Qt Widgets 学习材料，覆盖 GUI、布局、对话框、SQL、网络和多线程，并为 `TaskManager` 综合项目提供前置知识。

## 内容范围

- `01`-`19`：Qt 核心概念、控件、布局、对话框和计算器/记事本示例
- `20`-`26`：Qt SQL、SQLite、CRUD、事务和关联查询
- `27`-`31`：网络请求、JSON、文件下载和天气应用
- `32`-`38`：线程、同步、QtConcurrent 和阶段总结
- 综合项目规划：`qt_capstone_project_plan.md`

## 状态与限制

| 项目 | 内容 |
|---|---|
| 状态 | 学习完成，工程化中 |
| 平台 | Qt Creator 或 Qt 命令行环境；Windows/Linux 均需单独验证 |
| 标准 | 主要示例使用 C++11 及以上，具体文件以文件头为准 |
| 构建 | 示例依赖 Qt Widgets、Sql、Network、Concurrent 等模块；统一 CMake/qmake 工程待补充 |
| 测试 | 暂无目录级自动化测试 |
| 限制 | Qt 5/Qt 6、MinGW/MSVC 和 SQLite 驱动兼容性尚未统一验证 |

## 使用建议

先确认 Qt 主版本、编译器和所需模块，再为单个示例建立最小 `.pro` 或 CMake 工程。不要直接把源文件复制到 `TaskManager` 工程中混编。
