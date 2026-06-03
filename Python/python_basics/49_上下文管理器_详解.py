# ============================================
# 上下文管理器 - 超详细注释版
# 日期：2026-05-31（第20天）
# 难度：*** 高级
# ============================================

"""
【核心概念】上下文管理器（Context Manager）

是什么？
- 上下文管理器是支持 with 语句的对象
- 它定义了进入和退出"上下文"时要执行的操作

为什么需要？
- 自动管理资源：无论是否发生异常，都能保证资源被正确释放
- 减少重复代码：不需要每次都写 try/finally
- 代码更简洁：with 语句比 try/finally 更易读

核心特点？
- 实现 __enter__ 方法：进入 with 块时调用
- 实现 __exit__ 方法：退出 with 块时调用（无论是否异常）
- 也可以用 contextlib.contextmanager 装饰器简化实现
"""

# ============================================
# 第一部分：核心概念详解 *****
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 with 语句的执行原理
# --------------------------------------------

print("\n1.1 with 语句的执行原理")
print("-" * 60)

print("""
【with 语句的执行流程】

with 表达式 as 变量:
    代码块

执行顺序：
  1. 计算"表达式"，得到上下文管理器对象
  2. 调用对象的 __enter__() 方法
  3. 将 __enter__() 的返回值赋给"变量"（as 子句）
  4. 执行"代码块"
  5. 无论代码块是否异常，都调用 __exit__() 方法

等价的 try/finally 写法：
  mgr = 表达式
  变量 = mgr.__enter__()
  try:
      代码块
  finally:
      mgr.__exit__(...)  # 无论如何都会执行

为什么用 with 更好？
  - 更简洁，不容易忘记释放资源
  - 异常处理逻辑封装在 __exit__ 里，调用方不需要关心
""")

# 演示：文件操作（最常见的上下文管理器）
print("演示：文件操作的 with 语句")
print("-" * 40)

import tempfile
import os

# 创建临时文件
with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                  delete=False, encoding='utf-8') as f:
    f.write("Hello, Context Manager!\n第二行内容\n")
    temp_path = f.name

# 用 with 读取文件
print("用 with 读取文件：")
with open(temp_path, 'r', encoding='utf-8') as f:
    # __enter__ 被调用，返回文件对象赋给 f
    content = f.read()
    print(content)
# 退出 with 块，__exit__ 被调用，文件自动关闭

print(f"文件是否已关闭：{f.closed}")  # True，自动关闭了
os.unlink(temp_path)

print()

# --------------------------------------------
# 1.2 重要方法：__enter__ 和 __exit__ *****
# --------------------------------------------

print("1.2 __enter__ 和 __exit__ 详解")
print("-" * 60)

print("""
【__enter__ 方法】

语法：def __enter__(self)
调用时机：进入 with 块时
返回值：赋给 as 后面的变量（如果没有 as，返回值被忽略）

【__exit__ 方法】

语法：def __exit__(self, exc_type, exc_val, exc_tb)
调用时机：退出 with 块时（无论是否异常）
参数：
  exc_type - 异常类型（无异常时为 None）
  exc_val  - 异常值（无异常时为 None）
  exc_tb   - 异常追踪信息（无异常时为 None）
返回值：
  True  - 抑制异常（异常不会向外传播）
  False 或 None - 不抑制异常（异常继续传播）

关键点：
  - 即使代码块抛出异常，__exit__ 也会被调用
  - 通过返回值控制是否"吞掉"异常
""")

print()

# ============================================
# 第二部分：基础操作实践
# ============================================

print("=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 用类实现上下文管理器
# --------------------------------------------

print("\n2.1 用类实现上下文管理器")
print("-" * 60)

class ManagedResource:
    """演示上下文管理器执行流程的类"""

    def __init__(self, name):
        self.name = name
        print(f"[{self.name}] 创建对象")

    def __enter__(self):
        print(f"[{self.name}] __enter__ 被调用，进入 with 块")
        return self  # 返回 self，赋给 as 后面的变量

    def __exit__(self, exc_type, exc_val, exc_tb):
        print(f"[{self.name}] __exit__ 被调用，退出 with 块")
        if exc_type is not None:
            print(f"[{self.name}] 捕获到异常：{exc_type.__name__}: {exc_val}")
        return False  # 不抑制异常

    def do_work(self):
        print(f"[{self.name}] 正在工作...")

print("正常情况：")
with ManagedResource("资源A") as res:
    res.do_work()

print()
print("有异常的情况：")
try:
    with ManagedResource("资源B") as res:
        res.do_work()
        raise ValueError("模拟异常")
except ValueError as e:
    print(f"异常被外层捕获：{e}")

print()

# --------------------------------------------
# 2.2 实用示例：计时上下文管理器
# --------------------------------------------

print("2.2 计时上下文管理器")
print("-" * 60)

import time

class Timer:
    """计时上下文管理器：测量代码块的执行时间"""

    def __init__(self, name=""):
        self.name = name
        self.elapsed = 0.0  # 记录耗时

    def __enter__(self):
        self.start = time.time()  # 记录开始时间
        return self               # 返回 self，可以通过 as 访问 elapsed

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.elapsed = time.time() - self.start  # 计算耗时
        label = f"[{self.name}] " if self.name else ""
        print(f"{label}耗时：{self.elapsed:.4f}秒")
        return False  # 不抑制异常

print("使用计时上下文管理器：")
with Timer("计算任务") as t:
    result = sum(range(1000000))
    print(f"计算结果：{result}")

print(f"可以在外部访问耗时：{t.elapsed:.4f}秒")

print()

# --------------------------------------------
# 2.3 实用示例：临时目录上下文管理器
# --------------------------------------------

print("2.3 临时目录上下文管理器")
print("-" * 60)

import shutil

class TempDirectory:
    """临时目录上下文管理器：自动创建和清理临时目录"""

    def __init__(self, prefix="tmp_"):
        self.prefix = prefix
        self.path = None

    def __enter__(self):
        import tempfile
        self.path = tempfile.mkdtemp(prefix=self.prefix)
        print(f"[临时目录] 创建：{self.path}")
        return self.path  # 返回目录路径

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.path and os.path.exists(self.path):
            shutil.rmtree(self.path)  # 递归删除目录
            print(f"[临时目录] 已清理：{self.path}")
        return False

with TempDirectory("myapp_") as tmpdir:
    # 在临时目录中创建文件
    test_file = os.path.join(tmpdir, "test.txt")
    with open(test_file, 'w') as f:
        f.write("临时文件内容")
    print(f"创建临时文件：{test_file}")
    print(f"文件存在：{os.path.exists(test_file)}")

print(f"退出后目录存在：{os.path.exists(tmpdir)}")

print()

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("""
==============================================
深入理解：__exit__ 的异常抑制（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【__exit__ 返回 True 抑制异常】

class SuppressError:
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if exc_type is ValueError:
            print("ValueError 被抑制了")
            return True   # 抑制 ValueError
        return False      # 其他异常继续传播

with SuppressError():
    raise ValueError("这个异常会被抑制")
    print("这行不会执行")

print("with 块之后继续执行")  # 这行会执行！

【contextlib.suppress】

标准库提供了 suppress 上下文管理器，专门用于抑制指定异常：

  from contextlib import suppress

  with suppress(FileNotFoundError):
      os.remove("不存在的文件.txt")  # 不会报错

==============================================
""")

# --------------------------------------------
# 3.1 异常抑制演示
# --------------------------------------------

print("3.1 异常抑制演示")
print("-" * 60)

from contextlib import suppress

print("用 suppress 忽略 FileNotFoundError：")
with suppress(FileNotFoundError):
    os.remove("这个文件不存在.txt")
    print("这行不会执行")
print("with 块之后继续执行（异常被抑制了）")

print()

# ============================================
# 第四部分：contextlib.contextmanager *****
# ============================================

print("=" * 60)
print("第四部分：contextlib.contextmanager（简化写法）")
print("=" * 60)

print("""
【contextmanager 装饰器】

用生成器函数代替类，更简洁地实现上下文管理器。

语法：
  from contextlib import contextmanager

  @contextmanager
  def my_context():
      # __enter__ 的逻辑
      yield 值          # yield 的值赋给 as 后面的变量
      # __exit__ 的逻辑

执行流程：
  1. 进入 with 块：执行到 yield 之前的代码
  2. yield 的值赋给 as 变量
  3. 执行 with 块内的代码
  4. 退出 with 块：执行 yield 之后的代码

异常处理：
  用 try/finally 包裹 yield，确保清理代码一定执行：

  @contextmanager
  def my_context():
      # 准备资源
      try:
          yield 资源
      finally:
          # 清理资源（无论是否异常都执行）
""")

from contextlib import contextmanager

# 用 contextmanager 重写计时器
@contextmanager
def timer(name=""):
    """计时上下文管理器（生成器版）"""
    start = time.time()
    try:
        yield  # 执行 with 块内的代码
    finally:
        elapsed = time.time() - start
        label = f"[{name}] " if name else ""
        print(f"{label}耗时：{elapsed:.4f}秒")

print("使用 contextmanager 版计时器：")
with timer("排序任务"):
    data = list(range(100000, 0, -1))
    data.sort()
    print(f"排序完成，共 {len(data)} 个元素")

print()

# 带返回值的 contextmanager
@contextmanager
def managed_list():
    """管理一个列表，退出时打印统计信息"""
    items = []
    try:
        yield items  # 把列表暴露给 with 块
    finally:
        print(f"列表最终有 {len(items)} 个元素：{items}")

print("带返回值的 contextmanager：")
with managed_list() as lst:
    lst.append(1)
    lst.append(2)
    lst.append(3)

print()

# ============================================
# 第五部分：常见错误和陷阱 *****
# ============================================

print("=" * 60)
print("第五部分：常见错误和陷阱 *****")
print("=" * 60)

print("""
[X] 错误1：__exit__ 忘记返回值（默认返回 None，等同于 False）

这不是 bug，但要理解：
  def __exit__(self, exc_type, exc_val, exc_tb):
      cleanup()
      # 没有 return → 返回 None → 不抑制异常

如果你想抑制异常，必须显式 return True。
如果不想抑制，return False 或不写 return 都可以。

记忆技巧：想抑制异常 → return True；其他情况不用管
""")

print("""
[X] 错误2：contextmanager 里有多个 yield

错误代码：
  @contextmanager
  def bad_context():
      yield 1
      yield 2  # 错误！只能有一个 yield

结果：RuntimeError: generator didn't stop

原因：contextmanager 要求生成器只 yield 一次

[OK] 正确做法：
  @contextmanager
  def good_context():
      yield 1  # 只有一个 yield

记忆技巧：contextmanager 里只能有一个 yield
""")

print("""
[X] 错误3：contextmanager 里没有 try/finally，异常时不清理

错误代码：
  @contextmanager
  def bad_context():
      resource = acquire()
      yield resource
      release(resource)  # 如果 with 块抛异常，这行不会执行！

结果：资源泄漏

[OK] 正确做法：
  @contextmanager
  def good_context():
      resource = acquire()
      try:
          yield resource
      finally:
          release(resource)  # 无论是否异常都会执行

记忆技巧：contextmanager 里的清理代码必须放在 finally 里
""")

print("""
[X] 错误4：在 with 块外使用 as 变量（文件已关闭）

错误代码：
  with open("file.txt") as f:
      pass
  f.read()  # 错误！文件已关闭

结果：ValueError: I/O operation on closed file

[OK] 正确做法：
  with open("file.txt") as f:
      content = f.read()  # 在 with 块内读取
  print(content)  # 在 with 块外使用读取的内容

记忆技巧：资源在 with 块内使用，结果在 with 块外使用
""")

print("""
[X] 错误5：嵌套 with 语句写法繁琐

繁琐写法：
  with open("a.txt") as f1:
      with open("b.txt") as f2:
          pass

[OK] 简洁写法（Python 3.1+）：
  with open("a.txt") as f1, open("b.txt") as f2:
      pass

记忆技巧：多个上下文管理器用逗号分隔，写在同一行
""")

# ============================================
# 第六部分：函数卡片速查
# ============================================

print("=" * 60)
print("第六部分：函数卡片速查")
print("=" * 60)

print("""
【contextlib.contextmanager】

语法：@contextmanager
作用：将生成器函数转换为上下文管理器
模块：contextlib（标准库）

示例：
  from contextlib import contextmanager

  @contextmanager
  def my_context(param):
      # 准备阶段（__enter__）
      resource = setup(param)
      try:
          yield resource      # 暴露给 with 块
      finally:
          cleanup(resource)   # 清理阶段（__exit__）

────────────────────────────────────

【contextlib.suppress(*exceptions)】

语法：with suppress(ExceptionType):
作用：抑制指定类型的异常
模块：contextlib（标准库）

示例：
  from contextlib import suppress
  with suppress(FileNotFoundError, PermissionError):
      os.remove("可能不存在的文件.txt")

────────────────────────────────────

【上下文管理器类模板】

  class MyContextManager:
      def __enter__(self):
          # 准备资源
          return self  # 或返回资源对象

      def __exit__(self, exc_type, exc_val, exc_tb):
          # 清理资源
          return False  # 不抑制异常
""")

# ============================================
# 第七部分：练习题
# ============================================

print("=" * 60)
print("第七部分：练习题")
print("=" * 60)

print("""
1. 实现一个 indent 上下文管理器
   要求：进入时增加缩进级别，退出时恢复
   示例：
     print("第一级")
     with indent():
         print("  第二级")
         with indent():
             print("    第三级")
         print("  回到第二级")
     print("回到第一级")
   提示：用一个全局变量记录缩进级别

2. 实现一个 transaction 上下文管理器（模拟数据库事务）
   要求：
     - 进入时开始事务（打印"开始事务"）
     - 正常退出时提交（打印"提交事务"）
     - 异常退出时回滚（打印"回滚事务"）
   提示：在 __exit__ 中判断 exc_type 是否为 None

3. 用 contextmanager 重写练习2的 transaction
   提示：用 try/except/finally 结构

4. 思考题：
   - with 语句和 try/finally 有什么本质区别？
   - 什么时候用类实现，什么时候用 contextmanager？
   - Python 内置的哪些对象支持 with 语句？
""")

print("=" * 60)
print("第20天 - 上下文管理器详解 学习完成！")
print("=" * 60)
