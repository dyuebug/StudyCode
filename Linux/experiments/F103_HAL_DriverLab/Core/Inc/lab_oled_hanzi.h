/*
 * lab_oled_hanzi.h —— 在 SSD1306 上显示 16x16 汉字（教材 8.5 的落地实现）
 *
 * 与课程 driver_oled.c 的关系：
 *   driver_oled.c 只支持 8x16 的 ASCII（ascii_font[128][16]），没有任何汉字路径。
 *   本模块**不改动**课程驱动，只复用它的公开接口 OLED_SetPosition(page, col)，
 *   再自己发控制字节 0x40 写数据——这正是教材 8.5.3 讲的两类字节。
 */
#ifndef __LAB_OLED_HANZI_H
#define __LAB_OLED_HANZI_H

#include <stdint.h>

/*
 * 在 (page, col) 显示一个 UTF-8 字符串，每个汉字占 16x16 像素（两页 × 16 列）。
 *
 * page ：起始页地址 0..6（汉字跨 page 与 page+1，故 7 不可用）
 * col  ：起始列地址 0..112（像素列，须是 8 的倍数才能对齐）
 * utf8 ：UTF-8 编码字符串；'\n' 换行（page += 2, col = 0）
 *
 * 返回值：实际画出的字形数（含查表失败画出的方框）
 * 错误处理：字库未收录的码点画一个 16x16 空心方框，不报错、不丢字符
 */
int OLED_PrintHanzi(uint8_t page, uint8_t col, const char *utf8);

/* 画单个码点；g 由 Hanzi16_Find 提供，传 NULL 画方框 */
void OLED_PutHanzi16(uint8_t page, uint8_t col, const void *g);

/* 模块自测（LAB_TEST=19）：先 ASCII 后汉字，最后滚动显示一个计数 */
void OLED_Hanzi_Test(void);

#endif /* __LAB_OLED_HANZI_H */
