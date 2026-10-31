"""SQLite 连接与初始化。

设计要点：
1. 每个请求复用同一个连接，请求结束时由 Flask 关闭。
2. 外键约束需要显式开启，SQLite 默认关闭。
3. 建表脚本使用 IF NOT EXISTS，重复执行不会破坏数据。
"""

from __future__ import annotations

import sqlite3
from pathlib import Path

from flask import current_app, g

SCHEMA_PATH = Path(__file__).with_name("schema.sql")


def get_db() -> sqlite3.Connection:
    """取得当前请求的数据库连接。"""
    if "db" not in g:
        connection = sqlite3.connect(current_app.config["DATABASE"])
        connection.row_factory = sqlite3.Row
        connection.execute("PRAGMA foreign_keys = ON")
        g.db = connection
    return g.db


def close_db(exception: BaseException | None = None) -> None:
    """请求结束时关闭连接，异常由 Flask 继续向上抛出。"""
    connection = g.pop("db", None)
    if connection is not None:
        connection.close()


def init_db() -> None:
    """执行建表脚本并写入初始分类。"""
    connection = get_db()
    connection.executescript(SCHEMA_PATH.read_text(encoding="utf-8"))
    connection.commit()


def init_app(application) -> None:
    """注册数据库生命周期钩子，并确保表结构存在。"""
    application.teardown_appcontext(close_db)
    with application.app_context():
        init_db()
