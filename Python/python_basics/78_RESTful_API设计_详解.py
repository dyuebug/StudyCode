# ============================================
# RESTful API 设计 - 超详细注释版
# 日期：2026-06-16（第35天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【核心概念】RESTful API

是什么？
- REST = Representational State Transfer（表现层状态转换）
- 一种 Web API 设计规范和风格
- 用 URL 代表资源，用 HTTP 方法代表操作

为什么需要？
- 统一接口：前后端通信标准化
- 易于理解：URL 即文档，一看就懂
- 跨语言：任何支持 HTTP 的语言都能用

核心特点？
- 资源导向：URL 代表资源（名词）
- 统一接口：HTTP 方法代表操作（动词）
- 无状态：每次请求独立，不依赖上下文
"""

print("""
============================================
RESTful API 设计详解
============================================

今天的学习目标：
1. 理解 REST 设计思想（资源导向）
2. 掌握 HTTP 四种方法（GET/POST/PUT/DELETE）
3. 学会 URL 设计规范（名词 vs 动词）
4. 了解常用 HTTP 状态码

重要提示：
- REST 不是协议，是一种设计风格
- 遵循 REST 规范，API 更易用、易维护
- 现代 Web API 大多采用 RESTful 设计
""")

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 什么是 REST？
# --------------------------------------------

print("\n1.1 什么是 REST？")
print("-" * 60)

print("""
【REST】Representational State Transfer

生活例子：图书馆
- 资源：书（用书号标识）
- 操作：借书、还书、查询、预约

传统 API 设计（不好）：
- /getBook?id=1        - 获取书
- /addBook             - 添加书
- /deleteBook?id=1     - 删除书
- /updateBook?id=1     - 更新书

问题：
- URL 中有动词（get/add/delete）
- 不统一，难记忆
- 不符合 HTTP 语义

RESTful API 设计（好）：
- GET    /api/books/1  - 获取书
- POST   /api/books    - 添加书
- DELETE /api/books/1  - 删除书
- PUT    /api/books/1  - 更新书

优点：
- URL 只有名词（books）
- HTTP 方法表示操作
- 统一、直观、易理解

核心思想：
┌────────────────────────────────┐
│ URL = 资源（名词）              │
│ HTTP 方法 = 操作（动词）        │
└────────────────────────────────┘
""")

# --------------------------------------------
# 1.2 HTTP 四种方法（CRUD）⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.2 HTTP 四种方法（CRUD）")
print("-" * 60)

print("""
【CRUD】Create、Read、Update、Delete

HTTP 方法与 CRUD 对应：
┌──────────┬────────┬──────────────────┐
│ HTTP方法 │ CRUD   │ 说明             │
├──────────┼────────┼──────────────────┤
│ POST     │ Create │ 创建新资源       │
│ GET      │ Read   │ 查询资源         │
│ PUT      │ Update │ 更新资源（全量） │
│ DELETE   │ Delete │ 删除资源         │
└──────────┴────────┴──────────────────┘

留言板 API 示例：

1. GET /api/messages
   - 作用：获取所有留言
   - 请求：无需 body
   - 响应：[{留言1}, {留言2}, ...]

2. POST /api/messages
   - 作用：创建新留言
   - 请求：{"name": "小明", "content": "你好"}
   - 响应：{"status": "success", "message": {新留言}}

3. GET /api/messages/1
   - 作用：获取 id=1 的留言
   - 请求：无需 body
   - 响应：{留言详情}

4. DELETE /api/messages/1
   - 作用：删除 id=1 的留言
   - 请求：无需 body
   - 响应：{"status": "success"}

5. PUT /api/messages/1
   - 作用：更新 id=1 的留言
   - 请求：{"name": "小明", "content": "修改后的内容"}
   - 响应：{"status": "success", "message": {更新后留言}}

重要规则：
- GET：只读，不修改数据
- POST：创建，改变服务器状态
- PUT：更新，幂等（多次相同请求结果一样）
- DELETE：删除，幂等
""")

# --------------------------------------------
# 1.3 URL 设计规范 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.3 URL 设计规范")
print("-" * 60)

print("""
【URL 设计原则】

1. 使用名词（不要动词）
   - ✅ 正确：/api/messages
   - ❌ 错误：/api/getMessages、/api/deleteMessage

2. 使用复数形式
   - ✅ 正确：/api/messages（复数）
   - ❌ 错误：/api/message（单数）

3. 用 HTTP 方法表示操作
   - ✅ 正确：DELETE /api/messages/1
   - ❌ 错误：GET /api/messages/delete/1

4. 层级关系用路径表示
   - /api/users/123/posts        - 用户123的所有文章
   - /api/users/123/posts/456    - 用户123的文章456

5. 过滤、排序用查询参数
   - /api/messages?limit=10      - 最新10条
   - /api/messages?sort=time     - 按时间排序
   - /api/messages?name=小明     - 筛选小明的留言

完整示例：留言板 API

资源：messages（留言）

操作：
┌────────┬─────────────────────┬──────────────┐
│ 方法   │ URL                 │ 说明         │
├────────┼─────────────────────┼──────────────┤
│ GET    │ /api/messages       │ 获取所有留言 │
│ POST   │ /api/messages       │ 创建留言     │
│ GET    │ /api/messages/1     │ 获取留言1    │
│ PUT    │ /api/messages/1     │ 更新留言1    │
│ DELETE │ /api/messages/1     │ 删除留言1    │
└────────┴─────────────────────┴──────────────┘

记忆技巧：
- 集合操作：/api/messages（GET 获取、POST 创建）
- 单个资源：/api/messages/:id（GET/PUT/DELETE）
""")

# --------------------------------------------
# 1.4 HTTP 状态码 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.4 HTTP 状态码")
print("-" * 60)

print("""
【常用状态码】

2xx 成功系列：
- 200 OK：请求成功（GET、PUT、DELETE）
- 201 Created：创建成功（POST）
- 204 No Content：成功但无返回内容（DELETE）

4xx 客户端错误：
- 400 Bad Request：请求参数错误
- 404 Not Found：资源不存在
- 401 Unauthorized：未认证
- 403 Forbidden：无权限

5xx 服务器错误：
- 500 Internal Server Error：服务器内部错误

留言板 API 状态码使用：

1. GET /api/messages → 200
   - 成功返回留言列表
   - 即使列表为空，也返回 200（不是 404）

2. POST /api/messages → 200 或 201
   - 200：创建成功
   - 201：创建成功（更标准）

3. DELETE /api/messages/1 → 200 或 404
   - 200：删除成功
   - 404：留言不存在（id=1 不存在）

4. GET /api/messages/999 → 404
   - 留言不存在

5. POST /api/messages（参数错误）→ 400
   - 例：content 为空

Flask 中返回状态码：
```python
# 方法1：直接返回
return jsonify({"status": "success"}), 200

# 方法2：错误时
return jsonify({"error": "Not found"}), 404

# 方法3：使用 abort
from flask import abort
abort(404)
```

记忆技巧：
- 2xx：成功（OK）
- 4xx：客户端的错（你的问题）
- 5xx：服务器的错（我的问题）
""")

# ============================================
# 第二部分：基础操作实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 示例1：Flask 路径参数 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n2.1 示例1：Flask 路径参数")
print("-" * 60)

print("""
【路径参数】获取 URL 中的动态部分

语法：<参数名> 或 <类型:参数名>

示例1：基本用法
```python
@app.route('/api/messages/<id>')
def get_message(id):
    # id 是字符串类型
    return jsonify({"id": id})
```
访问：/api/messages/123 → id = "123"（字符串）

示例2：指定类型（推荐）⭐⭐⭐⭐⭐
```python
@app.route('/api/messages/<int:id>')
def get_message(id):
    # id 自动转换为整数
    return jsonify({"id": id})
```
访问：/api/messages/123 → id = 123（整数）
访问：/api/messages/abc → 404（自动处理格式错误）

支持的类型：
- string：默认，接受任何文本（不含斜杠）
- int：整数
- float：浮点数
- path：接受包含斜杠的文本

留言板删除接口示例：
```python
@app.route('/api/messages/<int:id>', methods=['DELETE'])
def delete_message(id):
    # id 已经是整数，可以直接比较
    global messages

    # 查找留言
    for i, msg in enumerate(messages):
        if msg['id'] == id:
            # 找到了，删除
            deleted = messages.pop(i)
            save_messages()
            return jsonify({"status": "success", "deleted": deleted}), 200

    # 未找到
    return jsonify({"error": "Message not found"}), 404
```

重点：
- <int:id>：自动转换类型 + 自动验证
- 格式错误（abc）：Flask 自动返回 404
- 逻辑清晰：找到删除、未找到404
""")

# --------------------------------------------
# 2.2 示例2：前端 fetch DELETE 请求
# --------------------------------------------

print("\n2.2 示例2：前端 fetch DELETE 请求")
print("-" * 60)

print("""
【DELETE 请求】前端发送删除请求

基本用法：
```javascript
async function deleteMessage(id) {
    try {
        // 发送 DELETE 请求
        const response = await fetch(`http://localhost:5000/api/messages/${id}`, {
            method: 'DELETE'  // 指定方法
        })

        // 检查响应状态
        if (response.ok) {
            console.log('删除成功')
            // 重新加载列表
            loadMessages()
        } else if (response.status === 404) {
            alert('留言不存在')
        } else {
            alert('删除失败')
        }

    } catch (error) {
        console.error('删除出错：', error)
    }
}
```

带确认提示：
```javascript
async function deleteMessage(id) {
    // 弹出确认对话框
    if (!confirm('确定要删除这条留言吗？')) {
        return  // 用户点击"取消"，不执行删除
    }

    try {
        const response = await fetch(`http://localhost:5000/api/messages/${id}`, {
            method: 'DELETE'
        })

        if (response.ok) {
            alert('删除成功')
            loadMessages()
        } else {
            alert('删除失败')
        }
    } catch (error) {
        alert('删除出错：' + error.message)
    }
}
```

HTML 绑定：
```html
<!-- 每条留言的删除按钮 -->
<button onclick="deleteMessage(1)">删除</button>

<!-- 或用 data 属性 -->
<button class="delete-btn" data-id="1">删除</button>

<script>
document.querySelectorAll('.delete-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        const id = btn.dataset.id
        deleteMessage(id)
    })
})
</script>
```

重点：
- method: 'DELETE'：必须指定
- DELETE 请求一般不需要 body
- 删除前必须确认（confirm）
- 删除后刷新列表
""")

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("\n" + "=" * 60)
print("🔍 深入理解：REST 六大约束（选学）")
print("=" * 60)

print("""
==============================================
🔍 深入理解：REST 架构风格六大约束（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【REST 六大约束】

1. 客户端-服务器分离（Client-Server）
   - 前端和后端独立演进
   - 各自关注点分离

2. 无状态（Stateless）
   - 每次请求独立，不依赖之前的请求
   - 服务器不保存客户端状态
   - 所有信息都在请求中

3. 可缓存（Cacheable）
   - 响应可以标记为可缓存/不可缓存
   - 提高性能

4. 统一接口（Uniform Interface）
   - 资源识别（URL）
   - 通过表述操作资源（JSON）
   - 自描述消息
   - 超媒体（HATEOAS）

5. 分层系统（Layered System）
   - 客户端不知道直接连接的是服务器还是中间层
   - 可以有负载均衡、缓存层等

6. 按需代码（Code-On-Demand，可选）
   - 服务器可以返回可执行代码
   - 例如：JavaScript

【为什么要遵循 REST？】

- 统一标准：降低学习成本
- 可扩展性：分层、无状态
- 可见性：URL 即文档
- 简单性：HTTP 原生支持

==============================================
""")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

# --------------------------------------------
# 4.1 错误1：URL 中使用动词
# --------------------------------------------

print("\n4.1 错误1：URL 中使用动词")
print("-" * 60)

print("""
❌ 错误代码：
```python
@app.route('/api/deleteMessage/<int:id>')
def delete_message(id):
    # URL 中有动词 delete
    pass
```

结果：不符合 REST 规范

原因：
- REST 原则：URL 是名词（资源），HTTP 方法是动词（操作）
- deleteMessage 既有名词又有动词，混乱

✅ 正确代码：
```python
@app.route('/api/messages/<int:id>', methods=['DELETE'])
def delete_message(id):
    # URL 只有名词 messages，方法是 DELETE
    pass
```

记忆技巧：URL = 名词，方法 = 动词
""")

# --------------------------------------------
# 4.2 错误2：忘记指定 methods
# --------------------------------------------

print("\n4.2 错误2：忘记指定 methods")
print("-" * 60)

print("""
❌ 错误代码：
```python
@app.route('/api/messages/<int:id>')  # 缺少 methods
def delete_message(id):
    # 默认只支持 GET
    pass
```

结果：前端 DELETE 请求返回 405 Method Not Allowed

原因：Flask 路由默认只接受 GET

✅ 正确代码：
```python
@app.route('/api/messages/<int:id>', methods=['DELETE'])
def delete_message(id):
    pass
```

预防措施：DELETE/POST/PUT 必须指定 methods
""")

# --------------------------------------------
# 4.3 错误3：删除后忘记保存
# --------------------------------------------

print("\n4.3 错误3：删除后忘记保存")
print("-" * 60)

print("""
❌ 错误代码：
```python
@app.route('/api/messages/<int:id>', methods=['DELETE'])
def delete_message(id):
    messages.pop(i)
    # 忘记调用 save_messages()
    return jsonify({"status": "success"}), 200
```

结果：内存中删除了，但文件未更新，重启后数据又回来了

原因：只修改了内存，没有持久化

✅ 正确代码：
```python
@app.route('/api/messages/<int:id>', methods=['DELETE'])
def delete_message(id):
    messages.pop(i)
    save_messages()  # 立即保存！
    return jsonify({"status": "success"}), 200
```

预防措施：任何修改操作后都要 save_messages()
""")

# --------------------------------------------
# 4.4 错误4：前端删除后不刷新列表
# --------------------------------------------

print("\n4.4 错误4：前端删除后不刷新列表")
print("-" * 60)

print("""
❌ 错误代码：
```javascript
async function deleteMessage(id) {
    await fetch(url, {method: 'DELETE'})
    // 删除成功，但没刷新列表
    alert('删除成功')
}
```

结果：删除的留言还显示在页面上（视觉Bug）

原因：后端删除了，前端页面没更新

✅ 正确代码：
```javascript
async function deleteMessage(id) {
    await fetch(url, {method: 'DELETE'})
    alert('删除成功')
    loadMessages()  // 重新加载列表！
}
```

预防措施：修改数据后必须刷新UI
""")

# --------------------------------------------
# 4.5 错误5：删除前不确认
# --------------------------------------------

print("\n4.5 错误5：删除前不确认")
print("-" * 60)

print("""
❌ 错误代码：
```javascript
async function deleteMessage(id) {
    // 直接删除，没有确认
    await fetch(url, {method: 'DELETE'})
}
```

结果：用户误点删除按钮，数据丢失

原因：删除是危险操作，必须确认

✅ 正确代码：
```javascript
async function deleteMessage(id) {
    if (!confirm('确定要删除这条留言吗？')) {
        return  // 用户取消
    }
    await fetch(url, {method: 'DELETE'})
}
```

预防措施：
- 删除、清空等危险操作必须确认
- 可以用 confirm() 或自定义模态框
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【RESTful API 设计规范】

────────────────────────────────────

【HTTP 方法】

GET - 查询资源
- 用途：获取数据
- 特点：幂等、安全、可缓存
- 示例：GET /api/messages

POST - 创建资源
- 用途：创建新数据
- 特点：非幂等
- 示例：POST /api/messages

PUT - 更新资源
- 用途：更新已有数据（全量）
- 特点：幂等
- 示例：PUT /api/messages/1

DELETE - 删除资源
- 用途：删除数据
- 特点：幂等
- 示例：DELETE /api/messages/1

────────────────────────────────────

【Flask 路径参数】

<int:id> - 整数参数
```python
@app.route('/api/messages/<int:id>')
def get_message(id):
    # id 自动转换为整数
```

<string:name> - 字符串参数（默认）
```python
@app.route('/api/users/<string:name>')
def get_user(name):
    # name 是字符串
```

────────────────────────────────────

【状态码】

200 OK - 成功
201 Created - 创建成功
204 No Content - 成功但无内容
400 Bad Request - 请求错误
404 Not Found - 资源不存在
500 Internal Server Error - 服务器错误

返回状态码：
```python
return jsonify(data), 200
return jsonify({"error": "Not found"}), 404
```

────────────────────────────────────

【记忆口诀】

URL = 名词（资源）
方法 = 动词（操作）

GET = 读（Read）
POST = 增（Create）
PUT = 改（Update）
DELETE = 删（Delete）
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题")
print("=" * 60)

print("""
练习1：设计 RESTful API
题目：为一个图书管理系统设计 RESTful API。
要求：
  - 资源：books（图书）
  - 操作：查询所有图书、查询单个图书、添加图书、更新图书、删除图书
  - 写出5个 API 的 HTTP 方法和 URL

提示：
  - 集合：/api/books
  - 单个：/api/books/:id
  - 用 GET/POST/PUT/DELETE

────────────────────────────────────

练习2：实现删除接口
题目：为留言板添加删除功能（后端）。
要求：
  - 路由：DELETE /api/messages/<int:id>
  - 找到留言：删除 + 保存 + 返回 200
  - 未找到：返回 404

提示：
  - 用 for 循环查找
  - messages.pop(i) 删除
  - save_messages() 保存

────────────────────────────────────

练习3：前端删除按钮
题目：在前端添加删除按钮（HTML + JS）。
要求：
  - 每条留言添加删除按钮
  - 点击弹出确认提示
  - 确认后发送 DELETE 请求
  - 删除成功刷新列表

提示：
  - <button onclick="deleteMessage(id)">删除</button>
  - confirm('确定删除吗？')
  - fetch(url, {method: 'DELETE'})

────────────────────────────────────

练习4：判断正误
题目：判断以下 API 设计是否符合 REST 规范。

1. GET /api/deleteMessage/1        （  ）
2. DELETE /api/messages/1          （  ）
3. POST /api/message               （  ）
4. GET /api/messages               （  ）
5. PUT /api/updateMessage/1        （  ）

提示：
  - URL 不能有动词
  - 用复数形式
  - 用 HTTP 方法表示操作

────────────────────────────────────

练习5：实现完整的 CRUD
题目：为留言板实现完整的 CRUD API。
要求：
  - GET /api/messages - 查询所有
  - POST /api/messages - 创建
  - GET /api/messages/:id - 查询单个
  - PUT /api/messages/:id - 更新
  - DELETE /api/messages/:id - 删除

提示：
  - 已有 GET（所有）、POST、DELETE
  - 需要添加 GET（单个）、PUT
  - PUT 需要 request.get_json() 获取数据
""")

print("\n" + "=" * 60)
print("恭喜！你已经掌握了 RESTful API 设计的核心知识！")
print("接下来：动手为留言板添加删除功能！")
print("=" * 60)

