# QEMU 验证结果

**日期**：2026-10-25
**环境**：QEMU 10.2.1 `vexpress-a9`，Linux 7.0.0（Ubuntu `linux-source-7.0.0`）
**工具链**：arm-linux-gnueabihf-gcc 15.2.0
**载荷**：initramfs 921 KB，含静态 init 与 11 个外部模块

## 内核配置

基础配置 `vexpress_defconfig`，另需：

```bash
./scripts/config --enable ARCH_VIRT              # 带出 ARM_AMBA
./scripts/config --enable GPIO_GENERIC_PLATFORM  # basic-mmio-gpio
./scripts/config --enable SPI                    # Day14 编译检查需要；QEMU 运行不需要
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- olddefconfig
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- zImage modules
```

> 注意 1：`olddefconfig` 必须带 `ARCH=arm`，否则配置会被重算成主机 x86，导致后续模块结构体布局错误。
> 注意 2：`CONFIG_SPI` 在现代内核中是 `bool` 而非 `tristate`，用 `--module SPI` 会被忽略。

## 命令

```bash
make initramfs
make dtb
bash run.sh
```

## 实测输出（首次验证，Day2–Day7）

```text
===== qemu-vexpress verification =====
[ OK ] loaded /hello_module.ko
hello_module: init
[ OK ] loaded /hello_chrdev.ko
[ OK ] mknod /dev/hello_chrdev major=248
[ OK ] Day3 open /dev/hello_chrdev
[ OK ] Day3 write returns 1 (callback stub)
[ OK ] Day3 read returns 1 (no data copied)
[ OK ] loaded /hello_copy.ko
[ OK ] mknod /dev/hello_copy major=247
[ OK ] Day4 open /dev/hello_copy
[ OK ] Day4 read initial value = 123
[ OK ] Day4 write 456
[ OK ] Day4 read back 456
[ OK ] Day4 short read returns 1 (no overflow)
[ OK ] loaded /hello_platform_dev.ko
[ OK ] loaded /hello_platform_drv.ko
[ OK ] Day6 platform driver registered
[ OK ] loaded /led_platform_dev.ko
[ OK ] loaded /led_platform_drv.ko
[ OK ] Day7 /dev/led0 created
[ OK ] Day7 /dev/led1 created
[ OK ] Day7 write to /dev/led0
hello_platform hello_platform: probe matched: hello_platform
led_multi led_multi.0: probe: minor=0 pin=3
led_multi led_multi.1: probe: minor=1 pin=5
led_multi: minor=0 pin=3 value=1
===== result: PASSED (failures=0) =====
```

## 实测输出（扩展验证，Day8–Day9）

自定义设备树 `100ask-vexpress.dts`：

- 禁用 `gpio-leds`，释放 `v2m_led_gpios`
- 追加 `myled_for_test`（`compatible = "100ask,led"`）
- 追加 `myled_gpio`（`compatible = "100ask,led-gpio"`，`led-gpios = <&v2m_led_gpios 0 GPIO_ACTIVE_HIGH>`）

```text
[ OK ] loaded /led_dts_drv.ko
[ OK ] Day8 DT node present (myled_for_test)
[ OK ] Day8 driver registered
[ OK ] loaded /led_gpio_drv.ko
[ OK ] Day9 /dev/myled_gpio created
[ OK ] Day9 set logical 1
[ OK ] Day9 read back 1
[ OK ] Day9 set logical 0
[ OK ] Day9 read back 0
===== result: PASSED (failures=0) =====

led_dts myled_for_test: probe: pin=3
```

## 实测输出（Day5，交叉编译与模块生命周期）

```text
[ OK ] Day5 cross-built module loaded, /dev/myled created
100ask_led: init, major = 244, /dev/myled ready
100ask_led: exit
[ OK ] unload led_drv (remove path)
```

> 限制：板级寄存器（i.MX6ULL GPIO5 / IOMUXC）在 vexpress 上无对应硬件，未访问。
> 本验证覆盖交叉编译、架构校验、加载、设备节点创建与卸载路径。

## 实测输出（Day10–Day11，定时器事件替代硬件中断）

```text
[ OK ] loaded /event_demo.ko
[ OK ] Day10 /dev/qemu_event created
[ OK ] Day10 ioctl PAUSE
[ OK ] Day10 ioctl CLEAR
[ OK ] Day10 open nonblocking
[ OK ] Day10 nonblocking read returns EAGAIN
[ OK ] Day10 ioctl RESUME
[ OK ] Day10 poll reports POLLIN
[ OK ] Day10 blocking read returns event
event_demo: tasklet ran, count=1
[ OK ] Day11 unload triggers remove path
===== result: PASSED (failures=0) =====
event_demo: exit, resources released
```

> 限制：QEMU vexpress 没有可注入的中断源，`request_irq` / `request_threaded_irq`
> 的真实投递未验证。上面的验证覆盖等待队列、`poll`、`ioctl`、timer、tasklet、
> workqueue 与卸载清理顺序。

## 编译验证（Day10–Day14，不入 QEMU）

QEMU 没有 SR501、AT24C02、SPI OLED，也没有可注入的中断源，因此这 5 天的驱动
只做交叉编译验证（2026-10-25，同一 ARM 内核树）：

| 实验 | 模块 | 测试程序 | 结果 |
| --- | --- | --- | --- |
| Day10 | `gpio_key_irq.ko` | `button_test` | ARM ELF，通过 |
| Day11 | `key_ctx.ko` | `key_ctx_test` | ARM ELF，通过 |
| Day12 | `sr501_drv.ko` | `sr501_test` | ARM ELF，通过 |
| Day13 | `at24c02_drv.ko` | `at24c02_test` | ARM ELF，通过 |
| Day14 | `oled_fb.ko` | `oled_test` | ARM ELF，通过 |

> Day10/11 的**运行期**验证用的是替代模块 `module/event_demo.c`，不是这两天的驱动代码。
> 这 5 天此前从未编译过，首次编译时修复了 5 个缺陷，见「过程中发现并修复的问题」7–11。

## 实测输出（Day15，Input 子系统）

`init.c` 加了 `run_program()`（fork+exec 独立测试进程）与 `test_day15()`；
`Makefile` 把两个 Day15 `.ko` 与静态链接的 `key_input_test` 一并打包。
原始日志在 `logs/2026-10-31_day15_*.log`。

```text
input: qemu-key-sim as /devices/virtual/input/input3
key_sim: input device registered, code=28 interval=100ms
[ OK ] loaded /key_input_drv.ko
[ OK ] Day15 /dev/input 存在
[INFO] found /dev/input/event1 as "qemu-key-sim"
[ OK ] ev#1 KEY_ENTER value=1
...
[ OK ] ev#6 KEY_ENTER value=0
[ OK ] 收齐 6 个 EV_KEY 事件
[ OK ] EV_SYN 同步事件数与按键事件匹配
[SUMMARY] key_events=6 syn_events=6 failures=0
RESULT: PASS
[ OK ] Day15 用户态经 evdev 收到 EV_KEY 事件（退出码 0）
[ OK ] Day15 对照：私有字符设备已注册
[ OK ] Day15 open /dev/key_sim
[INFO] Day15 private read=0 (空队列与 EOF 同值)
[INFO] Day15 private byte='1' (no timestamp/type/code)
===== result: PASSED (failures=0) =====
```

> 覆盖：`input_allocate_device` → `evbit`/`keybit` → `input_register_device` →
> `input_report_key` + `input_sync` → input core → evdev → 用户态 `read(struct input_event)`
> 的完整链路，以及"同一事件源走私有字符设备"的对照。
> 不覆盖：真实按键的电气消抖、`EV_REP`、`EV_ABS`、`EVIOCGRAB`、真板输入延迟。
> 事件源是定时器模拟；QEMU vexpress 没有可注入的物理按键。

负向验证（每变一处重建 initramfs 重跑，日志同目录）：

| 变异 | 改动 | 读数 | 判定 |
| --- | --- | --- | --- |
| A | 删 `input_sync()` | `key_events=24 syn_events=3 failures=2` | FAIL（预期） |
| B | `KEY_SIM_CODE` 改 `KEY_A` | `key_events=0 failures=2` | FAIL（预期） |
| C | 删 `__set_bit(EV_KEY, ...)` | `key_events=0 syn_events=0 failures=1` | FAIL（预期） |
| D | 恢复原样 | `key_events=6 syn_events=6 failures=0` | PASS |

变异 A 的机制已用源码钉死：`drivers/input/input.c:351-354` 在
`dev->num_vals >= dev->max_vals - 2`（`max_vals` 初始为 10，即 8 个）时
**自动插入 `input_value_sync` 并提交**，所以 24 个事件被切成 3 批、每批 8 个。
漏调 `input_sync()` 的后果不是丢事件，而是帧边界与按键语义脱钩。

## 实测输出（Day16，sysfs 属性）

`param_demo_dev.ko`（platform_device）+ `param_demo_drv.ko`（`attribute_group`）+
静态链接的 `param_test`。原始日志 `logs/2026-10-31_day16_sysfs.log`。

```text
param_demo param_demo: probe ok, attrs at /sys/devices/platform/param_demo/
[ OK ] Day16 interval 属性存在
[ OK ] Day16 stats 属性存在
[INFO] write "200\n" rc=4 (expect 4)
[INFO] out-of-range write rc=-1 errno=22(Invalid argument)
[INFO] write to RO attr rc=-1 errno=13(Permission denied)
[INFO] interval=50ms, ticks 0 -> 11 in 1s
[INFO] ticks=11 writes=2 rejects=2 interval=50
RESULT: PASS
param_demo param_demo: remove ok, ticks=12 writes=2 rejects=2
===== result: PASSED (failures=0) =====
```

> 覆盖：`DEVICE_ATTR_RW`/`DEVICE_ATTR_RO`、`attribute_group` 成组注册与移除、
> `sysfs_emit`、`kstrtouint`、`store()` 返回 `count` 的语义、负 errno 作为失败通道、
> 只读属性在 kernfs 层被拒（`EACCES`，驱动 `store()` 不被调用）、周期改写后 tick 频率实际变化。
> 不覆盖：`sysfs_notify`/`poll`、`driver_attr`、`bin_attribute`、并发保护。

负向验证：

| 变异 | 改动 | 读数 | 判定 |
| --- | --- | --- | --- |
| A | `return count` → `return 3` | `write "200\n" rc=3`（参数仍被改） | FAIL（预期） |
| B | 越界 `return -EINVAL` → `return 0` | `rc=0 errno=0(Success)` | FAIL（预期） |
| D | 恢复原样 | `rc=4`，`failures=0` | PASS |

## 结论

| Day | 验证项 | 结果 |
| --- | --- | --- |
| 2 | 交叉编译、模块加载、printk | 通过 |
| 3 | `register_chrdev`、`mknod`、open/write/read/release | 通过 |
| 4 | `copy_to_user` / `copy_from_user`、短读边界 | 通过 |
| 5 | 交叉编译、加载、设备节点、卸载 | 通过（寄存器未访问） |
| 6 | platform 设备与驱动匹配、probe | 通过 |
| 7 | 多实例、次设备号、drvdata、设备节点 | 通过 |
| 8 | 设备树 compatible 匹配、属性读取 | 通过 |
| 9 | `devm_gpiod_get`、GPIO 写入与读回 | 通过 |
| 10 | 阻塞/非阻塞/poll/ioctl | 通过（等价替代模块；中断未覆盖） |
| 11 | timer/tasklet/workqueue、卸载清理 | 通过（等价替代模块；中断未覆盖） |
| 15 | input 子系统：`evbit`/`keybit`、`input_sync`、evdev → 用户态 `input_event` | 通过（事件源为定时器模拟）；私有字符设备对照通过 |
| 16 | sysfs 属性：`attribute_group`、`DEVICE_ATTR_RW/RO`、`store()` 返回值语义、kernfs 权限拒绝 | 通过（12 条判据；3 条负向验证） |

## 未覆盖

- Day5：i.MX6ULL 寄存器写入与 LED 点亮现象需要真实目标板。
- Day10/11：需要可触发的中断源，QEMU 无外部输入，`request_irq` 的真实投递未验证。
- Day12/13/14：编译已通过（见上节）；运行期需要 SR501、AT24C02、SPI OLED，`vexpress-a9` 未建模。
- Day15：QEMU 无物理按键，事件源为定时器模拟；真实消抖、`EV_REP`、`EV_ABS`、`EVIOCGRAB` 与真板输入延迟未验证。
- Day16：`sysfs_notify`/`poll`、`driver_attr`、`bin_attribute` 未覆盖；`CONFIG_PROVE_LOCKING` 与 `CONFIG_KCSAN` 均未开启，属性与定时器之间的并发保护无法确定性检出。
- 本内核 `CONFIG_DEBUG_FS` / `CONFIG_DYNAMIC_DEBUG` / `CONFIG_IIO` 均未设置，因此 `dynamic_debug`、`ftrace` 的用户态接口与 IIO 子系统实验在本环境不可用（不是代码问题，是配置缺失）。
- 模块卸载走 `SYS_delete_module` 系统调用（Day5、Day10/11 已覆盖）；`insmod` / `rmmod` 命令行工具路径未验证。

## 过程中发现并修复的问题

1. **Day7 驱动缺少 `.remove` 版本分支**：Linux 6.11 起 `platform_driver.remove` 返回 `void`，编译报 `incompatible pointer type`。
2. **Day9–Day12 缺少 `<linux/of.h>`**：`struct of_device_id` 不完整，编译失败。
3. **initramfs 缺少 `/proc`、`/sys` 目录**：挂载失败，`/proc/devices` 解析不到主设备号。
4. **`init_module` 返回 `EFAULT`**：改用 `finit_module` 由内核直接读取模块文件。
5. **`make olddefconfig` 未带 `ARCH=arm`**：配置被重算成 x86，模块结构体布局错误，加载时在 `platform_device_add` 触发内核 oops。恢复 ARM 配置并全量重编后解决。
6. **vexpress GPIO 驱动识别错误**：`v2m_*_gpios` 由 `basic-mmio-gpio`（`GPIO_GENERIC_PLATFORM`）处理，不是 PL061；且 `sys_mci` 仅 2 个引脚、`sys_led` 被 gpio-leds 占用，最终禁用 gpio-leds 后使用 `sys_led` 第 0 号引脚。
7. **Day10/Day11 函数名冲突**：`key_init` 与 `include/linux/key.h:513` 的 `#define key_init() do { } while (0)` 冲突，报 `macro passed 1 arguments, but takes just 0`。改名为 `key_irq_init` / `key_ctx_init`。
8. **Day11 定时器 API 重命名**：`from_timer` → `timer_container_of`、`del_timer_sync` → `timer_delete_sync`（6.15）。改用 `container_of` 与版本分支。
9. **Day13 用错 drvdata API**：I2C 设备上调用 `platform_set_drvdata` 导致 `implicit declaration`，且语义错误；改为 `i2c_set_clientdata` / `i2c_get_clientdata`。同时补 6.11 起 `.remove` 返回 `void` 的分支。
10. **Day14 完全缺少 `fb_ops`**：`register_framebuffer` 之后 `/dev/fb0` 无法 `read`/`write`，文档里的 `dd ... of=/dev/fb0` 也不可能成功。补 `fb_sys_read`/`fb_sys_write` + 三个 `sys_*` 绘图函数。另修正 `dma_alloc_wc(NULL, ...)`——DMA API 会解引用 `dev`，必须传 `&spi->dev`。
11. **Makefile 交叉编译不一致**：8 个实验用 `$(CC)` 编译用户态测试程序，实测评测产物为 x86-64，与 ARM 模块不匹配；另有 5 处注释写 `make KERN_DIR=...` 而实际变量是 `KDIR`。统一为 `CC_TARGET ?= $(CROSS_COMPILE)gcc`。Day14 还需目标内核开启 `CONFIG_SPI`（bool，不是 tristate）。
12. **`ARCH` / `CROSS_COMPILE` 未 export**：Makefile 里写的 `ARCH ?= arm` 只影响本层变量，不会传给 `$(MAKE) -C $(KDIR)` 的 kbuild 子进程，因此 `make KDIR=<arm-tree>` 单命令构建会按主机 x86 解析头文件路径，报 `fatal error: asm/rwonce.h: No such file or directory`。在 12 个实验 Makefile 中补 `export ARCH` / `export CROSS_COMPILE` 后，仅给 `KDIR` 即可完成 ARM 交叉编译（Day02/Day08 已复验，产物为 ARM ELF）。
13. **`init:` 目标没有先决条件（Day15）**：`init:` 不声明 `init.c` 为依赖时，make 见 `init` 文件已存在即判定最新，改了 `init.c` 也**静默沿用旧二进制**。首轮 QEMU 里 Day15 新增判据一条都没执行，整体却打印 `result: PASSED (failures=0)`——`strings init | grep Day15` 返回 0 才暴露。已改为 `init: init.c`。教训：判据没跑和判据跑过是两种状态，仅看"全是 OK"分不出来。
14. **`key_input_test.c` 的帧闭合判据写错（Day15）**：循环条件只判 `got_key < NEED`，读到第 6 个 `EV_KEY` 立即退出，而该帧的 `EV_SYN` 排在其后，永远读不到 ⇒ `syn_events` 恒为 `key_events - 1`，唯一能证明 `input_sync()` 生效的判据永远假失败。改为 `got_key < NEED || got_syn < got_key`（帧必须闭合）。注意修正后的 `got_syn >= got_key` 在零事件时是 `0 >= 0` 恒真，必须与 `got_key == 6` 联合判读。
