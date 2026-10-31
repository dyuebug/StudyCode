# Web 开发学习仓库

## 快速开始

```bash
py -3.12 -m venv python_basics/.venv
python_basics/.venv/Scripts/python.exe -m pip install -r requirements.txt
python_basics/.venv/Scripts/python.exe -m pytest
python_basics/.venv/Scripts/python.exe -m forum_project.run
```

访问 <http://127.0.0.1:5006>。

## 详细说明

- 运行各项目：[`运行指南.md`](运行指南.md)
- 学习进度与统计口径：[`学习进度与统计口径.md`](学习进度与统计口径.md)
- 剩余学习路线：[`ROADMAP_剩余阶段.md`](ROADMAP_剩余阶段.md)
- 资料异常记录：[`资料异常记录.md`](资料异常记录.md)
- 本次材料生成清单：[`学习材料生成说明.md`](学习材料生成说明.md)
- 材料完整性审计报告：[`学习材料审计.md`](学习材料审计.md)
- 自动生成的周报索引：[`../weekly_reports/INDEX.md`](../weekly_reports/INDEX.md)

## 测试

```bash
python_basics/.venv/Scripts/python.exe -m pytest        # 61 passed
python_basics/.venv/Scripts/python.exe -m pytest practice_projects -q  # 11 passed, 1 skipped
cd web_projects/todo_app && npm test                     # 19 passed（需 Node 18+）
```
