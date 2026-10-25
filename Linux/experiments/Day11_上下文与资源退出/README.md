# Day11 实验：上下文与资源退出

## 目的

验证线程化中断、定时器、工作队列三种机制，并重点检验**卸载顺序**是否正确。

## 步骤

```bash
make KERN_DIR=~/100ask_imx6ull-sdk/Linux-4.9.88
insmod key_ctx.ko

./key_ctx_test -b     # 按一次键
./key_ctx_test -n     # 无事件时立即 EAGAIN

dmesg | tail -10      # 应看到 thread/timer/work 三类日志

rmmod key_ctx
dmesg | tail -5       # 应看到 remove: irq/timer/work released
```

稳定性验证：

```bash
for i in $(seq 100); do
	insmod key_ctx.ko && rmmod key_ctx || break
done
dmesg | grep -iE "bug|oops|warning|use-after-free"
```

## 预期输出

```text
key_ctx: probe: irq=...
key_ctx: work: value=1 count=1
key_ctx: timer: count=1
key_ctx: remove: irq/timer/work released
```

## 机制与上下文

| 机制 | 上下文 | 能否睡眠 | 清理接口 |
| --- | --- | --- | --- |
| `key_hardirq` | 硬中断 | 否 | — |
| `key_thread` | 内核线程 | 是 | `free_irq` |
| `key_timer_cb` | 软中断 | 否 | `del_timer_sync` |
| `key_work_fn` | 进程上下文 | 是 | `cancel_work_sync` |

## 常见问题

| 现象 | 处理 |
| --- | --- |
| `schedule while atomic` | 在 timer 回调里调用了可睡眠函数 |
| 卸载后 oops | 用 `del_timer` 而非 `del_timer_sync` |
| 定时器不触发 | 未 `mod_timer` 或 jiffies 计算错误 |
| 新内核编译报 timer 相关错误 | 本代码已用 4.15 版本分支 |

## 验证状态

未在本机验证（WSL2 无法构建和加载内核模块）。
