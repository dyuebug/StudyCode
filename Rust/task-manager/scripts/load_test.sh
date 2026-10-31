#!/usr/bin/env bash
# ============================================
# task-manager 并发压测脚本（第 39 天）
# ============================================
#
# 用法：
#   BASE=http://127.0.0.1:8095 N=100 P=10 ./scripts/load_test.sh
#
#   BASE  服务地址（默认 127.0.0.1:8095）
#   N     总请求数（默认 100）
#   P     并发度（默认 10，即最多 10 个 curl 同时在飞）
#   MODE  tasks（默认，往 /projects/1/tasks 灌任务）
#         projects（创建项目，唯一名，验证唯一索引在并发下不炸）
#
# 输出：状态码分布 + 最大/平均耗时。**给的是数字，不是感觉。**
#
# 【为什么并发度 10 而 worker 是 4】
# 并发度指"同时在飞的请求数"，worker 指"能同时处理请求的线程数"。
# 两者不必相等：并发 > worker 才能形成排队，才测得到池和写锁的争用。
# 并发 == worker 时，请求刚好一人一个，什么都压不出来。

set -u

BASE=${BASE:-http://127.0.0.1:8095}
N=${N:-100}
P=${P:-10}
MODE=${MODE:-tasks}
TOKEN=${TOKEN:-token-alice}
OUT=$(mktemp -d)

export BASE N TOKEN OUT MODE

echo "目标：$BASE   请求数：$N   并发度：$P   模式：$MODE"

seq 1 "$N" | xargs -P "$P" -I{} bash -c '
  if [ "$MODE" = "projects" ]; then
    # 每个请求用不同名字：不会撞唯一约束，测的是纯写入能力
    #
    # ⚠️ description 必须是纯 ASCII。Git Bash 里 curl 的 `-d` 会把中文
    # 按本地代码页（GBK）发出去，服务端收到的是非法 UTF-8，
    # 于是 100 个请求全是 400 `invalid unicode code point`——
    # 这次真的踩到了（第 44 周踩过同一个坑）。**先怀疑请求本身是否合法**，
    # 再去怀疑服务端逻辑。
    URL="$BASE/projects"
    BODY="{\"name\":\"load-{}-$$\",\"description\":\"load-test\"}"
  elif [ "$MODE" = "tags" ]; then
    # 事务模式：`INSERT tags` → `SELECT id` → `INSERT task_tags` → COMMIT
    # 写窗口比单条 INSERT 长得多，最容易撞上写锁
    URL="$BASE/tasks/1/tags"
    BODY="{\"name\":\"tag-{}-$$\"}"
  else
    URL="$BASE/projects/1/tasks"
    BODY="{\"title\":\"load-{}\",\"priority\":2}"
  fi

  # 【为什么每个请求写自己的文件，而不是统统 >> 同一个文件】
  # 第一版就是直接 `>> "$OUT/codes.txt"`，结果 200 个请求只记到 105 行：
  # 20 个并发进程同时追加，行是「短追加」而非原子追加，会互相覆盖。
  # 服务端日志显示 200 个请求全部 201——**丢的是测量数据，不是请求**。
  # 这类错误最像"服务有毛病"，实际是尺子坏了。
  F="$OUT/r-{}-$$.txt"
  curl -s -o /dev/null -w "%{http_code} %{time_total}\n" \
    -X POST "$URL" \
    -H "Authorization: Bearer $TOKEN" \
    -H "Content-Type: application/json" \
    -d "$BODY" > "$F" 2>&1
  RC=$?
  # curl 自己失败（连不上、被重置）时 -w 也可能不输出，单独补一行
  [ "$RC" -ne 0 ] && echo "curl_rc_$RC 0" >> "$F"
  exit 0
'

echo "────────────── 结果 ──────────────"
cat "$OUT"/r-*.txt > "$OUT/all.txt"
echo "收到响应数：$(wc -l < "$OUT/all.txt" | tr -d ' ')"
awk '
{
  c[$1]++
  sum += $2
  if ($2 > max) max = $2
  if (min == "" || $2 < min) min = $2
}
END {
  for (k in c) printf "  状态 %s：%d 次\n", k, c[k]
  printf "  最小耗时：%.3f s\n", min
  printf "  平均耗时：%.3f s\n", sum / NR
  printf "  最大耗时：%.3f s\n", max
}' "$OUT/all.txt"

rm -rf "$OUT"
