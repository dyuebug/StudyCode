# ============================================
# fetch 异步请求 - 超详细注释版
# 日期：2026-06-13（第32天）
# 难度：⭐⭐⭐ 中高级
# ============================================

"""
【核心概念】fetch 异步请求

是什么？
- fetch() 是 JavaScript 用来发送网络请求的内置函数
- 异步请求：发出请求后不等待，数据回来后再处理
- 类比 Python 的 requests 库，但 fetch 是异步的

为什么需要？
- JavaScript 是单线程，同步请求会卡住整个页面
- 网络请求慢（可能几秒），异步可以让页面继续响应用户
- 前后端分离：前端用 fetch 调用后端 API 获取数据

核心特点？
- 基于 Promise（承诺对象，代表未来会返回的结果）
- 支持链式调用 .then().then()
- 可以用 async/await 语法让异步代码看起来像同步
"""

print("""
============================================
fetch 异步请求详解
============================================

今天的学习目标：
1. 理解同步 vs 异步的区别
2. 掌握 fetch() 基本用法
3. 学会用 async/await 处理异步
4. 能调用 API 并显示数据

重要提示：
- 这是 JavaScript 知识，示例代码在 HTML 中运行
- Python 文件只是讲解，实际代码在后面的 HTML 示例
- 异步是前端开发的核心概念，多花时间理解
""")

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 同步 vs 异步
# --------------------------------------------

print("\n1.1 同步 vs 异步")
print("-" * 60)

print("""
【同步（Synchronous）】

生活例子：排队买票
- 前面的人不买完，你不能买
- 必须等待，不能做其他事
- 一个接一个按顺序执行

代码例子：
```python
# Python 同步代码
result = requests.get("https://api.com/data")  # 等待服务器响应
print(result)  # 必须等上一行完成
```

特点：
- ✅ 简单直观，按顺序执行
- ❌ 如果某步很慢，会卡住后面的代码
- ❌ 网络请求慢时，页面会"冻住"

────────────────────────────────────

【异步（Asynchronous）】

生活例子：餐厅点餐
- 点完餐拿号，不用站在柜台等
- 可以去找座位、玩手机
- 叫号了再去取餐

代码例子：
```javascript
// JavaScript 异步代码
fetch("https://api.com/data")  // 发出请求，不等待
  .then(response => console.log(response))  // 数据回来后执行这个

console.log("我先执行了！")  // 不等 fetch 完成
```

特点：
- ✅ 不阻塞，程序可以继续运行
- ✅ 适合网络请求、文件读取等耗时操作
- ❌ 代码执行顺序不直观，需要回调函数
""")

# --------------------------------------------
# 1.2 JavaScript 为什么需要异步？
# --------------------------------------------

print("\n1.2 JavaScript 为什么需要异步？")
print("-" * 60)

print("""
【JavaScript 是单线程】

什么是单线程？
- 一次只能做一件事（Python 可以多线程）
- 如果一个操作很慢，会卡住整个页面

例子：同步网络请求的问题
```javascript
// ❌ 如果是同步（假设）
let data = fetch_sync("https://api.com/data")  // 等3秒...
console.log(data)
// 这3秒内，页面完全冻住，按钮点不了，无法滚动！
```

【异步是救星】

用异步的效果：
```javascript
// ✅ 异步方式
fetch("https://api.com/data")  // 发出请求，立即返回
  .then(data => console.log(data))  // 数据回来后执行

// 页面不卡，用户可以继续操作
```

为什么前端必须异步？
1. 网络请求慢（可能几秒）
2. 用户体验：页面不能"冻住"
3. 单线程限制：不能等待
""")

# --------------------------------------------
# 1.3 Promise 是什么？⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.3 Promise 是什么？")
print("-" * 60)

print("""
【Promise（承诺对象）】

生活例子：外卖订单
- 下单后，店家给你一个"订单号"（Promise）
- 这个订单号代表"未来会送到的外卖"
- 外卖可能：送到（成功）、取消（失败）

Promise 是什么？
- 代表一个"未来会完成的操作"
- 有三种状态：
  1. pending（进行中）：外卖正在做
  2. fulfilled（成功）：外卖送到了
  3. rejected（失败）：外卖取消了

【Promise 的用法】

基本语法：
```javascript
fetch("https://api.com/data")  // 返回一个 Promise
  .then(response => {
    // 成功时执行这里
    console.log("数据到了！", response)
  })
  .catch(error => {
    // 失败时执行这里
    console.log("出错了！", error)
  })
```

关键点：
- fetch() 立即返回 Promise（不等待）
- .then() 注册"成功后做什么"
- .catch() 注册"失败后做什么"
- 数据回来时，自动执行对应的函数
""")

# --------------------------------------------
# 1.4 函数卡片：fetch() ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.4 fetch() 函数详解")
print("-" * 60)

print("""
【函数卡片】

函数名：fetch()
作用：发送网络请求（HTTP 请求）
类型：浏览器内置函数

完整语法：
fetch(url, options)

参数详解：
1. url（必需）
   - 类型：字符串
   - 作用：要请求的网址
   - 示例：'https://api.example.com/data'

2. options（可选）
   - 类型：对象
   - 作用：请求配置（方法、headers、body等）
   - 示例：{method: 'POST', headers: {...}}

返回值：
- Promise 对象
- 成功时返回 Response 对象（包含响应数据）

基本用法：
```javascript
// GET 请求（最常用）
fetch('https://api.example.com/data')
  .then(response => response.json())  // 解析 JSON
  .then(data => console.log(data))    // 使用数据
  .catch(error => console.error(error))  // 错误处理
```

推荐用法（async/await）：
```javascript
async function getData() {
  try {
    const response = await fetch('https://api.example.com/data')
    const data = await response.json()
    console.log(data)
  } catch (error) {
    console.error(error)
  }
}
```

重要方法：
- response.json()：解析 JSON 响应（返回 Promise）
- response.text()：获取文本响应
- response.ok：检查状态码是否 200-299
""")

# ============================================
# 第二部分：基础操作实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 示例1：fetch 基本用法（.then 链式调用）
# --------------------------------------------

print("\n2.1 示例1：fetch 基本用法（.then 链式调用）")
print("-" * 60)

print("""
创建文件：demo_fetch_basic.html

<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>fetch 基础示例</title>
</head>
<body>
    <h1>fetch 异步请求示例</h1>
    <button id="btn">获取数据</button>
    <div id="result"></div>

    <script>
        // 获取按钮元素
        const btn = document.getElementById('btn')
        const resultDiv = document.getElementById('result')

        // 点击按钮时发送请求
        btn.addEventListener('click', () => {
            console.log('开始请求...')

            // 调用公开 API：JSONPlaceholder（假数据）
            // 参数：url（API 地址）
            fetch('https://jsonplaceholder.typicode.com/posts/1')
                // .then() 第一步：处理响应对象
                // 参数：response - Response 对象（包含状态码、headers等）
                .then(response => {
                    console.log('响应到了！', response)
                    // response.json() - 解析 JSON 数据
                    // 返回：Promise（因为解析需要时间）
                    return response.json()
                })
                // .then() 第二步：处理解析后的数据
                // 参数：data - 解析好的 JavaScript 对象
                .then(data => {
                    console.log('数据解析完成！', data)
                    // 显示到页面
                    resultDiv.innerHTML = `
                        <h2>${data.title}</h2>
                        <p>${data.body}</p>
                    `
                })
                // .catch() 错误处理
                // 参数：error - 错误对象
                .catch(error => {
                    console.error('出错了！', error)
                    resultDiv.innerHTML = '请求失败：' + error
                })

            console.log('请求已发出，不等待！')
        })
    </script>
</body>
</html>

运行结果：
1. 点击按钮
2. 控制台输出："开始请求..." → "请求已发出，不等待！" → "响应到了！" → "数据解析完成！"
3. 页面显示文章标题和内容

重点理解：
- fetch() 立即返回，不等待
- .then() 是"数据回来后做什么"
- 链式调用：第一个 .then() 处理响应，第二个处理数据
""")

# --------------------------------------------
# 2.2 示例2：async/await 语法（推荐）⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n2.2 示例2：async/await 语法（推荐）")
print("-" * 60)

print("""
创建文件：demo_fetch_async.html

<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>async/await 示例</title>
</head>
<body>
    <h1>async/await 异步语法</h1>
    <button id="btn">获取数据</button>
    <div id="result"></div>

    <script>
        const btn = document.getElementById('btn')
        const resultDiv = document.getElementById('result')

        // 用 async 标记这是异步函数
        // async 函数会自动返回 Promise
        async function fetchData() {
            try {
                console.log('开始请求...')

                // await 等待 Promise 完成
                // 等待：fetch 返回响应
                const response = await fetch('https://jsonplaceholder.typicode.com/posts/1')
                console.log('响应到了！', response)

                // 等待：解析 JSON
                const data = await response.json()
                console.log('数据解析完成！', data)

                // 显示到页面
                resultDiv.innerHTML = `
                    <h2>${data.title}</h2>
                    <p>${data.body}</p>
                `

            } catch (error) {
                // 用 try/catch 处理错误
                console.error('出错了！', error)
                resultDiv.innerHTML = '请求失败：' + error
            }
        }

        // 点击按钮时调用异步函数
        btn.addEventListener('click', fetchData)
    </script>
</body>
</html>

async/await 优点：
- ✅ 代码看起来像同步，容易理解
- ✅ 错误处理用 try/catch，更直观
- ✅ 避免 .then() 链式调用的"回调地狱"

对比：
.then() 链式：      fetch().then().then().catch()
async/await：       await fetch(); await response.json()

记忆技巧：
- async：标记"这个函数有异步操作"
- await：等待 Promise 完成，拿到结果
- try/catch：捕获错误
""")

# --------------------------------------------
# 2.3 示例3：调用不同的 API
# --------------------------------------------

print("\n2.3 示例3：调用不同的 API")
print("-" * 60)

print("""
创建文件：demo_fetch_apis.html

<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>调用不同 API</title>
    <style>
        button { margin: 10px; padding: 10px; }
        #result { margin: 20px; padding: 10px; border: 1px solid #ccc; }
        img { max-width: 300px; }
    </style>
</head>
<body>
    <h1>调用不同的公开 API</h1>
    <button id="btn1">获取用户信息</button>
    <button id="btn2">获取随机狗狗图片</button>
    <button id="btn3">获取天气（模拟）</button>
    <div id="result"></div>

    <script>
        const resultDiv = document.getElementById('result')

        // API 1：JSONPlaceholder - 用户信息
        document.getElementById('btn1').addEventListener('click', async () => {
            try {
                // 获取用户数据
                const response = await fetch('https://jsonplaceholder.typicode.com/users/1')
                const user = await response.json()

                // 显示用户信息
                resultDiv.innerHTML = `
                    <h2>用户信息</h2>
                    <p>姓名：${user.name}</p>
                    <p>邮箱：${user.email}</p>
                    <p>公司：${user.company.name}</p>
                `
            } catch (error) {
                resultDiv.innerHTML = '请求失败：' + error
            }
        })

        // API 2：Dog API - 随机狗狗图片
        document.getElementById('btn2').addEventListener('click', async () => {
            try {
                // 获取随机狗狗图片
                const response = await fetch('https://dog.ceo/api/breeds/image/random')
                const data = await response.json()

                // 显示图片
                resultDiv.innerHTML = `
                    <h2>随机狗狗</h2>
                    <img src="${data.message}" alt="狗狗">
                `
            } catch (error) {
                resultDiv.innerHTML = '请求失败：' + error
            }
        })

        // API 3：模拟天气 API
        document.getElementById('btn3').addEventListener('click', async () => {
            try {
                // 使用 JSONPlaceholder 模拟天气数据
                const response = await fetch('https://jsonplaceholder.typicode.com/todos/1')
                const data = await response.json()

                // 模拟显示（实际项目用真实天气 API）
                resultDiv.innerHTML = `
                    <h2>天气信息（模拟）</h2>
                    <p>城市：北京</p>
                    <p>温度：${data.id}°C</p>
                    <p>状态：${data.completed ? '晴天' : '多云'}</p>
                `
            } catch (error) {
                resultDiv.innerHTML = '请求失败：' + error
            }
        })
    </script>
</body>
</html>

常用公开 API：
1. JSONPlaceholder：https://jsonplaceholder.typicode.com
   - 假数据，用于测试
   - 提供用户、文章、评论等数据

2. Dog API：https://dog.ceo/dog-api
   - 随机狗狗图片
   - 按品种查询

3. 真实项目常用：
   - 天气：OpenWeatherMap
   - 地图：高德、百度地图 API
   - 支付：微信、支付宝 API
""")

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("\n" + "=" * 60)
print("🔍 深入理解：异步机制（选学）")
print("=" * 60)

print("""
==============================================
🔍 深入理解：JavaScript 异步机制（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【JavaScript 事件循环（Event Loop）】

为什么 JavaScript 能异步？
- JavaScript 有一个"事件循环"机制
- 遇到异步操作（fetch、setTimeout），放入"任务队列"
- 继续执行后面的同步代码
- 同步代码执行完，再处理任务队列

示例：
```javascript
console.log('1')
fetch('https://api.com/data').then(data => console.log('3'))
console.log('2')
```

执行顺序：
1. 输出 '1'
2. fetch 发出请求，注册回调，放入队列
3. 输出 '2'（不等 fetch）
4. 数据回来，执行回调，输出 '3'

结果：1 → 2 → 3

【Promise 状态转换】

Promise 的三个状态：
1. pending（进行中）
   - 刚创建，还没结果
   - fetch() 刚发出请求

2. fulfilled（成功）
   - 操作成功，有结果
   - 服务器返回数据
   - 触发 .then()

3. rejected（失败）
   - 操作失败，有错误
   - 网络断了、服务器错误
   - 触发 .catch()

状态只能转换一次：
pending → fulfilled 或 pending → rejected

【async/await 本质】

async/await 是语法糖：
```javascript
// 这两种写法等价

// 方式1：.then()
function getData() {
    return fetch(url)
        .then(res => res.json())
        .then(data => data)
}

// 方式2：async/await
async function getData() {
    const res = await fetch(url)
    const data = await res.json()
    return data
}
```

await 的本质：
- 暂停函数执行，等 Promise 完成
- 拿到结果后继续执行
- 不阻塞其他代码运行

==============================================
""")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

# --------------------------------------------
# 4.1 错误1：忘记处理 response.json()
# --------------------------------------------

print("\n4.1 错误1：忘记处理 response.json()")
print("-" * 60)

print("""
❌ 错误代码：
```javascript
fetch('https://api.com/data')
    .then(response => {
        console.log(response)  // Response 对象，不是数据！
        // 忘记调用 response.json()
    })
```

结果：打印的是 Response 对象，不是实际数据

原因：
- fetch() 返回的是 Response 对象（包含状态码、headers）
- 需要调用 .json() 才能解析数据
- response.json() 也返回 Promise，需要再 .then()

✅ 正确代码：
```javascript
fetch('https://api.com/data')
    .then(response => response.json())  // 解析 JSON
    .then(data => console.log(data))    // 这才是数据
```

记忆技巧：fetch 两步走
1. 第一个 .then() 处理 response.json()
2. 第二个 .then() 拿到真正的数据
""")

# --------------------------------------------
# 4.2 错误2：在 async 函数外用 await
# --------------------------------------------

print("\n4.2 错误2：在 async 函数外用 await")
print("-" * 60)

print("""
❌ 错误代码：
```javascript
function getData() {  // 忘记加 async
    const data = await fetch('https://api.com/data')  // 报错！
}
```

结果：SyntaxError: await is only valid in async functions

原因：
- await 只能在 async 函数内使用
- 普通函数不能用 await

✅ 正确代码：
```javascript
async function getData() {  // 加 async
    const response = await fetch('https://api.com/data')
    const data = await response.json()
}
```

记忆技巧：await 必须配 async，成双成对
""")

# --------------------------------------------
# 4.3 错误3：忘记错误处理
# --------------------------------------------

print("\n4.3 错误3：忘记错误处理")
print("-" * 60)

print("""
❌ 错误代码：
```javascript
async function getData() {
    const response = await fetch('https://api.com/data')
    const data = await response.json()
    console.log(data)
    // 如果网络断了，程序崩溃，没有提示！
}
```

结果：网络错误时，程序静默失败，用户不知道发生了什么

原因：没有捕获异常

✅ 正确代码：
```javascript
async function getData() {
    try {
        const response = await fetch('https://api.com/data')
        const data = await response.json()
        console.log(data)
    } catch (error) {
        console.error('请求失败：', error)
        // 显示友好的错误提示
        alert('网络错误，请稍后重试')
    }
}
```

预防措施：
- async/await 必须配 try/catch
- .then() 必须配 .catch()
- 给用户友好的错误提示
""")

# --------------------------------------------
# 4.4 错误4：误以为 await 会阻塞其他代码
# --------------------------------------------

print("\n4.4 错误4：误以为 await 会阻塞其他代码")
print("-" * 60)

print("""
❌ 错误理解：
"await 会让整个页面卡住，等待请求完成"

实际情况：
```javascript
async function getData() {
    console.log('1')
    await fetch('https://api.com/data')  // 只暂停这个函数
    console.log('3')
}

getData()
console.log('2')  // 不会等 getData 完成
```

输出顺序：1 → 2 → 3

解释：
- await 只暂停当前 async 函数
- 其他代码继续运行
- 页面不会卡住

✅ 正确理解：
- await = 暂停当前函数，等 Promise 完成
- 不影响其他代码运行
- 页面仍然可以响应用户操作
""")

# --------------------------------------------
# 4.5 错误5：不检查响应状态码
# --------------------------------------------

print("\n4.5 错误5：不检查响应状态码")
print("-" * 60)

print("""
❌ 错误代码：
```javascript
const response = await fetch('https://api.com/data')
const data = await response.json()
// 如果服务器返回 404、500，仍然会尝试解析 JSON
```

结果：服务器错误时，解析 JSON 失败

原因：
- fetch() 只在网络错误时才 reject
- 服务器返回 404、500 等，fetch 仍然 resolve
- 需要手动检查 response.ok

✅ 正确代码：
```javascript
const response = await fetch('https://api.com/data')

// 检查状态码是否 200-299
if (!response.ok) {
    throw new Error(`HTTP 错误！状态：${response.status}`)
}

const data = await response.json()
```

预防措施：
- 总是检查 response.ok
- response.ok = true 代表状态码 200-299
- response.status 可以获取具体状态码
""")

# --------------------------------------------
# 4.6 错误6：在循环中使用 await
# --------------------------------------------

print("\n4.6 错误6：在循环中使用 await")
print("-" * 60)

print("""
❌ 低效代码：
```javascript
// 串行请求，慢！
for (let id of [1, 2, 3]) {
    const data = await fetch(`https://api.com/posts/${id}`)
    // 等第1个完成，再请求第2个...
}
// 总耗时 = 3个请求时间之和
```

问题：请求串行执行，浪费时间

✅ 正确代码（并行请求）：
```javascript
// 并行请求，快！
const promises = [1, 2, 3].map(id =>
    fetch(`https://api.com/posts/${id}`)
)
const responses = await Promise.all(promises)
// 3个请求同时发出，总耗时 = 最慢的那个
```

记忆技巧：
- 需要等待顺序：用 await 在循环中
- 不需要顺序：用 Promise.all() 并行
""")

# --------------------------------------------
# 4.7 错误7：忘记 return
# --------------------------------------------

print("\n4.7 错误7：忘记 return")
print("-" * 60)

print("""
❌ 错误代码：
```javascript
async function getData() {
    const response = await fetch('https://api.com/data')
    const data = await response.json()
    // 忘记 return
}

const result = await getData()
console.log(result)  // undefined
```

结果：result 是 undefined

原因：函数没有返回值

✅ 正确代码：
```javascript
async function getData() {
    const response = await fetch('https://api.com/data')
    const data = await response.json()
    return data  // 记得 return
}

const result = await getData()
console.log(result)  // 正确的数据
```

预防措施：async 函数也需要 return 返回值
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【函数1：fetch()】

语法：fetch(url, options)
参数：
  - url: 请求地址（字符串）
  - options: 配置对象（可选）
返回：Promise<Response>

示例：
```javascript
fetch('https://api.com/data')
```

────────────────────────────────────

【函数2：response.json()】

语法：response.json()
参数：无
返回：Promise<any>（解析后的数据）

示例：
```javascript
const data = await response.json()
```

────────────────────────────────────

【函数3：response.text()】

语法：response.text()
参数：无
返回：Promise<string>（文本内容）

示例：
```javascript
const text = await response.text()
```

────────────────────────────────────

【函数4：Promise.all()】

语法：Promise.all(promises)
参数：
  - promises: Promise 数组
返回：Promise<array>（所有结果）

示例：
```javascript
const results = await Promise.all([
    fetch('url1'),
    fetch('url2')
])
```

────────────────────────────────────

【关键属性】

response.ok
- 类型：布尔值
- 作用：状态码是否 200-299
- 示例：if (response.ok) {...}

response.status
- 类型：数字
- 作用：HTTP 状态码（200、404、500等）
- 示例：console.log(response.status)

response.headers
- 类型：Headers 对象
- 作用：响应头信息
- 示例：response.headers.get('Content-Type')
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题")
print("=" * 60)

print("""
练习1：基础 fetch 请求
题目：创建一个按钮，点击后获取 https://jsonplaceholder.typicode.com/users
      并在页面显示前3个用户的姓名。
要求：
  - 使用 async/await
  - 添加 try/catch 错误处理
  - 只显示前3个用户

提示：
  - 用 .slice(0, 3) 截取前3个
  - 用 .map().join() 拼接字符串

────────────────────────────────────

练习2：错误处理
题目：故意请求一个不存在的 URL（https://jsonplaceholder.typicode.com/notfound）
      捕获错误并显示友好的错误提示。
要求：
  - 检查 response.ok
  - 如果 !response.ok，抛出错误
  - catch 中显示错误信息

提示：
  - throw new Error('错误信息')
  - 在 catch 中用 error.message

────────────────────────────────────

练习3：并行请求
题目：同时请求3个不同的 post（id=1,2,3）
      等所有请求完成后，显示标题列表。
要求：
  - 使用 Promise.all()
  - 3个请求并行发出
  - 显示所有标题

提示：
  - 用 .map() 创建 Promise 数组
  - Promise.all(promises)
  - 结果是数组，遍历显示

────────────────────────────────────

练习4：加载状态
题目：添加"加载中..."提示
      请求期间显示"加载中..."，完成后显示数据。
要求：
  - 请求前显示"加载中..."
  - await 等待请求
  - 完成后替换为实际数据

提示：
  - 先 innerHTML = '加载中...'
  - 再 await fetch(...)
  - 最后 innerHTML = 实际数据

────────────────────────────────────

练习5：综合应用
题目：创建一个"随机笑话"按钮
      API：https://official-joke-api.appspot.com/random_joke
      显示：setup（问题）和 punchline（答案）
要求：
  - 按钮点击获取随机笑话
  - 分两行显示 setup 和 punchline
  - 添加加载状态和错误处理

提示：
  - 响应格式：{setup: "...", punchline: "..."}
  - 用 <p> 标签分行显示
""")

print("\n" + "=" * 60)
print("恭喜！你已经掌握了 fetch 异步请求的核心知识！")
print("下一步：学习前后端联动，调用自己的 Flask API")
print("=" * 60)

