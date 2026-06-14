# 📝 留言板项目 - 数据持久化升级

> 第34天更新 - 让留言真正"记住"！

## 🆕 v2.0 更新内容（2026-06-15）

### 核心升级：数据持久化

**问题**：v1.0 版本留言保存在内存，重启后数据丢失  
**解决**：v2.0 使用 JSON 文件持久化，重启后数据不丢失

### 新增功能

✅ **启动时自动加载**
- 从 `messages.json` 加载历史留言
- 文件不存在时初始化为空列表
- 自动计算 `next_id`（避免 id 冲突）

✅ **操作时实时保存**
- 每次提交留言立即保存到文件
- 确保数据不丢失

✅ **异常处理**
- 文件不存在：返回空列表（首次运行）
- JSON 格式错误：创建新文件
- 保存失败：记录错误但不影响用户

---

## 📂 文件说明

```
留言板项目/
├── app.py                    # v1.0 原始版本（内存存储）
├── app_v2_持久化版.py         # v2.0 持久化版本 ⭐ 推荐使用
├── index.html                # 前端页面（无需修改）
├── messages.json             # 数据文件（自动生成）
├── README.md                 # 原始说明
└── README_v2_持久化升级.md   # 本文件
```

---

## 🔧 代码改动详解

### 1. 新增数据文件路径

```python
# 数据文件路径
DATA_FILE = 'messages.json'
```

### 2. 新增 load_messages() 函数

```python
def load_messages():
    """启动时从文件加载"""
    global messages, next_id
    
    try:
        if not os.path.exists(DATA_FILE):
            # 文件不存在：首次运行
            messages = []
            next_id = 1
            return
        
        # 从文件加载
        with open(DATA_FILE, 'r', encoding='utf-8') as f:
            messages = json.load(f)
        
        # 计算 next_id
        if messages:
            next_id = max(msg['id'] for msg in messages) + 1
        else:
            next_id = 1
            
    except json.JSONDecodeError:
        # JSON 格式错误
        messages = []
        next_id = 1
```

### 3. 新增 save_messages() 函数

```python
def save_messages():
    """保存到文件"""
    with open(DATA_FILE, 'w', encoding='utf-8') as f:
        json.dump(messages, f, indent=2, ensure_ascii=False)
```

### 4. 修改启动流程

```python
if __name__ == '__main__':
    # 启动前先加载数据
    load_messages()
    app.run(debug=True, port=5000)
```

### 5. 修改 POST 接口

```python
@app.route('/api/messages', methods=['POST'])
def add_message():
    # ... 添加留言逻辑 ...
    
    # 【关键】立即保存！
    save_messages()
    
    return jsonify({"status": "success", "message": new_message})
```

---

## 🚀 使用方法

### 运行 v2.0 版本

```bash
# 进入项目目录
cd 留言板项目

# 运行持久化版本
python app_v2_持久化版.py
```

### 测试持久化

1. **提交几条留言**（通过前端或 API）
2. **停止服务**（Ctrl+C）
3. **重新启动**（`python app_v2_持久化版.py`）
4. **验证**：留言依然存在 ✅

---

## 🧪 测试结果

### 测试1：首次运行
```
启动 → messages.json 不存在 → 初始化为空列表 ✅
提交留言 → 保存到文件 ✅
```

### 测试2：重启验证
```
重启前：2条留言（Alice、Bob）
重启后：2条留言依然存在 ✅
新增留言：id 正确自增到 3 ✅
```

### 测试3：数据文件格式
```json
[
  {
    "id": 1,
    "name": "Alice",
    "content": "First persistent message",
    "time": "2026-06-14 01:45:50"
  },
  {
    "id": 2,
    "name": "Bob",
    "content": "Second persistent message",
    "time": "2026-06-14 01:45:50"
  }
]
```

格式化、可读 ✅

---

## 💡 核心知识点

### 数据持久化模式

**启动加载 + 操作保存**
```
程序启动 → load_messages() → 内存
                              ↓
用户操作 → 修改内存 → save_messages() → 文件
                              ↓
程序关闭 → 数据已保存
                              ↓
下次启动 → load_messages() → 数据恢复
```

### JSON 文件操作

```python
# 保存：Python → JSON
json.dump(data, f, indent=2, ensure_ascii=False)

# 加载：JSON → Python
data = json.load(f)
```

### 重点技巧

1. **encoding='utf-8'**：支持中文
2. **ensure_ascii=False**：保留中文可读
3. **indent=2**：格式化，方便调试
4. **try-except**：处理文件不存在
5. **global**：修改全局变量必须声明

---

## 🎯 下一步可以做什么？

- [ ] 添加删除留言功能（根据 id 删除）
- [ ] 添加自动备份（每次保存前备份到 .bak 文件）
- [ ] 升级到数据库（SQLite）
- [ ] 添加分页功能（只返回最新 10 条）
- [ ] 添加搜索功能（按关键词搜索留言）

---

**更新日期**：2026-06-15（第34天）  
**升级版本**：v2.0 (持久化版)  
**技术栈**：Flask + JSON 文件存储  
**测试状态**：✅ 全部通过
