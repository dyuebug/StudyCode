"""第 49 天练习：Linux 部署检查的可测试实现。

把常见的“登录服务器后手工检查”固化成函数：
- 检查目录是否存在、是否可写；
- 解析 `ss -lntp` 输出，定位监听端口与进程；
- 输出目录权限的八进制表示。

这些函数不执行系统命令，输入来自调用方，因此可在 Windows 上测试。

运行示例：
    python practice_projects/day49_linux_tools/layout_checker.py
"""

from __future__ import annotations

import os
import re
import stat
from pathlib import Path

LISTEN_PATTERN = re.compile(
    r"LISTEN\s+\d+\s+\d+\s+\S*?:(\d+)\s+\S+\s+users:\(\("
    r'"([^"]+)",pid=(\d+)'
)
FALLBACK_PORT_PATTERN = re.compile(r":(\d+)\s")


def parse_listening_ports(ss_output: str) -> list[dict[str, object]]:
    """解析 `ss -lntp` 输出，返回端口、进程名与 PID。"""
    results: list[dict[str, object]] = []
    for line in ss_output.splitlines():
        match = LISTEN_PATTERN.search(line)
        if match:
            results.append(
                {
                    "port": int(match.group(1)),
                    "process": match.group(2),
                    "pid": int(match.group(3)),
                }
            )
            continue
        # 没有进程信息时仍提取端口，便于在无权限的容器中排查。
        port_match = FALLBACK_PORT_PATTERN.search(line)
        if line.strip().startswith("LISTEN") and port_match:
            results.append(
                {
                    "port": int(port_match.group(1)),
                    "process": None,
                    "pid": None,
                }
            )
    return results


def permission_octal(path: Path) -> str:
    """返回权限的八进制表示，例如 750。"""
    mode = path.stat().st_mode
    return f"{stat.S_IMODE(mode):03o}"


def check_layout(root: Path, directories: list[str]) -> dict[str, object]:
    """检查部署目录是否存在、是否为目录、是否可写。"""
    missing: list[str] = []
    not_directory: list[str] = []
    not_writable: list[str] = []

    for relative in directories:
        target = root / relative
        if not target.exists():
            missing.append(relative)
        elif not target.is_dir():
            not_directory.append(relative)
        elif not os.access(target, os.W_OK):
            not_writable.append(relative)

    return {
        "root": str(root),
        "checked": directories,
        "missing": missing,
        "not_directory": not_directory,
        "not_writable": not_writable,
        "ok": not missing and not not_directory and not not_writable,
    }


if __name__ == "__main__":
    sample = """State Recv-Q Send-Q Local Address:Port Peer Address:Port Process
LISTEN 0      4096   127.0.0.1:8000      0.0.0.0:*    users:(("gunicorn",pid=1201,fd=3))
LISTEN 0      511    0.0.0.0:80          0.0.0.0:*
"""
    print(parse_listening_ports(sample))
    print(check_layout(Path.cwd(), ["practice_projects", "daily_logs"]))
