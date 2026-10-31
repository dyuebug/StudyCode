"""第 48 天练习：Flask 应用工厂与请求生命周期。

与 `forum_project/app.py` 的教学差异：
- 只保留工厂、配置覆盖、`g`、`teardown_appcontext` 和两个只读路由。
- 不连接数据库，便于单独运行和测试。

运行：
    python practice_projects/day48_app_factory/app.py
"""

from __future__ import annotations

import uuid
from typing import Any

from flask import Flask, g, jsonify


def create_app(test_config: dict[str, Any] | None = None) -> Flask:
    """创建应用；测试可通过 test_config 覆盖配置。"""
    application = Flask(__name__)
    application.config.from_mapping(
        APP_NAME="day48-app-factory",
        PER_PAGE=10,
        REQUEST_HEADER="X-Request-Id",
    )
    if test_config:
        # 必须放在默认值之后，否则会被默认值覆盖。
        application.config.update(test_config)

    @application.before_request
    def assign_request_id() -> None:
        g.request_id = str(uuid.uuid4())
        g.events = []

    @application.teardown_appcontext
    def release_request_state(exception: BaseException | None = None) -> None:
        # 演示资源释放；真实项目在这里关闭数据库连接或文件句柄。
        g.pop("events", None)

    @application.get("/health")
    def health() -> Any:
        return jsonify(
            {
                "status": "ok",
                "app": application.config["APP_NAME"],
                "request_id": g.request_id,
                "per_page": application.config["PER_PAGE"],
            }
        )

    @application.get("/echo/<name>")
    def echo(name: str) -> Any:
        return jsonify({"name": name, "request_id": g.request_id})

    @application.after_request
    def add_request_header(response):
        response.headers[application.config["REQUEST_HEADER"]] = g.get("request_id", "")
        return response

    return application


if __name__ == "__main__":
    create_app().run(host="127.0.0.1", port=5007, debug=False)
