# 学习日志索引

> 统计口径：真实完成日志计入学习天数与时长；示例日志与底稿不计入。
> 由仓库结构测试与 `scripts/audit_learning_materials.py` 校验。

## 一、真实完成日志（46 份）

- 序号：第 1～46 天，文件名格式 `YYYY-MM-DD_第X天_日志.md`
- 文件头特征：无“示例日志（非真实学习记录，不计入统计）”标记
- 有效累计时长：57.16 小时（逐份日志“学习时长”求和）

## 二、示例日志（6 份，不计入统计）

| 序号 | 文件 | 对应任务 |
|---|---|---|
| 第47天 | `2026-10-30_第47天_日志.md` | `daily_tasks/2026-06-22_第47天_任务.md` |
| 第48天 | `2026-10-30_第48天_日志.md` | `daily_tasks/2026-10-30_第48天_任务.md` |
| 第49天 | `2026-10-30_第49天_日志.md` | `daily_tasks/2026-10-30_第49天_任务.md` |
| 第50天 | `2026-10-30_第50天_日志.md` | `daily_tasks/2026-10-30_第50天_任务.md` |
| 第51天 | `2026-10-30_第51天_日志.md` | `daily_tasks/2026-10-30_第51天_任务.md` |
| 第52天 | `2026-10-30_第52天_日志.md` | `daily_tasks/2026-10-30_第52天_任务.md` |

说明：示例日志由 `scripts/generate_learning_logs.py` 校验或生成，只演示日志结构；文件头固定标注“示例日志（非真实学习记录，不计入统计）”。

## 三、日志底稿（待填写，不计入统计）

| 序号 | 文件 |
|---|---|
| 第47～52天 | `_drafts/第47天_日志底稿.md` ～ `_drafts/第52天_日志底稿.md` |

底稿用于实际学习时逐项填写；填写完成并删除“待填写”后，再复制为正式日志。

## 四、统计与校验

```bash
python_basics/.venv/Scripts/python.exe scripts/generate_learning_logs.py --check --days 47-52
python_basics/.venv/Scripts/python.exe scripts/audit_learning_materials.py
python_basics/.venv/Scripts/python.exe -m pytest tests -q
```

真实学习完成后，按 `CLAUDE.md` 第 6 节生成正式日志，并重新运行周报生成器。
