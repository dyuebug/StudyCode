# ============================================
# 数据持久化 - 超详细注释版
# 日期：2026-06-15（第34天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【核心概念】数据持久化

是什么？
- 把内存中的数据保存到磁盘（文件、数据库）
- 程序重启后，数据还在
- 类比：把笔记写在纸上，而不是只记在脑子里

为什么需要？
- 内存数据：程序关闭就丢失（易失性）
- 持久化：数据永久保存（持久性）
- 实际应用：用户数据、配置、日志等都需要保存

核心特点？
- 内存：快但易失
- 磁盘：慢但持久
- 权衡：重要数据必须持久化
"""

print("""
============================================
数据持久化详解
============================================

今天的学习目标：
1. 理解数据持久化的概念和价值
2. 掌握 JSON 文件读写（json.dump/load）
3. 学会"启动加载、操作保存"模式
4. 改造留言板项目，让数据持久化

重要提示：
- 这是实用技能，所有项目都会用到
- JSON 是最常用的数据交换格式
- 注意文件不存在、编码等异常处理
""")

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 什么是数据持久化？
# --------------------------------------------

print("\n1.1 什么是数据持久化？")
print("-" * 60)

print("""
【数据持久化】

生活例子：记笔记
- 只记在脑子里 = 内存（睡一觉可能就忘了）
- 写在笔记本上 = 持久化（永久保存）

程序中的两种数据：
1. 内存数据（易失性）
   - 存在变量、列表、字典中
   - 程序关闭 → 数据消失
   - 优点：快
   - 缺点：不持久

2. 持久化数据（永久性）
   - 保存到文件或数据库
   - 程序关闭 → 数据还在
   - 优点：持久
   - 缺点：相对慢

例子：留言板的两种存储
┌──────────────────────────────────┐
│ 内存存储（昨天的留言板）           │
├──────────────────────────────────┤
│ messages = []                    │
│ 运行中：可以添加、查看留言         │
│ 重启后：留言全部消失 ❌            │
└──────────────────────────────────┘

┌──────────────────────────────────┐
│ 持久化存储（今天要做的）           │
├──────────────────────────────────┤
│ messages = []                    │
│ 保存到：messages.json 文件        │
│ 运行中：可以添加、查看留言         │
│ 重启后：从文件加载，留言还在 ✅    │
└──────────────────────────────────┘

什么数据需要持久化？
- 用户信息：账号、密码、资料
- 业务数据：订单、留言、文章
- 配置信息：设置、偏好
- 日志记录：操作历史、错误日志
""")

# --------------------------------------------
# 1.2 常见持久化方式
# --------------------------------------------

print("\n1.2 常见持久化方式")
print("-" * 60)

print("""
【持久化方式对比】

1. 文本文件（.txt）
   - 优点：简单直观
   - 缺点：结构化数据不方便
   - 适用：日志、配置

2. JSON 文件（.json）⭐⭐⭐⭐⭐
   - 优点：结构化、易读、跨语言
   - 缺点：大数据慢
   - 适用：配置、小规模数据
   - 今天学的！

3. CSV 文件（.csv）
   - 优点：表格数据、Excel 可打开
   - 缺点：只能二维表格
   - 适用：数据导入导出

4. 数据库（SQLite、MySQL）
   - 优点：查询快、数据量大
   - 缺点：复杂
   - 适用：生产环境、大数据

为什么选择 JSON？
- ✅ 和 Python 字典/列表完美匹配
- ✅ 人类可读（打开文件能看懂）
- ✅ 前后端通用（API 也用 JSON）
- ✅ 内置 json 模块，无需安装

JSON 示例（messages.json）：
[
  {
    "id": 1,
    "name": "小明",
    "content": "你好世界",
    "time": "2026-06-14 10:00:00"
  },
  {
    "id": 2,
    "name": "小红",
    "content": "留言板真棒",
    "time": "2026-06-14 11:00:00"
  }
]
""")

# --------------------------------------------
# 1.3 函数卡片：json.dump() ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.3 json.dump() 函数详解")
print("-" * 60)

print("""
【函数卡片】

函数名：json.dump()
作用：把 Python 数据保存到 JSON 文件
类型：json 模块函数

完整语法：
import json
json.dump(obj, fp, indent=None, ensure_ascii=True)

参数详解：
1. obj（必需）
   - 类型：Python 对象（字典、列表等）
   - 作用：要保存的数据
   - 示例：messages 列表

2. fp（必需）
   - 类型：文件对象
   - 作用：打开的文件（写模式）
   - 示例：open('data.json', 'w')

3. indent（可选）
   - 类型：整数
   - 作用：缩进空格数（美化格式）
   - 示例：indent=2（每层缩进2个空格）
   - 不设置：一行显示（压缩）

4. ensure_ascii（可选）
   - 类型：布尔值
   - 作用：是否转义非 ASCII 字符
   - 默认：True（中文变成 \\uXXXX）
   - 推荐：False（保留中文）

返回值：无

基本用法：
```python
import json

# 准备数据
data = [
    {"name": "小明", "age": 18},
    {"name": "小红", "age": 20}
]

# 保存到文件
with open('data.json', 'w', encoding='utf-8') as f:
    json.dump(data, f, indent=2, ensure_ascii=False)
```

生成的文件（data.json）：
[
  {
    "name": "小明",
    "age": 18
  },
  {
    "name": "小红",
    "age": 20
  }
]

重点：
- 必须用 'w' 模式打开文件
- encoding='utf-8' 支持中文
- ensure_ascii=False 保留中文可读
- indent=2 让文件格式美观
""")

# --------------------------------------------
# 1.4 函数卡片：json.load() ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.4 json.load() 函数详解")
print("-" * 60)

print("""
【函数卡片】

函数名：json.load()
作用：从 JSON 文件加载数据到 Python
类型：json 模块函数

完整语法：
import json
data = json.load(fp)

参数详解：
1. fp（必需）
   - 类型：文件对象
   - 作用：打开的文件（读模式）
   - 示例：open('data.json', 'r')

返回值：
- Python 对象（字典、列表等）
- JSON 自动转成对应的 Python 类型

类型转换（JSON → Python）：
JSON          →  Python
────────────────────────
object        →  dict
array         →  list
string        →  str
number (int)  →  int
number (real) →  float
true          →  True
false         →  False
null          →  None

基本用法：
```python
import json

# 从文件加载
with open('data.json', 'r', encoding='utf-8') as f:
    data = json.load(f)

# data 现在是 Python 列表/字典
print(data)
```

重点：
- 必须用 'r' 模式打开文件
- encoding='utf-8' 支持中文
- 返回的是 Python 原生类型
- 可以直接用索引、循环等操作

对比：json.load() vs json.loads()
- json.load(文件对象)：从文件读
- json.loads(字符串)：从字符串读
""")

# ============================================
# 第二部分：基础操作实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 示例1：保存数据到 JSON 文件
# --------------------------------------------

print("\n2.1 示例1：保存数据到 JSON 文件")
print("-" * 60)

# 导入 json 模块
import json

# 准备数据（列表套字典）
students = [
    {"id": 1, "name": "小明", "score": 85},
    {"id": 2, "name": "小红", "score": 92},
    {"id": 3, "name": "小刚", "score": 78}
]

print("原始数据：")
print(students)

# 保存到文件
# 参数1：students - 要保存的数据
# 参数2：f - 文件对象
# 参数3：indent=2 - 每层缩进2个空格（美化格式）
# 参数4：ensure_ascii=False - 保留中文（不转义）
with open('students.json', 'w', encoding='utf-8') as f:
    json.dump(students, f, indent=2, ensure_ascii=False)

print("\n已保存到 students.json 文件")
print("打开文件可以看到格式化的 JSON 数据")

# --------------------------------------------
# 2.2 示例2：从 JSON 文件加载数据
# --------------------------------------------

print("\n2.2 示例2：从 JSON 文件加载数据")
print("-" * 60)

# 从文件加载
# 参数：f - 文件对象（读模式）
# 返回：Python 列表
with open('students.json', 'r', encoding='utf-8') as f:
    loaded_data = json.load(f)

print("从文件加载的数据：")
print(loaded_data)

# 验证：loaded_data 是普通的 Python 列表
print(f"\n数据类型：{type(loaded_data)}")
print(f"第一个学生：{loaded_data[0]}")
print(f"小红的成绩：{loaded_data[1]['score']}")

# --------------------------------------------
# 2.3 示例3：异常处理（文件不存在）⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n2.3 示例3：异常处理（文件不存在）")
print("-" * 60)

print("""
【重点】第一次运行时，文件不存在怎么办？

问题场景：
- 留言板第一次启动
- messages.json 文件还不存在
- 直接 json.load() 会报错 FileNotFoundError

解决方案：用 try-except 处理
""")

def load_messages_safe():
    """
    安全加载留言
    文件不存在时返回空列表
    """
    try:
        # 尝试打开文件
        with open('messages.json', 'r', encoding='utf-8') as f:
            # 加载数据
            messages = json.load(f)
            print(f"✅ 成功加载 {len(messages)} 条留言")
            return messages
    except FileNotFoundError:
        # 文件不存在：第一次运行
        print("⚠️  文件不存在，返回空列表（第一次运行）")
        return []
    except json.JSONDecodeError:
        # JSON 格式错误：文件损坏
        print("❌ JSON 格式错误，返回空列表（文件损坏）")
        return []

# 测试：第一次运行（文件不存在）
print("\n测试1：文件不存在")
result1 = load_messages_safe()
print(f"返回结果：{result1}")

# 先保存一些数据
test_data = [{"id": 1, "msg": "测试"}]
with open('messages.json', 'w', encoding='utf-8') as f:
    json.dump(test_data, f, ensure_ascii=False)

# 测试：文件存在
print("\n测试2：文件存在")
result2 = load_messages_safe()
print(f"返回结果：{result2}")

# --------------------------------------------
# 2.4 示例4：完整的持久化模式 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n2.4 示例4：完整的持久化模式")
print("-" * 60)

print("""
【持久化模式】启动加载 + 操作保存

步骤：
1. 程序启动：从文件加载数据到内存
2. 程序运行：在内存中修改数据
3. 数据修改：立即保存到文件
4. 程序关闭：数据已保存，下次启动能加载

示例：简单的待办清单
""")

# 全局变量：待办清单
todos = []

def load_todos():
    """启动时：从文件加载"""
    global todos
    try:
        with open('todos.json', 'r', encoding='utf-8') as f:
            todos = json.load(f)
            print(f"✅ 加载了 {len(todos)} 条待办")
    except FileNotFoundError:
        todos = []
        print("⚠️  首次运行，待办清单为空")

def save_todos():
    """操作后：保存到文件"""
    with open('todos.json', 'w', encoding='utf-8') as f:
        json.dump(todos, f, indent=2, ensure_ascii=False)
    print(f"💾 已保存 {len(todos)} 条待办")

def add_todo(task):
    """添加待办"""
    todos.append({"id": len(todos) + 1, "task": task, "done": False})
    save_todos()  # 立即保存！

def list_todos():
    """查看所有待办"""
    if not todos:
        print("暂无待办")
    else:
        for todo in todos:
            status = "✓" if todo["done"] else "○"
            print(f"{status} {todo['id']}. {todo['task']}")

# 模拟程序生命周期
print("\n--- 第一次运行 ---")
load_todos()  # 启动时加载
add_todo("学习 Python")
add_todo("写代码")
list_todos()

print("\n--- 模拟重启（清空内存） ---")
todos = []  # 模拟程序关闭，内存清空
print("内存已清空，todos =", todos)

print("\n--- 第二次运行（重启后） ---")
load_todos()  # 重新加载，数据还在！
list_todos()
print("\n✅ 数据持久化成功！重启后数据不丢失")

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("\n" + "=" * 60)
print("🔍 深入理解：JSON vs pickle vs 数据库（选学）")
print("=" * 60)

print("""
==============================================
🔍 深入理解：持久化方式对比（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【三种持久化方式对比】

1. JSON（json 模块）⭐⭐⭐⭐⭐
   优点：
   - 人类可读（打开文件能看懂）
   - 跨语言（前后端通用）
   - 简单易用

   缺点：
   - 只支持基本类型（dict/list/str/int/float/bool/None）
   - 不支持自定义类、函数、日期等
   - 大数据慢

   适用：配置文件、小规模数据、API 数据交换

────────────────────────────────────

2. pickle（pickle 模块）
   优点：
   - 支持几乎所有 Python 对象
   - 包括自定义类、函数等
   - 速度快

   缺点：
   - 不可读（二进制格式）
   - 只能 Python 用（不跨语言）
   - 不安全（不要加载不信任的 pickle 文件）

   适用：Python 专用、复杂对象、临时缓存

────────────────────────────────────

3. 数据库（SQLite、MySQL）
   优点：
   - 支持大数据量
   - 查询快（索引、SQL）
   - 并发访问
   - 数据完整性保证

   缺点：
   - 复杂（需要学 SQL）
   - 重量级

   适用：生产环境、大规模数据、多用户

【选择建议】

小项目（< 1000条数据）：
- 配置/设置 → JSON
- 缓存/临时 → pickle
- 日志 → 文本文件

中型项目（1000-10万条）：
- SQLite（单机数据库）

大型项目（> 10万条）：
- MySQL/PostgreSQL（服务器数据库）

今天的留言板：用 JSON（简单、够用）

==============================================
""")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

# --------------------------------------------
# 4.1 错误1：忘记 ensure_ascii=False（中文乱码）
# --------------------------------------------

print("\n4.1 错误1：忘记 ensure_ascii=False（中文乱码）")
print("-" * 60)

print("""
❌ 错误代码：
```python
data = {"name": "小明", "msg": "你好"}
with open('data.json', 'w', encoding='utf-8') as f:
    json.dump(data, f)  # 缺少 ensure_ascii=False
```

结果：文件中中文变成 \\uXXXX
{"name": "\\u5c0f\\u660e", "msg": "\\u4f60\\u597d"}

原因：
- ensure_ascii 默认 True
- 把非 ASCII 字符（中文）转义成 Unicode 编码

✅ 正确代码：
```python
data = {"name": "小明", "msg": "你好"}
with open('data.json', 'w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False)  # 保留中文
```

结果：文件中保留中文
{"name": "小明", "msg": "你好"}

记忆技巧：save 中文必备 ensure_ascii=False
""")

# --------------------------------------------
# 4.2 错误2：文件不存在时直接 load（未处理异常）
# --------------------------------------------

print("\n4.2 错误2：文件不存在时直接 load（未处理异常）")
print("-" * 60)

print("""
❌ 错误代码：
```python
with open('data.json', 'r') as f:
    data = json.load(f)  # 文件不存在 → FileNotFoundError
```

结果：程序崩溃
FileNotFoundError: [Errno 2] No such file or directory: 'data.json'

原因：第一次运行时文件不存在

✅ 正确代码：
```python
try:
    with open('data.json', 'r', encoding='utf-8') as f:
        data = json.load(f)
except FileNotFoundError:
    data = []  # 文件不存在，返回空列表
```

预防措施：加载数据必须用 try-except 处理文件不存在
""")

# --------------------------------------------
# 4.3 错误3：忘记 encoding='utf-8'（Windows 中文问题）
# --------------------------------------------

print("\n4.3 错误3：忘记 encoding='utf-8'（Windows 中文问题）")
print("-" * 60)

print("""
❌ 错误代码（Windows 系统）：
```python
with open('data.json', 'w') as f:  # 缺少 encoding='utf-8'
    json.dump(data, f, ensure_ascii=False)
```

结果：Windows 默认 GBK 编码，中文可能出错

原因：
- Windows 默认编码是 GBK（中国）或其他
- 保存和加载编码不一致导致乱码

✅ 正确代码：
```python
# 保存时
with open('data.json', 'w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False)

# 加载时
with open('data.json', 'r', encoding='utf-8') as f:
    data = json.load(f)
```

记忆技巧：文件操作永远加 encoding='utf-8'
""")

# --------------------------------------------
# 4.4 错误4：修改全局变量忘记 global 关键字
# --------------------------------------------

print("\n4.4 错误4：修改全局变量忘记 global 关键字")
print("-" * 60)

print("""
❌ 错误代码：
```python
messages = []  # 全局变量

def load_messages():
    # 忘记 global！
    messages = json.load(...)  # 创建了局部变量
    # 全局的 messages 没有改变

load_messages()
print(messages)  # 还是 []，没有加载成功
```

结果：全局变量没有被修改

原因：
- Python 函数内赋值默认创建局部变量
- 需要 global 声明才能修改全局变量

✅ 正确代码：
```python
messages = []  # 全局变量

def load_messages():
    global messages  # 声明要修改全局变量
    try:
        with open('messages.json', 'r', encoding='utf-8') as f:
            messages = json.load(f)
    except FileNotFoundError:
        messages = []

load_messages()
print(messages)  # 成功加载
```

记忆技巧：
- 只读全局变量：不需要 global
- 修改全局变量：必须 global
""")

# --------------------------------------------
# 4.5 错误5：忘记 indent 参数（文件难读）
# --------------------------------------------

print("\n4.5 错误5：忘记 indent 参数（文件难读）")
print("-" * 60)

print("""
❌ 错误代码：
```python
json.dump(data, f)  # 没有 indent
```

结果：所有内容挤在一行
[{"id":1,"name":"小明"},{"id":2,"name":"小红"}]

难以阅读和调试

✅ 正确代码：
```python
json.dump(data, f, indent=2)  # 每层缩进2个空格
```

结果：格式化、易读
[
  {
    "id": 1,
    "name": "小明"
  },
  {
    "id": 2,
    "name": "小红"
  }
]

记忆技巧：indent=2 让文件可读，方便调试
""")

# --------------------------------------------
# 4.6 错误6：保存后不立即读取验证
# --------------------------------------------

print("\n4.6 错误6：保存后不立即读取验证")
print("-" * 60)

print("""
❌ 常见情况：
- 保存数据后没验证
- 程序以为保存成功
- 其实文件是空的或格式错误

✅ 最佳实践：保存后验证
```python
# 保存
with open('data.json', 'w', encoding='utf-8') as f:
    json.dump(data, f, indent=2, ensure_ascii=False)

# 立即读取验证
with open('data.json', 'r', encoding='utf-8') as f:
    verify = json.load(f)
    assert verify == data, "保存失败！数据不一致"
    print("✅ 保存成功，已验证")
```

预防措施：
- 关键数据保存后验证
- 开发时多检查生成的文件
- 测试时重启几次确保数据持久
""")

# --------------------------------------------
# 4.7 错误7：多次保存性能问题
# --------------------------------------------

print("\n4.7 错误7：多次保存性能问题")
print("-" * 60)

print("""
❌ 低效代码：
```python
# 添加100条留言，每次都保存文件
for i in range(100):
    messages.append(new_message)
    save_messages()  # 写文件100次！
```

结果：慢！每次写文件都有 I/O 开销

✅ 优化方案1：批量保存
```python
# 添加完所有留言，保存一次
for i in range(100):
    messages.append(new_message)
save_messages()  # 只写一次
```

✅ 优化方案2：定时保存
```python
# 每10次操作保存一次
count = 0
for i in range(100):
    messages.append(new_message)
    count += 1
    if count >= 10:
        save_messages()
        count = 0
# 最后保存剩余的
save_messages()
```

权衡：
- 实时保存：数据安全，但慢
- 批量保存：快，但崩溃时可能丢数据
- 留言板用实时保存（数据重要、操作不频繁）
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【JSON 文件操作】

────────────────────────────────────

json.dump(obj, f, indent=2, ensure_ascii=False)
- 作用：保存数据到 JSON 文件
- 参数：
  * obj：要保存的数据
  * f：文件对象（'w' 模式）
  * indent：缩进空格数
  * ensure_ascii：False 保留中文
- 示例：
  with open('data.json', 'w', encoding='utf-8') as f:
      json.dump(data, f, indent=2, ensure_ascii=False)

────────────────────────────────────

json.load(f)
- 作用：从 JSON 文件加载数据
- 参数：f - 文件对象（'r' 模式）
- 返回：Python 对象（dict/list）
- 示例：
  with open('data.json', 'r', encoding='utf-8') as f:
      data = json.load(f)

────────────────────────────────────

json.dumps(obj, indent=2, ensure_ascii=False)
- 作用：把对象转成 JSON 字符串
- 参数：同 dump
- 返回：JSON 字符串
- 示例：
  json_str = json.dumps(data, indent=2, ensure_ascii=False)

────────────────────────────────────

json.loads(s)
- 作用：把 JSON 字符串转成对象
- 参数：s - JSON 字符串
- 返回：Python 对象
- 示例：
  data = json.loads('{"name": "小明"}')

════════════════════════════════════

【记忆口诀】

dump/dumps = 保存（d = down，往下写）
load/loads = 加载（l = load，往上读）

带 s = 字符串（string）
不带 s = 文件（file）

中文必备三件套：
1. encoding='utf-8'
2. ensure_ascii=False
3. indent=2（可读）
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题")
print("=" * 60)

print("""
练习1：保存和加载配置
题目：创建一个程序，保存用户配置到 config.json。
要求：
  - 配置包括：username、theme（主题）、font_size
  - 提供 save_config() 和 load_config() 函数
  - 第一次运行返回默认配置

提示：
  - 用字典存储配置
  - load 时 try-except 处理文件不存在
  - 默认配置：{"username": "Guest", "theme": "light", "font_size": 14}

────────────────────────────────────

练习2：学生成绩管理（持久化版）
题目：改造之前的学生成绩管理器，添加数据持久化。
要求：
  - 启动时从 students.json 加载
  - 添加/修改学生后立即保存
  - 重启后数据不丢失

提示：
  - students = []  # 全局变量
  - load_students() 启动时调用
  - save_students() 每次修改后调用

────────────────────────────────────

练习3：笔记本应用
题目：创建一个简单的笔记本，支持添加、查看、保存笔记。
要求：
  - 每条笔记：id、title、content、time
  - 数据保存到 notes.json
  - 提供：add_note()、list_notes()、save()、load()

提示：
  - 用列表存储所有笔记
  - 添加笔记时自动生成 id 和 time
  - 启动时自动加载

────────────────────────────────────

练习4：改造留言板（今天的重点）
题目：给昨天的留言板添加数据持久化。
要求：
  - 修改 app.py：添加 load_messages() 和 save_messages()
  - 启动时自动加载 messages.json
  - POST 提交后立即保存
  - 测试：提交留言 → 重启 → 验证数据还在

提示：
  - 文件路径：'messages.json'（和 app.py 同目录）
  - global messages 声明
  - next_id 从现有留言中计算：max(m['id']) + 1

────────────────────────────────────

练习5：自动备份
题目：每次保存时，自动备份上一个版本。
要求：
  - 保存到 data.json
  - 同时保存备份到 data.json.bak
  - 如果保存失败，可以从备份恢复

提示：
  - import shutil
  - shutil.copy('data.json', 'data.json.bak')
  - 先备份再保存新数据
""")

print("\n" + "=" * 60)
print("恭喜！你已经掌握了数据持久化的核心知识！")
print("接下来：动手改造留言板项目，让数据真正保存！")
print("=" * 60)

