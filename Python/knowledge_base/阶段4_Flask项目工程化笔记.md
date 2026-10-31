# 阶段 4：Flask 项目工程化笔记

> **整理日期**：2026-10-29
> **数据来源**：`forum_project/` 实际代码、`python_basics/留言板项目/` v1～v4、`forum_project/tests/test_forum.py`。
> **状态说明**：论坛项目的 16 项测试已通过；留言板 v4 的 5 项 API 测试已通过。本文描述的是**代码现状**，不是学习完成记录。

---

## 1. 应用工厂（Application Factory）

### 1.1 为什么不用模块级全局 `app`

留言板 v1～v3 采用模块级 `app = Flask(__name__)`，简单直接，但有两个问题：

- 测试时无法为每个用例创建独立数据库，模块只在导入时初始化一次。
- 配置在导入时固定，无法按环境切换。

论坛项目改为工厂函数：

```python
def create_app(test_config: dict[str, Any] | None = None) -> Flask:
    application = Flask(__name__)
    application.config.from_mapping(
        SECRET_KEY=os.environ.get("SECRET_KEY", "dev-secret-change-before-deploy"),
        DATABASE=os.environ.get("DATABASE", str(DEFAULT_DATABASE_PATH)),
        PER_PAGE=int(os.environ.get("PER_PAGE", "10")),
        SESSION_COOKIE_HTTPONLY=True,
        SESSION_COOKIE_SAMESITE="Lax",
    )
    if test_config:
        application.config.update(test_config)

    Path(application.config["DATABASE"]).parent.mkdir(parents=True, exist_ok=True)
    init_database(application)
    register_hooks(application)
    register_routes(application)
    register_error_handlers(application)
    return application
```

要点：

- 默认值来自环境变量，方便部署时覆盖。
- `test_config` 最后更新，测试可注入临时数据库和固定密钥。
- 数据库父目录在启动时创建，避免首次运行因目录不存在失败。
- 建表、钩子、路由、错误处理分函数注册，职责单一。

### 1.2 入口文件

`forum_project/run.py` 只负责创建实例并启动开发服务器：

```python
application = create_app()

if __name__ == "__main__":
    application.run(host="127.0.0.1", port=5006, debug=True)
```

生产环境不使用 `debug=True`，而是由 gunicorn 导入 `forum_project:create_app()`。

---

## 2. 请求生命周期与 `g` / `session`

一次请求的典型顺序：

1. `before_request`：`load_logged_in_user()` 读取 `session["user_id"]`，把用户行放入 `g.user`。
2. `before_request`：`protect_form_posts()` 校验所有非 `/api/` 的 POST 请求的 CSRF token。
3. 视图函数执行，通过 `get_db()` 取得当前请求的数据库连接。
4. `teardown_appcontext`：`close_db()` 关闭连接。

`g` 与 `session` 的区别：

| 对象 | 生命周期 | 存储位置 | 用途 |
|---|---|---|---|
| `g` | 单次请求 | 进程内存 | 当前用户、数据库连接等请求级缓存 |
| `session` | 跨请求 | 客户端签名 Cookie | 登录状态、CSRF token |

不要把数据库连接放进 `session`，它无法序列化，也会泄露服务端细节。

---

## 3. 数据库访问

### 3.1 每请求一个连接

```python
def get_db() -> sqlite3.Connection:
    if "db" not in g:
        connection = sqlite3.connect(current_app.config["DATABASE"])
        connection.row_factory = sqlite3.Row
        connection.execute("PRAGMA foreign_keys = ON")
        g.db = connection
    return g.db


def close_db(exception: BaseException | None = None) -> None:
    connection = g.pop("db", None)
    if connection is not None:
        connection.close()
```

设计理由：

- SQLite 连接不适合跨线程共享；每请求一个连接最简单。
- `sqlite3.Row` 支持 `row["username"]` 按列名访问，模板可读性更好。
- **外键默认关闭**，必须每个连接执行 `PRAGMA foreign_keys = ON`，否则 `ON DELETE CASCADE` 不生效。
- 在 `teardown_appcontext` 关闭，即使视图抛异常也能释放。

### 3.2 参数化查询

```python
user = get_db().execute(
    "SELECT id, username, password_hash FROM users WHERE username = ?",
    (username,),
).fetchone()
```

禁止字符串拼接 SQL：

```python
# 错误：SQL 注入
"SELECT * FROM users WHERE username = '" + username + "'"
```

### 3.3 约束与索引

`schema.sql` 中的关键约束：

- `users.username UNIQUE`：数据库层兜底，防止并发注册重名。
- `posts.user_id ... ON DELETE CASCADE`：删除用户时连带删除其帖子。
- `posts.category_id ... ON DELETE RESTRICT`：分类仍被引用时禁止删除。
- `idx_posts_created_at`、`idx_posts_category`、`idx_replies_post`：覆盖列表排序、分类筛选、回复查询。

应用层校验不能替代数据库约束；两者都要有。

---

## 4. 认证与安全

### 4.1 密码存储

```python
from werkzeug.security import check_password_hash, generate_password_hash

password_hash = generate_password_hash(password)
# 校验
check_password_hash(user["password_hash"], password)
```

- 不保存明文，不使用 MD5/SHA1。
- `check_password_hash` 使用恒定时间比较，降低时序攻击风险。
- 登录失败统一提示“用户名或密码不正确”，不区分用户是否存在。

### 4.2 CSRF 防护

论坛项目采用**会话 token + 表单隐藏字段**：

```python
@application.before_request
def protect_form_posts() -> None:
    if request.method != "POST" or request.path.startswith("/api/"):
        return
    expected_token = session.get("_csrf_token")
    submitted_token = request.form.get("csrf_token", "")
    if not expected_token or not secrets.compare_digest(expected_token, submitted_token):
        abort(400, description="CSRF token 校验失败，请刷新页面后重试。")
```

模板中通过上下文处理器拿到 `csrf_token`：

```html
<input type="hidden" name="csrf_token" value="{{ csrf_token() }}">
```

要点：

- `secrets.compare_digest` 做恒定时间比较。
- 登录成功后 `session.clear()`，但要把 CSRF token 取出来再写回，否则下一次 POST 全部失败。
- `GET` 不校验；因此**任何有副作用的操作都不能用 GET**，删除和退出登录都限定为 `POST`。

### 4.3 开放重定向

```python
def safe_next_url(target: str | None) -> str:
    if not target or not target.startswith("/") or target.startswith("//"):
        return url_for("index")
    return target
```

只允许站内相对路径。`//evil.com` 也是合法 URL（协议相对），必须显式排除。

### 4.4 其他响应头与 Cookie

- `SESSION_COOKIE_HTTPONLY=True`：禁止 JavaScript 读取会话 Cookie。
- `SESSION_COOKIE_SAMESITE="Lax"`：降低跨站请求携带 Cookie 的风险。
- 生产环境应再加 `SESSION_COOKIE_SECURE=True` 并启用 HTTPS。

---

## 5. 路由设计

| 方法 | 路径 | 作用 | 登录要求 |
|---|---|---|---|
| GET | `/` | 帖子列表，支持分页与分类筛选 | 否 |
| GET | `/post/<int:post_id>` | 帖子详情与回复 | 否 |
| GET/POST | `/post/new` | 发布帖子 | 是 |
| POST | `/post/<int:post_id>/delete` | 删除自己的帖子 | 是 |
| POST | `/post/<int:post_id>/reply` | 回复 | 是 |
| GET/POST | `/register` | 注册 | 否 |
| GET/POST | `/login` | 登录 | 否 |
| POST | `/logout` | 退出 | 是 |
| GET | `/api/posts` | JSON 列表 | 否 |

设计原则：

- 资源用名词，动作用 HTTP 方法表达。
- 修改状态一律 `POST`，便于统一 CSRF 校验。
- 路径参数用 `<int:post_id>`，非整数直接 404。
- API 与页面路由分离，`/api/` 前缀跳过 CSRF（只读接口）。

---

## 6. 分页

```python
page = max(request.args.get("page", 1, type=int), 1)
per_page = min(max(request.args.get("per_page", 10, type=int), 1), 50)
rows = get_db().execute(
    """SELECT ... ORDER BY p.id DESC LIMIT ? OFFSET ?""",
    (per_page, (page - 1) * per_page),
).fetchall()
```

- `type=int` 解析失败时返回默认值，不抛异常。
- 页码下限 1；每页数量上限 50，防止一次拉取过多数据。
- 排序使用 `id DESC` 而不是 `created_at`，避免同秒创建时顺序不稳定。

---

## 7. 错误处理

```python
@application.errorhandler(400)
@application.errorhandler(403)
@application.errorhandler(404)
def handle_client_error(error):
    return render_template("error.html", error=error), error.code
```

- 统一渲染错误页，避免暴露堆栈。
- 生产环境关闭 `debug`，否则异常页会泄露源码片段。
- 参数校验失败返回 400，未登录访问受保护页面由 `login_required` 重定向。

---

## 8. 测试策略

`forum_project/tests/conftest.py` 提供四个 fixture：

| Fixture | 作用 |
|---|---|
| `app` | 用 `tmp_path` 建临时数据库，覆盖 `DATABASE` 与 `SECRET_KEY` |
| `client` | Flask 测试客户端 |
| `csrf_token` | 从会话中取出 token，供 POST 使用 |
| `register` / `login` / `create_post` | 封装常用操作，减少重复 |

典型用例覆盖：

- 正常路径：注册、登录、发帖、回复、删除。
- 参数错误：空标题、超长正文、错误分类。
- 未授权：未登录发帖返回重定向；删除他人帖子返回 403。
- 资源不存在：不存在的帖子返回 404。
- 持久化：发帖后重新查询数据库能读到。
- 安全：缺少 CSRF token 返回 400；`next=//evil.com` 不跳出站外。

运行：

```bash
python_basics/.venv/Scripts/python.exe -m pytest forum_project/tests -q
```

---

## 9. 常见陷阱

1. **忘记 `PRAGMA foreign_keys = ON`**：级联删除静默失效。
2. **在视图外使用 `g`**：`g` 只在应用上下文中可用；脚本任务需 `with app.app_context():`。
3. **登录后 `session.clear()` 丢掉 CSRF token**：下一个表单直接 400。
4. **用 GET 删除资源**：无法被 CSRF 钩子保护，也可能被浏览器预取误触发。
5. **`debug=True` 上线**：Werkzeug 调试器可执行任意代码。
6. **默认 `SECRET_KEY` 上线**：会话可被伪造。部署时必须注入随机值。
7. **SQLite 多进程写入**：gunicorn 使用多 worker 时容易出现 `database is locked`；论坛部署示例采用 1 worker + 4 threads。

---

## 10. 对应文件

| 内容 | 文件 |
|---|---|
| 应用工厂与路由 | `forum_project/app.py` |
| 数据库连接 | `forum_project/db.py` |
| 表结构与索引 | `forum_project/schema.sql` |
| 启动入口 | `forum_project/run.py` |
| 测试 | `forum_project/tests/test_forum.py`、`tests/conftest.py` |
| 设计说明 | `forum_project/DESIGN.md` |
| 留言板 v4 | `python_basics/留言板项目/app_v4_sqlite.py` |
| 部署材料 | `deployment/` |

---

## 11. 尚未验证

- 未在真实服务器上运行 gunicorn + Nginx + systemd 组合；Docker 与 Nginx 已在 Linux 容器中实测（`deployment/验证记录_2026-10-29.md`），systemd 仍未验证。
- 未做并发压测，SQLite 锁行为仅依据单机测试与部署配置推断（置信度：中）。
- 未接入邮件验证、找回密码、权限分级等功能。
