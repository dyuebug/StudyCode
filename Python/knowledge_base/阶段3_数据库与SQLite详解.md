# 阶段 3：SQL 与 SQLite 详解

> **整理日期**：2026-10-30
> **数据范围**：`python_basics/81_SQL基础_详解.py`、`82_SQLite数据库_详解.py`、`python_basics/留言板项目/app_v4_sqlite.py`、`forum_project/db.py`、`forum_project/schema.sql`。
> **状态说明**：论坛与留言板 v4 有实际代码与测试；本笔记中的 SQL 片段可在 `sqlite3` 中直接执行。
> **与代码冲突时**：以 `schema.sql`、`db.py` 和测试结果为准。

---

## 1. 关系模型与建表

```sql
CREATE TABLE IF NOT EXISTS posts (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id     INTEGER NOT NULL,
    category_id INTEGER NOT NULL,
    title       TEXT    NOT NULL,
    content     TEXT    NOT NULL,
    created_at  TEXT    NOT NULL DEFAULT (datetime('now', 'localtime')),
    FOREIGN KEY (user_id)     REFERENCES users (id)      ON DELETE CASCADE,
    FOREIGN KEY (category_id) REFERENCES categories (id) ON DELETE RESTRICT
);
```

- `PRIMARY KEY` 唯一标识一行；`AUTOINCREMENT` 保证 id 单调递增且不复用，但会增加额外开销。
- `NOT NULL` 是数据库层约束，应用层校验不能替代它。
- `DEFAULT` 在未提供值时生效；`datetime('now', 'localtime')` 使用本地时间，跨时区部署时应改用 UTC。
- 外键约束只有在连接上执行 `PRAGMA foreign_keys = ON` 后才生效；SQLite 默认关闭。
- `ON DELETE CASCADE` 删除父记录时级联删除子记录；`RESTRICT` 阻止删除仍被引用的父记录。

---

## 2. 查询基础

```sql
-- 条件、排序、分页
SELECT id, title, created_at
FROM posts
WHERE category_id = ?
ORDER BY created_at DESC, id DESC
LIMIT ? OFFSET ?;

-- 计数
SELECT COUNT(*) AS total FROM posts WHERE user_id = ?;
```

- `ORDER BY` 必须稳定：`created_at` 相同时用 `id` 作为第二排序键，否则分页可能重复或遗漏。
- 分页使用 `LIMIT ... OFFSET ...`；深分页时 OFFSET 越大越慢，可改用“上一页最后一条的 id”作为游标。
- `COUNT(*)` 统计行数；`COUNT(column)` 忽略 `NULL`。

### 2.1 聚合与分组

```sql
SELECT category_id, COUNT(*) AS post_count
FROM posts
GROUP BY category_id
HAVING COUNT(*) > 1
ORDER BY post_count DESC;
```

执行顺序：`FROM` → `WHERE` → `GROUP BY` → `HAVING` → `SELECT` → `ORDER BY` → `LIMIT`。`WHERE` 过滤行，`HAVING` 过滤分组。

### 2.2 连接

```sql
SELECT p.id, p.title, u.username, c.name AS category
FROM posts AS p
JOIN users AS u      ON u.id = p.user_id
JOIN categories AS c ON c.id = p.category_id
WHERE p.id = ?;
```

| 类型 | 结果 |
|---|---|
| `INNER JOIN` | 只保留两边都匹配的行 |
| `LEFT JOIN` | 保留左表全部行，右表无匹配时为 `NULL` |
| `CROSS JOIN` | 笛卡尔积，慎用 |

多表连接时给表起别名，并在每个列前写别名，避免同名列歧义。

---

## 3. 写入、更新与删除

```sql
INSERT INTO messages (name, content, time) VALUES (?, ?, ?);

UPDATE messages
SET content = ?
WHERE id = ?;

DELETE FROM messages WHERE id = ?;
```

- 始终使用参数绑定（`?` 占位符），不要拼接用户输入，否则会产生 SQL 注入。
- `UPDATE`/`DELETE` 忘记 `WHERE` 会作用于全表；执行前先用同条件 `SELECT` 确认范围。
- `cursor.rowcount` 可判断实际影响行数；删除不存在的 id 时返回 0，可据此返回 404。
- `INSERT OR IGNORE` 在唯一约束冲突时跳过；`INSERT OR REPLACE` 会先删除旧行再插入，可能触发级联删除，慎用。

```python
cursor = connection.execute(
    "INSERT INTO messages (name, content, time) VALUES (?, ?, ?)",
    (name, content, created_time),
)
message_id = cursor.lastrowid
connection.commit()
```

---

## 4. 索引与查询计划

`schema.sql` 中的索引：

```sql
CREATE INDEX IF NOT EXISTS idx_posts_created_at ON posts (created_at DESC);
CREATE INDEX IF NOT EXISTS idx_posts_category   ON posts (category_id);
CREATE INDEX IF NOT EXISTS idx_replies_post     ON replies (post_id);
```

- 索引加速 `WHERE`、`JOIN`、`ORDER BY`，但会增加写入成本和存储占用。
- 复合索引遵循最左前缀原则：`(category_id, created_at)` 能服务 `category_id` 和 `category_id + created_at`，不能单独服务 `created_at`。
- 低选择性列（如布尔值）单独建索引收益有限。
- 用 `EXPLAIN QUERY PLAN` 查看是否走索引：

  ```sql
  EXPLAIN QUERY PLAN
  SELECT * FROM posts WHERE category_id = 1 ORDER BY created_at DESC;
  ```

  出现 `USING INDEX` 表示使用索引；出现 `SCAN` 表示全表扫描。

- 外键列通常需要索引，否则删除父记录时的级联检查可能变慢。

---

## 5. 事务

```python
with connection:
    connection.execute("UPDATE accounts SET balance = balance - ? WHERE id = ?", (100, 1))
    connection.execute("UPDATE accounts SET balance = balance + ? WHERE id = ?", (100, 2))
```

- `with connection:` 在成功时提交，异常时回滚；比手动 `commit`/`rollback` 更不易漏。
- 转账等操作必须放在同一事务中，保证要么全部成功、要么全部失败。
- SQLite 默认自动提交模式；Python 的 `sqlite3` 在 DML 后会隐式开启事务，需要显式提交。
- 长时间事务会持有锁，阻塞其他写入；事务内不要做网络请求或耗时计算。

### 5.1 并发与锁

- SQLite 同一时刻只允许一个写入者；多进程并发写会出现 `database is locked`。
- 设置 `timeout` 让连接在锁释放前等待：

  ```python
  sqlite3.connect(path, timeout=5)
  ```

- WAL 模式允许读写并发，适合读多写少的 Web 应用：

  ```sql
  PRAGMA journal_mode = WAL;
  ```

- 生产部署常采用“单 worker + 多线程”，让写入在进程内串行化，参见 `knowledge_base/阶段5_Linux与Docker部署详解.md`。

---

## 6. Python `sqlite3` API

```python
import sqlite3
from pathlib import Path

connection = sqlite3.connect("messages.db")
connection.row_factory = sqlite3.Row
connection.execute("PRAGMA foreign_keys = ON")

row = connection.execute(
    "SELECT id, name, content FROM messages WHERE id = ?", (1,)
).fetchone()

if row is not None:
    print(row["name"])      # 按列名访问
    print(dict(row))        # 转为普通字典

connection.close()
```

- `row_factory = sqlite3.Row` 支持按列名访问，也支持 `dict(row)`。
- `fetchone()` 返回一行或 `None`；`fetchall()` 返回列表；`fetchmany(n)` 分批读取。
- 连接、游标、结果集都应关闭；Flask 项目用 `teardown_appcontext` 统一关闭。
- `executescript()` 执行多条语句，适合初始化脚本；它不会自动参数化，不要传入用户输入。

### 6.1 连接生命周期（Flask）

`forum_project/db.py` 的做法：

```python
def get_db():
    if "db" not in g:
        connection = sqlite3.connect(current_app.config["DATABASE"])
        connection.row_factory = sqlite3.Row
        connection.execute("PRAGMA foreign_keys = ON")
        g.db = connection
    return g.db

def close_db(exception=None):
    connection = g.pop("db", None)
    if connection is not None:
        connection.close()

def init_app(application):
    application.teardown_appcontext(close_db)
```

- 每个请求复用同一连接，避免频繁建立连接。
- 请求结束时关闭，防止连接泄漏。
- `g.pop("db", None)` 让重复调用安全。
- 建表脚本使用 `IF NOT EXISTS`，应用启动时可重复执行。

---

## 7. 备份与完整性

```bash
sqlite3 forum.db ".backup 'forum_20261030.db'"
sqlite3 forum.db "PRAGMA integrity_check;"
```

- 不要直接 `cp` 正在写入的数据库文件；`.backup` 会在事务边界生成一致快照。
- `PRAGMA integrity_check;` 返回 `ok` 表示结构完整；损坏时应从备份恢复。
- 恢复前先停止写入（停服务），替换文件后再启动。
- 备份应定期演练恢复，只生成备份文件不等于可恢复。

Python 等价实现：

```python
source = sqlite3.connect("forum.db")
target = sqlite3.connect("backup.db")
with target:
    source.backup(target)
source.close()
target.close()
```

---

## 8. 测试数据库

- 测试使用临时路径，避免污染开发数据库：

  ```python
  def test_create_post(app):
      ...
  ```

- 每个用例独立建库，或在 fixture 中建表、结束后删除。
- 测试断言应覆盖：正常写入、参数缺失、资源不存在、唯一约束冲突、外键约束。
- 事务测试要验证回滚后的状态，而不只是“没有抛异常”。

---

## 9. 常见错误对照

| 现象 | 原因 | 修正 |
|---|---|---|
| `no such table` | 未执行建表脚本，或连接到了错误的文件 | 启动时 `init_db()`，打印实际数据库路径 |
| `FOREIGN KEY constraint failed` | 插入不存在的父记录，或未开启外键 | 检查引用值；连接上执行 `PRAGMA foreign_keys = ON` |
| `UNIQUE constraint failed` | 重复插入唯一列 | 先查询，或用 `INSERT OR IGNORE` 并检查 `rowcount` |
| `database is locked` | 多写入者竞争 | 设置 timeout、减少事务长度、单写入者 |
| `sqlite3.ProgrammingError` | 参数数量与占位符不匹配 | 核对 `?` 数量与元组长度 |
| 中文乱码 | 编码不一致 | 统一 UTF-8；SQLite 文本默认按连接编码处理 |
| 分页重复/遗漏 | 排序不稳定 | 增加唯一列作为第二排序键 |

---

## 10. 对应文件与验证

| 主题 | 文件 |
|---|---|
| SQL 基础 | `python_basics/81_SQL基础_详解.py` |
| SQLite API | `python_basics/82_SQLite数据库_详解.py` |
| 留言板 v4 | `python_basics/留言板项目/app_v4_sqlite.py` |
| 留言板 v4 测试 | `python_basics/留言板项目/test_app_v4_sqlite.py` |
| 论坛表结构 | `forum_project/schema.sql` |
| 论坛连接管理 | `forum_project/db.py` |
| 论坛接口测试 | `forum_project/tests/test_forum.py` |

验证命令：

```bash
python_basics/.venv/Scripts/python.exe -m pytest forum_project/tests -q
python_basics/.venv/Scripts/python.exe -m pytest "python_basics/留言板项目/test_app_v4_sqlite.py" -q
```

---

## 11. 练习（不附答案）

1. 为 `posts` 表写一条查询：按分类统计帖子数，只显示帖子数大于 1 的分类。
2. 用 `LEFT JOIN` 列出所有用户及其帖子数，没有帖子的用户计为 0。
3. 用 `EXPLAIN QUERY PLAN` 比较有无索引时 `WHERE category_id = ?` 的计划差异。
4. 写一个事务：删除帖子及其全部回复，验证回复被级联删除。
5. 用 Python 的 `backup()` 生成备份，再用 `PRAGMA integrity_check` 验证。

---

## 12. 待修正内容

- 未在真实高并发场景压测 WAL 与 `database is locked` 的表现。
- 论坛项目当前使用单连接 + 单 worker 设计；迁移到 PostgreSQL 后，索引与事务细节需重新评估。
