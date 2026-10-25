# Day08 实验：设备树匹配

## 目的

验证 `compatible` → `of_match_table` → `probe` 的匹配链路，并正确读取整型属性。

## 步骤

### 1. 合入设备树

把 `led_dts.dts` 中的节点复制到目标板主 dts 的根节点下：

```dts
/ {
	myled: myled_for_test {
		compatible = "100ask,led";
		pin = <3>;
	};
};
```

### 2. 编译 dtb

```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- dtbs
```

### 3. 编译模块

```bash
make KDIR=~/100ask_imx6ull-sdk/Linux-4.9.88
```

### 4. 目标板验证

```bash
insmod led_dts_drv.ko
ls /proc/device-tree/myled_for_test/
cat /proc/device-tree/myled_for_test/compatible; echo
dmesg | tail -5
```

## 预期输出

```text
$ cat /proc/device-tree/myled_for_test/compatible; echo
100ask,led

$ dmesg | tail -2
led_dts myled_for_test: probe: pin=3
```

## 排错顺序

1. `/proc/device-tree` 中没有节点 → dtb 未更新
2. 有节点但 `probe` 未执行 → `compatible` 不一致
3. `probe` 报 `failed to read 'pin'` → 属性名或类型不对
4. 编译报 `of_*` 未声明 → 缺 `<linux/of.h>`

## 验证状态

**已实测**（2026-10-25，QEMU 10.2.1 `vexpress-a9` + Linux 7.0.0 + arm-linux-gnueabihf-gcc 15.2.0）：

- 交叉编译：`led_dts_drv.ko` 为 ARM ELF
- 自定义 dtb 中禁用 `gpio-leds`、追加 `myled_for_test`（`compatible = "100ask,led"`）
- 驱动注册后与节点 `compatible` 匹配成功，probe 从设备树读到 `pin=3`

**未验证**：真实板卡上覆盖原始 dtb 的加载流程。详见 [`../../tests/qemu-vexpress/RESULT.md`](../../tests/qemu-vexpress/RESULT.md)。
