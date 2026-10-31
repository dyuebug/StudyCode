"""学习材料完整性审计工具。

用途：
1. 区分真实日志与示例日志，避免示例内容污染统计。
2. 检查学习序号是否重复、是否出现非预期缺号。
3. 汇总真实日志的学习次数与有效时长。
4. 检查每日任务、周报与关键项目文件是否齐全。

退出码：
    0  未发现错误（警告不影响退出码）
    1  发现重复序号、缺失关键文件等错误

运行：
    python scripts/audit_learning_materials.py
    python scripts/audit_learning_materials.py --json
    python scripts/audit_learning_materials.py --write docs/学习材料审计.md
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import asdict, dataclass, field
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
SAMPLE_MARKER = "**文档性质**：示例日志（非真实学习记录"
NUMBER_PATTERN = re.compile(r"第(\d+)天")
DURATION_PATTERN = re.compile(r"\*\*学习时长\*\*：([0-9]+(?:\.[0-9]+)?)")
WEEKLY_REPORT_NAME_PATTERN = re.compile(r"^\d{4}-W\d{2}\.md$")

REQUIRED_PROJECT_FILES = (
    "forum_project/app.py",
    "forum_project/db.py",
    "forum_project/schema.sql",
    "forum_project/tests/test_forum.py",
    "python_basics/留言板项目/app_v4_sqlite.py",
    "python_basics/留言板项目/test_app_v4_sqlite.py",
    "web_projects/todo_app/index.html",
    "web_projects/todo_app/tests/app.test.mjs",
    "deployment/docker/Dockerfile",
    "deployment/nginx/forum.conf",
    ".github/workflows/ci.yml",
)


@dataclass
class LogRecord:
    number: int
    path: str
    sample: bool
    duration: float | None


@dataclass
class AuditResult:
    real_logs: list[LogRecord] = field(default_factory=list)
    sample_logs: list[LogRecord] = field(default_factory=list)
    task_numbers: list[int] = field(default_factory=list)
    weekly_reports: list[str] = field(default_factory=list)
    missing_files: list[str] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)

    @property
    def real_count(self) -> int:
        return len(self.real_logs)

    @property
    def effective_duration(self) -> float:
        return round(sum(item.duration or 0 for item in self.real_logs), 2)

    @property
    def sample_count(self) -> int:
        return len(self.sample_logs)


def scan_logs(log_directory: Path) -> tuple[list[LogRecord], list[LogRecord]]:
    """扫描日志目录，返回（真实日志, 示例日志）。"""
    real: list[LogRecord] = []
    samples: list[LogRecord] = []
    for path in sorted(log_directory.glob("*_第*天_日志.md")):
        text = path.read_text(encoding="utf-8")
        number_match = NUMBER_PATTERN.search(path.name)
        if number_match is None:
            continue
        duration_match = DURATION_PATTERN.search(text)
        record = LogRecord(
            number=int(number_match.group(1)),
            path=str(path),
            sample=SAMPLE_MARKER in text,
            duration=float(duration_match.group(1)) if duration_match else None,
        )
        (samples if record.sample else real).append(record)
    return real, samples


def scan_task_numbers(task_directory: Path) -> list[int]:
    """返回有效任务文件的序号；归档与草稿目录不计入。"""
    numbers: list[int] = []
    for path in sorted(task_directory.glob("*_第*天_任务.md")):
        if "_archive" in path.parts or "_drafts" in path.parts:
            continue
        match = NUMBER_PATTERN.search(path.name)
        if match:
            numbers.append(int(match.group(1)))
    return numbers


def find_duplicates(values: list[int]) -> list[int]:
    seen: set[int] = set()
    duplicates: set[int] = set()
    for value in values:
        if value in seen:
            duplicates.add(value)
        seen.add(value)
    return sorted(duplicates)


def find_gaps(numbers: list[int]) -> list[int]:
    """返回 1～最大值之间缺失的序号。"""
    if not numbers:
        return []
    return sorted(set(range(1, max(numbers) + 1)) - set(numbers))


def audit(
    root: Path = REPOSITORY_ROOT,
    required_files: tuple[str, ...] = REQUIRED_PROJECT_FILES,
) -> AuditResult:
    result = AuditResult()
    result.real_logs, result.sample_logs = scan_logs(root / "daily_logs")
    result.task_numbers = scan_task_numbers(root / "daily_tasks")

    report_directory = root / "weekly_reports"
    if report_directory.is_dir():
        result.weekly_reports = sorted(
            path.name
            for path in report_directory.iterdir()
            if WEEKLY_REPORT_NAME_PATTERN.match(path.name)
        )

    result.missing_files = [
        relative for relative in required_files if not (root / relative).is_file()
    ]
    if result.missing_files:
        result.errors.append(f"缺少关键文件：{', '.join(result.missing_files)}")

    real_duplicates = find_duplicates([item.number for item in result.real_logs])
    if real_duplicates:
        result.errors.append(f"真实日志序号重复：{real_duplicates}")

    sample_duplicates = find_duplicates([item.number for item in result.sample_logs])
    if sample_duplicates:
        result.errors.append(f"示例日志序号重复：{sample_duplicates}")

    real_numbers = {item.number for item in result.real_logs}
    sample_numbers = {item.number for item in result.sample_logs}
    overlap = sorted(real_numbers & sample_numbers)
    if overlap:
        result.errors.append(f"示例日志占用了真实日志序号：{overlap}")

    task_duplicates = find_duplicates(result.task_numbers)
    if task_duplicates:
        result.errors.append(f"有效任务序号重复：{task_duplicates}")

    missing_durations = [
        item.path for item in result.real_logs if item.duration is None
    ]
    if missing_durations:
        result.errors.append(f"真实日志缺少学习时长：{missing_durations}")

    gaps = find_gaps(sorted(real_numbers))
    if gaps:
        result.warnings.append(f"真实日志存在缺号：{gaps}")

    logs_without_task = sorted(real_numbers - set(result.task_numbers))
    if logs_without_task:
        result.warnings.append(
            f"以下序号有日志但没有任务文件：{logs_without_task}（需确认是否为补记）"
        )

    if not result.weekly_reports:
        result.warnings.append("未找到周报文件")
    if not result.sample_logs:
        result.warnings.append("未找到示例日志；若已删除，请同步更新文档")

    return result


def render_markdown(result: AuditResult) -> str:
    real_numbers = [item.number for item in result.real_logs]
    lines = [
        "# 学习材料审计报告",
        "",
        "> 由 `scripts/audit_learning_materials.py` 生成。示例日志不计入统计。",
        "",
        "## 一、总览",
        "",
        "| 指标 | 值 |",
        "|---|---|",
        f"| 真实日志数 | {result.real_count} |",
        f"| 示例日志数 | {result.sample_count} |",
        f"| 有效学习时长 | {result.effective_duration:g} 小时 |",
        f"| 真实日志序号范围 | {min(real_numbers) if real_numbers else '-'}～{max(real_numbers) if real_numbers else '-'} |",
        f"| 有效任务数 | {len(result.task_numbers)} |",
        f"| 周报数 | {len(result.weekly_reports)} |",
        "",
        "## 二、错误",
        "",
    ]
    if result.errors:
        lines.extend(f"- {item}" for item in result.errors)
    else:
        lines.append("- 无。")

    lines.extend(["", "## 三、警告", ""])
    if result.warnings:
        lines.extend(f"- {item}" for item in result.warnings)
    else:
        lines.append("- 无。")

    lines.extend(["", "## 四、示例日志", ""])
    if result.sample_logs:
        lines.extend(
            f"- 第{item.number}天：`{Path(item.path).name}`" for item in result.sample_logs
        )
    else:
        lines.append("- 无。")

    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="审计学习材料的完整性与统计口径")
    parser.add_argument("--json", action="store_true", help="输出 JSON")
    parser.add_argument("--write", type=Path, help="把 Markdown 报告写入指定文件")
    arguments = parser.parse_args()

    result = audit()
    if arguments.json:
        print(
            json.dumps(
                {
                    "real_count": result.real_count,
                    "sample_count": result.sample_count,
                    "effective_duration": result.effective_duration,
                    "errors": result.errors,
                    "warnings": result.warnings,
                    "real_logs": [asdict(item) for item in result.real_logs],
                    "sample_logs": [asdict(item) for item in result.sample_logs],
                },
                ensure_ascii=False,
                indent=2,
            )
        )
    else:
        report = render_markdown(result)
        print(report)
        if arguments.write:
            arguments.write.write_text(report, encoding="utf-8")
            print(f"\n报告已写入：{arguments.write}")

    return 1 if result.errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
