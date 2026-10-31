/*
 * render_hanzi.c —— 主机端把 16x16 点阵画成字符画
 *
 * 目的：不接屏幕也能用"眼睛"验证两件事——
 *   ① 每字节内到底是 bit0 在上还是 bit7 在上（-D 选项直接对比两种解释）；
 *   ② upper[] / lower[] 的 16 字节切分是否对应 page y / page y+1。
 *
 * 构建（宿主 gcc，不链接 HAL）：
 *   gcc -std=c11 -Wall -Wextra -I ../../Core/Inc \
 *       render_hanzi.c ../../Core/Src/hanzi_font.c -o render_hanzi
 *
 * 用法：
 *   ./render_hanzi            # 按 SSD1306 约定（bit0 在上）画
 *   ./render_hanzi --msb      # 故意按 bit7 在上画，作为反例对照
 */
#include <stdio.h>
#include <string.h>

#include "hanzi_font.h"

/* 把一个字节的第 bit 位映射成"屏幕上从上往下第几行" */
static int row_of_bit(int bit, int msb_top)
{
    return msb_top ? (7 - bit) : bit;
}

static void draw_glyph(const hanzi16_t *g, int msb_top)
{
    int row, col;

    /* U+XXXX 用 ASCII 打印，避免不同终端的编码差异影响判断 */
    printf("U+%04X  (bit%d = top)\n", g->code, msb_top ? 7 : 0);

    for (row = 0; row < 16; row++) {
        for (col = 0; col < 16; col++) {
            uint8_t byte = (row < 8) ? g->upper[col] : g->lower[col];
            int bit      = (row < 8) ? row : (row - 8);
            int on       = (byte >> row_of_bit(bit, msb_top)) & 1;
            putchar(on ? '#' : '.');
        }
        if (row == 7)
            printf("   <- page y   (upper[0..15])");
        else if (row == 15)
            printf("   <- page y+1 (lower[0..15])");
        putchar('\n');
    }
    putchar('\n');
}

int main(int argc, char **argv)
{
    int msb_top = (argc > 1 && strcmp(argv[1], "--msb") == 0);
    unsigned i;

    printf("glyphs = %u, mode = %s\n\n", g_hanzi16_count,
           msb_top ? "bit7 on top (WRONG for SSD1306)" : "bit0 on top (SSD1306)");

    for (i = 0; i < g_hanzi16_count; i++)
        draw_glyph(&g_hanzi16[i], msb_top);

    /* 查表接口的自检：命中的三个码点 + 一个不该命中的 */
    {
        static const uint16_t probe[] = { 0x767E, 0x95EE, 0x7F51, 0x4E00 };
        unsigned k;
        for (k = 0; k < sizeof probe / sizeof probe[0]; k++)
            printf("find U+%04X -> %s\n", probe[k],
                   Hanzi16_Find(probe[k]) ? "hit" : "miss");
    }
    return 0;
}
