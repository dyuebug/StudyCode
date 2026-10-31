# 第 51 天练习：Docker 构建与非 root 运行

对应任务：`daily_tasks/2026-10-30_第51天_任务.md`。

## 构建与运行

```bash
docker build -f practice_projects/day51_docker_app/Dockerfile \
  -t day51-practice:1.0 practice_projects/day51_docker_app

docker run -d --name day51-test \
  -p 127.0.0.1:18081:8000 \
  -e SECRET_KEY=test-only \
  -v day51-data:/data \
  day51-practice:1.0
```

## 验证

```bash
docker inspect --format '{{.State.Health.Status}}' day51-test   # healthy
docker exec day51-test id                                      # 非 root
curl -sS http://127.0.0.1:18081/health                          # {"status": "ok"}
curl -sS http://127.0.0.1:18081/api/posts                       # items 为空
docker restart day51-test
docker logs day51-test --tail 20
```

清理：

```bash
docker rm -f day51-test
docker volume rm day51-data
```

## Compose 校验

```bash
cd practice_projects/day51_docker_app
SECRET_KEY=test-only docker compose config
```

未设置 `SECRET_KEY` 时应报：

```text
required variable SECRET_KEY is missing a value
```

这是预期行为，不要改成默认密钥绕过。

## 对应知识点

- 依赖清单先复制、代码后复制，才能命中构建缓存；
- `PYTHONUNBUFFERED=1` 让日志立即进入 `docker logs`；
- 健康检查用 `urllib.request`，避免安装 `curl`；
- 端口绑定 `127.0.0.1`，由宿主机反向代理对外；
- 命名卷保存数据，删除容器不丢数据。

> 本目录是练习配置，未在本次生成中执行；实际结果需按运行输出记录。
