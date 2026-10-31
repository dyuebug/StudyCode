"""论坛系统接口与页面行为测试。

覆盖：注册、登录、发帖、回复、删帖权限、搜索、分页、JSON API、CSRF、404。
"""

from __future__ import annotations

from conftest import create_post, fetch_csrf_token, login, register


def test_index_starts_empty(client):
    response = client.get("/")
    assert response.status_code == 200
    assert "还没有帖子" in response.get_data(as_text=True)


def test_register_then_login(client):
    response = register(client)
    assert "注册成功" in response.get_data(as_text=True)

    response = login(client)
    assert response.status_code == 200
    assert "alice" in response.get_data(as_text=True)


def test_register_rejects_short_password(client):
    response = register(client, password="short")
    assert "密码至少 8 位" in response.get_data(as_text=True)
    assert "注册成功" not in response.get_data(as_text=True)


def test_register_rejects_duplicate_username(client):
    register(client)
    response = register(client)
    assert "已被注册" in response.get_data(as_text=True)


def test_login_rejects_wrong_password(client):
    register(client)
    response = login(client, password="wrong-password")
    assert "用户名或密码不正确" in response.get_data(as_text=True)


def test_create_post_requires_login(client):
    response = client.get("/post/new")
    assert response.status_code == 302
    assert "/login" in response.headers["Location"]


def test_create_post_appears_on_index(client):
    register(client)
    login(client)
    response = create_post(client, title="Flask 学习记录", content="今天学习了路由。")
    assert response.status_code == 302

    index_page = client.get("/").get_data(as_text=True)
    assert "Flask 学习记录" in index_page
    assert "1 条回复" not in index_page


def test_create_post_rejects_short_title(client):
    register(client)
    login(client)
    response = create_post(client, title="短")
    assert "标题长度" in response.get_data(as_text=True)


def test_reply_is_displayed(client):
    register(client)
    login(client)
    create_post(client, title="如何理解闭包", content="求解答")
    token = fetch_csrf_token(client, "/post/1")

    response = client.post(
        "/post/1/reply",
        data={"content": "闭包保存了外层作用域。", "csrf_token": token},
        follow_redirects=True,
    )
    assert "闭包保存了外层作用域" in response.get_data(as_text=True)
    assert "回复（1）" in response.get_data(as_text=True)


def test_author_can_delete_own_post(client):
    register(client)
    login(client)
    create_post(client)
    token = fetch_csrf_token(client, "/post/1")

    response = client.post(
        "/post/1/delete", data={"csrf_token": token}, follow_redirects=True
    )
    assert "帖子已删除" in response.get_data(as_text=True)
    assert "还没有帖子" in response.get_data(as_text=True)


def test_other_user_cannot_delete_post(app):
    author = app.test_client()
    register(author)
    login(author)
    create_post(author)

    visitor = app.test_client()
    register(visitor, username="bob")
    login(visitor, username="bob")
    token = fetch_csrf_token(visitor, "/post/1")

    response = visitor.post("/post/1/delete", data={"csrf_token": token})
    assert response.status_code == 403


def test_search_and_category_filter(client):
    register(client)
    login(client)
    create_post(client, title="SQLite 索引", content="数据库优化")
    create_post(client, title="Python 生成器", content="迭代协议")

    search_page = client.get("/?q=SQLite").get_data(as_text=True)
    assert "SQLite 索引" in search_page
    assert "Python 生成器" not in search_page

    category_page = client.get("/?category=database").get_data(as_text=True)
    assert "还没有帖子" in category_page


def test_pagination_uses_configured_page_size(client):
    register(client)
    login(client)
    for index in range(3):
        create_post(client, title=f"帖子 {index}", content="内容")

    first_page = client.get("/").get_data(as_text=True)
    assert "第 1 / 2 页" in first_page
    second_page = client.get("/?page=2").get_data(as_text=True)
    assert "第 2 / 2 页" in second_page


def test_api_posts_returns_json(client):
    register(client)
    login(client)
    create_post(client, title="API 测试", content="内容")

    response = client.get("/api/posts")
    assert response.status_code == 200
    payload = response.get_json()
    assert payload["status"] == "success"
    assert payload["items"][0]["title"] == "API 测试"


def test_missing_post_returns_404(client):
    response = client.get("/post/9999")
    assert response.status_code == 404
    assert "帖子不存在" in response.get_data(as_text=True)


def test_post_without_csrf_token_is_rejected(client):
    register(client)
    login(client)
    response = client.post(
        "/post/new",
        data={"title": "无令牌", "content": "内容", "category": "python-basics"},
    )
    assert response.status_code == 400
    assert "CSRF" in response.get_data(as_text=True)
