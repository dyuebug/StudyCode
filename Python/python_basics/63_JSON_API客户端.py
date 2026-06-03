# ============================================
# JSON API 客户端 - 实战项目
# 日期：2026-05-31（第25天）
# 难度：⭐⭐⭐⭐ 综合应用
# ============================================

"""
【项目目标】

开发一个 JSON API 客户端，能够：
1. 调用公开 API 获取数据
2. 解析和处理 JSON 数据
3. 数据存储（JSON/CSV）
4. 错误处理和重试

【使用的公开 API】

1. JSONPlaceholder（测试用假数据）
   - https://jsonplaceholder.typicode.com
   - 无需认证，免费使用

2. Open-Meteo（天气数据）
   - https://api.open-meteo.com
   - 无需认证，免费使用

【用到的知识点】
- requests 库（HTTP 请求）
- json 模块（数据解析）
- csv 模块（数据导出）
- pathlib 模块（文件操作）
- 异常处理（网络错误）
"""

import requests
import json
import csv
import time
from pathlib import Path
from datetime import datetime
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry

# ============================================
# 第一部分：API 客户端基类
# ============================================

print("=" * 60)
print("JSON API 客户端 - 实战项目")
print("=" * 60)

class APIClient:
    """
    API 客户端基类

    提供：
    - 带重试的 Session
    - 统一的错误处理
    - 请求日志
    """

    def __init__(self, base_url, api_key=None, timeout=10):
        """
        初始化 API 客户端

        参数：
            base_url - API 基础 URL
            api_key  - API 密钥（可选）
            timeout  - 请求超时时间（秒）
        """
        self.base_url = base_url.rstrip('/')
        self.timeout = timeout
        self.session = self._create_session(api_key)

    def _create_session(self, api_key):
        """创建带重试机制的 Session"""
        session = requests.Session()

        # 设置公共请求头
        headers = {
            'User-Agent': 'Python-API-Client/1.0',
            'Accept': 'application/json',
            'Content-Type': 'application/json'
        }

        # 如果有 API Key，添加认证头
        if api_key:
            headers['Authorization'] = f'Bearer {api_key}'

        session.headers.update(headers)

        # 配置重试策略
        retry = Retry(
            total=3,
            backoff_factor=1,
            status_forcelist=[429, 500, 502, 503, 504]
        )
        adapter = HTTPAdapter(max_retries=retry)
        session.mount('https://', adapter)
        session.mount('http://', adapter)

        return session

    def get(self, endpoint, params=None):
        """
        发送 GET 请求

        参数：
            endpoint - API 端点（如 '/users'）
            params   - 查询参数

        返回：
            响应数据（字典或列表）或 None
        """
        url = f"{self.base_url}{endpoint}"

        try:
            response = self.session.get(
                url,
                params=params,
                timeout=self.timeout
            )
            response.raise_for_status()
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

    def post(self, endpoint, data):
        """
        发送 POST 请求

        参数：
            endpoint - API 端点
            data     - 请求数据（字典）

        返回：
            响应数据或 None
        """
        url = f"{self.base_url}{endpoint}"

        try:
            response = self.session.post(
                url,
                json=data,
                timeout=self.timeout
            )
            response.raise_for_status()
            return response.json()

        except requests.exceptions.RequestException as e:
            print(f"POST 请求失败：{e}")
            return None

    def close(self):
        """关闭 Session"""
        self.session.close()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

# ============================================
# 第二部分：JSONPlaceholder 客户端
# ============================================

class JSONPlaceholderClient(APIClient):
    """
    JSONPlaceholder API 客户端

    提供测试用的假数据：
    - 用户（/users）
    - 文章（/posts）
    - 评论（/comments）
    - 待办事项（/todos）
    """

    def __init__(self):
        super().__init__('https://jsonplaceholder.typicode.com')

    def get_users(self):
        """获取所有用户"""
        return self.get('/users') or []

    def get_user(self, user_id):
        """获取指定用户"""
        return self.get(f'/users/{user_id}')

    def get_posts(self, user_id=None):
        """获取文章列表"""
        params = {'userId': user_id} if user_id else None
        return self.get('/posts', params=params) or []

    def get_post(self, post_id):
        """获取指定文章"""
        return self.get(f'/posts/{post_id}')

    def get_comments(self, post_id=None):
        """获取评论列表"""
        params = {'postId': post_id} if post_id else None
        return self.get('/comments', params=params) or []

    def get_todos(self, user_id=None):
        """获取待办事项"""
        params = {'userId': user_id} if user_id else None
        return self.get('/todos', params=params) or []

    def create_post(self, title, body, user_id):
        """创建文章（模拟）"""
        return self.post('/posts', {
            'title': title,
            'body': body,
            'userId': user_id
        })

# ============================================
# 第三部分：天气 API 客户端
# ============================================

class WeatherClient(APIClient):
    """
    Open-Meteo 天气 API 客户端

    无需认证，免费使用
    """

    def __init__(self):
        super().__init__('https://api.open-meteo.com')

    def get_current_weather(self, latitude, longitude):
        """
        获取当前天气

        参数：
            latitude  - 纬度
            longitude - 经度

        返回：
            天气数据字典
        """
        params = {
            'latitude': latitude,
            'longitude': longitude,
            'current': 'temperature_2m,wind_speed_10m,weather_code',
            'timezone': 'auto'
        }
        return self.get('/v1/forecast', params=params)

    def get_forecast(self, latitude, longitude, days=7):
        """
        获取天气预报

        参数：
            latitude  - 纬度
            longitude - 经度
            days      - 预报天数

        返回：
            天气预报数据
        """
        params = {
            'latitude': latitude,
            'longitude': longitude,
            'daily': 'temperature_2m_max,temperature_2m_min,precipitation_sum',
            'forecast_days': days,
            'timezone': 'auto'
        }
        return self.get('/v1/forecast', params=params)

# ============================================
# 第四部分：数据存储工具
# ============================================

class DataExporter:
    """
    数据导出工具

    支持导出为 JSON 和 CSV
    """

    def __init__(self, output_dir="api_data"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)

    def save_json(self, data, filename):
        """保存为 JSON 文件"""
        filepath = self.output_dir / filename
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        print(f"已保存 JSON：{filepath}")
        return filepath

    def save_csv(self, data, filename, fields=None):
        """
        保存为 CSV 文件

        参数：
            data     - 数据列表（每项是字典）
            filename - 文件名
            fields   - 要导出的字段列表（None 表示全部）
        """
        if not data:
            print("没有数据可导出")
            return None

        filepath = self.output_dir / filename

        # 确定字段
        if fields is None:
            fields = list(data[0].keys())

        with open(filepath, 'w', newline='', encoding='utf-8') as f:
            writer = csv.DictWriter(f, fieldnames=fields, extrasaction='ignore')
            writer.writeheader()
            writer.writerows(data)

        print(f"已保存 CSV：{filepath}（{len(data)} 行）")
        return filepath

# ============================================
# 第五部分：测试代码
# ============================================

if __name__ == '__main__':
    import sys

    exporter = DataExporter()

    print("\n测试模式：演示 JSON API 客户端")
    print("=" * 60)

    # 测试1：JSONPlaceholder API
    print("\n测试1：JSONPlaceholder API")
    print("-" * 60)

    with JSONPlaceholderClient() as client:
        # 获取用户列表
        print("获取用户列表...")
        users = client.get_users()
        if users:
            print(f"获取到 {len(users)} 个用户")
            print(f"第一个用户：{users[0]['name']} ({users[0]['email']})")

            # 保存用户数据
            exporter.save_json(users, 'users.json')
            exporter.save_csv(
                users,
                'users.csv',
                fields=['id', 'name', 'email', 'phone', 'website']
            )

        # 获取第一个用户的文章
        print("\n获取用户1的文章...")
        posts = client.get_posts(user_id=1)
        if posts:
            print(f"获取到 {len(posts)} 篇文章")
            print(f"第一篇文章：{posts[0]['title'][:40]}...")

        # 获取待办事项统计
        print("\n获取用户1的待办事项...")
        todos = client.get_todos(user_id=1)
        if todos:
            completed = sum(1 for t in todos if t['completed'])
            print(f"共 {len(todos)} 个待办，已完成 {completed} 个")

        # 模拟创建文章
        print("\n模拟创建文章...")
        new_post = client.create_post(
            title="Python 学习笔记",
            body="今天学习了 requests 高级用法...",
            user_id=1
        )
        if new_post:
            print(f"创建成功，ID：{new_post.get('id')}")

    # 测试2：天气 API
    print("\n测试2：天气 API（北京）")
    print("-" * 60)

    with WeatherClient() as weather:
        # 北京的经纬度
        data = weather.get_current_weather(39.9042, 116.4074)
        if data and 'current' in data:
            current = data['current']
            print(f"当前温度：{current.get('temperature_2m')}°C")
            print(f"风速：{current.get('wind_speed_10m')} km/h")

        # 获取7天预报
        forecast = weather.get_forecast(39.9042, 116.4074, days=3)
        if forecast and 'daily' in forecast:
            daily = forecast['daily']
            print(f"\n未来3天天气预报：")
            for i, date in enumerate(daily['time']):
                max_t = daily['temperature_2m_max'][i]
                min_t = daily['temperature_2m_min'][i]
                rain = daily['precipitation_sum'][i]
                print(f"  {date}: {min_t}°C ~ {max_t}°C, 降水 {rain}mm")

    print("\n" + "=" * 60)
    print("测试完成！")
    print("\n使用方法：")
    print("  # 直接运行测试")
    print("  python 63_JSON_API客户端.py")
    print("=" * 60)
