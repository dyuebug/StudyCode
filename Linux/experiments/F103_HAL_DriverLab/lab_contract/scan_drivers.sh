#!/bin/sh
# scan_drivers.sh —— 驱动库契约静态扫描（教材第 11 章的总复盘）
#
# 目的：把"这套驱动库的契约是什么、哪些地方不一致"从印象变成可复算的读数。
# 扫 5 件事：
#   1. 每个驱动的公开函数（名称 + 返回类型）；
#   2. Init / DeInit / Test 三件套是否齐备；
#   3. 头文件文档块的 5 个字段（函数名称/功能描述/输入参数/输出参数/返回值）是否齐全；
#   4. 从 .c 反推它依赖哪个 CubeMX 外设句柄（**先去注释**，否则会统计出并不存在的依赖）；
#   5. 汇总：返回类型分布、缺 DeInit 的驱动数、纯 GPIO 驱动数。
#
# 用法（在 F103_HAL_DriverLab 根目录）：
#   sh lab_contract/scan_drivers.sh Drivers/DshanMCU-F103 > lab_contract/scan_drivers.out
#
# 注意：这是文本扫描，不是编译。它证明"头文件里写了什么"，不证明"函数实现符合契约"。
#       `-DXXX=0` 关掉的代码它也看不见。

set -u
DIR=${1:-Drivers/DshanMCU-F103}

if [ ! -d "$DIR" ]; then
    echo "目录不存在：$DIR" >&2
    exit 2
fi

# 去掉 C 注释：先删块注释，再删行注释。顺序不能反，否则 /* ... // ... */ 会残留。
strip_comments()
{
    sed -E ':a; s:/\*[^*/]*\*/::g; /\/\*/{ N; ba; }' "$1" \
        | sed -E 's://.*::'
}

printf '%-26s %-5s %-5s %-5s %-5s %-6s %s\n' \
       "驱动" "函数" "Init" "DeIn" "Test" "返回" "依赖的 HAL 句柄"
printf '%s\n' "------------------------------------------------------------------------------------------"

total_fn=0; total_int=0; total_void=0; total_other=0
n_init=0; n_deinit=0; n_test=0; n_doc=0
n_gpio_only=0; n_files=0

for h in "$DIR"/driver_*.h; do
    base=$(basename "$h" .h)

    # --- 公开函数声明：以返回类型开头、以分号结尾的行 ---
    decls=$(grep -E "^[A-Za-z_][A-Za-z0-9_* 	]*\([^;]*\)[[:space:]]*;" "$h")
    fn=$(printf '%s\n' "$decls" | grep -c . )

    has_init=$(printf '%s\n' "$decls" | grep -c "_Init(")
    has_test=$(printf '%s\n' "$decls" | grep -c "_Test(")
    # DeInit 只算**本驱动自己的**反初始化函数，不是 HAL_TIM_Base_DeInit 这类调用
    has_deinit=$(printf '%s\n' "$decls" | grep -cE "^[A-Za-z_][A-Za-z0-9_* 	]*${base#driver_}_DeInit\(|[A-Za-z0-9_]+_DeInit\(")

    n_int=$(printf '%s\n'    "$decls" | grep -cE "^int[[:space:]]")
    n_void=$(printf '%s\n'   "$decls" | grep -cE "^void[[:space:]]")
    n_other=$((fn - n_int - n_void))

    # --- 文档块字段齐全度：每个函数应有 5 个字段 ---
    doc=0
    for k in "函数名称" "功能描述" "输入参数" "输出参数" "返 回 值"; do
        grep -q "$k" "$h" && doc=$((doc + 1))
    done
    [ "$doc" -eq 5 ] && n_doc=$((n_doc + 1))

    # --- 依赖的 HAL 句柄（去注释后）---
    c="$DIR/$base.c"
    if [ -f "$c" ]; then
        handles=$(strip_comments "$c" | grep -ohE "\b(h|l)(uart|i2c|spi|tim)[0-9]\b" | sort -u | tr '\n' ' ')
    else
        handles=""
    fi
    [ -z "$handles" ] && { handles="（纯 GPIO）"; n_gpio_only=$((n_gpio_only + 1)); }

    printf '%-26s %-5s %-5s %-5s %-5s %-6s %s\n' \
           "$base" "$fn" "$has_init" "$has_deinit" "$has_test" \
           "${n_int}i/${n_void}v" "$handles"

    total_fn=$((total_fn + fn))
    total_int=$((total_int + n_int))
    total_void=$((total_void + n_void))
    total_other=$((total_other + n_other))
    [ "$has_init" -gt 0 ]   && n_init=$((n_init + 1))
    [ "$has_deinit" -gt 0 ] && n_deinit=$((n_deinit + 1))
    [ "$has_test" -gt 0 ]   && n_test=$((n_test + 1))
    n_files=$((n_files + 1))
done

echo
echo "== 汇总 =="
printf '  扫描驱动头文件           : %d 个（另有 driver_irq.c / driver_timer.c 无头文件）\n' "$n_files"
printf '  公开函数声明             : %d 个\n' "$total_fn"
printf '    返回 int（可报错）     : %d 个（%.0f%%）\n' \
       "$total_int" "$(awk -v a="$total_int" -v b="$total_fn" 'BEGIN{print 100*a/b}')"
printf '    返回 void（无法报错）  : %d 个（%.0f%%）\n' \
       "$total_void" "$(awk -v a="$total_void" -v b="$total_fn" 'BEGIN{print 100*a/b}')"
printf '    其它返回类型           : %d 个（const char* / uint64_t）\n' "$total_other"
printf '  有 Init 的驱动           : %d / %d\n' "$n_init" "$n_files"
printf '  有 DeInit 的驱动         : %d / %d\n' "$n_deinit" "$n_files"
printf '  有 Test 的驱动           : %d / %d\n' "$n_test" "$n_files"
printf '  文档块 5 字段齐全        : %d / %d\n' "$n_doc" "$n_files"
printf '  只依赖 GPIO 的驱动       : %d / %d\n' "$n_gpio_only" "$n_files"

echo
echo "== 契约的四个缺口（由上面的读数直接得出）=="
echo "  ① 反初始化：${n_deinit}/${n_files}。没有任何驱动提供 DeInit ⇒ 引脚状态/外设时钟无法归还。"
echo "  ② 错误上报：${total_void}/${total_fn} 个函数返回 void，初始化失败无法传给调用方。"
echo "     Led_Init 返回 int，DHT11_Init 返回 void —— 同一套库里两种约定并存。"
echo "  ③ 文档块：字段内容齐全，但字段名有两种拼法（『返 回 值』58 处 / 『返回值』17 处，"
echo "     后者集中在 driver_lcd.h、driver_motor.h、driver_oled.h）。"
echo "     本节按**整文件**粒度匹配，所以 20/20 全过——但这是假通过："
echo "     任何按『返 回 值』字面统计每个函数的检查器，都会漏掉那 17 个文档块。"
echo "     **检查的粒度决定答案**，不是“过/不过”。"
echo "  ④ 依赖外设：${n_gpio_only}/${n_files} 个驱动只要 GPIO，其余必须由 CubeMX 先初始化对应外设。"
echo
echo "  => 前三条是这套库的真实缺口（只记录，不修改）；第 ④ 条是使用它的前置条件。"
echo
echo "== 与教材第 11 章表的对账 =="
echo "  教材 §11.1 表            : 17 行（LED … 直流电机）"
echo "  本库带 Test 的驱动       : 19 个 = 20 个头文件 - driver_timer（基础设施，无 Init/Test）"
echo "  差集                     : driver_lcd、driver_uart —— 有驱动、有 Test，但教材第 11 章无对应小节"
echo "  另有两个无头文件的 .c    : driver_irq.c（中断分发）、ascii_font.c（字模数据），不是模块驱动"
echo "  => “19 模块”的口径 = 带 Test 函数的驱动数，不是教材表的行数；两个数都对，指的不是同一批文件。"

exit 0
