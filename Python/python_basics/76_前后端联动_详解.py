# ============================================
# 前后端联动 - 超详细注释版
# 日期：2026-06-14（第33天）
# 难度：⭐⭐⭐ 中高级
# ============================================

"""
【核心概念】前后端联动

是什么？
- 前端（HTML/CSS/JS）负责界面和交互
- 后端（Flask/Python）负责数据和逻辑
- 通过 API 接口传递数据（JSON 格式）

为什么需要？
- 分工明确：前端管显示，后端管数据
- 数据动态：网页内容从后端实时获取
- 这是现代 Web 开发的标准模式

核心特点？
- 前后端分离：各自独立开发
- API 是桥梁：约定好接口格式
- JSON 是数据格式：通用、轻量
"""

print("""
============================================
前后端联动详解
============================================

今天的学习目标：
1. 理解前后端分离和 API 概念
2. 掌握 Flask 返回 JSON（jsonify）
3. 解决 CORS 跨域问题
4. 前端 fetch 调用后端 API
5. 完成留言板项目

重要提示：
- 这是综合应用，结合 Flask + fetch
- 今天打通前后端，是重要里程碑
- 多关注数据如何在前后端之间流转
""")

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 前后端分离是什么？
# --------------------------------------------

print("\n1.1 前后端分离是什么？")
print("-" * 60)

print("""
【前后端分离】

生活例子：餐厅
- 前端 = 服务员 + 餐桌（顾客看到的）
- 后端 = 厨房 + 仓库（顾客看不到的）
- API = 点菜单（服务员和厨房沟通的桥梁）

传统方式（前后端混在一起）：
- Flask 用模板（Jinja2）直接生成 HTML
- 后端把数据填进模板，返回完整页面
- 缺点：页面刷新才能更新数据

前后端分离方式（今天学的）：
- 后端只提供数据（JSON），不管页面
- 前端用 JS 获取数据，自己渲染页面
- 优点：局部更新，体验好（如点赞不刷新页面）

数据流向：
┌─────────┐   请求数据    ┌─────────┐
│  前端    │ ──────────→  │  后端    │
│ (浏览器) │              │ (Flask)  │
│         │ ←──────────  │         │
└─────────┘   返回JSON    └─────────┘
""")

# --------------------------------------------
# 1.2 API 是什么？
# --------------------------------------------

print("\n1.2 API 是什么？")
print("-" * 60)

print("""
【API（应用程序接口）】

是什么？
- API = 前后端约定好的"接口"
- 前端通过 URL 访问，后端返回数据
- 类似"菜单"，规定了能点什么菜

例子：
- GET /api/messages    → 获取所有留言
- POST /api/messages   → 提交新留言
- GET /api/users/1     → 获取 id=1 的用户

API 的组成：
1. URL：访问地址（如 /api/messages）
2. 方法：GET（获取）/ POST（提交）等
3. 数据：请求和响应的 JSON 格式

为什么用 JSON？
- 通用：各种语言都支持
- 轻量：比 XML 简洁
- 直观：类似 Python 字典

JSON 示例：
{
    "id": 1,
    "name": "小明",
    "message": "你好世界"
}
""")

# --------------------------------------------
# 1.3 函数卡片：jsonify() ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.3 jsonify() 函数详解")
print("-" * 60)

print("""
【函数卡片】

函数名：jsonify()
作用：把 Python 数据转成 JSON 响应
类型：Flask 提供的函数

完整语法：
from flask import jsonify
jsonify(数据)

参数详解：
- 数据（必需）
  - 类型：字典、列表等
  - 作用：要返回的数据
  - 示例：jsonify({"name": "小明"})

返回值：
- Flask Response 对象
- 自动设置 Content-Type: application/json
- 自动把 Python 数据转成 JSON 字符串

基本用法：
```python
from flask import Flask, jsonify

app = Flask(__name__)

@app.route('/api/data')
def get_data():
    # 返回字典
    return jsonify({"name": "小明", "age": 18})

@app.route('/api/list')
def get_list():
    # 返回列表
    return jsonify([1, 2, 3])
```

为什么用 jsonify 而不是直接返回字典？
- jsonify 自动设置正确的 Content-Type
- 自动处理中文编码
- 更规范、更安全

对比：
```python
# ❌ 旧方式（需要手动处理）
import json
return json.dumps(data), 200, {'Content-Type': 'application/json'}

# ✅ 推荐方式
return jsonify(data)
```
""")

# --------------------------------------------
# 1.4 函数卡片：request.get_json() ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.4 request.get_json() 函数详解")
print("-" * 60)

print("""
【函数卡片】

函数名：request.get_json()
作用：获取前端 POST 发送的 JSON 数据
类型：Flask request 对象的方法

完整语法：
from flask import request
data = request.get_json()

参数详解：无（直接调用）

返回值：
- Python 字典（解析后的 JSON 数据）
- 如果没有 JSON 数据，返回 None

基本用法：
```python
from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route('/api/messages', methods=['POST'])
def add_message():
    # 获取前端发来的 JSON 数据
    data = request.get_json()

    # data 是字典，可以直接访问
    name = data['name']
    message = data['message']

    return jsonify({"status": "success"})
```

重点：
- 必须设置 methods=['POST']
- 前端必须设置 Content-Type: application/json
- 返回的是 Python 字典，用 [] 或 .get() 访问

常用搭配：
```python
data = request.get_json()
name = data.get('name', '匿名')  # 用 get 避免 KeyError
```
""")

# --------------------------------------------
# 1.5 CORS 跨域问题 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.5 CORS 跨域问题")
print("-" * 60)

print("""
【CORS 跨域】

什么是跨域？
- 浏览器的安全机制（同源策略）
- "同源" = 协议 + 域名 + 端口都相同
- 不同源的请求会被浏览器拦截

例子（这些算跨域）：
- 前端：file:///index.html
- 后端：http://localhost:5000
- → 协议不同（file vs http），算跨域！

- 前端：http://localhost:3000
- 后端：http://localhost:5000
- → 端口不同（3000 vs 5000），算跨域！

为什么有跨域限制？
- 安全考虑，防止恶意网站
- 比如：防止恶意网站偷偷调用你的银行 API

跨域错误长什么样？
```
Access to fetch at 'http://localhost:5000/api'
from origin 'null' has been blocked by CORS policy
```

【解决方案：flask-cors】

第1步：安装
pip install flask-cors

第2步：使用
```python
from flask import Flask
from flask_cors import CORS

app = Flask(__name__)
CORS(app)  # 一行解决，允许所有跨域请求

@app.route('/api/data')
def get_data():
    return jsonify({"msg": "hello"})
```

进阶用法（指定允许的来源）：
```python
# 只允许特定域名
CORS(app, origins=['http://localhost:3000'])
```

记忆技巧：
- 跨域 = 不同源（协议/域名/端口任一不同）
- 解决 = CORS(app) 一行搞定
""")

# ============================================
# 第二部分：基础操作实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 示例1：Flask 创建 GET API
# --------------------------------------------

print("\n2.1 示例1：Flask 创建 GET API")
print("-" * 60)

print("""
创建文件：backend_demo.py

```python
# 导入需要的模块
from flask import Flask, jsonify
from flask_cors import CORS

# 创建 Flask 应用
app = Flask(__name__)
# 启用 CORS，允许前端跨域请求
CORS(app)

# GET 接口：返回一个用户信息
# 路由：/api/user
# 方法：GET（默认）
@app.route('/api/user')
def get_user():
    # 准备数据（字典）
    user = {
        "id": 1,
        "name": "小明",
        "age": 18
    }
    # 用 jsonify 返回 JSON
    return jsonify(user)

# GET 接口：返回列表
@app.route('/api/users')
def get_users():
    # 准备数据（列表）
    users = [
        {"id": 1, "name": "小明"},
        {"id": 2, "name": "小红"},
        {"id": 3, "name": "小刚"}
    ]
    return jsonify(users)

# 启动服务
if __name__ == '__main__':
    # debug=True：代码改动自动重启
    # port=5000：端口号
    app.run(debug=True, port=5000)
```

测试方法：
1. 运行：python backend_demo.py
2. 浏览器访问：http://localhost:5000/api/user
3. 看到：{"id":1,"name":"小明","age":18}

重点理解：
- @app.route 定义 API 路径
- jsonify 把字典/列表转成 JSON
- CORS(app) 允许前端调用
""")

# --------------------------------------------
# 2.2 示例2：前端 fetch 调用 GET API
# --------------------------------------------

print("\n2.2 示例2：前端 fetch 调用 GET API")
print("-" * 60)

print("""
创建文件：frontend_demo.html

<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>前端调用后端</title>
</head>
<body>
    <h1>用户列表</h1>
    <button id="loadBtn">加载用户</button>
    <ul id="userList"></ul>

    <script>
        const loadBtn = document.getElementById('loadBtn')
        const userList = document.getElementById('userList')

        // 点击按钮加载用户
        loadBtn.addEventListener('click', async () => {
            try {
                // 显示加载状态
                userList.innerHTML = '<li>加载中...</li>'

                // fetch 调用后端 API
                // URL 就是 Flask 的路由地址
                const response = await fetch('http://localhost:5000/api/users')

                // 检查响应状态
                if (!response.ok) {
                    throw new Error('请求失败：' + response.status)
                }

                // 解析 JSON 数据
                const users = await response.json()

                // 清空列表
                userList.innerHTML = ''

                // 遍历用户，添加到列表
                users.forEach(user => {
                    const li = document.createElement('li')
                    li.textContent = `${user.id}. ${user.name}`
                    userList.appendChild(li)
                })

            } catch (error) {
                userList.innerHTML = '<li>加载失败：' + error.message + '</li>'
            }
        })
    </script>
</body>
</html>

运行步骤：
1. 先运行后端：python backend_demo.py
2. 浏览器打开 frontend_demo.html
3. 点击"加载用户"按钮
4. 看到用户列表显示出来

关键点：
- fetch 的 URL = Flask 路由地址
- await response.json() 解析数据
- forEach 遍历数据，动态创建 DOM
""")

# --------------------------------------------
# 2.3 示例3：POST 请求（提交数据）⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n2.3 示例3：POST 请求（提交数据）")
print("-" * 60)

print("""
后端 POST 接口：

```python
from flask import Flask, jsonify, request
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# POST 接口：接收前端提交的数据
# methods=['POST'] 表示只接受 POST 请求
@app.route('/api/submit', methods=['POST'])
def submit():
    # 获取前端发来的 JSON 数据
    data = request.get_json()

    # data 是字典，访问字段
    name = data.get('name', '匿名')
    message = data.get('message', '')

    # 打印（后端控制台可以看到）
    print(f'收到留言：{name} 说 {message}')

    # 返回成功响应
    return jsonify({
        "status": "success",
        "received": data
    })

if __name__ == '__main__':
    app.run(debug=True, port=5000)
```

────────────────────────────────────

前端 POST 请求：

```javascript
// POST 请求提交数据
async function submitData() {
    try {
        const response = await fetch('http://localhost:5000/api/submit', {
            // method: 指定请求方法
            method: 'POST',
            // headers: 告诉后端发送的是 JSON
            headers: {
                'Content-Type': 'application/json'
            },
            // body: 要发送的数据（转成 JSON 字符串）
            body: JSON.stringify({
                name: '小明',
                message: '你好世界'
            })
        })

        const result = await response.json()
        console.log('提交成功：', result)

    } catch (error) {
        console.error('提交失败：', error)
    }
}
```

POST 请求三要素：
1. method: 'POST'
2. headers: {'Content-Type': 'application/json'}
3. body: JSON.stringify(数据)

重点：
- body 必须用 JSON.stringify() 转成字符串
- headers 必须设置 Content-Type
- 后端用 request.get_json() 接收
""")

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("\n" + "=" * 60)
print("🔍 深入理解：HTTP 请求方法（选学）")
print("=" * 60)

print("""
==============================================
🔍 深入理解：HTTP 请求方法和状态码（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【HTTP 请求方法】

常用的 4 种方法（对应"增删改查"）：

1. GET（查）
   - 获取数据
   - 参数在 URL 里
   - 例：GET /api/users（获取用户列表）

2. POST（增）
   - 创建数据
   - 数据在 body 里
   - 例：POST /api/users（新增用户）

3. PUT（改）
   - 更新数据
   - 数据在 body 里
   - 例：PUT /api/users/1（更新 id=1 的用户）

4. DELETE（删）
   - 删除数据
   - 例：DELETE /api/users/1（删除 id=1 的用户）

这套规范叫 RESTful API。

【HTTP 状态码】

后端返回的状态码，表示请求结果：

2xx（成功）：
- 200 OK：请求成功
- 201 Created：创建成功

4xx（客户端错误）：
- 400 Bad Request：请求格式错误
- 401 Unauthorized：未登录
- 403 Forbidden：没权限
- 404 Not Found：找不到资源

5xx（服务器错误）：
- 500 Internal Server Error：服务器报错

前端检查状态码：
```javascript
if (response.ok) {       // 200-299
    // 成功
} else if (response.status === 404) {
    // 资源不存在
} else if (response.status === 500) {
    // 服务器错误
}
```

【为什么要区分方法？】

- 语义清晰：看方法就知道做什么
- 安全性：GET 不应该修改数据
- 缓存：GET 可以缓存，POST 不行
- 幂等性：GET/PUT/DELETE 多次调用结果一样

==============================================
""")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

# --------------------------------------------
# 4.1 错误1：忘记处理 CORS 跨域
# --------------------------------------------

print("\n4.1 错误1：忘记处理 CORS 跨域")
print("-" * 60)

print("""
❌ 错误代码（后端）：
```python
from flask import Flask, jsonify

app = Flask(__name__)
# 忘记加 CORS！

@app.route('/api/data')
def get_data():
    return jsonify({"msg": "hello"})
```

结果：前端 fetch 报错
```
Access to fetch ... blocked by CORS policy
```

原因：浏览器同源策略，跨域请求被拦截

✅ 正确代码：
```python
from flask import Flask, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)  # 加上这一行！

@app.route('/api/data')
def get_data():
    return jsonify({"msg": "hello"})
```

预防措施：前后端分离开发，记得装 flask-cors 并 CORS(app)
""")

# --------------------------------------------
# 4.2 错误2：POST 忘记设置 Content-Type
# --------------------------------------------

print("\n4.2 错误2：POST 忘记设置 Content-Type")
print("-" * 60)

print("""
❌ 错误代码（前端）：
```javascript
fetch('http://localhost:5000/api/submit', {
    method: 'POST',
    // 忘记 headers！
    body: JSON.stringify({name: '小明'})
})
```

结果：后端 request.get_json() 返回 None

原因：
- 没设置 Content-Type，后端不知道是 JSON
- request.get_json() 无法解析

✅ 正确代码：
```javascript
fetch('http://localhost:5000/api/submit', {
    method: 'POST',
    headers: {
        'Content-Type': 'application/json'  // 必须设置
    },
    body: JSON.stringify({name: '小明'})
})
```

预防措施：POST 发 JSON 必须设置 Content-Type: application/json
""")

# --------------------------------------------
# 4.3 错误3：body 忘记 JSON.stringify
# --------------------------------------------

print("\n4.3 错误3：body 忘记 JSON.stringify")
print("-" * 60)

print("""
❌ 错误代码（前端）：
```javascript
fetch('http://localhost:5000/api/submit', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: {name: '小明'}  // 直接传对象，错误！
})
```

结果：后端收到 [object Object]，无法解析

原因：
- body 必须是字符串
- JavaScript 对象需要转成 JSON 字符串

✅ 正确代码：
```javascript
fetch('http://localhost:5000/api/submit', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({name: '小明'})  // 转成字符串
})
```

记忆技巧：
- 发送：JSON.stringify(对象) → 字符串
- 接收：response.json() → 对象
- 一发一收，正好相反
""")

# --------------------------------------------
# 4.4 错误4：POST 路由忘记 methods=['POST']
# --------------------------------------------

print("\n4.4 错误4：POST 路由忘记 methods=['POST']")
print("-" * 60)

print("""
❌ 错误代码（后端）：
```python
@app.route('/api/submit')  # 没有 methods 参数
def submit():
    data = request.get_json()
    return jsonify({"status": "ok"})
```

结果：前端 POST 请求返回 405 Method Not Allowed

原因：
- Flask 路由默认只接受 GET
- 需要明确指定接受 POST

✅ 正确代码：
```python
@app.route('/api/submit', methods=['POST'])  # 加上 methods
def submit():
    data = request.get_json()
    return jsonify({"status": "ok"})
```

进阶：同时支持 GET 和 POST
```python
@app.route('/api/messages', methods=['GET', 'POST'])
def messages():
    if request.method == 'GET':
        return jsonify(message_list)  # 返回列表
    else:  # POST
        data = request.get_json()
        message_list.append(data)
        return jsonify({"status": "ok"})
```

预防措施：POST 接口必须加 methods=['POST']
""")

# --------------------------------------------
# 4.5 错误5：后端没启动就调用
# --------------------------------------------

print("\n4.5 错误5：后端没启动就调用")
print("-" * 60)

print("""
❌ 常见情况：
- 直接打开前端 HTML
- 没有运行 Flask 后端
- 点击按钮，fetch 失败

结果：
```
Failed to fetch
net::ERR_CONNECTION_REFUSED
```

原因：后端服务没启动，localhost:5000 无法访问

✅ 正确步骤：
1. 先启动后端：
   python backend_demo.py
   看到：Running on http://localhost:5000

2. 再打开前端 HTML

3. 测试 fetch 请求

预防措施：
- 前后端联动，确保后端先启动
- 浏览器访问 http://localhost:5000/api/xxx 测试后端
- 看控制台 Network 标签排查问题
""")

# --------------------------------------------
# 4.6 错误6：访问字段用 [] 导致 KeyError
# --------------------------------------------

print("\n4.6 错误6：访问字段用 [] 导致 KeyError")
print("-" * 60)

print("""
❌ 错误代码（后端）：
```python
data = request.get_json()
name = data['name']  # 如果没有 name 字段，报 KeyError
```

结果：前端没传 name 时，后端崩溃 500 错误

原因：字典用 [] 访问不存在的键会报错

✅ 正确代码：
```python
data = request.get_json()
name = data.get('name', '匿名')  # 用 get，有默认值
message = data.get('message', '')
```

记忆技巧（回顾字典知识）：
- data['name']：键不存在 → KeyError
- data.get('name')：键不存在 → None
- data.get('name', '默认值')：键不存在 → 返回默认值

预防措施：处理前端数据用 .get() 更安全
""")

# --------------------------------------------
# 4.7 错误7：前端 URL 写错或端口不对
# --------------------------------------------

print("\n4.7 错误7：前端 URL 写错或端口不对")
print("-" * 60)

print("""
❌ 错误情况：
```javascript
// 后端在 5000 端口，前端写了 8000
fetch('http://localhost:8000/api/data')

// 路径写错
fetch('http://localhost:5000/api/user')  // 后端是 /api/users
```

结果：404 Not Found 或连接失败

原因：URL 和后端路由不匹配

✅ 排查方法：
1. 确认后端端口（app.run(port=5000)）
2. 确认路由路径（@app.route('/api/users')）
3. 前端 URL 完全匹配

调试技巧：
- 浏览器直接访问后端 URL 测试
- 看控制台 Network 标签的请求详情
- 检查请求的完整 URL

预防措施：
- 前端 URL = 协议 + 域名 + 端口 + 路由
- 例：http://localhost:5000/api/users
- 一个字符都不能错
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【后端 Flask】

────────────────────────────────────

jsonify(数据)
- 作用：返回 JSON 响应
- 参数：字典或列表
- 示例：return jsonify({"msg": "ok"})

────────────────────────────────────

request.get_json()
- 作用：获取前端 POST 的 JSON
- 返回：字典
- 示例：data = request.get_json()

────────────────────────────────────

CORS(app)
- 作用：允许跨域请求
- 来源：from flask_cors import CORS
- 示例：CORS(app)

────────────────────────────────────

@app.route('/path', methods=['POST'])
- 作用：定义路由和方法
- 参数：methods 指定请求方法
- 示例：methods=['GET', 'POST']

────────────────────────────────────

request.method
- 作用：获取当前请求方法
- 返回：'GET' / 'POST' 等
- 示例：if request.method == 'POST':

════════════════════════════════════

【前端 fetch】

────────────────────────────────────

fetch(url) — GET 请求
```javascript
const res = await fetch('http://localhost:5000/api/data')
const data = await res.json()
```

────────────────────────────────────

fetch(url, options) — POST 请求
```javascript
const res = await fetch(url, {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify(数据)
})
```

────────────────────────────────────

JSON.stringify(对象)
- 作用：对象转 JSON 字符串
- 用于：POST 的 body
- 示例：JSON.stringify({name: '小明'})

────────────────────────────────────

JSON.parse(字符串)
- 作用：JSON 字符串转对象
- 注：response.json() 已自动处理
- 示例：JSON.parse('{"a":1}')
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题")
print("=" * 60)

print("""
练习1：创建简单 GET API
题目：用 Flask 创建一个 API，返回当前时间。
要求：
  - 路由：/api/time
  - 返回：{"time": "2026-06-14 10:30:00"}
  - 启用 CORS

提示：
  - import datetime
  - datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

────────────────────────────────────

练习2：前端调用并显示
题目：写一个前端页面，点击按钮获取上面的时间 API。
要求：
  - 按钮"获取时间"
  - 点击后显示当前时间
  - 添加错误处理

提示：
  - fetch + async/await
  - 显示到 div

────────────────────────────────────

练习3：POST 提交数据
题目：创建一个 POST API 接收用户名和年龄，返回欢迎语。
要求：
  - 后端：POST /api/greet
  - 接收：{"name": "小明", "age": 18}
  - 返回：{"message": "你好小明，你18岁了"}

提示：
  - request.get_json()
  - data.get('name')
  - f-string 拼接

────────────────────────────────────

练习4：计算器 API
题目：创建一个计算 API，接收两个数字和运算符。
要求：
  - POST /api/calc
  - 接收：{"a": 10, "b": 5, "op": "+"}
  - 返回：{"result": 15}
  - 支持 + - * /

提示：
  - if op == '+': result = a + b
  - 注意除零错误处理

────────────────────────────────────

练习5：综合应用（待办清单）
题目：创建一个待办事项 API + 前端。
要求：
  - GET /api/todos：获取所有待办
  - POST /api/todos：添加待办
  - 前端：输入框 + 添加按钮 + 列表显示
  - 用列表存储（内存）

提示：
  - todos = [] 全局列表
  - GET 返回 todos
  - POST 添加到 todos
  - 前端提交后重新获取列表
""")

print("\n" + "=" * 60)
print("恭喜！你已经掌握了前后端联动的核心知识！")
print("接下来：动手完成留言板项目，真正打通前后端！")
print("=" * 60)

