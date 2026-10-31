#!/usr/bin/env bash
# 健康检查脚本：验证 HTTP 状态码与 JSON 结构。
# 退出码：0 正常；1 异常。

set -euo pipefail

HEALTH_URL="${HEALTH_URL:-http://127.0.0.1:8000/api/posts}"

response="$(curl -fsS --max-time 5 "${HEALTH_URL}")" || {
    echo "[失败] 无法访问 ${HEALTH_URL}" >&2
    exit 1
}

echo "${response}" | grep -q '"status": *"success"' || {
    echo "[失败] 响应内容不符合预期：${response}" >&2
    exit 1
}

echo "[成功] ${HEALTH_URL}"
