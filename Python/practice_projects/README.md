# 实践项目索引（第 48～52 天）

对应 `daily_tasks/2026-10-30_第48天_任务.md` 至 `第52天_任务.md`。
这些是为学习任务准备的**可运行示例**，不是真实学习记录，也不代表任务已完成。

| 目录 | 主题 | 关键文件 | 可自动验证 |
|---|---|---|---|
| `day48_app_factory/` | Flask 应用工厂与配置覆盖 | `app.py`、`test_day48_app_factory.py` | `pytest` 4 项 |
| `day49_linux_tools/` | Linux 输出解析与目录权限检查 | `layout_checker.py`、`test_layout_checker.py` | `pytest` 5 项（1 项在 Windows 跳过） |
| `day50_nginx_proxy/` | Nginx 反代、静态别名、限流 | `nginx.conf`、`compose.yaml`、`verify.sh` | 需 Docker + curl |
| `day51_docker_app/` | 非 root 镜像、健康检查、命名卷 | `Dockerfile`、`compose.yaml`、`app.py` | `pytest` 3 项；镜像验证需 Docker |
| `day52_ci/` | 本地复现 CI、前端任务、回滚流程 | `ci_local.sh`、`github-workflow.yml` | `bash -n`；完整执行需 npm/pip-audit |

## 运行 Python 测试

```bash
python_basics/.venv/Scripts/python.exe -m pytest practice_projects -q
```

## 约定

- 每个目录的 `README.md` 记录运行命令、预期输出和清理步骤；
- 需要 Docker、npm 或网络的部分，README 明确标注「未在生成时执行」；
- 示例任务文件中的 `[ ]` 保持未完成，只有用户确认真实学习后才更新日志与统计。
