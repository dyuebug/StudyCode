/**
  ******************************************************************************
  * @file    main.c
  * @brief   DShanMCU-103 官方驱动库 —— GCC/CMake 构建入口（裸机版）
  *
  * 与课程原始工程的关系：
  *   - 外设初始化（MX_xxx_Init）来自 STM32CubeMX 生成的 Core/Src，
  *     一字未改，可直接用 CubeMX 重新生成后覆盖。
  *   - 本文件负责启动顺序与测试入口。课程原版把测试调用放在
  *     Core/Src/freertos.c 的 StartDefaultTask 里；本工程是裸机版，
  *     等价地放在 main() 里。
  *   - 驱动（Drivers/DshanMCU-F103）与课程原版完全相同。
  *
  * 切换测试模块：改 LAB_TEST 的值，或用 -DLAB_TEST=<编号> 配置。
  * 编号见下方 Lab_Run() 的 #if 分支；0 是只用 PC13 的 LED 自测（不需要外接模块）。
  *
  * 平台：STM32F103C8T6 / DShanMCU-103，HSE 8 MHz → PLL×9 = 72 MHz
  ******************************************************************************
  */

#include "main.h"
#include "gpio.h"
#include "usart.h"
#include "i2c.h"
#include "spi.h"
#include "adc.h"
#include "tim.h"
#include "lab_pins.h"

#include <stdio.h>

/* 驱动头文件：每个模块一个，只提供 Test 函数与业务接口的原型 */
#include "driver_led.h"
#include "driver_timer.h"
#include "driver_key.h"
#include "driver_uart.h"
#include "driver_color_led.h"
#include "driver_active_buzzer.h"
#include "driver_passive_buzzer.h"
#include "driver_ir_sender.h"
#include "driver_ir_receiver.h"
#include "driver_ir_obstacle.h"
#include "driver_light_sensor.h"
#include "driver_motor.h"
#include "driver_rotary_encoder.h"
#include "driver_oled.h"
#include "driver_lcd.h"
#include "driver_ds18b20.h"
#include "driver_dht11.h"
#include "driver_ultrasonic_sr04.h"
#include "driver_spiflash_w25q64.h"
#include "driver_mpu6050.h"

/* 本工程自加（非课程驱动）：16x16 汉字显示，见 notes/F103_HAL入门/Day10 */
#include "lab_oled_hanzi.h"

#if !defined(LAB_TEST)
#define LAB_TEST 0
#endif

void SystemClock_Config(void);
static void Lab_Run(void);

/**
  * @brief  主函数
  * @retval 不会返回（被选中的 Test 都是课程驱动里的死循环）
  */
int main(void)
{
  /* 复位所有外设并初始化 Flash 接口；同时按 stm32f1xx_hal_timebase_tim.c
     把 HAL 时基重定向到 TIM4（1 MHz 计数 / 1 ms 更新），为后续接管
     SysTick 留出余地。这一点与课程工程一致。 */
  HAL_Init();

  /* HSE 8 MHz → PLL×9 = 72 MHz，APB1 = 36 MHz，APB2 = 72 MHz */
  SystemClock_Config();

  /* ---- 外设初始化 ----
     与 CubeMX 生成的 main() 一致：进去就全部初始化，而不是用哪个才初始化哪个。
     驱动只调用外设的收发函数，初始化全部落在这里（见课程第 5 章）。
     注意引脚冲突：M2(DS18B20) 与 M10(DHT11) 不能同时插，M4(红外发射) 与
     M9(无源蜂鸣器) 共用 PA8/TIM1_CH1，M6(超声波) 与 M12(Flash) 同时插上也无法同时访问。 */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();

  /* ---- 驱动要求、CubeMX 没给的纯 GPIO ----
     模板的 gpio.c 只配了 PC13，而驱动注释明确要求 MX_GPIO_Init 配好
     PB9/PB8/PB13/PB0/PB1/PB12/PB10/PA1。这里补齐，理由见 lab_pins.c 文件头。 */
  Lab_Pins_Init();

  printf("\r\n[hal-lab] LAB_TEST=%d build=%s %s\r\n", LAB_TEST, __DATE__, __TIME__);

  Lab_Run();

  /* 所有 Test 都是死循环；走到这里说明 LAB_TEST 不在支持范围内 */
  printf("[hal-lab] LAB_TEST=%d 无对应模块，停在空循环\r\n", LAB_TEST);
  while (1)
  {
  }
}

/**
  * @brief  按编号分发到课程驱动的自测函数（编译期选择）
  * @note   必须用预处理而不是运行期 switch：实测运行期 switch 会把 19 个 Test
  *         全部当作可能调用点保留，镜像从 10.9 KB 涨到 33.9 KB。
  *         这里用 #if 之后，只有选中的那个 Test 会进镜像，其余由
  *         --gc-sections 回收（可用 arm-none-eabi-nm 核对符号表）。
  *         模块编号只采用课程第 4 章冲突表里出现过的 M 号；表中没有的只写名称。
  */
static void Lab_Run(void)
{
#if   LAB_TEST == 0
  Led_Test();            /* 板载单色 LED（PC13），无需外接模块 */
#elif LAB_TEST == 1
  Key_Test();            /* 按键 K1 */
#elif LAB_TEST == 2
  UART_Test();           /* 串口（USART1，PA9/PA10） */
#elif LAB_TEST == 3
  ColorLED_Test();       /* RGB 全彩 LED（PA15/PB3/PA2, TIM2） */
#elif LAB_TEST == 4
  ActiveBuzzer_Test();   /* 有源蜂鸣器（PA8 推挽） */
#elif LAB_TEST == 5
  PassiveBuzzer_Test();  /* 无源蜂鸣器（PA8, TIM1_CH1；与红外发射冲突） */
#elif LAB_TEST == 6
  IRSender_Test();       /* 红外发射 38 kHz（PA8, TIM1_CH1；与无源蜂鸣器冲突） */
#elif LAB_TEST == 7
  IRReceiver_Test();     /* 红外接收 1838（EXTI） */
#elif LAB_TEST == 8
  IRObstacle_Test();     /* 红外避障 LM393 */
#elif LAB_TEST == 9
  LightSensor_Test();    /* 光敏电阻（ADC1） */
#elif LAB_TEST == 10
  Motor_Test();          /* 直流电机 DRV8833（PB4, TIM3_CH1） */
#elif LAB_TEST == 11
  RotaryEncoder_Test();  /* 旋转编码器 EC11（EXTI） */
#elif LAB_TEST == 12
  OLED_Test();           /* I2C OLED（SSD1306, I2C1） */
#elif LAB_TEST == 13
  LCD_Test();            /* LCD 驱动是 OLED 的别名封装，与 12 同效 */
#elif LAB_TEST == 14
  DS18B20_Test();        /* DS18B20（M2，单总线；与 M10 DHT11 冲突） */
#elif LAB_TEST == 15
  DHT11_Test();          /* DHT11（M10，单总线；与 M2 DS18B20 冲突） */
#elif LAB_TEST == 16
  SR04_Test();           /* HC-SR04（M6；与 M12 Flash 可同时接、不能同时访问） */
#elif LAB_TEST == 17
  W25Q64_Test();         /* SPI Flash（M12；与 M6 超声波同上） */
#elif LAB_TEST == 18
  MPU6050_Test();        /* 六轴陀螺仪加速度计（I2C1） */
#elif LAB_TEST == 19
  OLED_Hanzi_Test();     /* SSD1306 显示 16x16 汉字（教材 8.5，自加模块） */
#endif
}

/**
  * @brief  System clock configuration
  * @note   由 CubeMX 生成，与课程 01_freertos_template 的 main.c 逐行一致。
  *         参数依据：《FreeRTOS入门与工程实践》第6章 6.2 配置时钟。
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  /* 72 MHz 下 Flash 需 2 个等待周期，少配会取指错乱 */
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @note   关中断后原地停住。调试时在此打断点即可看到出错点；
  *         量产代码应在此记录复位原因。
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
