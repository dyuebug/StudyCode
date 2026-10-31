# 阶段 5：Linux 与 Docker 部署详解

> **整理日期**：2026-10-29
> **数据来源**：`deployment/` 下的 Dockerfile、docker-compose、Nginx、systemd、部署脚本。
> **状态说明**：Docker 镜像构建、容器健康检查、非 root 运行、命名卷持久化、Compose 启停与 Nginx 反向代理已在 Linux 容器中实测通过（2026-10-29，见 `deployment/验证记录_2026-10-29.md`）。systemd 单元语法已通过 `systemd-analyze verify`，但**未真实启停**；真实服务器与真实域名证书仍未验证。示例配置不代表生产可用。

---

## 1. 部署目标与链路

目标：把本地 Flask 论坛项目变成可通过域名访问、重启后自动恢复、数据不丢失的服务。

最小链路：

```
浏览器 → Nginx（443，TLS） → gunicorn（127.0.0.1:8000） → Flask → SQLite（/var/lib/study-forum/forum.db）
```

职责划分：

| 组件 | 负责 | 不负责 |
|---|---|---|
| Nginx | TLS、静态文件、限流、反向代理、安全响应头 | 运行 Python |
| gunicorn | 多线程处理请求、进程管理 | TLS、静态文件优化 |
| Flask | 业务逻辑、模板渲染、会话 | 长期驻留、崩溃重启 |
| systemd / Docker | 开机自启、崩溃重启、日志收集 | 业务逻辑 |

---

## 2. Linux 基础

### 2.1 目录约定

| 路径 | 用途 | 权限建议 |
|---|---|---|
| `/opt/study-forum` | 代码与虚拟环境 | 属主 `forum:forum`，755 |
| `/var/lib/study-forum` | SQLite 数据库与备份 | 属主 `forum:forum`，750 |
| `/etc/study-forum/forum.env` | 环境变量 | 属主 `root:forum`，640 |
| `/etc/systemd/system/study-forum.service` | 服务单元 | `root:root`，644 |

原则：**代码、数据、配置分离**。升级代码不覆盖数据，备份数据不影响代码。

### 2.2 常用命令

```bash
# 权限
ls -ld /opt/study-forum
sudo chown -R forum:forum /opt/study-forum
sudo chmod 640 /etc/study-forum/forum.env

# 服务
sudo systemctl daemon-reload
sudo systemctl enable --now study-forum
sudo systemctl status study-forum
sudo systemctl restart study-forum

# 日志
sudo journalctl -u study-forum -n 100 --no-pager
sudo journalctl -u study-forum -f

# 端口
ss -lntp | grep 8000
```

### 2.3 环境变量文件

```bash
SECRET_KEY=<随机值>
DATABASE=/var/lib/study-forum/forum.db
PER_PAGE=10
```

生成随机密钥：

```bash
python3 -c "import secrets; print(secrets.token_urlsafe(48))"
```

不要把真实密钥提交到 Git。`.env`、`forum.env` 应出现在 `.gitignore` 中。

---

## 3. gunicorn

### 3.1 为什么不用 `flask run`

`flask run` 是开发服务器：单进程、无进程管理、调试器可能暴露。生产环境用 WSGI 服务器。

### 3.2 启动命令

```bash
gunicorn \
  --bind 127.0.0.1:8000 \
  --workers 1 \
  --threads 4 \
  --access-logfile - \
  --error-logfile - \
  "forum_project:create_app()"
```

### 3.3 为什么是 1 worker + 4 threads

SQLite 同一时刻只允许一个写入者。多 worker 进程并发写入时容易出现：

```
sqlite3.OperationalError: database is locked
```

单 worker + 多线程让写入在进程内串行化，读请求仍可并发。代价是 CPU 密集场景无法利用多核；本项目是 I/O 为主的 CRUD，可以接受。

如果将来迁移到 PostgreSQL，可改为 `workers = 2 * CPU + 1`。

### 3.4 超时

默认 worker 超时 30 秒。长任务应改为异步队列，而不是简单调大超时。

---

## 4. systemd

`deployment/systemd/forum.service` 的关键字段：

```ini
[Service]
Type=simple
User=forum
Group=forum
WorkingDirectory=/opt/study-forum
EnvironmentFile=/etc/study-forum/forum.env
ExecStart=/opt/study-forum/.venv/bin/gunicorn ...
Restart=on-failure
RestartSec=5

NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/lib/study-forum
```

逐项说明：

| 配置 | 作用 |
|---|---|
| `User=forum` | 不以 root 运行 |
| `EnvironmentFile` | 从受保护文件读取密钥，不写进单元文件 |
| `Restart=on-failure` | 崩溃后 5 秒重启 |
| `NoNewPrivileges` | 禁止通过 setuid 提权 |
| `PrivateTmp` | 独立 `/tmp`，避免与其他服务互相影响 |
| `ProtectSystem=strict` | 整个文件系统只读，除 `ReadWritePaths` |
| `ProtectHome` | 禁止访问 `/home` |
| `ReadWritePaths=/var/lib/study-forum` | 唯一可写目录，SQLite 日志文件也在这里 |

注意：`ProtectSystem=strict` 下 `/opt/study-forum` 是只读的，应用不能在那里写缓存或上传文件。需要写入的内容必须放到 `/var/lib/study-forum`。

启用：

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now study-forum
```

---

## 5. Nginx

### 5.1 反向代理

```nginx
location / {
    limit_req zone=forum_limit burst=20 nodelay;

    proxy_pass http://127.0.0.1:8000;
    proxy_http_version 1.1;
    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto $scheme;
}
```

- `proxy_pass` 指向回环地址，gunicorn 不直接暴露公网。
- `X-Forwarded-*` 让后端知道真实客户端和协议。
- `limit_req` 限制每 IP 每秒 10 个请求，突发 20 个。

### 5.2 静态文件

```nginx
location /static/ {
    alias /opt/study-forum/forum_project/static/;
    expires 7d;
    add_header Cache-Control "public";
    access_log off;
}
```

Nginx 直接读文件，不经过 Python；`alias` 末尾的 `/` 必须保留，否则路径拼接会出错。

### 5.3 TLS

```nginx
listen 443 ssl;
http2 on;
ssl_certificate     /etc/letsencrypt/live/forum.example.com/fullchain.pem;
ssl_certificate_key /etc/letsencrypt/live/forum.example.com/privkey.pem;
ssl_protocols TLSv1.2 TLSv1.3;
```

证书用 Certbot 申请。80 端口只保留 ACME 验证和跳转：

```nginx
location / {
    return 301 https://$host$request_uri;
}
```

### 5.4 安全响应头

```nginx
add_header X-Content-Type-Options "nosniff" always;
add_header X-Frame-Options "DENY" always;
add_header Referrer-Policy "strict-origin-when-cross-origin" always;
```

`always` 确保错误响应也带这些头。

### 5.5 已知缺口

- 应用未使用 `ProxyFix`，Flask 看到的 `scheme` 仍是 `http`，`url_for(_external=True)` 和 Secure Cookie 判断可能不正确。生产环境应加：

  ```python
  from werkzeug.middleware.proxy_fix import ProxyFix
  application.wsgi_app = ProxyFix(application.wsgi_app, x_for=1, x_proto=1, x_host=1)
  ```

- 未配置 HSTS、CSP；前者需确认全站 HTTPS 后再加。
- 未限制请求方法，未接入 WAF。

---

## 6. Docker

### 6.1 Dockerfile 结构

```dockerfile
FROM python:3.12-slim AS runtime

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PIP_NO_CACHE_DIR=1 \
    DATABASE=/data/forum.db

WORKDIR /app

COPY forum_project/requirements.txt /app/requirements.txt
RUN pip install --no-cache-dir -r /app/requirements.txt

COPY forum_project /app/forum_project

RUN groupadd --system app \
    && useradd --system --gid app --create-home app \
    && mkdir -p /data \
    && chown -R app:app /app /data

USER app
EXPOSE 8000
```

要点：

- **先复制依赖清单再复制代码**：改代码不会让依赖层缓存失效。
- `PYTHONUNBUFFERED=1`：日志立即输出，便于 `docker logs` 查看。
- `PYTHONDONTWRITEBYTECODE=1`：容器内不生成 `.pyc`。
- 非 root 用户运行。
- `/data` 单独 chown，供挂载卷写入。

### 6.2 健康检查

```dockerfile
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
    CMD python -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:8000/api/posts', timeout=2)"
```

- 不用 `curl`，因为 slim 镜像默认没有安装。
- `start-period` 给应用启动留时间，避免刚启动就被判失败。
- 检查只读接口，不产生副作用。

### 6.3 数据卷

```yaml
volumes:
  - forum-data:/data
```

容器删除后数据仍在。不要用绑定挂载把整个 `/app` 挂进去，否则会覆盖镜像内的代码。

### 6.4 docker compose

```yaml
services:
  forum:
    build:
      context: ../..
      dockerfile: deployment/docker/Dockerfile
    restart: unless-stopped
    ports:
      - "127.0.0.1:8000:8000"
    environment:
      SECRET_KEY: ${SECRET_KEY:?请在 .env 中设置 SECRET_KEY}
      DATABASE: /data/forum.db
    volumes:
      - forum-data:/data
    security_opt:
      - no-new-privileges:true
```

- 端口绑定 `127.0.0.1`，由宿主机 Nginx 转发；不要直接 `0.0.0.0:8000` 暴露。
- `${SECRET_KEY:?...}` 在变量缺失时直接失败，避免用默认密钥启动。
- `restart: unless-stopped` 兼顾开机自启与手动停止。

构建与运行：

```bash
docker compose -f deployment/docker/docker-compose.yml build
docker compose -f deployment/docker/docker-compose.yml up -d
docker compose -f deployment/docker/docker-compose.yml logs -f
```

### 6.5 已知缺口

- 未做多阶段构建压缩体积；`python:3.12-slim` 已足够小。
- 未加资源限制（`mem_limit`、`cpus`）。
- 未配置日志轮转，长期运行可能占满磁盘。
- 镜像未固定 digest，重建时基础镜像可能变化。

---

## 7. 部署脚本

`deployment/scripts/deploy.sh` 的流程：

1. 检查应用目录、虚拟环境、环境变量文件、`git`、`curl`。
2. `git fetch` + `git checkout` + `git pull --ff-only`。
3. `pip install -r requirements.txt`。
4. `pytest forum_project/tests -q`。
5. `systemctl restart`。
6. 最多 5 次健康检查，失败时输出最近 50 行日志并退出非零。

设计原则：

- `set -euo pipefail`：任一命令失败立即停止。
- 测试不通过不重启服务。
- 健康检查失败保留现场，不自动回滚（学习项目简化处理）。
- 脚本不创建用户、目录或密钥文件，缺失时明确报错。

生产项目还应增加：版本标记、旧版本保留、失败自动回滚、部署锁、通知。

---

## 8. 备份与恢复

`deployment/scripts/backup.sh` 使用 SQLite 在线备份：

```bash
sqlite3 "$DB_PATH" ".backup '$BACKUP_PATH'"
```

不要直接 `cp` 正在写入的数据库文件，可能得到不一致的快照。`.backup` 命令会在事务边界复制。

恢复前先停服务，替换文件后再启动，并校验：

```bash
sqlite3 /var/lib/study-forum/forum.db "PRAGMA integrity_check;"
```

---

## 9. 常见故障对照

| 现象 | 可能原因 | 定位命令 |
|---|---|---|
| 502 Bad Gateway | gunicorn 未启动 / 端口不对 | `systemctl status study-forum`、`ss -lntp` |
| 静态文件 404 | `alias` 路径或末尾 `/` 错误 | `nginx -T`、`ls /opt/study-forum/forum_project/static` |
| 登录后立刻退出 | `SECRET_KEY` 每次启动变化 | 检查环境变量文件 |
| `database is locked` | 多 worker 并发写 | 改为 1 worker + threads |
| 启动即失败 | 数据目录无写权限 | `journalctl`、`ls -ld /var/lib/study-forum` |
| 容器健康检查失败 | 启动慢或 `/api/posts` 报错 | `docker logs`、手动 `curl` |
| 表单全部 400 | CSRF token 丢失或会话 Cookie 未带 | 浏览器开发者工具查看 Cookie |
| 脚本报 `$'\r': command not found` | 脚本是 Windows CRLF 换行 | `file script.sh`；`sed -i 's/\r$//' script.sh`，并用 `.gitattributes` 固定 `*.sh text eol=lf` |

---

## 10. 上线前检查清单

- [ ] `SECRET_KEY` 为随机值，未提交到仓库。
- [ ] 数据库目录属主正确，应用可写。
- [ ] gunicorn 只监听 `127.0.0.1`。
- [ ] Nginx 配置通过 `nginx -t`。
- [ ] HTTPS 证书有效，80 端口正确跳转。
- [ ] 防火墙只开放 22/80/443。
- [ ] 服务已 `systemctl enable` 或 `restart: unless-stopped`。
- [ ] 备份脚本已手工运行一次，恢复流程已演练。
- [ ] 日志可查，磁盘空间充足。
- [ ] 关闭 Flask `debug`。

---

## 11. 对应文件

| 内容 | 文件 |
|---|---|
| 部署总览 | `deployment/README.md` |
| Linux 基础 | `deployment/linux/01_Linux基础命令.md` |
| 进程与服务 | `deployment/linux/02_进程与服务管理.md` |
| Nginx | `deployment/linux/03_Nginx反向代理.md`、`deployment/nginx/forum.conf` |
| systemd | `deployment/systemd/forum.service` |
| Docker | `deployment/docker/Dockerfile`、`docker-compose.yml` |
| 脚本 | `deployment/scripts/deploy.sh`、`backup.sh`、`healthcheck.sh` |
| 速查 | `knowledge_base/阶段5_部署运维速查.md` |
| CI | `.github/workflows/ci.yml` |

---

## 12. 验证状态与置信度

**已实测（2026-10-29，Windows 11 + Docker Desktop 29.2.0，Linux 容器）**：`docker build`、容器 `healthy`、`/api/posts` 200、非 root `uid=999`、命名卷重启后数据保留、`docker compose up -d` 健康、`nginx -t` 通过、反向代理 200、80→443 301、静态缺失 404、后端停机 502、限流触发 503。完整输出见 `deployment/验证记录_2026-10-29.md`。

**仍未实测**：systemd 真实启停与日志、`deploy.sh` 的真实依赖安装与服务重启、`backup.sh` 使用发行版 `sqlite3` CLI、真实域名证书、Ubuntu 主机上的 Docker 行为。`systemd-analyze verify` 仅证明单元语法正确，不证明服务能启动。置信度说明：

- systemd 字段含义、Nginx 配置语义、Docker 构建逻辑：高（依据官方文档与通用实践）。
- 1 worker + 4 threads 对 SQLite 的实际效果：中（依据 SQLite 单写入者模型推断，未压测）。
- 部署脚本端到端可用性：低（依赖目标服务器的用户、目录、权限预先配置）。
