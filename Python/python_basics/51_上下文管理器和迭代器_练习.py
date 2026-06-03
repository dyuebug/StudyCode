# ============================================
# 上下文管理器和迭代器 - 综合练习
# 日期：2026-05-31（第20天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【综合练习目标】

项目1：实现实用上下文管理器（计时、事务、临时环境）
项目2：实现自定义迭代器（范围迭代器、分块迭代器）
项目3：综合应用（上下文管理器 + 迭代器）
"""

import time
import os
import functools
from contextlib import contextmanager

# ============================================
# 项目1：实用上下文管理器
# ============================================

print("=" * 60)
print("项目1：实用上下文管理器")
print("=" * 60)

# --------------------------------------------
# 1.1 事务上下文管理器（模拟数据库事务）
# --------------------------------------------

print("\n1.1 事务上下文管理器")
print("-" * 40)

class Transaction:
    """
    模拟数据库事务的上下文管理器

    正常退出 → 提交
    异常退出 → 回滚
    """

    def __init__(self, name="事务"):
        self.name = name
        self.operations = []  # 记录操作

    def execute(self, sql):
        """执行一条操作"""
        self.operations.append(sql)
        print(f"  [执行] {sql}")

    def __enter__(self):
        print(f"[{self.name}] 开始事务")
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if exc_type is None:
            print(f"[{self.name}] 提交事务（共{len(self.operations)}条操作）")
        else:
            print(f"[{self.name}] 回滚事务（异常：{exc_val}）")
            self.operations.clear()
        return False  # 不抑制异常

print("正常事务：")
with Transaction("转账") as tx:
    tx.execute("UPDATE accounts SET balance = balance - 100 WHERE id = 1")
    tx.execute("UPDATE accounts SET balance = balance + 100 WHERE id = 2")

print()
print("异常事务（回滚）：")
try:
    with Transaction("转账") as tx:
        tx.execute("UPDATE accounts SET balance = balance - 100 WHERE id = 1")
        raise ValueError("余额不足，无法转账")
        tx.execute("UPDATE accounts SET balance = balance + 100 WHERE id = 2")
except ValueError as e:
    print(f"捕获异常：{e}")

print()

# --------------------------------------------
# 1.2 临时环境变量上下文管理器
# --------------------------------------------

print("1.2 临时环境变量上下文管理器")
print("-" * 40)

@contextmanager
def temp_env(**kwargs):
    """
    临时设置环境变量，退出后恢复原值

    用法：
      with temp_env(DEBUG="1", LOG_LEVEL="DEBUG"):
          # 在这里 DEBUG 和 LOG_LEVEL 被临时设置
          pass
      # 退出后恢复原来的值
    """
    old_values = {}
    try:
        for key, value in kwargs.items():
            old_values[key] = os.environ.get(key)  # 保存原值
            os.environ[key] = str(value)            # 设置新值
            print(f"  [环境变量] 设置 {key}={value}")
        yield
    finally:
        for key, old_val in old_values.items():
            if old_val is None:
                del os.environ[key]  # 原来没有，删除
            else:
                os.environ[key] = old_val  # 恢复原值
            print(f"  [环境变量] 恢复 {key}={old_val}")

print("临时设置环境变量：")
print(f"设置前 DEBUG={os.environ.get('DEBUG', '未设置')}")
with temp_env(DEBUG="1", APP_ENV="test"):
    print(f"  with块内 DEBUG={os.environ.get('DEBUG')}")
    print(f"  with块内 APP_ENV={os.environ.get('APP_ENV')}")
print(f"设置后 DEBUG={os.environ.get('DEBUG', '未设置')}")

print()

# --------------------------------------------
# 1.3 重试上下文管理器
# --------------------------------------------

print("1.3 重试上下文管理器")
print("-" * 40)

print("1.3 重试装饰器（实用版）")
print("-" * 40)

# 注意：重试逻辑更适合用装饰器实现，而不是上下文管理器
# 因为 contextmanager 只能 yield 一次，无法循环重试
# 这里展示装饰器版本的重试

def retry(times=3, delay=0.0, exceptions=(Exception,)):
    """重试装饰器：函数失败时自动重试"""
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            last_exc = None
            for attempt in range(1, times + 1):
                try:
                    return func(*args, **kwargs)
                except exceptions as e:
                    last_exc = e
                    print(f"  [重试] 第{attempt}次失败：{e}")
                    if attempt < times:
                        time.sleep(delay)
            raise last_exc
        return wrapper
    return decorator

call_count = 0

@retry(times=3, delay=0)
def unstable_connect():
    """模拟不稳定的连接"""
    global call_count
    call_count += 1
    if call_count < 3:
        raise ConnectionError("连接失败")
    return "连接成功"

print("重试装饰器演示：")
try:
    result = unstable_connect()
    print(f"  最终结果：{result}")
except ConnectionError as e:
    print(f"  最终失败：{e}")

print()

# ============================================
# 项目2：自定义迭代器
# ============================================

print("=" * 60)
print("项目2：自定义迭代器")
print("=" * 60)

# --------------------------------------------
# 2.1 范围迭代器（支持浮点数步长）
# --------------------------------------------

print("\n2.1 浮点数范围迭代器")
print("-" * 40)

class FloatRange:
    """
    支持浮点数步长的范围迭代器

    类似 range()，但支持浮点数
    """

    def __init__(self, start, stop, step=1.0):
        if step == 0:
            raise ValueError("step 不能为 0")
        self.start = float(start)
        self.stop = float(stop)
        self.step = float(step)

    def __iter__(self):
        current = self.start
        if self.step > 0:
            while current < self.stop:
                yield round(current, 10)  # 避免浮点精度问题
                current += self.step
        else:
            while current > self.stop:
                yield round(current, 10)
                current += self.step

    def __len__(self):
        import math
        if self.step > 0:
            return max(0, math.ceil((self.stop - self.start) / self.step))
        else:
            return max(0, math.ceil((self.start - self.stop) / (-self.step)))

print("浮点数范围（0到1，步长0.2）：")
for x in FloatRange(0, 1, 0.2):
    print(f"  {x:.1f}")

print(f"\n长度：{len(FloatRange(0, 1, 0.2))}")

print()

# --------------------------------------------
# 2.2 分块迭代器
# --------------------------------------------

print("2.2 分块迭代器（批量处理数据）")
print("-" * 40)

class Chunked:
    """
    分块迭代器：将序列分成固定大小的块

    用于批量处理大数据（如批量插入数据库）
    """

    def __init__(self, iterable, size):
        self.iterable = iterable
        self.size = size

    def __iter__(self):
        chunk = []
        for item in self.iterable:
            chunk.append(item)
            if len(chunk) == self.size:
                yield chunk
                chunk = []
        if chunk:  # 最后一块（可能不满）
            yield chunk

print("将列表分成每块3个：")
data = list(range(1, 11))
for chunk in Chunked(data, 3):
    print(f"  处理批次：{chunk}")

print()

# --------------------------------------------
# 2.3 枚举迭代器（带索引和额外信息）
# --------------------------------------------

print("2.3 增强枚举迭代器")
print("-" * 40)

class EnhancedEnumerate:
    """
    增强版 enumerate：提供更多上下文信息

    返回 (index, item, is_first, is_last) 元组
    """

    def __init__(self, iterable, start=0):
        self.data = list(iterable)  # 需要知道总长度
        self.start = start

    def __iter__(self):
        total = len(self.data)
        for i, item in enumerate(self.data):
            index = i + self.start
            is_first = (i == 0)
            is_last = (i == total - 1)
            yield index, item, is_first, is_last

print("增强枚举（知道是否是第一个/最后一个）：")
fruits = ["苹果", "香蕉", "橙子", "葡萄"]
for idx, fruit, first, last in EnhancedEnumerate(fruits, start=1):
    tags = []
    if first:
        tags.append("第一个")
    if last:
        tags.append("最后一个")
    tag_str = f" [{', '.join(tags)}]" if tags else ""
    print(f"  {idx}. {fruit}{tag_str}")

print()

# ============================================
# 项目3：综合应用
# ============================================

print("=" * 60)
print("项目3：综合应用 - 数据处理框架")
print("=" * 60)

print("""
场景：处理大量数据文件
  - 用上下文管理器管理文件和计时
  - 用迭代器分块处理数据
  - 组合使用，构建完整的数据处理流程
""")

import tempfile
import json

# 创建测试数据文件
test_records = [
    {"id": i, "name": f"用户{i}", "score": (i * 37) % 100}
    for i in range(1, 21)
]

with tempfile.NamedTemporaryFile(mode='w', suffix='.jsonl',
                                  delete=False, encoding='utf-8') as f:
    for record in test_records:
        f.write(json.dumps(record, ensure_ascii=False) + '\n')
    data_file = f.name

# 数据处理迭代器
class JsonlReader:
    """逐行读取 JSONL 文件的迭代器"""

    def __init__(self, filename):
        self.filename = filename

    def __iter__(self):
        with open(self.filename, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if line:
                    yield json.loads(line)

# 数据处理上下文管理器
@contextmanager
def processing_context(name, batch_size=5):
    """数据处理上下文：计时 + 统计"""
    stats = {"processed": 0, "batches": 0, "errors": 0}
    start = time.time()
    print(f"[{name}] 开始处理...")
    try:
        yield stats
    finally:
        elapsed = time.time() - start
        print(f"[{name}] 处理完成：")
        print(f"  处理记录：{stats['processed']} 条")
        print(f"  批次数：{stats['batches']} 批")
        print(f"  错误数：{stats['errors']} 条")
        print(f"  耗时：{elapsed:.4f}秒")

# 运行数据处理
print("运行数据处理流程：")
with processing_context("成绩分析") as stats:
    reader = JsonlReader(data_file)
    for batch in Chunked(reader, size=5):
        stats["batches"] += 1
        high_scores = [r for r in batch if r["score"] >= 60]
        stats["processed"] += len(batch)
        print(f"  批次{stats['batches']}：{len(batch)}条，"
              f"及格{len(high_scores)}条")

os.unlink(data_file)

print()

# ============================================
# 总结
# ============================================

print("=" * 60)
print("综合练习总结")
print("=" * 60)

print("""
【上下文管理器核心要点】
  [OK] with 语句：进入调用 __enter__，退出调用 __exit__
  [OK] __exit__ 无论是否异常都会执行
  [OK] __exit__ 返回 True 可以抑制异常
  [OK] contextmanager 装饰器：用 yield 简化实现
  [OK] contextmanager 里的清理代码必须放在 finally 里

【迭代器协议核心要点】
  [OK] 可迭代对象：有 __iter__
  [OK] 迭代器：有 __iter__ 和 __next__
  [OK] 迭代器的 __iter__ 返回自身
  [OK] __next__ 耗尽时必须 raise StopIteration
  [OK] 生成器是迭代器的一种特殊形式

【组合使用】
  [OK] 上下文管理器管理资源生命周期
  [OK] 迭代器处理数据流
  [OK] 两者结合构建健壮的数据处理框架
""")

print("=" * 60)
print("第20天 - 综合练习 完成！")
print("=" * 60)
