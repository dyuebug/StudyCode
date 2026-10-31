/*
 * lab_oled_hanzi.c —— 16x16 汉字显示（教材 8.5.1–8.5.4 的实现）
 *
 * 三层职责，与教材 8.1.3 的分层一一对应：
 *   hanzi_font.c      数据层：点阵字节（PCtoLCD2002 生成）
 *   本文件            协议层：UTF-8 解码 → 查表 → 算列地址 → 分两页写
 *   HAL_I2C_Mem_Write 控制器层：只认"设备地址 + 控制字节 + 数据"
 *
 * 关键设计取舍（见 notes/F103_HAL入门/Day10_OLED显示汉字与点阵.md）：
 *   - 不复制 driver_oled.c 的私有函数（OLED_WriteNBytes 是 static），改用公开的
 *     OLED_SetPosition 定位 + 一次 0x40 控制字节的数据写，共 3 次 I2C 传输/汉字
 *     （定位 3 条命令合并为一次 SetPosition 调用里的 3 次小传输，数据 2 次）。
 *   - 整字一次写完（16 字节），而不是逐列写：减少传输次数，也让"半字被覆盖"的
 *     窗口消失——driver_oled.c 的 OLED_PutChar 正是分两次写两页。
 */
#include "lab_oled_hanzi.h"
#include "hanzi_font.h"
#include "driver_oled.h"
#include "i2c.h"

#include <string.h>

#define HANZI_I2C_ADDR     0x78u   /* SSD1306，7 位地址 0x3C 左移一位（教材 8.4.3） */
#define HANZI_TIMEOUT      500u    /* ms，与 driver_oled.c 的 OLED_TIMEOUT 一致 */
#define HANZI_CTRL_DATA    0x40u   /* Co=0, D/C#=1：后续全是 GDDRAM 数据（教材 8.5.3） */
#define HANZI_CTRL_CMD     0x00u   /* Co=0, D/C#=0：后续是命令（本模块未用，仅注释对照） */

#define OLED_PAGE_NUM      8u
#define OLED_COL_NUM       128u

/* ---------------- 数据通道 ---------------- */

/*
 * 写 n 字节显存数据。MemAddress 位置放的是"控制字节"，不是寄存器地址——
 * 与 driver_mpu6050.c 里 HAL_I2C_Mem_Read(..., reg, ...) 的语义不同（Day09 §6）。
 */
static int hanzi_write_data(const uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Write(&hi2c1, HANZI_I2C_ADDR, HANZI_CTRL_DATA,
                             I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len, HANZI_TIMEOUT);
}

/* ---------------- 缺字占位 ---------------- */

/*
 * 查表失败时画一个 16x16 空心方框。
 * 与"什么都不画"相比，方框能区分"字库没这个字"和"屏幕/总线坏了"。
 */
static void hanzi_draw_box(uint8_t page, uint8_t col)
{
    static const uint8_t up_edge = 0xFF;          /* 整列 8 行全亮：左右竖边 */
    static const uint8_t lo_edge = 0xFF;
    uint8_t up[16], lo[16];
    int i;

    for (i = 0; i < 16; i++) {
        up[i] = 0x01;   /* bit0 在上：page y 只有最上面一行亮（上横边） */
        lo[i] = 0x80;   /* bit7 在下：page y+1 只有最下面一行亮（下横边） */
    }
    up[0] = up_edge; up[15] = up_edge;
    lo[0] = lo_edge; lo[15] = lo_edge;

    OLED_SetPosition(page, col);
    hanzi_write_data(up, 16);
    OLED_SetPosition((uint8_t)(page + 1u), col);
    hanzi_write_data(lo, 16);
}

/* ---------------- 单字绘制 ---------------- */

void OLED_PutHanzi16(uint8_t page, uint8_t col, const void *g)
{
    const hanzi16_t *glyph = (const hanzi16_t *)g;

    /* page+1 也要落在 0..7 内，否则下半部分写到页外（Day06 §2 的 y=7 缺陷同型） */
    if (page + 1u >= OLED_PAGE_NUM || col >= OLED_COL_NUM)
        return;

    if (glyph == 0) {
        hanzi_draw_box(page, col);
        return;
    }

    /* 上半：page, 从 col 起 16 列 */
    OLED_SetPosition(page, col);
    hanzi_write_data(glyph->upper, 16);

    /* 下半：page+1, 同一列范围 */
    OLED_SetPosition((uint8_t)(page + 1u), col);
    hanzi_write_data(glyph->lower, 16);
}

/* ---------------- UTF-8 解码 ---------------- */

/*
 * 返回字节数（1..4），并把码点写入 *cp；非法序列返回 0。
 * 为什么要自己解码：汉字在 UTF-8 里占 3 字节、在 GB2312 里占 2 字节，
 * 教材的点阵是按"一个字"生成的，程序必须先知道"下一个字从哪个字节开始"。
 */
static int utf8_next(const char *s, uint16_t *cp)
{
    const uint8_t *p = (const uint8_t *)s;

    if (p[0] < 0x80u) {                       /* 0xxxxxxx：ASCII */
        *cp = p[0];
        return 1;
    }
    if ((p[0] & 0xE0u) == 0xC0u && (p[1] & 0xC0u) == 0x80u) {
        *cp = (uint16_t)(((p[0] & 0x1Fu) << 6) | (p[1] & 0x3Fu));
        return 2;
    }
    if ((p[0] & 0xF0u) == 0xE0u &&
        (p[1] & 0xC0u) == 0x80u && (p[2] & 0xC0u) == 0x80u) {
        *cp = (uint16_t)(((uint16_t)(p[0] & 0x0Fu) << 12) |
                         ((uint16_t)(p[1] & 0x3Fu) << 6) |
                         (p[2] & 0x3Fu));
        return 3;
    }
    if ((p[0] & 0xF8u) == 0xF0u &&
        (p[1] & 0xC0u) == 0x80u && (p[2] & 0xC0u) == 0x80u &&
        (p[3] & 0xC0u) == 0x80u) {
        /* BMP 之外（如 emoji）：本字库不含，返回码点由查表失败兜底 */
        *cp = 0xFFFFu;
        return 4;
    }
    return 0;                                  /* 非法首字节/续字节，调用方应终止 */
}

/* ---------------- 字符串绘制 ---------------- */

int OLED_PrintHanzi(uint8_t page, uint8_t col, const char *utf8)
{
    int n = 0;

    if (utf8 == 0)
        return 0;

    while (*utf8) {
        uint16_t cp = 0;
        int len = utf8_next(utf8, &cp);

        if (len == 0)                          /* 非法 UTF-8：停止，不猜 */
            break;
        utf8 += len;

        if (cp == (uint16_t)'\n') {            /* 换行：下一页组 */
            col  = 0;
            page = (uint8_t)(page + 2u);
            continue;
        }

        if (page + 1u >= OLED_PAGE_NUM)        /* 页用尽，停止 */
            break;
        if ((unsigned)col + 16u > OLED_COL_NUM) {   /* 列不够：换行 */
            col  = 0;
            page = (uint8_t)(page + 2u);
            if (page + 1u >= OLED_PAGE_NUM)
                break;
        }

        OLED_PutHanzi16(page, col, Hanzi16_Find(cp));
        col = (uint8_t)(col + 16u);
        n++;
    }
    return n;
}

/* ---------------- 自测 ---------------- */

void OLED_Hanzi_Test(void)
{
    uint32_t cnt = 0;

    OLED_Init();
    OLED_Clear();

    OLED_PrintString(0, 0, "DShanMCU-103");   /* 8x16 ASCII，走课程驱动 */
    OLED_PrintHanzi(2, 0, "百问网");          /* 教材 8.5.4 的三个字，16x16 */
    OLED_PrintString(4, 0, "16x16 hanzi");    /* 第二行仍是 ASCII，便于对照字高 */

    for (;;) {
        /* 每轮只刷一小块，避免整屏刷新带来的闪烁（Day06 §3 的耗时结论） */
        OLED_PrintSignedVal(0, 6, (int32_t)(cnt & 0xFFFF));
        cnt++;
        HAL_Delay(200);
    }
}
