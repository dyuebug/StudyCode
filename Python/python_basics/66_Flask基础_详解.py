# ============================================
# Flask 基础 - 超详细注释版
# 日期：2026-06-07（第27天）
# 难度：⭐⭐ 中级
# ============================================

"""
【核心概念】

是什么？
- Flask 是 Python 最流行的轻量级 Web 框架
- 用几行代码就能启动一个 Web 服务器

为什么需要？
- 把 Python 程序变成可以用浏览器访问的 Web 应用
- 提供 API 接口供前端或其他程序调用

核心特点：
- 轻量：核心只有路由 + 视图函数，按需扩展
- 灵活：不强制项目结构，适合学习和小项目
- 生态丰富：有大量插件（数据库、认证、表单等）
"""

# 安装：pip install flask
from flask import Flask, request, jsonify, redirect, url_for

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

print("""
【Flask 核心三要素】

1. app = Flask(__name__)
   - 创建 Flask 应用实例
   - __name__ 告诉 Flask 当前文件的位置

2. @app.route('/路径')
   - 路由装饰器：把 URL 路径绑定到视图函数
   - 访问该路径时，自动调用被装饰的函数

3. 视图函数
   - 普通 Python 函数
   - 返回值就是浏览器看到的内容
   - 可以返回：字符串、HTML、JSON、重定向

【HTTP 请求流程】
浏览器 → 发送请求(URL + 方法) → Flask路由匹配 → 视图函数 → 返回响应 → 浏览器显示
""")

print("""
【函数卡片：Flask()】

作用：创建 Flask 应用实例
语法：app = Flask(__name__)

参数：
- __name__: 当前模块名（固定写法）

示例：
app = Flask(__name__)
""")

print("""
【函数卡片：@app.route()】

作用：注册路由（URL → 视图函数的映射）
语法：@app.route(路径, methods=[方法列表])

参数：
- 路径: URL 字符串，如 '/'、'/hello'、'/user/<name>'
- methods: 允许的 HTTP 方法，默认 ['GET']
  常用值：['GET']、['POST']、['GET', 'POST']

路径变量：
- <name>: 字符串变量
- <int:id>: 整数变量
- <float:price>: 浮点数变量

示例：
@app.route('/')                    # 首页
@app.route('/user/<name>')         # 带变量的路径
@app.route('/login', methods=['GET', 'POST'])  # 支持两种方法
""")

print("""
【函数卡片：request 对象】

作用：获取客户端发来的请求数据
来源：from flask import request

常用属性：
- request.method: 请求方法（'GET' 或 'POST'）
- request.args: GET 参数（URL中 ?key=value）
  用法：request.args.get('key', '默认值')
- request.form: POST 表单数据
  用法：request.form.get('key', '默认值')
- request.json: POST 的 JSON 数据
  用法：request.json.get('key')
- request.args.to_dict(): 转为普通字典

示例：
name = request.args.get('name', '游客')   # GET 参数
""")

print("""
【函数卡片：jsonify()】

作用：把 Python 字典/列表转成 JSON 响应
来源：from flask import jsonify

语法：return jsonify(数据)

示例：
return jsonify({'name': '张三', 'age': 18})
return jsonify([1, 2, 3])
return jsonify(message='成功', code=200)  # 关键字参数
""")

# ============================================
# 第二部分：基础路由示例
# ============================================

print("=" * 60)
print("第二部分：创建 Flask 应用")
print("=" * 60)

# 创建应用实例
# __name__ 是 Python 内置变量，代表当前模块名
# 当直接运行时：__name__ = '__main__'
# 当被导入时：__name__ = 模块文件名
app = Flask(__name__)

# ── 路由1：首页 ──
@app.route('/')
def index():
    # 返回 HTML 字符串
    # Flask 会自动设置 Content-Type: text/html
    return """
    <h1>欢迎来到 Flask 学习</h1>
    <p>这是第27天的学习项目</p>
    <ul>
        <li><a href="/hello">打招呼</a></li>
        <li><a href="/hello/张三">向张三打招呼</a></li>
        <li><a href="/add?a=10&b=20">计算 10+20</a></li>
        <li><a href="/api/info">JSON API</a></li>
    </ul>
    """

# ── 路由2：带路径变量 ──
@app.route('/hello')
@app.route('/hello/<name>')   # 同一函数绑定两个路由
def hello(name='世界'):
    # name 是路径变量，没有传入时使用默认值
    return f'<h2>你好，{name}！</h2><p><a href="/">返回首页</a></p>'

# ── 路由3：GET 参数 ──
@app.route('/add')
def add():
    # request.args.get(key, default) 获取 URL 参数
    # 访问：/add?a=10&b=20
    try:
        a = float(request.args.get('a', 0))  # 获取参数 a，默认 0
        b = float(request.args.get('b', 0))  # 获取参数 b，默认 0
        result = a + b
        return f'<h2>{a} + {b} = {result}</h2><p><a href="/">返回首页</a></p>'
    except ValueError:
        return '<p>参数必须是数字</p>', 400  # 返回 400 状态码

# ── 路由4：返回 JSON ──
@app.route('/api/info')
def api_info():
    # jsonify 把字典转成标准 JSON 响应
    # Content-Type 自动设为 application/json
    data = {
        'app': 'Flask学习项目',
        'day': 27,
        'topics': ['路由', '视图函数', 'GET参数', 'JSON响应'],
        'status': 'ok'
    }
    return jsonify(data)

# ── 路由5：GET + POST 双方法 ──
@app.route('/greet', methods=['GET', 'POST'])
def greet():
    if request.method == 'POST':
        # POST 请求：从表单获取数据
        name = request.form.get('name', '游客')
        return f'<h2>（POST）你好，{name}！</h2>'
    else:
        # GET 请求：显示表单
        return """
        <form method="POST">
            <input type="text" name="name" placeholder="输入你的名字">
            <button type="submit">打招呼</button>
        </form>
        """

# ── 路由6：路径变量类型转换 ──
@app.route('/square/<int:n>')
def square(n):
    # <int:n> 自动把路径变量转为整数
    # 访问非整数时 Flask 自动返回 404
    return jsonify({'number': n, 'square': n ** 2})

# ── 路由7：重定向 ──
@app.route('/go-home')
def go_home():
    # redirect() 发送 302 重定向响应
    # url_for('index') 根据函数名生成 URL（不硬编码路径）
    return redirect(url_for('index'))

# ============================================
# 第三部分：深入理解
# ============================================

print("""
🔍 深入理解：Flask 请求上下文（选学）

Flask 使用"上下文"来让视图函数能访问 request 对象：
- 每个请求进来时，Flask 自动把该请求绑定到当前线程
- 所以 request 看起来是全局变量，实际是线程本地变量
- 这就是为什么在视图函数外直接访问 request 会报错

可以跳过，不影响后续学习。
""")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
❌ 错误1：路由路径忘了斜杠开头
@app.route('hello')   # 错误
✅ 正确：
@app.route('/hello')  # 必须以 / 开头

────────────────────────────────────

❌ 错误2：视图函数名重复
@app.route('/a')
def index(): ...   # 函数名 index

@app.route('/b')
def index(): ...   # 同名！Flask 会报错
✅ 正确：每个视图函数名必须唯一

────────────────────────────────────

❌ 错误3：GET 请求用 request.form 取数据
name = request.form.get('name')  # GET 请求表单为空
✅ 正确：
- GET 参数 → request.args.get('key')
- POST 表单 → request.form.get('key')
- POST JSON → request.json.get('key')

────────────────────────────────────

❌ 错误4：直接运行被导入的模块
app.run()  # 没有 if __name__ == '__main__' 保护
✅ 正确：
if __name__ == '__main__':
    app.run(debug=True)

────────────────────────────────────

❌ 错误5：生产环境开启 debug=True
app.run(debug=True)  # 生产环境危险！暴露代码和错误详情
✅ 正确：
- 开发：debug=True（自动重载 + 错误页面）
- 生产：debug=False，用 gunicorn 等部署

────────────────────────────────────

❌ 错误6：端口被占用时不知道怎么办
OSError: [Errno 98] Address already in use
✅ 正确：换一个端口
app.run(port=5001)
""")

# ============================================
# 第五部分：函数速查
# ============================================

print("""
【Flask 函数速查】

创建应用：
  app = Flask(__name__)

路由：
  @app.route('/路径')
  @app.route('/路径/<变量名>')
  @app.route('/路径/<int:变量名>')
  @app.route('/路径', methods=['GET', 'POST'])

请求数据：
  request.method          → 'GET' 或 'POST'
  request.args.get('k')   → GET 参数
  request.form.get('k')   → POST 表单
  request.json            → POST JSON 体

响应：
  return '字符串'          → 文本/HTML
  return jsonify(dict)    → JSON
  return '内容', 状态码    → 带状态码
  return redirect(url)    → 重定向
  url_for('函数名')        → 生成 URL

启动：
  app.run(debug=True, port=5000)
""")

# ============================================
# 练习题
# ============================================

print("""
练习题

1. 创建一个路由 /bmi，接收 GET 参数 weight（体重kg）和 height（身高m），
   返回 BMI 值和健康状态（偏瘦/正常/超重/肥胖）

2. 创建一个路由 /api/weather/<city>，返回 JSON：
   {'city': city, 'temp': 25, 'weather': '晴天'}

3. 创建一个表单页面 /form，GET 显示表单（姓名+年龄），
   POST 接收数据并返回"你好，XX，你今年YY岁"

4. 思考：为什么要用 url_for('函数名') 而不直接写 '/路径'？
   提示：想想如果以后路径改了会怎样
""")

print("=" * 60)
print("Flask 基础学习完成！")
print("运行方式：python 66_Flask基础_详解.py")
print("然后浏览器访问：http://127.0.0.1:5000")
print("=" * 60)

# 启动服务器
if __name__ == '__main__':
    # debug=True：开发模式，代码修改后自动重启，显示详细错误
    # port=5000：监听端口（默认就是5000，可以省略）
    # host='0.0.0.0'：允许局域网访问（默认只允许本机）
    print("\n启动 Flask 开发服务器...")
    print("访问地址：http://127.0.0.1:5000")
    print("按 Ctrl+C 停止服务器\n")
    app.run(debug=True, port=5000)
