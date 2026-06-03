# ============================================
# requests 高级用法 - 超详细注释版
# 日期：2026-05-31（第25天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【核心概念】requests 高级用法

已学过的基础：
- GET/POST 请求
- 响应处理（status_code、json()、text）
- 基本异常处理

今天学习的高级内容：
- Session（会话复用）
- 认证（API Key、Bearer Token）
- 超时和重试
- 请求头和参数
- 完整的错误处理
"""

import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
import json
import time

# ============================================
# 第一部分：Session 会话
# ============================================

print("第一部分：Session 会话")
print("=" * 60)

print("""
【Session 的作用】

普通请求：每次请求都是独立的连接
Session：复用 TCP 连接，保持 Cookie，共享配置

优势：
  1. 性能更好：复用连接，减少握手开销
  2. 自动保持 Cookie（登录状态）
  3. 共享请求头、认证信息等配置

语法：
  with requests.Session() as session:
      session.headers.update({'User-Agent': 'MyApp/1.0'})
      response = session.get(url)
""")

# 示例：使用 Session
print("示例：使用 Session 发送请求")
print("-" * 60)

with requests.Session() as session:
    # 设置公共请求头（所有请求都会带上）
    session.headers.update({
        'User-Agent': 'Python-Learning/1.0',
        'Accept': 'application/json'
    })

    # 发送请求（使用公共配置）
    try:
        response = session.get(
            'https://httpbin.org/headers',
            timeout=10
        )
        if response.status_code == 200:
            data = response.json()
            print(f"请求头已发送：{list(data['headers'].keys())}")
    except requests.exceptions.RequestException as e:
        print(f"请求失败（网络问题）：{type(e).__name__}")

print()

# ============================================
# 第二部分：认证方式
# ============================================

print("=" * 60)
print("第二部分：认证方式")
print("=" * 60)

print("""
【常见 API 认证方式】

1. API Key（最常见）
   - 在请求头中传递：Authorization: ApiKey xxx
   - 在查询参数中传递：?api_key=xxx

2. Bearer Token（OAuth 2.0）
   - 在请求头中传递：Authorization: Bearer xxx
   - 常用于需要登录的 API

3. Basic Auth（用户名密码）
   - requests.get(url, auth=('user', 'pass'))
   - 自动编码为 Base64

4. 无认证（公开 API）
   - 直接请求，无需认证
   - 通常有频率限制

【代码示例】

# API Key 在请求头
headers = {'Authorization': 'ApiKey your_key_here'}
response = requests.get(url, headers=headers)

# Bearer Token
headers = {'Authorization': 'Bearer your_token_here'}
response = requests.get(url, headers=headers)

# API Key 在查询参数
params = {'api_key': 'your_key_here'}
response = requests.get(url, params=params)

# Basic Auth
response = requests.get(url, auth=('username', 'password'))
""")

print()

# ============================================
# 第三部分：超时和重试
# ============================================

print("=" * 60)
print("第三部分：超时和重试")
print("=" * 60)

print("""
【超时设置】

timeout 参数：
  timeout=5          # 连接和读取都是5秒
  timeout=(3, 10)    # 连接超时3秒，读取超时10秒

为什么需要超时？
  - 防止请求永久挂起
  - 提高程序健壮性
  - 避免资源浪费

【重试机制】

使用 urllib3 的 Retry：
  retry = Retry(
      total=3,           # 最多重试3次
      backoff_factor=1,  # 重试间隔：1s, 2s, 4s
      status_forcelist=[500, 502, 503, 504]  # 这些状态码触发重试
  )
  adapter = HTTPAdapter(max_retries=retry)
  session.mount('https://', adapter)
""")

# 示例：带重试的 Session
print("示例：带超时和重试的 Session")
print("-" * 60)

def create_session_with_retry(retries=3, backoff=1):
    """
    创建带重试机制的 Session

    参数：
        retries - 最大重试次数
        backoff - 重试间隔基数（秒）
    """
    session = requests.Session()

    # 配置重试策略
    retry_strategy = Retry(
        total=retries,
        backoff_factor=backoff,
        status_forcelist=[429, 500, 502, 503, 504]
    )

    # 挂载适配器
    adapter = HTTPAdapter(max_retries=retry_strategy)
    session.mount('https://', adapter)
    session.mount('http://', adapter)

    return session

print("已创建带重试机制的 Session")
print("  最大重试次数：3")
print("  重试间隔：1s, 2s, 4s（指数退避）")
print("  触发重试的状态码：429, 500, 502, 503, 504")

print()

# ============================================
# 第四部分：完整的错误处理
# ============================================

print("=" * 60)
print("第四部分：完整的错误处理")
print("=" * 60)

print("""
【requests 异常层次】

requests.exceptions.RequestException  # 所有异常的基类
  ├── ConnectionError                  # 连接失败
  │   ├── ProxyError                   # 代理错误
  │   └── SSLError                     # SSL 证书错误
  ├── Timeout                          # 超时
  │   ├── ConnectTimeout               # 连接超时
  │   └── ReadTimeout                  # 读取超时
  ├── HTTPError                        # HTTP 错误（4xx, 5xx）
  └── URLRequired                      # URL 无效

【最佳实践】

def safe_request(url, **kwargs):
    try:
        response = requests.get(url, timeout=10, **kwargs)
        response.raise_for_status()  # 4xx/5xx 抛出 HTTPError
        return response.json()

    except requests.exceptions.ConnectTimeout:
        print("连接超时")
    except requests.exceptions.ReadTimeout:
        print("读取超时")
    except requests.exceptions.HTTPError as e:
        print(f"HTTP 错误：{e.response.status_code}")
    except requests.exceptions.ConnectionError:
        print("网络连接失败")
    except requests.exceptions.RequestException as e:
        print(f"请求失败：{e}")

    return None
""")

# 示例：完整的错误处理函数
def safe_get(url, params=None, headers=None, timeout=10):
    """
    安全的 GET 请求

    参数：
        url     - 请求 URL
        params  - 查询参数
        headers - 请求头
        timeout - 超时时间（秒）

    返回：
        响应数据（字典）或 None（失败时）
    """
    try:
        response = requests.get(
            url,
            params=params,
            headers=headers,
            timeout=timeout
        )
        response.raise_for_status()  # 非 2xx 状态码抛出异常
        return response.json()

    except requests.exceptions.ConnectTimeout:
        print(f"连接超时：{url}")
    except requests.exceptions.ReadTimeout:
        print(f"读取超时：{url}")
    except requests.exceptions.HTTPError as e:
        print(f"HTTP 错误 {e.response.status_code}：{url}")
    except requests.exceptions.ConnectionError:
        print(f"网络连接失败：{url}")
    except ValueError:
        print(f"JSON 解析失败：{url}")
    except requests.exceptions.RequestException as e:
        print(f"请求失败：{e}")

    return None

# 测试安全请求
print("测试安全请求函数：")
result = safe_get('https://httpbin.org/get', timeout=10)
if result:
    print(f"请求成功，URL：{result.get('url', 'N/A')}")
else:
    print("请求失败（可能是网络问题）")

print()

# ============================================
# 第五部分：常见错误和陷阱
# ============================================

print("=" * 60)
print("第五部分：常见错误和陷阱")
print("=" * 60)

print("""
[X] 错误1：没有设置超时

错误代码：
  response = requests.get(url)  # 可能永久挂起

[OK] 正确做法：
  response = requests.get(url, timeout=10)

记忆技巧：永远设置 timeout

────────────────────────────────────

[X] 错误2：没有检查状态码

错误代码：
  response = requests.get(url)
  data = response.json()  # 404 时也会尝试解析

[OK] 正确做法：
  response = requests.get(url)
  response.raise_for_status()  # 非 2xx 抛出异常
  data = response.json()

记忆技巧：raise_for_status() 在 json() 之前

────────────────────────────────────

[X] 错误3：频繁创建 Session

错误代码：
  for url in urls:
      response = requests.get(url)  # 每次新连接

[OK] 正确做法：
  with requests.Session() as session:
      for url in urls:
          response = session.get(url)  # 复用连接

记忆技巧：批量请求用 Session

────────────────────────────────────

[X] 错误4：直接访问 response.json() 不处理异常

错误代码：
  data = response.json()  # 响应不是 JSON 时报错

[OK] 正确做法：
  try:
      data = response.json()
  except ValueError:
      print("响应不是有效的 JSON")

记忆技巧：json() 可能抛出 ValueError

────────────────────────────────────

[X] 错误5：忽略 API 频率限制

错误代码：
  for i in range(1000):
      requests.get(api_url)  # 可能被封 IP

[OK] 正确做法：
  for i in range(1000):
      requests.get(api_url)
      time.sleep(0.1)  # 控制请求频率

记忆技巧：尊重 API 频率限制，加 sleep
""")

print()

# ============================================
# 第六部分：常用操作速查
# ============================================

print("=" * 60)
print("第六部分：常用操作速查")
print("=" * 60)

print("""
【基础请求】

requests.get(url, params={}, headers={}, timeout=10)
requests.post(url, json={}, headers={}, timeout=10)

【Session】

with requests.Session() as s:
    s.headers.update({'Authorization': 'Bearer token'})
    response = s.get(url, timeout=10)

【带重试的 Session】

from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry

retry = Retry(total=3, backoff_factor=1,
              status_forcelist=[500, 502, 503, 504])
adapter = HTTPAdapter(max_retries=retry)
session.mount('https://', adapter)

【完整错误处理】

try:
    response = requests.get(url, timeout=10)
    response.raise_for_status()
    data = response.json()
except requests.exceptions.Timeout:
    print("超时")
except requests.exceptions.HTTPError as e:
    print(f"HTTP 错误：{e.response.status_code}")
except requests.exceptions.RequestException as e:
    print(f"请求失败：{e}")
""")

print()

# ============================================
# 第七部分：练习题
# ============================================

print("=" * 60)
print("第七部分：练习题")
print("=" * 60)

print("""
1. 编写函数，使用 Session 批量请求多个 URL
   提示：使用 with requests.Session() as s

2. 编写函数，实现带指数退避的重试逻辑
   提示：失败后等待 1s, 2s, 4s 再重试

3. 编写函数，下载文件并显示进度
   提示：使用 stream=True 和 iter_content()

4. 编写函数，处理分页 API（每页返回部分数据）
   提示：循环请求直到没有下一页

5. 思考题：
   - Session 和普通请求有什么区别？
   - 为什么需要重试机制？
   - API 频率限制应该如何处理？
""")

print("=" * 60)
print("第25天 - requests 高级用法 学习完成！")
print("=" * 60)
