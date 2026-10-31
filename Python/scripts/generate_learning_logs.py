"""从任务文件自动生成“示例学习日志”。

设计约束（对应 CLAUDE.md 1.3 与 4.2）：
- 只生成带 `SAMPLE_MARKER` 的示例日志，不伪造真实学习记录；
- 默认不覆盖任何已存在的日志；
- `--force` 也只允许覆盖示例日志，遇到真实日志立即跳过并报错；
- 日志中的完成项保持 `[ ]`，时长写“示例值”，完成度写“示例内容”。

用法：
    python scripts/generate_learning_logs.py --write
    python scripts/generate_learning_logs.py --write --days 47-52
    python scripts/generate_learning_logs.py --check
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
LOG_DIRECTORY = REPOSITORY_ROOT / "daily_logs"
TASK_DIRECTORY = REPOSITORY_ROOT / "daily_tasks"

SAMPLE_MARKER = "**文档性质**：示例日志（非真实学习记录"
NUMBER_PATTERN = re.compile(r"第(\d+)天")
TASK_FILE_PATTERN = re.compile(r"^\d{4}-\d{2}-\d{2}_第(\d+)天_任务\.md$")


@dataclass(frozen=True)
class LearningTask:
    number: int
    path: Path
    date: str
    stage: str
    duration: str
    goals: list[str]
    activities: list[str]


def parse_task(path: Path) -> LearningTask:
    text = path.read_text(encoding="utf-8")
    number_match = NUMBER_PATTERN.search(path.name)
    if number_match is None:
        raise ValueError(f"任务文件名缺少学习序号：{path.name}")

    return LearningTask(
        number=int(number_match.group(1)),
        path=path,
        date=field(text, r"\*\*记录日期\*\*：(.+)") or "待填写",
        stage=field(text, r"\*\*(?:学习)?阶段\*\*：(.+)") or "日志未记录",
        duration=field(text, r"\*\*预计时长\*\*：(.+)") or "待填写",
        goals=section_items(text, "## 🎯 今日学习目标"),
        activities=section_headings(text, "### 任务"),
    )


def field(text: str, pattern: str) -> str:
    match = re.search(pattern, text)
    return match.group(1).strip() if match else ""


def section_items(text: str, heading: str) -> list[str]:
    section = section_text(text, heading)
    return [
        line.strip()[len("- [ ] "):].strip()
        for line in section.splitlines()
        if line.strip().startswith("- [ ] ")
    ]


def section_headings(text: str, prefix: str) -> list[str]:
    headings = []
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith(prefix) and "：" in stripped:
            headings.append(stripped[len(prefix):].lstrip("0123456789").strip("： "))
    return headings


def section_text(text: str, heading: str) -> str:
    if heading not in text:
        return ""
    rest = text.split(heading, 1)[1]
    # 只取到下一个二级标题，避免把“成功标准”等后续条目算进当前小节。
    return re.split(r"\n## ", rest, maxsplit=1)[0]


def render_log(task: LearningTask) -> str:
    goals = "\n".join(f"- {goal}" for goal in task.goals) or "- 待填写"
    activities = "\n".join(
        f"- [ ] 任务{index}：{name}"
        for index, name in enumerate(task.activities, start=1)
    ) or "- [ ] 待填写"

    return f"""# 📝 第{task.number}天学习日志（示例）

> **文档性质**：示例日志（非真实学习记录，不计入统计）
> **用途**：由 `scripts/generate_learning_logs.py` 根据任务文件生成，仅演示日志结构，**不代表该学习已实际发生**。
> **统计口径**：本文件不计入学习天数、累计时长、完成度与周报；统计脚本与回归测试按标记跳过。

**日期**：{task.date}（任务记录日期，不代表实际学习日期）
**学习序号**：第{task.number}天
**学习阶段**：{task.stage}
**学习时长**：示例值 {task.duration}（非实际计时，不计入统计）
**完成度**：示例内容，不代表完成度

---

## ✅ 任务完成情况

下列任务项由任务清单复制而来，**保持未完成状态**，实际学习后由本人勾选并填写结果。

{activities}

**未完成任务及原因**：示例内容，未实际执行。

---

## 🎯 学习目标（来自任务清单）

{goals}

---

## 📖 学到的知识点

示例占位：实际学习后按主题填写，建议每个主题包含“是什么、为什么、最小示例、常见错误”。

---

## ❓ 遇到的问题与解决

示例占位：未实际发生。真实日志应记录报错信息、定位过程和最终修复方式。

---

## 🧪 测试或运行结果

未执行。实际学习后填写命令、输出摘要与退出码，不要只写“无 bug”。

---

## 📅 明天计划

待确认。

---

> 本文件由脚本生成。真实学习完成后，应删除示例标记、按实际结果重写，并重新生成周报。
"""


def find_tasks(day_filter: set[int] | None = None) -> list[LearningTask]:
    tasks = []
    for path in sorted(TASK_DIRECTORY.glob("*_第*天_任务.md")):
        match = TASK_FILE_PATTERN.match(path.name)
        if match is None:
            continue
        number = int(match.group(1))
        if day_filter is not None and number not in day_filter:
            continue
        if "_archive" in path.parts:
            continue
        tasks.append(parse_task(path))
    return tasks


def existing_logs() -> dict[int, Path]:
    logs: dict[int, Path] = {}
    for path in sorted(LOG_DIRECTORY.glob("*_第*天_日志.md")):
        match = NUMBER_PATTERN.search(path.name)
        if match:
            logs[int(match.group(1))] = path
    return logs


def is_sample(path: Path) -> bool:
    return SAMPLE_MARKER in path.read_text(encoding="utf-8")


def generate(tasks: list[LearningTask], force: bool = False) -> tuple[list[Path], list[str]]:
    created: list[Path] = []
    messages: list[str] = []
    logs = existing_logs()

    for task in tasks:
        target = LOG_DIRECTORY / f"{task.date}_第{task.number}天_日志.md"
        existing = logs.get(task.number)
        if existing is not None and not force:
            messages.append(f"跳过 第{task.number}天：已存在 {existing.name}")
            continue
        if existing is not None and force and not is_sample(existing):
            messages.append(f"拒绝覆盖 第{task.number}天：{existing.name} 不是示例日志")
            continue

        target.write_text(render_log(task), encoding="utf-8")
        created.append(target)
        logs[task.number] = target
        messages.append(f"生成 第{task.number}天：{target.name}")

    return created, messages


def parse_days(value: str) -> set[int]:
    days: set[int] = set()
    for part in value.split(","):
        part = part.strip()
        if "-" in part:
            start, end = part.split("-", 1)
            days.update(range(int(start), int(end) + 1))
        elif part:
            days.add(int(part))
    return days


def main() -> int:
    parser = argparse.ArgumentParser(description="生成带示例标记的学习日志")
    parser.add_argument("--write", action="store_true", help="写入缺失的示例日志")
    parser.add_argument("--force", action="store_true", help="覆盖已有示例日志（不覆盖真实日志）")
    parser.add_argument("--check", action="store_true", help="只检查，不写文件")
    parser.add_argument("--days", help="限定学习序号，例如 47-52 或 47,49")
    args = parser.parse_args()

    day_filter = parse_days(args.days) if args.days else None
    tasks = find_tasks(day_filter)
    if not tasks:
        print("未找到符合条件的任务文件。")
        return 1

    if args.check or not (args.write or args.force):
        missing = [task for task in tasks if task.number not in existing_logs()]
        for task in tasks:
            path = existing_logs().get(task.number)
            state = "示例" if path and is_sample(path) else ("真实" if path else "缺失")
            print(f"第{task.number}天：{state} {path.name if path else ''}".rstrip())
        return 0 if not missing else 1

    created, messages = generate(tasks, force=args.force)
    print("\n".join(messages))
    print(f"共生成 {len(created)} 份示例日志。")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
