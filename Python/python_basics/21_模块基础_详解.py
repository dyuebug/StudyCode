# ============================================
# 模块基础 - 超详细注释版
# 日期：2026-03-20（第8天）
# 难度：⭐⭐ 中级
# ============================================

"""
【核心概念】模块（Module）

是什么？
- 模块就是一个 .py 文件，里面包含函数、变量、类
- Python 自带很多模块（标准库），也可以自己写

为什么需要？
- 代码复用：别人写好的功能，直接导入使用
- 组织管理：把相关功能放在一个文件里
- 避免重复：不用重写已有的功能

核心特点：
- 用 import 语句导入
- Python 标准库包含 200+ 个模块
- 第三方库（pip安装）更是数以万计
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：import 三种导入方式")
print("=" * 60)

print("""
【import 三种方式】

方式1：import 模块名
  import math
  math.sqrt(16)     # 必须加模块名前缀
  优点：清晰，知道函数来自哪里
  缺点：每次都要写模块名

方式2：from 模块名 import 函数名
  from math import sqrt
  sqrt(16)          # 直接用，不需要前缀
  优点：方便，不用写前缀
  缺点：如果两个模块有同名函数会冲突

方式3：import 模块名 as 别名
  import math as m
  m.sqrt(16)        # 用别名代替模块名
  优点：模块名太长时可以缩短
  常见：import numpy as np, import pandas as pd
""")

# ============================================
# 第二部分：常用标准库实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：常用标准库实践")
print("=" * 60)

# --------------------------------------------
# 2.1 math 模块 - 数学运算
# --------------------------------------------

print("\n2.1 math 模块")
print("-" * 60)

# 导入 math 模块
# math 是 Python 内置的数学模块
import math

print("""
【math 模块常用函数】
  math.sqrt(x)      → 平方根
  math.pow(x, y)    → x的y次方
  math.floor(x)     → 向下取整
  math.ceil(x)      → 向上取整
  math.abs(x)       → 绝对值（内置函数 abs() 更常用）
  math.pi           → 圆周率 π
  math.e            → 自然常数 e
""")

# 使用 math 模块
print(f"sqrt(16) = {math.sqrt(16)}")          # 平方根：4.0
print(f"pow(2, 10) = {math.pow(2, 10)}")      # 2的10次方：1024.0
print(f"floor(3.7) = {math.floor(3.7)}")      # 向下取整：3
print(f"ceil(3.2) = {math.ceil(3.2)}")        # 向上取整：4
print(f"pi = {math.pi:.6f}")                  # 圆周率：3.141593
print(f"e = {math.e:.6f}")                    # 自然常数：2.718282

# 实际应用：计算圆的面积
radius = 5
area = math.pi * radius ** 2
print(f"\n半径为{radius}的圆，面积 = {area:.2f}")

# --------------------------------------------
# 2.2 random 模块 - 随机数
# --------------------------------------------

print("\n2.2 random 模块")
print("-" * 60)

import random

print("""
【random 模块常用函数】
  random.random()           → 0到1之间的随机浮点数
  random.randint(a, b)      → a到b之间的随机整数（含两端）
  random.choice(序列)       → 从序列中随机选一个
  random.shuffle(列表)      → 随机打乱列表（原地修改）
  random.sample(序列, k)    → 从序列中随机选k个（不重复）
""")

# random() - 0到1的随机浮点数
print(f"random() = {random.random():.4f}")

# randint(a, b) - 随机整数，含两端
print(f"randint(1, 6) = {random.randint(1, 6)}"  )  # 模拟骰子

# choice() - 随机选一个
fruits = ["苹果", "香蕉", "橙子", "葡萄", "西瓜"]
print(f"随机水果：{random.choice(fruits)}")

# shuffle() - 打乱列表
cards = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
random.shuffle(cards)  # 原地打乱，没有返回值
print(f"打乱后的牌：{cards}")

# sample() - 随机选k个（不重复）
lottery = random.sample(range(1, 36), 5)  # 从1-35中选5个
print(f"彩票号码：{sorted(lottery)}")

# --------------------------------------------
# 2.3 os 模块 - 操作系统接口
# --------------------------------------------

print("\n2.3 os 模块")
print("-" * 60)

import os

print("""
【os 模块常用功能】
  os.getcwd()               → 获取当前工作目录
  os.listdir(path)          → 列出目录内容
  os.path.exists(path)      → 判断路径是否存在
  os.path.join(a, b)        → 拼接路径（跨平台）
  os.path.basename(path)    → 获取文件名
  os.path.dirname(path)     → 获取目录名
  os.path.splitext(path)    → 分离文件名和扩展名
""")

# 获取当前工作目录
cwd = os.getcwd()
print(f"当前目录：{cwd}")

# 路径拼接（推荐用 os.path.join，跨平台兼容）
# 不要手动写 "/" 或 "\\"
path = os.path.join(cwd, "test.txt")
print(f"拼接路径：{path}")

# 判断文件是否存在
print(f"当前目录存在：{os.path.exists(cwd)}")
print(f"不存在的路径：{os.path.exists('/不存在的路径')}")

# 分离文件名和扩展名
filename = "hello_world.py"
name, ext = os.path.splitext(filename)
print(f"文件名：{name}，扩展名：{ext}")

# --------------------------------------------
# 2.4 datetime 模块 - 日期时间
# --------------------------------------------

print("\n2.4 datetime 模块")
print("-" * 60)

from datetime import datetime, date

print("""
【datetime 模块常用功能】
  datetime.now()            → 当前日期和时间
  date.today()              → 今天的日期
  dt.strftime(格式)         → 日期转字符串
  datetime.strptime(s, 格式) → 字符串转日期
""")

# 获取当前时间
now = datetime.now()
print(f"当前时间：{now}")

# 格式化输出
formatted = now.strftime("%Y年%m月%d日 %H:%M:%S")
print(f"格式化：{formatted}")

# 今天的日期
today = date.today()
print(f"今天：{today}")

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("\n" + "=" * 60)
print("第三部分：深入理解（选学，可跳过）")
print("=" * 60)

print("""
🔍 深入理解：Python 模块搜索路径

如果觉得难，可以跳过，不影响后续学习。

当你 import 一个模块时，Python 按以下顺序查找：
1. 当前目录
2. 环境变量 PYTHONPATH 中的目录
3. Python 标准库目录
4. site-packages（第三方库目录）

查看搜索路径：
  import sys
  print(sys.path)
""")

import sys
print("模块搜索路径（前3个）：")
for p in sys.path[:3]:
    print(f"  {p}")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
错误1：导入模块后忘记加前缀

❌ 错误代码：
  import math
  result = sqrt(16)   # NameError：sqrt 未定义

✅ 正确代码：
  import math
  result = math.sqrt(16)   # 必须加 math. 前缀

  或者：
  from math import sqrt
  result = sqrt(16)        # 这样才能直接用
""")

print("""
错误2：模块名和变量名冲突

❌ 错误代码：
  import random
  random = 42          # 覆盖了 random 模块！
  random.randint(1,6)  # AttributeError

✅ 正确代码：
  import random
  my_random = 42       # 用不同的变量名
  random.randint(1,6)  # 正常使用
""")

print("""
错误3：from module import * （不推荐）

❌ 不推荐：
  from math import *   # 导入所有内容
  from random import * # 可能覆盖 math 的同名函数！

✅ 推荐：
  import math
  import random        # 明确使用前缀，避免冲突
""")

print("""
错误4：os.path.join 和手动拼接路径

❌ 错误代码（Windows上可能失败）：
  path = folder + "/" + filename   # 手动拼接

✅ 正确代码（跨平台）：
  import os
  path = os.path.join(folder, filename)  # 自动处理分隔符
""")

# ============================================
# 第五部分：模块速查卡
# ============================================

print("\n" + "=" * 60)
print("第五部分：模块速查卡")
print("=" * 60)

print("""
【math 速查】
  math.sqrt(x)     平方根
  math.pow(x,y)    x^y
  math.floor(x)    向下取整
  math.ceil(x)     向上取整
  math.pi          π = 3.14159...

【random 速查】
  random.random()          0~1随机浮点
  random.randint(a,b)      随机整数[a,b]
  random.choice(seq)       随机选一个
  random.shuffle(list)     打乱列表
  random.sample(seq,k)     随机选k个

【os.path 速查】
  os.getcwd()              当前目录
  os.path.join(a,b)        拼接路径
  os.path.exists(p)        路径是否存在
  os.path.basename(p)      文件名
  os.path.splitext(p)      分离扩展名

【datetime 速查】
  datetime.now()           当前时间
  date.today()             今天日期
  dt.strftime("%Y-%m-%d")  格式化输出
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题（自己动手！）")
print("=" * 60)

print("""
练习1：猜数字游戏
  要求：用 random.randint() 生成1-100的随机数
       让用户猜，提示「太大了」或「太小了」
       猜对后显示猜了几次
  提示：用 while 循环 + input() + int() 转换

练习2：计算器升级版
  要求：用 math 模块实现：
       - 平方根计算
       - 幂运算
       - 圆的面积和周长计算
  提示：从 math 导入需要的函数

练习3：文件列表工具
  要求：用 os 模块列出当前目录的所有 .py 文件
       显示每个文件的名称
  提示：os.listdir() + 字符串的 endswith() 方法

练习4：时间戳生成器
  要求：用 datetime 模块生成当前时间的字符串
       格式："20260320_143052"（年月日_时分秒）
  提示：strftime("%Y%m%d_%H%M%S")
""")
