"""论坛系统应用工厂。

功能范围：
1. 用户注册、登录、登出（密码使用 werkzeug 哈希）。
2. 帖子列表、分类筛选、关键词搜索、分页。
3. 发帖、删帖、回复。
4. 只读 JSON API：GET /api/posts。

安全边界：
- 所有 SQL 使用参数绑定，不拼接用户输入。
- 表单 POST 校验 CSRF token；API 仅提供只读接口。
- Cookie 设置 HttpOnly + SameSite=Lax；生产环境必须更换 SECRET_KEY 并启用 HTTPS。

运行：python -m forum_project.run
"""

from __future__ import annotations

import os
import re
import secrets
from functools import wraps
from pathlib import Path
from typing import Any, Callable

from flask import (
    Flask,
    abort,
    flash,
    g,
    jsonify,
    redirect,
    render_template,
    request,
    session,
    url_for,
)
from werkzeug.security import check_password_hash, generate_password_hash

from .db import get_db, init_app as init_database

BASE_DIRECTORY = Path(__file__).resolve().parent
DEFAULT_DATABASE_PATH = BASE_DIRECTORY / "forum.db"

USERNAME_PATTERN = re.compile(r"^[A-Za-z0-9_一-龥]{3,20}$")
CATEGORY_NAMES = {
    "python-basics": "Python 基础",
    "web-development": "Web 开发",
    "database": "数据库",
    "engineering": "工程化与部署",
}


def create_app(test_config: dict[str, Any] | None = None) -> Flask:
    """创建应用实例；测试通过 test_config 覆盖数据库与密钥。"""
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


# ---------------------------------------------------------------- 请求钩子


def register_hooks(application: Flask) -> None:
    @application.before_request
    def load_logged_in_user() -> None:
        user_id = session.get("user_id")
        g.user = None
        if user_id is not None:
            g.user = get_db().execute(
                "SELECT id, username FROM users WHERE id = ?", (user_id,)
            ).fetchone()

    @application.before_request
    def protect_form_posts() -> None:
        if request.method != "POST" or request.path.startswith("/api/"):
            return
        expected_token = session.get("_csrf_token")
        submitted_token = request.form.get("csrf_token", "")
        if not expected_token or not secrets.compare_digest(expected_token, submitted_token):
            abort(400, description="CSRF token 校验失败，请刷新页面后重试。")

    @application.context_processor
    def inject_template_helpers() -> dict[str, Any]:
        return {
            "current_user": g.get("user"),
            "categories": get_db().execute(
                "SELECT name, slug FROM categories ORDER BY id"
            ).fetchall(),
            "csrf_token": csrf_token,
        }


def csrf_token() -> str:
    """取得或生成会话级 CSRF token。"""
    token = session.get("_csrf_token")
    if not token:
        token = secrets.token_urlsafe(24)
        session["_csrf_token"] = token
    return token


def login_required(view: Callable[..., Any]) -> Callable[..., Any]:
    """未登录时跳转登录页，并保留原始访问地址。"""

    @wraps(view)
    def wrapped_view(**kwargs: Any) -> Any:
        if g.get("user") is None:
            return redirect(url_for("login", next=request.path))
        return view(**kwargs)

    return wrapped_view


# ---------------------------------------------------------------- 表单校验


def validate_username(username: str) -> str | None:
    if not USERNAME_PATTERN.fullmatch(username):
        return "用户名需为 3～20 位中文、字母、数字或下划线。"
    return None


def validate_password(password: str) -> str | None:
    if len(password) < 8:
        return "密码至少 8 位。"
    if len(password) > 128:
        return "密码不能超过 128 位。"
    return None


def validate_post(title: str, content: str) -> str | None:
    if not 3 <= len(title) <= 100:
        return "标题长度需在 3～100 个字符之间。"
    if not 1 <= len(content) <= 5000:
        return "正文长度需在 1～5000 个字符之间。"
    return None


def safe_next_url(target: str | None) -> str:
    """仅允许站内相对路径，避免开放重定向。"""
    if not target or not target.startswith("/") or target.startswith("//"):
        return url_for("index")
    return target


# ---------------------------------------------------------------- 路由


def register_routes(application: Flask) -> None:
    @application.route("/")
    def index() -> Any:
        database = get_db()
        category_slug = request.args.get("category", "").strip()
        keyword = request.args.get("q", "").strip()
        page = max(request.args.get("page", 1, type=int), 1)
        per_page = application.config["PER_PAGE"]

        conditions: list[str] = []
        parameters: list[Any] = []
        if category_slug:
            conditions.append("c.slug = ?")
            parameters.append(category_slug)
        if keyword:
            conditions.append("(p.title LIKE ? OR p.content LIKE ?)")
            pattern = f"%{keyword}%"
            parameters.extend([pattern, pattern])

        where_clause = f"WHERE {' AND '.join(conditions)}" if conditions else ""
        total = database.execute(
            f"""SELECT COUNT(*) FROM posts p
                JOIN categories c ON c.id = p.category_id
                {where_clause}""",
            parameters,
        ).fetchone()[0]

        posts = database.execute(
            f"""SELECT p.id, p.title, p.created_at, u.username, c.name AS category_name,
                       c.slug AS category_slug,
                       (SELECT COUNT(*) FROM replies r WHERE r.post_id = p.id) AS reply_count
                FROM posts p
                JOIN users u ON u.id = p.user_id
                JOIN categories c ON c.id = p.category_id
                {where_clause}
                ORDER BY p.id DESC
                LIMIT ? OFFSET ?""",
            [*parameters, per_page, (page - 1) * per_page],
        ).fetchall()

        return render_template(
            "index.html",
            posts=posts,
            total=total,
            page=page,
            per_page=per_page,
            total_pages=max((total + per_page - 1) // per_page, 1),
            active_category=category_slug,
            keyword=keyword,
        )

    @application.route("/post/<int:post_id>")
    def post_detail(post_id: int) -> Any:
        database = get_db()
        post = database.execute(
            """SELECT p.*, u.username, c.name AS category_name
               FROM posts p
               JOIN users u ON u.id = p.user_id
               JOIN categories c ON c.id = p.category_id
               WHERE p.id = ?""",
            (post_id,),
        ).fetchone()
        if post is None:
            abort(404, description="帖子不存在。")
        replies = database.execute(
            """SELECT r.*, u.username FROM replies r
               JOIN users u ON u.id = r.user_id
               WHERE r.post_id = ? ORDER BY r.id""",
            (post_id,),
        ).fetchall()
        return render_template("post_detail.html", post=post, replies=replies)

    @application.route("/post/new", methods=("GET", "POST"))
    @login_required
    def create_post() -> Any:
        if request.method == "POST":
            title = request.form.get("title", "").strip()
            content = request.form.get("content", "").strip()
            category_slug = request.form.get("category", "").strip()

            error = validate_post(title, content)
            category = get_db().execute(
                "SELECT id FROM categories WHERE slug = ?", (category_slug,)
            ).fetchone()
            if error is None and category is None:
                error = "请选择有效的分类。"
            if error is not None:
                flash(error, "error")
            else:
                database = get_db()
                cursor = database.execute(
                    "INSERT INTO posts (user_id, category_id, title, content) VALUES (?, ?, ?, ?)",
                    (g.user["id"], category["id"], title, content),
                )
                database.commit()
                return redirect(url_for("post_detail", post_id=cursor.lastrowid))

        return render_template("new_post.html")

    @application.route("/post/<int:post_id>/delete", methods=("POST",))
    @login_required
    def delete_post(post_id: int) -> Any:
        database = get_db()
        post = database.execute("SELECT user_id FROM posts WHERE id = ?", (post_id,)).fetchone()
        if post is None:
            abort(404, description="帖子不存在。")
        if post["user_id"] != g.user["id"]:
            abort(403, description="只能删除自己发布的帖子。")
        database.execute("DELETE FROM posts WHERE id = ?", (post_id,))
        database.commit()
        flash("帖子已删除。", "info")
        return redirect(url_for("index"))

    @application.route("/post/<int:post_id>/reply", methods=("POST",))
    @login_required
    def create_reply(post_id: int) -> Any:
        content = request.form.get("content", "").strip()
        post = get_db().execute("SELECT id FROM posts WHERE id = ?", (post_id,)).fetchone()
        if post is None:
            abort(404, description="帖子不存在。")
        if not 1 <= len(content) <= 2000:
            flash("回复长度需在 1～2000 个字符之间。", "error")
        else:
            database = get_db()
            database.execute(
                "INSERT INTO replies (post_id, user_id, content) VALUES (?, ?, ?)",
                (post_id, g.user["id"], content),
            )
            database.commit()
            flash("回复成功。", "info")
        return redirect(url_for("post_detail", post_id=post_id))

    @application.route("/register", methods=("GET", "POST"))
    def register() -> Any:
        if request.method == "POST":
            username = request.form.get("username", "").strip()
            password = request.form.get("password", "")
            error = validate_username(username) or validate_password(password)
            database = get_db()
            if error is None and database.execute(
                "SELECT id FROM users WHERE username = ?", (username,)
            ).fetchone():
                error = "该用户名已被注册。"

            if error is not None:
                flash(error, "error")
            else:
                database.execute(
                    "INSERT INTO users (username, password_hash) VALUES (?, ?)",
                    (username, generate_password_hash(password)),
                )
                database.commit()
                flash("注册成功，请登录。", "info")
                return redirect(url_for("login"))

        return render_template("register.html")

    @application.route("/login", methods=("GET", "POST"))
    def login() -> Any:
        if request.method == "POST":
            username = request.form.get("username", "").strip()
            password = request.form.get("password", "")
            user = get_db().execute(
                "SELECT id, username, password_hash FROM users WHERE username = ?",
                (username,),
            ).fetchone()

            if user is None or not check_password_hash(user["password_hash"], password):
                flash("用户名或密码不正确。", "error")
            else:
                csrf_value = session.get("_csrf_token")
                session.clear()
                session["user_id"] = user["id"]
                session["username"] = user["username"]
                if csrf_value:
                    session["_csrf_token"] = csrf_value
                return redirect(safe_next_url(request.args.get("next")))

        return render_template("login.html")

    @application.route("/logout", methods=("POST",))
    def logout() -> Any:
        session.clear()
        flash("已退出登录。", "info")
        return redirect(url_for("index"))

    @application.get("/api/posts")
    def api_posts() -> Any:
        page = max(request.args.get("page", 1, type=int), 1)
        per_page = min(max(request.args.get("per_page", 10, type=int), 1), 50)
        rows = get_db().execute(
            """SELECT p.id, p.title, p.created_at, u.username, c.slug AS category
               FROM posts p
               JOIN users u ON u.id = p.user_id
               JOIN categories c ON c.id = p.category_id
               ORDER BY p.id DESC LIMIT ? OFFSET ?""",
            (per_page, (page - 1) * per_page),
        ).fetchall()
        return jsonify(
            {
                "status": "success",
                "page": page,
                "per_page": per_page,
                "items": [dict(row) for row in rows],
            }
        )


# ---------------------------------------------------------------- 错误处理


def register_error_handlers(application: Flask) -> None:
    @application.errorhandler(403)
    @application.errorhandler(404)
    @application.errorhandler(400)
    def handle_client_error(error) -> Any:
        code = getattr(error, "code", 500)
        description = getattr(error, "description", "请求无法处理。")
        return render_template("error.html", code=code, message=description), code
