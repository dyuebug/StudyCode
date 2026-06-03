# ============================================
# 迭代器协议 - 超详细注释版
# 日期：2026-05-31（第20天）
# 难度：*** 高级
# ============================================

"""
【核心概念】迭代器协议（Iterator Protocol）

是什么？
- 迭代器协议是 Python 中让对象支持 for 循环的规范
- 实现了 __iter__ 和 __next__ 方法的对象就是迭代器

为什么需要？
- 理解 for 循环的底层原理
- 自定义可迭代的数据结构
- 理解生成器的本质（生成器就是迭代器）

核心特点？
- 可迭代对象（Iterable）：有 __iter__ 方法
- 迭代器（Iterator）：有 __iter__ 和 __next__ 方法
- 生成器是迭代器的一种特殊形式
"""

# ============================================
# 第一部分：核心概念详解 *****
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 可迭代对象 vs 迭代器
# --------------------------------------------

print("\n1.1 可迭代对象 vs 迭代器")
print("-" * 60)

print("""
【关键区别】

可迭代对象（Iterable）：
  - 有 __iter__() 方法
  - 调用 __iter__() 返回一个迭代器
  - 可以多次遍历（每次调用 __iter__ 返回新迭代器）
  - 例子：list、tuple、str、dict、set

迭代器（Iterator）：
  - 有 __iter__() 方法（返回自身）
  - 有 __next__() 方法（返回下一个值）
  - 只能遍历一次（状态不可重置）
  - 例子：生成器、文件对象、zip对象

关系：
  迭代器 是 可迭代对象（迭代器也有 __iter__）
  可迭代对象 不一定是 迭代器（列表没有 __next__）

for 循环的本质：
  for item in iterable:
      ...

  等价于：
  iterator = iter(iterable)   # 调用 __iter__
  while True:
      try:
          item = next(iterator)  # 调用 __next__
          ...
      except StopIteration:
          break
""")

# 验证：列表是可迭代对象，但不是迭代器
my_list = [1, 2, 3]
print(f"列表有 __iter__：{hasattr(my_list, '__iter__')}")
print(f"列表有 __next__：{hasattr(my_list, '__next__')}")

# 获取列表的迭代器
list_iter = iter(my_list)  # 调用 my_list.__iter__()
print(f"\n列表的迭代器：{list_iter}")
print(f"迭代器有 __iter__：{hasattr(list_iter, '__iter__')}")
print(f"迭代器有 __next__：{hasattr(list_iter, '__next__')}")

print(f"\n手动调用 next()：")
print(next(list_iter))  # 1
print(next(list_iter))  # 2
print(next(list_iter))  # 3
try:
    print(next(list_iter))  # StopIteration
except StopIteration:
    print("StopIteration：迭代器耗尽")

print()

# --------------------------------------------
# 1.2 重要函数：iter() 和 next() *****
# --------------------------------------------

print("1.2 iter() 和 next() 详解")
print("-" * 60)

print("""
【函数卡片：iter(object)】

语法：iter(object)  或  iter(callable, sentinel)
作用：返回对象的迭代器
参数：
  object   - 可迭代对象（调用其 __iter__ 方法）
  callable - 可调用对象（第二种用法）
  sentinel - 哨兵值，callable 返回此值时停止迭代

返回值：迭代器对象

示例：
  iter([1, 2, 3])    # 列表的迭代器
  iter("hello")      # 字符串的迭代器
  iter({1, 2, 3})    # 集合的迭代器

────────────────────────────────────

【函数卡片：next(iterator, default)】

语法：next(iterator)  或  next(iterator, default)
作用：获取迭代器的下一个值
参数：
  iterator - 迭代器对象
  default  - 可选，迭代器耗尽时返回的默认值（不报错）

返回值：下一个值，或 default（如果提供且迭代器耗尽）

示例：
  it = iter([1, 2])
  next(it)           # 1
  next(it)           # 2
  next(it, "结束")   # "结束"（不报 StopIteration）
""")

print()

# ============================================
# 第二部分：基础操作实践
# ============================================

print("=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 手动实现迭代器类
# --------------------------------------------

print("\n2.1 手动实现迭代器类")
print("-" * 60)

class CountUp:
    """
    从 start 数到 end 的迭代器

    演示如何实现迭代器协议
    """

    def __init__(self, start, end):
        self.start = start
        self.end = end
        self.current = start  # 记录当前位置（迭代器的状态）

    def __iter__(self):
        # 迭代器的 __iter__ 返回自身
        return self

    def __next__(self):
        if self.current > self.end:
            raise StopIteration  # 告诉 for 循环停止
        value = self.current
        self.current += 1  # 更新状态
        return value

print("使用自定义迭代器：")
counter = CountUp(1, 5)
for n in counter:
    print(n, end=" ")
print()

print("\n手动调用 next()：")
counter2 = CountUp(1, 3)
print(next(counter2))  # 1
print(next(counter2))  # 2
print(next(counter2))  # 3
print(next(counter2, "结束"))  # "结束"

print()

# --------------------------------------------
# 2.2 可迭代对象（不是迭代器）
# --------------------------------------------

print("2.2 可迭代对象（每次遍历都是新的迭代器）")
print("-" * 60)

print("""
区别：
  迭代器：只能遍历一次（状态保存在迭代器里）
  可迭代对象：可以多次遍历（每次 __iter__ 返回新迭代器）

如果你的类需要支持多次遍历，让 __iter__ 返回新的迭代器对象
""")

class NumberRange:
    """
    可迭代对象（不是迭代器）
    支持多次遍历
    """

    def __init__(self, start, end):
        self.start = start
        self.end = end

    def __iter__(self):
        # 每次调用都返回一个新的迭代器
        return CountUp(self.start, self.end)

numbers = NumberRange(1, 3)

print("第一次遍历：", end="")
for n in numbers:
    print(n, end=" ")
print()

print("第二次遍历：", end="")
for n in numbers:  # 可以再次遍历！
    print(n, end=" ")
print()

print()

# --------------------------------------------
# 2.3 实用迭代器：循环迭代器
# --------------------------------------------

print("2.3 实用迭代器：循环迭代器")
print("-" * 60)

class Cycle:
    """
    循环迭代器：无限循环遍历序列

    类似 itertools.cycle
    """

    def __init__(self, iterable):
        self.data = list(iterable)  # 保存数据
        self.index = 0

    def __iter__(self):
        return self

    def __next__(self):
        if not self.data:
            raise StopIteration
        value = self.data[self.index]
        self.index = (self.index + 1) % len(self.data)  # 循环索引
        return value

print("循环迭代器（取前10个）：")
cycle = Cycle([1, 2, 3])
for i, val in enumerate(cycle):
    print(val, end=" ")
    if i >= 9:  # 只取前10个，否则无限循环
        break
print()

print()

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("""
==============================================
深入理解：for 循环的底层原理（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【for 循环完整执行流程】

for item in obj:
    body

Python 解释器实际执行：
  1. _iter = obj.__iter__()       # 获取迭代器
  2. while True:
  3.     try:
  4.         item = _iter.__next__()  # 获取下一个值
  5.         body                     # 执行循环体
  6.     except StopIteration:
  7.         break                    # 迭代结束

【生成器是迭代器】

生成器函数返回的生成器对象，同时实现了：
  - __iter__()：返回自身
  - __next__()：执行到下一个 yield

所以生成器可以直接用在 for 循环中。

验证：
  def gen():
      yield 1
      yield 2

  g = gen()
  print(hasattr(g, '__iter__'))  # True
  print(hasattr(g, '__next__'))  # True

==============================================
""")

# 验证生成器是迭代器
def simple_gen():
    yield 1
    yield 2

g = simple_gen()
print("验证生成器是迭代器：")
print(f"  有 __iter__：{hasattr(g, '__iter__')}")
print(f"  有 __next__：{hasattr(g, '__next__')}")
print(f"  __iter__ 返回自身：{g.__iter__() is g}")

print()

# ============================================
# 第四部分：常见错误和陷阱 *****
# ============================================

print("=" * 60)
print("第四部分：常见错误和陷阱 *****")
print("=" * 60)

print("""
[X] 错误1：迭代器只能遍历一次

错误代码：
  it = iter([1, 2, 3])
  print(list(it))  # [1, 2, 3]
  print(list(it))  # []  ← 空了！

原因：迭代器有状态，遍历完就耗尽了

[OK] 正确做法：
  data = [1, 2, 3]
  print(list(iter(data)))  # [1, 2, 3]
  print(list(iter(data)))  # [1, 2, 3]  ← 每次创建新迭代器

记忆技巧：迭代器是一次性的；可迭代对象可以多次遍历
""")

print("""
[X] 错误2：__iter__ 返回了 self，但类不是迭代器

错误代码：
  class MyList:
      def __init__(self, data):
          self.data = data

      def __iter__(self):
          return self  # 错误！self 没有 __next__

结果：TypeError: iter() returned non-iterator

[OK] 正确做法（方案1）：同时实现 __next__，让类成为迭代器
  class MyList:
      def __iter__(self):
          self.index = 0
          return self

      def __next__(self):
          if self.index >= len(self.data):
              raise StopIteration
          value = self.data[self.index]
          self.index += 1
          return value

[OK] 正确做法（方案2）：__iter__ 返回新的迭代器对象
  class MyList:
      def __iter__(self):
          return iter(self.data)  # 返回列表的迭代器

记忆技巧：__iter__ 返回 self → 类必须也有 __next__
""")

print("""
[X] 错误3：忘记在 __next__ 中抛出 StopIteration

错误代码：
  def __next__(self):
      if self.index >= len(self.data):
          return None  # 错误！应该抛出 StopIteration

结果：for 循环永远不会停止（None 被当作有效值）

[OK] 正确做法：
  def __next__(self):
      if self.index >= len(self.data):
          raise StopIteration  # 必须抛出，不能 return

记忆技巧：迭代结束 → 必须 raise StopIteration
""")

print("""
[X] 错误4：在迭代时修改被迭代的对象

错误代码：
  lst = [1, 2, 3, 4, 5]
  for item in lst:
      if item % 2 == 0:
          lst.remove(item)  # 边迭代边修改！

结果：可能跳过元素，行为不可预期

[OK] 正确做法：
  # 方法1：遍历副本
  for item in lst[:]:
      if item % 2 == 0:
          lst.remove(item)

  # 方法2：用列表推导式
  lst = [item for item in lst if item % 2 != 0]

记忆技巧：迭代时不要修改被迭代的对象
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【迭代器类模板】

  class MyIterator:
      def __init__(self, data):
          self.data = data
          self.index = 0

      def __iter__(self):
          return self  # 迭代器返回自身

      def __next__(self):
          if self.index >= len(self.data):
              raise StopIteration
          value = self.data[self.index]
          self.index += 1
          return value

────────────────────────────────────

【可迭代对象类模板（支持多次遍历）】

  class MyIterable:
      def __init__(self, data):
          self.data = data

      def __iter__(self):
          return iter(self.data)  # 每次返回新迭代器

────────────────────────────────────

【iter(object)】

语法：iter(obj)
作用：获取对象的迭代器（调用 obj.__iter__()）

示例：
  it = iter([1, 2, 3])
  it = iter("hello")

────────────────────────────────────

【next(iterator, default)】

语法：next(it) 或 next(it, default)
作用：获取迭代器下一个值

示例：
  next(it)           # 无默认值，耗尽时报 StopIteration
  next(it, None)     # 有默认值，耗尽时返回 None
""")

# ============================================
# 第六部分：练习题
# ============================================

print("=" * 60)
print("第六部分：练习题")
print("=" * 60)

print("""
1. 实现一个 Fibonacci 迭代器类
   要求：每次 next() 返回下一个斐波那契数
   示例：
     fib = Fibonacci()
     for i, n in enumerate(fib):
         print(n)
         if i >= 9: break  # 前10个

2. 实现一个 Reverse 迭代器类
   要求：反向遍历序列（不使用 reversed()）
   示例：
     for item in Reverse([1, 2, 3, 4, 5]):
         print(item)  # 5, 4, 3, 2, 1

3. 实现一个 Zip 迭代器类（简化版）
   要求：同时遍历两个序列，返回元组
   示例：
     for a, b in MyZip([1, 2, 3], ['a', 'b', 'c']):
         print(a, b)  # (1, 'a'), (2, 'b'), (3, 'c')
   提示：以较短的序列为准停止

4. 思考题：
   - 为什么生成器比手动实现迭代器更简洁？
   - 什么时候需要手动实现迭代器类？
   - iter() 和 next() 分别对应哪个魔法方法？
""")

print("=" * 60)
print("第20天 - 迭代器协议详解 学习完成！")
print("=" * 60)
