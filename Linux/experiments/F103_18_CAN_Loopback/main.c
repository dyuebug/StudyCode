/**
 * F103_18：bxCAN 回环模式自测
 *
 * 为什么用回环（BTR.LBKM）：回环模式下发送帧在芯片内部直接进接收 FIFO，
 * 不经过收发器，不需要第二块板、不需要 CAN 分析仪、不需要 120 Ω 终端电阻。
 * 这是唯一能在单板上把"配置 → 发送 → 过滤 → 接收"整条链路走完的办法。
 * 真板验证时它仍然有效；要验证真实总线电气特性则是另一回事（需示波器 + 对端节点）。
 *
 * 位时序（PCLK1 = 8 MHz，复位后 PPRE1=000 不分频）：
 *   BRP[9:0] = 0     → tq = 1 / 8 MHz = 125 ns
 *   TS1[3:0] = 12    → 段 1 = 13 tq
 *   TS2[2:0] = 1     → 段 2 = 2 tq
 *   1 位 = 1(SYNC) + 13 + 2 = 16 tq = 2 µs → 500 kbps
 *   SJW = 1 tq
 *   BTR = LBKM(bit30) | (12<<16) | (1<<20) = 0x401C0000
 *
 * 初始化顺序（RM0008 §24.5，顺序错了会被硬件忽略）：
 *   1. 开 CAN1 时钟（APB1）与 GPIOA 时钟（APB2）
 *   2. PA11 = 浮空输入（CAN_RX），PA12 = 复用推挽（CAN_TX）
 *   3. MCR.INRQ = 1 → 等 MSR.INAK = 1（硬件确认已进初始化模式）
 *   4. 写 BTR / 过滤器（**过滤器只能在 FINIT=1 时改**）
 *   5. FMR.FINIT = 0，FA1R 激活滤波器组
 *   6. MCR.INRQ = 0 → 等 MSR.INAK = 0（进入正常/回环工作）
 *
 * 本机限制（实测，见 Linux/tests/qemu-stm32/logs/probe.log）：
 *   QEMU 的 stm32vldiscovery 没有 CAN 模型，并且 0x40006400 这一段根本没映射，
 *   第一次访问就 BusFault → HardFault。因此本实验在 QEMU 里只能给出"访问位置"
 *   与故障解码，不能给出任何功能结论；下面的故障钩子就是为此准备的。
 *   在真板上这一整段会正常跑完并打印收发比对结果。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_tick.h"
#include "f103_uart.h"

#define UART_BAUD    115200U
#define POLL_LIMIT   100000U     /* 轮询上限，非时间单位 */

#define CAN_BTR_500K_8MHZ  0x401C0000UL   /* LBKM | TS1=12 | TS2=1 | BRP=0 */
#define CAN_STD_ID         0x123U
#define CAN_DLC            8U

/* 故障钩子：未映射地址的访问会升级成 HardFault，默认 Handler 是死循环，
 * 日志会停在半截。这里只取证 + 停机，不做恢复（恢复要改返回地址，超出本实验范围）。 */
static void fault_report(const char *name)
{
    printf("\r\n[FAULT] %s: CFSR=0x%08lX HFSR=0x%08lX BFAR=0x%08lX\r\n",
           name, (unsigned long)SCB->CFSR, (unsigned long)SCB->HFSR,
           (unsigned long)SCB->BFAR);
    printf("[FAULT] BFAR 即出错地址。0x40006400 是 CAN1 基址：本机未建模且未映射，\r\n");
    printf("[FAULT] 所以 QEMU 里无法验证 CAN 功能，只能作构建级 + 寄存器路径证据。\r\n");
    for (;;) {
        __asm volatile("nop");
    }
}

void BusFault_Handler(void)
{
    fault_report("BusFault");
}

void HardFault_Handler(void)
{
    fault_report("HardFault");
}

static uint32_t pack_le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void unpack_le(uint32_t reg, uint8_t *p)
{
    p[0] = (uint8_t)(reg & 0xFFU);
    p[1] = (uint8_t)((reg >> 8) & 0xFFU);
    p[2] = (uint8_t)((reg >> 16) & 0xFFU);
    p[3] = (uint8_t)((reg >> 24) & 0xFFU);
}

/* 等某位置位 / 清零，带迭代上限；返回 1 表示等到 */
static uint32_t wait_set(volatile uint32_t *reg, uint32_t mask)
{
    for (uint32_t i = 0U; i < POLL_LIMIT; i++) {
        if ((*reg & mask) != 0U) {
            return 1U;
        }
    }
    return 0U;
}

static uint32_t wait_clear(volatile uint32_t *reg, uint32_t mask)
{
    for (uint32_t i = 0U; i < POLL_LIMIT; i++) {
        if ((*reg & mask) == 0U) {
            return 1U;
        }
    }
    return 0U;
}

/* 过滤器 0：32 位掩码模式、掩码全 0 = 全部通过、挂 FIFO0。
 * 调试阶段先全通，确认链路通了再把掩码收紧——这样失败时只需怀疑一处。 */
static void can_filter_all_pass(void)
{
    CAN1->FMR  |= CAN_FMR_FINIT;         /* 只有 FINIT=1 时过滤器寄存器才可写 */
    CAN1->FM1R &= ~CAN_FM1R_FBM0;        /* 0 = 位宽 32 位（1 = 16 位两个） */
    CAN1->FS1R &= ~CAN_FS1R_FSC0;        /* 0 = 掩码模式（1 = 标识符列表） */
    CAN1->FFA1R &= ~CAN_FFA1R_FFA0;      /* 0 = 收到的帧进 FIFO0 */
    CAN1->sFilterRegister[0].FR1 = 0U;   /* 标识符寄存器 = 0 */
    CAN1->sFilterRegister[0].FR2 = 0U;   /* 掩码全 0 → 任意标识符都匹配 */
    CAN1->FA1R |= CAN_FA1R_FACT0;        /* 激活滤波器组 0 */
    CAN1->FMR &= ~CAN_FMR_FINIT;         /* 离开过滤器初始化模式 */
}

int main(void)
{
    f103_tick_init_irq();                /* 用毫秒计数给各步骤计时 */
    f103_uart_init(UART_BAUD);

    printf("\r\n=== F103_18 bxCAN 回环自测 ===\r\n");

    /* ---- 1. GPIO：PA11 = CAN_RX（浮空输入），PA12 = CAN_TX（复用推挽 50 MHz）----
     * 都在 CRH 里：PA11 占 bit[15:12] = 0x4，PA12 占 bit[19:16] = 0xB。
     * 忘了把 PA12 配成复用推挽是新手最常见的"帧发不出去"原因：普通推挽下
     * CAN 外设拿不到引脚控制权，波形根本不出现在引脚上。 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    GPIOA->CRH = (GPIOA->CRH & ~0x000FF000U) | 0x000B4000U;
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    printf("[1] GPIOA 时钟与 CAN1 时钟已开，PA11=浮空输入 PA12=复用推挽\r\n");

    /* ---- 2. 进入初始化模式 ---- */
    printf("[2] 写 CAN1->MCR.INRQ（第一次访问 0x40006400）…\r\n");
    CAN1->MCR |= CAN_MCR_INRQ;
    const uint32_t inak_set = wait_set(&CAN1->MSR, CAN_MSR_INAK);
    printf("    等待 INAK 置位：%s，MSR=0x%08lX\r\n",
           inak_set ? "ok" : "超时（未建模或 CAN 时钟没开）",
           (unsigned long)CAN1->MSR);

    if (inak_set == 0U) {
        printf("    未进入初始化模式，后面的配置都会被硬件忽略，此处停止\r\n");
        for (;;) {
            f103_tick_delay_ms(1000U);
        }
    }

    /* ---- 3. 位时序 + 回环 ---- */
    CAN1->BTR = CAN_BTR_500K_8MHZ;
    printf("[3] BTR 写 0x%08lX，读回 0x%08lX（LBKM=1 即回环；读回不同说明寄存器不可写）\r\n",
           (unsigned long)CAN_BTR_500K_8MHZ, (unsigned long)CAN1->BTR);

    /* ---- 4. 过滤器 ---- */
    can_filter_all_pass();
    printf("[4] 过滤器组 0：32 位掩码模式，掩码=0（全通），挂 FIFO0，已激活\r\n");

    /* ---- 5. 退出初始化 ---- */
    CAN1->MCR &= ~CAN_MCR_INRQ;
    const uint32_t inak_clear = wait_clear(&CAN1->MSR, CAN_MSR_INAK);
    printf("[5] 退出初始化：INAK %s，MSR=0x%08lX\r\n",
           inak_clear ? "已清零" : "仍置位（初始化没有退出）",
           (unsigned long)CAN1->MSR);

    /* ---- 6. 发一帧标准帧（ID 0x123，8 字节）---- */
    static const uint8_t tx_data[CAN_DLC] = {
        0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U
    };

    CAN1->sTxMailBox[0].TDTR = CAN_DLC;                    /* DLC = 8 */
    CAN1->sTxMailBox[0].TDLR = pack_le(&tx_data[0]);        /* byte0..3 */
    CAN1->sTxMailBox[0].TDHR = pack_le(&tx_data[4]);        /* byte4..7 */
    /* IDE=0（标准帧）| STDID<<21 | TXRQ=1（请求发送）。最后写 TIR：
     * TXRQ 一置位硬件就开始发送，所以标识符和数据必须先写完。 */
    CAN1->sTxMailBox[0].TIR = ((uint32_t)CAN_STD_ID << CAN_TI0R_STID_Pos) | CAN_TI0R_TXRQ;

    const uint32_t t0 = f103_tick_now_ms();
    const uint32_t rqcp = wait_set(&CAN1->TSR, CAN_TSR_RQCP0);
    const uint32_t t1 = f103_tick_now_ms();
    const uint32_t txok = (CAN1->TSR & CAN_TSR_TXOK0) ? 1U : 0U;
    printf("[6] 发送：RQCP0 %s，TXOK0=%lu，用时 %lu ms\r\n",
           rqcp ? "完成" : "超时", (unsigned long)txok, (unsigned long)(t1 - t0));
    if (rqcp) {
        CAN1->TSR = CAN_TSR_RQCP0;      /* rc_w1：写 1 清零 */
    }

    /* ---- 7. 回环收帧 ---- */
    const uint32_t fmp = wait_set(&CAN1->RF0R, CAN_RF0R_FMP0);
    printf("[7] 接收：FIFO0 待处理 %s，RF0R=0x%08lX\r\n",
           fmp ? "有报文" : "空（超时）", (unsigned long)CAN1->RF0R);

    if (fmp) {
        const uint32_t rir = CAN1->sFIFOMailBox[0].RIR;
        const uint32_t rx_id  = (rir & CAN_TI0R_STID_Msk) >> CAN_TI0R_STID_Pos;
        const uint32_t rx_dlc = CAN1->sFIFOMailBox[0].RDTR & CAN_TDT0R_DLC_Msk;
        uint8_t rx_data[CAN_DLC];

        unpack_le(CAN1->sFIFOMailBox[0].RDLR, &rx_data[0]);
        unpack_le(CAN1->sFIFOMailBox[0].RDHR, &rx_data[4]);

        uint32_t diff = 0U;
        for (uint32_t i = 0U; i < CAN_DLC; i++) {
            if (rx_data[i] != tx_data[i]) {
                diff++;
            }
        }
        printf("    收到 ID=0x%03lX DLC=%lu，数据与发送%s",
               (unsigned long)rx_id, (unsigned long)rx_dlc,
               (diff == 0U && rx_id == CAN_STD_ID && rx_dlc == CAN_DLC) ? "完全一致" : "不一致");
        printf("：");
        for (uint32_t i = 0U; i < CAN_DLC; i++) {
            printf(" %02X", (unsigned)rx_data[i]);
        }
        printf("\r\n");

        CAN1->RF0R |= CAN_RF0R_RFOM0;   /* 释放 FIFO0 输出邮箱，否则下一帧进不来 */
    }

    printf("=== 结束（QEMU 在步骤 2 就会 HardFault；真板需重跑本节）===\r\n");
    for (;;) {
        f103_tick_delay_ms(1000U);
    }
}
