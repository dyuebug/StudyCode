# 第 50 天练习：Nginx 反向代理

对应任务：`daily_tasks/2026-10-30_第50天_任务.md`。

## 文件

| 文件 | 作用 |
|---|---|
| `nginx.conf` | 最小反向代理、静态资源 `alias`、限流 |
| `compose.yaml` | Nginx + `traefik/whoami` 上游 |
| `static/app.css` | 验证静态映射 |
| `verify.sh` | 检查 200/200/404 |

## 运行

```bash
docker compose -f practice_projects/day50_nginx_proxy/compose.yaml up -d
bash practice_projects/day50_nginx_proxy/verify.sh
```

预期输出：

```text
[通过] / -> 200
[通过] /static/app.css -> 200
[通过] /static/missing.css -> 404
```

验证 502：

```bash
docker compose -f practice_projects/day50_nginx_proxy/compose.yaml stop backend
curl -sS -o /dev/null -w '%{http_code}\n' http://127.0.0.1:18080/
# 502
docker compose -f practice_projects/day50_nginx_proxy/compose.yaml start backend
```

清理：

```bash
docker compose -f practice_projects/day50_nginx_proxy/compose.yaml down
```

## 观察点

1. `alias` 末尾斜杠缺失时，`/static/app.css` 会被映射成 `staticapp.css`，返回 404。
2. `proxy_set_header X-Forwarded-Proto $scheme` 只传递协议，后端仍需 `ProxyFix` 才会信任。
3. `limit_req` 的 503 与后端停机的 502 是不同来源，不能只看状态码判断。

> 本目录是练习配置，未在本次生成中执行；执行结果需按实际输出记录。
