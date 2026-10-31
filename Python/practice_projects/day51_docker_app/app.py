"""第 51 天练习：可容器化的最小 JSON 服务（仅标准库）。

路由：
- GET /health  健康检查，返回 {"status": "ok"}
- GET /api/posts 返回空列表，用于验证容器与卷挂载

运行：
    PORT=8000 python practice_projects/day51_docker_app/app.py
"""

from __future__ import annotations

import json
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


class Handler(BaseHTTPRequestHandler):
    server_version = "day51-practice/1.0"

    def do_GET(self) -> None:  # noqa: N802 - BaseHTTPRequestHandler 约定
        if self.path == "/health":
            self._send_json({"status": "ok"})
        elif self.path == "/api/posts":
            self._send_json({"items": [], "status": "success"})
        else:
            self._send_json({"status": "error", "message": "not found"}, status=404)

    def _send_json(self, payload: dict, status: int = 200) -> None:
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format: str, *args) -> None:  # noqa: A002
        # 输出到 stderr，便于 docker logs 收集。
        super().log_message(format, *args)


def make_server(host: str = "127.0.0.1", port: int = 8000) -> ThreadingHTTPServer:
    return ThreadingHTTPServer((host, port), Handler)


def main() -> None:
    host = os.environ.get("HOST", "0.0.0.0")
    port = int(os.environ.get("PORT", "8000"))
    server = make_server(host, port)
    print(f"listening on {host}:{port}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
