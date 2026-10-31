"""学习材料审计工具的单元测试。

覆盖：
- 示例日志与真实日志的分离；
- 任务序号扫描忽略归档与草稿；
- 重复序号与缺号检测；
- 示例占用真实序号时报告错误；
- Markdown 报告包含统计口径；
- 对当前仓库的回归检查（46 份真实日志、57.16 小时）。
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path
from types import ModuleType

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = REPOSITORY_ROOT / "scripts" / "audit_learning_materials.py"


def load_audit_module() -> ModuleType:
    spec = importlib.util.spec_from_file_location("audit_learning_materials_test", SCRIPT_PATH)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


audit_module = load_audit_module()


def write_log(path: Path, number: int, duration: str, sample: bool = False) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    marker = (
        "> **文档性质**：示例日志（非真实学习记录，不计入统计）\n"
        if sample
        else ""
    )
    path.write_text(
        f"# 第{number}天学习日志\n\n{marker}"
        f"**日期**：2026-10-30\n"
        f"**学习时长**：{duration}\n",
        encoding="utf-8",
    )


def test_scan_logs_separates_samples(tmp_path: Path) -> None:
    log_directory = tmp_path / "daily_logs"
    write_log(log_directory / "2026-01-01_第1天_日志.md", 1, "2 小时")
    write_log(log_directory / "2026-01-02_第2天_日志.md", 2, "1.5 小时")
    write_log(log_directory / "2026-01-03_第3天_日志.md", 3, "示例值 2 小时", sample=True)

    real, samples = audit_module.scan_logs(log_directory)

    assert [item.number for item in real] == [1, 2]
    assert [item.number for item in samples] == [3]
    assert real[0].duration == 2.0
    assert samples[0].sample is True
    assert samples[0].duration is None  # “示例值”不是数字，不会被计入


def test_scan_task_numbers_ignores_archive_and_drafts(tmp_path: Path) -> None:
    task_directory = tmp_path / "daily_tasks"
    task_directory.mkdir()
    (task_directory / "2026-01-01_第1天_任务.md").write_text("x", encoding="utf-8")
    (task_directory / "2026-01-02_第2天_任务.md").write_text("x", encoding="utf-8")
    for folder in ("_archive", "_drafts"):
        nested = task_directory / folder
        nested.mkdir()
        (nested / "2026-01-03_第9天_任务.md").write_text("x", encoding="utf-8")

    assert audit_module.scan_task_numbers(task_directory) == [1, 2]


def test_find_duplicates_and_gaps() -> None:
    assert audit_module.find_duplicates([1, 2, 2, 3, 3]) == [2, 3]
    assert audit_module.find_gaps([1, 2, 4, 6]) == [3, 5]
    assert audit_module.find_gaps([]) == []


def test_audit_reports_sample_overlap(tmp_path: Path) -> None:
    write_log(tmp_path / "daily_logs" / "2026-01-01_第1天_日志.md", 1, "2 小时")
    write_log(
        tmp_path / "daily_logs" / "2026-01-02_第1天_日志.md",
        1,
        "示例值 2 小时",
        sample=True,
    )

    result = audit_module.audit(tmp_path, required_files=())

    assert any("示例日志占用了真实日志序号" in item for item in result.errors)
    assert result.real_count == 1
    assert result.sample_count == 1


def test_audit_reports_missing_files(tmp_path: Path) -> None:
    result = audit_module.audit(tmp_path, required_files=("missing.py",))

    assert result.missing_files == ["missing.py"]
    assert any("缺少关键文件" in item for item in result.errors)


def test_render_markdown_contains_totals() -> None:
    result = audit_module.AuditResult(
        real_logs=[
            audit_module.LogRecord(number=1, path="daily_logs/a.md", sample=False, duration=2.0),
            audit_module.LogRecord(number=2, path="daily_logs/b.md", sample=False, duration=1.5),
        ],
        sample_logs=[
            audit_module.LogRecord(number=3, path="daily_logs/c.md", sample=True, duration=None)
        ],
        weekly_reports=["2026-W01.md"],
    )

    report = audit_module.render_markdown(result)

    assert "| 真实日志数 | 2 |" in report
    assert "| 示例日志数 | 1 |" in report
    assert "| 有效学习时长 | 3.5 小时 |" in report
    assert "第3天" in report


def test_current_repository_statistics() -> None:
    """当前仓库的真实日志应为 46 份、合计 57.16 小时，且无错误。"""
    result = audit_module.audit(REPOSITORY_ROOT)

    assert result.errors == []
    assert result.real_count == 46
    assert result.effective_duration == 57.16
    assert result.sample_count == 6
