# ============================================
# 文件操作综合练习 - 学生成绩数据持久化
# 日期：2026-03-20（第8天）
# 难度：⭐⭐ 中级
# ============================================

"""
【综合练习目标】

通过「学生成绩管理系统」体会文件操作的实际价值：
- 之前的成绩系统：程序关闭后数据全丢失
- 今天的升级版：数据保存到文件，永久保留

功能：
1. 写入成绩数据到 CSV 格式文件
2. 从文件读取成绩数据
3. 追加新学生成绩
4. 生成成绩报告并保存
"""

import os
from datetime import datetime

# 数据文件路径
SCORES_FILE = "student_scores.txt"
REPORT_FILE = "score_report.txt"

# ============================================
# 功能1：保存成绩到文件
# ============================================

print("=" * 60)
print("功能1：保存成绩到文件")
print("=" * 60)

# 初始成绩数据
initial_students = [
    {"name": "张三", "score": 85},
    {"name": "李四", "score": 92},
    {"name": "王五", "score": 67},
    {"name": "赵六", "score": 78},
    {"name": "陈七", "score": 55},
]

def save_scores(students, filename):
    """
    保存学生成绩到文件
    参数：
        students: 学生列表，每个元素是 {name, score} 字典
        filename: 保存的文件名
    格式：每行 "姓名,分数"（CSV格式）
    """
    # 'w' 模式：覆盖写入（初始化文件）
    with open(filename, 'w', encoding='utf-8') as f:
        # 写入表头
        f.write("姓名,分数\n")
        # 遍历学生列表，逐行写入
        for student in students:
            # 格式：姓名,分数\n
            line = f"{student['name']},{student['score']}\n"
            f.write(line)
    print(f"已保存 {len(students)} 条成绩到 {filename}")

# 保存初始数据
save_scores(initial_students, SCORES_FILE)

# ============================================
# 功能2：从文件读取成绩
# ============================================

print("\n" + "=" * 60)
print("功能2：从文件读取成绩")
print("=" * 60)

def load_scores(filename):
    """
    从文件读取学生成绩
    参数：filename - 文件名
    返回：学生列表 [{name, score}, ...]
    """
    # 先检查文件是否存在
    if not os.path.exists(filename):
        print(f"文件 {filename} 不存在")
        return []

    students = []
    with open(filename, 'r', encoding='utf-8') as f:
        # 逐行读取
        for i, line in enumerate(f):
            line = line.strip()  # 去掉末尾的 \n

            # 跳过表头（第0行）和空行
            if i == 0 or not line:
                continue

            # 用逗号分割："张三,85" → ["张三", "85"]
            parts = line.split(",")
            if len(parts) == 2:  # 确保格式正确
                name = parts[0]           # 姓名
                score = int(parts[1])     # 分数（转为整数）
                students.append({"name": name, "score": score})

    print(f"从 {filename} 读取了 {len(students)} 条成绩")
    return students

# 读取成绩
students = load_scores(SCORES_FILE)
print("读取结果：")
for s in students:
    print(f"  {s['name']}：{s['score']}分")

# ============================================
# 功能3：追加新学生成绩
# ============================================

print("\n" + "=" * 60)
print("功能3：追加新学生成绩")
print("=" * 60)

def append_score(name, score, filename):
    """
    追加一个学生的成绩到文件
    参数：
        name: 学生姓名
        score: 分数
        filename: 文件名
    """
    # 'a' 模式：追加，不覆盖原有内容
    with open(filename, 'a', encoding='utf-8') as f:
        f.write(f"{name},{score}\n")
    print(f"已追加：{name} - {score}分")

# 追加新学生
new_students = [
    ("周八", 96),
    ("吴九", 73),
]

for name, score in new_students:
    append_score(name, score, SCORES_FILE)

# 重新读取，验证追加成功
print("\n追加后重新读取：")
all_students = load_scores(SCORES_FILE)
for s in all_students:
    print(f"  {s['name']}：{s['score']}分")

# ============================================
# 功能4：生成并保存成绩报告
# ============================================

print("\n" + "=" * 60)
print("功能4：生成并保存成绩报告")
print("=" * 60)

def get_grade(score):
    """根据分数返回等级"""
    if score >= 90: return "A"
    elif score >= 80: return "B"
    elif score >= 70: return "C"
    elif score >= 60: return "D"
    else: return "F"

def generate_report(students, report_file):
    """
    生成成绩报告并保存到文件
    参数：
        students: 学生列表
        report_file: 报告文件名
    """
    # 计算统计数据
    scores = [s["score"] for s in students]
    avg = sum(scores) / len(scores)
    highest = max(scores)
    lowest = min(scores)
    passing = sum(1 for s in scores if s >= 60)

    # 生成报告时间
    now = datetime.now().strftime("%Y年%m月%d日 %H:%M:%S")

    # 构建报告内容（列表，每项是一行）
    report_lines = [
        "=" * 50 + "\n",
        f"学生成绩报告\n",
        f"生成时间：{now}\n",
        "=" * 50 + "\n",
        "\n",
        "【学生成绩明细】\n",
        "-" * 30 + "\n",
    ]

    # 按分数从高到低排序
    sorted_students = sorted(students, key=lambda s: s["score"], reverse=True)
    for i, s in enumerate(sorted_students, 1):
        grade = get_grade(s["score"])
        status = "及格" if s["score"] >= 60 else "不及格"
        report_lines.append(f"  {i}. {s['name']}：{s['score']}分  等级{grade}  {status}\n")

    report_lines += [
        "\n",
        "【班级统计】\n",
        "-" * 30 + "\n",
        f"  总人数：{len(students)}人\n",
        f"  平均分：{avg:.1f}分\n",
        f"  最高分：{highest}分\n",
        f"  最低分：{lowest}分\n",
        f"  及格人数：{passing}人\n",
        f"  及格率：{passing/len(students)*100:.1f}%\n",
        "=" * 50 + "\n",
    ]

    # 写入报告文件
    with open(report_file, 'w', encoding='utf-8') as f:
        f.writelines(report_lines)  # writelines 写入列表

    print(f"报告已保存到：{report_file}")
    return report_lines

# 生成报告
report = generate_report(all_students, REPORT_FILE)

# 在控制台显示报告
print("\n报告内容预览：")
for line in report:
    print(line, end="")  # end="" 避免双重换行

# ============================================
# 功能5：读取并显示已保存的报告
# ============================================

print("\n" + "=" * 60)
print("功能5：验证 - 读取已保存的报告")
print("=" * 60)

# 验证文件确实写入成功
if os.path.exists(REPORT_FILE):
    file_size = os.path.getsize(REPORT_FILE)
    print(f"报告文件大小：{file_size} 字节")
    print("文件读取验证成功！")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结：文件操作的价值")
print("=" * 60)

print("""
通过今天的练习，学会了：

1. 数据持久化
   - 用 'w' 模式保存初始数据
   - 用 'a' 模式追加新数据
   - 程序关闭后数据不丢失

2. 数据读取
   - 用 readlines() + split() 解析 CSV 格式
   - 用 int() 把字符串转为数字
   - 用 strip() 去掉多余空白

3. 报告生成
   - 用 writelines() 批量写入
   - 用 datetime 记录生成时间
   - 格式化输出，方便阅读

下一步：学习面向对象编程（类和对象）！
""")

# 清理测试文件
for f in [SCORES_FILE, REPORT_FILE]:
    if os.path.exists(f):
        os.remove(f)
print("（测试文件已清理）")
