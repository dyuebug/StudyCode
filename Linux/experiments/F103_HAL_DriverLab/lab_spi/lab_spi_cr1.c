/*
 * lab_spi_cr1.c —— SPI1 配置的主机端校验（教材 9.3–9.4）
 *
 * 不接板也能查清三件事：
 *   1. CubeMX 在 spi.c 里给的 11 个字段，落到 CR1/CR2 上究竟是哪几位、什么值；
 *   2. 分频比与 SCK 频率、每字节耗时的换算表（哪几档超规格）；
 *   3. "只收不发在物理上不存在"：HAL_SPI_Receive 在 master+2LINES 下就是把
 *      pData 当作发送缓冲，再让收到的字节覆盖它。
 *
 * 位定义逐条抄自仓库内头文件，不凭记忆：
 *   Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xb.h:9086-9113
 *   Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_spi.h:206-293
 *
 * 构建（宿主 gcc，不链接 HAL）：
 *   gcc -std=c11 -Wall -Wextra -o lab_spi_cr1 lab_spi_cr1.c
 *   ./lab_spi_cr1
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ---- CR1 位定义（stm32f103xb.h）---- */
#define SPI_CR1_CPHA      0x00000001u
#define SPI_CR1_CPOL      0x00000002u
#define SPI_CR1_MSTR      0x00000004u
#define SPI_CR1_BR_Msk    0x00000038u
#define SPI_CR1_SPE       0x00000040u
#define SPI_CR1_LSBFIRST  0x00000080u
#define SPI_CR1_SSI       0x00000100u
#define SPI_CR1_SSM       0x00000200u
#define SPI_CR1_DFF       0x00000800u   /* bit11：16 位帧 */
#define SPI_CR1_RXONLY    0x00000400u   /* bit10：只收模式 */
#define SPI_CR1_BIDIMODE  0x00008000u   /* bit15：单线双向模式 */

/* ---- 初始化字段的取值（stm32f1xx_hal_spi.h）---- */
#define SPI_MODE_MASTER          (SPI_CR1_MSTR | SPI_CR1_SSI)  /* :206 自带 SSI */
#define SPI_DIRECTION_2LINES     (0x00000000u)                 /* :214 */
#define SPI_DATASIZE_8BIT        (0x00000000u)                 /* :224 */
#define SPI_POLARITY_LOW         (0x00000000u)                 /* :233 */
#define SPI_PHASE_1EDGE          (0x00000000u)                 /* :242 */
#define SPI_NSS_SOFT             (SPI_CR1_SSM)                 /* :251 */
#define SPI_FIRSTBIT_MSB         (0x00000000u)                 /* :276 */
#define SPI_CRCCALCULATION_DISABLE (0x00000000u)               /* :293 */

typedef struct {
    const char *name;
    uint32_t    bits;
    unsigned    div;      /* fPCLK / div */
} presc_t;

static const presc_t PRESC[] = {
    {"SPI_BAUDRATEPRESCALER_2",   0x00u,   2u},
    {"SPI_BAUDRATEPRESCALER_4",   0x08u,   4u},   /* BR_0 = 1<<3 */
    {"SPI_BAUDRATEPRESCALER_8",   0x10u,   8u},
    {"SPI_BAUDRATEPRESCALER_16",  0x18u,  16u},
    {"SPI_BAUDRATEPRESCALER_32",  0x20u,  32u},
    {"SPI_BAUDRATEPRESCALER_64",  0x28u,  64u},
    {"SPI_BAUDRATEPRESCALER_128", 0x30u, 128u},
    {"SPI_BAUDRATEPRESCALER_256", 0x38u, 256u},
};

/* spi.c:41-51 的实测取值 */
#define INIT_MODE        SPI_MODE_MASTER
#define INIT_DIRECTION   SPI_DIRECTION_2LINES
#define INIT_DATASIZE    SPI_DATASIZE_8BIT
#define INIT_CPOL        SPI_POLARITY_LOW
#define INIT_CPHA        SPI_PHASE_1EDGE
#define INIT_NSS         SPI_NSS_SOFT
#define INIT_PRESC       (0x08u)              /* SPI_BAUDRATEPRESCALER_4 */
#define INIT_FIRSTBIT    SPI_FIRSTBIT_MSB
#define INIT_CRC         SPI_CRCCALCULATION_DISABLE

/*
 * 复刻 HAL_SPI_Init 的写法（stm32f1xx_hal_spi.c:90-101）：
 * 每个字段先与自己的掩码按位与，再整体或起来写进 CR1；CR2 只放 SSOE。
 */
static uint32_t encode_cr1(void)
{
    return ((INIT_MODE      & (SPI_CR1_MSTR | SPI_CR1_SSI)) |
            (INIT_DIRECTION & (SPI_CR1_RXONLY | SPI_CR1_BIDIMODE)) |
            (INIT_DATASIZE  & SPI_CR1_DFF) |
            (INIT_CPOL      & SPI_CR1_CPOL) |
            (INIT_CPHA      & SPI_CR1_CPHA) |
            (INIT_NSS       & SPI_CR1_SSM) |
            (INIT_PRESC     & SPI_CR1_BR_Msk) |
            (INIT_FIRSTBIT  & SPI_CR1_LSBFIRST) |
            (INIT_CRC       & 0x00000000u));
}

static void decode_cr1(uint32_t cr1)
{
    unsigned br = (cr1 & SPI_CR1_BR_Msk) >> 3;

    printf("  CPHA=%u CPOL=%u MSTR=%u SSM=%u SSI=%u SPE=%u LSBFIRST=%u DFF=%u BR=%u%u%u\n",
           (cr1 & SPI_CR1_CPHA) ? 1u : 0u,
           (cr1 & SPI_CR1_CPOL) ? 1u : 0u,
           (cr1 & SPI_CR1_MSTR) ? 1u : 0u,
           (cr1 & SPI_CR1_SSM) ? 1u : 0u,
           (cr1 & SPI_CR1_SSI) ? 1u : 0u,
           (cr1 & SPI_CR1_SPE) ? 1u : 0u,
           (cr1 & SPI_CR1_LSBFIRST) ? 1u : 0u,
           (cr1 & SPI_CR1_DFF) ? 1u : 0u,
           (br >> 2) & 1u, (br >> 1) & 1u, br & 1u);
    printf("  mode=%u (CPOL=%u,CPHA=%u)  br_code=%u  => fPCLK/%u\n",
           ((cr1 & SPI_CR1_CPOL) ? 2u : 0u) | ((cr1 & SPI_CR1_CPHA) ? 1u : 0u),
           (cr1 & SPI_CR1_CPOL) ? 1u : 0u,
           (cr1 & SPI_CR1_CPHA) ? 1u : 0u,
           br, 1u << (br + 1));
}

/* ---------------- 全双工模型 ---------------- */

/* 假装从机把收到的字节取反后回吐，便于看清"发出去的是什么" */
static uint8_t fake_slave_miso(uint8_t mosi)
{
    return (uint8_t)(mosi ^ 0xFFu);
}

/*
 * HAL_SPI_Receive 在 (MASTER && 2LINES) 下等价于
 * HAL_SPI_TransmitReceive(hspi, pData, pData, Size, Timeout)
 * —— 见 stm32f1xx_hal_spi.c:977-982。
 * 也就是说：调用者缓冲区里原来的内容会被当作 MOSI 数据发出去。
 */
static void hal_receive_model(uint8_t *pData, unsigned size)
{
    unsigned i;

    for (i = 0; i < size; i++) {
        uint8_t tx = pData[i];          /* 先进移位寄存器 */
        pData[i] = fake_slave_miso(tx); /* 收到的字节覆盖同一格 */
    }
}

int main(void)
{
    uint32_t cr1_disabled = encode_cr1();
    uint32_t cr1_enabled  = cr1_disabled | SPI_CR1_SPE;
    const double pclk2 = 72e6;

    printf("== 1. CR1/CR2（按 HAL_SPI_Init 的写法复刻）==\n");
    printf("CR1(未使能) = 0x%08X\n", cr1_disabled);
    printf("CR1(已使能) = 0x%08X\n", cr1_enabled);
    printf("CR2         = 0x%08X  （NSS 走软件，SSOE 不置位）\n", 0u);
    decode_cr1(cr1_enabled);

    /* 自校验：与手算值一致，否则说明本程序的位定义或掩码抄错了 */
    assert(cr1_disabled == 0x0000030Cu);
    assert(cr1_enabled  == 0x0000034Cu);

    /* SSI 必须为 1：SSM=1 时 SSI=0 会让主机看到 NSS 低电平 → MODF，主机位被清 */
    assert((cr1_enabled & SPI_CR1_SSM) && (cr1_enabled & SPI_CR1_SSI));

    printf("\n== 2. 分频比与 SCK（PCLK2 = 72 MHz）==\n");
    /* 18 MHz 是 F103 数据手册标称的 SPI1 上限；本仓库未收录该手册，属"标称值"，
       只用于说明"为什么最低分频档 /2 不能用、/4 是合法配置里最快的" */
    printf("%-28s %-6s %-10s %-12s %s\n", "宏", "code", "SCK", "us/Byte", "18MHz 标称上限");
    for (unsigned i = 0; i < sizeof(PRESC) / sizeof(PRESC[0]); i++) {
        double sck  = pclk2 / (double)PRESC[i].div;
        double us_b = 8.0 / sck * 1e6;
        printf("%-28s %-6u %-10.0f %-12.3f %s\n",
               PRESC[i].name, (PRESC[i].bits & SPI_CR1_BR_Msk) >> 3,
               sck, us_b, sck > 18e6 ? "超规格" : "ok");
    }

    printf("\n== 3. 片选与命令帧（W25Q64 读，driver 用 0x03 + 24 位地址）==\n");
    {
        uint32_t offset = 0x001234;
        uint8_t  cmd[4];
        unsigned len = 4096;

        cmd[0] = 0x03;
        cmd[1] = (uint8_t)((offset >> 16) & 0xFF);
        cmd[2] = (uint8_t)((offset >> 8) & 0xFF);
        cmd[3] = (uint8_t)(offset & 0xFF);
        printf("  Tx: %02X %02X %02X %02X（4 字节命令）\n",
               cmd[0], cmd[1], cmd[2], cmd[3]);
        printf("  读 %u 字节共 %u 个 SCK = %.0f us @18MHz\n",
               len, (4 + len) * 8, (4 + len) * 8 / 18.0);
        printf("  同样数据量走 I2C 100 kHz（每字节 9 SCL）：%.0f us\n",
               (4 + len) * 9 * 10.0);
    }

    printf("\n== 4. \"只收不发\"的真相 ==\n");
    {
        uint8_t buf[4] = {0xAA, 0xAA, 0xAA, 0xAA};   /* 未清空的缓冲区 */

        printf("  调用前 buf : %02X %02X %02X %02X\n",
               buf[0], buf[1], buf[2], buf[3]);
        hal_receive_model(buf, sizeof buf);
        printf("  调用后 buf : %02X %02X %02X %02X\n",
               buf[0], buf[1], buf[2], buf[3]);
        printf("  从机收到的 MOSI 就是调用前的 buf 内容（0xAA），不是 0x00\n");
    }

    return 0;
}
