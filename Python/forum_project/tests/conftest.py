"""论坛测试夹具与辅助函数。"""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
if str(REPOSITORY_ROOT) not in sys.path:
    sys.path.insert(0, str(REPOSITORY_ROOT))

from forum_project import create_app  # noqa: E402


@pytest.fixture()
def app(tmp_path):
    application = create_app(
        {
            "TESTING": True,
            "SECRET_KEY": "test-secret",
            "DATABASE": str(tmp_path / "forum-test.db"),
            "PER_PAGE": 2,
        }
    )
    yield application


@pytest.fixture()
def client(app):
    return app.test_client()


def fetch_csrf_token(client, path: str = "/register") -> str:
    """访问一个渲染表单的页面，取得会话中的 CSRF token。"""
    client.get(path)
    with client.session_transaction() as session:
        return session["_csrf_token"]


def register(client, username: str = "alice", password: str = "password123"):
    token = fetch_csrf_token(client, "/register")
    return client.post(
        "/register",
        data={"username": username, "password": password, "csrf_token": token},
        follow_redirects=True,
    )


def login(client, username: str = "alice", password: str = "password123"):
    token = fetch_csrf_token(client, "/login")
    return client.post(
        "/login",
        data={"username": username, "password": password, "csrf_token": token},
        follow_redirects=True,
    )


def create_post(client, title: str = "第一个帖子", content: str = "正文内容"):
    token = fetch_csrf_token(client, "/post/new")
    return client.post(
        "/post/new",
        data={
            "title": title,
            "content": content,
            "category": "python-basics",
            "csrf_token": token,
        },
    )
