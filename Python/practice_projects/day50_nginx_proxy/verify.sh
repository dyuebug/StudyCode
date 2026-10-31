#!/usr/bin/env bash
# 第 50 天练习：验证反向代理与静态资源映射。
# 运行前先启动：docker compose -f .../compose.yaml up -d

set -euo pipefail

BASE_URL="${BASE_URL:-http://127.0.0.1:18080}"

check_status() {
    local path="$1"
    local expected="$2"
    local actual
    actual="$(curl -sS -o /dev/null -w '%{http_code}' "${BASE_URL}${path}")"
    if [[ "$actual" != "$expected" ]]; then
        echo "[失败] ${path} 期望 ${expected}，实际 ${actual}" >&2
        exit 1
    fi
    echo "[通过] ${path} -> ${actual}"
}

check_status "/" "200"
check_status "/static/app.css" "200"
check_status "/static/missing.css" "404"

echo "全部检查通过。502 验证：docker compose stop backend 后再次请求 /"
