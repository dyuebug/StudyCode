/**
 * F103_08：SPI1 主机读写外部 SPI NOR Flash（W25Q 系列）
 *
 * 硬件（课程 [09]_SPI/0901_spi_flash）：
 *   PA5 = SPI1_SCK，PA6 = SPI1_MISO，PA7 = SPI1_MOSI，PB9 = 片选（普通 GPIO 输出）
 *   开发板底板为 Flash 预留了独立排针（原理图中 4 根信号线标注 PA5/PA6/PA7/PB9 与 3.3 V），
 *   板上没有焊接存储芯片，必须外接 W25Qxx 模块才能读到 JEDEC ID。
 *
 * 时钟：SPI1 挂 APB2，本实验 PCLK2 = 8 MHz，BR = 001（/4）→ SCLK = 2 MHz。
 *       课程工程开了 PLL（72 MHz），同样 /4 得到 18 MHz —— 参数相同、结果不同的又一例。
 *
 * 总线约定：模式 0（CPOL = 0, CPHA = 0）、8 位、MSB 先行、软件 NSS。
 *           SPI 是全双工：发一个字节的同时必然收到一个字节，读数据就是"发哑字节换数据"。
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "stm32f1xx.h"
#include "f103_tick.h"
#include "f103_uart.h"

#define UART_BAUD          115200U
#define SPI_BYTE_TIMEOUT_MS 10U     /* 2 MHz 下一个字节约 4 us，10 ms 是 2500 倍余量 */

#define FLASH_CS_LOW()   (GPIOB->BSRR = GPIO_BSRR_BR9)
#define FLASH_CS_HIGH()  (GPIOB->BSRR = GPIO_BSRR_BS9)

/* W25Qxx 指令 */
#define FLASH_CMD_READ    0x03U   /* Read Data */
#define FLASH_CMD_PP      0x02U   /* Page Program（≤256 B，不得跨页） */
#define FLASH_CMD_SE      0x20U   /* Sector Erase 4 KB */
#define FLASH_CMD_WREN    0x06U   /* Write Enable，擦/写之前必须先发 */
#define FLASH_CMD_RDSR    0x05U   /* Read Status Register */
#define FLASH_CMD_JEDEC   0x9FU   /* JEDEC ID，返回 3 字节 */

#define FLASH_SR_WIP      0x01U   /* bit0：Write In Progress */

#define FLASH_PAGE_SIZE   256U
#define TEST_ADDR         0x000000U   /* 4 KB 扇区对齐 */

enum {
    OK = 0,
    ERR_SPI_TIMEOUT = -1,
    ERR_FLASH_BUSY = -2,
    ERR_PARAM = -3,
    ERR_PAGE = -4,
};

/* ---- 初始化 ------------------------------------------------------------- */

static void spi1_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_SPI1EN;

    /*
     * PA5 在 CRL 的 bit[23:20]，PA6 在 bit[27:24]，PA7 在 bit[31:28]。
     *   SCK / MOSI：复用推挽 50 MHz = CNF 10 + MODE 11 = 0xB（由 SPI 外设驱动）
     *   MISO      ：浮空输入         = CNF 01 + MODE 00 = 0x4（由从机驱动）
     * 半字节从高到低是 PA7/PA6/PA5 = B/4/B，组合值 0xB4B00000，掩码 0xFFF00000。
     * 注意：若写成 0xBF400000，得到的是 PA7=B、PA6=F、PA5=4 —— SCK 与 MISO 正好互换。
     */
    GPIOA->CRL = (GPIOA->CRL & ~0xFFF00000U) | 0xB4B00000U;

    /* PB9 = 通用推挽输出 50 MHz = 0x3，位于 CRH 的 bit[7:4]；片选空闲为高 */
    GPIOB->CRH = (GPIOB->CRH & ~(0xFU << 4)) | (0x3U << 4);
    FLASH_CS_HIGH();
}

static void spi1_init(void)
{
    /*
     * MSTR：主机。SSM + SSI：软件管理 NSS，且内部 NSS 强制为高。
     * 主机模式下若 SSI = 0 会被判为"被其他主机选中"，SPI 直接进入 MODF 故障并停摆——
     * 这是只接三根线（不用硬件 NSS 引脚）时最常见的坑。
     * BR_0：fPCLK2 / 4 = 2 MHz。
     */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

/* ---- SPI 字节传输 ------------------------------------------------------- */

static int spi_wait_flag(uint32_t mask)
{
    uint32_t start = f103_tick_now_ms();

    while ((SPI1->SR & mask) == 0U) {
        if ((uint32_t)(f103_tick_now_ms() - start) > SPI_BYTE_TIMEOUT_MS) {
            return ERR_SPI_TIMEOUT;
        }
    }
    return OK;
}

/*
 * 全双工交换一个字节：写 DR 启动发送，同时移位进来的字节落在 DR 里。
 * 必须先等 TXE（发送缓冲空）再写，写后等 RXNE（接收完成）再读。
 * 返回值只表示"这次交换有没有完成"，收到的数据通过 *rx 带回。
 */
static int spi_xfer(uint8_t tx, uint8_t *rx)
{
    int rc = spi_wait_flag(SPI_SR_TXE);
    if (rc != OK) {
        return rc;
    }

    SPI1->DR = tx;

    rc = spi_wait_flag(SPI_SR_RXNE);
    if (rc != OK) {
        return rc;
    }
    *rx = (uint8_t)SPI1->DR;
    return OK;
}

/*
 * 等 BSY 清零再拉高 CS。
 * TXE 只说明"发送缓冲空了"，最后一位可能还在移位寄存器里；
 * 此时提前拉高 CS 会把最后一位截断，表现为读回的数据末位随机错。
 */
static int spi_wait_idle(void)
{
    uint32_t start = f103_tick_now_ms();

    while ((SPI1->SR & SPI_SR_BSY) != 0U) {
        if ((uint32_t)(f103_tick_now_ms() - start) > SPI_BYTE_TIMEOUT_MS) {
            return ERR_SPI_TIMEOUT;
        }
    }
    return OK;
}

/* 收尾：等总线空闲 → 拉高 CS；把之前的错误码原样带出 */
static int flash_deselect(int rc)
{
    int rc_idle = spi_wait_idle();
    FLASH_CS_HIGH();
    return (rc != OK) ? rc : rc_idle;
}

/* ---- Flash 指令层 ------------------------------------------------------- */

/* 前提：CS 已经拉低。发送 1 字节指令 + 24 位地址（MSB 先行） */
static int flash_send_cmd_addr(uint8_t cmd, uint32_t addr)
{
    uint8_t dummy;

    int rc = spi_xfer(cmd, &dummy);
    if (rc == OK) {
        rc = spi_xfer((uint8_t)(addr >> 16), &dummy);
    }
    if (rc == OK) {
        rc = spi_xfer((uint8_t)(addr >> 8), &dummy);
    }
    if (rc == OK) {
        rc = spi_xfer((uint8_t)addr, &dummy);
    }
    return rc;
}

static int flash_write_enable(void)
{
    uint8_t dummy;

    FLASH_CS_LOW();
    int rc = spi_xfer(FLASH_CMD_WREN, &dummy);
    return flash_deselect(rc);
}

static int flash_read_status(uint8_t *sr)
{
    uint8_t dummy;

    FLASH_CS_LOW();
    int rc = spi_xfer(FLASH_CMD_RDSR, &dummy);
    if (rc == OK) {
        rc = spi_xfer(0xFFU, sr);
    }
    return flash_deselect(rc);
}

/* 擦除/写入期间 Flash 会自己忙，任何新指令都会被忽略，必须轮询 WIP */
static int flash_wait_ready(uint32_t timeout_ms)
{
    uint32_t start = f103_tick_now_ms();

    for (;;) {
        uint8_t sr = 0U;
        int rc = flash_read_status(&sr);
        if (rc != OK) {
            return rc;
        }
        if ((sr & FLASH_SR_WIP) == 0U) {
            return OK;
        }
        if ((uint32_t)(f103_tick_now_ms() - start) > timeout_ms) {
            return ERR_FLASH_BUSY;
        }
    }
}

static int flash_read_jedec(uint8_t id[3])
{
    uint8_t dummy;

    FLASH_CS_LOW();
    int rc = spi_xfer(FLASH_CMD_JEDEC, &dummy);
    for (uint32_t i = 0U; (rc == OK) && (i < 3U); i++) {
        rc = spi_xfer(0xFFU, &id[i]);
    }
    return flash_deselect(rc);
}

static int flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    FLASH_CS_LOW();
    int rc = flash_send_cmd_addr(FLASH_CMD_READ, addr);
    for (uint32_t i = 0U; (rc == OK) && (i < len); i++) {
        rc = spi_xfer(0xFFU, &buf[i]);
    }
    return flash_deselect(rc);
}

static int flash_page_program(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if ((len == 0U) || (len > FLASH_PAGE_SIZE)) {
        return ERR_PARAM;
    }
    /* 跨页时芯片会在页内回卷，把开头的字节覆盖掉 —— 必须由软件拆分 */
    if (((addr & 0xFFU) + len) > FLASH_PAGE_SIZE) {
        return ERR_PAGE;
    }

    int rc = flash_write_enable();
    if (rc != OK) {
        return rc;
    }

    FLASH_CS_LOW();
    rc = flash_send_cmd_addr(FLASH_CMD_PP, addr);
    for (uint32_t i = 0U; (rc == OK) && (i < len); i++) {
        uint8_t dummy;
        rc = spi_xfer(data[i], &dummy);
    }
    rc = flash_deselect(rc);
    if (rc != OK) {
        return rc;
    }
    return flash_wait_ready(100U);   /* 页编程典型 0.7 ms，最大 3 ms */
}

static int flash_erase_sector(uint32_t addr)
{
    int rc = flash_write_enable();
    if (rc != OK) {
        return rc;
    }

    FLASH_CS_LOW();
    rc = flash_send_cmd_addr(FLASH_CMD_SE, addr);
    rc = flash_deselect(rc);
    if (rc != OK) {
        return rc;
    }
    return flash_wait_ready(1000U);  /* 4 KB 扇区擦除典型 45 ms，最大 400 ms */
}

static const char *rc_name(int rc)
{
    switch (rc) {
    case OK:               return "ok";
    case ERR_SPI_TIMEOUT:  return "SPI 超时";
    case ERR_FLASH_BUSY:   return "Flash 忙超时";
    case ERR_PARAM:        return "参数错误";
    case ERR_PAGE:         return "跨页写入被拒绝";
    default:               return "未知错误";
    }
}

/* ---- 自检 --------------------------------------------------------------- */

static int flash_self_test(void)
{
    static const char msg[] = "F103 SPI flash self-test: erase + program + read back.";
    uint8_t buf[96];
    uint32_t t0, dt;
    int rc;

    printf("\r\n--- 自检：擦除 → 写入 → 回读（会清空 4 KB 扇区 0）---\r\n");

    t0 = f103_tick_now_ms();
    rc = flash_erase_sector(TEST_ADDR);
    dt = f103_tick_now_ms() - t0;
    printf("扇区擦除 : %-10s (%lu ms)\r\n", rc_name(rc), (unsigned long)dt);
    if (rc != OK) {
        return rc;
    }

    t0 = f103_tick_now_ms();
    rc = flash_page_program(TEST_ADDR, (const uint8_t *)msg, (uint32_t)sizeof(msg));
    dt = f103_tick_now_ms() - t0;
    printf("页编程 %2u B: %-10s (%lu ms)\r\n", (unsigned)sizeof(msg), rc_name(rc),
           (unsigned long)dt);
    if (rc != OK) {
        return rc;
    }

    rc = flash_read(TEST_ADDR, buf, (uint32_t)sizeof(msg));
    if (rc != OK) {
        printf("回读     : %s\r\n", rc_name(rc));
        return rc;
    }

    printf("回读内容 : %s\r\n", (const char *)buf);
    printf("原始字节 : ");
    for (uint32_t i = 0U; (i < 16U) && (i < sizeof(msg)); i++) {
        printf("%02X ", buf[i]);
    }
    printf("\r\n");

    if (memcmp(buf, msg, sizeof(msg)) == 0) {
        printf("结果     : PASS\r\n\r\n");
        return OK;
    }
    printf("结果     : FAIL（内容不一致）\r\n\r\n");
    return -1;
}

int main(void)
{
    f103_tick_init_irq();
    f103_uart_init(UART_BAUD);
    spi1_gpio_init();
    spi1_init();

    printf("\r\n[F103_08] SPI1 master, mode 0, 8-bit, SCLK = %lu Hz, CR1 = 0x%04lX\r\n",
           (unsigned long)(SystemCoreClock / 4U), (unsigned long)SPI1->CR1);

    uint8_t id[3] = {0U, 0U, 0U};
    int rc = flash_read_jedec(id);
    if (rc != OK) {
        printf("JEDEC ID 读取失败：%s\r\n", rc_name(rc));
        printf("检查模块的 SCK/MISO/MOSI/CS 与 3.3 V、GND 连接\r\n");
    } else {
        printf("JEDEC ID = %02X %02X %02X\r\n", id[0], id[1], id[2]);
        if ((id[0] == 0xEFU) && (id[2] >= 0x10U) && (id[2] <= 0x1FU)) {
            printf("厂商 Winbond，容量码 0x%02X → %lu KB\r\n", id[2],
                   (unsigned long)((1UL << id[2]) / 1024UL));
        } else if (id[0] == 0xFFU) {
            printf("全 0xFF：MISO 没有驱动，通常是没有接模块（浮空输入被读成高）\r\n");
        } else {
            printf("非 Winbond 厂商码，按原始值记录，不猜型号\r\n");
        }
        (void)flash_self_test();
    }

    for (;;) {
        f103_tick_delay_ms(3000U);

        rc = flash_read_jedec(id);
        if (rc == OK) {
            printf("[%lu ms] ID = %02X %02X %02X\r\n",
                   (unsigned long)f103_tick_now_ms(), id[0], id[1], id[2]);
        } else {
            printf("[%lu ms] ID 读取失败：%s\r\n",
                   (unsigned long)f103_tick_now_ms(), rc_name(rc));
        }
    }
}
