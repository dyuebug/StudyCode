# ============================================
# 函数基础 - 超详细注释版
# 日期：2026-03-20（第7天）
# 难度：⭐⭐ 中级
# ============================================

"""
【核心概念】函数（Function）

是什么？
- 函数是一段有名字的、可以重复调用的代码块
- 用 def 关键字定义，用函数名加括号调用

为什么需要？
- 代码复用：写一次，用多次，避免重复
- 模块化：把复杂问题拆分成小块，各自解决
- 易维护：修改一处，处处生效

核心特点：
- 可以接收输入（参数）
- 可以返回输出（返回值）
- 内部变量与外部隔离（作用域）
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 函数是什么？
# --------------------------------------------

print("\n1.1 函数是什么？")
print("-" * 60)

print("""
【函数】

生活类比：
- 函数就像一台「自动机器」
- 你放入原材料（参数）
- 机器自动加工
- 输出成品（返回值）

没有函数时：
  # 计算两个数的和（第1次）
  a = 3
  b = 5
  result = a + b
  print(result)

  # 计算两个数的和（第2次）
  a = 10
  b = 20
  result = a + b
  print(result)

  # 重复代码！维护困难！

有了函数：
  def add(a, b):    # 定义一次
      return a + b

  add(3, 5)         # 调用多次
  add(10, 20)
  add(100, 200)
""")

# --------------------------------------------
# 1.2 def 关键字详解 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.2 def 关键字 - 定义函数")
print("-" * 60)

print("""
【def 语法卡片】

完整语法：
  def 函数名(参数1, 参数2, ...):
      # 文档字符串（可选）
      函数体
      return 返回值（可选）

各部分说明：
  def         → Python关键字，告诉Python「我要定义一个函数」
  函数名      → 自己起的名字，动词开头，如 calculate、get、check
  (参数)      → 函数的输入，可以没有，写成 ()
  :           → 冒号，必须有！
  函数体      → 缩进4个空格，函数的实际代码
  return      → 返回结果（可选，没有则返回 None）

命名规范：
  def calculate_sum()   → 动词+名词，下划线分隔
  def get_name()        → 动词开头
  def is_valid()        → is开头表示判断
  不用大写开头如 Sum()
  不用中文如 求和()
""")


# ============================================
# 第二部分：基础操作实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 最简单的函数
# --------------------------------------------

print("\n2.1 最简单的函数")
print("-" * 60)

# 定义函数：使用 def 关键字
# greet 是函数名
# () 里没有参数，表示不需要输入
def greet():
    # 函数体：缩进4个空格
    # 这里的代码只有调用函数时才执行
    print("你好，欢迎学习Python！")

# 调用函数：函数名 + ()
# 不调用，函数不会自动执行
greet()   # 输出：你好，欢迎学习Python！
greet()   # 可以多次调用
greet()   # 每次都执行函数体里的代码

# --------------------------------------------
# 2.2 带参数的函数
# --------------------------------------------

print("\n2.2 带参数的函数")
print("-" * 60)

print("""
【参数】
- 参数是函数的「输入」
- 定义时写在括号里，叫「形参」（形式参数）
- 调用时传入实际值，叫「实参」（实际参数）
""")

# 定义带参数的函数
# name 是参数（形参），是一个占位符
def greet_person(name):
    # 函数内部可以使用参数 name
    # 调用时传入什么，name就是什么
    print(f"你好，{name}！欢迎学习Python！")

# 调用时传入实际值（实参）
greet_person("魔尊")    # name = "魔尊"
greet_person("小明")    # name = "小明"
greet_person("Python") # name = "Python"

# 带两个参数的函数
# a 和 b 是两个参数
def add_numbers(a, b):
    # 计算两数之和
    result = a + b
    print(f"{a} + {b} = {result}")

# 调用时按顺序传入两个值
add_numbers(3, 5)    # a=3, b=5
add_numbers(10, 20)  # a=10, b=20

# --------------------------------------------
# 2.3 返回值（return）
# --------------------------------------------

print("\n2.3 返回值（return语句）")
print("-" * 60)

print("""
【return 卡片】

作用：把函数的计算结果「返回」给调用者

语法：return 值

规则：
- return 后面的代码不会执行（函数立即结束）
- 没有 return，函数返回 None
- 可以返回任何类型：数字、字符串、列表、字典...
- 可以返回多个值（用逗号分隔，实际是元组）
""")

# 有返回值的函数
# 函数计算结果后，用 return 返回
def calculate_sum(a, b):
    result = a + b
    return result  # 把结果返回给调用者

# 接收返回值
# 函数返回的值赋给变量 total
total = calculate_sum(3, 5)
print(f"3 + 5 = {total}")  # 输出：3 + 5 = 8

# 返回值可以直接使用
print(f"10 + 20 = {calculate_sum(10, 20)}")

# 没有 return 的函数返回 None
def no_return():
    x = 1 + 1  # 计算了，但没有返回

result = no_return()
print(f"没有return的函数返回：{result}")  # 输出：None

# 返回多个值
def get_min_max(numbers):
    # 返回列表中的最小值和最大值
    # return 多个值时，用逗号分隔
    # 实际上返回的是一个元组
    return min(numbers), max(numbers)

# 用两个变量接收两个返回值（元组解包）
minimum, maximum = get_min_max([3, 1, 4, 1, 5, 9, 2, 6])
print(f"最小值：{minimum}，最大值：{maximum}")

# --------------------------------------------
# 2.4 三种参数类型
# --------------------------------------------

print("\n2.4 三种参数类型")
print("-" * 60)

print("""
【参数类型总览】

1. 位置参数：按顺序传入，最常用
2. 关键字参数：用参数名=值传入，顺序可以不同
3. 默认参数：有默认值，调用时可以不传
""")

# --- 位置参数 ---
print("位置参数：")

# a, b, c 都是位置参数
# 调用时必须按顺序传入
def describe_person(name, age, city):
    print(f"{name}，{age}岁，来自{city}")

# 按位置顺序传入：name="张三", age=25, city="北京"
describe_person("张三", 25, "北京")

# --- 关键字参数 ---
print("\n关键字参数：")

# 调用时指定参数名，顺序可以改变
describe_person(city="上海", name="李四", age=30)  # 顺序不同也可以

# --- 默认参数 ---
print("\n默认参数：")

# city="北京" 是默认参数，有默认值
# 默认参数必须放在位置参数后面
def describe_person2(name, age, city="北京"):
    print(f"{name}，{age}岁，来自{city}")

# 不传 city，使用默认值 "北京"
describe_person2("王五", 20)

# 传入 city，覆盖默认值
describe_person2("赵六", 28, "广州")

# --------------------------------------------
# 2.5 可变参数 *args 和 **kwargs
# --------------------------------------------

print("\n2.5 可变参数")
print("-" * 60)

print("""
【可变参数卡片】

*args（位置可变参数）：
- 接收任意数量的位置参数
- 在函数内部是一个「元组」
- * 是语法，args 是惯用名（可改）

**kwargs（关键字可变参数）：
- 接收任意数量的关键字参数
- 在函数内部是一个「字典」
- ** 是语法，kwargs 是惯用名（可改）
""")

# *args：接收任意数量的位置参数
def sum_all(*args):
    # args 在函数内部是一个元组
    # 例如：sum_all(1,2,3) → args = (1, 2, 3)
    print(f"收到的参数：{args}")
    total = 0
    for num in args:  # 遍历元组中的每个数
        total += num
    return total

# 可以传入任意数量的参数
print(sum_all(1, 2))           # args = (1, 2)
print(sum_all(1, 2, 3))        # args = (1, 2, 3)
print(sum_all(1, 2, 3, 4, 5))  # args = (1, 2, 3, 4, 5)

# **kwargs：接收任意数量的关键字参数
def print_info(**kwargs):
    # kwargs 在函数内部是一个字典
    # 例如：print_info(name="张三") → kwargs = {"name": "张三"}
    print(f"收到的关键字参数：{kwargs}")
    for key, value in kwargs.items():
        print(f"  {key}: {value}")

# 可以传入任意数量的关键字参数
print_info(name="张三", age=25, city="北京")

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("\n" + "=" * 60)
print("第三部分：深入理解（选学，可跳过）")
print("=" * 60)

print("""
🔍 深入理解：变量作用域

如果觉得难，可以跳过，不影响后续学习。

【作用域】
- 变量的「可见范围」，决定变量在哪里可以被访问
- 局部变量：函数内部定义的变量，只在函数内部可用
- 全局变量：函数外部定义的变量，全局可用

LEGB规则（Python查找变量的顺序）：
  L → Local（局部）：当前函数内部
  E → Enclosing（外层）：嵌套函数的外层
  G → Global（全局）：模块级别
  B → Built-in（内置）：Python内置函数
""")

# 演示局部变量
def show_local():
    # x 是局部变量，只在函数内部存在
    x = 100
    print(f"函数内部 x = {x}")  # 可以访问

show_local()
# print(x)  # ❌ 错误！x 在函数外部不存在

# 演示全局变量
global_name = "全局变量"  # 定义在函数外部

def show_global():
    # 函数内部可以「读取」全局变量
    print(f"函数内部访问全局变量：{global_name}")

show_global()

# 在函数内部修改全局变量（需要 global 声明）
counter = 0  # 全局变量

def increment():
    global counter  # 声明使用全局变量
    counter += 1    # 修改全局变量

increment()
increment()
increment()
print(f"counter = {counter}")  # 输出：3

print("""
🔍 深入理解：函数也是对象

在Python中，函数也是一种对象：
- 可以赋值给变量
- 可以作为参数传递
- 这是Python「高阶函数」的基础
""")

# 函数赋值给变量
def say_hello():
    print("Hello!")

# greet_func 指向同一个函数
greet_func = say_hello
greet_func()  # 和 say_hello() 效果相同

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
错误1：忘记调用函数（漏掉括号）

❌ 错误代码：
  result = calculate_sum   # 只是引用函数对象，没有调用
  print(result)            # 输出函数对象，不是结果

✅ 正确代码：
  result = calculate_sum(3, 5)  # 加括号才是调用
  print(result)                 # 输出：8

原因：函数名不加括号是引用函数本身，加括号才是执行
""")

print("""
错误2：参数数量不匹配

❌ 错误代码：
  def add(a, b):
      return a + b
  add(1)        # 少传一个参数
  add(1, 2, 3)  # 多传一个参数

结果：TypeError

✅ 正确代码：
  add(1, 2)  # 刚好两个参数

原因：位置参数数量必须与定义一致
""")

print("""
错误3：默认参数使用可变对象（经典陷阱！）

❌ 错误代码：
  def add_item(item, lst=[]):  # 默认参数是列表（可变对象）
      lst.append(item)
      return lst

  print(add_item(1))  # [1]  看起来正常
  print(add_item(2))  # [1, 2]  !!! 不是 [2]！
  print(add_item(3))  # [1, 2, 3]  !!! 越来越长！

原因：默认参数只在函数定义时创建一次，不是每次调用都创建

✅ 正确代码：
  def add_item(item, lst=None):  # 默认参数用 None
      if lst is None:
          lst = []  # 每次调用都创建新列表
      lst.append(item)
      return lst
""")

print("""
错误4：在函数内部修改全局变量（忘记 global）

❌ 错误代码：
  count = 0
  def increment():
      count += 1  # UnboundLocalError！

结果：UnboundLocalError

✅ 正确代码：
  count = 0
  def increment():
      global count  # 先声明使用全局变量
      count += 1

原因：Python默认函数内赋值创建局部变量，修改全局变量需要声明
""")

print("""
错误5：return 后面还有代码（死代码）

❌ 错误代码：
  def check(x):
      return x > 0
      print("这行永远不会执行！")  # 死代码

原因：return 执行后函数立即结束，后面的代码不会执行

✅ 正确代码：
  def check(x):
      print("检查中...")
      return x > 0  # return 放在最后
""")

print("""
错误6：递归没有终止条件（无限递归）

❌ 错误代码：
  def countdown(n):
      print(n)
      countdown(n - 1)  # 没有终止条件，无限递归

结果：RecursionError（栈溢出）

✅ 正确代码：
  def countdown(n):
      if n <= 0:    # 终止条件
          return
      print(n)
      countdown(n - 1)
""")

print("""
错误7：混淆位置参数和关键字参数顺序

❌ 错误代码：
  def greet(name, greeting):
      print(f"{greeting}, {name}!")

  greet(greeting="你好", "张三")  # SyntaxError！

原因：关键字参数后面不能有位置参数

✅ 正确代码：
  greet("张三", greeting="你好")  # 位置参数在前
  greet(name="张三", greeting="你好")  # 全用关键字也行
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【def 定义函数】
语法：def 函数名(参数列表):
          函数体
返回：无（只是定义，不执行）
示例：
  def add(a, b):
      return a + b

────────────────────────────────────

【return 返回值】
语法：return 表达式
作用：结束函数，返回值给调用者
注意：没有return返回None；return后代码不执行
示例：
  return a + b         # 返回一个值
  return a, b          # 返回多个值（元组）
  return               # 返回None，提前结束

────────────────────────────────────

【参数类型速查】
  def func(a, b, c="默认", *args, **kwargs):
  │    │   │   └ 默认参数（有默认值，可不传）
  │    └───┘ 位置参数（必须按顺序传）
  │
  *args   → 接收多余的位置参数，变成元组
  **kwargs → 接收多余的关键字参数，变成字典

────────────────────────────────────

【global 声明】
语法：global 变量名
作用：在函数内部声明使用全局变量（修改时需要）
示例：
  count = 0
  def add_one():
      global count
      count += 1
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题（自己动手，不看答案！）")
print("=" * 60)

print("""
练习1：计算器函数
  要求：定义四个函数 add、subtract、multiply、divide
       分别实现加、减、乘、除
       divide 函数要处理除数为0的情况
  提示：用 if 判断除数是否为0，返回错误提示字符串

练习2：判断奇偶数
  要求：定义函数 is_even(number)
       判断一个数是否为偶数
       返回 True 或 False
  提示：用 % 取余运算符

练习3：统计列表信息
  要求：定义函数 list_stats(numbers)
       接收一个数字列表
       返回：总和、平均值、最大值、最小值（返回多个值）
  提示：使用 sum()、len()、max()、min() 内置函数

练习4：格式化问候语
  要求：定义函数 make_greeting(name, age=18, city="北京")
       返回格式化的问候字符串
       例如："你好！我是张三，今年18岁，来自北京。"
  提示：使用 f-string 格式化

练习5：统计单词频率
  要求：定义函数 count_words(text)
       接收一个字符串（文章）
       返回每个单词出现次数的字典
  提示：用 split() 分割字符串，用字典统计
""")