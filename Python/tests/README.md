# 仓库级回归测试

## 目的

防止目录、关键文档、学习日志统计在后续修改中悄悄漂移。

## 内容

| 测试 | 检查项 |
|---|---|
| `test_forum_project_layout_complete` | 论坛项目关键文件齐全 |
| `test_deployment_materials_complete` | 部署材料与 CI 配置齐全 |
| `test_learning_log_numbers_are_unique` | 真实日志序号 1～46 唯一连续，示例日志不占用真实序号 |
| `test_sample_logs_are_marked_and_excluded` | 示例日志带“不计入统计”标记，且 CLAUDE.md 说明口径 |
| `test_active_task_numbers_are_unique` | 有效任务序号不重复 |
| `test_archived_duplicates_are_preserved` | 6 份重复任务仍保留在归档目录 |
| `test_effective_duration_matches_logs` | 逐日时长求和为 57.16，且 CLAUDE.md 已记录 |
| `test_governance_documents_exist` | 异常记录与补记说明存在 |
| `test_knowledge_notes_exist` | 七份阶段知识笔记存在 |
| `test_learning_templates_exist` | 任务与日志模板存在 |
| `test_weekly_reports_are_generated_from_logs` | 11 份周报存在且时长合计为 57.16 |
| `test_message_board_version_tests_exist` | 留言板 v1～v3 测试文件存在 |
| `test_todo_storage_tests_exist` | Todo App 存储与 DOM 测试文件、npm 配置存在 |
| `test_deployment_scripts_use_lf_line_endings` | 部署脚本为 LF 换行，且存在 `.gitattributes` |
| `test_practice_projects_complete` | 第 48～52 天练习项目的入口、测试与脚本齐全 |
| `test_practice_shell_scripts_use_lf` | 练习脚本为 LF 换行 |

## 运行

```bash
python_basics/.venv/Scripts/python.exe -m pytest tests -q
```

预期：`16 passed`（本文件）。

## 审计工具测试

`test_audit_learning_materials.py` 覆盖 `scripts/audit_learning_materials.py`：

| 测试 | 检查项 |
|---|---|
| `test_scan_logs_separates_samples` | 示例日志与真实日志分离，示例时长不计入 |
| `test_scan_task_numbers_ignores_archive_and_drafts` | 任务扫描忽略归档与草稿 |
| `test_find_duplicates_and_gaps` | 重复序号与缺号检测 |
| `test_audit_reports_sample_overlap` | 示例占用真实序号时报错 |
| `test_audit_reports_missing_files` | 关键文件缺失时报错 |
| `test_render_markdown_contains_totals` | 报告包含真实数、示例数与有效时长 |
| `test_current_repository_statistics` | 当前仓库 46 份真实日志、57.16 小时、6 份示例 |

预期：`7 passed`。

## 学习日志生成器测试

`test_generate_learning_logs.py` 覆盖 `scripts/generate_learning_logs.py`：

| 测试 | 检查项 |
|---|---|
| `test_parse_task_extracts_fields` | 从任务文件解析序号、日期、阶段、时长、目标与活动 |
| `test_render_log_is_marked_and_incomplete` | 生成内容带示例标记，任务项保持 `[ ]`，不伪造完成 |
| `test_generate_creates_missing_sample_log` | 缺失日志时按任务生成示例日志 |
| `test_generate_skips_existing_real_log` | 默认不覆盖，`--force` 也拒绝覆盖真实日志 |
| `test_force_regenerates_only_sample_log` | 仅允许覆盖示例日志 |
| `test_current_repository_sample_logs_cover_tasks` | 当前仓库第 47～52 天均有示例日志 |

预期：`6 passed`。

`tests/` 合计：`29 passed`（结构 16 + 审计 7 + 日志生成 6）。

## 说明

这些测试只读取仓库文件；生成器测试写入 `tmp_path`，不会改动 `daily_logs/`。若确实需要调整统计口径，应先更新 `docs/学习进度与统计口径.md` 与 `CLAUDE.md`，再同步修改断言，不能为了通过测试而删掉检查。
