/**
 * F103_17：ADC1 轮询采样（内部温度传感器 + Vrefint）
 *
 * 为什么选这两个通道：
 *   它们在本片内部，**不需要外接任何东西**，所以"能采到数"这件事只取决于
 *   寄存器序列是否正确，不取决于手上有没有传感器。外部通道（PA0..PA7）留作练习。
 *
 * 时钟链（不配 PLL，全部取复位值）：
 *   HCLK = PCLK2 = 8 MHz（HSI）
 *   RCC_CFGR.ADCPRE = 00  → ADCCLK = PCLK2 / 2 = 4 MHz（上限 14 MHz）
 *   采样时间 239.5 周期 → (239.5 + 12.5) / 4 MHz ≈ 63 µs/次
 *   温度传感器要求最小采样时间 17.1 µs，239.5 周期是最宽的一档，留足余量。
 *
 * 顺序（本实现的实际顺序，真板需再核一遍 RM0008 的措辞）：
 *   1. 开 ADC1 时钟（APB2），再置 ADON 上电
 *   2. RSTCAL 复位校准寄存器 → 等硬件清零
 *   3. CAL 启动校准 → 等硬件清零（不自校准就采，读数会整体偏移）
 *   4. 置 TSVREFE，给内部温度传感器与 Vrefint 供电
 *   5. 逐个通道：写 SQR3 → SWSTART → 轮询 EOC → 读 DR
 *
 * 所有等待都带迭代上限：通道没接、时钟没开、或者干脆跑在未建模的仿真器上时，
 * EOC 永远不会来；不设上限就是挂死，日志停在半截也看不出为什么。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_tick.h"
#include "f103_uart.h"

#define UART_BAUD       115200U
#define POLL_LIMIT      200000U   /* 轮询上限，非时间单位 */

#define ADC_CH_TEMP     16U       /* 内部温度传感器 */
#define ADC_CH_VREFINT  17U       /* 内部参考电压 */

/* 以下三个常数是数据手册的典型值，不是本板实测值。
 * 要得到可用的绝对温度，必须用出厂标定值做两点校准；见 read_factory_cal()。 */
#define VREFINT_TYP_MV  1200L
#define V25_MV          1430L
#define AVG_SLOPE_UV    4300L

/* 轮询等待某位被硬件清零，返回 1 表示等到、0 表示超时 */
static uint32_t wait_clear(volatile uint32_t *reg, uint32_t mask)
{
    for (uint32_t i = 0U; i < POLL_LIMIT; i++) {
        if ((*reg & mask) == 0U) {
            return 1U;
        }
    }
    return 0U;
}

/* 复位校准 + 启动校准。
 *
 * 这里刻意分两步判：先看"写进去的位读回还在不在"，再看"硬件有没有把它清掉"。
 * 只看第二步会得到一个假结论——未建模的 ADC 恒读 0，"等待清零"立刻成功，
 * 看起来像校准完成，其实是这个位压根没有被置起来过。 */
static void adc_calibrate(void)
{
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    const uint32_t rstcal_set = (ADC1->CR2 & ADC_CR2_RSTCAL) ? 1U : 0U;
    const uint32_t rstcal_done = wait_clear(&ADC1->CR2, ADC_CR2_RSTCAL);
    printf("RSTCAL：写入后读回 %s，清零 %s\r\n",
           rstcal_set ? "置位" : "仍为 0（该位没有状态）",
           rstcal_done ? "ok" : "超时");

    ADC1->CR2 |= ADC_CR2_CAL;
    const uint32_t cal_set = (ADC1->CR2 & ADC_CR2_CAL) ? 1U : 0U;
    const uint32_t cal_done = wait_clear(&ADC1->CR2, ADC_CR2_CAL);
    printf("CAL   ：写入后读回 %s，清零 %s\r\n",
           cal_set ? "置位" : "仍为 0（该位没有状态）",
           cal_done ? "ok" : "超时");

    if (!rstcal_set || !cal_set) {
        printf("校准流程不可判定：CR2 的可写状态位读不回，后面的 code 不能当校准后的读数\r\n");
    }
}

static uint32_t adc_convert(uint32_t ch, uint32_t *code)
{
    ADC1->SQR3 = ch & 0x1FU;          /* 序列只有 1 个转换，SQ1 就是通道号 */

    /* SWSTART 是"写 1 触发"的自清位：不需要也不能在之后手工清 */
    ADC1->CR2 |= ADC_CR2_SWSTART;

    for (uint32_t i = 0U; i < POLL_LIMIT; i++) {
        if ((ADC1->SR & ADC_SR_EOC) != 0U) {
            *code = ADC1->DR & 0x0FFFU;   /* 读 DR 同时清 EOC */
            return 1U;
        }
    }
    *code = 0U;
    return 0U;
}

/* 出厂标定值：F103 中容量器件放在系统存储区，地址取自数据手册。
 * 读到 0x0000 / 0xFFFF 说明地址或器件型号对不上——这种情况下不能算绝对温度。 */
static void read_factory_cal(void)
{
    const uint16_t ts_cal1 = *(const volatile uint16_t *)0x1FFFF7B8U;
    const uint16_t ts_cal2 = *(const volatile uint16_t *)0x1FFFF7C2U;
    const uint16_t vref_cal = *(const volatile uint16_t *)0x1FFFF7BAU;

    printf("出厂标定：TS_CAL1=%u TS_CAL2=%u VREFINT_CAL=%u%s\r\n",
           (unsigned)ts_cal1, (unsigned)ts_cal2, (unsigned)vref_cal,
           (ts_cal1 == 0U || ts_cal1 == 0xFFFFU) ? "（无效，无法算绝对温度）" : "");
}

int main(void)
{
    f103_tick_init();                  /* 顺便把 SystemCoreClock 刷成真实值 */
    f103_uart_init(UART_BAUD);

    printf("\r\n=== F103_17 ADC1 轮询采样 ===\r\n");

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    ADC1->CR2 |= ADC_CR2_ADON;         /* 上电（首次置位是从掉电状态唤醒） */
    printf("ADON 读回：%s\r\n",
           (ADC1->CR2 & ADC_CR2_ADON) ? "置位" : "仍为 0（ADC 没上电，或本机未建模）");

    /* 序列长度 1（SQR1.L = 0）；采样时间两路都取 239.5 周期 */
    ADC1->SQR1 = 0U;
    ADC1->SMPR1 = (0x7UL << 18) | (0x7UL << 21);   /* SMP16 / SMP17 = 111 */

    adc_calibrate();
    ADC1->CR2 |= ADC_CR2_TSVREFE;      /* 内部通道必须先供电，否则读到的是 0 */

    uint32_t temp_code = 0U;
    uint32_t vref_code = 0U;
    const uint32_t temp_ok = adc_convert(ADC_CH_TEMP, &temp_code);
    const uint32_t vref_ok = adc_convert(ADC_CH_VREFINT, &vref_code);

    printf("温度通道 ch%u：%s code=%lu\r\n", (unsigned)ADC_CH_TEMP,
           temp_ok ? "ok" : "超时（EOC 未置位）", (unsigned long)temp_code);
    printf("参考通道 ch%u：%s code=%lu\r\n", (unsigned)ADC_CH_VREFINT,
           vref_ok ? "ok" : "超时（EOC 未置位）", (unsigned long)vref_code);

    if (vref_ok && vref_code != 0U) {
        /* Vdda = Vrefint_typ × 4095 / code。这只是估算：Vrefint 本身有 ±3% 离散 */
        const uint32_t vdda_mv = (uint32_t)(VREFINT_TYP_MV * 4095L / (int32_t)vref_code);
        printf("估算 Vdda ≈ %lu mV（用 1.20 V 典型值反推，未做单板校准）\r\n",
               (unsigned long)vdda_mv);

        if (temp_ok) {
            /* T = 25 + (V25 - Vsense) / Avg_Slope，两者都按毫伏算 */
            const int32_t vsense_mv = (int32_t)((uint32_t)vdda_mv * temp_code / 4095U);
            const int32_t t_c_x10 = 250 + ((V25_MV - vsense_mv) * 10000) / AVG_SLOPE_UV;
            printf("估算温度 ≈ %ld.%ld ℃（典型参数，误差可达数摄氏度）\r\n",
                   (long)(t_c_x10 / 10), (long)(t_c_x10 % 10));
        }
    } else {
        printf("Vrefint 无有效读数：本机无法反推 Vdda，也就无法把码值换成电压\r\n");
    }

    read_factory_cal();

    printf("=== 结束（QEMU 未建模 ADC 时会整段停在“超时”）===\r\n");
    for (;;) {
        f103_tick_delay_ms(1000U);
    }
}
