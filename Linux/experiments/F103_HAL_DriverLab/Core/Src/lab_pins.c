/**
  ******************************************************************************
  * @file    lab_pins.c
  * @brief   补齐驱动需要的"纯 GPIO"引脚
  *
  * 为什么需要这个文件（不是"多写了一个文件"，是补一个真实缺口）：
  *
  *   课程的驱动依赖 CubeMX 配好引脚，且驱动源码里写明了是哪一侧负责：例如
  *     driver_ultrasonic_sr04.c: "Trig PB9, Echo PB8 在MX_GPIO_Init里被初始化为输出/输入引脚了"
  *     driver_spiflash_w25q64.c: "片选信号PB9在MX_GPIO_Init中被配置为输出引脚"
  *     driver_ir_obstacle.c:     "PB13在MX_GPIO_Init里被初始化为输入引脚了"
  *     driver_rotary_encoder.c:  "PB0,PB1 ... 配置为输入引脚" / "PB12 ... 中断引脚,上升沿触发"
  *     driver_ir_receiver.c:     "PA10在MX_GPIO_Init()中已经被配置为双边沿触发, 并使能了中断"
  *                               （原文写作 PA10，但同文件被 #if 0 掉的自配置代码是 PB10，
  *                                 且 driver_irq.c 按 GPIO_PIN_10 分发、USART1_RX 占用 PA10，
  *                                 故 PA10 是注释笔误，实际引脚是 PB10。见 README「已知缺陷」）
  *     driver_dht11.c / driver_ds18b20.c: 要求数据引脚预先配成 "open drain, pull-up"
  *
  *   本工程的外设初始化来自 02_FreeRTOS程序/01_文档配套的源码/01_freertos_template，
  *   它的 gpio.c 只配置了 PC13（已核实：全文只有 GPIOC/GPIO_PIN_13 一处 HAL_GPIO_Init）。
  *   课程里没有任何一份 CubeMX 工程把这 20 个模块的引脚一次配齐——
  *   03_nwatch_dshanmcu-f103_all 是手表产品，只配它用到的引脚，且缺 PB13。
  *
  *   所以：把"驱动要求、CubeMX 没给"的纯 GPIO 集中放在这里，而不是去改 gpio.c。
  *   好处是重新生成 Core/ 不会覆盖，且"哪些引脚是课程生成的、哪些是本工程补的"一目了然。
  *
  * 不在本文件里的引脚（已由别处配置，无需重复）：
  *   PA9/PA10        USART1，见 usart.c:HAL_UART_MspInit
  *   PB6/PB7         I2C1，见 i2c.c:HAL_I2C_MspInit
  *   PA5/PA6/PA7     SPI1，见 spi.c:HAL_SPI_MspInit（NSS 为软件，片选 PB9 见下）
  *   PA8             TIM1_CH1，见 tim.c
  *   PA2/PA15/PB3    TIM2_CH1/2/3，见 tim.c
  *   PB4             TIM3_CH1，见 tim.c
  *   PA3             ADC1_IN3，见 adc.c:HAL_ADC_MspInit
  *   PC13            板载 LED，见 gpio.c
  *   PB14、PA0/PA4/PB15/PB4  驱动自己在 Init 里配置（driver_key.c / driver_motor.c）
  ******************************************************************************
  */

#include "lab_pins.h"
#include "main.h"

/**
  * @brief  配置驱动需要、但 CubeMX 的 gpio.c 没有配置的纯 GPIO
  * @note   在 MX_GPIO_Init() 之后、任何驱动 Test 之前调用一次。
  *         重复配置同一个引脚不会出错，但这里只放 CubeMX 没配的那些，
  *         以便单独核对"谁配了什么"。
  * @retval None
  */
void Lab_Pins_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();

  /* ---- PA1：DHT11 / DS18B20 单总线 ----
     两个模块共用 PA1（这也是二者"不能同时接"的物理原因）。
     开漏 + 内部上拉：写 1 时释放总线由上拉拉高，写 0 时拉低；
     读的时候直接读 IDR。驱动注释要求的就是 "open drain, pull-up"。
     初值置 1（释放总线）：1-Wire 的空闲电平是高。 */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
  GPIO_InitStruct.Pin   = GPIO_PIN_1;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* ---- PB9：HC-SR04 的 TRIG == W25Q64 的 CS（同一个引脚）----
     "M6 与 M12 可以同时接，但不能同时访问"的物理原因就在这一行：
     两个模块抢同一根线，而它们要求的空闲电平相反——
       W25Q64 片选低有效，空闲应为高；
       HC-SR04 TRIG 需要 ≥10 µs 的高脉冲触发，空闲应为低。
     取低，是为了让 SR04 可用；flash 在 CS 低、SCLK 静止时只是被选中而收不到任何位，
     不会误执行命令。这条是源码推论，未在真板用逻辑分析仪确认。
     初值取低，与课程 03_nwatch 工程的 gpio.c 一致。 */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin   = GPIO_PIN_9;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* ---- PB8：HC-SR04 的 ECHO 输入 ----
     由模块推挽驱动，不需要上下拉（加上拉会让悬空时被读成高）。 */
  GPIO_InitStruct.Pin  = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* ---- PB13：红外避障 LM393 输出 ----
     LM393 是开集输出，板上若无外部上拉，不加上拉就会读到悬空值。
     课程注释只说"输入引脚"，未给上拉信息；这里加上拉属于工程推断，真板待验。 */
  GPIO_InitStruct.Pin  = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* ---- PB0 / PB1：EC11 的 S1 / S2 ----
     机械触点对地闭合，必须上拉才有确定的空闲高电平。 */
  GPIO_InitStruct.Pin  = GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* ---- PB10：红外接收 1838，双边沿外部中断 ----
     1838 输出为开集，需上拉。
     注意：EXTI 线号只有 10 这一个，具体映射到哪个端口由 AFIO_EXTICR 决定。
     调用 HAL_GPIO_Init(GPIOB, PIN_10) 会把 EXTI10 映射到端口 B；
     若不配这一步，EXTI10 默认落在端口 A，而 PA10 正是 USART1_RX——
     串口每收一个字节都会误触发一次"红外中断"。这一点是本工程实测过的配置必要性，
     但"误触发"本身未在真板上抓过波形。 */
  GPIO_InitStruct.Pin  = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* ---- PB12：EC11 的按键，上升沿中断 ----
     课程 03_nwatch 的 gpio.c 用的是 NOPULL（推断板上另有外部上拉）；
     这里改用内部上拉：有外部上拉时并联无害，没有时它是唯一保证。
     该改动未在真板验证，属本工程与课程配置的已知差异。 */
  GPIO_InitStruct.Pin  = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* ---- EXTI 中断使能 ----
     stm32f1xx_it.c 里有 EXTI15_10_IRQHandler，但 NVIC 不使能就永远不会被调用。
     这段在 CubeMX 正常生成时应位于 gpio.c 的 "EXTI interrupt init" 注释块下，
     模板的 gpio.c 没有该块，故在此补齐。
     优先级取 0：本工程是裸机、没有 RTOS，不涉及 FreeRTOS 的
     configMAX_SYSCALL_INTERRUPT_PRIORITY 门槛问题。 */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
