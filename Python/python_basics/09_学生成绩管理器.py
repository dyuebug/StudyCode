# ============================================
# 学生成绩管理器 - 综合项目
# 日期：2026-02-26（第3天）
# 难度：⭐⭐ 中级
# ============================================

"""
【项目简介】学生成绩管理器

功能：
- 预置学生成绩数据
- 查看所有学生成绩
- 计算平均分、最高分、最低分
- 成绩排序（升序/降序）
- 添加新学生成绩
- 查找特定学生
- 成绩等级统计

涉及知识点：
- 列表的创建和遍历（for循环）
- 列表方法（append、sort、copy）
- 内置函数（len、max、min、sum、sorted）
- 嵌套列表（二维列表）
- 字符串格式化（f-string）
- 列表推导式
- in 成员判断
"""

print("=" * 60)
print("       📊 学生成绩管理器")
print("=" * 60)

# ============================================
# 功能1：初始化学生数据
# ============================================

print("\n功能1：初始化学生数据")
print("-" * 60)

# 用嵌套列表存储学生数据
# 每个内层列表：[姓名, 语文, 数学, 英语]
students = [
    ["小明", 85, 92, 78],
    ["小红", 95, 88, 96],
    ["小刚", 72, 65, 80],
    ["小丽", 90, 95, 88],
    ["小强", 68, 75, 62],
    ["小芳", 88, 82, 91],
]

# 科目名称列表（方便后面使用）
subjects = ["语文", "数学", "英语"]

print(f"已加载 {len(students)} 名学生的成绩数据")

# ============================================
# 功能2：查看所有学生成绩
# ============================================

print("\n功能2：查看所有学生成绩")
print("-" * 60)

# 打印表头
# ljust(6) 让字符串左对齐，占6个字符宽度，保证对齐
print(f"  {'姓名':<6} {'语文':>4} {'数学':>4} {'英语':>4} {'总分':>4} {'平均':>6}")
print(f"  {'-' * 40}")

# 遍历每个学生，计算总分和平均分
for student in students:
    name = student[0]          # 姓名
    chinese = student[1]       # 语文
    math = student[2]          # 数学
    english = student[3]       # 英语
    total = chinese + math + english          # 总分
    avg = total / len(subjects)               # 平均分
    # :.1f 表示保留1位小数
    print(f"  {name:<6} {chinese:>4} {math:>4} {english:>4} {total:>4} {avg:>6.1f}")

# ============================================
# 功能3：成绩统计分析
# ============================================

print("\n功能3：成绩统计分析")
print("-" * 60)

# 提取所有学生的各科成绩（用列表推导式）
# student[1] 是语文成绩，student[2] 是数学，student[3] 是英语
chinese_scores = [student[1] for student in students]
math_scores = [student[2] for student in students]
english_scores = [student[3] for student in students]

# 计算每科的统计数据
for i, subject in enumerate(subjects):
    # 根据索引获取对应科目的成绩列表
    if i == 0:
        scores = chinese_scores
    elif i == 1:
        scores = math_scores
    else:
        scores = english_scores

    print(f"\n  【{subject}】")
    print(f"    最高分：{max(scores)}")
    print(f"    最低分：{min(scores)}")
    print(f"    平均分：{sum(scores) / len(scores):.1f}")

# 计算每个学生的总分，找出总分最高和最低的学生
total_scores = []  # 存储每个学生的 [姓名, 总分]
for student in students:
    name = student[0]
    total = student[1] + student[2] + student[3]
    total_scores.append([name, total])

# 按总分排序（用sorted + key参数）
# key=lambda x: x[1] 表示按每个元素的第2个值（总分）排序
sorted_by_total = sorted(total_scores, key=lambda x: x[1], reverse=True)

print(f"\n  【总分排名】")
for rank, item in enumerate(sorted_by_total):
    # rank从0开始，+1变成从1开始
    print(f"    第{rank + 1}名：{item[0]}（总分：{item[1]}）")

# ============================================
# 功能4：成绩等级统计
# ============================================

print("\n功能4：成绩等级统计")
print("-" * 60)

print("""
  等级标准：
  A（优秀）：90分及以上
  B（良好）：80-89分
  C（及格）：60-79分
  D（不及格）：60分以下
""")

# 统计所有成绩的等级分布
# 先把所有成绩放到一个列表里
all_scores = []
for student in students:
    # student[1:] 取索引1到末尾，即三科成绩
    all_scores.extend(student[1:])

# 用列表推导式统计各等级人次
count_a = len([s for s in all_scores if s >= 90])
count_b = len([s for s in all_scores if 80 <= s < 90])
count_c = len([s for s in all_scores if 60 <= s < 80])
count_d = len([s for s in all_scores if s < 60])

total_count = len(all_scores)
print(f"  总成绩数：{total_count}个")
print(f"  A（优秀）：{count_a}个（{count_a/total_count*100:.1f}%）")
print(f"  B（良好）：{count_b}个（{count_b/total_count*100:.1f}%）")
print(f"  C（及格）：{count_c}个（{count_c/total_count*100:.1f}%）")
print(f"  D（不及格）：{count_d}个（{count_d/total_count*100:.1f}%）")

# ============================================
# 功能5：添加新学生
# ============================================

print("\n功能5：添加新学生")
print("-" * 60)

# 模拟添加新学生（不用input，直接用变量演示）
new_student = ["小华", 93, 87, 91]
students.append(new_student)
print(f"  已添加：{new_student[0]}（语文{new_student[1]} "
      f"数学{new_student[2]} 英语{new_student[3]}）")
print(f"  当前学生总数：{len(students)}名")

# ============================================
# 功能6：查找特定学生
# ============================================

print("\n功能6：查找特定学生")
print("-" * 60)

# 查找函数：根据姓名查找学生
search_name = "小红"
found = False  # 标记是否找到

for student in students:
    if student[0] == search_name:
        found = True
        total = student[1] + student[2] + student[3]
        avg = total / 3
        print(f"  找到学生：{student[0]}")
        print(f"    语文：{student[1]}")
        print(f"    数学：{student[2]}")
        print(f"    英语：{student[3]}")
        print(f"    总分：{total}")
        print(f"    平均分：{avg:.1f}")
        break  # 找到了就停止循环

if not found:
    print(f"  未找到学生：{search_name}")

# 用列表推导式查找（更简洁的方式）
search_name2 = "小刚"
results = [s for s in students if s[0] == search_name2]
if results:
    s = results[0]
    print(f"\n  列表推导式查找：{s[0]} - 语文{s[1]} 数学{s[2]} 英语{s[3]}")
else:
    print(f"\n  未找到：{search_name2}")

# ============================================
# 功能7：成绩排序展示
# ============================================

print("\n功能7：成绩排序展示")
print("-" * 60)

# 按数学成绩降序排列
# 使用sorted()不修改原列表
# key=lambda x: x[2] 表示按每个学生的数学成绩（索引2）排序
math_rank = sorted(students, key=lambda x: x[2], reverse=True)

print("  【数学成绩排名】")
for rank, student in enumerate(math_rank):
    print(f"    第{rank + 1}名：{student[0]} - {student[2]}分")

# 按平均分排序
avg_rank = sorted(students,
                  key=lambda x: (x[1] + x[2] + x[3]) / 3,
                  reverse=True)

print("\n  【平均分排名】")
for rank, student in enumerate(avg_rank):
    avg = (student[1] + student[2] + student[3]) / 3
    print(f"    第{rank + 1}名：{student[0]} - 平均{avg:.1f}分")

# ============================================
# 项目总结
# ============================================

print("\n" + "=" * 60)
print("项目总结：本项目用到的知识点")
print("=" * 60)

print("""
  ✅ 嵌套列表：用二维列表存储学生数据
  ✅ for循环遍历：逐个处理每个学生
  ✅ 列表推导式：快速提取和过滤数据
  ✅ 内置函数：len、max、min、sum
  ✅ sorted()排序：配合key参数自定义排序规则
  ✅ enumerate()：获取排名编号
  ✅ append()：添加新学生
  ✅ extend()：展开成绩列表
  ✅ in 成员判断：查找学生
  ✅ f-string格式化：对齐输出表格
  ✅ lambda表达式：简单的匿名函数（初次接触）
""")

print("""
【新概念预告：lambda表达式】

本项目中用到了 lambda，这是一个"匿名函数"的简写：

  key=lambda x: x[2]

等价于：
  def get_math(x):
      return x[2]
  key=get_math

lambda 就是一种写简单函数的快捷方式，
不需要def和函数名，一行搞定。
后面学函数的时候会详细讲解。
""")

print("=" * 60)
print("  🎉 学生成绩管理器项目完成！")
print("  综合运用了列表基础和进阶的所有知识")
print("=" * 60)
