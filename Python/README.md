# Web 开发学习仓库（Python + Flask + JavaScript）

> 本仓库是一个按“学习序号”组织的实践型学习档案：既有 Python 基础教程，也有可运行的 Web 项目、测试与部署材料。
>
> **状态基线（2026-10-29）**：已完成 46 个学习序号，累计有效时长 **57.16 小时**；第 47 天（JavaScript 高级特性）资料已备、**尚未开始**。统计口径与历史异常见 [`docs/学习进度与统计口径.md`](docs/学习进度与统计口径.md)。

---

## 快速开始

```bash
# 1. 创建虚拟环境（Windows 使用 py 启动器；本机 python 未加入 PATH）
py -3.12 -m venv python_basics/.venv
# macOS / Linux: python3 -m venv python_basics/.venv

# 2. 安装依赖（直接使用虚拟环境解释器，不依赖激活）
python_basics/.venv/Scripts/python.exe -m pip install -r requirements.txt
# macOS / Linux: python_basics/.venv/bin/python -m pip install -r requirements.txt

# 3. 运行测试（应显示 61 passed）
python_basics/.venv/Scripts/python.exe -m pytest

# 4. 运行 Todo App 存储层测试（可选，需 Node 18+）
cd web_projects/todo_app && npm test && cd ../..

# 5. 启动论坛项目
python_basics/.venv/Scripts/python.exe -m forum_project.run
# 访问 http://127.0.0.1:5006
```

更多项目的启动方式见 [`docs/运行指南.md`](docs/运行指南.md)。

---

## 目录导航

| 目录 | 内容 | 状态 |
|---|---|---|
| `python_basics/` | 82 个 Python 教学文件 + 留言板项目 v1～v4 | ✅ 可运行 |
| `web_basics/` | JavaScript 高级特性（闭包、原型链、this、ES6 模块） | ✅ 资料完整，待学习 |
| `web_development/` | 数组方法、异步、Fetch、ES6 等 HTML 教程 | ✅ 可直接打开 |
| `web_projects/todo_app/` | Modern Todo App 综合项目 | ✅ 功能完成，`storage.js` 有单元测试 |
| `forum_project/` | Flask + SQLite 论坛系统 | ✅ 16 个接口测试通过 |
| `deployment/` | Linux、Docker、Nginx、systemd、部署脚本 | 📘 Docker/Nginx 已容器验证；systemd 未实机验证 |
| `daily_logs/` | 46 份完成日志 + 6 份示例日志 + 模板 | ✅ 序号 1～46 唯一；示例不计入统计 |
| `daily_tasks/` | 学习任务 + 任务索引 + 模板；重复文件已归档 | ✅ 49 份有效任务，覆盖 1～38、40、43～52 |
| `weekly_reports/` | 由脚本从真实日志生成的 11 份周报 | ✅ 可重复生成，自动跳过示例日志 |
| `knowledge_base/` | 知识体系、阶段笔记与速查（阶段 1～5） | ✅ 持续更新 |
| `docs/` | 进度口径、异常记录、运行指南、路线图 | ✅ |
| `scripts/` | 周报生成、材料审计、学习日志生成 | ✅ 审计 7 项 + 日志生成 6 项测试 |
| `practice_projects/` | 第 48～52 天练习：应用工厂、Linux、Nginx、Docker、CI | ✅ 12 项测试（11 通过 + 1 项 Windows 跳过） |
| `tests/` | 仓库结构、统计口径、审计与日志生成回归测试 | ✅ 29 项（结构 16 + 审计 7 + 日志生成 6） |

每个主要目录都有独立 `README.md`，说明用途、运行方式与验收标准。本次补充的周报、模板、知识笔记与测试清单见 [`docs/学习材料生成说明.md`](docs/学习材料生成说明.md)。

---

## 测试

```bash
python -m pytest                              # 全量：61 passed
python -m pytest -q forum_project/tests       # 论坛 16 项
python -m pytest -q python_basics/留言板项目   # 留言板 API 16 项（v4 5 项 + v1～v3 11 项）
python -m pytest -q tests                     # 仓库结构、统计、审计与日志生成 29 项
python -m pytest -q practice_projects          # 第 48～52 天练习 12 项（默认不收集，需显式指定路径）
python -m pytest -q tests/test_audit_learning_materials.py  # 审计工具 7 项

python scripts/audit_learning_materials.py    # 材料完整性审计
python scripts/generate_learning_logs.py --check  # 检查 47～52 天示例日志覆盖

cd web_projects/todo_app && npm test          # 前端 19 项（存储 10 + DOM 9，需先 npm install）
```

测试使用临时数据库，不会修改仓库中的开发数据。

---

## 工程化约定

- 依赖记录在 `requirements.txt`，并带版本范围；不提交 `.venv`、缓存和 IDE 配置。
- 换行符由 `.gitattributes` 统一：Shell、Python、配置与文档使用 LF，避免 Windows 检出的 CRLF 破坏 Linux 脚本。
- 新增项目必须包含 `README.md`；有设计的项目同时提供 `DESIGN.md`。
- 新增学习序号前先建任务，完成后再写日志；任务中的 `[ ]` 不得预填为 `[x]`。
- 只修复与当前任务相关的问题，避免无目标重写教学示例。
- 详细的资料治理规则见 [`CLAUDE.md`](CLAUDE.md)，异常登记见 [`docs/资料异常记录.md`](docs/资料异常记录.md)。

---

## 尚未完成

- 第 47 天 JavaScript 高级特性的实际学习与日志。
- `deployment/` 在真实服务器上的验证记录。
- Todo App 的自动化浏览器测试。
- 管理员角色、帖子编辑等论坛扩展功能。

以上内容保持“计划中/待验证”状态，不计入完成统计。
