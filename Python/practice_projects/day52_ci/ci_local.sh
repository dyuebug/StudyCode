#!/usr/bin/env bash
# 第 52 天练习：在本地复现 CI 的核心检查。
# 用法：
#   bash practice_projects/day52_ci/ci_local.sh
#   SKIP_AUDIT=1 bash practice_projects/day52_ci/ci_local.sh
#   SKIP_FRONTEND=1 bash practice_projects/day52_ci/ci_local.sh
#
# 退出码：0 全部通过；1 至少一项失败。失败不中断，便于一次看全结果。

set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT_DIR"

if [[ -x "python_basics/.venv/Scripts/python.exe" ]]; then
    PYTHON="python_basics/.venv/Scripts/python.exe"
elif [[ -x "python_basics/.venv/bin/python" ]]; then
    PYTHON="python_basics/.venv/bin/python"
else
    PYTHON="${PYTHON:-python}"
fi

passed=0
failed=0
skipped=0
declare -a results=()

record() {
    local name="$1"
    local code="$2"
    if [[ "$code" -eq 0 ]]; then
        results+=("[通过] ${name}")
        passed=$((passed + 1))
    else
        results+=("[失败] ${name}（退出码 ${code}）")
        failed=$((failed + 1))
    fi
}

skip() {
    results+=("[跳过] $1")
    skipped=$((skipped + 1))
}

run() {
    local name="$1"
    shift
    echo "==> ${name}: $*"
    "$@"
    record "${name}" "$?"
}

command -v "$PYTHON" >/dev/null 2>&1 || { echo "找不到 Python：${PYTHON}" >&2; exit 1; }

run "语法编译" "$PYTHON" -m compileall -q tests scripts practice_projects
run "Python 测试" "$PYTHON" -m pytest -q

if [[ "${SKIP_AUDIT:-0}" == "1" ]]; then
    skip "依赖审计（SKIP_AUDIT=1）"
else
    run "依赖审计" "$PYTHON" -m pip_audit
fi

if [[ "${SKIP_FRONTEND:-0}" == "1" ]]; then
    skip "前端测试（SKIP_FRONTEND=1）"
elif command -v npm >/dev/null 2>&1 && [[ -f "web_projects/todo_app/package.json" ]]; then
    (
        cd web_projects/todo_app
        npm ci --silent && npm test
    )
    record "前端测试" "$?"
else
    skip "前端测试（未安装 npm 或缺少 package.json）"
fi

echo
echo "===== 结果汇总 ====="
printf '%s\n' "${results[@]}"
echo "通过 ${passed}，失败 ${failed}，跳过 ${skipped}"

[[ "$failed" -eq 0 ]]
