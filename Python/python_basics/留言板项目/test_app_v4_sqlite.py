"""留言板 SQLite v4.0 API 测试。"""

from pathlib import Path

import pytest

from app_v4_sqlite import create_app


@pytest.fixture()
def client(tmp_path: Path):
    application = create_app(tmp_path / "messages.db")
    application.config.update(TESTING=True)
    with application.test_client() as test_client:
        yield test_client


def test_get_messages_returns_empty_list_at_start(client) -> None:
    response = client.get("/api/messages")

    assert response.status_code == 200
    assert response.get_json() == []


def test_add_message_persists_message(client) -> None:
    response = client.post(
        "/api/messages",
        json={"name": "Alice", "content": "Hello SQLite"},
    )

    assert response.status_code == 201
    response_body = response.get_json()
    assert response_body["status"] == "success"
    assert response_body["message"]["name"] == "Alice"
    assert response_body["message"]["content"] == "Hello SQLite"

    messages_response = client.get("/api/messages")
    assert messages_response.get_json()[0]["content"] == "Hello SQLite"


def test_add_message_rejects_empty_content(client) -> None:
    response = client.post("/api/messages", json={"name": "Alice", "content": "   "})

    assert response.status_code == 400
    assert response.get_json()["message"] == "留言内容不能为空"


def test_delete_message_removes_existing_message(client) -> None:
    create_response = client.post(
        "/api/messages",
        json={"name": "Bob", "content": "To be removed"},
    )
    message_id = create_response.get_json()["message"]["id"]

    delete_response = client.delete(f"/api/messages/{message_id}")

    assert delete_response.status_code == 200
    assert delete_response.get_json()["deleted"]["id"] == message_id
    assert client.get("/api/messages").get_json() == []


def test_delete_message_returns_not_found_for_unknown_id(client) -> None:
    response = client.delete("/api/messages/9999")

    assert response.status_code == 404
    assert response.get_json()["status"] == "error"
