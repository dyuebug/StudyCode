"""第 51 天练习的测试：启动真实 HTTP 服务并请求接口。"""

from __future__ import annotations

import importlib.util
import json
import sys
import threading
import urllib.request
from pathlib import Path

import pytest

MODULE_PATH = Path(__file__).with_name("app.py")


@pytest.fixture(scope="module")
def base_url():
    spec = importlib.util.spec_from_file_location("day51_app", MODULE_PATH)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)

    server = module.make_server(port=0)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    host, port = server.server_address
    try:
        yield f"http://{host}:{port}"
    finally:
        server.shutdown()
        server.server_close()
        thread.join(timeout=2)


def get_json(url: str) -> dict:
    with urllib.request.urlopen(url, timeout=2) as response:
        return json.loads(response.read().decode("utf-8"))


def test_health(base_url: str) -> None:
    assert get_json(f"{base_url}/health") == {"status": "ok"}


def test_posts_shape(base_url: str) -> None:
    payload = get_json(f"{base_url}/api/posts")

    assert payload["status"] == "success"
    assert payload["items"] == []


def test_unknown_path_returns_404(base_url: str) -> None:
    with pytest.raises(urllib.error.HTTPError) as error:
        urllib.request.urlopen(f"{base_url}/missing", timeout=2)

    assert error.value.code == 404
