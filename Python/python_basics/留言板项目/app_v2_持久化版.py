# ============================================
# 留言板项目 - 后端服务（Flask API + 数据持久化）
# 日期：2026-06-15（第34天）
# 功能：提供留言的获取和提交 API，数据保存到文件
# ============================================

"""
【项目说明】留言板后端（持久化版本）

提供两个 API 接口：
1. GET  /api/messages  - 获取所有留言
2. POST /api/messages  - 提交新留言

数据存储：JSON 文件（messages.json）
跨域处理：flask-cors

升级内容（第34天）：
✅ 启动时从 messages.json 加载历史留言
✅ 提交留言后立即保存到文件
✅ 重启后数据不丢失

运行方法：
1. 安装依赖：pip install flask flask-cors
2. 运行：python app.py
3. 后端地址：http://localhost:5000
"""

# 导入需要的模块
from flask import Flask, jsonify, request  # Flask 核心 + JSON + 请求对象
from flask_cors import CORS  # 解决跨域
import datetime  # 用于记录留言时间
import json  # 用于 JSON 文件读写
import os  # 用于文件路径操作

# 创建 Flask 应用
app = Flask(__name__)
# 启用 CORS，允许前端跨域访问
CORS(app)

# --------------------------------------------
# 数据存储（JSON 文件持久化）
# --------------------------------------------

# 数据文件路径（和 app.py 同目录）
DATA_FILE = 'messages.json'

# 留言列表（全局变量）
# 启动时从文件加载，不再预置数据
messages = []

# 用于生成新留言的 id
# 启动时根据现有留言计算
next_id = 1


# --------------------------------------------
# 数据持久化函数
# --------------------------------------------

def load_messages():
    """
    启动时：从 JSON 文件加载留言
    如果文件不存在：返回空列表（第一次运行）
    """
    global messages, next_id

    try:
        # 检查文件是否存在
        if not os.path.exists(DATA_FILE):
            print(f"[INFO] {DATA_FILE} 不存在，初始化为空列表（第一次运行）")
            messages = []
            next_id = 1
            return

        # 从文件加载
        with open(DATA_FILE, 'r', encoding='utf-8') as f:
            messages = json.load(f)

        # 计算 next_id（从现有留言中获取最大 id + 1）
        if messages:
            next_id = max(msg['id'] for msg in messages) + 1
        else:
            next_id = 1

        print(f"[OK] 成功加载 {len(messages)} 条留言，next_id={next_id}")

    except json.JSONDecodeError as e:
        # JSON 格式错误（文件损坏）
        print(f"[ERROR] JSON 格式错误：{e}")
        print("   将创建新的空列表")
        messages = []
        next_id = 1

    except Exception as e:
        # 其他异常
        print(f"[ERROR] 加载失败：{e}")
        messages = []
        next_id = 1


def save_messages():
    """
    操作后：保存留言到 JSON 文件
    确保数据持久化
    """
    try:
        # 保存到文件
        # indent=2：格式化，易读
        # ensure_ascii=False：保留中文
        with open(DATA_FILE, 'w', encoding='utf-8') as f:
            json.dump(messages, f, indent=2, ensure_ascii=False)

        print(f"[SAVE] 已保存 {len(messages)} 条留言到 {DATA_FILE}")

    except Exception as e:
        # 保存失败（权限、磁盘满等）
        print(f"[ERROR] 保存失败：{e}")
        # 注意：这里不抛出异常，避免影响用户请求
        # 实际项目中应该记录到日志


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
    # 数据已经在内存中（启动时加载）
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
    # 声明使用全局变量
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

    # 【重点】立即保存到文件！
    save_messages()

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
        "message": "留言板 API 服务（持久化版）",
        "version": "2.0 (with persistence)",
        "data_file": DATA_FILE,
        "total_messages": len(messages),
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
    print("留言板后端服务启动中（持久化版）...")
    print("=" * 50)

    # 【重点】启动前先加载数据！
    load_messages()

    print("访问地址：http://localhost:5000")
    print("API 文档：http://localhost:5000")
    print(f"数据文件：{DATA_FILE}")
    print("=" * 50)

    # debug=True：代码改动自动重启，显示详细错误
    # port=5000：服务端口
    app.run(debug=True, port=5000)
