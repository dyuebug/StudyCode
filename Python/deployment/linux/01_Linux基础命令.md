# Linux 基础命令

> 学习目标：能在 Linux 服务器上完成“找文件、看日志、查端口、改权限”四类日常操作。
> 建议时长：45 分钟。
> 环境：Ubuntu 22.04/24.04，普通用户 + `sudo`。

---

## 1. 先理解三个概念

| 概念 | 含义 | 为什么重要 |
|---|---|---|
| 一切皆文件 | 目录、设备、进程信息都通过文件系统暴露 | 很多操作最终是读写文件 |
| 权限三元组 | 属主 / 属组 / 其他人，各含读(4)、写(2)、执行(1) | 部署失败常由权限引起 |
| 进程有父进程 | 服务通常由 systemd 拉起，而不是手动 `python app.py` | 手动启动的进程在退出登录后会结束 |

---

## 2. 目录与文件

### 2.1 查看与切换

```bash
pwd                     # 当前目录
ls -lah                 # 含隐藏文件、人类可读大小
cd /opt/study-forum     # 切换目录
cd -                    # 返回上一个目录
tree -L 2               # 两级目录树（需安装 tree）
```

### 2.2 查找文件

```bash
find /opt/study-forum -name "*.py" -type f
find /var/log -name "*.log" -mtime -1        # 最近 1 天修改
which python3
```

`find` 的常见陷阱：忘记加 `-type f` 会把目录也列出来；在 `/` 下全盘查找可能很慢，先缩小路径。

### 2.3 查看内容

```bash
cat forum.env
less /var/log/nginx/access.log       # 支持搜索，q 退出
head -n 20 app.log
tail -n 50 app.log
tail -f app.log                      # 持续跟踪，Ctrl+C 退出
```

### 2.4 权限

```bash
ls -l forum.db
# -rw-r----- 1 forum forum 40960 ...
chmod 640 forum.db
chown forum:forum forum.db
```

**判断方法**：`-rw-r-----` = 属主读写、属组只读、其他人无权限。数据库不应给其他人读权限。

---

## 3. 进程与端口

```bash
ps aux | grep gunicorn
pgrep -af gunicorn
ss -tlnp | grep 8000          # 查看监听端口与对应进程
curl -i http://127.0.0.1:8000/api/posts
```

**排查顺序**：先确认进程存在 → 再确认端口监听 → 最后用 `curl` 验证应用响应。不要一上来就重启服务。

---

## 4. 日志

Ubuntu 使用 systemd-journald 统一收集服务日志：

```bash
journalctl -u study-forum -n 100 --no-pager
journalctl -u study-forum -f
journalctl -u study-forum --since "10 minutes ago"
```

Nginx 日志：

```bash
tail -f /var/log/nginx/access.log
tail -f /var/log/nginx/error.log
```

---

## 5. 压缩与传输

```bash
tar -czf backup.tar.gz /var/lib/study-forum
scp backup.tar.gz user@server:/tmp/
rsync -av --delete /opt/study-forum/ user@server:/opt/study-forum/
```

`rsync --delete` 会删除目标端多余文件，执行前先用 `--dry-run` 预览。

---

## 6. 常见错误与陷阱

| 现象 | 常见原因 | 定位命令 |
|---|---|---|
| `Permission denied` | 文件属主或权限不对 | `ls -l`、`id` |
| `Address already in use` | 端口被占用 | `ss -tlnp \| grep 8000` |
| 服务启动后立即退出 | 环境变量缺失或路径错误 | `journalctl -u study-forum -n 50` |
| `command not found` | 未安装或不在 PATH | `which`、`apt list --installed` |
| 修改配置后不生效 | 未 reload 服务 | `systemctl reload nginx` |

---

## 7. 练习

1. 在 `/tmp` 下创建三级目录，写入 3 个文件，用一条 `find` 命令找出所有 `.txt`。
2. 创建一个文件，把权限改为 `640`，用 `ls -l` 解释每一位含义。
3. 启动论坛应用后，用 `ss` 找到监听 5006 的进程，再用 `curl` 请求 `/api/posts`。
4. 制造一次端口占用：同时启动两个实例，记录报错原文与解决方式。

---

## 8. 成功标准

- [ ] 能不看笔记写出“查找最近修改的日志”的命令。
- [ ] 能解释 `-rw-r-----` 的含义。
- [ ] 能按“进程 → 端口 → HTTP 响应”的顺序定位服务问题。
- [ ] 能说出 `tail -f` 与 `journalctl -f` 的适用场景。
