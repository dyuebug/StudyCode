# Nginx 反向代理

> 学习目标：理解浏览器 → Nginx → Gunicorn 的请求路径，能独立配置反向代理与静态资源缓存。
> 建议时长：45 分钟。

---

## 1. 为什么需要反向代理

| 能力 | 直接暴露 Gunicorn | 经过 Nginx |
|---|---|---|
| HTTPS | 需要额外配置 | 证书与 TLS 集中管理 |
| 静态文件 | 由 Python 进程处理，效率低 | Nginx 直接返回 |
| 限流/黑名单 | 需自己实现 | `limit_req`、`deny` 等内置 |
| 多应用共存 | 端口直接暴露 | 按域名分流 |
| 缓冲慢客户端 | 占用应用进程 | Nginx 先缓冲请求 |

---

## 2. 请求路径

```
浏览器
  │ HTTPS 443
  ▼
Nginx
  ├── /static/*  → 直接读取 /opt/study-forum/forum_project/static
  └── /*         → proxy_pass http://127.0.0.1:8000
                       │
                       ▼
                    Gunicorn → Flask
```

**关键点**：应用只监听 `127.0.0.1`，不监听 `0.0.0.0`，避免绕过 Nginx 直接访问。

---

## 3. 配置要点

完整配置见 `../nginx/forum.conf`。

```nginx
server {
    listen 80;
    server_name forum.example.com;

    location /static/ {
        alias /opt/study-forum/forum_project/static/;
        expires 7d;
        add_header Cache-Control "public";
    }

    location / {
        proxy_pass http://127.0.0.1:8000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

| 指令 | 作用 | 常见错误 |
|---|---|---|
| `proxy_pass` | 转发到上游 | 末尾多写 `/` 会改变路径拼接结果 |
| `proxy_set_header Host` | 保留原始域名 | 不设置会导致 Flask 生成错误的重定向地址 |
| `X-Forwarded-Proto` | 告知应用原始协议 | 不设置会导致 HTTPS 下生成 http 链接 |
| `alias` | 映射静态目录 | 与 `root` 混用会拼出错误路径 |

**`root` 与 `alias` 的区别**：`root` 会把 `location` 部分拼到路径后；`alias` 直接替换。静态目录通常用 `alias`。

---

## 4. 启用配置

```bash
sudo cp deployment/nginx/forum.conf /etc/nginx/sites-available/forum.conf
sudo ln -s /etc/nginx/sites-available/forum.conf /etc/nginx/sites-enabled/forum.conf
sudo nginx -t                 # 语法检查，必须先通过
sudo systemctl reload nginx
```

**变更流程**：改配置 → `nginx -t` → `reload`。不要直接 `restart`，reload 不会中断现有连接。

---

## 5. HTTPS

使用 Certbot：

```bash
sudo apt install certbot python3-certbot-nginx
sudo certbot --nginx -d forum.example.com
sudo certbot renew --dry-run
```

证书自动续期由 systemd timer 完成，可用 `systemctl list-timers | grep certbot` 检查。

---

## 6. 限流（公网必做）

```nginx
limit_req_zone $binary_remote_addr zone=forum:10m rate=10r/s;

location / {
    limit_req zone=forum burst=20 nodelay;
    ...
}
```

含义：单 IP 平均 10 请求/秒，允许突发 20 个，超出返回 503。

---

## 7. 排错

| 现象 | 排查 |
|---|---|
| 502 Bad Gateway | Gunicorn 未启动或端口不对：`ss -tlnp \| grep 8000` |
| 504 Gateway Timeout | 上游处理超时：看应用日志与慢查询 |
| 静态文件 404 | `alias` 路径错误或权限不足 |
| 重定向到 http | 缺少 `X-Forwarded-Proto` |
| 配置不生效 | 未 `reload`，或站点未软链接到 `sites-enabled` |

---

## 8. 练习

1. 用 `curl -I` 验证静态文件返回 `Cache-Control` 与 `Expires`。
2. 停掉 Gunicorn，访问首页，记录 502 页面与 `error.log` 内容。
3. 用 `ab` 或 `wrk` 对首页做一次压测，观察 Nginx 与应用日志。
4. 解释 `proxy_pass http://127.0.0.1:8000` 与 `.../` 的差异。

---

## 9. 成功标准

- [ ] 能画出完整请求链路。
- [ ] 能独立写出反向代理与静态资源配置。
- [ ] 能区分 502 与 504 的成因。
- [ ] 能说明为什么应用只监听 `127.0.0.1`。
