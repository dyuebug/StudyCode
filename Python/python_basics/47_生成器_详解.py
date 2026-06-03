# ============================================
# 生成器 - 超详细注释版
# 日期：2026-05-30（第19天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【核心概念】生成器（Generator）

是什么？
- 生成器是一种特殊的迭代器，使用 yield 关键字定义
- 它不会一次性生成所有数据，而是按需生成（惰性计算）

为什么需要？
- 节省内存：处理大数据时不需要把所有数据加载到内存
- 惰性计算：只在需要时才计算下一个值
- 简化代码：比手动实现迭代器更简洁

核心特点？
- 使用 yield 关键字（不是 return）
- 每次调用 next() 时从上次暂停的地方继续
- 生成器对象只能遍历一次
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 理解 yield 关键字
# --------------------------------------------

print("\n1.1 yield 关键字详解")
print("-" * 60)

print("""
【yield 关键字】

yield 的作用：
  1. 暂停函数执行，返回一个值
  2. 下次调用时，从暂停的地方继续执行
  3. 函数变成生成器函数（返回生成器对象）

与 return 的区别：
  return - 函数结束，返回一个值
  yield  - 函数暂停，返回一个值，下次继续

执行流程：
  1. 调用生成器函数 → 返回生成器对象（不执行函数体！）
  2. 调用 next() → 执行到第一个 yield，暂停，返回值
  3. 再次调用 next() → 从暂停处继续，到下一个 yield
  4. 没有更多 yield → 抛出 StopIteration 异常
""")

# 演示 yield 的执行流程
def count_up(start, end):
    """从start数到end的生成器"""
    print(f"[生成器] 开始，从{start}数到{end}")
    current = start
    while current <= end:
        print(f"[生成器] 即将yield {current}")
        yield current  # 暂停，返回current的值
        print(f"[生成器] 从{current}之后继续")
        current += 1
    print("[生成器] 结束")

print("创建生成器对象（不执行函数体）：")
gen = count_up(1, 3)
print(f"生成器对象：{gen}")
print()

print("调用 next() 逐步获取值：")
print(f"第1次 next()：{next(gen)}")
print()
print(f"第2次 next()：{next(gen)}")
print()
print(f"第3次 next()：{next(gen)}")
print()

print("用 for 循环遍历（推荐方式）：")
for value in count_up(1, 3):
    print(f"  获得值：{value}")

print()

# --------------------------------------------
# 1.2 生成器 vs 普通函数
# --------------------------------------------

print("1.2 生成器 vs 普通函数对比")
print("-" * 60)

print("""
【对比：生成所有数字1到N】

普通函数（列表）：
  def get_numbers(n):
      result = []
      for i in range(1, n+1):
          result.append(i)
      return result  # 一次性生成所有数据，存在内存中

  numbers = get_numbers(1000000)  # 占用大量内存！

生成器函数：
  def gen_numbers(n):
      for i in range(1, n+1):
          yield i  # 每次只生成一个，按需计算

  numbers = gen_numbers(1000000)  # 几乎不占内存！

内存对比（N=1000000）：
  列表：约 8MB
  生成器：约 200字节（只存储当前状态）
""")

import sys

# 普通列表
numbers_list = list(range(1, 1000001))
print(f"列表占用内存：{sys.getsizeof(numbers_list):,} 字节")

# 生成器
def gen_numbers(n):
    for i in range(1, n + 1):
        yield i

numbers_gen = gen_numbers(1000000)
print(f"生成器占用内存：{sys.getsizeof(numbers_gen):,} 字节")

print()

# ============================================
# 第二部分：基础操作实践
# ============================================

print("=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 简单生成器
# --------------------------------------------

print("\n2.1 简单生成器示例")
print("-" * 60)

def fibonacci():
    """斐波那契数列生成器（无限序列）"""
    a, b = 0, 1
    while True:  # 无限循环，但不会占满内存
        yield a  # 每次返回当前值
        a, b = b, a + b  # 更新状态

print("斐波那契数列（前10个）：")
fib = fibonacci()
for _ in range(10):
    print(next(fib), end=" ")
print()

print()

# --------------------------------------------
# 2.2 生成器表达式
# --------------------------------------------

print("2.2 生成器表达式")
print("-" * 60)

print("""
【生成器表达式】

语法：(表达式 for 变量 in 可迭代对象 if 条件)

与列表推导式的区别：
  列表推导式：[x*2 for x in range(10)]  → 方括号，立即生成列表
  生成器表达式：(x*2 for x in range(10)) → 圆括号，惰性生成

使用场景：
  - 只需要遍历一次的数据
  - 数据量很大，不想全部加载到内存
  - 作为函数参数（如sum、max、min）
""")

# 列表推导式 vs 生成器表达式
squares_list = [x**2 for x in range(10)]      # 列表推导式
squares_gen = (x**2 for x in range(10))        # 生成器表达式

print(f"列表推导式：{squares_list}")
print(f"生成器表达式：{squares_gen}")
print(f"生成器内存：{sys.getsizeof(squares_gen)} 字节")

# 遍历生成器
print("遍历生成器：", end="")
for sq in squares_gen:
    print(sq, end=" ")
print()

# 生成器作为函数参数（不需要额外的方括号）
total = sum(x**2 for x in range(10))  # 直接传生成器表达式
print(f"平方和：{total}")

print()

# --------------------------------------------
# 2.3 实用生成器：读取大文件
# --------------------------------------------

print("2.3 实用生成器：处理大数据")
print("-" * 60)

def read_large_file(filename, chunk_size=1024):
    """
    分块读取大文件的生成器

    参数：
      filename - 文件路径
      chunk_size - 每次读取的字节数，默认1024字节
    """
    with open(filename, 'r', encoding='utf-8') as f:
        while True:
            chunk = f.read(chunk_size)  # 读取一块数据
            if not chunk:               # 读到文件末尾
                break
            yield chunk                 # 返回这块数据

def read_lines(filename):
    """逐行读取文件的生成器"""
    with open(filename, 'r', encoding='utf-8') as f:
        for line in f:
            yield line.rstrip('\n')  # 去掉行尾换行符

# 演示：创建一个临时文件来测试
import tempfile
import os

# 创建临时文件
with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                  delete=False, encoding='utf-8') as f:
    for i in range(1, 6):
        f.write(f"第{i}行：这是测试数据\n")
    temp_file = f.name

print(f"创建临时文件：{temp_file}")
print("逐行读取：")
for line in read_lines(temp_file):
    print(f"  {line}")

os.unlink(temp_file)  # 删除临时文件

print()

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("""
==============================================
🔍 深入理解：生成器的内部机制（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【生成器是迭代器】

生成器实现了迭代器协议：
  - __iter__() 方法：返回自身
  - __next__() 方法：返回下一个值

所以生成器可以用在任何需要迭代器的地方：
  - for 循环
  - list()、tuple()、set() 等转换
  - sum()、max()、min() 等函数

【生成器的状态】

生成器保存了函数的执行状态：
  - 局部变量的值
  - 当前执行位置（yield 的位置）
  - 调用栈

这就是为什么生成器可以"暂停"和"继续"。

【send() 方法】

生成器还支持 send() 方法，可以向生成器发送值：
  value = yield expression  # yield 可以接收发送的值

  gen = my_generator()
  next(gen)           # 启动生成器
  gen.send(42)        # 发送值42，yield 表达式的值变成42

==============================================
""")

# --------------------------------------------
# 3.1 生成器的 send() 方法（进阶）
# --------------------------------------------

print("3.1 生成器的 send() 方法")
print("-" * 60)

def accumulator():
    """累加器生成器：接收数字，返回累计和"""
    total = 0
    while True:
        value = yield total  # yield 返回total，同时接收发送的值
        if value is None:
            break
        total += value

print("使用 send() 向生成器发送值：")
acc = accumulator()
next(acc)           # 启动生成器（到第一个yield）
print(f"发送 10：{acc.send(10)}")
print(f"发送 20：{acc.send(20)}")
print(f"发送 30：{acc.send(30)}")

print()

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
❌ 错误1：生成器只能遍历一次

错误代码：
  gen = (x for x in range(5))
  print(list(gen))  # [0, 1, 2, 3, 4]
  print(list(gen))  # []  ← 空了！

原因：生成器遍历完后就"耗尽"了，不能重置

✅ 正确做法：
  # 方法1：每次使用时重新创建生成器
  def make_gen():
      return (x for x in range(5))

  print(list(make_gen()))  # [0, 1, 2, 3, 4]
  print(list(make_gen()))  # [0, 1, 2, 3, 4]

  # 方法2：如果需要多次遍历，转换为列表
  data = list(x for x in range(5))

记忆技巧：生成器是"一次性"的，用完就没了
""")

print("""
❌ 错误2：在生成器函数中混用 return 和 yield

错误代码：
  def bad_gen():
      yield 1
      return [2, 3]  # 这不会yield 2和3！

结果：return 只是停止生成器，不会yield返回值

✅ 正确做法：
  def good_gen():
      yield 1
      yield 2  # 每个值都要yield
      yield 3

  # 或者用 yield from
  def good_gen2():
      yield 1
      yield from [2, 3]  # yield from 会逐个yield列表中的值

记忆技巧：生成器里每个要返回的值都要用yield
""")

print("""
❌ 错误3：忘记生成器是惰性的，修改了源数据

错误代码：
  data = [1, 2, 3, 4, 5]
  gen = (x for x in data)  # 创建生成器
  data.clear()              # 清空源数据！
  print(list(gen))          # [] ← 空了！

原因：生成器表达式引用了data，data被清空后生成器也没数据了

✅ 正确做法：
  data = [1, 2, 3, 4, 5]
  gen = (x for x in data[:])  # 用切片创建副本
  data.clear()
  print(list(gen))  # [1, 2, 3, 4, 5]

记忆技巧：如果源数据可能变化，先复制一份
""")

print("""
❌ 错误4：在生成器中捕获 StopIteration

错误代码：
  def bad_gen():
      try:
          yield 1
          yield 2
      except StopIteration:  # 不要在生成器里捕获StopIteration！
          pass

原因：Python 3.7+ 中，生成器内部的 StopIteration 会变成 RuntimeError

✅ 正确做法：
  def good_gen():
      yield 1
      yield 2
      # 自然结束，不需要捕获StopIteration

记忆技巧：生成器自然结束就好，不要手动处理StopIteration
""")

print("""
❌ 错误5：把生成器当列表用

错误代码：
  gen = (x for x in range(5))
  print(gen[2])   # TypeError！生成器不支持索引
  print(len(gen)) # TypeError！生成器没有长度

原因：生成器是惰性的，不支持随机访问

✅ 正确做法：
  gen = (x for x in range(5))
  gen_list = list(gen)  # 转换为列表
  print(gen_list[2])    # 2
  print(len(gen_list))  # 5

记忆技巧：需要索引或长度时，先转换为列表
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【yield 关键字】

语法：yield 表达式
作用：暂停生成器函数，返回一个值
特点：下次调用时从暂停处继续

示例：
  def gen():
      yield 1
      yield 2
      yield 3

────────────────────────────────────

【next(iterator)】

语法：next(iterator, default=None)
作用：获取迭代器的下一个值
参数：
  iterator - 迭代器或生成器
  default  - 可选，迭代器耗尽时返回的默认值

示例：
  gen = (x for x in range(3))
  print(next(gen))        # 0
  print(next(gen))        # 1
  print(next(gen, "结束")) # 2
  print(next(gen, "结束")) # "结束"（不报错）

────────────────────────────────────

【yield from 表达式】

语法：yield from 可迭代对象
作用：逐个yield可迭代对象中的每个元素
等价于：for item in iterable: yield item

示例：
  def gen():
      yield from [1, 2, 3]  # 等价于逐个yield
      yield from range(4, 7)

────────────────────────────────────

【生成器表达式】

语法：(表达式 for 变量 in 可迭代对象 if 条件)
作用：创建生成器对象（惰性计算）

示例：
  squares = (x**2 for x in range(10))
  evens = (x for x in range(20) if x % 2 == 0)
  total = sum(x**2 for x in range(10))  # 直接作为参数
""")

# ============================================
# 第六部分：练习题
# ============================================

print("=" * 60)
print("第六部分：练习题")
print("=" * 60)

print("""
1. 实现一个 range_float 生成器
   要求：类似 range()，但支持浮点数步长
   示例：
     for x in range_float(0, 1, 0.1):
         print(x)  # 0.0, 0.1, 0.2, ..., 0.9
   提示：用 while 循环和 yield

2. 实现一个 flatten 生成器
   要求：将嵌套列表展平为一维
   示例：
     list(flatten([1, [2, 3], [4, [5, 6]]])) == [1, 2, 3, 4, 5, 6]
   提示：用递归和 yield from

3. 实现一个 take 函数
   要求：从生成器中取前N个元素
   示例：
     list(take(fibonacci(), 10))  # 斐波那契数列前10个
   提示：用 itertools.islice 或手动实现

4. 思考题：
   - 什么时候用生成器，什么时候用列表？
   - 生成器和迭代器有什么关系？
   - 为什么说生成器是"惰性"的？
""")

print("=" * 60)
print("第19天 - 生成器详解 学习完成！")
print("=" * 60)
