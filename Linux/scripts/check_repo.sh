#!/usr/bin/env bash
# CLAUDE.md §13「变更检查清单」里能机器校验的那部分。
#
#   ./scripts/check_repo.sh
#
# 只读：不修改、不删除任何文件。输出一行一个结论，FAIL 会让退出码非零。
# 依赖：bash 4+、grep/sed/find/git（用 `git check-ignore` 区分"残留"与"已被忽略的产物"）。
# 不支持 Windows cmd/PowerShell（需在 WSL 内运行）。
#
# 判断口径（与 §1.1 事实来源优先级一致）：
#   * 结构类问题（缺 README、命名不合约定、断链）直接 FAIL；
#   * 内容类问题（文档声称了什么）本脚本不判，它读不出"是否实测"。
set -u
cd "$(dirname "$0")/.." || exit 2

fails=0
passes=0

ok() { printf 'LINUX|check|name=%s|status=PASS|detail=%s\n' "$1" "$2"; passes=$((passes + 1)); }
bad() { printf 'LINUX|check|name=%s|status=FAIL|detail=%s\n' "$1" "$2"; fails=$((fails + 1)); }
warn() { printf 'LINUX|check|name=%s|status=WARN|detail=%s\n' "$1" "$2"; }

# 1. 每个实验目录必须有 README.md
missing=""
for d in experiments/*/; do
        [ -d "$d" ] || continue
        [ -f "$d/README.md" ] || missing="$missing $(basename "$d")"
done
if [ -z "$missing" ]; then
        ok "experiments.readme" "所有实验目录都有 README.md"
else
        bad "experiments.readme" "缺少 README.md:$missing"
fi

# 2. 每个项目目录必须有 README.md
missing=""
for d in projects/*/; do
        [ -d "$d" ] || continue
        [ -f "$d/README.md" ] || missing="$missing $(basename "$d")"
done
if [ -z "$missing" ]; then
        ok "projects.readme" "所有项目目录都有 README.md"
else
        bad "projects.readme" "缺少 README.md:$missing"
fi

# 3. 命名约定（§5）
shopt -s nullglob
badnames=()
for f in daily_logs/*.md; do
        b=$(basename "$f")
        [ "$b" = "README.md" ] && continue
        printf '%s' "$b" | grep -qE '^[0-9]{4}-[0-9]{2}-[0-9]{2}_第[0-9]+天_日志\.md$' || badnames+=("daily_logs/$b")
done
for f in daily_tasks/*.md; do
        b=$(basename "$f")
        [ "$b" = "README.md" ] && continue
        printf '%s' "$b" | grep -qE '^[0-9]{4}-[0-9]{2}-[0-9]{2}_第[0-9]+天_任务\.md$' || badnames+=("daily_tasks/$b")
done
for f in weekly_reports/*.md; do
        b=$(basename "$f")
        [ "$b" = "README.md" ] && continue
        printf '%s' "$b" | grep -qE '^[0-9]{4}-W[0-9]{2}\.md$' || badnames+=("weekly_reports/$b")
done
if [ ${#badnames[@]} -eq 0 ]; then
        ok "naming" "日志/任务/周报文件名符合 §5"
else
        bad "naming" "命名不符合约定: ${badnames[*]}"
fi

# 4. README 里的相对链接必须存在（只查实验与项目，外链与锚点跳过）
broken=()
checked=0
for md in experiments/*/README.md projects/*/README.md; do
        [ -f "$md" ] || continue
        d=$(dirname "$md")
        while IFS= read -r target; do
                case $target in
                http* | '#'* | mailto:*) continue ;;
                esac
                # C++ 的 lambda 写法 `= [](int fd, ...)` 会被链接正则误当成 Markdown 链接。
                # 真路径不含空格与这些符号，含其一即判定为代码而非链接。
                case $target in
                *' '* | *'<'* | *'>'* | *'{'* | *'}'* | *','*) continue ;;
                esac
                path=${target%%#*}
                [ -z "$path" ] && continue
                checked=$((checked + 1))
                [ -e "$d/$path" ] || broken+=("$md -> $target")
        done < <(grep -oE '\]\([^)]+\)' "$md" | sed 's/^](//; s/)$//')
done
if [ ${#broken[@]} -eq 0 ]; then
        ok "links" "$checked 个相对链接全部可达"
else
        bad "links" "断链: ${broken[*]}"
fi

# 5. 不得残留临时文件与编译产物目录（已被 .gitignore 覆盖的不算残留，它们不会入库）
tmpfiles=$(find . -path ./Linux_WEIDS -prune -o \( -name '*.orig' -o -name '*.rej' -o -name '*.bak' -o -name '__pycache__' \) -print 2>/dev/null \
        | while IFS= read -r f; do
                git check-ignore -q "$f" 2>/dev/null || printf '%s\n' "$f"
        done)
if [ -z "$tmpfiles" ]; then
        ok "tempfiles" "无 .orig/.rej/.bak/__pycache__ 残留"
else
        bad "tempfiles" "残留: $(printf '%s' "$tmpfiles" | tr '\n' ' ')"
fi

# 6. 证据目录不能只有说明文件（§13「是否引用不存在的成果」）
for pair in "benchmarks:基准记录" "scripts:脚本"; do
        dir=${pair%%:*}
        n=$(find "$dir" -maxdepth 1 -type f ! -name 'README.md' ! -name '.gitignore' 2>/dev/null | wc -l)
        if [ "$n" -gt 0 ]; then
                ok "$dir.content" "$n 个实质文件"
        else
                warn "$dir.content" "只有 README.md，文档不得引用其中的成果"
        fi
done

printf 'LINUX|check|summary|pass=%d|fail=%d|warn=see_above\n' "$passes" "$fails"
[ "$fails" -eq 0 ] || exit 1
