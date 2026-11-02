# Day16 实验：sysfs 属性（驱动参数的内核标准接口）

> **编号说明**：本实验是**驱动入门课程的扩展补做**（课程 14 天主线外的子系统接入练习），
> 不是教材的第 16 天，与 `notes/F103_HAL入门/` 下的 Day16 讲义无关。

## 目的

Day02–Day09 的驱动全部靠**私有字符设备**与用户态交互：自己定协议、自己 `mknod`、
自己解析字节流。但有一类交互根本不该走字符设备——**改一个参数、读一个统计**。
内核给这类动作的标准答案是 sysfs。

本实验把两个属性挂到 platform 设备上：

| 属性 | 权限 | 语义 |
| --- | --- | --- |
| `interval` | RW | 定时器周期（ms），范围 10–5000，越界写返回 `-EINVAL` |
| `stats` | RO | 计数：`ticks` / `writes` / `rejects` / `interval` |

用户态只有普通的 `open`/`read`/`write`，**没有 `mknod`、没有私有协议、没有 ioctl**。

## 内核前提

```text
CONFIG_SYSFS=y
CONFIG_MODULES=y
```

`make info` 会检查。本仓库验证用的内核树 `~/kbuild/linux-source-7.0.0` 已开启。

## 文件

| 文件 | 作用 |
| --- | --- |
| `param_demo_dev.c` | platform_device，只描述"有个设备叫 param_demo" |
| `param_demo_drv.c` | platform_driver + `attribute_group`（`DEVICE_ATTR_RW` / `DEVICE_ATTR_RO`） |
| `param_test.c` | 用户态：按路径读写属性，校验返回值与 errno |

## 步骤

```bash
make KDIR=~/kbuild/linux-source-7.0.0

cd ../../tests/qemu-vexpress
make KDIR=~/kbuild/linux-source-7.0.0
bash run.sh
```

设备与驱动同名（`param_demo`），必须**先加载设备、再加载驱动**，否则 probe 不会发生。

## 预期输出

实测（2026-10-31，QEMU 10.2.1 `vexpress-a9` + Linux 7.0.0 + arm-linux-gnueabihf-gcc 15.2.0）：

```text
param_demo: device register -> 0
[ OK ] loaded /param_demo_dev.ko
[ OK ] loaded /param_demo_drv.ko
param_demo param_demo: probe ok, attrs at /sys/devices/platform/param_demo/
[ OK ] Day16 interval 属性存在
[ OK ] Day16 stats 属性存在
[INFO] initial interval=500
[ OK ] Day16 初始 interval = 500
[INFO] write "200\n" rc=4 (expect 4)
[ OK ] Day16 写 200 返回 4 字节（store 返回 count）
[ OK ] Day16 读回 200
[INFO] out-of-range write rc=-1 errno=22(Invalid argument)
[ OK ] Day16 越界写得到 EINVAL
[ OK ] Day16 非数字写得到 EINVAL
[INFO] write to RO attr rc=-1 errno=13(Permission denied)
[ OK ] Day16 只读属性 stats 拒绝写入
[INFO] interval=50ms, ticks 0 -> 11 in 1s
[ OK ] Day16 新周期生效（1s 内 tick >= 10）
[INFO] ticks=11 writes=2 rejects=2 interval=50
[ OK ] Day16 writes 计数 = 2
[ OK ] Day16 rejects 计数 = 2
[SUMMARY] failures=0
RESULT: PASS
[ OK ] unload param_demo_drv (remove path)
param_demo param_demo: remove ok, ticks=12 writes=2 rejects=2
[ OK ] unload param_demo_dev (remove path)
===== result: PASSED (failures=0) =====
```

## 关键语义

### 1. `store()` 必须返回 `count`

`DEVICE_ATTR_RW` 展开出的 `store()` 签名是：

```c
static ssize_t interval_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count);
```

`count` 是内核交给你的字节数（含结尾换行）。只有 `return count` 才表示"全部消费"。
返回一个更小的正数，内核**不会报错**，`write()` 就返回那个短值——用户态看到的是短写。

实测变异 A 把 `return count` 改成 `return 3`（写入 `"200\n"` 共 4 字节）：

```text
[INFO] write "200\n" rc=3 (expect 4)
param_demo: interval set to 200ms by userspace
```

参数**确实改了**，但 `write()` 少返回 1 字节。`echo` 之类不检查返回值的工具完全察觉不到，
用返回值校验的程序才能发现——这是"看起来成功了"的典型来源。

### 2. 拒绝写入要返回负 errno，不能返回 0 或短值

```c
if (val < INTERVAL_MIN || val > INTERVAL_MAX) {
	ctx->rejects++;
	return -EINVAL;      /* 用户态 write() 得到 -1 / EINVAL */
}
```

实测变异 B 改成 `return 0`：

```text
[INFO] out-of-range write rc=0 errno=0(Success)
```

用户态拿到 `rc=0`、`errno` 未变——**和"写了 0 个字节"无法区分**。
负 errno 才是内核与用户态之间唯一可靠的失败通道。

### 3. 只读属性的拒绝发生在 kernfs 层，驱动收不到

写 `stats`（`DEVICE_ATTR_RO`）得到：

```text
[INFO] write to RO attr rc=-1 errno=13(Permission denied)
```

`errno=13` 是 `EACCES`，由 `kernfs` 依据属性权限位拒绝，`stats_show` 之外的代码一行都没执行。
所以这次失败**不计入驱动的 `rejects`**（实测 `rejects=2`，只算了越界与非数字两次）。

这条区分很重要：属性权限不生效时，先看是 kernfs 拒绝还是 store 校验拒绝，两者位置不同。

### 4. `attribute_group` 一次注册、一次移除

```c
static struct attribute *param_attrs[] = {
	&dev_attr_interval.attr,
	&dev_attr_stats.attr,
	NULL,
};
static const struct attribute_group param_group = { .attrs = param_attrs };

sysfs_create_group(&pdev->dev.kobj, &param_group);
...
sysfs_remove_group(&pdev->dev.kobj, &param_group);
```

逐条 `device_create_file()` 也能实现，但失败回滚要自己写；成组注册由内核统一处理。
`sysfs_emit()` 负责格式化输出（带 `PAGE_SIZE` 边界检查）。

### 5. 卸载顺序：先断用户态入口，再停事件源

```c
sysfs_remove_group(&pdev->dev.kobj, &param_group);
timer_delete_sync(&ctx->timer);
```

反过来会留下一个窗口：属性仍可写，定时器也在跑，`store()` 改的正是定时器正在读的结构体。
`interval_ms` 用 `READ_ONCE`/`WRITE_ONCE` 修饰，语义上是"单变量读写可见性"，
但真正消除竞争还需要锁——见本文「未验证」。

### 6. 路径来自命名约定，不来自驱动

`platform_device.name = "param_demo"` → `/sys/devices/platform/param_demo/`。
`/sys/class/...` 是另一条路径（需要 `class_create`），两者指向同一个属性文件。

## 负向验证

| 变异 | 改动 | 实测读数 | 判定 |
| --- | --- | --- | --- |
| A | `return count` → `return 3` | `write "200\n" rc=3`，其余判据通过 | FAIL（预期） |
| B | 越界 `return -EINVAL` → `return 0` | `rc=0 errno=0(Success)` | FAIL（预期） |
| D | 恢复原样 | `rc=4`，`failures=0` | PASS（`diff` 确认源码一致） |

日志：`tests/qemu-vexpress/logs/2026-10-31_day16_*.log`。

## 常见问题

| 现象 | 原因 |
| --- | --- |
| `/sys/devices/platform/param_demo/` 不存在 | 驱动没 probe：设备与驱动名字不一致，或设备在驱动之后加载 |
| `write()` 返回比写入长度小 | `store()` 没 `return count` |
| `write()` 返回 0 但值没变 | `store()` 返回 0，被当成"没写"，调用方可能重试或忽略 |
| 写只读属性得到 `EACCES` | 正常，kernfs 按权限位拒绝，不是驱动的问题 |
| 卸载后 `dmesg` 出现 oops | `sysfs_remove_group` 放在 `timer_delete_sync` 之后 |

## 验证状态

**已实测**（2026-10-31，QEMU 10.2.1 `vexpress-a9` + Linux 7.0.0）：

- 构建零 warning / 零 error
- 12 条判据全部通过（`failures=0`），`store` / `show` / 越界 / 只读 / 计数 / 周期生效全覆盖
- 两条变异各自精确失败，恢复后转绿
- Day15 的 5 条判据无回归

**未验证**：

- `interval_ms` 的并发保护——`READ_ONCE`/`WRITE_ONCE` 只保证单变量读写可见性，
  不是互斥。本内核 `CONFIG_PROVE_LOCKING` / `CONFIG_KCSAN` 均未开启，竞争无法确定性检出
- `CONFIG_DEBUG_FS` 未开启，因此 `dynamic_debug` 与 `ftrace` 的用户态接口在本内核不可用
  （两者的开关都在 debugfs 下），本实验无法演示这两类调试手段
- `sysfs_notify()` 与 `poll()` 联动、`driver_attr`（挂在 driver 而非 device 上）、
  `device_create_file()` 逐条注册的失败回滚、二进制属性 `bin_attribute` 均未覆盖
