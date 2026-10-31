# 阶段 5：部署与运维

> 建立日期：2026-10-29
> 状态：学习材料已生成；Docker 与 Nginx 配置已在 Linux 容器中实机验证（见 `验证记录_2026-10-29.md`），**systemd 与真实服务器仍未验证**。

## 目录

```
deployment/
├── README.md                 # 本文件：学习路径与验收标准
├── 验证记录_2026-10-29.md    # 已执行命令与真实输出
├── linux/
│   ├── README.md
│   ├── 01_Linux基础命令.md
│   ├── 02_进程与服务管理.md
│   └── 03_Nginx反向代理.md
├── docker/
│   ├── README.md
│   ├── Dockerfile
│   └── docker-compose.yml
├── nginx/
│   └── forum.conf
├── systemd/
│   └── forum.service
└── scripts/
    ├── deploy.sh
    ├── backup.sh
    └── healthcheck.sh
```

CI 配置位于仓库根目录：`.github/workflows/ci.yml`。

## 学习顺序

| 顺序 | 主题 | 材料 | 建议时长 | 验收 |
|---|---|---|---|---|
| 1 | Linux 基础命令 | `linux/01_Linux基础命令.md` | 45 分钟 | 能独立在服务器上定位文件、查看日志、检查端口 |
| 2 | 进程与服务管理 | `linux/02_进程与服务管理.md` | 45 分钟 | 能用 systemd 启停服务并查看日志 |
| 3 | Nginx 反向代理 | `linux/03_Nginx反向代理.md` | 45 分钟 | 能解释 `proxy_pass` 与静态资源分离 |
| 4 | Docker 容器化 | `docker/README.md` | 60 分钟 | 能构建镜像并运行容器，数据卷持久化 |
| 5 | 自动化部署与备份 | `scripts/*.sh` | 45 分钟 | 能说明部署脚本每一步失败时如何处理 |
| 6 | CI/CD | `.github/workflows/ci.yml` | 30 分钟 | 能解释流水线各步骤与失败排查方式 |

## 环境约定

| 项目 | 约定 |
|---|---|
| 服务器 | Ubuntu 22.04 / 24.04 LTS |
| 应用目录 | `/opt/study-forum` |
| 运行用户 | `forum`（非 root） |
| 数据目录 | `/var/lib/study-forum/forum.db` |
| 环境变量文件 | `/etc/study-forum/forum.env` |
| 监听方式 | Gunicorn `127.0.0.1:8000`，由 Nginx 反向代理 |
| 对外端口 | 80 / 443 |

## 安全清单（公网部署前必须逐项确认）

- [ ] `SECRET_KEY` 使用随机值，且不写入仓库。
- [ ] 数据库文件权限为 `640`，属主为 `forum`。
- [ ] 服务器仅开放 22、80、443；8000 不对外。
- [ ] 配置 HTTPS（Let's Encrypt 或云厂商证书）。
- [ ] 关闭 Flask 调试模式。
- [ ] 定期备份并验证恢复流程。
- [ ] 在 Nginx 层增加请求速率限制。

## 验证状态

### 已实机验证（2026-10-29 ～ 10-30，Windows 11 + Docker Desktop 29.2.0，Linux 容器）

| 项目 | 结果 |
|---|---|
| `docker build` | 成功，镜像 `study-forum:1.0` |
| 容器健康检查 | `healthy` |
| `GET /api/posts` | `HTTP 200` |
| 非 root 运行 | `uid=999(app)` |
| 命名卷持久化 | 重启容器后数据仍在 |
| Compose 启动 | `healthy`，`HTTP 200`；缺少 `SECRET_KEY` 时按预期拒绝启动 |
| `nginx -t`（临时自签证书） | 语法通过 |
| Nginx 反代 | `200`；80 → 443 跳转 `301`；静态缺失 `404`；后端停机 `502`；限流触发 `503` |
| `healthcheck.sh` | 正向 `exit 0`；404 时 `exit 1` |
| `backup.sh` | 生成 `.db.gz`，解压后数据可查询（`sqlite3` CLI 用 Python 等价接口替代） |
| `deploy.sh` 控制流 | 真实 Git + pytest + curl；`pip`/`systemctl` 为 mock，最终 `exit 0` |
| `systemd-analyze verify` | 在 WSL Ubuntu（systemd 259）通过，`EXIT=0` |
| 脚本换行符 | 发现 CRLF 导致 Linux 执行失败，已改为 LF，并新增 `.gitattributes` 与回归测试 |

完整命令与原始输出见 `验证记录_2026-10-29.md`。

### 仍未验证

以下内容**没有**在真实服务器执行，不能视为已完成：

1. `systemd/forum.service` 的真实启停、开机自启与日志收集（单元语法已通过 `systemd-analyze verify`）。
2. `deploy.sh` 的真实依赖安装与服务重启（`pip`、`systemctl` 为 mock）。
3. `backup.sh` 使用发行版 `sqlite3` CLI 的备份，以及 14 天保留清理分支。
4. 真实域名下的 Let's Encrypt 证书申请与续期。
5. Docker 在 Ubuntu 主机（非 Docker Desktop）上的行为差异。

完成以上记录前，阶段 5 状态保持“学习中”，不计入完成。
