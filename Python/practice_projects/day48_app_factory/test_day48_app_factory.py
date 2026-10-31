"""第 48 天练习的测试：工厂、配置覆盖、请求 id 与响应头。"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

import pytest

MODULE_PATH = Path(__file__).with_name("app.py")


@pytest.fixture()
def application():
    spec = importlib.util.spec_from_file_location("day48_app", MODULE_PATH)
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module.create_app({"TESTING": True, "PER_PAGE": 3})


def test_health_uses_overridden_config(application) -> None:
    response = application.test_client().get("/health")

    assert response.status_code == 200
    assert response.get_json()["per_page"] == 3
    assert response.get_json()["status"] == "ok"


def test_two_requests_get_different_ids(application) -> None:
    client = application.test_client()

    first = client.get("/health").get_json()["request_id"]
    second = client.get("/health").get_json()["request_id"]

    assert first != second


def test_request_id_is_returned_in_header(application) -> None:
    response = application.test_client().get("/echo/python")

    assert response.status_code == 200
    assert response.headers["X-Request-Id"] == response.get_json()["request_id"]


def test_default_config_is_not_mutated_between_apps() -> None:
    factory = sys.modules["day48_app"]
    first = factory.create_app({"TESTING": True, "PER_PAGE": 1})
    second = factory.create_app({"TESTING": True})

    assert first.config["PER_PAGE"] == 1
    assert second.config["PER_PAGE"] == 10
