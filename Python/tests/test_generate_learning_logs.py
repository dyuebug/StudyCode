"""学习日志生成器的回归测试。"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

import pytest

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
SCRIPT_PATH = REPOSITORY_ROOT / "scripts/generate_learning_logs.py"
SAMPLE_MARKER = "**文档性质**：示例日志（非真实学习记录"


def load_module():
    spec = importlib.util.spec_from_file_location("generate_learning_logs", SCRIPT_PATH)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


@pytest.fixture()
def module(tmp_path, monkeypatch):
    module = load_module()
    log_dir = tmp_path / "daily_logs"
    task_dir = tmp_path / "daily_tasks"
    log_dir.mkdir()
    task_dir.mkdir()
    monkeypatch.setattr(module, "LOG_DIRECTORY", log_dir)
    monkeypatch.setattr(module, "TASK_DIRECTORY", task_dir)
    return module


def write_task(task_dir: Path, number: int, date: str = "2026-10-30") -> Path:
    path = task_dir / f"{date}_第{number}天_任务.md"
    path.write_text(
        f"""# 📅 第{number}天学习任务清单

> **记录日期**：{date}
> **学习阶段**：阶段 5 — 示例阶段
> **预计时长**：2 小时

## 🎯 今日学习目标

- [ ] 目标一
- [ ] 目标二

## 📝 具体任务

### 任务1：阅读配置

### 任务2：运行验证

## ✅ 成功标准

- [ ] 标准
""",
        encoding="utf-8",
    )
    return path


def test_parse_task_extracts_fields(module) -> None:
    path = write_task(module.TASK_DIRECTORY, 47)

    task = module.parse_task(path)

    assert task.number == 47
    assert task.date == "2026-10-30"
    assert task.stage == "阶段 5 — 示例阶段"
    assert task.duration == "2 小时"
    assert task.goals == ["目标一", "目标二"]
    assert task.activities == ["阅读配置", "运行验证"]


def test_render_log_is_marked_and_incomplete(module) -> None:
    task = module.parse_task(write_task(module.TASK_DIRECTORY, 47))

    text = module.render_log(task)

    assert SAMPLE_MARKER in text
    assert "不计入统计" in text
    assert "- [ ] 任务1：阅读配置" in text
    assert "- [x]" not in text
    assert "示例值 2 小时" in text
    assert "示例内容，不代表完成度" in text


def test_generate_creates_missing_sample_log(module) -> None:
    write_task(module.TASK_DIRECTORY, 48)

    created, messages = module.generate(module.find_tasks())

    assert len(created) == 1
    assert created[0].name == "2026-10-30_第48天_日志.md"
    assert SAMPLE_MARKER in created[0].read_text(encoding="utf-8")
    assert "生成 第48天" in messages[0]


def test_generate_skips_existing_real_log(module) -> None:
    write_task(module.TASK_DIRECTORY, 47)
    real_log = module.LOG_DIRECTORY / "2026-10-30_第47天_日志.md"
    real_log.write_text("# 📝 第47天学习日志\n\n真实记录\n", encoding="utf-8")

    created, messages = module.generate(module.find_tasks())
    assert created == []
    assert "跳过 第47天" in messages[0]

    created, messages = module.generate(module.find_tasks(), force=True)
    assert created == []
    assert "拒绝覆盖" in messages[0]
    assert "真实记录" in real_log.read_text(encoding="utf-8")


def test_force_regenerates_only_sample_log(module) -> None:
    write_task(module.TASK_DIRECTORY, 49)
    sample_log = module.LOG_DIRECTORY / "2026-10-30_第49天_日志.md"
    sample_log.write_text(f"# 旧示例\n\n{SAMPLE_MARKER}\n", encoding="utf-8")

    created, messages = module.generate(module.find_tasks(), force=True)

    assert created == [sample_log]
    assert "生成 第49天" in messages[0]
    assert "任务完成情况" in sample_log.read_text(encoding="utf-8")


def test_current_repository_sample_logs_cover_tasks() -> None:
    module = load_module()
    logs = module.existing_logs()

    for number in range(47, 53):
        assert number in logs, f"第{number}天缺少学习日志"
        assert module.is_sample(logs[number]), f"第{number}天日志未标记为示例"
