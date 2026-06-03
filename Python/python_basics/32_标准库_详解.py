# ============================================
# 常用标准库基础 - 超详细注释版
# 日期：2026-05-30（第13天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【核心概念】Python 标准库（Standard Library）

是什么？
- 标准库是 Python 自带的模块集合
- 无需安装，导入即可使用
- 提供了大量常用功能

为什么需要？
- 避免重复造轮子
- 提高开发效率
- 功能稳定可靠
- 跨平台兼容

核心特点？
- 开箱即用：无需安装
- 功能丰富：覆盖常见需求
- 官方维护：稳定可靠
- 文档完善：易于学习
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 什么是标准库？
# --------------------------------------------

print("\n1.1 什么是标准库？")
print("-" * 60)

print("""
【标准库的定义】

标准库 = Python 自带的模块集合

比喻：
- 就像买手机自带的应用
- 不需要额外下载，开箱即用
- 功能丰富，覆盖常见需求

Python 标准库包含：
- 文件操作：os、shutil
- 时间处理：datetime、time
- 数据处理：json、csv、pickle
- 网络通信：urllib、http
- 数学计算：math、random
- 系统交互：sys、subprocess
- 还有100+个其他模块...
""")

# --------------------------------------------
# 1.2 今天学习的三个核心模块
# --------------------------------------------

print("\n1.2 今天学习的三个核心模块")
print("-" * 60)

print("""
【os 模块】
作用：操作系统接口
用途：文件和目录操作、路径处理、环境变量
重要性：⭐⭐⭐⭐⭐

【datetime 模块】
作用：日期和时间处理
用途：获取当前时间、日期计算、格式化
重要性：⭐⭐⭐⭐

【json 模块】
作用：JSON 数据处理
用途：数据序列化、配置文件、API 接口
重要性：⭐⭐⭐⭐⭐
""")

# ============================================
# 第二部分：os 模块详解 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第二部分：os 模块详解")
print("=" * 60)

# 导入 os 模块
import os

# --------------------------------------------
# 2.1 文件路径操作
# --------------------------------------------

print("\n2.1 文件路径操作")
print("-" * 60)

print("""
【os.path.join() 函数卡片】

作用：拼接路径
语法：os.path.join(path1, path2, ...)

为什么需要？
- 不同操作系统的路径分隔符不同
  Windows: \\
  Linux/Mac: /
- os.path.join 自动处理，跨平台兼容

示例：
path = os.path.join("folder", "subfolder", "file.txt")
# Windows: folder\\subfolder\\file.txt
# Linux: folder/subfolder/file.txt
""")

# 示例：拼接路径
folder = "my_project"
subfolder = "data"
filename = "config.json"
full_path = os.path.join(folder, subfolder, filename)
print(f"拼接后的路径：{full_path}")

print("\n" + "-" * 60)
print("""
【os.path.exists() 函数卡片】

作用：检查路径是否存在
语法：os.path.exists(path)
返回：True（存在）或 False（不存在）

用途：
- 在操作文件前检查是否存在
- 避免 FileNotFoundError

示例：
if os.path.exists("file.txt"):
    print("文件存在")
""")

# 示例：检查路径是否存在
print(f"当前目录是否存在：{os.path.exists('.')}")
print(f"不存在的文件：{os.path.exists('不存在的文件.txt')}")

print("\n" + "-" * 60)
print("""
【其他常用路径函数】

os.path.dirname(path)  # 获取目录名
os.path.basename(path) # 获取文件名
os.path.split(path)    # 分割路径
os.path.abspath(path)  # 获取绝对路径
os.path.isfile(path)   # 是否是文件
os.path.isdir(path)    # 是否是目录
""")

# 示例：路径操作
example_path = os.path.join("folder", "file.txt")
print(f"完整路径：{example_path}")
print(f"目录名：{os.path.dirname(example_path)}")
print(f"文件名：{os.path.basename(example_path)}")

# --------------------------------------------
# 2.2 目录操作
# --------------------------------------------

print("\n2.2 目录操作")
print("-" * 60)

print("""
【os.listdir() 函数卡片】

作用：列出目录中的所有文件和文件夹
语法：os.listdir(path)
返回：文件名列表

示例：
files = os.listdir(".")
print(files)  # ['file1.txt', 'folder1', ...]
""")

# 示例：列出当前目录
print("当前目录的内容（前5个）：")
try:
    items = os.listdir(".")[:5]
    for item in items:
        print(f"  - {item}")
except Exception as e:
    print(f"  错误：{e}")

print("\n" + "-" * 60)
print("""
【os.mkdir() 和 os.makedirs() 函数卡片】

os.mkdir(path)
- 作用：创建单个目录
- 注意：父目录必须存在

os.makedirs(path)
- 作用：创建多级目录
- 注意：自动创建所有父目录

示例：
os.mkdir("folder")              # 创建单个目录
os.makedirs("a/b/c")            # 创建多级目录
os.makedirs("a/b/c", exist_ok=True)  # 如果存在不报错
""")

# ============================================
# 第三部分：datetime 模块详解 ⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第三部分：datetime 模块详解")
print("=" * 60)

# 导入 datetime 模块
from datetime import datetime, date, time, timedelta

# --------------------------------------------
# 3.1 获取当前时间
# --------------------------------------------

print("\n3.1 获取当前时间")
print("-" * 60)

print("""
【datetime.now() 函数卡片】

作用：获取当前日期和时间
语法：datetime.now()
返回：datetime 对象

示例：
now = datetime.now()
print(now)  # 2026-05-30 14:30:25.123456
""")

# 示例：获取当前时间
now = datetime.now()
print(f"当前时间：{now}")
print(f"年份：{now.year}")
print(f"月份：{now.month}")
print(f"日期：{now.day}")
print(f"小时：{now.hour}")
print(f"分钟：{now.minute}")
print(f"秒数：{now.second}")

# --------------------------------------------
# 3.2 日期格式化
# --------------------------------------------

print("\n3.2 日期格式化")
print("-" * 60)

print("""
【strftime() 函数卡片】

作用：将日期对象转换为字符串（格式化）
语法：datetime_obj.strftime(format)

常用格式符号：
%Y - 四位年份（2026）
%m - 两位月份（01-12）
%d - 两位日期（01-31）
%H - 24小时制小时（00-23）
%M - 分钟（00-59）
%S - 秒（00-59）
%A - 星期几（Monday）
%B - 月份名（January）

示例：
now = datetime.now()
formatted = now.strftime("%Y-%m-%d %H:%M:%S")
print(formatted)  # 2026-05-30 14:30:25
""")

# 示例：日期格式化
now = datetime.now()
print(f"标准格式：{now.strftime('%Y-%m-%d %H:%M:%S')}")
print(f"中文格式：{now.strftime('%Y年%m月%d日 %H时%M分%S秒')}")
print(f"简短格式：{now.strftime('%Y/%m/%d')}")
print(f"时间格式：{now.strftime('%H:%M:%S')}")

# --------------------------------------------
# 3.3 日期解析
# --------------------------------------------

print("\n3.3 日期解析")
print("-" * 60)

print("""
【strptime() 函数卡片】

作用：将字符串转换为日期对象（解析）
语法：datetime.strptime(date_string, format)

示例：
date_str = "2026-05-30"
date_obj = datetime.strptime(date_str, "%Y-%m-%d")
print(date_obj)  # 2026-05-30 00:00:00
""")

# 示例：日期解析
date_str = "2026-05-30 14:30:25"
date_obj = datetime.strptime(date_str, "%Y-%m-%d %H:%M:%S")
print(f"解析后的日期对象：{date_obj}")
print(f"年份：{date_obj.year}")

# --------------------------------------------
# 3.4 时间差计算
# --------------------------------------------

print("\n3.4 时间差计算")
print("-" * 60)

print("""
【timedelta 函数卡片】

作用：表示时间差
语法：timedelta(days=0, hours=0, minutes=0, seconds=0)

用途：
- 日期加减运算
- 计算两个日期之间的差值

示例：
from datetime import timedelta

# 3天后
future = datetime.now() + timedelta(days=3)

# 2小时前
past = datetime.now() - timedelta(hours=2)

# 计算时间差
diff = date2 - date1
print(diff.days)  # 相差多少天
""")

# 示例：时间差计算
now = datetime.now()
tomorrow = now + timedelta(days=1)
yesterday = now - timedelta(days=1)
next_week = now + timedelta(weeks=1)

print(f"现在：{now.strftime('%Y-%m-%d %H:%M:%S')}")
print(f"明天：{tomorrow.strftime('%Y-%m-%d %H:%M:%S')}")
print(f"昨天：{yesterday.strftime('%Y-%m-%d %H:%M:%S')}")
print(f"下周：{next_week.strftime('%Y-%m-%d %H:%M:%S')}")

# 计算时间差
diff = tomorrow - now
print(f"明天距离现在：{diff.total_seconds()} 秒")

# ============================================
# 第四部分：json 模块详解 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：json 模块详解")
print("=" * 60)

# 导入 json 模块
import json

# --------------------------------------------
# 4.1 什么是 JSON？
# --------------------------------------------

print("\n4.1 什么是 JSON？")
print("-" * 60)

print("""
【JSON 的定义】

JSON = JavaScript Object Notation（JavaScript 对象表示法）

是什么？
- 一种轻量级的数据交换格式
- 易于人阅读和编写
- 易于机器解析和生成

为什么需要？
- API 接口常用 JSON 格式
- 配置文件常用 JSON 格式
- 数据存储和传输

JSON 格式示例：
{
    "name": "张三",
    "age": 25,
    "skills": ["Python", "Java"],
    "is_student": false
}
""")

# --------------------------------------------
# 4.2 序列化（Python → JSON）
# --------------------------------------------

print("\n4.2 序列化（Python → JSON）")
print("-" * 60)

print("""
【json.dumps() 函数卡片】

作用：将 Python 对象转换为 JSON 字符串
语法：json.dumps(obj, ensure_ascii=False, indent=None)

参数：
- obj：要转换的 Python 对象
- ensure_ascii：False 表示支持中文
- indent：缩进空格数（美化输出）

示例：
data = {"name": "张三", "age": 25}
json_str = json.dumps(data, ensure_ascii=False, indent=2)
print(json_str)
""")

# 示例：序列化
data = {
    "name": "张三",
    "age": 25,
    "skills": ["Python", "Java", "JavaScript"],
    "is_student": False,
    "score": 95.5
}

# 不美化
json_str = json.dumps(data, ensure_ascii=False)
print(f"不美化：{json_str}")

# 美化输出
json_str_pretty = json.dumps(data, ensure_ascii=False, indent=2)
print(f"美化输出：\n{json_str_pretty}")

# --------------------------------------------
# 4.3 反序列化（JSON → Python）
# --------------------------------------------

print("\n4.3 反序列化（JSON → Python）")
print("-" * 60)

print("""
【json.loads() 函数卡片】

作用：将 JSON 字符串转换为 Python 对象
语法：json.loads(json_string)

示例：
json_str = '{"name": "张三", "age": 25}'
data = json.loads(json_str)
print(data["name"])  # 张三
""")

# 示例：反序列化
json_str = '{"name": "李四", "age": 30, "skills": ["Python", "Go"]}'
data = json.loads(json_str)
print(f"解析后的数据：{data}")
print(f"姓名：{data['name']}")
print(f"年龄：{data['age']}")
print(f"技能：{data['skills']}")

# --------------------------------------------
# 4.4 文件操作
# --------------------------------------------

print("\n4.4 文件操作")
print("-" * 60)

print("""
【json.dump() 和 json.load() 函数卡片】

json.dump(obj, file)
- 作用：将 Python 对象写入 JSON 文件
- 示例：
  with open("data.json", "w", encoding="utf-8") as f:
      json.dump(data, f, ensure_ascii=False, indent=2)

json.load(file)
- 作用：从 JSON 文件读取数据
- 示例：
  with open("data.json", "r", encoding="utf-8") as f:
      data = json.load(f)
""")

# 示例：写入和读取 JSON 文件
print("演示：写入和读取 JSON 文件")
print("（实际项目中会真正操作文件）")

# ============================================
# 第五部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第五部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
错误1：路径拼接用字符串拼接

❌ 错误代码：
path = "folder" + "/" + "file.txt"

问题：
- 不同操作系统的路径分隔符不同
- Windows 用 \\，Linux/Mac 用 /
- 代码不跨平台

✅ 正确代码：
path = os.path.join("folder", "file.txt")

记忆技巧：
- 永远使用 os.path.join 拼接路径
- 自动处理不同操作系统的差异
""")

print("""
错误2：不检查文件是否存在就操作

❌ 错误代码：
os.remove("file.txt")  # 如果文件不存在会报错

问题：
- 文件不存在会抛出 FileNotFoundError

✅ 正确代码：
if os.path.exists("file.txt"):
    os.remove("file.txt")
else:
    print("文件不存在")

记忆技巧：
- 操作文件前先检查是否存在
- 或者用 try-except 处理异常
""")

print("""
错误3：日期格式化和解析的格式不匹配

❌ 错误代码：
date_str = "2026-05-30"
date_obj = datetime.strptime(date_str, "%Y/%m/%d")  # 格式不匹配

问题：
- 字符串是 "-" 分隔，格式用 "/" 分隔
- 会抛出 ValueError

✅ 正确代码：
date_str = "2026-05-30"
date_obj = datetime.strptime(date_str, "%Y-%m-%d")

记忆技巧：
- 格式字符串必须与日期字符串完全匹配
- 包括分隔符、顺序、位数
""")

print("""
错误4：JSON 序列化时不处理中文

❌ 错误代码：
data = {"name": "张三"}
json_str = json.dumps(data)
print(json_str)  # {"name": "\\u5f20\\u4e09"}

问题：
- 中文被转义成 Unicode 编码
- 不易阅读

✅ 正确代码：
json_str = json.dumps(data, ensure_ascii=False)
print(json_str)  # {"name": "张三"}

记忆技巧：
- 处理中文时总是加上 ensure_ascii=False
""")

print("""
错误5：忘记关闭文件

❌ 错误代码：
f = open("file.txt", "w")
f.write("content")
# 忘记 f.close()

问题：
- 文件可能没有正确保存
- 资源泄漏

✅ 正确代码：
with open("file.txt", "w") as f:
    f.write("content")
# 自动关闭文件

记忆技巧：
- 总是使用 with 语句操作文件
- 自动处理关闭，即使发生异常
""")

print("""
错误6：时间差计算单位搞混

❌ 错误代码：
diff = date2 - date1
print(diff)  # 以为是天数，其实是 timedelta 对象

问题：
- timedelta 对象不是简单的数字
- 需要用 .days 获取天数

✅ 正确代码：
diff = date2 - date1
print(diff.days)  # 天数
print(diff.total_seconds())  # 总秒数

记忆技巧：
- 时间差是 timedelta 对象
- 用 .days 获取天数，.total_seconds() 获取总秒数
""")

print("""
错误7：JSON 不支持的数据类型

❌ 错误代码：
data = {"date": datetime.now()}
json.dumps(data)  # TypeError

问题：
- JSON 不支持 datetime 对象
- 只支持：字符串、数字、布尔、列表、字典、None

✅ 正确代码：
data = {"date": datetime.now().strftime("%Y-%m-%d")}
json.dumps(data)

记忆技巧：
- 先将 datetime 转换为字符串
- 或者自定义 JSON 编码器
""")

# ============================================
# 第六部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第六部分：函数卡片速查")
print("=" * 60)

print("""
【os 模块】

os.path.join(path1, path2, ...)  # 拼接路径
os.path.exists(path)             # 检查路径是否存在
os.path.dirname(path)            # 获取目录名
os.path.basename(path)           # 获取文件名
os.path.isfile(path)             # 是否是文件
os.path.isdir(path)              # 是否是目录
os.listdir(path)                 # 列出目录内容
os.mkdir(path)                   # 创建单个目录
os.makedirs(path)                # 创建多级目录
os.remove(path)                  # 删除文件
os.rename(old, new)              # 重命名文件

────────────────────────────────────

【datetime 模块】

datetime.now()                   # 获取当前时间
datetime.strptime(str, format)   # 字符串 → 日期对象
datetime_obj.strftime(format)    # 日期对象 → 字符串
timedelta(days=0, hours=0, ...)  # 时间差

常用格式符号：
%Y - 四位年份    %m - 月份    %d - 日期
%H - 小时        %M - 分钟    %S - 秒

────────────────────────────────────

【json 模块】

json.dumps(obj, ensure_ascii=False, indent=2)  # Python → JSON 字符串
json.loads(json_string)                        # JSON 字符串 → Python
json.dump(obj, file, ...)                      # Python → JSON 文件
json.load(file)                                # JSON 文件 → Python
""")

# ============================================
# 第七部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第七部分：练习题（自己动手！）")
print("=" * 60)

print("""
练习1：文件路径工具
  要求：
  - 写一个函数 safe_join(*paths)
  - 拼接路径并检查是否存在
  - 如果不存在，返回 None 并打印提示
  提示：使用 os.path.join 和 os.path.exists

练习2：日期格式转换器
  要求：
  - 写一个函数 convert_date_format(date_str, from_format, to_format)
  - 将一种格式的日期字符串转换为另一种格式
  - 例如："2026-05-30" → "2026年05月30日"
  提示：使用 strptime 和 strftime

练习3：日期计算器
  要求：
  - 写一个函数 days_until(target_date_str)
  - 计算距离目标日期还有多少天
  - 返回天数（整数）
  提示：使用 datetime.now() 和 timedelta

练习4：配置文件管理器
  要求：
  - 写一个类 ConfigManager
  - 提供 load(filename) 方法读取 JSON 配置
  - 提供 save(filename) 方法保存配置
  - 提供 get(key, default) 方法获取配置项
  提示：使用 json.load 和 json.dump

练习5：日志记录器
  要求：
  - 写一个函数 log(message, level="INFO")
  - 记录日志到文件，格式：[时间] [级别] 消息
  - 例如：[2026-05-30 14:30:25] [INFO] 程序启动
  提示：使用 datetime.now().strftime() 和文件操作
""")

print("\n" + "=" * 60)
print("学习完成！")
print("=" * 60)
print("""
今天学习了标准库的核心知识：
1. 标准库的概念和优势
2. os 模块（文件和目录操作）
3. datetime 模块（日期和时间处理）
4. json 模块（JSON 数据处理）
5. 常见错误和最佳实践

下一步：
- 完成练习题
- 运行 33_标准库_练习.py 进行综合练习
""")









