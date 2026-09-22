# 📝 留言板项目（前后端联动）

> 第33天实战项目 - 第一次打通前后端！

## 📖 项目介绍

一个简单的留言板，演示**前后端分离**的完整数据交互：
- **后端**（Flask）：提供留言的获取和提交 API
- **前端**（HTML/CSS/JS）：用 fetch 调用后端，动态显示留言

## 🗂️ 文件结构

```
留言板项目/
├── app.py                    # v1.0 内存存储版
├── app_v2_持久化版.py         # v2.0 JSON 持久化版
├── app_v3_完整版.py           # v3.0 RESTful API 版
├── app_v4_sqlite.py          # v4.0 SQLite 持久化版
├── test_app_v4_sqlite.py     # v4.0 API 自动化测试
├── index.html                # 前端页面
├── messages.json             # v2/v3 数据文件
├── README_v2_持久化升级.md    # v2.0 说明
├── README_v4_sqlite.md       # v4.0 说明
└── README.md                 # 本说明文档
```

## 🔌 API 接口

| 方法 | 路径 | 功能 | 请求数据 | 返回数据 |
|------|------|------|---------|---------|
| GET | `/api/messages` | 获取所有留言 | 无 | 留言数组 |
| POST | `/api/messages` | 提交新留言 | `{name, content}` | 成功状态 |

## 🚀 运行步骤

### 第1步：安装依赖

```bash
pip install flask flask-cors
```

### 第2步：启动后端

```bash
cd 留言板项目
python app.py
```

看到提示：
```
留言板后端服务启动中...
访问地址：http://localhost:5000
```

### 第3步：打开前端

直接双击 `index.html`，或用浏览器打开。

### 第4步：测试功能

1. ✅ 页面自动加载已有留言
2. ✅ 输入名字和内容，点击"发表留言"
3. ✅ 留言立即显示在列表顶部（无需刷新页面）

## 🔍 数据流转图

```
┌─────────────────────────────────────────────┐
│  用户在前端输入留言，点击"发表留言"            │
└──────────────────┬──────────────────────────┘
                   │ fetch POST
                   │ body: {name, content}
                   ↓
┌─────────────────────────────────────────────┐
│  Flask 后端接收（request.get_json）           │
│  保存到 messages 列表                          │
│  返回成功状态（jsonify）                       │
└──────────────────┬──────────────────────────┘
                   │ 提交成功后
                   │ 前端再次 fetch GET
                   ↓
┌─────────────────────────────────────────────┐
│  Flask 返回所有留言（jsonify）                 │
│  前端用 DOM 渲染显示                           │
└─────────────────────────────────────────────┘
```

## 💡 核心知识点

### 后端（Flask）
- `jsonify()`：返回 JSON 数据
- `request.get_json()`：获取前端 POST 数据
- `CORS(app)`：解决跨域问题
- `methods=['GET', 'POST']`：指定请求方法

### 前端（fetch）
- `fetch(url)`：GET 请求获取数据
- `fetch(url, {method, headers, body})`：POST 提交数据
- `JSON.stringify()`：对象转 JSON 字符串
- `response.json()`：解析后端返回的 JSON

## 🎯 可以扩展的功能

学有余力可以尝试：
- [x] 添加删除留言功能（v3.0）
- [x] 留言数据保存到文件（v2/v3）或数据库（v4.0 SQLite）
- [ ] 添加点赞功能
- [ ] 留言分页显示
- [ ] 输入内容长度限制和敏感词过滤

## ⚠️ 常见问题

**Q：点击按钮没反应，控制台报 CORS 错误？**
A：确认后端 `app.py` 有 `CORS(app)`，并已安装 flask-cors。

**Q：提示"加载失败"？**
A：确认后端已启动（运行 `python app.py`），端口是 5000。

**Q：运行哪个版本才能持久化？**
A：`app.py` 是内存版；`app_v2_持久化版.py` 和 `app_v3_完整版.py` 使用 JSON；当前推荐 `app_v4_sqlite.py`，使用 SQLite 持久化。

---

**项目起始日期**：2026-06-14（第33天）
**当前版本**：v4.0 SQLite 持久化版
**技术栈**：Flask + flask-cors + HTML/CSS/JavaScript + fetch + SQLite
**当前状态**：v1～v4 已完成；v4.0 API 测试已建立
