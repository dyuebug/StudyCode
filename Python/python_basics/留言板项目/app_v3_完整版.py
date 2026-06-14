# ============================================
# 留言板项目 - 后端服务（Flask API + 完整 CRUD）
# 日期：2026-06-16（第35天）
# 功能：完整的留言增删查功能，RESTful API 设计
# ============================================

"""
【项目说明】留言板后端（v3.0 完整版）

提供三个 API 接口：
1. GET    /api/messages     - 获取所有留言
2. POST   /api/messages     - 提交新留言
3. DELETE /api/messages/:id - 删除指定留言 ⭐ 新增

数据存储：JSON 文件（messages.json）
跨域处理：flask-cors
API 设计：RESTful 规范

升级内容（第35天）：
✅ 添加删除留言接口（DELETE）
✅ RESTful API 设计规范
✅ 路径参数（<int:id>）
✅ 404 错误处理（留言不存在）

运行方法：
1. 安装依赖：pip install flask flask-cors
2. 运行：python app_v3_完整版.py
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
messages = []

# 用于生成新留言的 id
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
        print(f"[ERROR] JSON 格式错误：{e}")
        print("   将创建新的空列表")
        messages = []
        next_id = 1

    except Exception as e:
        print(f"[ERROR] 加载失败：{e}")
        messages = []
        next_id = 1


def save_messages():
    """
    操作后：保存留言到 JSON 文件
    确保数据持久化
    """
    try:
        with open(DATA_FILE, 'w', encoding='utf-8') as f:
            json.dump(messages, f, indent=2, ensure_ascii=False)
        print(f"[SAVE] 已保存 {len(messages)} 条留言到 {DATA_FILE}")

    except Exception as e:
        print(f"[ERROR] 保存失败：{e}")


# --------------------------------------------
# API 1：获取所有留言（GET）
# --------------------------------------------

@app.route('/api/messages', methods=['GET'])
def get_messages():
    """
    获取所有留言
    返回：留言列表（JSON）
    """
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
    global next_id

    data = request.get_json()
    name = data.get('name', '匿名')
    content = data.get('content', '')

    # 验证：内容不能为空
    if not content.strip():
        return jsonify({"status": "error", "message": "留言内容不能为空"}), 400

    # 创建新留言
    new_message = {
        "id": next_id,
        "name": name,
        "content": content,
        "time": datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    }

    # 添加到列表
    messages.append(new_message)
    next_id += 1

    # 立即保存
    save_messages()

    print(f'[ADD] 新留言：{name} 说 "{content}"')

    return jsonify({
        "status": "success",
        "message": new_message
    })


# --------------------------------------------
# API 3：删除留言（DELETE）⭐ 新增
# --------------------------------------------

@app.route('/api/messages/<int:id>', methods=['DELETE'])
def delete_message(id):
    """
    删除指定留言
    路径参数：id（整数）
    返回：成功状态或 404 错误
    """
    global messages

    # 查找要删除的留言
    for i, msg in enumerate(messages):
        if msg['id'] == id:
            # 找到了，删除
            deleted = messages.pop(i)

            # 立即保存
            save_messages()

            print(f'[DELETE] 删除留言 id={id}：{deleted["name"]} 说 "{deleted["content"]}"')

            return jsonify({
                "status": "success",
                "deleted": deleted
            }), 200

    # 未找到，返回 404
    print(f'[DELETE] 留言 id={id} 不存在')
    return jsonify({
        "status": "error",
        "message": f"留言 id={id} 不存在"
    }), 404


# --------------------------------------------
# 首页路由（可选，方便测试）
# --------------------------------------------

@app.route('/')
def index():
    """首页，提示 API 用法"""
    return jsonify({
        "message": "留言板 API 服务（v3.0 完整版）",
        "version": "3.0 (with DELETE)",
        "data_file": DATA_FILE,
        "total_messages": len(messages),
        "endpoints": {
            "获取留言": "GET /api/messages",
            "提交留言": "POST /api/messages",
            "删除留言": "DELETE /api/messages/:id  ⭐ 新增"
        }
    })


# --------------------------------------------
# 启动服务
# --------------------------------------------

if __name__ == '__main__':
    print("=" * 50)
    print("留言板后端服务启动中（v3.0 完整版）...")
    print("=" * 50)

    # 启动前先加载数据
    load_messages()

    print("访问地址：http://localhost:5000")
    print("API 文档：http://localhost:5000")
    print(f"数据文件：{DATA_FILE}")
    print("=" * 50)
    print("新增功能：DELETE /api/messages/:id")
    print("=" * 50)

    app.run(debug=True, port=5000)
