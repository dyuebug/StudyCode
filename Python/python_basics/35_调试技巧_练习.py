# ============================================
# 调试技巧综合练习 - 实战项目
# 日期：2026-05-31（第14天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【综合练习目标】

通过实际项目综合练习调试技巧：
1. print 调试技巧
2. logging 模块使用
3. 常见 bug 排查
4. 性能问题定位

这个练习重点是体验：
- 如何快速定位问题
- 如何使用调试工具
- 如何分析和解决 bug
- 如何建立调试思维
"""

import logging

# 配置 logging
logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s',
    datefmt='%Y-%m-%d %H:%M:%S'
)

# ============================================
# 项目1：调试有 bug 的计算器
# ============================================

print("=" * 60)
print("项目1：调试有 bug 的计算器")
print("=" * 60)

print("""
任务：下面的计算器有多个 bug，使用 print 调试找出并修复

原始代码（有 bug）：
def calculator_buggy(a, b, op):
    if op = "+":  # Bug 1：应该用 ==
        return a + b
    elif op == "-":
        return a - b
    elif op == "*":
        return a * b
    elif op == "/":
        return a / b  # Bug 2：没有检查除零
    else:
        return "未知操作"  # Bug 3：应该返回 None 或抛出异常
""")

# 修复后的版本（带调试信息）
def calculator_fixed(a, b, op):
    """修复后的计算器（带调试信息）"""
    print("\n[DEBUG] calculator_fixed 被调用")
    print(f"[DEBUG] 参数: a={a}, b={b}, op='{op}'")
    print(f"[DEBUG] 参数类型: a={type(a)}, b={type(b)}, op={type(op)}")

    # 参数验证
    if not isinstance(a, (int, float)) or not isinstance(b, (int, float)):
        print("[DEBUG] 参数类型错误")
        return None

    # 操作符判断
    if op == "+":
        print("[DEBUG] 执行加法")
        result = a + b
    elif op == "-":
        print("[DEBUG] 执行减法")
        result = a - b
    elif op == "*":
        print("[DEBUG] 执行乘法")
        result = a * b
    elif op == "/":
        print("[DEBUG] 执行除法")
        # Bug 修复：检查除零
        if b == 0:
            print("[DEBUG] 除数为零，返回 None")
            return None
        result = a / b
    else:
        print(f"[DEBUG] 未知操作符: '{op}'")
        return None  # Bug 修复：返回 None

    print(f"[DEBUG] 计算结果: {result}")
    return result

# 测试
print("\n测试修复后的计算器：")
print(f"10 + 5 = {calculator_fixed(10, 5, '+')}")
print(f"10 - 5 = {calculator_fixed(10, 5, '-')}")
print(f"10 * 5 = {calculator_fixed(10, 5, '*')}")
print(f"10 / 5 = {calculator_fixed(10, 5, '/')}")
print(f"10 / 0 = {calculator_fixed(10, 0, '/')}")  # 除零测试
print(f"10 % 5 = {calculator_fixed(10, 5, '%')}")  # 未知操作符测试

# ============================================
# 项目2：使用 logging 记录数据处理过程
# ============================================

print("\n" + "=" * 60)
print("项目2：使用 logging 记录数据处理过程")
print("=" * 60)

class DataProcessor:
    """数据处理器：使用 logging 记录所有操作"""

    def __init__(self):
        logging.info("DataProcessor 初始化")

    def process_data(self, data):
        """处理数据"""
        logging.info("=" * 40)
        logging.info("开始处理数据")
        logging.debug(f"输入数据: {data}")
        logging.debug(f"数据类型: {type(data)}")

        # 检查数据是否为空
        if not data:
            logging.warning("数据为空，无法处理")
            return None

        # 检查数据类型
        if not isinstance(data, list):
            logging.error(f"数据类型错误，期望 list，实际 {type(data)}")
            return None

        logging.debug(f"数据长度: {len(data)}")

        # 过滤无效数据
        logging.debug("步骤1：过滤无效数据")
        valid_data = []
        for i, item in enumerate(data):
            logging.debug(f"  检查第 {i+1} 项: {item}")
            if isinstance(item, (int, float)) and item > 0:
                valid_data.append(item)
                logging.debug(f"    有效数据")
            else:
                logging.debug(f"    无效数据，已过滤")

        if not valid_data:
            logging.warning("没有有效数据")
            return None

        logging.info(f"有效数据数量: {len(valid_data)}/{len(data)}")

        # 计算统计信息
        logging.debug("步骤2：计算统计信息")
        result = {
            "count": len(valid_data),
            "sum": sum(valid_data),
            "average": sum(valid_data) / len(valid_data),
            "max": max(valid_data),
            "min": min(valid_data)
        }

        logging.info("数据处理完成")
        logging.debug(f"处理结果: {result}")
        logging.info("=" * 40)

        return result

# 测试
print("\n测试数据处理器：")
processor = DataProcessor()

# 测试1：正常数据
print("\n测试1：正常数据")
data1 = [10, 20, 30, 40, 50]
result1 = processor.process_data(data1)
print(f"结果: {result1}")

# 测试2：包含无效数据
print("\n测试2：包含无效数据")
data2 = [10, -5, 20, 0, 30, "abc", 40]
result2 = processor.process_data(data2)
print(f"结果: {result2}")

# 测试3：空数据
print("\n测试3：空数据")
data3 = []
result3 = processor.process_data(data3)
print(f"结果: {result3}")

# 测试4：错误类型
print("\n测试4：错误类型")
data4 = "not a list"
result4 = processor.process_data(data4)
print(f"结果: {result4}")

# ============================================
# 项目3：调试排序算法
# ============================================

print("\n" + "=" * 60)
print("项目3：调试排序算法")
print("=" * 60)

print("""
任务：下面的冒泡排序有 bug，使用 print 调试找出并修复

原始代码（有 bug）：
def bubble_sort_buggy(arr):
    n = len(arr)
    for i in range(n):
        for j in range(n - i - 1):
            if arr[j] < arr[j + 1]:  # Bug：应该是 >
                arr[j], arr[j + 1] = arr[j + 1], arr[j]
    return arr

问题：排序结果是降序，应该是升序
""")

# 带调试信息的版本
def bubble_sort_debug(arr):
    """冒泡排序（带调试信息）"""
    print("\n[DEBUG] bubble_sort_debug 开始")
    print(f"[DEBUG] 输入数组: {arr}")

    n = len(arr)
    print(f"[DEBUG] 数组长度: {n}")

    # 外层循环
    for i in range(n):
        print(f"\n[DEBUG] 外层循环 i={i}")
        swapped = False  # 优化：记录是否发生交换

        # 内层循环
        for j in range(n - i - 1):
            print(f"  [DEBUG] 内层循环 j={j}, 比较 arr[{j}]={arr[j]} 和 arr[{j+1}]={arr[j+1]}")

            # Bug 修复：改为 > 实现升序
            if arr[j] > arr[j + 1]:
                print(f"    [DEBUG] 交换 {arr[j]} 和 {arr[j+1]}")
                arr[j], arr[j + 1] = arr[j + 1], arr[j]
                swapped = True
            else:
                print(f"    [DEBUG] 不交换")

        print(f"  [DEBUG] 第 {i+1} 轮后: {arr}")

        # 优化：如果没有发生交换，说明已经有序
        if not swapped:
            print(f"  [DEBUG] 没有发生交换，数组已有序，提前结束")
            break

    print(f"\n[DEBUG] 排序完成: {arr}")
    return arr

# 测试
print("\n测试排序算法：")
test_arr = [64, 34, 25, 12, 22, 11, 90]
print(f"原始数组: {test_arr}")
sorted_arr = bubble_sort_debug(test_arr.copy())
print(f"排序结果: {sorted_arr}")

# ============================================
# 项目4：调试文件读取（使用 logging）
# ============================================

print("\n" + "=" * 60)
print("项目4：调试文件读取（使用 logging）")
print("=" * 60)

import json

class ConfigReader:
    """配置文件读取器：使用 logging 记录所有操作"""

    def __init__(self):
        logging.info("ConfigReader 初始化")

    def read_config(self, filename):
        """读取配置文件"""
        logging.info(f"开始读取配置文件: {filename}")

        try:
            # 尝试打开文件
            logging.debug(f"尝试打开文件: {filename}")
            with open(filename, "r", encoding="utf-8") as f:
                logging.debug("文件打开成功")

                # 尝试解析 JSON
                logging.debug("尝试解析 JSON")
                config = json.load(f)
                logging.debug(f"JSON 解析成功，内容: {config}")

            logging.info(f"配置文件读取成功: {filename}")
            return config

        except FileNotFoundError:
            logging.error(f"文件不存在: {filename}")
            return None

        except json.JSONDecodeError as e:
            logging.error(f"JSON 格式错误: {e}")
            return None

        except Exception as e:
            logging.error(f"未知错误: {e}")
            return None

# 测试
print("\n测试配置文件读取器：")
reader = ConfigReader()

# 测试1：读取不存在的文件
print("\n测试1：读取不存在的文件")
config1 = reader.read_config("not_exist.json")
print(f"结果: {config1}")

# 测试2：模拟读取配置（实际项目中会真正读取文件）
print("\n测试2：模拟读取配置")
print("（实际项目中会真正读取文件）")
logging.info("模拟配置读取成功")

# ============================================
# 项目5：性能调试和优化
# ============================================

print("\n" + "=" * 60)
print("项目5：性能调试和优化")
print("=" * 60)

import time

print("""
任务：下面的代码很慢，找出原因并优化

原始代码（效率低）：
def find_duplicates_slow(arr):
    duplicates = []
    for i in range(len(arr)):
        for j in range(i + 1, len(arr)):
            if arr[i] == arr[j] and arr[i] not in duplicates:
                duplicates.append(arr[i])
    return duplicates

问题：
1. 嵌套循环，时间复杂度 O(n²)
2. arr[i] not in duplicates 又是 O(n)
3. 总时间复杂度 O(n³)
""")

# 原始版本（慢）
def find_duplicates_slow(arr):
    """查找重复元素（慢版本）"""
    duplicates = []
    for i in range(len(arr)):
        for j in range(i + 1, len(arr)):
            if arr[i] == arr[j] and arr[i] not in duplicates:
                duplicates.append(arr[i])
    return duplicates

# 优化版本（快）
def find_duplicates_fast(arr):
    """查找重复元素（快版本）"""
    # 使用字典统计每个元素出现的次数
    count = {}
    for item in arr:
        count[item] = count.get(item, 0) + 1

    # 找出出现次数 > 1 的元素
    duplicates = [item for item, cnt in count.items() if cnt > 1]
    return duplicates

# 性能测试
print("\n性能测试：")

# 测试数据
test_data = list(range(100)) * 2  # [0,1,2,...,99,0,1,2,...,99]
print(f"测试数据长度: {len(test_data)}")

# 测试慢版本
print("\n测试慢版本：")
start = time.time()
result_slow = find_duplicates_slow(test_data)
end = time.time()
print(f"结果: {len(result_slow)} 个重复元素")
print(f"耗时: {(end - start)*1000:.2f} 毫秒")

# 测试快版本
print("\n测试快版本：")
start = time.time()
result_fast = find_duplicates_fast(test_data)
end = time.time()
print(f"结果: {len(result_fast)} 个重复元素")
print(f"耗时: {(end - start)*1000:.2f} 毫秒")

# 性能对比
print("\n性能对比：")
print("优化后的版本快了很多倍！")
print("原因：")
print("  - 慢版本：O(n³) 时间复杂度")
print("  - 快版本：O(n) 时间复杂度")
print("  - 使用字典（哈希表）提高查找效率")

# ============================================
# 项目6：综合调试项目 - 学生成绩分析系统
# ============================================

print("\n" + "=" * 60)
print("项目6：综合调试项目 - 学生成绩分析系统")
print("=" * 60)

class GradeAnalyzer:
    """学生成绩分析系统：综合运用调试技巧"""

    def __init__(self):
        logging.info("GradeAnalyzer 初始化")
        self.students = []

    def add_student(self, name, scores):
        """添加学生"""
        logging.debug(f"添加学生: name={name}, scores={scores}")

        # 参数验证
        if not name or not isinstance(name, str):
            logging.error(f"学生姓名无效: {name}")
            return False

        if not scores or not isinstance(scores, list):
            logging.error(f"成绩列表无效: {scores}")
            return False

        # 检查成绩是否有效
        for score in scores:
            if not isinstance(score, (int, float)) or score < 0 or score > 100:
                logging.error(f"成绩无效: {score}")
                return False

        # 添加学生
        student = {
            "name": name,
            "scores": scores,
            "average": sum(scores) / len(scores)
        }
        self.students.append(student)
        logging.info(f"学生添加成功: {name}")

        return True

    def analyze(self):
        """分析成绩"""
        logging.info("开始分析成绩")

        if not self.students:
            logging.warning("没有学生数据")
            return None

        logging.debug(f"学生数量: {len(self.students)}")

        # 计算班级平均分
        all_averages = [s["average"] for s in self.students]
        class_average = sum(all_averages) / len(all_averages)
        logging.debug(f"班级平均分: {class_average:.2f}")

        # 找出最高分和最低分
        best_student = max(self.students, key=lambda s: s["average"])
        worst_student = min(self.students, key=lambda s: s["average"])
        logging.debug(f"最高分学生: {best_student['name']}")
        logging.debug(f"最低分学生: {worst_student['name']}")

        # 统计各分数段人数
        grade_distribution = {
            "优秀(>=90)": 0,
            "良好(80-89)": 0,
            "中等(70-79)": 0,
            "及格(60-69)": 0,
            "不及格(<60)": 0
        }

        for student in self.students:
            avg = student["average"]
            if avg >= 90:
                grade_distribution["优秀(>=90)"] += 1
            elif avg >= 80:
                grade_distribution["良好(80-89)"] += 1
            elif avg >= 70:
                grade_distribution["中等(70-79)"] += 1
            elif avg >= 60:
                grade_distribution["及格(60-69)"] += 1
            else:
                grade_distribution["不及格(<60)"] += 1

        logging.debug(f"分数段分布: {grade_distribution}")

        result = {
            "class_average": class_average,
            "best_student": best_student,
            "worst_student": worst_student,
            "grade_distribution": grade_distribution
        }

        logging.info("成绩分析完成")
        return result

    def print_report(self):
        """打印分析报告"""
        logging.info("生成分析报告")

        result = self.analyze()
        if not result:
            print("没有数据可分析")
            return

        print("\n" + "=" * 60)
        print("学生成绩分析报告")
        print("=" * 60)

        print(f"\n班级平均分: {result['class_average']:.2f}")

        print(f"\n最高分学生: {result['best_student']['name']}")
        print(f"  平均分: {result['best_student']['average']:.2f}")
        print(f"  各科成绩: {result['best_student']['scores']}")

        print(f"\n最低分学生: {result['worst_student']['name']}")
        print(f"  平均分: {result['worst_student']['average']:.2f}")
        print(f"  各科成绩: {result['worst_student']['scores']}")

        print("\n分数段分布:")
        for grade, count in result['grade_distribution'].items():
            print(f"  {grade}: {count}人")

        print("=" * 60)

# 测试
print("\n测试学生成绩分析系统：")
analyzer = GradeAnalyzer()

# 添加学生
analyzer.add_student("张三", [85, 92, 78, 95, 88])
analyzer.add_student("李四", [92, 88, 95, 90, 93])
analyzer.add_student("王五", [78, 75, 82, 80, 79])
analyzer.add_student("赵六", [65, 70, 68, 72, 69])
analyzer.add_student("钱七", [55, 58, 52, 60, 56])

# 测试无效数据
analyzer.add_student("", [85, 92])  # 姓名无效
analyzer.add_student("孙八", [85, 120])  # 成绩无效

# 生成报告
analyzer.print_report()

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结：调试技巧的价值")
print("=" * 60)

print("""
通过今天的综合练习，你已经实际用到了：

1. print 调试技巧
   - 在关键位置打印变量值
   - 使用分隔线和标记
   - 观察程序执行流程
   - 定位 bug 位置

2. logging 模块
   - 配置日志系统
   - 使用不同日志级别
   - 记录程序运行过程
   - 分析日志信息

3. 常见 bug 排查
   - 语法错误：看错误信息
   - 逻辑错误：打印中间结果
   - 运行时错误：处理异常
   - 性能问题：时间测量

4. 调试思维
   - 重现问题
   - 定位问题
   - 理解原因
   - 修复验证

现在你对调试的理解已经从"知道有调试"进步到：
你开始理解"如何系统地调试程序"。
""")

# ============================================
# 附加练习题（不提供答案）
# ============================================

print("\n" + "=" * 60)
print("附加练习题（自己动手）")
print("=" * 60)

print("""
练习1：调试递归函数
  要求：
  - 下面的斐波那契数列有 bug
  - 使用 print 调试找出问题
  - 修复并优化性能

  代码：
  def fibonacci(n):
      if n <= 1:
          return n
      return fibonacci(n-1) + fibonacci(n-1)  # Bug：应该是 n-2

练习2：调试类的方法
  要求：
  - 创建一个 BankAccount 类
  - 实现存款、取款、查询余额功能
  - 使用 logging 记录所有操作
  - 处理余额不足等异常情况

练习3：调试数据处理管道
  要求：
  - 创建一个数据处理管道
  - 包含：读取 → 清洗 → 转换 → 分析 → 输出
  - 每个步骤使用 logging 记录
  - 处理各种异常情况

练习4：性能优化挑战
  要求：
  - 优化下面的代码（查找两个列表的交集）
  - 使用时间测量对比性能
  - 分析时间复杂度

  代码：
  def find_intersection_slow(list1, list2):
      result = []
      for item in list1:
          if item in list2 and item not in result:
              result.append(item)
      return result

  提示：使用集合（set）优化

练习5：综合调试项目
  要求：
  - 创建一个图书管理系统
  - 功能：添加、删除、查询、借阅、归还
  - 使用 logging 记录所有操作
  - 处理各种异常情况
  - 使用 print 调试复杂逻辑
""")

print("\n" + "=" * 60)
print("学习完成！")
print("=" * 60)
print("""
恭喜你完成了调试技巧的学习！

你已经掌握了：
✅ print 调试的实际应用
✅ logging 模块的专业使用
✅ 常见 bug 的排查方法
✅ 性能问题的定位和优化
✅ 系统的调试思维和方法

下一步：
- 完成附加练习题
- 在实际项目中应用调试技巧
- 培养良好的调试习惯
- 学习更高级的调试工具（如 pdb）

记住：
- 调试能力是程序员的核心竞争力
- 好的调试习惯能让你事半功倍
- 多练习，多思考，多总结
- 从错误中学习，不断进步
""")
