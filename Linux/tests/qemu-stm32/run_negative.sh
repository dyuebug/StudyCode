#!/usr/bin/env bash
# 反向对照用例：把"不能仿真的实验在 QEMU 里到底是什么表现"记录下来。
#
# 这些用例**预期失败**，不是为了通过，而是为了给 README 的能力边界提供证据：
#   - 01/02/03/06：只操作 GPIO / TIM，寄存器读回恒 0，且不初始化串口，完全无输出
#   - 07：I2C 未建模，只跑到超时分支；CCR/TRISE 读回 0 说明配置根本没生效
#   - 08：SPI 半建模，Flash 读出的 ID 是 00 00 00，擦写"成功"但回读全 0，自检 FAIL
#   - 09：DWT / TIM4 未建模 → 延时循环永不退出，打印到 TIM4 第一档就挂死
#   - 17：ADC1 未建模，ADON/CAL 位读不回 → 校准判据主动判"不可判定"，两路采样走超时分支
#   - 18：CAN1 的 0x40006400 段未映射 → 第一次访问即 BusFault → HardFault（BFAR 定位到基址）
#   - 19：DMA1 未建模，CCR 读回 0 → 三个用例全部落在超时/原值分支
#
# 前置：这些实验已用 -DF103_QEMU=ON 构建到 build-qemu/。

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
EXPERIMENTS="$(cd "$SCRIPT_DIR/../../experiments" && pwd)"
LOG_DIR="$SCRIPT_DIR/logs"
QEMU_BIN="${QEMU_BIN:-qemu-system-arm}"
SECS="${SECS:-4}"

mkdir -p "$LOG_DIR"

cases=(
  "F103_01_GPIO_Blink:f103_01_gpio_blink"
  "F103_02_GPIO_Input:f103_02_gpio_input"
  "F103_03_EXTI_Key:f103_03_exti_key"
  "F103_06_TIM_PWM_RGB:f103_06_tim_pwm_rgb"
  "F103_07_I2C_MPU6050:f103_07_i2c_mpu6050"
  "F103_08_SPI_Flash:f103_08_spi_flash"
  "F103_09_Delay_Precision:f103_09_delay_precision"
  "F103_17_ADC_Polling:f103_17_adc_polling"
  "F103_18_CAN_Loopback:f103_18_can_loopback"
  "F103_19_DMA_Mem2Mem:f103_19_dma_mem2mem"
)

for c in "${cases[@]}"; do
  dir="${c%%:*}"
  elf="${c##*:}"
  echo "=== $dir（预期无输出或挂死，${SECS} s）==="
  # 注意：不能写 ... | tee "$log" | head -12。head 读满 12 行就关闭管道，
  # tee 收到 SIGPIPE 退出，QEMU 的 stdout 随之失效，日志会被截断成 12 行。
  # 正确做法是先完整落盘，再从文件里取前几行做预览。
  timeout "$SECS" "$QEMU_BIN" -M stm32vldiscovery -display none -monitor none \
    -serial stdio -kernel "$EXPERIMENTS/$dir/build-qemu/$elf.elf" </dev/null \
    2>&1 | tr -d '\r' > "$LOG_DIR/neg_${elf}.log"
  head -12 "$LOG_DIR/neg_${elf}.log"
  echo
done
