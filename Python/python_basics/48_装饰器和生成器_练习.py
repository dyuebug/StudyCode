# ============================================
# 装饰器和生成器 - 综合练习
# 日期：2026-05-30（第19天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【综合练习目标】

通过三个实战项目，综合运用装饰器和生成器：
  项目1：实现常用装饰器（日志、计时、缓存、重试）
  项目2：实现生成器（斐波那契、文件读取、数据流处理）
  项目3：装饰器 + 生成器综合应用
"""

import functools
import time
import sys

# ============================================
# 项目1：常用装饰器工具库
# ============================================

print("=" * 60)
print("项目1：常用装饰器工具库")
print("=" * 60)

# --------------------------------------------
# 1.1 日志装饰器
# --------------------------------------------

print("\n1.1 日志装饰器")
print("-" * 40)

def logger(level="INFO"):
    """
    日志装饰器：记录函数调用信息

    参数：
      level - 日志级别（INFO/DEBUG/WARNING）
    """
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            print(f"[{level}] 调用 {func.__name__}，参数：{args} {kwargs}")
            try:
                result = func(*args, **kwargs)
                print(f"[{level}] {func.__name__} 成功，返回：{result}")
                return result
            except Exception as e:
                print(f"[ERROR] {func.__name__} 异常：{e}")
                raise
        return wrapper
    return decorator

@logger("DEBUG")
def divide(a, b):
    return a / b

divide(10, 2)
print()
try:
    divide(10, 0)
except ZeroDivisionError:
    pass

print()

# --------------------------------------------
# 1.2 缓存装饰器（手动实现）
# --------------------------------------------

print("1.2 缓存装饰器")
print("-" * 40)

def simple_cache(func):
    """
    简单缓存装饰器：缓存函数结果，相同参数直接返回缓存
    """
    cache = {}  # 用字典存储缓存：{参数: 结果}

    @functools.wraps(func)
    def wrapper(*args):
        if args in cache:
            print(f"[缓存] 命中缓存，参数：{args}")
            return cache[args]
        print(f"[缓存] 未命中，计算中，参数：{args}")
        result = func(*args)
        cache[args] = result  # 存入缓存
        return result

    wrapper.cache = cache       # 暴露缓存字典，方便调试
    wrapper.cache_clear = lambda: cache.clear()  # 清除缓存的方法
    return wrapper

@simple_cache
def slow_fibonacci(n):
    """递归斐波那契（没有缓存会很慢）"""
    if n <= 1:
        return n
    return slow_fibonacci(n - 1) + slow_fibonacci(n - 2)

print("计算斐波那契数列（带缓存）：")
start = time.time()
for i in range(10):
    print(f"  fib({i}) = {slow_fibonacci(i)}")
elapsed = time.time() - start
print(f"耗时：{elapsed:.4f}秒")
print(f"缓存大小：{len(slow_fibonacci.cache)} 个")

print()

# --------------------------------------------
# 1.3 计时装饰器（带统计）
# --------------------------------------------

print("1.3 计时装饰器（带统计）")
print("-" * 40)

class TimerDecorator:
    """
    计时装饰器（类实现）：统计函数调用时间和次数
    """
    def __init__(self, func):
        self.func = func
        self.call_count = 0
        self.total_time = 0.0
        functools.update_wrapper(self, func)

    def __call__(self, *args, **kwargs):
        self.call_count += 1
        start = time.time()
        result = self.func(*args, **kwargs)
        elapsed = time.time() - start
        self.total_time += elapsed
        print(f"[计时] {self.func.__name__} 第{self.call_count}次：{elapsed:.4f}秒")
        return result

    @property
    def avg_time(self):
        """平均执行时间"""
        if self.call_count == 0:
            return 0
        return self.total_time / self.call_count

    def stats(self):
        """打印统计信息"""
        print(f"\n[统计] {self.func.__name__}：")
        print(f"  调用次数：{self.call_count}")
        print(f"  总耗时：{self.total_time:.4f}秒")
        print(f"  平均耗时：{self.avg_time:.4f}秒")

@TimerDecorator
def process(n):
    """模拟处理任务"""
    time.sleep(0.05)
    return n * 2

process(1)
process(2)
process(3)
process.stats()

print()

# ============================================
# 项目2：生成器工具库
# ============================================

print("=" * 60)
print("项目2：生成器工具库")
print("=" * 60)

# --------------------------------------------
# 2.1 无限序列生成器
# --------------------------------------------

print("\n2.1 无限序列生成器")
print("-" * 40)

def fibonacci():
    """斐波那契数列（无限）"""
    a, b = 0, 1
    while True:
        yield a
        a, b = b, a + b

def primes():
    """素数序列（无限）"""
    def is_prime(n):
        if n < 2:
            return False
        for i in range(2, int(n**0.5) + 1):
            if n % i == 0:
                return False
        return True

    n = 2
    while True:
        if is_prime(n):
            yield n
        n += 1

def take(gen, n):
    """从生成器取前n个元素"""
    for _ in range(n):
        yield next(gen)

print("斐波那契前15个：", list(take(fibonacci(), 15)))
print("素数前10个：", list(take(primes(), 10)))

print()

# --------------------------------------------
# 2.2 数据处理管道
# --------------------------------------------

print("2.2 数据处理管道（生成器链）")
print("-" * 40)

print("""
生成器可以像管道一样串联：
  数据源 → 过滤 → 转换 → 聚合

每个步骤都是惰性的，只在需要时处理数据
""")

def read_numbers(data):
    """数据源：逐个产生数字"""
    for item in data:
        yield item

def filter_even(numbers):
    """过滤：只保留偶数"""
    for n in numbers:
        if n % 2 == 0:
            yield n

def square(numbers):
    """转换：计算平方"""
    for n in numbers:
        yield n ** 2

def running_sum(numbers):
    """聚合：累计求和"""
    total = 0
    for n in numbers:
        total += n
        yield total

# 构建处理管道
raw_data = range(1, 11)
pipeline = running_sum(square(filter_even(read_numbers(raw_data))))

print("处理管道：1-10 → 过滤偶数 → 平方 → 累计求和")
print("结果：", list(pipeline))
# 偶数：2,4,6,8,10 → 平方：4,16,36,64,100 → 累计：4,20,56,120,220

print()

# --------------------------------------------
# 2.3 文件数据流处理
# --------------------------------------------

print("2.3 文件数据流处理")
print("-" * 40)

import tempfile
import os

# 创建测试数据文件
test_data = """Alice,85,90,78
Bob,92,88,95
Charlie,70,75,80
Diana,95,98,92
Eve,60,65,70
"""

with tempfile.NamedTemporaryFile(mode='w', suffix='.csv',
                                  delete=False, encoding='utf-8') as f:
    f.write(test_data)
    temp_file = f.name

def read_csv_rows(filename):
    """逐行读取CSV文件的生成器"""
    with open(filename, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line:
                yield line.split(',')

def parse_student(rows):
    """解析学生数据"""
    for row in rows:
        name = row[0]
        scores = [int(x) for x in row[1:]]
        yield {"name": name, "scores": scores, "avg": sum(scores) / len(scores)}

def filter_passing(students, min_avg=75):
    """过滤及格学生"""
    for student in students:
        if student["avg"] >= min_avg:
            yield student

# 构建处理管道
rows = read_csv_rows(temp_file)
students = parse_student(rows)
passing = filter_passing(students, min_avg=75)

print("及格学生（平均分>=75）：")
for s in passing:
    print(f"  {s['name']}：平均分 {s['avg']:.1f}")

os.unlink(temp_file)

print()

# ============================================
# 项目3：装饰器 + 生成器综合应用
# ============================================

print("=" * 60)
print("项目3：装饰器 + 生成器综合应用")
print("=" * 60)

print("""
场景：构建一个数据处理框架
  - 用装饰器添加日志、计时、错误处理
  - 用生成器实现惰性数据处理管道
  - 组合使用，处理大量数据
""")

# --------------------------------------------
# 3.1 带装饰器的生成器管道
# --------------------------------------------

print("3.1 带装饰器的生成器管道")
print("-" * 40)

def log_pipeline(name):
    """给生成器函数添加日志的装饰器"""
    def decorator(gen_func):
        @functools.wraps(gen_func)
        def wrapper(*args, **kwargs):
            print(f"[管道] 启动：{name}")
            count = 0
            for item in gen_func(*args, **kwargs):
                count += 1
                yield item
            print(f"[管道] 完成：{name}，处理了 {count} 条数据")
        return wrapper
    return decorator

@log_pipeline("数据源")
def data_source(n):
    """生成测试数据"""
    for i in range(1, n + 1):
        yield {"id": i, "value": i * 10}

@log_pipeline("数据过滤")
def filter_data(items, threshold=30):
    """过滤value大于threshold的数据"""
    for item in items:
        if item["value"] > threshold:
            yield item

@log_pipeline("数据转换")
def transform_data(items):
    """转换数据格式"""
    for item in items:
        yield {
            "id": item["id"],
            "value": item["value"],
            "label": f"item_{item['id']}",
            "doubled": item["value"] * 2
        }

# 构建并运行管道
print("运行数据处理管道：")
source = data_source(8)
filtered = filter_data(source, threshold=30)
transformed = transform_data(filtered)

results = list(transformed)
print(f"\n最终结果（{len(results)}条）：")
for r in results:
    print(f"  {r}")

print()

# --------------------------------------------
# 3.2 带缓存的生成器
# --------------------------------------------

print("3.2 带缓存的生成器（避免重复计算）")
print("-" * 40)

def memoize_generator(gen_func):
    """
    缓存生成器结果的装饰器
    注意：会把所有结果存在内存中，适合小数据量
    """
    cache = {}

    @functools.wraps(gen_func)
    def wrapper(*args):
        if args not in cache:
            print(f"[缓存] 首次计算，参数：{args}")
            cache[args] = list(gen_func(*args))  # 计算并缓存所有结果
        else:
            print(f"[缓存] 命中缓存，参数：{args}")
        yield from cache[args]  # 从缓存中逐个yield

    return wrapper

@memoize_generator
def expensive_sequence(n):
    """模拟耗时的序列生成"""
    for i in range(n):
        time.sleep(0.01)  # 模拟耗时操作
        yield i * i

print("第一次调用（需要计算）：")
start = time.time()
result1 = list(expensive_sequence(5))
print(f"结果：{result1}，耗时：{time.time()-start:.3f}秒")

print("\n第二次调用（使用缓存）：")
start = time.time()
result2 = list(expensive_sequence(5))
print(f"结果：{result2}，耗时：{time.time()-start:.3f}秒")

print()

# ============================================
# 总结
# ============================================

print("=" * 60)
print("综合练习总结")
print("=" * 60)

print("""
【装饰器核心要点】
  [OK] 本质：接收函数，返回新函数
  [OK] 必须用 @functools.wraps 保留元信息
  [OK] wrapper 要用 *args, **kwargs 接收任意参数
  [OK] wrapper 要 return 原函数的返回值
  [OK] 带参数的装饰器需要三层嵌套

【生成器核心要点】
  [OK] yield 暂停函数，返回值，下次继续
  [OK] 生成器是惰性的，节省内存
  [OK] 生成器只能遍历一次
  [OK] 生成器表达式：(x for x in ...)
  [OK] yield from 可以委托给另一个可迭代对象

【组合使用】
  [OK] 装饰器可以装饰生成器函数
  [OK] 生成器可以构建数据处理管道
  [OK] 两者结合可以构建强大的数据处理框架
""")

print("=" * 60)
print("第19天 - 综合练习 完成！")
print("=" * 60)
