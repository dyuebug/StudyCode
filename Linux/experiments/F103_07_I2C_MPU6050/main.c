/**
 * F103_07：I2C1 主机轮询读写 MPU6050
 *
 * 硬件连接（DShanMCU-103 + 外接 MPU6050 模块，课程 [08]_I2C/0801_i2c_hal_poll）：
 *   PB6 = I2C1_SCL，PB7 = I2C1_SDA，均为复用开漏，模块自带上拉。
 *   注意：原理图中 I2C1 只引到 J2/J4 排针（网络标号 PIJ205/206、PIJ405/406），
 *   开发板上没有焊接传感器，必须外接模块才能读到数据。
 *
 * 时钟：I2C1 挂 APB1。本实验未配 PLL，PPRE1 = 1（复位值），
 *       PCLK1 = SystemCoreClock = 8 MHz。
 *       CR2.FREQ = 8（单位 MHz，是给内部时序发生器用的参考频率，不是分频值）
 *       CCR      = PCLK1 / (2 * 100 kHz) = 40   → SCL = 100 kHz
 *       TRISE    = (FREQ) + 1 = 9               → 标准模式最大上升时间 1000 ns
 *
 * 单字节读的时序顺序是本实验的核心：先清 ACK，再清 ADDR（读 SR1、SR2），
 * 紧接着置 STOP，最后读 DR。三段顺序与间隔都有约束，见 i2c1_read_reg() 内注释。
 */
#include <stdint.h>
#include <stdio.h>

#include "stm32f1xx.h"
#include "f103_tick.h"
#include "f103_uart.h"

#define UART_BAUD        115200U
#define I2C_SPEED_HZ     100000U
#define I2C_TIMEOUT_MS   10U
#define POLL_PERIOD_MS   1000U

#define MPU6050_ADDR      0x68U   /* AD0 接地时的 7 位地址 */
#define MPU_REG_WHO_AM_I  0x75U
#define MPU_REG_PWR_MGMT1 0x6BU
#define MPU_REG_ACCEL_XH  0x3BU   /* 0x3B..0x40 = AX_H, AX_L, AY_H, AY_L, AZ_H, AZ_L */
#define MPU_WHO_AM_I_VAL  0x68U

/* ±2g 量程下 1 g = 16384 LSB（PWR_MGMT_1 复位后 AFS_SEL = 00） */
#define MPU_LSB_PER_G     16384L

enum {
    I2C_OK = 0,
    I2C_ERR_TIMEOUT = -1,
    I2C_ERR_NACK = -2,
};

/* ---- 硬件初始化 --------------------------------------------------------- */

static void i2c1_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    /*
     * PB6 在 CRL 的 bit[27:24]，PB7 在 bit[31:28]。
     * 复用开漏 50 MHz = CNF 11 + MODE 11 = 0xF。
     * 开漏是 I2C 的硬性要求：总线上任何设备都可能拉低，推挽会短路。
     */
    GPIOB->CRL = (GPIOB->CRL & ~((0xFU << 24) | (0xFU << 28)))
               | (0xFU << 24) | (0xFU << 28);
}

static void i2c1_hw_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* 软件复位：把可能残留在 SR1/SR2 里的错误状态一并清掉，代价是 PE 也归零 */
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    /* 前提：PPRE1 = 1，PCLK1 = SystemCoreClock。改时钟树时必须按 RCC->CFGR 重算 */
    uint32_t pclk1_mhz = SystemCoreClock / 1000000U;

    I2C1->CR2   = (pclk1_mhz & I2C_CR2_FREQ);
    I2C1->CCR   = SystemCoreClock / (2U * I2C_SPEED_HZ);  /* 8e6 / 2e5 = 40 */
    I2C1->TRISE = pclk1_mhz + 1U;                          /* 标准模式：1000 ns */
    I2C1->CR1  |= I2C_CR1_PE;
}

/* ---- 传输原语 ----------------------------------------------------------- */

/* 等一个 SR1 条件成立；同时把总线错误和超时当作失败，避免裸机死等 */
static int i2c1_wait_sr1(uint32_t mask)
{
    uint32_t start = f103_tick_now_ms();

    for (;;) {
        uint32_t sr1 = I2C1->SR1;
        if ((sr1 & mask) != 0U) {
            return I2C_OK;
        }
        if ((sr1 & (I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO)) != 0U) {
            return I2C_ERR_NACK;
        }
        if ((uint32_t)(f103_tick_now_ms() - start) > I2C_TIMEOUT_MS) {
            return I2C_ERR_TIMEOUT;
        }
    }
}

static int i2c1_wait_idle(void)
{
    uint32_t start = f103_tick_now_ms();

    while ((I2C1->SR2 & I2C_SR2_BUSY) != 0U) {
        if ((uint32_t)(f103_tick_now_ms() - start) > I2C_TIMEOUT_MS) {
            return I2C_ERR_TIMEOUT;
        }
    }
    return I2C_OK;
}

/* START + 地址字节，成功后 ADDR 已置位；调用方负责清 ADDR */
static int i2c1_start(uint8_t addr_rw)
{
    I2C1->CR1 |= I2C_CR1_START;

    int rc = i2c1_wait_sr1(I2C_SR1_SB);
    if (rc != I2C_OK) {
        return rc;
    }
    I2C1->DR = addr_rw;
    return i2c1_wait_sr1(I2C_SR1_ADDR);
}

/*
 * 出错后的恢复：先尝试 STOP 释放总线，再整体复位。
 * 若从机把 SDA 钳在低电平，STOP 发不出去，此时 SWRST 也救不回物理总线，
 * 真实板子上还需要手动打 9 个时钟——这里不实现，只在 README 中注明。
 */
static void i2c1_recover(void)
{
    I2C1->CR1 |= I2C_CR1_STOP;
    i2c1_hw_init();
}

static int i2c1_write_reg(uint8_t dev, uint8_t reg, uint8_t val)
{
    int rc = i2c1_wait_idle();
    if (rc != I2C_OK) {
        i2c1_recover();
        return rc;
    }

    rc = i2c1_start((uint8_t)(dev << 1));
    if (rc != I2C_OK) {
        i2c1_recover();
        return rc;
    }
    (void)I2C1->SR1;            /* 清 ADDR：先读 SR1 */
    (void)I2C1->SR2;            /*           再读 SR2 */

    I2C1->DR = reg;
    rc = i2c1_wait_sr1(I2C_SR1_TXE);
    if (rc == I2C_OK) {
        I2C1->DR = val;
        /* BTF：数据已从移位寄存器发完，此时置 STOP 不会截断最后一位 */
        rc = i2c1_wait_sr1(I2C_SR1_BTF);
    }

    if (rc != I2C_OK) {
        i2c1_recover();
        return rc;
    }
    I2C1->CR1 |= I2C_CR1_STOP;
    return I2C_OK;
}

static int i2c1_read_reg(uint8_t dev, uint8_t reg, uint8_t *out)
{
    int rc = i2c1_wait_idle();
    if (rc != I2C_OK) {
        i2c1_recover();
        return rc;
    }

    /* 阶段一：写方向，发送寄存器地址并停在重复起始前 */
    rc = i2c1_start((uint8_t)(dev << 1));
    if (rc != I2C_OK) {
        i2c1_recover();
        return rc;
    }
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    I2C1->DR = reg;
    rc = i2c1_wait_sr1(I2C_SR1_TXE);
    if (rc == I2C_OK) {
        rc = i2c1_wait_sr1(I2C_SR1_BTF);
    }
    if (rc != I2C_OK) {
        i2c1_recover();
        return rc;
    }

    /* 阶段二：重复起始，读方向 */
    rc = i2c1_start((uint8_t)((dev << 1) | 1U));
    if (rc != I2C_OK) {
        i2c1_recover();
        return rc;
    }

    /*
     * 单字节接收（参考手册里称 EV6_3）：
     *   1. 清 ACK —— 本字节之后回 NACK，告诉从机不要再发
     *   2. 读 SR1、再读 SR2 —— 清 ADDR，接收立即开始
     *   3. 置 STOP —— 本字节结束后释放总线
     *   4. 等 RXNE，读 DR
     * ACK 必须先于 ADDR 清：迟一步，从机就会在收到 ACK 后继续发下一个字节。
     * ADDR 清零到 STOP 置位之间存在一个窗口，ST 勘误表（ES096，I2C 的 EV6_3 事件）指出：
     * 这组操作若没能在当前字节结束前完成，可能多收一个字节、重复读到同一数据或丢数据。
     * HAL 的做法就是在这里关中断，本实验照做。
     */
    I2C1->CR1 &= ~I2C_CR1_ACK;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    I2C1->CR1 |= I2C_CR1_STOP;
    if (primask == 0U) {
        __enable_irq();
    }

    rc = i2c1_wait_sr1(I2C_SR1_RXNE);
    if (rc != I2C_OK) {
        i2c1_recover();
        return rc;
    }
    *out = (uint8_t)I2C1->DR;
    return I2C_OK;
}

/*
 * 读 6 个加速度寄存器。这里用 6 次单字节读拼装，而不是一次突发读：
 * 突发读要为倒数第二个字节保留 ACK、最后一个字节回 NACK，时序分支更多，
 * 与本节要讲的「单字节读顺序」不是同一个知识点。突发读留作练习。
 */
static int mpu_read_accel(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t raw[6];
    for (uint32_t i = 0; i < sizeof(raw); i++) {
        int rc = i2c1_read_reg(MPU6050_ADDR, (uint8_t)(MPU_REG_ACCEL_XH + i), &raw[i]);
        if (rc != I2C_OK) {
            return rc;
        }
    }
    *ax = (int16_t)((raw[0] << 8) | raw[1]);
    *ay = (int16_t)((raw[2] << 8) | raw[3]);
    *az = (int16_t)((raw[4] << 8) | raw[5]);
    return I2C_OK;
}

static const char *rc_name(int rc)
{
    switch (rc) {
    case I2C_OK:          return "ok";
    case I2C_ERR_TIMEOUT: return "timeout";
    case I2C_ERR_NACK:    return "nack";
    default:              return "unknown";
    }
}

int main(void)
{
    f103_tick_init_irq();
    f103_uart_init(UART_BAUD);
    i2c1_gpio_init();
    i2c1_hw_init();

    printf("\r\n[F103_07] I2C1 master @ %lu Hz, PCLK1 = %lu Hz\r\n",
           (unsigned long)I2C_SPEED_HZ, (unsigned long)SystemCoreClock);
    printf("CCR = %lu, TRISE = %lu\r\n",
           (unsigned long)I2C1->CCR, (unsigned long)I2C1->TRISE);

    uint8_t who = 0U;
    int rc = i2c1_read_reg(MPU6050_ADDR, MPU_REG_WHO_AM_I, &who);
    if (rc == I2C_OK) {
        printf("WHO_AM_I = 0x%02X (%s)\r\n", who,
               (who == MPU_WHO_AM_I_VAL) ? "MPU6050 在线" : "值异常，检查型号");
    } else {
        printf("WHO_AM_I 读取失败：%s。检查 J2/J4 是否接了模块、上拉与供电。\r\n",
               rc_name(rc));
    }

    if (rc == I2C_OK) {
        /* 复位后处于 sleep 模式，必须先写 PWR_MGMT_1 唤醒，加速度计才有数据 */
        uint8_t pwr = 0xFFU;
        rc = i2c1_write_reg(MPU6050_ADDR, MPU_REG_PWR_MGMT1, 0x00U);
        if (rc == I2C_OK) {
            rc = i2c1_read_reg(MPU6050_ADDR, MPU_REG_PWR_MGMT1, &pwr);
        }
        if (rc == I2C_OK) {
            printf("PWR_MGMT_1 回读 = 0x%02X (%s)\r\n", pwr,
                   (pwr == 0x00U) ? "已唤醒" : "写入未生效");
        } else {
            printf("PWR_MGMT_1 唤醒失败：%s\r\n", rc_name(rc));
        }
    }

    uint32_t next_poll = POLL_PERIOD_MS;

    for (;;) {
        uint32_t now = f103_tick_now_ms();
        if ((int32_t)(now - next_poll) >= 0) {
            next_poll += POLL_PERIOD_MS;

            int16_t ax = 0, ay = 0, az = 0;
            rc = mpu_read_accel(&ax, &ay, &az);
            if (rc == I2C_OK) {
                printf("[%lu ms] accel raw = %6d %6d %6d  (%ld mg, %ld mg, %ld mg)\r\n",
                       (unsigned long)now, ax, ay, az,
                       (long)((int32_t)ax * 1000L / MPU_LSB_PER_G),
                       (long)((int32_t)ay * 1000L / MPU_LSB_PER_G),
                       (long)((int32_t)az * 1000L / MPU_LSB_PER_G));
            } else {
                printf("[%lu ms] accel 读取失败：%s\r\n",
                       (unsigned long)now, rc_name(rc));
            }
        }
    }
}
