# Day15 实验：Input 子系统（按键上报）

> **编号说明**：本实验是**驱动入门课程第 23 集（Input 子系统）的补做**，不属于课程 14 天主线
> （Day01–Day14），也不是教材的第 15 天。与本仓库 `notes/F103_HAL入门/Day15_显示与交互.md`
> 无任何关系——那是 STM32F103 的 OLED/按键讲义，这里是 Linux 内核驱动。

## 目的

同一事件源（100 ms 定时器翻转的模拟按键），用两条完全不同的路径交给用户态：

| 路径 | 驱动做的事 | 用户态拿到什么 |
| --- | --- | --- |
| **A. input 子系统** | `input_allocate_device` → 声明 `evbit`/`keybit` → `input_register_device` → `input_report_key` + `input_sync` | `/dev/input/eventN`，读到标准 `struct input_event`（时间戳 + 类型 + 键码 + 值） |
| **B. 私有字符设备**（对照） | `register_chrdev` + 自定协议 | `/dev/key_sim`，读到 1 个字节 `'1'`/`'0'`，**无时间戳、无类型、无键码** |

这一步对应 `CLAUDE.md` §3 的验收线：**能说明何时接入内核子系统，而非套用私有字符设备**。
Day02–Day09 全部是私有字符设备 / platform 驱动，本实验是第一次把设备接到内核子系统上。

## 内核前提

本实验依赖 input core 与 evdev，缺任一都不成立：

```text
CONFIG_MODULES=y
CONFIG_INPUT=y
CONFIG_INPUT_EVDEV=y
```

`make info` 会检查这三项。本仓库验证用的内核树 `~/kbuild/linux-source-7.0.0` 三项齐全；
缺 `CONFIG_INPUT_EVDEV` 时 `/dev/input` 不出现，用户态一个事件都读不到——这与"驱动写错"是两种故障。

## 文件

| 文件 | 作用 |
| --- | --- |
| `key_input_drv.c` | 实验组：定时器 → `input_report_key` → `input_sync` |
| `key_chrdev_drv.c` | 对照组：同样的定时器 → 环形缓冲 → 私有 `read()` |
| `key_input_test.c` | 用户态测试：按名字找 `eventN`，读 `struct input_event`，校验键码与电平交替 |
| `Makefile` | 两个 `.ko` + 一个静态链接用户态程序 |

## 步骤

```bash
# 1. 交叉编译（需要已编译的 ARM 内核树）
make KDIR=~/kbuild/linux-source-7.0.0

# 2. 产物
file key_input_drv.ko key_chrdev_drv.ko key_input_test
#   ARM relocatable / ARM statically linked

# 3. 放进 initramfs 并在 QEMU 里跑（由 tests/qemu-vexpress 统一驱动）
cd ../../tests/qemu-vexpress
make KDIR=~/kbuild/linux-source-7.0.0
bash run.sh
```

QEMU 里由 `init`（PID 1）`finit_module` 加载模块，再 **fork+exec** `key_input_test` 作为普通进程。
不把测试逻辑写进 `init`：`init` 崩了会 panic，子进程崩了只是一个退出码。

## 预期输出

实测（2026-10-31，QEMU 10.2.1 `vexpress-a9` + Linux 7.0.0 + arm-linux-gnueabihf-gcc 15.2.0）：

```text
input: qemu-key-sim as /devices/virtual/input/input3
key_sim: input device registered, code=28 interval=100ms
[ OK ] loaded /key_input_drv.ko
[ OK ] Day15 /dev/input 存在
[INFO] found /dev/input/event1 as "qemu-key-sim"
[ OK ] ev#1 KEY_ENTER value=1
[ OK ] ev#2 KEY_ENTER value=0
...
[ OK ] ev#6 KEY_ENTER value=0
[ OK ] 收齐 6 个 EV_KEY 事件
[ OK ] EV_SYN 同步事件数与按键事件匹配
[SUMMARY] key_events=6 syn_events=6 failures=0
RESULT: PASS
[ OK ] Day15 用户态经 evdev 收到 EV_KEY 事件（退出码 0）
[ OK ] Day15 对照：私有字符设备已注册
[ OK ] Day15 对照 open /dev/key_sim
[INFO] Day15 private read=0 (空队列与 EOF 同值)
[INFO] Day15 private byte='1' (no timestamp/type/code)
[ OK ] unload key_chrdev_drv (remove path)
[ OK ] unload key_input_drv (remove path)
===== result: PASSED (failures=0) =====
```

- `code=28` 是 `KEY_ENTER`；`input3` 是 input core 的编号，`event1` 是 evdev 的编号，
  **两套编号互不对应**，且两次运行分别出现过 `event1`/`event2`——所以测试必须按
  `EVIOCGNAME` 匹配名字，不能假设编号。
- 对照组第一条读数是 `read=0`：`key_read()` 在队列空时 `return 0`，与 EOF 同值。
  用户态无法区分"暂无数据"和"设备结束"，这是私有协议自造的歧义。

## 关键实现点

### 1. `evbit` / `keybit` 必须显式声明

```c
__set_bit(EV_KEY, ks->idev->evbit);
__set_bit(KEY_SIM_CODE, ks->idev->keybit);
```

不声明也能 `input_register_device()` 成功，但 evdev 不会为它上报任何事件——
"注册成功但不产生事件"。负向验证 C 实测即此（`key_events=0`）。

### 2. `input_sync()` 不是可选项

`input_report_key()` 只把事件放进 input 设备的事件缓冲；`input_sync()` 才把缓冲当作
**一帧**提交给 input core 与 evdev。

实测：删掉 `input_sync()` 后事件**不会消失**，而是被内核强制切帧——
`drivers/input/input.c:351-354` 在 `dev->num_vals >= dev->max_vals - 2`
（`max_vals` 初始为 10，即达到 8 个）时自动插入 `input_value_sync` 并提交。
于是 24 个按键事件被切成 3 批、每批 8 个，`key_events=24 syn_events=3`。

后果不是丢数据，而是**帧边界与按键语义脱钩**：一批 8 个事件跨了 4 次物理按键，
用户态无法判断哪几个属于同一次按下。这条比"收不到事件"更难现场定位。

### 3. 帧闭合才算收完

测试循环的退出条件是 `got_key < NEED || got_syn < got_key`，不是只等到 6 个 `EV_KEY`。
首版只判 `got_key < NEED`，读到第 6 个按键立刻退出，而这一帧的 `EV_SYN` 排在它后面，
永远读不到 ⇒ `syn_events` 恒为 `key_events - 1`，**唯一能证明 `input_sync` 生效的判据
永远假失败**。这是本实验实测出来的判据缺陷（见「验证状态」）。

### 4. 退出顺序不能反

```c
timer_delete_sync(&ks->timer);      /* 先停事件源 */
input_unregister_device(ks->idev);  /* 再注销设备 */
kfree(ks);
```

反过来会让 timer 回调访问已释放的 `idev`，是 use-after-free。对照组同理：
`key_chrdev_exit()` 里 `dropped` 必须在 `kfree(kc)` **之前**取出，否则打印的就是释放后的内存。

### 5. `bustype = BUS_VIRTUAL`

模拟设备不是真实总线上的硬件，标 `BUS_HOST` 会让 udev 规则把它当真按键处理。

## 对照实验：接入子系统到底省了什么

| 项 | input 子系统 | 私有字符设备 |
| --- | --- | --- |
| 用户态读到的数据 | `struct input_event`（`time/code/type/value`） | 1 字节 `'1'`/`'0'` |
| 时间戳 | input core 提供 | 无，用户态自己 `clock_gettime` 且只能在读到时记 |
| 多设备区分 | 内核分配 `eventN` + `EVIOCGNAME` | 自己 `mknod`，编号自己管 |
| 空队列语义 | 阻塞 / `EAGAIN` / `poll()` 可等 | `read()` 返回 0，与 EOF 同值 |
| 事件分组 | `EV_SYN` 显式分帧 | 无分帧概念 |
| 换硬件（真按键） | 只改驱动上报，用户态不动 | 协议要重定，用户态跟着改 |

本实验**没有**验证的部分：真实按键的消抖、`EV_REP` 自动重复、多点触控 `EV_ABS`、
`EVIOCGRAB` 抢占设备、input 子系统的 handler 链（evdev 之外还有 joydev/input-leds）。

## 负向验证

每条只改一处驱动源码，重建 initramfs 后重跑 QEMU，日志在 `tests/qemu-vexpress/logs/`：

| 变异 | 改动 | 实测读数 | 判定 |
| --- | --- | --- | --- |
| A | 删 `input_sync(k->idev);` | `key_events=24 syn_events=3 failures=2` | FAIL（预期） |
| B | `KEY_SIM_CODE` 改 `KEY_A` | `key_events=0 failures=2`，`[FAIL] 键码不符` | FAIL（预期） |
| C | 删 `__set_bit(EV_KEY, ...)` | `key_events=0 syn_events=0 failures=1` | FAIL（预期） |
| D | 恢复原样 | `key_events=6 syn_events=6 failures=0` | PASS（`diff` 确认源码一致） |

**注意 C 的判据弱点**：`got_syn >= got_key` 在零事件时是 `0 >= 0`，
语义上恒真。所以这条判据**不能单独看**，必须与 `got_key == 6` 联合判读——
单看 SYN 判据会让"一个事件都没收到"显示为通过。

日志文件：

- `2026-10-31_day15_input.log`（正常，`PASSED (failures=0)`）
- `2026-10-31_day15_mut_a_no_sync.log`
- `2026-10-31_day15_mut_b_wrong_code.log`
- `2026-10-31_day15_mut_c_no_evbit.log`

## 常见问题

| 现象 | 原因 |
| --- | --- |
| 模块加载成功，`/dev/input/` 下没有新节点 | 缺 `CONFIG_INPUT_EVDEV`，或 `evbit` 未声明 |
| 有 `eventN` 但读不到事件 | 漏 `input_sync()`；事件会被内核按 8 个一批强制切帧 |
| `EVIOCGNAME` 读到的名字不对 | `input_dev->name` 未设置；本测试靠名字匹配，名字错就找不到设备 |
| `input_register_device` 返回 `-ENODEV` | `evbit`/`keybit` 全空，内核拒绝注册一个"什么都不会报"的设备 |
| 卸载时 oops | `timer_delete_sync()` 放在了 `input_unregister_device()` 之后 |

## 验证状态

**已实测**（2026-10-31，QEMU 10.2.1 `vexpress-a9` + Linux 7.0.0 + arm-linux-gnueabihf-gcc 15.2.0）：

- 构建零 warning / 零 error；产物为 ARM ELF
- 设备注册：`input: qemu-key-sim as /devices/virtual/input/input3`，`code=28 interval=100ms`
- 用户态经 evdev 收到 6 个 `EV_KEY`，值严格交替 1/0；`syn_events=6` 证明 `input_sync` 生效
- 整体判据 `result: PASSED (failures=0)`，Day02–Day11 既有判据无回归
- 对照组：私有字符设备读到 1 字节 `'1'`，并实测到空队列 `read=0` 与 EOF 同值
- 四条负向验证（3 变异 + 1 恢复）全部按预期

**过程中实测出的两个真实缺陷**（已修，均非驱动逻辑错误）：

1. `tests/qemu-vexpress/Makefile` 的 `init:` 目标**没有先决条件**，make 见 `init` 文件已存在即判定最新，
   改了 `init.c` 也静默沿用旧二进制——首轮 QEMU 里 Day15 判据一条都没执行，整体却显示 `failures=0`。
   已改为 `init: init.c`。
2. `key_input_test.c` 的循环只判 `got_key`，导致第 6 帧的 `EV_SYN` 永远读不到，
   `syn_events` 恒为 `key_events-1`。已改为要求帧闭合（`got_syn >= got_key`）。

**未验证**：真实按键的电气消抖与抖动波形、`EV_REP` 自动重复、`EV_ABS`（触摸/摇杆）、
`EVIOCGRAB`、多 handler 竞争、真板上的输入延迟。QEMU 没有可注入的物理按键，
本实验的事件源是**定时器模拟**，上报路径（`input_report_key` → `input_sync` → input core →
evdev → 用户态 `read`）是真实的，触发原因是模拟的。
