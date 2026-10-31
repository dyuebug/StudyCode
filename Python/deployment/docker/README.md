# Docker 容器化

> 学习目标：把论坛应用打包成镜像，用数据卷持久化 SQLite，并理解构建上下文与层缓存。
> 建议时长：60 分钟。
> **验证状态（2026-10-29）**：镜像构建、容器健康检查、非 root 运行、命名卷持久化与 Compose 启动均已实测通过，详见 `../验证记录_2026-10-29.md`。

---

## 1. 镜像与容器的区别

| 概念 | 类比 | 特点 |
|---|---|---|
| 镜像 Image | 安装光盘 | 只读、分层、可版本化 |
| 容器 Container | 运行中的程序 | 在镜像上增加可写层，生命周期短 |
| 数据卷 Volume | 外接硬盘 | 独立于容器，删除容器后数据仍在 |

**关键原则**：容器内不保存需要长期保留的数据。SQLite 文件必须放在卷中。

---

## 2. 构建

构建上下文是**仓库根目录**，因为 Dockerfile 需要复制 `forum_project/`：

```bash
docker build -f deployment/docker/Dockerfile -t study-forum:1.0 .
```

`-f` 指定 Dockerfile 路径，最后的 `.` 是构建上下文。两者不能混淆。

查看镜像分层：

```bash
docker history study-forum:1.0
```

**为什么先复制 `requirements.txt` 再复制代码**：依赖不变时，这一层命中缓存，重建只需复制代码，速度更快。

---

## 3. 运行

```bash
docker run --rm -p 8000:8000 \
  -e SECRET_KEY="$(python3 -c 'import secrets;print(secrets.token_urlsafe(48))')" \
  -v forum-data:/data \
  study-forum:1.0
```

访问 <http://127.0.0.1:8000>。

| 参数 | 作用 |
|---|---|
| `-p 8000:8000` | 宿主机端口:容器端口 |
| `-e` | 注入环境变量 |
| `-v forum-data:/data` | 命名卷持久化数据库 |
| `--rm` | 容器退出后自动删除 |

生产环境建议只映射到回环地址：`-p 127.0.0.1:8000:8000`，由 Nginx 对外。

---

## 4. Compose

```bash
cd deployment/docker
export SECRET_KEY="$(python3 -c 'import secrets;print(secrets.token_urlsafe(48))')"
docker compose up -d --build
docker compose ps
docker compose logs -f forum
docker compose down
```

Compose 会自动在项目名前缀下创建命名卷。`docker compose down -v` 会**删除数据卷**，执行前确认。

---

## 5. 验证清单

```bash
docker compose exec forum python -c "import sqlite3;print(sqlite3.connect('/data/forum.db').execute('select count(*) from users').fetchone())"
docker inspect --format '{{.State.Health.Status}}' docker-forum-1
curl -i http://127.0.0.1:8000/api/posts
```

重启容器后确认数据仍在：

```bash
docker compose restart forum
curl http://127.0.0.1:8000/api/posts
```

---

## 6. 常见问题

| 问题 | 原因 | 处理 |
|---|---|---|
| `COPY failed: forum_project not found` | 构建上下文不是仓库根目录 | 在根目录执行构建，或调整 `context` |
| 容器启动后立即退出 | 缺少 `SECRET_KEY` 或端口冲突 | `docker logs <容器>` |
| 数据重启后丢失 | 未挂载卷，或写了容器内路径 | 检查 `DATABASE` 与 `-v` |
| `database is locked` | 多 worker 并发写 SQLite | 保持 1 worker，或迁移数据库 |
| 镜像体积过大 | 未使用 slim 基础镜像、无 `.dockerignore` | 检查 `.dockerignore` 与分层 |

---

## 7. 练习

1. 构建镜像并记录镜像大小；修改一行 CSS 后重建，观察哪些层命中缓存。
2. 用 `docker run` 启动容器，注册一个用户，重启容器后确认用户仍在。
3. 故意不挂载卷，重复上一步，记录数据丢失过程。
4. 解释 `EXPOSE` 与 `-p` 的区别。
5. 用 `docker compose down -v` 前后的 `docker volume ls` 对比，说明风险。

---

## 8. 成功标准

- [ ] 能独立构建并运行镜像。
- [ ] 能说明构建上下文的作用。
- [ ] 能解释数据卷为何必需。
- [ ] 能根据 `docker logs` 定位启动失败原因。
- [ ] 能说出 `down` 与 `down -v` 的差异。
