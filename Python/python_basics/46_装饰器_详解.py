# ============================================
# 装饰器 - 超详细注释版
# 日期：2026-05-30（第19天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【核心概念】装饰器（Decorator）

是什么？
- 装饰器是一个函数，它接收一个函数作为参数，返回一个新函数
- 用于在不修改原函数代码的情况下，给函数添加额外功能

为什么需要？
- 遵循"开放封闭原则"：对扩展开放，对修改封闭
- 避免重复代码：日志、计时、权限检查等通用功能可以复用
- 代码更简洁：用@语法糖一行代码添加功能

核心特点？
- 本质是高阶函数（接收函数，返回函数）
- @语法糖让使用更简洁
- 可以叠加多个装饰器
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 理解装饰器的本质
# --------------------------------------------

print("\n1.1 装饰器的本质")
print("-" * 60)

print("""
【装饰器的本质】

装饰器 = 接收函数 → 返回新函数 的函数

最简单的理解：
  def decorator(func):      # 接收一个函数
      def wrapper():        # 定义一个新函数
          # 在原函数前后添加功能
          func()            # 调用原函数
      return wrapper        # 返回新函数

使用装饰器：
  @decorator
  def my_func():
      pass

等价于：
  my_func = decorator(my_func)

关键点：
  1. 装饰器是一个函数
  2. 它接收一个函数作为参数
  3. 它返回一个新函数（通常叫 wrapper）
  4. @语法糖只是简写形式
""")

# 演示：不用@语法糖的装饰器
print("演示：手动使用装饰器（不用@语法糖）")
print("-" * 40)

def simple_decorator(func):
    """最简单的装饰器"""
    def wrapper():
        print("【装饰器】函数执行前")
        func()
        print("【装饰器】函数执行后")
    return wrapper

def say_hello():
    print("Hello, World!")

# 手动装饰：等价于 @simple_decorator
say_hello = simple_decorator(say_hello)
say_hello()

print()

# --------------------------------------------
# 1.2 @语法糖
# --------------------------------------------

print("1.2 @语法糖（推荐写法）")
print("-" * 60)

def simple_decorator2(func):
    """同样的装饰器，用@语法糖"""
    def wrapper():
        print("【装饰器】函数执行前")
        func()
        print("【装饰器】函数执行后")
    return wrapper

@simple_decorator2  # 等价于：greet = simple_decorator2(greet)
def greet():
    print("你好，Python！")

greet()

print()

# --------------------------------------------
# 1.3 重要函数：functools.wraps ⭐⭐⭐⭐⭐
# --------------------------------------------

print("1.3 functools.wraps 详解")
print("-" * 60)

print("""
【函数卡片】

函数名：functools.wraps(func)
作用：保留被装饰函数的元信息（名称、文档字符串等）
模块：functools（标准库）

为什么需要？
  不用wraps时，装饰后的函数名变成了wrapper
  用wraps后，装饰后的函数名保持原来的名字

完整语法：
  from functools import wraps

  def decorator(func):
      @wraps(func)          # 保留func的元信息
      def wrapper(*args, **kwargs):
          # 额外功能
          return func(*args, **kwargs)
      return wrapper

参数：
  func - 被装饰的原函数

使用示例：
  @wraps(func)
  def wrapper(*args, **kwargs):
      return func(*args, **kwargs)
""")

import functools

# 不用wraps的问题
def bad_decorator(func):
    def wrapper():
        func()
    return wrapper

@bad_decorator
def my_function():
    """这是我的函数"""
    pass

print(f"不用wraps - 函数名：{my_function.__name__}")
print(f"不用wraps - 文档：{my_function.__doc__}")

print()

# 用wraps的正确做法
def good_decorator(func):
    @functools.wraps(func)  # 保留原函数的元信息
    def wrapper():
        func()
    return wrapper

@good_decorator
def my_function2():
    """这是我的函数"""
    pass

print(f"用wraps - 函数名：{my_function2.__name__}")
print(f"用wraps - 文档：{my_function2.__doc__}")

print()

# ============================================
# 第二部分：基础操作实践
# ============================================

print("=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 基础装饰器（无参数函数）
# --------------------------------------------

print("\n2.1 基础装饰器")
print("-" * 60)

def log_decorator(func):
    """日志装饰器：记录函数调用"""
    @functools.wraps(func)
    def wrapper():
        print(f"[日志] 开始调用：{func.__name__}")
        func()
        print(f"[日志] 调用完成：{func.__name__}")
    return wrapper

@log_decorator
def process_data():
    print("正在处理数据...")

process_data()

print()

# --------------------------------------------
# 2.2 处理有参数的函数 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("2.2 处理有参数的函数（*args, **kwargs）")
print("-" * 60)

print("""
关键技巧：使用 *args 和 **kwargs 接收任意参数
  *args   - 接收任意数量的位置参数（元组）
  **kwargs - 接收任意数量的关键字参数（字典）

这样装饰器就能装饰任何函数，不管它有多少参数
""")

def log_with_args(func):
    """能处理任意参数的日志装饰器"""
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        # *args 接收所有位置参数
        # **kwargs 接收所有关键字参数
        print(f"[日志] 调用 {func.__name__}，参数：{args}，关键字参数：{kwargs}")
        result = func(*args, **kwargs)  # 传递所有参数给原函数
        print(f"[日志] {func.__name__} 返回：{result}")
        return result  # 返回原函数的返回值
    return wrapper

@log_with_args
def add(a, b):
    return a + b

@log_with_args
def greet_person(name, greeting="你好"):
    return f"{greeting}，{name}！"

add(3, 5)
print()
greet_person("小明", greeting="早上好")

print()

# --------------------------------------------
# 2.3 计时装饰器（实用示例）
# --------------------------------------------

print("2.3 计时装饰器")
print("-" * 60)

import time

def timer(func):
    """计时装饰器：测量函数执行时间"""
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.time()        # 记录开始时间
        result = func(*args, **kwargs)  # 执行原函数
        end_time = time.time()          # 记录结束时间
        elapsed = end_time - start_time # 计算耗时
        print(f"[计时] {func.__name__} 执行耗时：{elapsed:.4f}秒")
        return result
    return wrapper

@timer
def slow_function():
    """模拟耗时操作"""
    time.sleep(0.1)  # 暂停0.1秒
    return "完成"

@timer
def calculate_sum(n):
    """计算1到n的和"""
    return sum(range(n))

slow_function()
result = calculate_sum(1000000)
print(f"计算结果：{result}")

print()

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("""
==============================================
🔍 深入理解：装饰器的执行顺序（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【装饰器的执行时机】

装饰器在函数定义时就执行，不是在调用时！

@decorator
def func():
    pass

等价于：
  func = decorator(func)  ← 这行在模块加载时就执行了

所以：
  1. 定义函数时：decorator(func) 被调用，返回 wrapper
  2. 调用函数时：wrapper() 被调用

【多个装饰器的顺序】

@decorator_a
@decorator_b
def func():
    pass

等价于：
  func = decorator_a(decorator_b(func))

执行顺序：
  - 装饰时：从下到上（先b后a）
  - 调用时：从上到下（先a后b）

==============================================
""")

# --------------------------------------------
# 3.1 多个装饰器叠加
# --------------------------------------------

print("3.1 多个装饰器叠加")
print("-" * 60)

def decorator_a(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        print("[A] 开始")
        result = func(*args, **kwargs)
        print("[A] 结束")
        return result
    return wrapper

def decorator_b(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        print("[B] 开始")
        result = func(*args, **kwargs)
        print("[B] 结束")
        return result
    return wrapper

@decorator_a  # 外层（后执行）
@decorator_b  # 内层（先执行）
def my_func():
    print("执行函数体")

print("调用 my_func()：")
my_func()

print()

# ============================================
# 第四部分：带参数的装饰器 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第四部分：带参数的装饰器")
print("=" * 60)

print("""
【带参数的装饰器】

有时候我们需要给装饰器传参数，比如：
  @retry(times=3)       # 重试3次
  @cache(timeout=60)    # 缓存60秒

实现方式：再包一层函数！

def decorator_factory(参数):    # 接收装饰器参数
    def decorator(func):        # 接收被装饰的函数
        @wraps(func)
        def wrapper(*args, **kwargs):
            # 使用外层的参数
            return func(*args, **kwargs)
        return wrapper
    return decorator            # 返回真正的装饰器

使用：
  @decorator_factory(参数)
  def my_func():
      pass
""")

# --------------------------------------------
# 4.1 带参数的重试装饰器
# --------------------------------------------

print("4.1 带参数的重试装饰器")
print("-" * 60)

def retry(times=3, exceptions=(Exception,)):
    """
    重试装饰器：函数失败时自动重试

    参数：
      times - 最大重试次数，默认3次
      exceptions - 捕获的异常类型，默认所有异常
    """
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            last_exception = None
            for attempt in range(1, times + 1):
                try:
                    return func(*args, **kwargs)
                except exceptions as e:
                    last_exception = e
                    print(f"[重试] 第{attempt}次失败：{e}")
                    if attempt < times:
                        print(f"[重试] 准备第{attempt + 1}次尝试...")
            print(f"[重试] 已达最大重试次数({times}次)，放弃")
            raise last_exception
        return wrapper
    return decorator

# 模拟一个不稳定的函数
import random
call_count = 0

@retry(times=3)
def unstable_function():
    """模拟不稳定的函数（随机失败）"""
    global call_count
    call_count += 1
    if call_count < 3:  # 前两次失败，第三次成功
        raise ValueError(f"第{call_count}次调用失败")
    return "成功！"

try:
    result = unstable_function()
    print(f"最终结果：{result}")
except Exception as e:
    print(f"最终失败：{e}")

print()

# --------------------------------------------
# 4.2 带参数的权限检查装饰器
# --------------------------------------------

print("4.2 带参数的权限检查装饰器")
print("-" * 60)

def require_permission(permission):
    """
    权限检查装饰器

    参数：
      permission - 需要的权限名称
    """
    def decorator(func):
        @functools.wraps(func)
        def wrapper(user, *args, **kwargs):
            # 检查用户是否有所需权限
            user_permissions = user.get("permissions", [])
            if permission not in user_permissions:
                print(f"[权限] 拒绝：用户 {user['name']} 没有 {permission} 权限")
                return None
            print(f"[权限] 允许：用户 {user['name']} 有 {permission} 权限")
            return func(user, *args, **kwargs)
        return wrapper
    return decorator

@require_permission("admin")
def delete_user(user, target_id):
    print(f"删除用户 {target_id}")
    return True

@require_permission("read")
def view_data(user):
    print("查看数据")
    return "数据内容"

admin_user = {"name": "管理员", "permissions": ["admin", "read", "write"]}
normal_user = {"name": "普通用户", "permissions": ["read"]}

delete_user(admin_user, 42)
delete_user(normal_user, 42)
print()
view_data(normal_user)

print()

# ============================================
# 第五部分：类装饰器（进阶）
# ============================================

print("=" * 60)
print("第五部分：类装饰器")
print("=" * 60)

print("""
【类装饰器】

除了函数，类也可以作为装饰器。
类装饰器需要实现 __call__ 方法。

class MyDecorator:
    def __init__(self, func):
        self.func = func
        functools.update_wrapper(self, func)  # 保留元信息

    def __call__(self, *args, **kwargs):
        # 额外功能
        return self.func(*args, **kwargs)

使用场景：
  - 需要维护状态（如调用次数）
  - 逻辑比较复杂，用类更清晰
""")

class CallCounter:
    """统计函数调用次数的类装饰器"""

    def __init__(self, func):
        self.func = func
        self.count = 0  # 调用次数
        functools.update_wrapper(self, func)  # 保留原函数元信息

    def __call__(self, *args, **kwargs):
        self.count += 1  # 每次调用计数+1
        print(f"[计数] {self.func.__name__} 第{self.count}次调用")
        return self.func(*args, **kwargs)

@CallCounter
def say_hi(name):
    print(f"Hi, {name}!")

say_hi("Alice")
say_hi("Bob")
say_hi("Charlie")
print(f"总调用次数：{say_hi.count}")

print()

# ============================================
# 第六部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第六部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
❌ 错误1：忘记返回原函数的返回值

错误代码：
  def decorator(func):
      def wrapper(*args, **kwargs):
          func(*args, **kwargs)  # 没有return！
      return wrapper

结果：装饰后的函数总是返回None

✅ 正确代码：
  def decorator(func):
      def wrapper(*args, **kwargs):
          return func(*args, **kwargs)  # 必须return！
      return wrapper

记忆技巧：wrapper里调用func，一定要return结果
""")

print("""
❌ 错误2：忘记使用 @functools.wraps

错误代码：
  def decorator(func):
      def wrapper(*args, **kwargs):
          return func(*args, **kwargs)
      return wrapper  # 没有@wraps

结果：func.__name__ 变成了 "wrapper"，调试困难

✅ 正确代码：
  def decorator(func):
      @functools.wraps(func)  # 必须加！
      def wrapper(*args, **kwargs):
          return func(*args, **kwargs)
      return wrapper

记忆技巧：写装饰器时，wrapper上面第一行就加@wraps
""")

print("""
❌ 错误3：带参数装饰器少包了一层

错误代码：
  def retry(func, times=3):  # 错误！参数和func混在一起
      def wrapper(*args, **kwargs):
          ...
      return wrapper

结果：@retry(times=3) 会报错

✅ 正确代码：
  def retry(times=3):         # 外层接收装饰器参数
      def decorator(func):    # 内层接收被装饰函数
          def wrapper(*args, **kwargs):
              ...
          return wrapper
      return decorator        # 返回真正的装饰器

记忆技巧：带参数的装饰器 = 三层嵌套（参数→函数→wrapper）
""")

print("""
❌ 错误4：装饰器修改了函数签名

错误代码：
  def decorator(func):
      def wrapper():  # 没有*args, **kwargs！
          return func()
      return wrapper

  @decorator
  def add(a, b):  # 有参数的函数
      return a + b

结果：add(1, 2) 报错，因为wrapper()不接受参数

✅ 正确代码：
  def decorator(func):
      def wrapper(*args, **kwargs):  # 接收任意参数
          return func(*args, **kwargs)
      return wrapper

记忆技巧：wrapper永远用*args, **kwargs，除非你明确知道参数
""")

print("""
❌ 错误5：在装饰器外层修改了被装饰函数

错误代码：
  @decorator
  def my_func():
      pass

  my_func = some_other_function  # 覆盖了装饰后的函数！

结果：装饰器失效

✅ 正确做法：
  不要在装饰后再重新赋值函数名

记忆技巧：@decorator 之后，函数名就是装饰后的版本，不要覆盖
""")

# ============================================
# 第七部分：函数卡片速查
# ============================================

print("=" * 60)
print("第七部分：函数卡片速查")
print("=" * 60)

print("""
【functools.wraps(func)】

语法：@functools.wraps(func)
作用：保留被装饰函数的元信息（__name__、__doc__等）
模块：functools（标准库，需要import）

示例：
  import functools
  def decorator(func):
      @functools.wraps(func)
      def wrapper(*args, **kwargs):
          return func(*args, **kwargs)
      return wrapper

────────────────────────────────────

【time.time()】

语法：time.time()
作用：返回当前时间的时间戳（浮点数，单位：秒）
模块：time（标准库）

示例：
  import time
  start = time.time()
  # 执行一些操作
  end = time.time()
  print(f"耗时：{end - start:.4f}秒")

────────────────────────────────────

【装饰器模板（最常用）】

  import functools

  def my_decorator(func):
      @functools.wraps(func)
      def wrapper(*args, **kwargs):
          # 函数执行前的操作
          result = func(*args, **kwargs)
          # 函数执行后的操作
          return result
      return wrapper

【带参数的装饰器模板】

  import functools

  def my_decorator(param1, param2="default"):
      def decorator(func):
          @functools.wraps(func)
          def wrapper(*args, **kwargs):
              # 可以使用 param1, param2
              return func(*args, **kwargs)
          return wrapper
      return decorator
""")

# ============================================
# 第八部分：练习题
# ============================================

print("=" * 60)
print("第八部分：练习题")
print("=" * 60)

print("""
1. 实现一个 @validate_types 装饰器
   要求：检查函数参数的类型是否符合预期
   提示：可以用 isinstance() 检查类型
   示例：
     @validate_types(int, int)
     def add(a, b):
         return a + b
     add(1, 2)    # 正常
     add("1", 2)  # 报错：参数类型不匹配

2. 实现一个 @cache 装饰器（简单版）
   要求：缓存函数的计算结果，相同参数直接返回缓存
   提示：用字典存储 {参数: 结果}
   示例：
     @cache
     def fibonacci(n):
         if n <= 1: return n
         return fibonacci(n-1) + fibonacci(n-2)
     # 第一次计算，第二次直接返回缓存

3. 实现一个 @rate_limit 装饰器
   要求：限制函数在指定时间内的调用次数
   提示：记录调用时间，超过限制时等待或报错
   示例：
     @rate_limit(max_calls=3, period=1.0)  # 每秒最多3次
     def api_call():
         pass

4. 思考题：
   - 装饰器和继承有什么相似之处？有什么不同？
   - 什么时候用装饰器，什么时候用继承？
   - Python内置了哪些装饰器？（提示：@property, @classmethod, @staticmethod）
""")

print("=" * 60)
print("第19天 - 装饰器详解 学习完成！")
print("=" * 60)
