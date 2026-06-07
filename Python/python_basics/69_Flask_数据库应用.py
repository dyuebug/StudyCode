# ============================================
# Flask + SQLite 学生成绩管理系统 - 超详细注释版
# 日期：2026-06-07（第28天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【核心概念】Flask + SQLite 数据库应用

是什么？
- 将 Flask Web 应用与 SQLite 数据库结合，实现数据持久化
- 数据存储在 .db 文件中，重启服务器后数据依然存在

为什么需要？
- 之前的数据（列表、字典）存在内存里，服务器重启就丢失
- 数据库让数据永久保存，并支持查询、增删改操作

核心特点：
- sqlite3：Python 内置模块，无需安装，适合学习和小型应用
- with sqlite3.connect() as conn：自动提交事务、自动关闭连接
- render_template_string + Jinja2：在 Python 中直接写 HTML 模板
"""

import sqlite3
import os
from flask import Flask, render_template_string, request, redirect, url_for

app = Flask(__name__)

# 数据库文件路径：与本 .py 文件同目录
# os.path.dirname(__file__)：获取本文件所在目录的绝对路径
# os.path.join：拼接目录和文件名，跨平台兼容
DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "students.db")


# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("""
【函数卡片：sqlite3.connect()】

函数名：sqlite3.connect()
作用：连接（或创建）SQLite 数据库文件，返回数据库连接对象
所属：sqlite3 模块（Python 内置）

完整语法：
sqlite3.connect(database)

参数详解：
1. database（必需）
   - 类型：str（文件路径）
   - 作用：指定数据库文件的路径，文件不存在时自动创建
   - 示例：sqlite3.connect("students.db")
            sqlite3.connect("/absolute/path/students.db")
   - 特殊值：":memory:" 表示内存数据库（程序结束即消失）

返回值：Connection 对象（可以执行 SQL 的连接）

推荐用法（with 语句）：
with sqlite3.connect(DB_PATH) as conn:
    cursor = conn.cursor()
    cursor.execute("SQL语句")
    conn.commit()   # with 块结束自动 commit（成功）或 rollback（异常）
→ with 语句确保连接正确关闭，即使发生异常

────────────────────────────────────

【函数卡片：cursor.execute()】

函数名：cursor.execute()
作用：执行一条 SQL 语句

完整语法：
cursor.execute(sql, parameters=())

参数详解：
1. sql（必需）
   - 类型：str
   - 作用：要执行的 SQL 语句，用 ? 作为参数占位符
   - 示例："INSERT INTO students VALUES (?, ?, ?)"

2. parameters（可选）
   - 类型：tuple 或 list
   - 作用：按顺序替换 sql 中的 ? 占位符
   - 示例：(name, score, subject)

⚠️ 重要：永远用 ? 占位符，不要用字符串格式化拼接 SQL！
❌ f"INSERT INTO t VALUES ('{name}')"  → SQL 注入漏洞
✅ cursor.execute("INSERT INTO t VALUES (?)", (name,))

────────────────────────────────────

【函数卡片：cursor.fetchall() / fetchone()】

fetchall()：返回所有查询结果，格式为 [(行1), (行2), ...]
fetchone()：返回第一行结果，格式为 (字段1, 字段2, ...)

conn.row_factory = sqlite3.Row：让结果支持按列名访问
例：row["name"] 而不是 row[0]
""")


# ============================================
# 第二部分：数据库初始化
# ============================================

def init_db():
    """
    初始化数据库：创建表和示例数据
    - 只在表不存在时创建（IF NOT EXISTS），确保重启不重置数据
    - 只在表为空时插入示例数据
    """
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.cursor()

        # CREATE TABLE IF NOT EXISTS：表已存在时不报错，直接跳过
        # 字段说明：
        #   id：整数主键，AUTOINCREMENT 让 SQLite 自动分配唯一 ID
        #   name：文本，NOT NULL 表示不允许为空
        #   python_score / math_score / english_score：整数成绩，默认值 0
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS students (
                id              INTEGER PRIMARY KEY AUTOINCREMENT,
                name            TEXT    NOT NULL,
                python_score    INTEGER NOT NULL DEFAULT 0,
                math_score      INTEGER NOT NULL DEFAULT 0,
                english_score   INTEGER NOT NULL DEFAULT 0
            )
        """)

        # 检查表中是否已有数据，避免重复插入示例数据
        cursor.execute("SELECT COUNT(*) FROM students")
        count = cursor.fetchone()[0]  # fetchone() 返回第一行，[0] 取第一列

        if count == 0:
            # executemany()：一次插入多条记录，比多次 execute() 更高效
            # ? 占位符按顺序对应元组中的值
            sample_data = [
                ("张三", 92, 85, 88),
                ("李四", 78, 90, 76),
                ("王五", 65, 72, 80),
                ("赵六", 95, 88, 92),
                ("孙七", 55, 60, 58),
            ]
            cursor.execute(
                "INSERT INTO students (name, python_score, math_score, english_score) VALUES (?, ?, ?, ?)",
                sample_data[0]
            )
            # executemany：批量插入，参数是列表，每个元素对应一条记录的占位符值
            cursor.executemany(
                "INSERT INTO students (name, python_score, math_score, english_score) VALUES (?, ?, ?, ?)",
                sample_data[1:]
            )
            print(f"✅ 已插入 {len(sample_data)} 条示例数据")
        else:
            print(f"✅ 数据库已存在，共 {count} 条记录")


# ============================================
# 第三部分：HTML 模板定义
# ============================================

# 公共 HTML 结构（用字符串模拟模板继承效果）
def make_page(title, body_html):
    """
    生成带公共导航和样式的完整 HTML 页面
    参数：
      title：页面标题（显示在浏览器标签栏）
      body_html：页面主体内容（HTML 字符串）
    返回：完整的 HTML 字符串
    """
    return f"""
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>{title} - 学生成绩管理系统</title>
  <style>
    body {{ font-family: Arial, sans-serif; max-width: 900px; margin: 0 auto; padding: 20px; background: #f5f5f5; }}
    nav {{ background: #2c3e50; padding: 12px 20px; border-radius: 8px; margin-bottom: 20px; }}
    nav a {{ color: #ecf0f1; text-decoration: none; margin-right: 20px; font-size: 15px; }}
    nav a:hover {{ color: #3498db; }}
    .card {{ background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
    table {{ width: 100%; border-collapse: collapse; }}
    th, td {{ padding: 10px 14px; text-align: left; border-bottom: 1px solid #eee; }}
    th {{ background: #2c3e50; color: white; }}
    tr:hover {{ background: #f9f9f9; }}
    .btn {{ display: inline-block; padding: 7px 16px; border-radius: 5px; text-decoration: none; font-size: 14px; cursor: pointer; border: none; }}
    .btn-primary {{ background: #3498db; color: white; }}
    .btn-danger  {{ background: #e74c3c; color: white; }}
    .btn:hover {{ opacity: 0.85; }}
    input, select {{ padding: 8px; border: 1px solid #ddd; border-radius: 4px; width: 100%; box-sizing: border-box; margin-top: 4px; }}
    .form-group {{ margin-bottom: 15px; }}
    label {{ font-weight: bold; color: #555; }}
    .msg-ok  {{ background: #d5f5e3; border: 1px solid #27ae60; padding: 10px; border-radius: 5px; margin-bottom: 15px; }}
    .msg-err {{ background: #fadbd8; border: 1px solid #e74c3c; padding: 10px; border-radius: 5px; margin-bottom: 15px; }}
  </style>
</head>
<body>
  <nav>
    <a href="/">📋 学生列表</a>
    <a href="/add">➕ 添加学生</a>
    <a href="/api/students">🔌 JSON API</a>
  </nav>
  <div class="card">
    {body_html}
  </div>
</body>
</html>
"""

# 首页：学生成绩列表模板（Jinja2 语法）
LIST_TEMPLATE = """
<h2>📋 学生成绩列表</h2>
<p>共 {{ students|length }} 名学生</p>

{% if students %}
<table>
  <tr>
    <th>序号</th>
    <th>姓名</th>
    <th>Python</th>
    <th>数学</th>
    <th>英语</th>
    <th>平均分</th>
    <th>等级</th>
    <th>操作</th>
  </tr>
  {% for s in students %}
  <tr>
    <!-- loop.index：从 1 开始的序号 -->
    <td>{{ loop.index }}</td>
    <td>{{ s.name }}</td>
    <td>{{ s.python_score }}</td>
    <td>{{ s.math_score }}</td>
    <td>{{ s.english_score }}</td>

    <!-- avg 在 Python 中计算好传入，用 round(1) 过滤器保留1位小数 -->
    <td>{{ s.avg|round(1) }}</td>

    <!-- 根据平均分显示不同颜色的等级标签 -->
    <td>
      {% if s.avg >= 90 %}
        <span style="color:#27ae60;font-weight:bold">优秀</span>
      {% elif s.avg >= 75 %}
        <span style="color:#2980b9;font-weight:bold">良好</span>
      {% elif s.avg >= 60 %}
        <span style="color:#e67e22;font-weight:bold">及格</span>
      {% else %}
        <span style="color:#e74c3c;font-weight:bold">不及格</span>
      {% endif %}
    </td>

    <!-- 删除按钮：点击后 POST 到 /delete/<id> -->
    <td>
      <form method="POST" action="/delete/{{ s.id }}"
            onsubmit="return confirm('确定删除 {{ s.name }}？')">
        <button type="submit" class="btn btn-danger">删除</button>
      </form>
    </td>
  </tr>
  {% endfor %}
</table>
{% else %}
<p>暂无学生数据，<a href="/add">添加第一个学生</a></p>
{% endif %}
"""

# 添加学生表单模板
ADD_TEMPLATE = """
<h2>➕ 添加学生</h2>

<!-- 显示错误信息（如果有） -->
{% if error %}
<div class="msg-err">❌ {{ error }}</div>
{% endif %}

<!-- method="POST"：表单提交方式，数据放在请求体中（不显示在 URL）-->
<form method="POST" action="/add">
  <div class="form-group">
    <label>姓名</label>
    <!-- value="{{ name }}"：保留用户输入，提交失败后不需要重新填写 -->
    <input type="text" name="name" placeholder="请输入姓名" value="{{ name }}" required>
  </div>
  <div class="form-group">
    <label>Python 成绩（0-100）</label>
    <input type="number" name="python_score" min="0" max="100" value="{{ python_score }}" required>
  </div>
  <div class="form-group">
    <label>数学成绩（0-100）</label>
    <input type="number" name="math_score" min="0" max="100" value="{{ math_score }}" required>
  </div>
  <div class="form-group">
    <label>英语成绩（0-100）</label>
    <input type="number" name="english_score" min="0" max="100" value="{{ english_score }}" required>
  </div>
  <button type="submit" class="btn btn-primary">保存学生</button>
  <a href="/" class="btn" style="background:#95a5a6;color:white;margin-left:10px">取消</a>
</form>
"""


# ============================================
# 第四部分：路由和视图函数
# ============================================

@app.route('/')
def index():
    """
    首页：从数据库查询所有学生，用 Jinja2 模板渲染列表
    """
    with sqlite3.connect(DB_PATH) as conn:
        # row_factory = sqlite3.Row：让查询结果支持 row["列名"] 访问
        # 默认返回的是 tuple，只能用 row[0]、row[1] 访问，可读性差
        conn.row_factory = sqlite3.Row

        cursor = conn.cursor()
        # ORDER BY id：按插入顺序排序（最早插入的在前面）
        cursor.execute("SELECT * FROM students ORDER BY id")
        rows = cursor.fetchall()  # 获取所有行，每行是 sqlite3.Row 对象

    # 将 Row 对象转为普通字典，并计算平均分
    students = []
    for row in rows:
        student = dict(row)  # Row 对象转为字典：{"id": 1, "name": "张三", ...}
        # 计算三门课平均分（保留1位小数在模板中用 round 过滤器处理）
        student["avg"] = (student["python_score"] + student["math_score"] + student["english_score"]) / 3
        students.append(student)

    # 用 make_page 包装模板内容，render_template_string 渲染 Jinja2 语法
    body = render_template_string(LIST_TEMPLATE, students=students)
    return make_page("学生列表", body)


@app.route('/add', methods=['GET', 'POST'])
def add_student():
    """
    添加学生：
    - GET 请求：显示空表单
    - POST 请求：接收表单数据，验证后写入数据库
    """
    # request.method：获取当前请求的方法（"GET" 或 "POST"）
    if request.method == 'GET':
        # GET 请求：渲染空表单，所有字段为空
        body = render_template_string(ADD_TEMPLATE, error="", name="",
                                      python_score="", math_score="", english_score="")
        return make_page("添加学生", body)

    # POST 请求：处理表单提交
    # request.form['字段名']：获取表单中 name="字段名" 的输入值（字符串类型）
    name = request.form.get('name', '').strip()  # strip() 去除首尾空格
    python_score = request.form.get('python_score', '')
    math_score   = request.form.get('math_score', '')
    english_score = request.form.get('english_score', '')

    # ---- 输入验证 ----
    error = ""
    if not name:
        error = "姓名不能为空"
    else:
        try:
            # int()：将字符串转为整数，如果不是数字会抛出 ValueError
            ps = int(python_score)
            ms = int(math_score)
            es = int(english_score)
            # 检查成绩范围（0-100）
            if not (0 <= ps <= 100 and 0 <= ms <= 100 and 0 <= es <= 100):
                error = "成绩必须在 0-100 之间"
        except ValueError:
            error = "成绩必须是整数"

    if error:
        # 验证失败：重新显示表单，保留用户输入，显示错误信息
        body = render_template_string(ADD_TEMPLATE, error=error, name=name,
                                      python_score=python_score,
                                      math_score=math_score,
                                      english_score=english_score)
        return make_page("添加学生", body)

    # ---- 写入数据库 ----
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.cursor()
        # ? 占位符：安全地传入用户输入，防止 SQL 注入
        # 不指定 id，SQLite 自动分配 AUTOINCREMENT 的唯一 ID
        cursor.execute(
            "INSERT INTO students (name, python_score, math_score, english_score) VALUES (?, ?, ?, ?)",
            (name, ps, ms, es)
        )
        # with 块结束时自动 commit（提交事务），数据才真正写入文件

    # redirect()：重定向到首页，避免刷新页面重复提交表单
    # url_for('index')：根据视图函数名生成 URL（比硬编码 "/" 更安全）
    return redirect(url_for('index'))


@app.route('/delete/<int:student_id>', methods=['POST'])
def delete_student(student_id):
    """
    删除学生：根据 ID 删除一条记录
    - <int:student_id>：URL 中的动态参数，Flask 自动转为整数
    - 只接受 POST 请求（防止通过直接访问 URL 误删数据）
    """
    with sqlite3.connect(DB_PATH) as conn:
        cursor = conn.cursor()
        # WHERE id = ?：精确匹配要删除的记录
        cursor.execute("DELETE FROM students WHERE id = ?", (student_id,))
        # cursor.rowcount：受影响的行数，0 表示没有找到对应 ID

    # 删除后重定向回首页
    return redirect(url_for('index'))


@app.route('/api/students')
def api_students():
    """
    JSON API 接口：返回所有学生数据（JSON 格式）
    前端 JavaScript 或其他程序可以调用此接口获取数据
    """
    from flask import jsonify  # jsonify：将 Python 字典/列表转为 JSON 响应

    with sqlite3.connect(DB_PATH) as conn:
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM students ORDER BY id")
        rows = cursor.fetchall()

    students = []
    for row in rows:
        s = dict(row)
        # 计算平均分，保留1位小数
        s["avg"] = round((s["python_score"] + s["math_score"] + s["english_score"]) / 3, 1)
        students.append(s)

    # jsonify：自动设置 Content-Type: application/json，并序列化为 JSON
    return jsonify({"total": len(students), "students": students})


# ============================================
# 第五部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("""
────────────────────────────────────────────
错误1：用字符串格式化拼接 SQL（SQL 注入漏洞）
────────────────────────────────────────────

❌ 错误代码：
```python
name = request.form['name']  # 用户输入：'; DROP TABLE students; --
cursor.execute(f"SELECT * FROM students WHERE name = '{name}'")
```
结果：整张表被删除！这是严重的安全漏洞

✅ 正确做法：永远用 ? 占位符
```python
cursor.execute("SELECT * FROM students WHERE name = ?", (name,))
```
SQLite 会自动处理特殊字符，用户输入不会被当作 SQL 代码执行

────────────────────────────────────────────
错误2：忘记提交事务（数据不保存）
────────────────────────────────────────────

❌ 错误代码：
```python
conn = sqlite3.connect("students.db")
cursor = conn.cursor()
cursor.execute("INSERT INTO students ...")
conn.close()  # 忘记 conn.commit()，数据未保存！
```

✅ 正确做法：用 with 语句，成功自动 commit，异常自动 rollback
```python
with sqlite3.connect(DB_PATH) as conn:
    cursor = conn.cursor()
    cursor.execute("INSERT INTO students ...")
    # with 块结束时自动 commit
```

────────────────────────────────────────────
错误3：数据库文件路径问题（找不到数据库）
────────────────────────────────────────────

❌ 错误代码：
```python
conn = sqlite3.connect("students.db")
# 相对路径！数据库在哪里取决于运行命令的当前目录
# 从不同目录运行会创建多个不同的 students.db
```

✅ 正确做法：使用绝对路径
```python
DB_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "students.db")
conn = sqlite3.connect(DB_PATH)
# 无论从哪个目录运行，数据库文件始终在 .py 文件同目录
```

────────────────────────────────────────────
错误4：表单 GET/POST 方法混淆
────────────────────────────────────────────

❌ 错误代码：
```python
@app.route('/add')   # 忘记添加 methods=['POST']
def add():
    data = request.form['name']
```
错误信息：405 Method Not Allowed

原因：Flask 路由默认只接受 GET 请求

✅ 正确做法：
```python
@app.route('/add', methods=['GET', 'POST'])
def add():
    if request.method == 'POST':
        data = request.form['name']
```

────────────────────────────────────────────
错误5：未验证用户输入，存入非法数据
────────────────────────────────────────────

❌ 错误代码：
```python
score = request.form['score']
cursor.execute("INSERT INTO students (score) VALUES (?)", (score,))
# score 可能是 "abc"、"-5"、"999"
```

✅ 正确做法：验证数据类型和范围
```python
try:
    score = int(request.form['score'])
    if not (0 <= score <= 100):
        return "成绩必须在 0-100 之间"
except ValueError:
    return "成绩必须是整数"
```

────────────────────────────────────────────
错误6：表单提交后刷新页面重复插入数据
────────────────────────────────────────────

❌ 错误代码：
```python
@app.route('/add', methods=['POST'])
def add():
    # 写入数据库后直接返回页面
    return "添加成功！"
    # 用户刷新浏览器 → 浏览器重新发送 POST → 数据再次插入！
```

✅ 正确做法：POST 成功后重定向（PRG 模式：Post-Redirect-Get）
```python
return redirect(url_for('index'))
# 重定向后浏览器发送 GET 请求，刷新不会重复提交
```
""")


# ============================================
# 第六部分：练习题（不提供答案）
# ============================================

print("""
练习题（不提供答案，鼓励独立思考）

1. 编辑功能
   要求：添加 /edit/<id> 路由（GET显示编辑表单，POST保存修改）
         编辑表单预填当前数据，提交后更新数据库（用 UPDATE SQL）
   提示：先用 SELECT WHERE id=? 查询当前数据，用 UPDATE SET WHERE 更新

2. 搜索功能
   要求：在首页添加搜索框，输入姓名关键词过滤学生列表
         搜索结果显示匹配的学生，支持模糊匹配（名字包含输入字符串）
   提示：SQL LIKE 语句：WHERE name LIKE ?，参数传 "%关键词%"

3. 排序功能
   要求：点击表头"Python"、"数学"、"英语"、"平均分"可以按该列排序
         默认降序，再次点击升序（切换排序方向）
   提示：URL 参数 /? sort=python_score&order=desc，用 request.args 获取

4. 统计分析页面
   要求：添加 /stats 路由，显示：
         ① 各科最高分、最低分、平均分
         ② 优秀/良好/及格/不及格各有多少人
   提示：SQL 聚合函数：MAX()、MIN()、AVG()、COUNT()

5. 批量导入
   要求：添加 /import 路由，接受上传的 CSV 文件（姓名,Python成绩,数学成绩,英语成绩）
         解析 CSV 后批量插入数据库，跳过格式错误的行并报告错误数量
   提示：用 csv 模块解析，executemany() 批量插入，request.files 获取上传文件
""")


# ============================================
# 启动服务器
# ============================================

if __name__ == '__main__':
    # 启动前初始化数据库（创建表 + 插入示例数据）
    print("=" * 60)
    print("Flask + SQLite 学生成绩管理系统")
    print("=" * 60)
    print(f"数据库文件：{DB_PATH}")
    init_db()
    print()
    print("访问地址和路由说明：")
    print("  GET  /                 首页 - 学生成绩列表")
    print("  GET  /add              显示添加学生表单")
    print("  POST /add              提交添加学生表单")
    print("  POST /delete/<id>      删除指定 ID 的学生")
    print("  GET  /api/students     返回所有学生的 JSON 数据")
    print()
    print("访问地址：http://127.0.0.1:5003/")
    print("=" * 60)
    # debug=True：开发模式，代码修改后自动重启，显示详细错误信息
    # port=5003：使用 5003 端口
    app.run(debug=True, port=5003)
