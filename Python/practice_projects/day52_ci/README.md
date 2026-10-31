# 第 52 天练习：本地复现 CI 与回滚演练

对应任务：`daily_tasks/2026-10-30_第52天_任务.md`。

## 文件

| 文件 | 作用 |
|---|---|
| `ci_local.sh` | 在本地按 CI 顺序执行：编译 → pytest → 依赖审计 → 前端测试，并汇总退出码 |
| `github-workflow.yml` | 带 `frontend-test` 任务的参考工作流，位于练习目录，不会被 GitHub 执行 |

## 运行

```bash
bash practice_projects/day52_ci/ci_local.sh
```

跳过不适用或耗时的步骤：

```bash
SKIP_AUDIT=1 bash practice_projects/day52_ci/ci_local.sh
SKIP_FRONTEND=1 bash practice_projects/day52_ci/ci_local.sh
```

脚本特点：

- 失败不中断，最后统一打印 `通过/失败/跳过` 计数；
- 自动优先使用 `python_basics/.venv` 中的解释器；
- `npm` 不存在时标记跳过，而不是伪装成通过。

## 关键差异：`npm ci` vs `npm install`

| 维度 | `npm ci` | `npm install` |
|---|---|---|
| 依据 | 严格按 `package-lock.json` | 可更新 lock 文件 |
| 行为 | 先删除 `node_modules` 再安装 | 增量安装 |
| 适用 | CI、可重复构建 | 本地添加依赖 |
| 缺 lock | 直接失败 | 仍然可用 |

CI 使用 `npm ci`，才能保证「本地通过 = CI 通过」。

## 可回滚发布流程（最小版）

```bash
# 1. 记录当前版本
git rev-parse HEAD > /tmp/previous_commit

# 2. 发布前检查
bash deployment/scripts/healthcheck.sh
python -m pytest -q

# 3. 备份数据库
bash deployment/scripts/backup.sh

# 4. 切换版本并重启
git fetch --all
git checkout <new-tag>
pip install -r requirements.txt
sudo systemctl restart study-forum

# 5. 发布后验证
bash deployment/scripts/healthcheck.sh
curl -fsS http://127.0.0.1:8000/api/posts >/dev/null

# 6. 失败回滚
git checkout "$(cat /tmp/previous_commit)"
# 恢复最近一次数据库备份
sudo systemctl restart study-forum
bash deployment/scripts/healthcheck.sh
```

回滚阈值建议：健康检查连续 3 次失败，或核心接口 5xx 持续 2 分钟。

## 当前 CI 的缺口

1. 未运行前端测试（本练习补上 `frontend-test`）。
2. `pip-audit` 使用 `continue-on-error`，漏洞不会阻塞合并。
3. 没有 Docker 构建验证，镜像问题到部署时才暴露。
4. 没有覆盖率门槛。
5. 没有产物缓存或构建产物留存。
6. 未验证 `deployment/scripts/*.sh` 的可执行性（建议 `bash -n`）。
7. 没有发布/回滚自动化，仍依赖手工步骤。

> 本目录是练习材料，脚本未在本次生成中执行；实际结果需按运行输出记录。
