# 留言板项目 v4.0：SQLite 持久化版

## 项目状态

v4.0 已提供独立 Flask 应用实现，使用 SQLite 替代 JSON 文件保存留言。数据库文件会在首次运行时自动创建，默认位置为当前目录下的 `messages.db`。

历史版本仍然保留，便于对比数据存储方案：

- `app.py`：v1.0，内存存储。
- `app_v2_持久化版.py`：v2.0，JSON 文件持久化。
- `app_v3_完整版.py`：v3.0，JSON 持久化加 RESTful DELETE 接口。
- `app_v4_sqlite.py`：v4.0，SQLite 持久化和完整 CRUD API。

## 运行方式

在项目目录执行：

```bash
pip install -r ../../requirements.txt
python app_v4_sqlite.py
```

服务地址：<http://localhost:5000>

## API 接口

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| `GET` | `/api/messages` | 按 id 倒序获取全部留言 |
| `POST` | `/api/messages` | 创建留言，JSON 请求体为 `{ "name": "姓名", "content": "内容" }` |
| `DELETE` | `/api/messages/<id>` | 删除指定留言 |

创建成功返回 HTTP `201`。内容为空、请求体不是 JSON 对象时返回 HTTP `400`。删除不存在的留言返回 HTTP `404`。

## 数据表

```sql
CREATE TABLE messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    content TEXT NOT NULL,
    time TEXT NOT NULL
);
```

所有用户输入通过 SQLite 参数化查询写入，避免把输入内容直接拼接到 SQL 语句中。

## 测试

在项目目录执行：

```bash
pytest test_app_v4_sqlite.py -q
```

测试覆盖：

- 初始查询返回空列表。
- 创建留言并再次查询，验证持久化结果。
- 拒绝空留言。
- 删除已有留言。
- 删除不存在的留言返回 `404`。

测试使用临时数据库，不会修改项目目录中的 `messages.db`。
