# 构建与辅助脚本

这里保存构建、测试、清理、部署、日志采集和基准辅助脚本的说明。

脚本必须注明：

- 支持的平台和 shell
- 输入参数
- 依赖工具
- 执行前提
- 生成或修改的文件
- 失败时的退出状态
- 是否包含破坏性操作

## 现有脚本

### `run_stage.sh`

阶段实验的统一入口，把各阶段自带 `run.sh` 的参数原样透传。

| 项 | 说明 |
| --- | --- |
| 平台 / shell | WSL2 Ubuntu，`bash 4+` |
| 用法 | `./scripts/run_stage.sh list`、`./scripts/run_stage.sh 6`、`./scripts/run_stage.sh 6 --judge` |
| 依赖 | 各阶段目录下的 `run.sh`（当前阶段 1–6 全部具备） |
| 前提 | 在仓库内执行；`experiments/SYS_0<N>_*/` 命名不改变 |
| 生成 / 修改 | 不生成文件；被调用的 `run.sh` 会写各自的 `logs/` 与 `build/` |
| 退出状态 | 透传对应 `run.sh`；编号不存在或缺少 `run.sh` 时返回 `2` |
| 破坏性操作 | 无 |

### `check_repo.sh`

`CLAUDE.md` §13「变更检查清单」里**能机器校验**的那部分。

| 项 | 说明 |
| --- | --- |
| 平台 / shell | WSL2 Ubuntu，`bash 4+` |
| 用法 | `./scripts/check_repo.sh`（无参数） |
| 依赖 | `grep` `sed` `find` `git`（用 `git check-ignore` 区分"残留"与"已被忽略的产物"） |
| 检查项 | 实验/项目 README 存在性、日志与周报命名（§5）、README 相对链接可达性、临时文件残留、`benchmarks/` 与 `scripts/` 是否有实质文件 |
| 生成 / 修改 | **只读**，不修改任何文件 |
| 退出状态 | `0` 全部通过；`1` 有 FAIL；`2` 不在预期目录 |
| 破坏性操作 | 无 |

边界：它读不出"文档里写的结论是否真的实测过"——那属于 §1.1 的人工判断。
它只保证**结构**不出错：目录有说明、命名合约定、链接不悬空、没有残留物。

两条误报已经踩过并修掉，留在这里免得下一个人重踩：

- C++ lambda 写法 `= [](int fd, ...)` 会被链接正则误认成 Markdown 链接 →
  现在跳过含空格或 `<>{},` 的目标；
- `tests/qemu-stm32/aviation/__pycache__` 被根 `.gitignore` 忽略，不该算"残留" →
  现在先过一遍 `git check-ignore`。
