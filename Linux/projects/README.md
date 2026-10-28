# 综合项目

综合项目必须围绕一个可验证目标组织，建议目录结构：

```text
projects/项目名称/
├── README.md
├── docs/
├── src/
├── include/
├── tests/
├── scripts/
└── evidence/
```

项目必须包含构建方式、测试方式、日志和故障排查记录。航空主题项目必须与工作代码、数据、参数和接口完全隔离。

## 现有项目

| 项目 | 对应主线 | 状态 | 证据 |
| --- | --- | --- | --- |
| [`aviation-node/`](aviation-node/README.md) | 阶段 7 嵌入式（路线图第 7–8 周） | 可仿真部分完成；CAN/ADC/示波器/`IWDG` 待真板 | `tests/qemu-stm32/logs/`、`tests/qemu-stm32/RESULT.md` §3.10 |
| [`reactor-server/`](reactor-server/README.md) | 阶段 4 高性能服务器模型（`CLAUDE.md` §12 P4 部分恢复） | 8 用例 / 105 判据，默认、ASan、TSan、UBSan 四个构建各自 105/105 `rc=0`；退出路径 `make stress` 20 轮无挂起、无静默丢弃；`make bench` 有固定亲和性 + 5 次分位的本机 loopback 读数（**不是 QPS 基线**）；600 s 挂起根因未闭环 | `projects/reactor-server/tests/logs/`、`projects/reactor-server/tests/bench_logs/`、`benchmarks/reactor-server_2026-10-27.md` |
