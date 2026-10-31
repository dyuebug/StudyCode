"""留言板 v1～v3 的 API 回归测试。

覆盖范围：
- v1：内存版，预置数据、提交、空内容校验、默认昵称。
- v2：JSON 持久化版，首次启动空数据、写入文件、重启后计算 next_id、损坏文件恢复。
- v3：在 v2 基础上增加 DELETE，验证删除成功、404、文件同步。

这些测试只读取或临时写入 pytest 的 tmp_path，不会修改仓库内的 messages.json。
"""

from __future__ import annotations

import importlib.util
import itertools
import json
import sys
from pathlib import Path
from types import ModuleType

import pytest

MODULE_DIRECTORY = Path(__file__).resolve().parent
_module_counter = itertools.count(1)


def load_module(filename: str) -> ModuleType:
    """按文件路径加载模块，避免与仓库中其他 app.py 重名。"""
    path = MODULE_DIRECTORY / filename
    name = f"message_board_test_{next(_module_counter)}"
    spec = importlib.util.spec_from_file_location(name, path)
    assert spec is not None and spec.loader is not None, f"无法加载 {path}"
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def reset_v1(module: ModuleType) -> None:
    module.messages = [
        {"id": 1, "name": "小明", "content": "第一条", "time": "2026-06-14 09:00:00"},
        {"id": 2, "name": "小红", "content": "第二条", "time": "2026-06-14 09:30:00"},
    ]
    module.next_id = 3
    module.app.config.update(TESTING=True)


@pytest.fixture()
def client_v1():
    module = load_module("app.py")
    reset_v1(module)
    return module, module.app.test_client()


@pytest.fixture()
def v2_workspace(tmp_path, monkeypatch):
    """把工作目录切到临时目录，使相对路径 messages.json 落在 tmp_path。"""
    monkeypatch.chdir(tmp_path)
    module = load_module("app_v2_持久化版.py")
    module.messages = []
    module.next_id = 1
    module.app.config.update(TESTING=True)
    return module, tmp_path


@pytest.fixture()
def v3_workspace(tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)
    module = load_module("app_v3_完整版.py")
    module.messages = []
    module.next_id = 1
    module.app.config.update(TESTING=True)
    return module, tmp_path


# ---------------------------------------------------------------- v1 内存版


def test_v1_returns_seed_messages(client_v1):
    _, client = client_v1
    response = client.get("/api/messages")

    assert response.status_code == 200
    payload = response.get_json()
    assert [item["id"] for item in payload] == [1, 2]


def test_v1_add_message_increments_id(client_v1):
    module, client = client_v1

    response = client.post("/api/messages", json={"name": "测试", "content": "你好"})

    assert response.status_code == 200
    body = response.get_json()
    assert body["status"] == "success"
    assert body["message"]["id"] == 3
    assert body["message"]["content"] == "你好"
    assert module.next_id == 4
    assert len(module.messages) == 3


def test_v1_rejects_blank_content(client_v1):
    module, client = client_v1
    original_count = len(module.messages)

    response = client.post("/api/messages", json={"name": "测试", "content": "   "})

    assert response.status_code == 400
    assert response.get_json()["status"] == "error"
    assert len(module.messages) == original_count


def test_v1_defaults_to_anonymous(client_v1):
    _, client = client_v1

    response = client.post("/api/messages", json={"content": "没有昵称"})

    assert response.status_code == 200
    assert response.get_json()["message"]["name"] == "匿名"


# ---------------------------------------------------------------- v2 持久化版


def test_v2_starts_empty_without_file(v2_workspace):
    module, workspace = v2_workspace

    module.load_messages()

    assert module.messages == []
    assert module.next_id == 1
    assert not (workspace / "messages.json").exists()


def test_v2_persists_new_message(v2_workspace):
    module, workspace = v2_workspace
    client = module.app.test_client()

    response = client.post("/api/messages", json={"name": "小明", "content": "持久化内容"})

    assert response.status_code == 200
    data_file = workspace / "messages.json"
    assert data_file.is_file()

    saved = json.loads(data_file.read_text(encoding="utf-8"))
    assert saved == [
        {
            "id": 1,
            "name": "小明",
            "content": "持久化内容",
            "time": saved[0]["time"],
        }
    ]

    # ensure_ascii=False：中文必须以原字符写入，而不是 \uXXXX
    assert "持久化内容" in data_file.read_text(encoding="utf-8")


def test_v2_load_recalculates_next_id(v2_workspace):
    module, workspace = v2_workspace
    (workspace / "messages.json").write_text(
        json.dumps(
            [
                {"id": 4, "name": "A", "content": "a", "time": "2026-06-15 10:00:00"},
                {"id": 9, "name": "B", "content": "b", "time": "2026-06-15 10:01:00"},
            ],
            ensure_ascii=False,
        ),
        encoding="utf-8",
    )

    module.load_messages()

    assert len(module.messages) == 2
    assert module.next_id == 10


def test_v2_recovers_from_corrupted_json(v2_workspace):
    module, workspace = v2_workspace
    (workspace / "messages.json").write_text("{not valid json", encoding="utf-8")

    module.load_messages()

    assert module.messages == []
    assert module.next_id == 1


# ---------------------------------------------------------------- v3 完整版


def test_v3_delete_existing_message(v3_workspace):
    module, workspace = v3_workspace
    client = module.app.test_client()
    client.post("/api/messages", json={"name": "A", "content": "待删除"})

    response = client.delete("/api/messages/1")

    assert response.status_code == 200
    assert response.get_json()["status"] == "success"
    assert module.messages == []

    saved = json.loads((workspace / "messages.json").read_text(encoding="utf-8"))
    assert saved == []


def test_v3_delete_missing_message_returns_404(v3_workspace):
    module, _ = v3_workspace
    client = module.app.test_client()

    response = client.delete("/api/messages/999")

    assert response.status_code == 404
    assert response.get_json()["status"] == "error"


def test_v3_keeps_remaining_messages_after_delete(v3_workspace):
    module, workspace = v3_workspace
    client = module.app.test_client()
    client.post("/api/messages", json={"name": "A", "content": "保留"})
    client.post("/api/messages", json={"name": "B", "content": "删除"})

    client.delete("/api/messages/2")

    remaining = json.loads((workspace / "messages.json").read_text(encoding="utf-8"))
    assert [item["content"] for item in remaining] == ["保留"]
