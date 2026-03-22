# ============================================
# 函数综合练习 - 超详细注释版
# 日期：2026-03-20（第7天）
# 难度：⭐⭐ 中级
# ============================================

"""
【综合练习目标】

通过实际项目体会函数的价值：
1. 代码复用：同一逻辑只写一次
2. 模块化：每个函数只做一件事
3. 可读性：函数名说明意图，代码更清晰
4. 易维护：修改一处，全局生效

本文件包含：
- 计算器（四则运算函数）
- 学生成绩分析（用函数重构第3天的代码）
- 简易密码生成器
"""

# ============================================
# 项目一：多功能计算器
# ============================================

print("=" * 60)
print("项目一：多功能计算器")
print("=" * 60)

# --------------------------------------------
# 定义四个基础运算函数
# --------------------------------------------

# 加法函数
# 参数：a（数字）、b（数字）
# 返回：两数之和
def add(a, b):
    return a + b

# 减法函数
# 参数：a（被减数）、b（减数）
# 返回：差
def subtract(a, b):
    return a - b

# 乘法函数
# 参数：a（数字）、b（数字）
# 返回：积
def multiply(a, b):
    return a * b

# 除法函数（带错误处理）
# 参数：a（被除数）、b（除数）
# 返回：商，或错误提示字符串
def divide(a, b):
    # 特殊情况：除数为0，数学上无意义
    if b == 0:
        return "错误：除数不能为0"
    return a / b

# 定义计算并打印结果的辅助函数
# 参数：a、b（操作数），op（运算符字符串），result（结果）
def print_result(a, op, b, result):
    print(f"  {a} {op} {b} = {result}")

# 调用计算器函数
print("基础运算：")
print_result(10, "+", 5, add(10, 5))
print_result(10, "-", 5, subtract(10, 5))
print_result(10, "×", 5, multiply(10, 5))
print_result(10, "÷", 5, divide(10, 5))
print_result(10, "÷", 0, divide(10, 0))   # 测试除以0的情况

# --------------------------------------------
# 更高级：通用计算函数
# --------------------------------------------

print("\n通用计算函数：")

# 使用默认参数和关键字参数
# operator 有默认值 "+"
def calculate(a, b, operator="+"):
    """
    通用计算函数
    参数：
        a: 第一个数
        b: 第二个数
        operator: 运算符，默认为 "+"
    返回：计算结果
    """
    if operator == "+":
        return add(a, b)        # 调用之前定义的函数
    elif operator == "-":
        return subtract(a, b)
    elif operator == "*":
        return multiply(a, b)
    elif operator == "/":
        return divide(a, b)
    else:
        return f"不支持的运算符：{operator}"

# 使用默认参数（不传operator，默认加法）
result = calculate(3, 5)
print(f"  calculate(3, 5) = {result}")

# 使用关键字参数指定运算符
result = calculate(10, 3, operator="*")
print(f"  calculate(10, 3, operator='*') = {result}")

# ============================================
# 项目二：学生成绩分析系统（函数版）
# ============================================

print("\n" + "=" * 60)
print("项目二：学生成绩分析系统（函数版）")
print("=" * 60)

print("""
【对比】
- 第3天：直接写代码，逻辑混在一起
- 今天：用函数封装，每个功能独立
- 好处：代码更清晰，可以复用，易于修改
""")

# 测试数据：学生成绩列表
# 每个元素是字典：{"name": 姓名, "score": 分数}
students = [
    {"name": "张三", "score": 85},
    {"name": "李四", "score": 92},
    {"name": "王五", "score": 67},
    {"name": "赵六", "score": 78},
    {"name": "陈七", "score": 55},
    {"name": "周八", "score": 96},
]

# --- 函数1：判断成绩等级 ---
# 参数：score（分数，整数）
# 返回：等级字符串（"A"/"B"/"C"/"D"/"F"）
def get_grade(score):
    if score >= 90:
        return "A"
    elif score >= 80:
        return "B"
    elif score >= 70:
        return "C"
    elif score >= 60:
        return "D"
    else:
        return "F"

# --- 函数2：判断是否及格 ---
# 参数：score（分数）
# 返回：True（及格）或 False（不及格）
def is_passing(score):
    return score >= 60  # 直接返回布尔表达式

# --- 函数3：计算班级统计信息 ---
# 参数：students（学生列表）
# 返回：包含统计信息的字典
def calc_stats(students):
    # 提取所有分数
    # 列表推导式：从每个学生字典中取出 score
    scores = [s["score"] for s in students]

    # 计算统计数据
    total = sum(scores)          # 总分
    count = len(scores)          # 人数
    average = total / count      # 平均分
    highest = max(scores)        # 最高分
    lowest = min(scores)         # 最低分

    # 统计及格人数
    # 用 for 循环统计 score >= 60 的人数
    passing_count = 0
    for s in students:
        if is_passing(s["score"]):  # 调用 is_passing 函数
            passing_count += 1

    # 返回字典（多个统计值打包）
    return {
        "total": total,
        "count": count,
        "average": average,
        "highest": highest,
        "lowest": lowest,
        "passing_count": passing_count,
        "passing_rate": passing_count / count * 100  # 及格率百分比
    }

# --- 函数4：打印单个学生信息 ---
# 参数：student（学生字典）
# 返回：无（只打印）
def print_student(student):
    name = student["name"]
    score = student["score"]
    grade = get_grade(score)      # 调用 get_grade 函数
    status = "及格" if is_passing(score) else "不及格"  # 调用 is_passing
    print(f"  {name}：{score}分，等级{grade}，{status}")

# --- 函数5：打印班级报告 ---
# 参数：students（学生列表）
# 返回：无（只打印）
def print_report(students):
    print("\n【学生成绩列表】")
    # 遍历每个学生，调用 print_student 函数
    for student in students:
        print_student(student)

    # 调用 calc_stats 获取统计信息
    stats = calc_stats(students)

    print("\n【班级统计】")
    print(f"  总人数：{stats['count']}人")
    print(f"  平均分：{stats['average']:.1f}分")
    print(f"  最高分：{stats['highest']}分")
    print(f"  最低分：{stats['lowest']}分")
    print(f"  及格人数：{stats['passing_count']}人")
    print(f"  及格率：{stats['passing_rate']:.1f}%")

# 调用入口函数，生成完整报告
print_report(students)

# ============================================
# 项目三：简易密码强度检测器
# ============================================

print("\n" + "=" * 60)
print("项目三：简易密码强度检测器")
print("=" * 60)

# --- 检测函数：各自负责一个条件 ---

# 检测长度是否足够
# 参数：password（字符串）
# 返回：True/False
def has_min_length(password, min_len=8):
    return len(password) >= min_len

# 检测是否含有大写字母
def has_uppercase(password):
    # any() 函数：只要有一个满足条件就返回True
    # c.isupper()：判断字符是否为大写字母
    return any(c.isupper() for c in password)

# 检测是否含有数字
def has_digit(password):
    # c.isdigit()：判断字符是否为数字
    return any(c.isdigit() for c in password)

# 检测是否含有特殊字符
def has_special_char(password):
    # 定义特殊字符集合
    special_chars = set("!@#$%^&*()_+-=[]{}|;':,./<>?")
    # 检查密码中是否有字符属于特殊字符集合
    return any(c in special_chars for c in password)

# --- 综合评分函数 ---
# 参数：password（要检测的密码）
# 返回：包含评分和建议的字典
def check_password_strength(password):
    # 逐项检测，收集结果
    checks = {
        "长度≥8位": has_min_length(password),
        "含大写字母": has_uppercase(password),
        "含数字": has_digit(password),
        "含特殊字符": has_special_char(password),
    }

    # 计算通过的检测项数量
    passed = sum(1 for v in checks.values() if v)  # True计为1，False计为0

    # 根据通过数量判断强度
    if passed == 4:
        strength = "强 💪"
    elif passed >= 2:
        strength = "中 ⚠️"
    else:
        strength = "弱 ❌"

    return {
        "checks": checks,
        "passed": passed,
        "strength": strength
    }

# --- 打印检测报告 ---
def print_password_report(password):
    print(f"\n密码：{'*' * len(password)}（长度{len(password)}位）")
    result = check_password_strength(password)

    print("检测结果：")
    for check_name, passed in result["checks"].items():
        status = "✅" if passed else "❌"
        print(f"  {status} {check_name}")

    print(f"综合强度：{result['strength']}（{result['passed']}/4项通过）")

# 测试几个密码
test_passwords = [
    "123456",           # 弱密码
    "password",         # 弱密码
    "Password1",        # 中等密码
    "P@ssw0rd!",        # 强密码
]

for pwd in test_passwords:
    print_password_report(pwd)

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结：函数的价值")
print("=" * 60)

print("""
通过今天的练习，体会到函数的三大价值：

1. 代码复用
   - get_grade() 被 print_student() 调用
   - is_passing() 被 print_student() 和 calc_stats() 调用
   - 写一次，多处使用

2. 模块化
   - 每个函数只做一件事（单一职责）
   - has_min_length、has_uppercase、has_digit 各自独立
   - 修改某个规则，只改对应函数即可

3. 可读性
   - print_report(students) 一眼看出：打印学生报告
   - is_passing(score) 一眼看出：判断是否及格
   - 代码像说明文，易于理解

下一步：学习模块和文件操作，让函数可以跨文件复用！
""")
