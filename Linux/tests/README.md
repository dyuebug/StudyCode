# 测试与验证

保存单元测试、集成测试、压力测试、长稳测试、测试数据和执行说明。

测试至少区分：

- 功能正确性
- 错误路径
- 并发正确性
- 资源释放
- 性能与稳定性

每次测试记录环境、命令、输入、预期结果和实际结果。

## 仿真验证套件

| 目录 | 目标 | 入口 |
| --- | --- | --- |
| [`qemu-vexpress/`](qemu-vexpress/) | ARM vexpress-a9 上的 Linux 用户态 / 驱动实验 | 见该目录 README 与 `RESULT.md` |
| [`qemu-stm32/`](qemu-stm32/) | STM32F103 裸机实验在 `-M stm32vldiscovery` 上的可验证边界 | [`qemu-stm32/README.md`](qemu-stm32/README.md)，实测记录在同目录 `RESULT.md` |
