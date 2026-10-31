"""第 49 天练习的测试：目录检查与 `ss` 输出解析。"""

from __future__ import annotations

import importlib.util
import os
import sys
from pathlib import Path

import pytest

MODULE_PATH = Path(__file__).with_name("layout_checker.py")


@pytest.fixture(scope="module")
def checker():
    spec = importlib.util.spec_from_file_location("day49_layout_checker", MODULE_PATH)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def test_parse_listening_ports_with_process(checker) -> None:
    output = (
        "State Recv-Q Send-Q Local Address:Port Peer Address:Port Process\n"
        'LISTEN 0 4096 127.0.0.1:8000 0.0.0.0:* users:(("gunicorn",pid=1201,fd=3))\n'
    )

    assert checker.parse_listening_ports(output) == [
        {"port": 8000, "process": "gunicorn", "pid": 1201}
    ]


def test_parse_listening_ports_without_process(checker) -> None:
    output = "LISTEN 0 511 0.0.0.0:80 0.0.0.0:*\n"

    assert checker.parse_listening_ports(output) == [
        {"port": 80, "process": None, "pid": None}
    ]


def test_check_layout_reports_missing_directory(checker, tmp_path: Path) -> None:
    (tmp_path / "data").mkdir()

    result = checker.check_layout(tmp_path, ["data", "logs"])

    assert result["missing"] == ["logs"]
    assert result["ok"] is False


def test_check_layout_accepts_writable_directory(checker, tmp_path: Path) -> None:
    (tmp_path / "data").mkdir()

    result = checker.check_layout(tmp_path, ["data"])

    assert result["missing"] == []
    assert result["not_writable"] == []
    assert result["ok"] is True


@pytest.mark.skipif(os.name == "nt", reason="Windows 的 chmod 只影响只读位，无法覆盖 640 语义")
def test_permission_octal(checker, tmp_path: Path) -> None:
    target = tmp_path / "forum.env"
    target.write_text("SECRET_KEY=test", encoding="utf-8")
    target.chmod(0o640)

    assert checker.permission_octal(target) == "640"
