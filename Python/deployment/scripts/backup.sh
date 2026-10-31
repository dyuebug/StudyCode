#!/usr/bin/env bash
# SQLite 在线备份脚本（模板）
# 使用 sqlite3 CLI 的 .backup 命令，可在应用运行时执行。

set -euo pipefail

DB_PATH="${DB_PATH:-/var/lib/study-forum/forum.db}"
BACKUP_DIR="${BACKUP_DIR:-/var/backups/study-forum}"
RETENTION_DAYS="${RETENTION_DAYS:-14}"

timestamp="$(date '+%Y%m%d_%H%M%S')"
destination="${BACKUP_DIR}/forum_${timestamp}.db"

[[ -f "${DB_PATH}" ]] || { echo "[错误] 数据库不存在：${DB_PATH}" >&2; exit 1; }
command -v sqlite3 >/dev/null 2>&1 || { echo "[错误] 未安装 sqlite3" >&2; exit 1; }

mkdir -p "${BACKUP_DIR}"

echo "[$(date '+%F %T')] 备份 ${DB_PATH} -> ${destination}"
sqlite3 "${DB_PATH}" ".backup '${destination}'"
gzip -f "${destination}"

# 校验备份可读
gzip -t "${destination}.gz"
echo "[$(date '+%F %T')] 备份完成：${destination}.gz"

echo "[$(date '+%F %T')] 清理 ${RETENTION_DAYS} 天前的备份"
find "${BACKUP_DIR}" -name 'forum_*.db.gz' -type f -mtime "+${RETENTION_DAYS}" -print -delete
