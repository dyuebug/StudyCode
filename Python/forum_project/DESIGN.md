# 学习论坛设计说明

> 适用版本：1.0（2026-10-29）
> 定位：阶段 4 教学项目，重点展示“请求 → 校验 → 持久化 → 渲染”的完整闭环。

---

## 1. 设计目标

| 目标 | 落地方式 |
|---|---|
| 结构清晰、便于逐层阅读 | 应用工厂 + 独立 `db.py` + 模板目录 |
| 数据可持久化且可测试 | SQLite + 可注入 `DATABASE` 配置 |
| 安全基线 | 参数绑定、密码哈希、CSRF、输出转义、开放重定向防护 |
| 可部署 | 提供 Dockerfile、Gunicorn、Nginx、systemd 配置 |

非目标：高并发、分布式、全文检索、富文本编辑器。

---

## 2. 分层与请求流程

```
浏览器
  │  HTTP
  ▼
Flask 路由（app.py）
  │  1. load_logged_in_user  读取 session → g.user
  │  2. protect_form_posts   校验 CSRF
  ▼
参数校验（validate_*）
  │
  ▼
SQLite（db.py / schema.sql）
  │  sqlite3.Row
  ▼
Jinja2 模板（自动转义）
```

**为什么用应用工厂**：测试可以传入临时数据库路径与独立密钥，避免共享全局状态。

**为什么不用 ORM**：教学目标是理解 SQL 与 SQLite 行为；表结构简单，手写 SQL 更透明。规模扩大后再引入 SQLAlchemy。

---

## 3. 数据模型

```
users 1 ────< posts >──── 1 categories
  │             │
  │             └────< replies >──── users
  └──────────────────────────────────┘
```

| 表 | 关键字段 | 约束 |
|---|---|---|
| `users` | `username`、`password_hash` | 用户名唯一 |
| `categories` | `name`、`slug` | 两者唯一，启动时写入 4 个初始分类 |
| `posts` | `user_id`、`category_id`、`title`、`content` | 外键级联删除作者；分类删除受 RESTRICT 保护 |
| `replies` | `post_id`、`user_id`、`content` | 帖子或用户删除时级联删除 |

索引：`posts(created_at DESC)`、`posts(category_id)`、`replies(post_id)`。

**设计取舍**：没有单独的“版块管理员”表。删除权限只判断作者，逻辑简单且可测试；需要管理员时再增加 `role` 字段。

---

## 4. 安全设计

| 风险 | 处理 |
|---|---|
| SQL 注入 | 全部使用 `?` 参数绑定；动态 SQL 只拼接固定条件片段 |
| 密码泄露 | `werkzeug.security.generate_password_hash`（PBKDF2） |
| CSRF | 会话级随机 token；所有非 API 的 POST 校验；`compare_digest` 常量时间比较 |
| XSS | Jinja2 默认自动转义；正文以文本方式渲染，不输出原始 HTML |
| 开放重定向 | `safe_next_url` 仅接受以 `/` 开头且不以 `//` 开头的站内路径 |
| 越权删除 | 删除前比对 `posts.user_id` 与 `g.user.id`，不匹配返回 403 |
| 会话固定 | 登录成功后 `session.clear()`，再写入用户标识并保留 CSRF token |
| Cookie | `HttpOnly=True`、`SameSite=Lax` |

**未覆盖**：速率限制、账户锁定、邮箱验证、审计日志。公网部署前应在网关层补齐。

---

## 5. 分页与查询

- 默认每页 10 条，测试配置为 2 条以便验证翻页。
- `COUNT(*)` 与列表查询使用相同的 `WHERE` 条件与参数，避免总数与列表不一致。
- 搜索使用 `LIKE '%关键词%'`；数据量增大后应改为 FTS5 全文索引。

---

## 6. 错误处理

| 状态码 | 场景 | 表现 |
|---|---|---|
| 400 | CSRF 校验失败、非法请求 | 错误页，提示刷新后重试 |
| 403 | 删除他人帖子 | 错误页，提示仅能删除自己的帖子 |
| 404 | 帖子不存在 | 错误页，提示帖子不存在 |

---

## 7. 扩展路线

1. 帖子编辑与软删除（`deleted_at`）。
2. 管理员角色与举报处理。
3. FTS5 全文搜索。
4. 附件上传与对象存储。
5. 迁移到 PostgreSQL + SQLAlchemy，补充数据库迁移脚本（Alembic）。
6. 增加速率限制与结构化日志，接入 CI 与容器部署。
