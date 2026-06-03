# ============================================
# psutil 模块 - 超详细注释版
# 日期：2026-05-31（第24天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【核心概念】psutil 模块

是什么？
- psutil (process and system utilities) 是一个跨平台的系统监控库
- 可以获取 CPU、内存、磁盘、网络、进程等信息
- 支持 Windows、Linux、macOS

为什么需要？
- 系统监控：实时监控系统资源使用情况
- 进程管理：查看和管理运行中的进程
- 性能分析：分析系统性能瓶颈
- 运维工具：开发自动化运维工具

核心特点：
- 跨平台：一套代码，多平台运行
- 功能丰富：涵盖系统监控的各个方面
- 实时性：获取实时的系统信息
"""

import psutil
import time
from datetime import datetime

# ============================================
# 第一部分：CPU 监控
# ============================================

print("第一部分：CPU 监控")
print("=" * 60)

# --------------------------------------------
# 1.1 CPU 基本信息
# --------------------------------------------

print("\n1.1 CPU 基本信息")
print("-" * 60)

print("""
【CPU 信息函数】

psutil.cpu_count()              # CPU 逻辑核心数
psutil.cpu_count(logical=False) # CPU 物理核心数
psutil.cpu_freq()               # CPU 频率（当前、最小、最大）
psutil.cpu_percent()            # CPU 使用率（百分比）
psutil.cpu_percent(percpu=True) # 每个核心的使用率
""")

# 示例：获取 CPU 基本信息
print("CPU 基本信息：")
print(f"  逻辑核心数：{psutil.cpu_count()} 个")
print(f"  物理核心数：{psutil.cpu_count(logical=False)} 个")

freq = psutil.cpu_freq()
if freq:
    print(f"  当前频率：{freq.current:.2f} MHz")
    print(f"  最小频率：{freq.min:.2f} MHz")
    print(f"  最大频率：{freq.max:.2f} MHz")

print()

# --------------------------------------------
# 1.2 CPU 使用率
# --------------------------------------------

print("\n1.2 CPU 使用率")
print("-" * 60)

print("""
【CPU 使用率】

psutil.cpu_percent(interval=1)
  - interval: 采样间隔（秒）
  - 返回：总体 CPU 使用率（百分比）

psutil.cpu_percent(interval=1, percpu=True)
  - percpu=True: 返回每个核心的使用率
  - 返回：列表，每个元素是一个核心的使用率

注意：
- 第一次调用返回 0.0（需要间隔时间）
- 建议 interval >= 0.1 秒
""")

# 示例：获取 CPU 使用率
print("CPU 使用率（采样1秒）：")
cpu_percent = psutil.cpu_percent(interval=1)
print(f"  总体使用率：{cpu_percent}%")

cpu_percent_per = psutil.cpu_percent(interval=1, percpu=True)
print(f"  每个核心使用率：")
for i, percent in enumerate(cpu_percent_per):
    print(f"    核心 {i}: {percent}%")

print()

# ============================================
# 第二部分：内存监控
# ============================================

print("=" * 60)
print("第二部分：内存监控")
print("=" * 60)

# --------------------------------------------
# 2.1 内存信息
# --------------------------------------------

print("\n2.1 内存信息")
print("-" * 60)

print("""
【内存信息函数】

psutil.virtual_memory()  # 物理内存信息
  - total: 总内存（字节）
  - available: 可用内存
  - used: 已用内存
  - percent: 使用率（百分比）

psutil.swap_memory()     # 交换内存（虚拟内存）信息
  - total: 总交换内存
  - used: 已用交换内存
  - percent: 使用率
""")

# 示例：获取内存信息
print("物理内存信息：")
mem = psutil.virtual_memory()
print(f"  总内存：{mem.total / (1024**3):.2f} GB")
print(f"  可用内存：{mem.available / (1024**3):.2f} GB")
print(f"  已用内存：{mem.used / (1024**3):.2f} GB")
print(f"  使用率：{mem.percent}%")

print("\n交换内存信息：")
swap = psutil.swap_memory()
print(f"  总交换内存：{swap.total / (1024**3):.2f} GB")
print(f"  已用交换内存：{swap.used / (1024**3):.2f} GB")
print(f"  使用率：{swap.percent}%")

print()

# ============================================
# 第三部分：磁盘监控
# ============================================

print("=" * 60)
print("第三部分：磁盘监控")
print("=" * 60)

# --------------------------------------------
# 3.1 磁盘分区
# --------------------------------------------

print("\n3.1 磁盘分区")
print("-" * 60)

print("""
【磁盘信息函数】

psutil.disk_partitions()  # 所有磁盘分区
  - device: 设备名
  - mountpoint: 挂载点
  - fstype: 文件系统类型

psutil.disk_usage(path)   # 指定路径的磁盘使用情况
  - total: 总容量
  - used: 已用容量
  - free: 可用容量
  - percent: 使用率
""")

# 示例：获取磁盘分区信息
print("磁盘分区信息：")
partitions = psutil.disk_partitions()
for partition in partitions:
    print(f"\n  设备：{partition.device}")
    print(f"  挂载点：{partition.mountpoint}")
    print(f"  文件系统：{partition.fstype}")

    try:
        usage = psutil.disk_usage(partition.mountpoint)
        print(f"  总容量：{usage.total / (1024**3):.2f} GB")
        print(f"  已用：{usage.used / (1024**3):.2f} GB")
        print(f"  可用：{usage.free / (1024**3):.2f} GB")
        print(f"  使用率：{usage.percent}%")
    except PermissionError:
        print(f"  无权限访问")

print()

# --------------------------------------------
# 3.2 磁盘 IO
# --------------------------------------------

print("\n3.2 磁盘 IO")
print("-" * 60)

print("""
【磁盘 IO 函数】

psutil.disk_io_counters()  # 磁盘 IO 统计
  - read_count: 读取次数
  - write_count: 写入次数
  - read_bytes: 读取字节数
  - write_bytes: 写入字节数
""")

# 示例：获取磁盘 IO 信息
print("磁盘 IO 统计：")
disk_io = psutil.disk_io_counters()
if disk_io:
    print(f"  读取次数：{disk_io.read_count}")
    print(f"  写入次数：{disk_io.write_count}")
    print(f"  读取字节：{disk_io.read_bytes / (1024**2):.2f} MB")
    print(f"  写入字节：{disk_io.write_bytes / (1024**2):.2f} MB")

print()

# ============================================
# 第四部分：网络监控
# ============================================

print("=" * 60)
print("第四部分：网络监控")
print("=" * 60)

# --------------------------------------------
# 4.1 网络 IO
# --------------------------------------------

print("\n4.1 网络 IO")
print("-" * 60)

print("""
【网络 IO 函数】

psutil.net_io_counters()  # 网络 IO 统计
  - bytes_sent: 发送字节数
  - bytes_recv: 接收字节数
  - packets_sent: 发送数据包数
  - packets_recv: 接收数据包数
""")

# 示例：获取网络 IO 信息
print("网络 IO 统计：")
net_io = psutil.net_io_counters()
print(f"  发送字节：{net_io.bytes_sent / (1024**2):.2f} MB")
print(f"  接收字节：{net_io.bytes_recv / (1024**2):.2f} MB")
print(f"  发送数据包：{net_io.packets_sent}")
print(f"  接收数据包：{net_io.packets_recv}")

print()

# ============================================
# 第五部分：进程管理
# ============================================

print("=" * 60)
print("第五部分：进程管理")
print("=" * 60)

# --------------------------------------------
# 5.1 进程列表
# --------------------------------------------

print("\n5.1 进程列表")
print("-" * 60)

print("""
【进程管理函数】

psutil.pids()              # 所有进程 ID 列表
psutil.Process(pid)        # 获取指定进程对象

进程对象方法：
  - name(): 进程名称
  - exe(): 可执行文件路径
  - cmdline(): 命令行参数
  - status(): 进程状态
  - cpu_percent(): CPU 使用率
  - memory_info(): 内存信息
  - create_time(): 创建时间
""")

# 示例：列出前5个进程
print("进程列表（前5个）：")
for pid in psutil.pids()[:5]:
    try:
        p = psutil.Process(pid)
        print(f"\n  PID: {pid}")
        print(f"  名称：{p.name()}")
        print(f"  状态：{p.status()}")
        print(f"  CPU：{p.cpu_percent(interval=0.1)}%")
        mem = p.memory_info()
        print(f"  内存：{mem.rss / (1024**2):.2f} MB")
    except (psutil.NoSuchProcess, psutil.AccessDenied):
        pass

print()

# --------------------------------------------
# 5.2 当前进程
# --------------------------------------------

print("\n5.2 当前进程")
print("-" * 60)

print("""
【当前进程】

psutil.Process()  # 当前 Python 进程
""")

# 示例：获取当前进程信息
print("当前 Python 进程信息：")
current = psutil.Process()
print(f"  PID: {current.pid}")
print(f"  名称：{current.name()}")
print(f"  可执行文件：{current.exe()}")
print(f"  CPU：{current.cpu_percent(interval=0.1)}%")
mem = current.memory_info()
print(f"  内存：{mem.rss / (1024**2):.2f} MB")

print()

# ============================================
# 第六部分：深入理解（可选）
# ============================================

print("""
==============================================
深入理解：系统监控的最佳实践（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【监控频率】

CPU 使用率：
  - 推荐间隔：1-5 秒
  - 太频繁：增加系统负担
  - 太慢：无法及时发现问题

内存使用率：
  - 推荐间隔：5-10 秒
  - 内存变化相对缓慢

磁盘使用率：
  - 推荐间隔：30-60 秒
  - 磁盘容量变化很慢

进程监控：
  - 推荐间隔：1-5 秒
  - 进程变化较快

【告警阈值】

CPU 使用率：
  - 警告：> 70%
  - 严重：> 90%

内存使用率：
  - 警告：> 80%
  - 严重：> 95%

磁盘使用率：
  - 警告：> 80%
  - 严重：> 90%

【性能优化】

1. 避免频繁调用
   - 使用缓存
   - 批量获取信息

2. 异步监控
   - 使用多线程
   - 避免阻塞主程序

3. 数据采样
   - 不需要每次都获取所有信息
   - 按需获取

==============================================
""")

# ============================================
# 第七部分：常见错误和陷阱
# ============================================

print("=" * 60)
print("第七部分：常见错误和陷阱")
print("=" * 60)

print("""
[X] 错误1：第一次调用 cpu_percent() 返回 0.0

错误代码：
  cpu = psutil.cpu_percent()  # 返回 0.0

原因：需要间隔时间才能计算使用率

[OK] 正确做法：
  cpu = psutil.cpu_percent(interval=1)  # 采样1秒

记忆技巧：CPU 使用率需要时间间隔

────────────────────────────────────

[X] 错误2：访问不存在的进程

错误代码：
  p = psutil.Process(99999)  # NoSuchProcess

原因：进程可能已经结束

[OK] 正确做法：
  try:
      p = psutil.Process(pid)
  except psutil.NoSuchProcess:
      print("进程不存在")

记忆技巧：进程操作要捕获异常

────────────────────────────────────

[X] 错误3：权限不足

错误代码：
  p = psutil.Process(1)
  p.kill()  # AccessDenied

原因：没有权限操作系统进程

[OK] 正确做法：
  try:
      p.kill()
  except psutil.AccessDenied:
      print("权限不足")

记忆技巧：系统进程需要管理员权限

────────────────────────────────────

[X] 错误4：单位转换错误

错误代码：
  mem = psutil.virtual_memory()
  print(f"{mem.total} GB")  # 错误：单位是字节

[OK] 正确做法：
  print(f"{mem.total / (1024**3):.2f} GB")

记忆技巧：
  - 字节 → KB：/ 1024
  - 字节 → MB：/ (1024**2)
  - 字节 → GB：/ (1024**3)

────────────────────────────────────

[X] 错误5：监控频率过高

错误代码：
  while True:
      cpu = psutil.cpu_percent(interval=0.01)  # 太频繁
      time.sleep(0.01)

原因：增加系统负担，影响性能

[OK] 正确做法：
  while True:
      cpu = psutil.cpu_percent(interval=1)
      time.sleep(1)

记忆技巧：监控间隔至少 1 秒
""")

print()

# ============================================
# 第八部分：常用操作速查
# ============================================

print("=" * 60)
print("第八部分：常用操作速查")
print("=" * 60)

print("""
【CPU 监控】

psutil.cpu_count()                    # 逻辑核心数
psutil.cpu_percent(interval=1)        # CPU 使用率
psutil.cpu_percent(interval=1, percpu=True)  # 每个核心使用率

【内存监控】

mem = psutil.virtual_memory()
mem.total / (1024**3)                 # 总内存（GB）
mem.available / (1024**3)             # 可用内存（GB）
mem.percent                           # 使用率

【磁盘监控】

psutil.disk_partitions()              # 磁盘分区
usage = psutil.disk_usage('/')
usage.total / (1024**3)               # 总容量（GB）
usage.percent                         # 使用率

【网络监控】

net = psutil.net_io_counters()
net.bytes_sent / (1024**2)            # 发送（MB）
net.bytes_recv / (1024**2)            # 接收（MB）

【进程管理】

psutil.pids()                         # 所有进程 ID
p = psutil.Process(pid)               # 获取进程
p.name()                              # 进程名称
p.cpu_percent(interval=0.1)           # CPU 使用率
p.memory_info().rss / (1024**2)       # 内存（MB）
""")

print()

# ============================================
# 第九部分：练习题
# ============================================

print("=" * 60)
print("第九部分：练习题")
print("=" * 60)

print("""
1. 编写函数，监控 CPU 使用率，超过 80% 时打印警告
   提示：使用 cpu_percent(interval=1)

2. 编写函数，找出内存占用最高的 5 个进程
   提示：遍历所有进程，按内存排序

3. 编写函数，检查磁盘使用率，超过 90% 时发出告警
   提示：使用 disk_usage() 和 disk_partitions()

4. 编写函数，实时显示网络速度（每秒发送/接收字节数）
   提示：两次采样的差值 / 时间间隔

5. 思考题：
   - 为什么监控频率不能太高？
   - 如何设计一个完整的系统监控工具？
   - 监控数据应该如何存储？
""")

print("=" * 60)
print("第24天 - psutil 模块详解 学习完成！")
print("=" * 60)
