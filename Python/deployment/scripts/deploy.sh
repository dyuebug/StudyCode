#!/usr/bin/env bash
# 学习论坛部署脚本（模板）
# 首次使用前：
#   1. 按 deployment/README.md 完成环境约定；
#   2. 赋予执行权限：chmod +x deployment/scripts/deploy.sh
# 本脚本不会自动创建用户、目录或环境变量文件，缺失时立即失败。

set -euo pipefail

APP_DIR="${APP_DIR:-/opt/study-forum}"
VENV_DIR="${VENV_DIR:-${APP_DIR}/.venv}"
SERVICE_NAME="${SERVICE_NAME:-study-forum}"
BRANCH="${BRANCH:-master}"
ENV_FILE="${ENV_FILE:-/etc/study-forum/forum.env}"
HEALTH_URL="${HEALTH_URL:-http://127.0.0.1:8000/api/posts}"

log() { printf '[%s] %s\n' "$(date '+%F %T')" "$*"; }
fail() { printf '[错误] %s\n' "$*" >&2; exit 1; }

log "检查前置条件"
[[ -d "${APP_DIR}" ]] || fail "应用目录不存在：${APP_DIR}"
[[ -x "${VENV_DIR}/bin/python" ]] || fail "虚拟环境不可用：${VENV_DIR}"
[[ -f "${ENV_FILE}" ]] || fail "环境变量文件不存在：${ENV_FILE}"
command -v git >/dev/null 2>&1 || fail "未安装 git"
command -v curl >/dev/null 2>&1 || fail "未安装 curl"

log "拉取代码（分支：${BRANCH}）"
cd "${APP_DIR}"
git fetch origin "${BRANCH}"
git checkout "${BRANCH}"
git pull --ff-only origin "${BRANCH}"

log "安装依赖"
"${VENV_DIR}/bin/pip" install --quiet -r requirements.txt

log "运行测试"
"${VENV_DIR}/bin/python" -m pytest forum_project/tests -q

log "重启服务"
sudo systemctl restart "${SERVICE_NAME}"

log "等待服务就绪"
for attempt in 1 2 3 4 5; do
    if curl -fsS --max-time 3 "${HEALTH_URL}" >/dev/null 2>&1; then
        log "健康检查通过"
        exit 0
    fi
    sleep 2
done

printf '[错误] 健康检查失败，最近日志：\n' >&2
sudo journalctl -u "${SERVICE_NAME}" -n 50 --no-pager >&2 || true
exit 1
