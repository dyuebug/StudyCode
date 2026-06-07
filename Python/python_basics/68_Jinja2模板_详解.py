# ============================================
# Jinja2 模板引擎 - 超详细注释版
# 日期：2026-06-07（第28天）
# 难度：⭐⭐ 中级
# ============================================

"""
【核心概念】Jinja2 模板引擎

是什么？
- Jinja2 是 Flask 内置的模板引擎，用于生成动态 HTML 页面
- 将"数据"和"页面结构"分离，让代码更清晰

为什么需要？
- 字符串拼接生成 HTML 代码混乱、难维护、有安全风险（XSS）
- 模板引擎：在 HTML 文件中嵌入特殊标签，Flask 自动替换为真实数据

核心特点：
- {{ 变量 }}：输出变量值
- {% 语句 %}：逻辑控制（for、if）
- 模板继承：避免重复编写相同的 HTML 结构
"""

from flask import Flask, render_template_string

app = Flask(__name__)


# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

print("""
【为什么用模板，不用字符串拼接？】

❌ 字符串拼接（糟糕的方式）：
```python
html = "<h1>你好，" + name + "</h1>"
html += "<ul>"
for item in items:
    html += "<li>" + item + "</li>"
html += "</ul>"
```
问题：
① 代码混乱，HTML 和 Python 混在一起
② 有 XSS 安全漏洞（用户输入未转义）
③ 难以维护和修改

✅ Jinja2 模板（正确的方式）：
```html
<h1>你好，{{ name }}</h1>
<ul>
  {% for item in items %}
    <li>{{ item }}</li>
  {% endfor %}
</ul>
```
优点：
① HTML 结构清晰，数据和展示分离
② 自动转义特殊字符，防止 XSS 攻击
③ 支持继承、过滤器等强大功能
""")

print("""
【函数卡片：render_template_string()】

函数名：render_template_string()
作用：将 Jinja2 模板字符串渲染为 HTML，返回给浏览器
所属：flask 模块

完整语法：
render_template_string(source, **context)

参数详解：
1. source（必需）
   - 类型：str
   - 作用：包含 Jinja2 语法的 HTML 模板字符串
   - 示例："<h1>{{ title }}</h1>"

2. **context（可选，键值对）
   - 类型：任意 Python 对象（字符串、列表、字典等）
   - 作用：传递给模板的变量，模板中用 {{ 变量名 }} 访问
   - 示例：title="首页", users=[...]

返回值：
- 渲染后的 HTML 字符串（Flask 自动设置 Content-Type: text/html）

注意：
- render_template_string() 用于学习演示（模板写在 Python 代码里）
- 实际项目用 render_template("文件名.html", **context) 加载外部文件
""")

print("""
【Jinja2 核心语法总览】

语法类型          | 格式                  | 用途
-----------------|-----------------------|------------------
输出变量          | {{ 变量名 }}          | 显示变量值
for 循环          | {% for x in list %}  | 遍历列表
结束 for          | {% endfor %}         | for 循环结束标记
if 判断           | {% if 条件 %}        | 条件判断
elif / else      | {% elif %} {% else %}| 分支判断
结束 if           | {% endif %}          | if 结束标记
模板继承          | {% extends "父.html" %}| 继承父模板
定义块            | {% block 名称 %}     | 定义可覆盖的区域
结束块            | {% endblock %}       | block 结束标记
过滤器            | {{ 变量|过滤器 }}     | 对变量做转换处理
循环序号          | {{ loop.index }}     | for 循环内的序号（从1开始）
""")


# ============================================
# 第二部分：基础操作实践
# ============================================

print("=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 基础变量输出：{{ 变量 }}
# --------------------------------------------

# 演示路由1：基础变量输出
# 将 Python 数据传入模板，用 {{ }} 显示
TEMPLATE_VARS = """
<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>{{ title }}</title></head>
<body>
  <h1>{{ title }}</h1>

  <!-- {{ 变量名 }} 直接输出变量值 -->
  <p>学生姓名：{{ name }}</p>
  <p>年龄：{{ age }}</p>

  <!-- 访问字典的属性：变量名.键名 或 变量名['键名'] -->
  <p>课程：{{ course.name }}（{{ course.level }}）</p>

  <!-- 数学运算可以直接在模板中做 -->
  <p>下一年龄：{{ age + 1 }}</p>
</body>
</html>
"""

@app.route('/vars')
def demo_vars():
    # 通过关键字参数把数据传给模板
    # 参数名 = Python对象，模板中用同名变量访问
    return render_template_string(
        TEMPLATE_VARS,
        title="变量演示",          # 字符串
        name="张三",               # 字符串
        age=20,                    # 整数
        course={"name": "Python", "level": "中级"}  # 字典
    )

# --------------------------------------------
# 2.2 for 循环：{% for x in list %}
# --------------------------------------------

TEMPLATE_FOR = """
<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>for 循环演示</title></head>
<body>
  <h1>学生成绩列表</h1>
  <table border="1" cellpadding="8">
    <tr>
      <th>序号</th>
      <th>姓名</th>
      <th>分数</th>
    </tr>

    <!-- {% for 变量 in 列表 %}...{% endfor %} 遍历列表 -->
    {% for student in students %}
    <tr>
      <!-- loop.index：当前循环序号，从 1 开始 -->
      <!-- loop.index0：从 0 开始的序号 -->
      <td>{{ loop.index }}</td>
      <td>{{ student.name }}</td>
      <td>{{ student.score }}</td>
    </tr>
    {% endfor %}
  </table>

  <!-- loop 对象的其他属性 -->
  <p>共 {{ students|length }} 名学生</p>
</body>
</html>
"""

@app.route('/for')
def demo_for():
    # 准备传给模板的列表数据
    students_data = [
        {"name": "张三", "score": 92},
        {"name": "李四", "score": 85},
        {"name": "王五", "score": 78},
    ]
    # 传入列表，模板中用 {% for %} 遍历
    return render_template_string(TEMPLATE_FOR, students=students_data)

# --------------------------------------------
# 2.3 if 判断：{% if 条件 %}
# --------------------------------------------

TEMPLATE_IF = """
<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>if 判断演示</title></head>
<body>
  <h1>成绩等级判断</h1>
  <table border="1" cellpadding="8">
    <tr><th>姓名</th><th>分数</th><th>等级</th></tr>
    {% for s in students %}
    <tr>
      <td>{{ s.name }}</td>
      <td>{{ s.score }}</td>
      <!-- {% if %}...{% elif %}...{% else %}...{% endif %} 条件判断 -->
      <td>
        {% if s.score >= 90 %}
          <span style="color:green">优秀</span>
        {% elif s.score >= 75 %}
          <span style="color:blue">良好</span>
        {% elif s.score >= 60 %}
          <span style="color:orange">及格</span>
        {% else %}
          <span style="color:red">不及格</span>
        {% endif %}
      </td>
    </tr>
    {% endfor %}
  </table>
</body>
</html>
"""

@app.route('/if')
def demo_if():
    students_data = [
        {"name": "张三", "score": 95},
        {"name": "李四", "score": 80},
        {"name": "王五", "score": 65},
        {"name": "赵六", "score": 45},
    ]
    return render_template_string(TEMPLATE_IF, students=students_data)

# --------------------------------------------
# 2.4 过滤器：{{ 变量|过滤器名 }}
# --------------------------------------------

TEMPLATE_FILTER = """
<!DOCTYPE html>
<html>
<head><meta charset="utf-8"><title>过滤器演示</title></head>
<body>
  <h1>Jinja2 过滤器</h1>
  <ul>
    <!-- upper：转为大写 -->
    <li>upper：{{ name|upper }}</li>

    <!-- lower：转为小写 -->
    <li>lower：{{ name|lower }}</li>

    <!-- length：获取长度 -->
    <li>length：{{ items|length }} 个元素</li>

    <!-- round(n)：保留n位小数 -->
    <li>round(1)：{{ score|round(1) }}</li>

    <!-- default('默认值')：变量为空时显示默认值 -->
    <li>default：{{ empty_var|default('暂无数据') }}</li>

    <!-- join(分隔符)：列表转字符串 -->
    <li>join：{{ items|join(', ') }}</li>

    <!-- 链式过滤器：先 upper 再取 length -->
    <li>链式过滤器（upper|length）：{{ name|upper|length }}</li>
  </ul>
</body>
</html>
"""

@app.route('/filter')
def demo_filter():
    return render_template_string(
        TEMPLATE_FILTER,
        name="python",         # 字符串，演示 upper/lower
        items=["苹果", "香蕉", "橙子"],  # 列表，演示 length/join
        score=3.14159,         # 浮点数，演示 round
        empty_var=None,        # None，演示 default
    )

# --------------------------------------------
# 2.5 模板继承：{% extends %}、{% block %}
# --------------------------------------------

# 父模板：定义公共结构，用 {% block %} 标记可覆盖区域
BASE_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <!-- {% block title %} 定义可被子模板覆盖的标题区域 -->
  <title>{% block title %}默认标题{% endblock %}</title>
  <style>
    body { font-family: Arial; max-width: 800px; margin: 0 auto; padding: 20px; }
    nav { background: #333; padding: 10px; }
    nav a { color: white; margin-right: 15px; text-decoration: none; }
    .content { padding: 20px; border: 1px solid #ddd; margin-top: 10px; }
    footer { text-align: center; color: gray; margin-top: 20px; }
  </style>
</head>
<body>
  <!-- 公共导航栏（每个页面都有）-->
  <nav>
    <a href="/">首页</a>
    <a href="/vars">变量演示</a>
    <a href="/for">for循环</a>
    <a href="/if">if判断</a>
    <a href="/filter">过滤器</a>
    <a href="/inherit">模板继承</a>
  </nav>

  <!-- {% block content %} 定义主体内容区域，子模板覆盖此处 -->
  <div class="content">
    {% block content %}
    <p>这是默认内容</p>
    {% endblock %}
  </div>

  <!-- 公共页脚 -->
  <footer>
    <p>第28天学习 - Jinja2 模板引擎</p>
  </footer>
</body>
</html>
"""

# 子模板：用 {% extends %} 继承父模板，用 {% block %} 覆盖指定区域
# 注意：render_template_string 不支持真正的文件继承，这里模拟演示
# 实际项目中 render_template("child.html") 才能真正继承文件
CHILD_TEMPLATE = BASE_TEMPLATE.replace(
    "{% block title %}默认标题{% endblock %}",
    "学习成果 - 子页面"
).replace(
    "{% block content %}\n    <p>这是默认内容</p>\n    {% endblock %}",
    """
    <h2>🎉 模板继承演示</h2>
    <p>这是子模板覆盖的内容区域。</p>
    <p>父模板提供：导航栏 + 样式 + 页脚</p>
    <p>子模板只需填写：标题 + 主体内容</p>
    <hr>
    <p>优点：修改导航栏只需改父模板，所有页面自动更新！</p>
    """
)

@app.route('/inherit')
def demo_inherit():
    # 渲染继承了父模板结构的子页面
    return render_template_string(CHILD_TEMPLATE)

# --------------------------------------------
# 2.6 首页：综合演示所有功能
# --------------------------------------------

HOME_TEMPLATE = BASE_TEMPLATE.replace(
    "{% block title %}默认标题{% endblock %}",
    "Jinja2 模板引擎学习"
).replace(
    "{% block content %}\n    <p>这是默认内容</p>\n    {% endblock %}",
    """
    <h2>{{ title }}</h2>
    <p>{{ description }}</p>
    <h3>📚 本次学习内容</h3>
    <ul>
      {% for topic in topics %}
      <li>
        <strong>{{ loop.index }}. {{ topic.name }}</strong>
        {% if topic.important %}
          <span style="color:red">⭐ 重要</span>
        {% endif %}
        — {{ topic.url }}
      </li>
      {% endfor %}
    </ul>
    <p>共 {{ topics|length }} 个主题</p>
    """
)

@app.route('/')
def index():
    return render_template_string(
        HOME_TEMPLATE,
        title="Jinja2 模板引擎",
        description="Flask 内置模板引擎，用于生成动态 HTML 页面",
        topics=[
            {"name": "变量输出", "url": "/vars", "important": True},
            {"name": "for 循环", "url": "/for", "important": True},
            {"name": "if 判断", "url": "/if", "important": True},
            {"name": "过滤器", "url": "/filter", "important": False},
            {"name": "模板继承", "url": "/inherit", "important": False},
        ]
    )


# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("""
==============================================
🔍 深入理解：Jinja2 自动转义（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【XSS 攻击与自动转义】

假设用户输入了 <script>alert('黑客')</script> 作为姓名：

❌ 字符串拼接（有安全漏洞）：
html = "<p>" + user_input + "</p>"
→ 浏览器执行了恶意脚本！

✅ Jinja2 自动转义（安全）：
{{ user_input }}
→ 输出：&lt;script&gt;alert('黑客')&lt;/script&gt;
→ 浏览器显示为文本，不执行脚本

Flask 对 .html 文件默认开启自动转义。
如果你确定内容是安全的 HTML，用 {{ content|safe }} 关闭转义。

==============================================
🔍 深入理解：render_template vs render_template_string（选学）
==============================================

render_template("index.html", data=data)
→ 从 templates/ 目录读取文件
→ 适合实际项目（HTML 和 Python 完全分离）

render_template_string(template_str, data=data)
→ 模板字符串直接写在 Python 代码里
→ 适合学习演示（无需创建额外文件）

本文件使用 render_template_string() 是为了方便直接运行学习。
下一步学习（Flask 完整项目）将使用 render_template()。
==============================================
""")


# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
────────────────────────────────────────────
错误1：模板文件找不到（使用 render_template 时）
────────────────────────────────────────────

❌ 错误代码：
```python
# templates/ 目录不存在，或文件名写错
return render_template("index.html")
```
错误信息：TemplateNotFound: index.html

原因：Flask 默认在 app 同级的 templates/ 目录找模板文件

✅ 正确做法：
① 确保 templates/ 目录存在（与 .py 文件同级）
② 文件名大小写必须完全匹配（Linux 区分大小写）
③ 或改用 render_template_string() 把模板写在代码里

────────────────────────────────────────────
错误2：变量名传错，模板中输出空白
────────────────────────────────────────────

❌ 错误代码：
```python
return render_template_string("{{ username }}", user_name="张三")
# 传入的是 user_name，模板用的是 username（少了下划线）
```
结果：{{ username }} 输出空字符串（不报错，静默失败！）

原因：Jinja2 默认变量未定义时输出空字符串

✅ 正确做法：
① 变量名保持一致，模板和 Python 代码都用同一个名字
② 开发时开启严格模式检查未定义变量：
   app.jinja_env.undefined = StrictUndefined
③ 用 {{ username|default('未知') }} 提供默认值

────────────────────────────────────────────
错误3：忘记写 {% endfor %} 或 {% endif %}
────────────────────────────────────────────

❌ 错误代码：
```html
{% for item in items %}
  <li>{{ item }}</li>
<!-- 忘记写 {% endfor %} -->
```
错误信息：TemplateSyntaxError: Unexpected end of template

原因：Jinja2 的 for/if 必须有对应的 endfor/endif 结束标记

✅ 正确做法：
写完 {% for %} 立刻写 {% endfor %}，写完 {% if %} 立刻写 {% endif %}

────────────────────────────────────────────
错误4：{% block %} 名称拼写不一致
────────────────────────────────────────────

❌ 错误代码：
```html
<!-- 父模板定义 -->
{% block content %}{% endblock %}

<!-- 子模板覆盖（名称写错了）-->
{% block contents %}  ← 多了个 s
  <p>内容</p>
{% endblock %}
```
结果：子模板的内容没有出现，显示父模板默认内容

原因：block 名称不匹配，Jinja2 不会报错，静默忽略

✅ 正确做法：父子模板的 block 名称必须完全一致，建议用有意义的名字

────────────────────────────────────────────
错误5：在 {{ }} 中写 Python 语句而不是表达式
────────────────────────────────────────────

❌ 错误代码：
```html
{{ if score > 90: "优秀" }}        ← 错误：不能用 Python 语法
{{ x = score + 10 }}              ← 错误：不能赋值
{{ print(score) }}                 ← 错误：不能调用 print
```
错误信息：TemplateSyntaxError

原因：{{ }} 只能写"表达式"（有返回值的），不能写"语句"（if/赋值/print）

✅ 正确做法：
- 条件逻辑用 {% if %}...{% endif %}
- 数学运算可以：{{ score + 10 }}
- 三元表达式可以：{{ "优秀" if score >= 90 else "及格" }}

────────────────────────────────────────────
错误6：过滤器名称写错或参数格式错误
────────────────────────────────────────────

❌ 错误代码：
```html
{{ score|Round(1) }}     ← 错误：过滤器名大小写敏感，应该是 round
{{ name|upper() }}       ← 错误：无参数的过滤器不加括号
{{ items|join[', '] }}   ← 错误：过滤器参数用括号，不用方括号
```

✅ 正确做法：
{{ score|round(1) }}
{{ name|upper }}
{{ items|join(', ') }}

────────────────────────────────────────────
错误7：loop.index 在 for 循环外使用
────────────────────────────────────────────

❌ 错误代码：
```html
{% for item in items %}
  <li>{{ item }}</li>
{% endfor %}
<p>最后序号：{{ loop.index }}</p>  ← 错误：在循环外访问 loop
```
错误信息：UndefinedError: 'loop' is undefined

原因：loop 对象只在 {% for %}...{% endfor %} 内部有效

✅ 正确做法：loop.index 只在 {% for %} 循环体内使用
""")


# ============================================
# 第五部分：函数卡片速查
# ============================================

print("=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【render_template_string()】
语法：render_template_string(source, **context)
参数：
  - source: 包含 Jinja2 语法的 HTML 字符串
  - **context: 传给模板的变量（名称=值）
返回：渲染后的 HTML 字符串
示例：render_template_string("<h1>{{ name }}</h1>", name="张三")

────────────────────────────────────

【Jinja2 常用过滤器速查】

| 过滤器              | 作用                     | 示例                          |
|--------------------|--------------------------|-------------------------------|
| upper              | 转大写                   | {{ "abc"|upper }} → ABC       |
| lower              | 转小写                   | {{ "ABC"|lower }} → abc       |
| length             | 获取长度                 | {{ [1,2,3]|length }} → 3      |
| round(n)           | 保留n位小数              | {{ 3.14159|round(2) }} → 3.14 |
| default('x')       | 变量为空时显示默认值      | {{ none|default('无') }} → 无 |
| join(sep)          | 列表转字符串              | {{ [1,2]|join('-') }} → 1-2   |
| safe               | 关闭自动转义（输出HTML）  | {{ html_str|safe }}           |
| int                | 转为整数                  | {{ "42"|int }} → 42           |
| float              | 转为浮点数                | {{ "3.14"|float }} → 3.14     |
| trim               | 去除首尾空格              | {{ "  hi  "|trim }} → hi      |

────────────────────────────────────

【loop 对象属性速查（在 for 循环内有效）】

| 属性          | 作用                        |
|--------------|-----------------------------|
| loop.index   | 当前序号（从 1 开始）        |
| loop.index0  | 当前序号（从 0 开始）        |
| loop.first   | 是否是第一个元素（True/False）|
| loop.last    | 是否是最后一个元素           |
| loop.length  | 列表总长度                  |
""")


# ============================================
# 第六部分：练习题
# ============================================

print("=" * 60)
print("第六部分：练习题（不提供答案，鼓励独立思考）")
print("=" * 60)

print("""
1. 成绩报告页面
   要求：创建路由 /report，传入一个包含5个学生（姓名+3门课成绩）的列表
         用表格展示，最后一行显示每门课的平均分（用 round(1) 过滤器保留1位小数）
   提示：平均分可以在 Python 中算好再传给模板，或在模板中用 sum/length 过滤器计算

2. 导航高亮
   要求：给首页模板添加"当前页高亮"功能
         当访问 /vars 时，导航栏的"变量演示"链接显示为黄色背景
   提示：通过传入 current_page 变量，在模板中用 {% if %} 判断是否是当前页

3. 带分页的列表
   要求：有 20 条数据，每页显示 5 条，创建路由 /page/<int:page_num>
         用 Jinja2 的 loop.first、loop.last 在列表首尾显示不同样式
   提示：数据切片在 Python 中做，传给模板时已经是当前页的5条数据

4. 自定义过滤器
   要求：注册一个自定义过滤器 grade，把数字分数转为等级
         90+ → 优秀，75-89 → 良好，60-74 → 及格，60以下 → 不及格
         在模板中用 {{ score|grade }} 显示等级
   提示：用 @app.template_filter('grade') 装饰器注册自定义过滤器

5. 表单页面
   要求：创建路由 /form（GET）显示一个输入姓名和分数的表单
         提交后跳转到 /result，用 Jinja2 模板显示：姓名、分数、等级
   提示：需要 from flask import request，用 request.form['字段名'] 获取表单数据
""")


# ============================================
# 启动服务器
# ============================================

if __name__ == '__main__':
    print("\n" + "=" * 60)
    print("Jinja2 模板引擎演示服务器启动")
    print("=" * 60)
    print("访问以下地址查看演示：")
    print("  首页（功能导航）：  http://127.0.0.1:5002/")
    print("  变量输出演示：      http://127.0.0.1:5002/vars")
    print("  for 循环演示：      http://127.0.0.1:5002/for")
    print("  if 判断演示：       http://127.0.0.1:5002/if")
    print("  过滤器演示：        http://127.0.0.1:5002/filter")
    print("  模板继承演示：      http://127.0.0.1:5002/inherit")
    print("=" * 60)
    # debug=True：修改代码后自动重启，显示详细错误信息（仅开发环境使用）
    # port=5002：使用 5002 端口（避免与其他 Flask 应用冲突）
    app.run(debug=True, port=5002)
