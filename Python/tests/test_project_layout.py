"""仓库结构与统计口径的回归测试。

目的：当目录、关键文档或学习日志被误改时，尽早失败并指出位置。
这些测试只读取文件，不修改任何内容。
"""

from __future__ import annotations

import re
from datetime import date
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]

LOG_PATTERN = re.compile(r"第(\d+)天")
DURATION_PATTERN = re.compile(r"\*\*学习时长\*\*：([0-9]+(?:\.[0-9]+)?)")
DATE_PATTERN = re.compile(r"\*\*日期\*\*：(\d{4}-\d{2}-\d{2})")

# 示例日志的文件头标记。带此标记的文件只用于演示格式，不代表学习已发生，
# 统计与测试必须跳过，否则会把示例内容算进真实天数与时长。
SAMPLE_MARKER = "**文档性质**：示例日志（非真实学习记录"


def log_files(include_samples: bool = False) -> list[Path]:
    """返回每日日志文件；默认排除标注为示例的文件。"""
    paths = sorted((REPOSITORY_ROOT / "daily_logs").glob("*_第*天_日志.md"))
    if include_samples:
        return paths
    return [path for path in paths if SAMPLE_MARKER not in path.read_text(encoding="utf-8")]


def is_sample_log(path: Path) -> bool:
    return SAMPLE_MARKER in path.read_text(encoding="utf-8")


def test_forum_project_layout_complete() -> None:
    required = [
        "forum_project/__init__.py",
        "forum_project/app.py",
        "forum_project/db.py",
        "forum_project/schema.sql",
        "forum_project/run.py",
        "forum_project/README.md",
        "forum_project/DESIGN.md",
        "forum_project/templates/base.html",
        "forum_project/tests/test_forum.py",
    ]
    missing = [path for path in required if not (REPOSITORY_ROOT / path).is_file()]
    assert not missing, f"缺少文件：{missing}"


def test_deployment_materials_complete() -> None:
    required = [
        "deployment/README.md",
        "deployment/验证记录_2026-10-29.md",
        "deployment/linux/01_Linux基础命令.md",
        "deployment/linux/02_进程与服务管理.md",
        "deployment/linux/03_Nginx反向代理.md",
        "deployment/docker/Dockerfile",
        "deployment/docker/docker-compose.yml",
        "deployment/nginx/forum.conf",
        "deployment/systemd/forum.service",
        "deployment/scripts/deploy.sh",
        "deployment/scripts/backup.sh",
        "deployment/scripts/healthcheck.sh",
        ".github/workflows/ci.yml",
        ".dockerignore",
    ]
    missing = [path for path in required if not (REPOSITORY_ROOT / path).is_file()]
    assert not missing, f"缺少文件：{missing}"


def test_deployment_scripts_use_lf_line_endings() -> None:
    """CRLF 会让脚本在 Linux 上以 $'\\r': command not found 失败。"""
    scripts = sorted((REPOSITORY_ROOT / "deployment/scripts").glob("*.sh"))
    assert scripts, "未找到部署脚本"
    offenders = [path.name for path in scripts if b"\r\n" in path.read_bytes()]
    assert not offenders, f"以下脚本包含 CRLF 换行：{offenders}"
    assert (REPOSITORY_ROOT / ".gitattributes").is_file(), "缺少 .gitattributes"


def test_learning_log_numbers_are_unique() -> None:
    real_numbers = sorted(
        int(LOG_PATTERN.search(path.name).group(1)) for path in log_files()
    )
    duplicates = sorted(
        {number for number in real_numbers if real_numbers.count(number) > 1}
    )
    assert not duplicates, f"日志学习序号重复：{duplicates}"
    assert real_numbers == list(range(1, 47)), "真实日志序号应为 1～46 连续且唯一"

    sample_numbers = sorted(
        int(LOG_PATTERN.search(path.name).group(1))
        for path in log_files(include_samples=True)
        if is_sample_log(path)
    )
    assert len(sample_numbers) == len(set(sample_numbers)), "示例日志序号重复"
    assert not set(sample_numbers) & set(real_numbers), "示例日志不得占用真实序号"


def test_sample_logs_are_marked_and_excluded() -> None:
    """示例日志必须自带标记，且不得声称真实完成。"""
    samples = [
        path for path in log_files(include_samples=True) if is_sample_log(path)
    ]
    assert samples, "未找到示例日志；若已删除，请同步更新本测试"

    for path in samples:
        text = path.read_text(encoding="utf-8")
        assert "不计入统计" in text, f"{path.name} 未声明不计入统计"
        assert "示例" in text, f"{path.name} 未标注为示例"

    claude_md = (REPOSITORY_ROOT / "CLAUDE.md").read_text(encoding="utf-8")
    assert "示例日志" in claude_md, "CLAUDE.md 未说明示例日志的统计口径"


def test_active_task_numbers_are_unique() -> None:
    task_dir = REPOSITORY_ROOT / "daily_tasks"
    task_files = [
        path
        for path in task_dir.glob("*_第*天_任务.md")
        if path.is_file() and "_archive" not in path.parts
    ]
    numbers = [int(LOG_PATTERN.search(path.name).group(1)) for path in task_files]
    duplicates = sorted({number for number in numbers if numbers.count(number) > 1})
    assert not duplicates, f"有效任务序号重复：{duplicates}"


def test_archived_duplicates_are_preserved() -> None:
    archive = REPOSITORY_ROOT / "daily_tasks" / "_archive"
    assert archive.is_dir(), "重复任务归档目录不存在"
    archived = list(archive.glob("*_第*天_任务.md"))
    assert len(archived) == 6, f"归档数量应为 6，实际 {len(archived)}"
    assert (archive / "README.md").is_file(), "归档清单缺失"


def test_effective_duration_matches_logs() -> None:
    total = 0.0
    for path in log_files():
        text = path.read_text(encoding="utf-8")
        match = DURATION_PATTERN.search(text)
        assert match is not None, f"{path.name} 缺少学习时长字段"
        total += float(match.group(1))

    assert round(total, 2) == 57.16, f"逐日时长求和应为 57.16，实际 {round(total, 2)}"

    claude_md = (REPOSITORY_ROOT / "CLAUDE.md").read_text(encoding="utf-8")
    assert "57.16" in claude_md, "CLAUDE.md 未记录有效累计时长 57.16"


def test_governance_documents_exist() -> None:
    for path in [
        "docs/资料异常记录.md",
        "docs/学习材料审计.md",
        "daily_tasks/INDEX.md",
        "daily_tasks/_补记说明_第39_41_42天.md",
        "daily_logs/INDEX.md",
        "scripts/generate_learning_logs.py",
        "tests/test_generate_learning_logs.py",
    ]:
        assert (REPOSITORY_ROOT / path).is_file(), f"缺少治理文档：{path}"


def test_knowledge_notes_exist() -> None:
    required = [
        "knowledge_base/阶段1_Python语法与数据结构详解.md",
        "knowledge_base/阶段2_面向对象与异常处理详解.md",
        "knowledge_base/阶段3_JavaScript进阶笔记.md",
        "knowledge_base/阶段3_数据库与SQLite详解.md",
        "knowledge_base/阶段4_Flask项目工程化笔记.md",
        "knowledge_base/阶段4_测试与依赖管理笔记.md",
        "knowledge_base/阶段5_Linux与Docker部署详解.md",
    ]
    missing = [path for path in required if not (REPOSITORY_ROOT / path).is_file()]
    assert not missing, f"缺少知识笔记：{missing}"


def test_learning_templates_exist() -> None:
    for path in [
        "daily_tasks/_模板_学习任务.md",
        "daily_logs/_模板_学习日志.md",
    ]:
        assert (REPOSITORY_ROOT / path).is_file(), f"缺少模板：{path}"


def test_weekly_reports_are_generated_from_logs() -> None:
    report_directory = REPOSITORY_ROOT / "weekly_reports"
    index = report_directory / "INDEX.md"
    assert index.is_file(), "缺少周报索引"

    log_dates = [
        date.fromisoformat(match.group(1))
        for path in log_files()
        if (match := DATE_PATTERN.search(path.read_text(encoding="utf-8")))
    ]
    expected_weeks = {value.isocalendar()[:2] for value in log_dates}

    reports = sorted(report_directory.glob("????-W??.md"))
    assert len(reports) == len(expected_weeks), (
        f"周报数量应与日志覆盖的 ISO 周数一致：{len(expected_weeks)}，实际 {len(reports)}"
    )

    for report in reports:
        text = report.read_text(encoding="utf-8")
        assert "自动汇总" in text, f"{report.name} 未标注自动生成来源"
        assert "本周学习次数" in text, f"{report.name} 缺少统计段落"

    weekly_hours = [float(value) for value in re.findall(r"\| ([0-9]+(?:\.[0-9]+)?) 小时 \|", index.read_text(encoding="utf-8"))]
    assert round(sum(weekly_hours), 2) == 57.16, "周报时长合计应与日志逐日求和一致"


def test_message_board_version_tests_exist() -> None:
    path = REPOSITORY_ROOT / "python_basics/留言板项目/test_message_board_versions.py"
    assert path.is_file(), "缺少留言板 v1～v3 测试"

    text = path.read_text(encoding="utf-8")
    for version in ["v1", "v2", "v3"]:
        assert f"test_{version}_" in text, f"缺少 {version} 测试"


def test_todo_storage_tests_exist() -> None:
    test_file = REPOSITORY_ROOT / "web_projects/todo_app/tests/storage.test.mjs"
    dom_test_file = REPOSITORY_ROOT / "web_projects/todo_app/tests/app.test.mjs"
    package_file = REPOSITORY_ROOT / "web_projects/todo_app/package.json"
    assert test_file.is_file(), "缺少 storage.js 单元测试"
    assert dom_test_file.is_file(), "缺少 app.js DOM 测试"
    assert package_file.is_file(), "缺少 npm test 配置"
    assert "node --test" in package_file.read_text(encoding="utf-8")
    assert "app.test.mjs" in package_file.read_text(encoding="utf-8")


def test_practice_projects_complete() -> None:
    """第 48～52 天练习项目必须各有入口、测试或可执行脚本。"""
    required = [
        "practice_projects/README.md",
        "practice_projects/day48_app_factory/app.py",
        "practice_projects/day48_app_factory/test_day48_app_factory.py",
        "practice_projects/day49_linux_tools/layout_checker.py",
        "practice_projects/day49_linux_tools/test_layout_checker.py",
        "practice_projects/day50_nginx_proxy/nginx.conf",
        "practice_projects/day50_nginx_proxy/compose.yaml",
        "practice_projects/day50_nginx_proxy/verify.sh",
        "practice_projects/day51_docker_app/Dockerfile",
        "practice_projects/day51_docker_app/compose.yaml",
        "practice_projects/day51_docker_app/test_day51_docker_app.py",
        "practice_projects/day52_ci/ci_local.sh",
        "practice_projects/day52_ci/github-workflow.yml",
    ]
    missing = [path for path in required if not (REPOSITORY_ROOT / path).is_file()]
    assert not missing, f"缺少练习文件：{missing}"


def test_practice_shell_scripts_use_lf() -> None:
    scripts = sorted((REPOSITORY_ROOT / "practice_projects").rglob("*.sh"))
    assert scripts, "未找到练习脚本"
    offenders = [path.name for path in scripts if b"\r\n" in path.read_bytes()]
    assert not offenders, f"以下练习脚本包含 CRLF 换行：{offenders}"
