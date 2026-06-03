# ============================================
# 调试技巧基础 - 超详细注释版
# 日期：2026-05-31（第14天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【核心概念】调试（Debug）

是什么？
- 调试是发现和修复程序错误（bug）的过程
- Debug = De（去除）+ Bug（错误）
- 是软件开发中最重要的技能之一

为什么需要？
- 程序不可能一次写对，总会有错误
- 快速定位问题，提高开发效率
- 理解程序运行过程，提升编程能力
- 保证代码质量和稳定性

核心特点？
- 系统性：需要有方法和思路
- 耐心性：需要细心和耐心
- 实践性：需要大量练习才能熟练
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 什么是调试？
# --------------------------------------------

print("\n1.1 什么是调试？")
print("-" * 60)

print("""
【调试的定义】

调试（Debug）= 发现问题 + 定位问题 + 解决问题

比喻：
- 就像医生看病：望闻问切 → 诊断 → 治疗
- 就像侦探破案：收集线索 → 分析推理 → 找出真相

调试的三个阶段：
1. 发现问题：程序出错了，或者结果不对
2. 定位问题：找出是哪里出错了
3. 解决问题：修复错误，验证修复效果

为什么调试很重要？
- 写代码只占20%的时间
- 调试代码占80%的时间
- 调试能力 = 开发效率
""")

# --------------------------------------------
# 1.2 常见的 bug 类型
# --------------------------------------------

print("\n1.2 常见的 bug 类型")
print("-" * 60)

print("""
【Bug 类型分类】

1. 语法错误（SyntaxError）
   - 特点：代码无法运行，Python 直接报错
   - 原因：拼写错误、缺少符号、缩进错误
   - 示例：if x = 5:（应该用 ==）
   - 难度：⭐ 最容易发现和修复

2. 逻辑错误（Logic Error）
   - 特点：代码能运行，但结果不对
   - 原因：算法错误、条件判断错误、循环错误
   - 示例：计算平均分时忘记除以人数
   - 难度：⭐⭐⭐ 需要仔细分析

3. 运行时错误（RuntimeError）
   - 特点：代码运行到某处时崩溃
   - 原因：除零、索引越界、类型错误
   - 示例：list[10]（列表只有5个元素）
   - 难度：⭐⭐ 看错误信息就能定位

4. 性能问题（Performance Issue）
   - 特点：代码能运行，但很慢
   - 原因：算法效率低、重复计算、内存泄漏
   - 示例：嵌套循环处理大数据
   - 难度：⭐⭐⭐⭐ 需要性能分析工具
""")

# --------------------------------------------
# 1.3 调试的基本思路
# --------------------------------------------

print("\n1.3 调试的基本思路")
print("-" * 60)

print("""
【调试的黄金法则】

1. 先重现问题
   - 能稳定重现才能调试
   - 记录重现步骤和条件
   - 准备测试数据

2. 缩小问题范围
   - 使用二分法定位
   - 从大范围到小范围
   - 找出最小重现代码

3. 理解问题原因
   - 不要盲目修改代码
   - 理解为什么会出错
   - 找出根本原因

4. 验证修复效果
   - 确保问题真正解决
   - 测试边界情况
   - 避免引入新问题

记忆口诀：
重现 → 定位 → 理解 → 修复 → 验证
""")

# ============================================
# 第二部分：print 调试技巧 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第二部分：print 调试技巧")
print("=" * 60)

# --------------------------------------------
# 2.1 基本 print 调试
# --------------------------------------------

print("\n2.1 基本 print 调试")
print("-" * 60)

print("""
【print 调试的基本方法】

作用：在关键位置打印变量值，观察程序执行过程

基本用法：
1. 打印变量值
2. 打印变量类型
3. 打印执行位置
4. 打印分隔线

优点：
- 简单直接，容易上手
- 不需要额外工具
- 适合快速排查问题

缺点：
- 需要手动添加和删除
- 输出信息混乱
- 不适合生产环境
""")

# 示例：调试一个有问题的函数
print("\n示例：调试计算平均分的函数")

def calculate_average_buggy(scores):
    """计算平均分（有 bug 的版本）"""
    total = 0
    for score in scores:
        total = score  # Bug：应该是 total += score
    average = total / len(scores)
    return average

# 不调试，直接运行
scores = [85, 92, 78, 95, 88]
result = calculate_average_buggy(scores)
print(f"平均分：{result}")  # 结果错误：17.6（应该是87.6）

print("\n使用 print 调试：")

def calculate_average_debug(scores):
    """计算平均分（带调试信息）"""
    print("=" * 40)  # 分隔线，方便识别
    print("开始调试 calculate_average")
    print(f"输入参数 scores: {scores}")
    print(f"scores 类型: {type(scores)}")
    print(f"scores 长度: {len(scores)}")
    print("-" * 40)

    total = 0
    print(f"初始 total: {total}")

    for i, score in enumerate(scores):
        print(f"循环第 {i+1} 次，当前 score: {score}")
        total = score  # Bug 在这里
        print(f"  更新后 total: {total}")  # 观察 total 的变化

    print("-" * 40)
    print(f"循环结束，最终 total: {total}")
    print(f"len(scores): {len(scores)}")

    average = total / len(scores)
    print(f"计算结果 average: {average}")
    print("=" * 40)

    return average

# 运行带调试信息的版本
result = calculate_average_debug(scores)

print("""
【调试分析】

从调试信息可以看出：
1. total 每次循环都被重新赋值，而不是累加
2. 最终 total = 88（最后一个元素）
3. 88 / 5 = 17.6（错误结果）

问题定位：total = score 应该改为 total += score
""")

# --------------------------------------------
# 2.2 高级 print 调试技巧
# --------------------------------------------

print("\n2.2 高级 print 调试技巧")
print("-" * 60)

print("""
【高级技巧】

1. 使用 f-string 格式化输出
   print(f"变量名={变量值}, 类型={type(变量)}")

2. 打印变量的多种信息
   print(f"len={len(x)}, type={type(x)}, value={x}")

3. 使用分隔线区分不同部分
   print("=" * 60)  # 主分隔线
   print("-" * 60)  # 次分隔线

4. 添加位置标记
   print("[DEBUG] 函数名 - 步骤描述")

5. 打印调用栈信息
   import traceback
   traceback.print_stack()
""")

# 示例：高级调试技巧
print("\n示例：调试列表操作")

def process_list_debug(data):
    """处理列表数据（带高级调试）"""
    print("\n[DEBUG] process_list_debug 开始")
    print(f"[DEBUG] 输入: data={data}, len={len(data)}, type={type(data)}")

    # 过滤出偶数
    print("\n[DEBUG] 步骤1：过滤偶数")
    even_numbers = [x for x in data if x % 2 == 0]
    print(f"[DEBUG] 结果: even_numbers={even_numbers}")

    # 计算平方
    print("\n[DEBUG] 步骤2：计算平方")
    squares = [x ** 2 for x in even_numbers]
    print(f"[DEBUG] 结果: squares={squares}")

    # 求和
    print("\n[DEBUG] 步骤3：求和")
    total = sum(squares)
    print(f"[DEBUG] 结果: total={total}")

    print("[DEBUG] process_list_debug 结束\n")
    return total

# 测试
data = [1, 2, 3, 4, 5, 6]
result = process_list_debug(data)
print(f"最终结果：{result}")

# ============================================
# 第三部分：logging 模块详解 ⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第三部分：logging 模块详解")
print("=" * 60)

# --------------------------------------------
# 3.1 为什么需要 logging？
# --------------------------------------------

print("\n3.1 为什么需要 logging？")
print("-" * 60)

print("""
【logging 模块的优势】

print 调试的问题：
- 需要手动添加和删除
- 无法控制输出级别
- 不能输出到文件
- 没有时间戳和上下文信息

logging 模块的优势：
- 可以控制日志级别（开发时详细，生产时简洁）
- 可以输出到文件，方便后续分析
- 自动添加时间戳和其他信息
- 不需要删除，只需调整级别
- 支持多种输出格式和目标

什么时候用 print，什么时候用 logging？
- print：快速调试，临时使用
- logging：生产环境，长期使用
""")

# --------------------------------------------
# 3.2 logging 的日志级别
# --------------------------------------------

print("\n3.2 logging 的日志级别")
print("-" * 60)

print("""
【日志级别】

从低到高：
DEBUG < INFO < WARNING < ERROR < CRITICAL

级别说明：
1. DEBUG（调试）
   - 最详细的信息
   - 用于诊断问题
   - 只在开发时使用

2. INFO（信息）
   - 一般信息
   - 程序正常运行的记录
   - 例如：程序启动、配置加载

3. WARNING（警告）
   - 警告信息
   - 程序能运行，但有潜在问题
   - 例如：配置文件不存在，使用默认值

4. ERROR（错误）
   - 错误信息
   - 程序某个功能失败
   - 例如：数据库连接失败

5. CRITICAL（严重）
   - 严重错误
   - 程序可能崩溃
   - 例如：系统资源耗尽

使用原则：
- 开发环境：设置为 DEBUG，看到所有信息
- 生产环境：设置为 INFO 或 WARNING，只看重要信息
""")

# --------------------------------------------
# 3.3 logging 基本使用
# --------------------------------------------

print("\n3.3 logging 基本使用")
print("-" * 60)

# 导入 logging 模块
import logging

print("""
【logging.basicConfig() 函数卡片】

作用：配置日志系统的基本设置
语法：logging.basicConfig(level=..., format=..., ...)

常用参数：
1. level（日志级别）
   - logging.DEBUG：显示所有日志
   - logging.INFO：显示 INFO 及以上
   - logging.WARNING：显示 WARNING 及以上（默认）

2. format（日志格式）
   - %(asctime)s：时间
   - %(levelname)s：级别
   - %(message)s：消息
   - %(filename)s：文件名
   - %(lineno)d：行号

3. filename（输出文件）
   - 指定日志文件路径
   - 不指定则输出到控制台

示例：
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
""")

# 配置 logging
logging.basicConfig(
    level=logging.DEBUG,  # 设置日志级别为 DEBUG
    format='%(asctime)s - %(levelname)s - %(message)s',  # 设置日志格式
    datefmt='%Y-%m-%d %H:%M:%S'  # 设置时间格式
)

print("\n示例：使用 logging 记录日志")

# 记录不同级别的日志
logging.debug("这是 DEBUG 级别的日志 - 详细的调试信息")
logging.info("这是 INFO 级别的日志 - 一般信息")
logging.warning("这是 WARNING 级别的日志 - 警告信息")
logging.error("这是 ERROR 级别的日志 - 错误信息")
logging.critical("这是 CRITICAL 级别的日志 - 严重错误")

# --------------------------------------------
# 3.4 logging 实际应用
# --------------------------------------------

print("\n3.4 logging 实际应用")
print("-" * 60)

print("\n示例：使用 logging 调试函数")

def divide_with_logging(a, b):
    """带日志记录的除法函数"""
    logging.debug(f"divide_with_logging 被调用，参数: a={a}, b={b}")

    # 参数验证
    if not isinstance(a, (int, float)) or not isinstance(b, (int, float)):
        logging.error(f"参数类型错误: a={type(a)}, b={type(b)}")
        return None

    # 除零检查
    if b == 0:
        logging.error("除数为零，无法计算")
        return None

    # 计算
    result = a / b
    logging.info(f"计算成功: {a} / {b} = {result}")

    return result

# 测试
print("\n测试1：正常情况")
divide_with_logging(10, 2)

print("\n测试2：除零错误")
divide_with_logging(10, 0)

print("\n测试3：类型错误")
divide_with_logging("10", 2)

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
错误1：print 调试信息太多，难以阅读

❌ 错误代码：
for i in range(100):
    print(f"i={i}")  # 输出100行，难以找到关键信息

问题：
- 输出信息太多，淹没了关键信息
- 难以快速定位问题

✅ 正确代码：
for i in range(100):
    if i % 10 == 0:  # 只打印关键位置
        print(f"[DEBUG] 进度: i={i}")

记忆技巧：
- 只在关键位置打印
- 使用条件控制输出频率
- 添加 [DEBUG] 标记方便识别
""")

print("""
错误2：忘记删除调试代码

❌ 错误代码：
def calculate(x):
    print(f"x={x}")  # 调试代码忘记删除
    result = x * 2
    print(f"result={result}")  # 调试代码忘记删除
    return result

问题：
- 生产环境输出大量调试信息
- 影响性能和用户体验

✅ 正确代码：
# 方案1：使用 logging 代替 print
def calculate(x):
    logging.debug(f"x={x}")  # 生产环境不会输出
    result = x * 2
    logging.debug(f"result={result}")
    return result

# 方案2：使用调试标志
DEBUG = False  # 生产环境设为 False

def calculate(x):
    if DEBUG:
        print(f"x={x}")
    result = x * 2
    if DEBUG:
        print(f"result={result}")
    return result

记忆技巧：
- 优先使用 logging 而不是 print
- 或者使用调试标志控制输出
""")

print("""
错误3：logging 配置错误，看不到日志

❌ 错误代码：
import logging

# 没有配置 logging
logging.debug("这条日志看不到")  # 默认级别是 WARNING

问题：
- logging 默认级别是 WARNING
- DEBUG 和 INFO 级别的日志不会显示

✅ 正确代码：
import logging

# 配置 logging
logging.basicConfig(level=logging.DEBUG)

logging.debug("现在可以看到了")

记忆技巧：
- 使用 logging 前必须先配置
- 开发环境设置为 DEBUG 级别
""")

print("""
错误4：盲目修改代码，不理解问题原因

❌ 错误做法：
# 程序出错了，随便改改试试
# 改了10个地方，不知道哪个有效
# 引入了新的 bug

问题：
- 不理解问题原因
- 盲目修改代码
- 可能引入新问题

✅ 正确做法：
1. 先重现问题
2. 使用调试工具定位问题
3. 理解问题的根本原因
4. 有针对性地修改
5. 验证修复效果

记忆技巧：
- 不要盲目修改代码
- 先理解，再修改
- 一次只改一个地方
""")

print("""
错误5：不看错误信息，直接问别人

❌ 错误做法：
# 程序报错了
# 不看错误信息
# 直接问："我的代码为什么不行？"

问题：
- Python 的错误信息很详细
- 包含错误类型、位置、原因
- 不看错误信息就无法定位问题

✅ 正确做法：
1. 仔细阅读错误信息
2. 找出错误类型（SyntaxError、TypeError等）
3. 找出错误位置（文件名、行号）
4. 理解错误原因
5. 尝试自己解决
6. 实在不行再问别人（带上错误信息）

记忆技巧：
- 错误信息是最好的老师
- 学会阅读和理解错误信息
- 90%的问题都能从错误信息中找到答案
""")

print("""
错误6：调试时修改了太多代码

❌ 错误做法：
# 为了调试，修改了很多代码
# 调试完忘记改回来
# 或者不知道改了哪些地方

问题：
- 调试代码和正式代码混在一起
- 容易忘记恢复
- 可能引入新问题

✅ 正确做法：
# 方案1：使用版本控制（git）
git stash  # 保存当前修改
# 调试...
git stash pop  # 恢复修改

# 方案2：复制一份代码调试
# 在副本上调试，不影响原代码

# 方案3：使用 logging，不修改原代码
# 只需要添加 logging 语句，不改变逻辑

记忆技巧：
- 使用版本控制工具
- 或者使用 logging 代替修改代码
- 调试完及时恢复
""")

print("""
错误7：性能问题用 print 调试

❌ 错误代码：
for i in range(1000000):
    print(f"i={i}")  # 打印100万次，程序变得更慢

问题：
- print 本身很慢
- 大量 print 会严重影响性能
- 无法定位真正的性能瓶颈

✅ 正确代码：
import time

# 使用时间测量定位瓶颈
start = time.time()
for i in range(1000000):
    # 代码...
    pass
end = time.time()
print(f"耗时: {end - start:.2f}秒")

记忆技巧：
- 性能问题用时间测量
- 不要用 print 调试性能问题
- 使用专业的性能分析工具
""")

# ============================================
# 第五部分：调试方法速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：调试方法速查")
print("=" * 60)

print("""
【print 调试】

基本用法：
print(f"变量名={变量值}")
print(f"变量名={变量值}, 类型={type(变量)}, 长度={len(变量)}")

添加标记：
print("[DEBUG] 位置描述")
print("=" * 60)  # 分隔线

适用场景：
- 快速调试
- 临时使用
- 简单问题

────────────────────────────────────

【logging 调试】

配置：
import logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

使用：
logging.debug("调试信息")
logging.info("一般信息")
logging.warning("警告信息")
logging.error("错误信息")
logging.critical("严重错误")

适用场景：
- 生产环境
- 长期使用
- 复杂项目

────────────────────────────────────

【调试思路】

1. 重现问题
   - 记录重现步骤
   - 准备测试数据

2. 定位问题
   - 使用二分法
   - 缩小范围

3. 理解原因
   - 分析错误信息
   - 理解代码逻辑

4. 修复问题
   - 有针对性地修改
   - 一次改一个地方

5. 验证效果
   - 测试修复结果
   - 测试边界情况
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题（自己动手！）")
print("=" * 60)

print("""
练习1：调试计算器
  要求：
  - 下面的计算器有多个 bug
  - 使用 print 调试找出所有 bug
  - 修复 bug 并验证

  代码：
  def calculator(a, b, op):
      if op = "+":  # Bug 1
          return a + b
      elif op == "-":
          return a - b
      elif op == "*":
          return a * b
      elif op == "/":
          return a / b  # Bug 2：没有检查除零
      else:
          return "未知操作"  # Bug 3：应该返回 None 或抛出异常

  提示：至少有3个 bug

练习2：使用 logging 记录程序运行
  要求：
  - 写一个函数 process_data(data)
  - 使用 logging 记录每个步骤
  - 包含 DEBUG、INFO、WARNING、ERROR 四个级别
  - 测试不同的输入数据

  提示：
  - 正常数据：记录 INFO
  - 空数据：记录 WARNING
  - 错误数据：记录 ERROR
  - 详细步骤：记录 DEBUG

练习3：调试排序函数
  要求：
  - 下面的冒泡排序有 bug
  - 使用 print 调试找出 bug
  - 修复并验证

  代码：
  def bubble_sort(arr):
      n = len(arr)
      for i in range(n):
          for j in range(n - i - 1):
              if arr[j] < arr[j + 1]:  # Bug：应该是 >
                  arr[j], arr[j + 1] = arr[j + 1], arr[j]
      return arr

  提示：排序结果是降序，应该是升序

练习4：调试文件读取
  要求：
  - 写一个函数 read_config(filename)
  - 读取配置文件（JSON 格式）
  - 使用 logging 记录所有操作
  - 处理文件不存在、格式错误等异常

  提示：
  - 文件不存在：ERROR
  - 格式错误：ERROR
  - 读取成功：INFO
  - 详细内容：DEBUG

练习5：性能调试
  要求：
  - 下面的代码很慢，找出原因
  - 使用时间测量定位瓶颈
  - 优化代码

  代码：
  def find_duplicates(arr):
      duplicates = []
      for i in range(len(arr)):
          for j in range(i + 1, len(arr)):
              if arr[i] == arr[j] and arr[i] not in duplicates:
                  duplicates.append(arr[i])
      return duplicates

  提示：
  - 嵌套循环效率低
  - 可以用集合（set）优化
""")

print("\n" + "=" * 60)
print("学习完成！")
print("=" * 60)
print("""
恭喜你完成了调试技巧的学习！

你已经掌握了：
✅ 调试的概念和重要性
✅ print 调试的技巧和最佳实践
✅ logging 模块的使用方法
✅ 常见 bug 类型和排查思路
✅ 调试的黄金法则

下一步：
- 完成练习题
- 运行 35_调试技巧_练习.py 进行综合练习
- 在实际项目中应用调试技巧
- 培养良好的调试习惯

记住：
- 调试能力 = 开发效率
- 好的调试习惯能让你事半功倍
- 多练习，多思考，多总结
""")
