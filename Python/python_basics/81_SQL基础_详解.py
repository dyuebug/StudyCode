# ============================================
# SQL 基础语法详解 - 超详细注释版
# 日期：2026-06-18（第37天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【核心概念】SQL 和数据库

是什么？
- SQL = Structured Query Language（结构化查询语言）
- 用来操作数据库的标准语言
- SQLite = 轻量级嵌入式数据库，Python 内置

为什么需要？
- 文件存储（JSON）：简单但有限
- 数据库：强大查询、高效索引、并发安全
- 适合：数据量大、复杂查询、多用户

核心特点？
- 声明式语言：说"要什么"，不说"怎么做"
- 标准化：主流数据库通用
- 强大查询：WHERE、JOIN、聚合等
"""

print("""
============================================
SQL 基础语法详解
============================================

今天的学习目标：
1. 理解数据库的概念和优势
2. 掌握 SQL 四大操作（SELECT/INSERT/UPDATE/DELETE）
3. 学会 WHERE 条件查询
4. 了解排序和限制

重要提示：
- SQLite 是 Python 内置的，无需安装
- SQL 语法不区分大小写，但习惯用大写
- 参数化查询防止 SQL 注入攻击
""")

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 什么是数据库？
# --------------------------------------------

print("\n1.1 什么是数据库？")
print("-" * 60)

print("""
【数据库】Database

定义：
- 结构化存储数据的仓库
- 支持高效查询、修改、删除
- 保证数据完整性和一致性

生活例子：图书馆
- 书 = 数据
- 书架 = 表（Table）
- 分类、索引 = 快速查找

数据库 vs 文件（JSON）对比：

┌──────────────┬──────────────┬──────────────┐
│ 特性         │ JSON 文件    │ 数据库       │
├──────────────┼──────────────┼──────────────┤
│ 复杂度       │ 简单         │ 相对复杂     │
│ 查询速度     │ 慢（顺序）   │ 快（索引）   │
│ 复杂查询     │ 困难         │ 容易（SQL）  │
│ 数据量       │ 小（< 1万）  │ 大（> 百万） │
│ 并发安全     │ 差           │ 好           │
│ 数据完整性   │ 无保证       │ 有约束       │
│ 学习成本     │ 低           │ 中等         │
└──────────────┴──────────────┴──────────────┘

什么时候用数据库？
- ✅ 数据量大（> 1000条）
- ✅ 需要复杂查询（搜索、筛选、排序）
- ✅ 多用户并发访问
- ✅ 数据完整性要求高

什么时候用 JSON 文件？
- ✅ 数据量小（< 100条）
- ✅ 简单的增删改查
- ✅ 单用户使用
- ✅ 快速原型开发

留言板项目：
- 现状：100条以内，JSON 够用
- 升级：学习数据库，提升技能
- 实际：根据需求选择
""")

# --------------------------------------------
# 1.2 SQLite 简介
# --------------------------------------------

print("\n1.2 SQLite 简介")
print("-" * 60)

print("""
【SQLite】轻量级数据库

特点：
- 无需安装：Python 内置 sqlite3 模块
- 嵌入式：数据库就是一个文件（.db）
- 零配置：不需要服务器、不需要配置
- 跨平台：Windows、Mac、Linux 通用

vs 其他数据库：
- MySQL、PostgreSQL：客户端-服务器架构，功能强大
- SQLite：单文件、嵌入式、轻量级

适用场景：
- ✅ 桌面应用
- ✅ 移动应用（Android、iOS）
- ✅ 小型 Web 应用
- ✅ 学习和原型开发

限制：
- 不适合高并发写入
- 不适合超大数据量（> 1TB）
- 功能相对简单

留言板项目：完美适配！
""")

# --------------------------------------------
# 1.3 SQL 四大操作（CRUD）⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.3 SQL 四大操作（CRUD）")
print("-" * 60)

print("""
【CRUD】Create、Read、Update、Delete

SQL 语句对应：
┌──────────┬────────┬──────────────────────┐
│ 操作     │ CRUD   │ SQL 语句             │
├──────────┼────────┼──────────────────────┤
│ 查询     │ Read   │ SELECT               │
│ 插入     │ Create │ INSERT INTO          │
│ 更新     │ Update │ UPDATE               │
│ 删除     │ Delete │ DELETE FROM          │
└──────────┴────────┴──────────────────────┘

1. SELECT - 查询数据
   语法：SELECT 字段 FROM 表名 WHERE 条件
   示例：SELECT * FROM messages WHERE id = 1

2. INSERT - 插入数据
   语法：INSERT INTO 表名 (字段) VALUES (值)
   示例：INSERT INTO messages (name, content) VALUES ('Alice', 'Hello')

3. UPDATE - 更新数据
   语法：UPDATE 表名 SET 字段=值 WHERE 条件
   示例：UPDATE messages SET content = 'Hi' WHERE id = 1

4. DELETE - 删除数据
   语法：DELETE FROM 表名 WHERE 条件
   示例：DELETE FROM messages WHERE id = 1

重点：
- SQL 不区分大小写，但习惯关键字用大写
- 每条语句以分号 ; 结尾（Python 中可省略）
- * 表示所有字段
- WHERE 条件很重要，避免误操作全部数据
""")

# ============================================
# 第二部分：SQL 语法详解
# ============================================

print("\n" + "=" * 60)
print("第二部分：SQL 语法详解")
print("=" * 60)

# --------------------------------------------
# 2.1 SELECT - 查询数据 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n2.1 SELECT - 查询数据")
print("-" * 60)

print("""
【SELECT】查询语句

基本语法：
SELECT 字段列表 FROM 表名

示例：

1. 查询所有字段：
   SELECT * FROM messages

2. 查询指定字段：
   SELECT name, content FROM messages

3. 查询并重命名：
   SELECT name AS 姓名, content AS 内容 FROM messages

4. 查询唯一值：
   SELECT DISTINCT name FROM messages

5. 查询数量：
   SELECT COUNT(*) FROM messages

查询结果：
- 返回：表格（行和列）
- 行：每条记录
- 列：每个字段

Python 中获取结果：
- fetchall()：所有结果（列表）
- fetchone()：一条结果（元组）
- fetchmany(n)：n 条结果
""")

# --------------------------------------------
# 2.2 INSERT - 插入数据 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n2.2 INSERT - 插入数据")
print("-" * 60)

print("""
【INSERT】插入语句

基本语法：
INSERT INTO 表名 (字段1, 字段2, ...) VALUES (值1, 值2, ...)

示例：

1. 插入完整数据：
   INSERT INTO messages (name, content, time)
   VALUES ('Alice', 'Hello', '2026-06-18 10:00:00')

2. 插入部分字段（其他为默认值）：
   INSERT INTO messages (name, content)
   VALUES ('Bob', 'Hi')

3. 插入多条（某些数据库支持）：
   INSERT INTO messages (name, content) VALUES
   ('Alice', 'Hello'),
   ('Bob', 'Hi'),
   ('Charlie', 'Hey')

参数化插入（Python）：
cursor.execute(
    "INSERT INTO messages (name, content) VALUES (?, ?)",
    ('Alice', 'Hello')
)

注意：
- 字段顺序要和值顺序一致
- 字符串用单引号
- 主键自增可以不插入
""")

# --------------------------------------------
# 2.3 UPDATE - 更新数据 ⭐⭐⭐⭐
# --------------------------------------------

print("\n2.3 UPDATE - 更新数据")
print("-" * 60)

print("""
【UPDATE】更新语句

基本语法：
UPDATE 表名 SET 字段1=值1, 字段2=值2 WHERE 条件

示例：

1. 更新单个字段：
   UPDATE messages SET content = 'New content' WHERE id = 1

2. 更新多个字段：
   UPDATE messages SET name = 'NewName', content = 'NewContent' WHERE id = 1

3. 更新所有记录（危险！）：
   UPDATE messages SET content = 'Same'
   -- 没有 WHERE，所有记录都被更新

⚠️ 警告：
- 必须加 WHERE 条件，否则更新所有记录
- 先 SELECT 确认要更新哪些，再 UPDATE

安全做法：
1. SELECT * FROM messages WHERE id = 1  -- 先查
2. UPDATE messages SET content = 'New' WHERE id = 1  -- 再改
""")

# --------------------------------------------
# 2.4 DELETE - 删除数据 ⭐⭐⭐⭐
# --------------------------------------------

print("\n2.4 DELETE - 删除数据")
print("-" * 60)

print("""
【DELETE】删除语句

基本语法：
DELETE FROM 表名 WHERE 条件

示例：

1. 删除单条：
   DELETE FROM messages WHERE id = 1

2. 删除多条（符合条件）：
   DELETE FROM messages WHERE name = 'Alice'

3. 删除所有（危险！）：
   DELETE FROM messages
   -- 没有 WHERE，所有记录都被删除

⚠️ 警告：
- 必须加 WHERE 条件，否则删除所有记录
- 删除无法撤销，要谨慎

安全做法：
1. SELECT * FROM messages WHERE id = 1  -- 先查
2. DELETE FROM messages WHERE id = 1  -- 再删

vs TRUNCATE（某些数据库）：
- DELETE：逐行删除，可回滚
- TRUNCATE：清空表，快但不可回滚
""")

# ============================================
# 第三部分：WHERE 条件查询 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第三部分：WHERE 条件查询")
print("=" * 60)

# --------------------------------------------
# 3.1 比较运算符
# --------------------------------------------

print("\n3.1 比较运算符")
print("-" * 60)

print("""
【WHERE 条件】筛选数据

基本运算符：
- =    等于
- !=   不等于（也可以用 <>）
- >    大于
- <    小于
- >=   大于等于
- <=   小于等于

示例：

1. 等于：
   SELECT * FROM messages WHERE id = 1

2. 不等于：
   SELECT * FROM messages WHERE name != 'Alice'

3. 大于：
   SELECT * FROM messages WHERE id > 10

4. 范围：
   SELECT * FROM messages WHERE id >= 1 AND id <= 10
   -- 或使用 BETWEEN
   SELECT * FROM messages WHERE id BETWEEN 1 AND 10
""")

# --------------------------------------------
# 3.2 逻辑运算符
# --------------------------------------------

print("\n3.2 逻辑运算符")
print("-" * 60)

print("""
【逻辑运算】组合条件

运算符：
- AND  并且（两个条件都满足）
- OR   或者（至少一个条件满足）
- NOT  非（条件取反）

示例：

1. AND - 同时满足：
   SELECT * FROM messages WHERE id > 5 AND name = 'Alice'

2. OR - 满足任一：
   SELECT * FROM messages WHERE name = 'Alice' OR name = 'Bob'

3. NOT - 取反：
   SELECT * FROM messages WHERE NOT name = 'Alice'

4. 复杂组合（用括号）：
   SELECT * FROM messages
   WHERE (name = 'Alice' OR name = 'Bob') AND id > 5

优先级：NOT > AND > OR
建议：用括号明确优先级
""")

# --------------------------------------------
# 3.3 模糊查询 LIKE
# --------------------------------------------

print("\n3.3 模糊查询 LIKE")
print("-" * 60)

print("""
【LIKE】模糊匹配

通配符：
- %    匹配任意多个字符（包括0个）
- _    匹配单个字符

示例：

1. 以某字符开头：
   SELECT * FROM messages WHERE content LIKE 'Hello%'
   -- 匹配：Hello、Hello World、Hello!

2. 以某字符结尾：
   SELECT * FROM messages WHERE content LIKE '%world'
   -- 匹配：Hello world、world

3. 包含某字符：
   SELECT * FROM messages WHERE content LIKE '%Python%'
   -- 匹配：I love Python、Python is great

4. 匹配单个字符：
   SELECT * FROM messages WHERE name LIKE 'A__e'
   -- 匹配：Anne（4个字符，A开头e结尾）

应用场景：
- 搜索功能
- 模糊匹配
- 内容筛选
""")

# ============================================
# 第四部分：排序和限制
# ============================================

print("\n" + "=" * 60)
print("第四部分：排序和限制")
print("=" * 60)

# --------------------------------------------
# 4.1 ORDER BY - 排序
# --------------------------------------------

print("\n4.1 ORDER BY - 排序")
print("-" * 60)

print("""
【ORDER BY】排序结果

语法：
SELECT * FROM 表名 ORDER BY 字段 [ASC|DESC]

- ASC：升序（默认）
- DESC：降序

示例：

1. 按 id 升序：
   SELECT * FROM messages ORDER BY id ASC
   -- 或简写
   SELECT * FROM messages ORDER BY id

2. 按 id 降序：
   SELECT * FROM messages ORDER BY id DESC

3. 按时间降序（最新的在前）：
   SELECT * FROM messages ORDER BY time DESC

4. 多字段排序：
   SELECT * FROM messages ORDER BY name ASC, id DESC
   -- 先按 name 升序，name 相同时按 id 降序

留言板应用：
- 最新留言在前：ORDER BY time DESC
- 按 id 顺序：ORDER BY id ASC
""")

# --------------------------------------------
# 4.2 LIMIT - 限制数量
# --------------------------------------------

print("\n4.2 LIMIT - 限制数量")
print("-" * 60)

print("""
【LIMIT】限制返回数量

语法：
SELECT * FROM 表名 LIMIT 数量 [OFFSET 偏移量]

示例：

1. 获取前10条：
   SELECT * FROM messages LIMIT 10

2. 获取最新10条：
   SELECT * FROM messages ORDER BY time DESC LIMIT 10

3. 分页查询（第2页，每页10条）：
   SELECT * FROM messages LIMIT 10 OFFSET 10
   -- OFFSET 10：跳过前10条
   -- LIMIT 10：取10条

4. 简写分页：
   SELECT * FROM messages LIMIT 10, 10
   -- 等价于 LIMIT 10 OFFSET 10

分页公式：
- 页码：page（从1开始）
- 每页数量：page_size
- OFFSET = (page - 1) * page_size
- LIMIT = page_size

示例：第3页，每页20条
OFFSET = (3 - 1) * 20 = 40
SELECT * FROM messages LIMIT 20 OFFSET 40
""")

# ============================================
# 第五部分：参数化查询（防 SQL 注入）⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第五部分：参数化查询（防 SQL 注入）")
print("=" * 60)

print("""
【SQL 注入】安全问题

什么是 SQL 注入？
- 用户输入的数据包含恶意 SQL 代码
- 拼接字符串导致执行意外的 SQL
- 可能导致数据泄露、删除、篡改

❌ 危险示例：
name = input("输入姓名：")
# 用户输入：Alice' OR '1'='1
sql = f"SELECT * FROM messages WHERE name = '{name}'"
# 实际执行：SELECT * FROM messages WHERE name = 'Alice' OR '1'='1'
# 结果：OR '1'='1' 永远为真，返回所有数据

更危险的输入：
# 用户输入：'; DROP TABLE messages; --
# 实际执行：SELECT * FROM messages WHERE name = ''; DROP TABLE messages; --'
# 结果：表被删除！

✅ 安全做法：参数化查询
cursor.execute(
    "SELECT * FROM messages WHERE name = ?",
    (name,)  # 参数用元组
)

原理：
- ? 是占位符
- 数据库自动转义特殊字符
- SQL 语句和数据分离
- 用户输入不会被当作 SQL 代码执行

Python sqlite3 使用：
# 单个参数（注意元组的逗号）
cursor.execute("SELECT * FROM messages WHERE id = ?", (id,))

# 多个参数
cursor.execute(
    "INSERT INTO messages (name, content) VALUES (?, ?)",
    (name, content)
)

# 命名参数（推荐：可读性好）
cursor.execute(
    "INSERT INTO messages (name, content) VALUES (:name, :content)",
    {"name": name, "content": content}
)

记住：永远使用参数化查询，不要拼接 SQL 字符串！
""")

# ============================================
# 第六部分：总结
# ============================================

print("\n" + "=" * 60)
print("第六部分：总结")
print("=" * 60)

print("""
核心知识点回顾：

1. 数据库概念
   - 结构化存储、高效查询、数据完整性
   - SQLite：轻量级、嵌入式、Python 内置

2. SQL 四大操作
   - SELECT：查询
   - INSERT：插入
   - UPDATE：更新
   - DELETE：删除

3. WHERE 条件
   - 比较：=、!=、>、<、>=、<=
   - 逻辑：AND、OR、NOT
   - 模糊：LIKE、%、_

4. 排序和限制
   - ORDER BY：排序（ASC、DESC）
   - LIMIT：限制数量
   - OFFSET：跳过数量（分页）

5. 参数化查询（重要！）
   - 使用 ? 占位符
   - 防止 SQL 注入
   - 数据和 SQL 分离

留言板应用：
- 查询所有：SELECT * FROM messages ORDER BY time DESC
- 插入留言：INSERT INTO messages (name, content, time) VALUES (?, ?, ?)
- 删除留言：DELETE FROM messages WHERE id = ?

记忆口诀：
- 增删改查：INSERT、DELETE、UPDATE、SELECT
- 条件排序：WHERE、ORDER BY、LIMIT
- 安全第一：参数化查询，防注入
""")

print("\n" + "=" * 60)
print("恭喜！你已经掌握了 SQL 基础语法！")
print("接下来：学习 Python sqlite3 模块，实战操作数据库。")
print("=" * 60)

