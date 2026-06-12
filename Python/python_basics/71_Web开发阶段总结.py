# ============================================
# Web 开发阶段总结 - 知识梳理版
# 日期：2026-06-09（第30天）
# 难度：⭐⭐ 中级（复习巩固）
# ============================================

"""
【本文件的作用】

这是一份 Web 开发阶段（第27-29天）的知识地图。

学了什么？
- 第27天：Flask 基础（路由、视图函数、运行服务器）
- 第28天：Jinja2 模板（动态渲染 HTML）
- 第29天：Flask + SQLite 数据库（博客系统综合项目）

为什么需要总结？
- 把零散的知识点连成一条线，看清"请求 → 处理 → 响应"的完整链路
- 形成速查表，以后写代码时可以快速回忆

怎么用这份总结？
- 通读一遍，能在脑海中复现整个流程，说明掌握了
- 卡住的地方，回到对应天数的学习文件重新看
"""

# ============================================
# 第一部分：Web 开发是什么？⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：Web 开发的核心概念")
print("=" * 60)

# --------------------------------------------
# 1.1 一次网页访问到底发生了什么？
# --------------------------------------------

print("\n1.1 一次网页访问的完整流程")
print("-" * 60)

print("""
【生活类比：去餐厅吃饭】

浏览器  = 顾客（发出请求："我要看首页"）
网络    = 服务员（传递请求和菜品）
Flask   = 后厨（接到订单，做菜）
数据库  = 仓库（存放食材：文章、用户数据）
HTML    = 装盘好的菜（最终端给顾客的东西）

【技术流程】

1. 用户在浏览器输入网址（如 http://127.0.0.1:5000/）
   ↓ 发出 HTTP 请求
2. Flask 服务器收到请求，根据 URL 找到对应的"视图函数"
   ↓ 路由匹配
3. 视图函数执行：可能去数据库查数据
   ↓ 查询 SQLite
4. 把数据填进 HTML 模板（Jinja2 渲染）
   ↓ 生成最终 HTML
5. 把 HTML 返回给浏览器
   ↓ HTTP 响应
6. 浏览器把 HTML 渲染成你看到的页面

【关键词】
- HTTP：浏览器和服务器对话的"语言"
- 请求(Request)：浏览器发给服务器的（"我要什么"）
- 响应(Response)：服务器返回给浏览器的（"给你这个"）
""")

# --------------------------------------------
# 1.2 三个核心角色
# --------------------------------------------

print("\n1.2 Flask Web 应用的三大支柱")
print("-" * 60)

print("""
【支柱1：路由（Routing）】第27天
作用：决定"哪个网址"由"哪个函数"处理
比喻：餐厅的菜单——点哪道菜，对应哪个厨师

【支柱2：模板（Template）】第28天
作用：把数据动态填进 HTML，生成最终页面
比喻：信件模板——固定格式 + 变动的收件人姓名

【支柱3：数据库（Database）】第29天
作用：持久化保存数据（关机后数据不丢）
比喻：仓库——文章、用户都存在这里，随时取用

三者协作：路由接收请求 → 数据库取数据 → 模板渲染 → 返回页面
""")

# ============================================
# 第二部分：Flask 路由速查（第27天）⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第二部分：Flask 路由核心要点")
print("=" * 60)

# --------------------------------------------
# 2.1 最小 Flask 应用
# --------------------------------------------

print("\n2.1 一个 Flask 应用的最小骨架")
print("-" * 60)

print('''
【代码骨架】

from flask import Flask          # 1. 导入 Flask 类

app = Flask(__name__)            # 2. 创建应用实例
                                 #    __name__ 告诉 Flask 当前模块名

@app.route("/")                  # 3. 路由装饰器：绑定网址 "/"
def index():                     # 4. 视图函数：处理这个网址的请求
    return "Hello, World!"       # 5. 返回响应（字符串/HTML）

if __name__ == "__main__":       # 6. 直接运行此文件时
    app.run(debug=True)          #    启动开发服务器
''')

print("""
【关键点逐行解析】

@app.route("/")  ← 这叫"路由装饰器"
  - "/" 是 URL 路径（首页）
  - 装饰器把下面的函数"注册"到这个网址
  - 用户访问 / 时，Flask 自动调用 index()

app.run(debug=True)
  - debug=True：开发模式，代码改动自动重启，出错显示详细信息
  - ⚠️ 上线部署时必须改成 debug=False（安全）
""")

# --------------------------------------------
# 2.2 动态路由和请求方法
# --------------------------------------------

print("\n2.2 动态路由 + HTTP 方法")
print("-" * 60)

print('''
【动态路由：URL 里带变量】

@app.route("/user/<name>")       # <name> 是变量占位符
def show_user(name):             # 自动作为参数传入
    return f"你好，{name}"
# 访问 /user/小明 → 显示"你好，小明"

@app.route("/post/<int:post_id>")  # <int:...> 限定类型为整数
def show_post(post_id):
    return f"文章编号：{post_id}"
# 访问 /post/5 → post_id=5（整数）

【HTTP 方法：GET vs POST】

@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":   # 表单提交（写数据）
        username = request.form["username"]
        return f"登录：{username}"
    return "请填写登录表单"          # GET（读页面）

记忆口诀：
- GET  = 取数据（看页面、查询）—— 参数在 URL 里
- POST = 送数据（提交表单、新增）—— 参数在请求体里
''')

# ============================================
# 第三部分：Jinja2 模板速查（第28天）⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第三部分：Jinja2 模板核心要点")
print("=" * 60)

# --------------------------------------------
# 3.1 模板的三种语法
# --------------------------------------------

print("\n3.1 Jinja2 三种核心语法")
print("-" * 60)

print('''
【语法1：变量输出 {{ }}】
<h1>{{ title }}</h1>           ← 把变量 title 的值填进来
<p>作者：{{ user.name }}</p>   ← 可以访问对象属性

【语法2：逻辑控制 {% %}】
{% if user %}                  ← if 判断
    <p>欢迎，{{ user }}</p>
{% else %}
    <p>请登录</p>
{% endif %}                    ← 必须有 endif 结尾

{% for post in posts %}        ← for 循环遍历列表
    <li>{{ post.title }}</li>
{% endfor %}                   ← 必须有 endfor 结尾

【语法3：过滤器 |】
{{ name | upper }}             ← 转大写
{{ text | length }}            ← 取长度
{{ price | round(2) }}         ← 保留2位小数
''')

# --------------------------------------------
# 3.2 在 Flask 中渲染模板
# --------------------------------------------

print("\n3.2 视图函数如何渲染模板")
print("-" * 60)

print('''
【方式1：render_template（推荐，模板在独立文件）】

from flask import render_template

@app.route("/")
def index():
    posts = ["文章A", "文章B"]
    # 渲染 templates/index.html，并传入数据
    return render_template("index.html", posts=posts)
    #                       ↑模板文件名        ↑传给模板的变量

【方式2：render_template_string（模板写成字符串）】

from flask import render_template_string

HTML = "<h1>{{ title }}</h1>"
@app.route("/")
def index():
    return render_template_string(HTML, title="首页")

【数据传递关键】
render_template("页面.html", 变量名=值, 变量名2=值2)
                              ↑ 模板里用 {{ 变量名 }} 接收
''')

# ============================================
# 第四部分：SQLite 数据库速查（第29天）⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：SQLite 数据库核心要点")
print("=" * 60)

# --------------------------------------------
# 4.1 数据库操作五步法
# --------------------------------------------

print("\n4.1 操作数据库的标准五步")
print("-" * 60)

print('''
【五步法】

import sqlite3

# 1. 连接数据库（不存在会自动创建）
conn = sqlite3.connect("blog.db")

# 2. 让查询结果可以用列名访问（很实用！）
conn.row_factory = sqlite3.Row

# 3. 创建游标（执行 SQL 的工具）
cursor = conn.cursor()

# 4. 执行 SQL
cursor.execute("SELECT * FROM posts")
rows = cursor.fetchall()       # 取所有结果

# 5. 提交 + 关闭
conn.commit()                  # 写操作后必须提交
conn.close()                   # 用完关闭释放资源
''')

# --------------------------------------------
# 4.2 四种基本 SQL（增删改查 CRUD）
# --------------------------------------------

print("\n4.2 CRUD：增删改查 SQL 速查")
print("-" * 60)

print('''
【建表 CREATE】
CREATE TABLE posts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 自增主键
    title TEXT NOT NULL,                   -- 标题（不能为空）
    content TEXT,                          -- 内容
    category TEXT                          -- 分类
)

【增 INSERT】
cursor.execute(
    "INSERT INTO posts (title, content) VALUES (?, ?)",
    (title, content)              -- ? 占位符防止 SQL 注入！⭐
)
new_id = cursor.lastrowid         -- 获取新插入记录的 ID

【查 SELECT】
cursor.execute("SELECT * FROM posts WHERE id = ?", (post_id,))
row = cursor.fetchone()           -- 取一条
rows = cursor.fetchall()          -- 取全部

【改 UPDATE】
cursor.execute(
    "UPDATE posts SET title = ? WHERE id = ?",
    (new_title, post_id)
)

【删 DELETE】
cursor.execute("DELETE FROM posts WHERE id = ?", (post_id,))

⭐⭐⭐ 安全铁律：永远用 ? 占位符传参，不要用 f-string 拼 SQL！
错误：f"SELECT * FROM posts WHERE id = {user_input}"  ← SQL 注入风险
正确："SELECT * FROM posts WHERE id = ?", (user_input,)
''')

# ============================================
# 第五部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第五部分：Web 开发常见错误和陷阱")
print("=" * 60)

print("""
4个高频踩坑点回顾：

❌ 错误1：写操作后忘记 conn.commit()
   现象：程序没报错，但数据库里查不到刚插入的数据
   原因：INSERT/UPDATE/DELETE 后不 commit，改动不会真正保存
   ✅ 正确：写操作后立刻 conn.commit()

❌ 错误2：用 f-string 拼接 SQL
   错误：cursor.execute(f"... WHERE id = {uid}")
   原因：SQL 注入漏洞，黑客可构造恶意输入破坏数据库
   ✅ 正确：cursor.execute("... WHERE id = ?", (uid,))

❌ 错误3：POST 表单提交成功后不重定向
   现象：用户刷新页面，表单被重复提交（重复发文章）
   原因：浏览器刷新会重发上一次的 POST 请求
   ✅ 正确：用 PRG 模式 → return redirect(url_for("index"))

❌ 错误4：Jinja2 的 {% if %} 忘了写 {% endif %}
   现象：模板渲染报错 TemplateSyntaxError
   原因：Jinja2 的块语句必须成对闭合
   ✅ 正确：{% if %}...{% endif %}，{% for %}...{% endfor %}
""")

# ============================================
# 第六部分：练习题（不提供答案）
# ============================================

print("\n" + "=" * 60)
print("第六部分：自测练习题")
print("=" * 60)

print("""
不写代码，用自己的话回答下面的问题，检验掌握程度：

1. 【流程理解】
   用户访问 http://127.0.0.1:5000/post/3 时，
   从浏览器到看到页面，中间经历了哪几个步骤？
   提示：路由 → 视图函数 → 数据库 → 模板 → 响应

2. 【路由】
   @app.route("/article/<int:aid>") 中的 <int:aid> 是什么意思？
   如果用户访问 /article/abc 会发生什么？

3. 【模板】
   render_template("list.html", items=data) 这行代码里，
   模板文件中应该用什么语法来显示 data 列表的每一项？

4. 【数据库】
   为什么查询数据库时推荐设置 conn.row_factory = sqlite3.Row？
   它解决了什么不方便的问题？

5. 【安全】
   下面两行代码，哪一行有安全风险？为什么？
   A. cursor.execute("SELECT * FROM users WHERE name = ?", (name,))
   B. cursor.execute(f"SELECT * FROM users WHERE name = '{name}'")

6. 【综合】
   如果让你给博客系统加一个"搜索文章"功能，
   你需要用到哪些今天复习的知识点？画出大致流程。
""")

print("\n" + "=" * 60)
print("✅ Web 开发阶段总结完毕！")
print("第27-29天：路由 + 模板 + 数据库，三大支柱已掌握")
print("下一站：HTML/CSS 前端基础，给网页穿上漂亮的外衣")
print("=" * 60)
