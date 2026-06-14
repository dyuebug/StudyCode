# ============================================
# 留言板项目 - 后端服务（Flask API）
# 日期：2026-06-14（第33天）
# 功能：提供留言的获取和提交 API
# ============================================

"""
【项目说明】留言板后端

提供两个 API 接口：
1. GET  /api/messages  - 获取所有留言
2. POST /api/messages  - 提交新留言

数据存储：内存列表（重启后清空）
跨域处理：flask-cors

运行方法：
1. 安装依赖：pip install flask flask-cors
2. 运行：python app.py
3. 后端地址：http://localhost:5000
"""

# 导入需要的模块
from flask import Flask, jsonify, request  # Flask 核心 + JSON + 请求对象
from flask_cors import CORS  # 解决跨域
import datetime  # 用于记录留言时间

# 创建 Flask 应用
app = Flask(__name__)
# 启用 CORS，允许前端跨域访问
CORS(app)

# --------------------------------------------
# 数据存储（用列表模拟数据库）
# --------------------------------------------

# 留言列表，每条留言是一个字典
# 预置两条示例留言
messages = [
    {
        "id": 1,
        "name": "小明",
        "content": "大家好，这是第一条留言！",
        "time": "2026-06-14 09:00:00"
    },
    {
        "id": 2,
        "name": "小红",
        "content": "前后端联动真有趣～",
        "time": "2026-06-14 09:30:00"
    }
]

# 用于生成新留言的 id（自增）
next_id = 3


# --------------------------------------------
# API 1：获取所有留言（GET）
# --------------------------------------------

@app.route('/api/messages', methods=['GET'])
def get_messages():
    """
    获取所有留言
    返回：留言列表（JSON）
    """
    # 直接返回留言列表
    # jsonify 会自动转成 JSON 格式
    return jsonify(messages)


# --------------------------------------------
# API 2：提交新留言（POST）
# --------------------------------------------

@app.route('/api/messages', methods=['POST'])
def add_message():
    """
    提交新留言
    接收：{"name": "姓名", "content": "留言内容"}
    返回：成功状态和新留言
    """
    # 声明使用全局变量 next_id
    global next_id

    # 获取前端发来的 JSON 数据
    data = request.get_json()

    # 用 get 安全获取字段（避免 KeyError）
    name = data.get('name', '匿名')
    content = data.get('content', '')

    # 简单验证：内容不能为空
    if not content.strip():
        # 返回错误，状态码 400
        return jsonify({"status": "error", "message": "留言内容不能为空"}), 400

    # 创建新留言
    new_message = {
        "id": next_id,
        "name": name,
        "content": content,
        # 记录当前时间
        "time": datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    }

    # 添加到留言列表
    messages.append(new_message)
    # id 自增，为下一条留言准备
    next_id += 1

    # 后端控制台打印（方便调试）
    print(f'收到新留言：{name} 说 "{content}"')

    # 返回成功响应和新留言
    return jsonify({
        "status": "success",
        "message": new_message
    })


# --------------------------------------------
# 首页路由（可选，方便测试）
# --------------------------------------------

@app.route('/')
def index():
    """首页，提示 API 用法"""
    return jsonify({
        "message": "留言板 API 服务",
        "endpoints": {
            "获取留言": "GET /api/messages",
            "提交留言": "POST /api/messages"
        }
    })


# --------------------------------------------
# 启动服务
# --------------------------------------------

if __name__ == '__main__':
    print("=" * 50)
    print("留言板后端服务启动中...")
    print("访问地址：http://localhost:5000")
    print("API 文档：http://localhost:5000")
    print("=" * 50)
    # debug=True：代码改动自动重启，显示详细错误
    # port=5000：服务端口
    app.run(debug=True, port=5000)
