"""从真实学习日志生成周报。

原则：
1. 只汇总日志中明确记录的内容，不推断、不补造。
2. 缺少的字段写“日志未记录”，不填默认值。
3. 异常（序号与日期倒挂、同日多份日志）单独列出。

运行：
    python scripts/generate_weekly_reports.py
"""

from __future__ import annotations

import re
from collections import defaultdict
from dataclasses import dataclass, field
from datetime import date
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
LOG_DIRECTORY = REPOSITORY_ROOT / "daily_logs"
OUTPUT_DIRECTORY = REPOSITORY_ROOT / "weekly_reports"

DATE_PATTERN = re.compile(r"\*\*日期\*\*：(\d{4}-\d{2}-\d{2})")
NUMBER_PATTERN = re.compile(r"第(\d+)天")
DURATION_PATTERN = re.compile(r"\*\*学习时长\*\*：([0-9]+(?:\.[0-9]+)?)")
COMPLETION_PATTERN = re.compile(r"\*\*完成度\*\*：(\d+)%")
STAGE_PATTERN = re.compile(r"\*\*(?:学习)?阶段\*\*：(.+)")
SECTION_PATTERN = re.compile(r"^##\s+(.+)$", re.MULTILINE)
CHECKED_PATTERN = re.compile(r"^-\s*\[x\]\s*(.+)$", re.MULTILINE | re.IGNORECASE)
UNCHECKED_PATTERN = re.compile(r"^-\s*\[ \]\s*(.+)$", re.MULTILINE)
SUBHEADING_PATTERN = re.compile(r"^###\s+(.+)$", re.MULTILINE)

# 带此标记的文件是示例日志，不代表学习已发生，必须排除在统计之外。
SAMPLE_MARKER = "**文档性质**：示例日志（非真实学习记录"


def collect_log_paths() -> tuple[list[Path], list[Path]]:
    """返回（真实日志, 示例日志），保持文件名排序。"""
    paths = sorted(LOG_DIRECTORY.glob("*_第*天_日志.md"))
    real: list[Path] = []
    samples: list[Path] = []
    for path in paths:
        if SAMPLE_MARKER in path.read_text(encoding="utf-8"):
            samples.append(path)
        else:
            real.append(path)
    return real, samples


@dataclass
class StudyLog:
    number: int
    recorded_date: date
    path: Path
    duration: float | None = None
    completion: int | None = None
    stage: str = "日志未记录"
    sections: dict[str, str] = field(default_factory=dict)

    @property
    def checked_items(self) -> list[str]:
        return [item.strip() for item in CHECKED_PATTERN.findall(self.section("任务完成情况"))]

    @property
    def unchecked_items(self) -> list[str]:
        return [item.strip() for item in UNCHECKED_PATTERN.findall(self.section("任务完成情况"))]

    @property
    def knowledge_topics(self) -> list[str]:
        section = self.section("知识点")
        return [item.strip() for item in SUBHEADING_PATTERN.findall(section)]

    def section(self, keyword: str) -> str:
        for title, body in self.sections.items():
            if keyword in title:
                return body
        return ""

    def problems(self) -> str:
        for title, body in self.sections.items():
            if "问题" in title and "解决" in title:
                return body.strip() or "日志未记录"
        return "日志未记录"

    def test_results(self) -> str:
        for title, body in self.sections.items():
            if "测试" in title or "运行" in title:
                return body.strip() or "日志未记录"
        return "日志未记录"


def parse_log(path: Path) -> StudyLog:
    text = path.read_text(encoding="utf-8")

    number_match = NUMBER_PATTERN.search(path.name)
    date_match = DATE_PATTERN.search(text)
    if number_match is None or date_match is None:
        raise ValueError(f"无法解析日志：{path}")

    headings = list(SECTION_PATTERN.finditer(text))
    sections: dict[str, str] = {}
    for index, match in enumerate(headings):
        start = match.end()
        end = headings[index + 1].start() if index + 1 < len(headings) else len(text)
        sections[match.group(1).strip()] = text[start:end].strip()

    duration_match = DURATION_PATTERN.search(text)
    completion_match = COMPLETION_PATTERN.search(text)
    stage_match = STAGE_PATTERN.search(text)

    return StudyLog(
        number=int(number_match.group(1)),
        recorded_date=date.fromisoformat(date_match.group(1)),
        path=path,
        duration=float(duration_match.group(1)) if duration_match else None,
        completion=int(completion_match.group(1)) if completion_match else None,
        stage=stage_match.group(1).strip() if stage_match else "日志未记录",
        sections=sections,
    )


def format_hours(value: float | None) -> str:
    return f"{value:g} 小时" if value is not None else "日志未记录"


def render_report(year: int, week: int, logs: list[StudyLog], anomalies: list[Anomaly]) -> str:
    ordered = sorted(logs, key=lambda item: item.number)
    total_duration = sum(item.duration or 0 for item in ordered)
    lines: list[str] = [
        f"# {year} 年第 {week:02d} 周学习总结",
        "",
        "> 本报告由 `scripts/generate_weekly_reports.py` 从 `daily_logs/` 自动汇总。",
        "> 只包含日志中明确记录的内容；缺失字段保留“日志未记录”。",
        "",
        "## 一、本周日志清单",
        "",
        "| 学习序号 | 记录日期 | 时长 | 完成度 | 阶段 |",
        "|---|---|---|---|---|",
    ]

    for log in ordered:
        completion = f"{log.completion}%" if log.completion is not None else "日志未记录"
        lines.append(
            f"| 第{log.number}天 | {log.recorded_date.isoformat()} | "
            f"{format_hours(log.duration)} | {completion} | {log.stage} |"
        )

    lines.extend(
        [
            "",
            f"- 本周学习次数：**{len(ordered)} 次**",
            f"- 本周记录时长合计：**{total_duration:g} 小时**",
            "",
            "## 二、实际完成内容",
            "",
        ]
    )

    has_checked = False
    for log in ordered:
        if log.checked_items:
            has_checked = True
            lines.append(f"### 第 {log.number} 天")
            lines.extend(f"- {item}" for item in log.checked_items)
            lines.append("")
    if not has_checked:
        lines.append("日志未记录。")
        lines.append("")

    lines.extend(["## 三、计划与实际偏差", ""])
    has_unchecked = False
    for log in ordered:
        if log.unchecked_items:
            has_unchecked = True
            lines.append(f"- 第 {log.number} 天未完成：")
            lines.extend(f"  - {item}" for item in log.unchecked_items)
    if not has_unchecked:
        lines.append("日志未记录未完成项。")
    lines.append("")

    lines.extend(["## 四、知识点主题", ""])
    has_topics = False
    for log in ordered:
        if log.knowledge_topics:
            has_topics = True
            lines.append(f"- 第 {log.number} 天：" + "；".join(log.knowledge_topics))
    if not has_topics:
        lines.append("日志未记录。")
    lines.append("")

    lines.extend(["## 五、问题与解决", ""])
    for log in ordered:
        problems = log.problems()
        if problems != "日志未记录":
            lines.append(f"### 第 {log.number} 天")
            lines.append(problems)
            lines.append("")
    if all(log.problems() == "日志未记录" for log in ordered):
        lines.append("日志未记录。")
        lines.append("")

    lines.extend(["## 六、运行与测试结果", ""])
    has_results = False
    for log in ordered:
        results = log.test_results()
        if results != "日志未记录":
            has_results = True
            lines.append(f"### 第 {log.number} 天")
            lines.append(results)
            lines.append("")
    if not has_results:
        lines.append("日志未记录。")
        lines.append("")

    lines.extend(["## 七、资料异常与风险", ""])
    if anomalies:
        lines.extend(f"- {anomaly.text}" for anomaly in anomalies)
    else:
        lines.append("- 未发现本周范围内的序号或日期异常。")
    lines.append("")

    lines.extend(["## 八、下周重点（建议）", "", "待确认。不根据日志自动生成学习计划。", ""])
    return "\n".join(lines)


@dataclass
class Anomaly:
    text: str
    dates: set[date]
    numbers: set[int]

    def belongs_to_week(self, week_dates: set[date], week_numbers: set[int]) -> bool:
        return bool(self.dates & week_dates) or bool(self.numbers & week_numbers)


def detect_anomalies(all_logs: list[StudyLog]) -> list[Anomaly]:
    anomalies: list[Anomaly] = []

    by_date: dict[date, list[StudyLog]] = defaultdict(list)
    for log in all_logs:
        by_date[log.recorded_date].append(log)
    for recorded_date, logs in sorted(by_date.items()):
        if len(logs) > 1:
            sorted_logs = sorted(logs, key=lambda log: log.number)
            numbers = "、".join(f"第{log.number}天" for log in sorted_logs)
            anomalies.append(
                Anomaly(
                    text=f"{recorded_date.isoformat()} 同日存在多份日志：{numbers}。",
                    dates={recorded_date},
                    numbers={log.number for log in sorted_logs},
                )
            )

    chronological = sorted(all_logs, key=lambda log: log.recorded_date)
    for earlier, later in zip(chronological, chronological[1:]):
        if earlier.recorded_date == later.recorded_date:
            continue  # 同日多份日志已在上面单独记录
        if earlier.number > later.number:
            anomalies.append(
                Anomaly(
                    text=(
                        f"序号与日期倒挂：第{earlier.number}天（{earlier.recorded_date}）"
                        f"晚于第{later.number}天（{later.recorded_date}）的记录日期。"
                    ),
                    dates={earlier.recorded_date, later.recorded_date},
                    numbers={earlier.number, later.number},
                )
            )

    for log in all_logs:
        if log.number == 21:
            anomalies.append(
                Anomaly(
                    text="第21天累计时长存在 +0.5 小时记账偏差，见 `docs/资料异常记录.md` A-001。",
                    dates={log.recorded_date},
                    numbers={21},
                )
            )

    return anomalies


def render_sample_report(samples: list[StudyLog]) -> str:
    """生成示例日志汇总；单独成文，不计入正式周报索引。"""
    ordered = sorted(samples, key=lambda item: item.number)
    numbers = [item.number for item in ordered]
    span = f"第{min(numbers)}～{max(numbers)}天" if numbers else "无"
    lines = [
        f"# {span} 示例日志汇总（非真实学习记录）",
        "",
        "> **文档性质**：示例周报（非真实学习记录，不计入统计）。",
        "> 本文件由 `scripts/generate_weekly_reports.py` 从标记为“示例日志”的文件生成，",
        "> 仅用于演示周报结构与知识点组织方式，不进入 `INDEX.md` 的正式统计。",
        "",
        "## 一、示例日志清单",
        "",
        "| 学习序号 | 记录日期 | 时长字段 | 阶段 |",
        "|---|---|---|---|",
    ]
    for log in ordered:
        duration = "示例值" if log.duration is None else f"{log.duration:g} 小时"
        lines.append(
            f"| 第{log.number}天 | {log.recorded_date.isoformat()} | {duration} | {log.stage} |"
        )

    lines.extend(["", "## 二、知识点主题", ""])
    has_topics = False
    for log in ordered:
        if log.knowledge_topics:
            has_topics = True
            lines.append(f"- 第 {log.number} 天：" + "；".join(log.knowledge_topics))
    if not has_topics:
        lines.append("示例日志未记录知识点标题。")

    lines.extend(["", "## 三、统计口径", ""])
    lines.extend(
        [
            "- 示例日志不计入学习天数、累计时长与完成度。",
            "- 真实统计仍以 `INDEX.md` 为准。",
            "- 实际完成学习后，应删除示例标记并按真实结果重写日志，再重新生成正式周报。",
            "",
        ]
    )
    return "\n".join(lines)


def main() -> None:
    paths, samples = collect_log_paths()
    logs = [parse_log(path) for path in paths]
    if not logs:
        raise SystemExit("未找到学习日志。")

    all_anomalies = detect_anomalies(logs)
    grouped: dict[tuple[int, int], list[StudyLog]] = defaultdict(list)
    for log in logs:
        iso = log.recorded_date.isocalendar()
        grouped[(iso.year, iso.week)].append(log)

    OUTPUT_DIRECTORY.mkdir(exist_ok=True)

    index_lines = [
        "# 周报索引",
        "",
        "> 由 `scripts/generate_weekly_reports.py` 生成，数据来源为 `daily_logs/`。",
        "",
        "| 周次 | 学习次数 | 时长合计 | 涉及学习序号 |",
        "|---|---|---|---|",
    ]

    for (year, week), week_logs in sorted(grouped.items()):
        week_dates = {log.recorded_date for log in week_logs}
        week_numbers = {log.number for log in week_logs}
        relevant_anomalies = [
            anomaly for anomaly in all_anomalies if anomaly.belongs_to_week(week_dates, week_numbers)
        ]
        report = render_report(year, week, week_logs, relevant_anomalies)
        filename = f"{year}-W{week:02d}.md"
        (OUTPUT_DIRECTORY / filename).write_text(report, encoding="utf-8")

        total = sum(log.duration or 0 for log in week_logs)
        numbers = "、".join(str(log.number) for log in sorted(week_logs, key=lambda x: x.number))
        index_lines.append(f"| W{week:02d} | {len(week_logs)} | {total:g} 小时 | {numbers} |")

    (OUTPUT_DIRECTORY / "INDEX.md").write_text("\n".join(index_lines) + "\n", encoding="utf-8")
    message = f"已生成 {len(grouped)} 份正式周报，索引：{OUTPUT_DIRECTORY / 'INDEX.md'}"

    if samples:
        sample_logs = [parse_log(path) for path in samples]
        sample_report = OUTPUT_DIRECTORY / "示例_第47-52天_周报.md"
        sample_report.write_text(render_sample_report(sample_logs), encoding="utf-8")
        message += f"；跳过 {len(samples)} 份示例日志（不计入统计），示例汇总：{sample_report}"

    print(message)


if __name__ == "__main__":
    main()
