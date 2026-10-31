/*
 * hanzi_font.h —— 16x16 汉字点阵（教材 8.5.4 的"百问网"三字）
 *
 * 数据来源：《HAL快速入门与项目实战(基于DshanMCU-103)》第 8.5.4 节，
 *           由 PCtoLCD2002 按"阴码 + 列行式 + 逆向"生成，原样抄录。
 *
 * 布局约定（与 SSD1306 页地址模式一致）：
 *   upper[0..15] —— 对应 page y   的 16 个列，每字节 1 列 8 个像素
 *   lower[0..15] —— 对应 page y+1 的 16 个列
 *   每字节内 **bit0 = 该页最上面一行**，bit7 = 最下面一行（与 SSD1306 GDDRAM 一致）
 *
 * 本文件不依赖 HAL，可被主机端程序直接编译（见 lab_oled_hanzi/render_hanzi.c）。
 */
#ifndef __HANZI_FONT_H
#define __HANZI_FONT_H

#include <stdint.h>

typedef struct {
    uint16_t code;        /* Unicode 码点（BMP 内，UTF-8 解码所得） */
    uint8_t  upper[16];   /* page y   ：16 列，每列 8 行，bit0 在上 */
    uint8_t  lower[16];   /* page y+1 ：16 列，每列 8 行，bit0 在上 */
} hanzi16_t;

extern const hanzi16_t g_hanzi16[];
extern const unsigned  g_hanzi16_count;

/* 按码点查表；找不到返回 NULL */
const hanzi16_t *Hanzi16_Find(uint16_t code);

#endif /* __HANZI_FONT_H */
