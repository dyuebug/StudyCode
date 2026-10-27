"""留言板项目 v4.0：Flask API + SQLite 持久化。

运行：
    python app_v4_sqlite.py

默认地址：http://localhost:5000
数据库文件：当前文件同目录下的 messages.db
"""

from __future__ import annotations

import sqlite3
from datetime import datetime
from pathlib import Path
from typing import Any

from flask import Flask, jsonify, request
from flask_cors import CORS

PROJECT_DIRECTORY = Path(__file__).resolve().parent
DEFAULT_DATABASE_PATH = PROJECT_DIRECTORY / "messages.db"


def create_app(database_path: str | Path = DEFAULT_DATABASE_PATH) -> Flask:
    """创建留言板应用，并初始化指定的 SQLite 数据库。"""
    application = Flask(__name__)
    CORS(application)
    application.config["DATABASE_PATH"] = str(Path(database_path).resolve())
    initialize_database(application.config["DATABASE_PATH"])

    @application.get("/")
    def get_api_description() -> Any:
        return jsonify(
            {
                "message": "留言板 API 服务（SQLite v4.0）",
                "version": "4.0",
                "database": application.config["DATABASE_PATH"],
                "endpoints": {
                    "获取留言": "GET /api/messages",
                    "提交留言": "POST /api/messages",
                    "删除留言": "DELETE /api/messages/<id>",
                },
            }
        )

    @application.get("/api/messages")
    def get_messages() -> Any:
        with open_database(application.config["DATABASE_PATH"]) as connection:
            message_rows = connection.execute(
                """
                SELECT id, name, content, time
                FROM messages
                ORDER BY id DESC
                """
            ).fetchall()

        return jsonify([dict(message_row) for message_row in message_rows])

    @application.post("/api/messages")
    def add_message() -> Any:
        request_data = request.get_json(silent=True)
        if not isinstance(request_data, dict):
            return jsonify({"status": "error", "message": "请求体必须是 JSON 对象"}), 400

        name = str(request_data.get("name", "匿名")).strip() or "匿名"
        content = str(request_data.get("content", "")).strip()
        if not content:
            return jsonify({"status": "error", "message": "留言内容不能为空"}), 400

        created_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        with open_database(application.config["DATABASE_PATH"]) as connection:
            cursor = connection.execute(
                """
                INSERT INTO messages (name, content, time)
                VALUES (?, ?, ?)
                """,
                (name, content, created_time),
            )
            message_id = cursor.lastrowid
            connection.commit()
            message_row = connection.execute(
                """
                SELECT id, name, content, time
                FROM messages
                WHERE id = ?
                """,
                (message_id,),
            ).fetchone()

        return jsonify({"status": "success", "message": dict(message_row)}), 201

    @application.delete("/api/messages/<int:message_id>")
    def delete_message(message_id: int) -> Any:
        with open_database(application.config["DATABASE_PATH"]) as connection:
            deleted_row = connection.execute(
                """
                SELECT id, name, content, time
                FROM messages
                WHERE id = ?
                """,
                (message_id,),
            ).fetchone()

            if deleted_row is None:
                return jsonify(
                    {"status": "error", "message": f"留言 id={message_id} 不存在"}
                ), 404

            connection.execute("DELETE FROM messages WHERE id = ?", (message_id,))
            connection.commit()

        return jsonify({"status": "success", "deleted": dict(deleted_row)})

    return application


def initialize_database(database_path: str | Path) -> None:
    """创建数据库表；已存在的表不会被覆盖。"""
    with open_database(database_path) as connection:
        connection.execute(
            """
            CREATE TABLE IF NOT EXISTS messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                content TEXT NOT NULL,
                time TEXT NOT NULL
            )
            """
        )
        connection.commit()


def open_database(database_path: str | Path) -> sqlite3.Connection:
    """打开 SQLite 连接并启用行对象，便于转换为 JSON。"""
    database_file = Path(database_path)
    database_file.parent.mkdir(parents=True, exist_ok=True)
    connection = sqlite3.connect(database_file)
    connection.row_factory = sqlite3.Row
    return connection


if __name__ == "__main__":
    application = create_app()
    print("留言板 SQLite v4.0 服务启动中...")
    print("访问地址：http://localhost:5000")
    print(f"数据库文件：{DEFAULT_DATABASE_PATH}")
    application.run(debug=True, port=5000)
