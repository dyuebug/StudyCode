# ============================================
# 成绩等级评定系统 - 综合项目
# 日期：2026-03-08（第5天）
# 难度：⭐⭐ 中级
# 综合运用：条件判断、集合、列表、字典
# ============================================

"""
【项目简介】

这是一个简单的成绩等级评定系统，综合运用了：
- 条件判断：判断等级、是否及格、学习建议
- 字典：保存等级说明和学习建议
- 集合：判断学生掌握的技能标签，演示去重思想
- 列表：保存多个学生成绩

功能：
1. 输入学生姓名和成绩
2. 自动判断成绩等级（A/B/C/D/F）
3. 判断是否及格
4. 给出学习建议
5. 支持查看多名学生的成绩汇总
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

print("""
【项目目标】

这个项目要解决什么问题？
- 输入一个学生成绩后，程序自动分析结果
- 不只是告诉你分数，还会告诉你等级、是否及格、学习建议

这个项目为什么重要？
- 因为它把“条件判断”真正用起来了
- 你会看到：程序如何根据不同分数走不同分支
- 同时也会复习前面学过的列表、字典、集合
""")

print("""
【函数卡片：append()】

语法：列表.append(元素)
参数：
  - 元素：要添加到列表末尾的数据
返回：无返回值
作用：把新的学生信息加入列表

示例：
```python
students.append(student_info)
```
""")

print("""
【函数卡片：get()】

语法：字典.get(键, 默认值)
参数：
  - 键：要查找的键
  - 默认值：找不到时返回的值（可选）
返回：对应的值或默认值
作用：安全地获取字典中的数据

示例：
```python
advice = advice_map.get(grade, '继续努力')
```
""")

# ============================================
# 第二部分：基础数据准备
# ============================================

print("\n" + "=" * 60)
print("第二部分：基础数据准备")
print("=" * 60)

# 等级说明字典
# 键：成绩等级
# 值：等级说明
grade_descriptions = {
    "A": "优秀",
    "B": "良好",
    "C": "中等",
    "D": "及格",
    "F": "不及格"
}

# 学习建议字典
# 根据不同等级给出不同建议
advice_map = {
    "A": "你的基础非常扎实，可以尝试挑战更难的题目。",
    "B": "你的表现不错，继续保持，并加强细节准确率。",
    "C": "基础还可以，但需要更多练习来提高熟练度。",
    "D": "虽然及格了，但基础不够稳，建议认真复习薄弱知识点。",
    "F": "当前基础较弱，建议从基础概念重新学起，并多做练习。"
}

# 学生成绩列表
# 每个学生信息会以字典形式保存到这里
students = []

# ============================================
# 第三部分：核心功能函数
# ============================================

# --------------------------------------------
# 3.1 判断成绩等级
# --------------------------------------------

def get_grade(score):
    """
    根据成绩返回等级

    参数：
        score (float): 学生成绩

    返回：
        str: 成绩等级（A/B/C/D/F）

    判断规则：
    - 90分及以上：A
    - 80-89分：B
    - 70-79分：C
    - 60-69分：D
    - 60分以下：F
    """
    # 注意顺序：高分条件必须写在前面
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

# --------------------------------------------
# 3.2 判断是否及格
# --------------------------------------------

def is_pass(score):
    """
    判断成绩是否及格

    参数：
        score (float): 学生成绩

    返回：
        bool: 及格返回True，否则返回False
    """
    return score >= 60

# --------------------------------------------
# 3.3 获取学习建议
# --------------------------------------------

def get_advice(grade):
    """
    根据等级返回学习建议

    参数：
        grade (str): 成绩等级

    返回：
        str: 对应的学习建议
    """
    # 使用 get() 而不是直接用 []
    # 这样即使等级不存在，也不会报错
    return advice_map.get(grade, "继续保持学习，稳步进步。")

# --------------------------------------------
# 3.4 显示单个学生信息
# --------------------------------------------

def display_student_result(student):
    """
    显示单个学生的成绩分析结果

    参数：
        student (dict): 学生信息字典
    """
    print("\n" + "-" * 60)
    print(f"姓名：{student['name']}")
    print(f"成绩：{student['score']} 分")
    print(f"等级：{student['grade']}（{student['grade_desc']}）")
    print(f"是否及格：{student['pass_status']}")
    print(f"学习建议：{student['advice']}")
    print(f"学习标签：{student['tags']}")
    print("-" * 60)

# --------------------------------------------
# 3.5 添加学生成绩
# --------------------------------------------

def add_student_score():
    """
    输入一个学生的信息，并进行成绩分析

    流程：
    1. 输入学生姓名
    2. 输入学生成绩
    3. 判断等级和及格状态
    4. 生成学习建议
    5. 保存到 students 列表
    """
    print("\n" + "=" * 60)
    print("添加学生成绩")
    print("=" * 60)

    # 输入姓名，并去掉首尾空格
    name = input("请输入学生姓名：").strip()

    # 如果用户什么都没输入，给出提示并结束本次操作
    if name == "":
        print("❌ 姓名不能为空")
        return

    # 循环输入成绩，直到输入合法为止
    while True:
        score_str = input("请输入学生成绩（0-100）：").strip()

        try:
            # 将字符串转换为浮点数
            score = float(score_str)

            # 判断分数范围是否合法
            if 0 <= score <= 100:
                break
            else:
                print("❌ 成绩必须在0到100之间，请重新输入")
        except ValueError:
            print("❌ 请输入有效数字")

    # 调用函数判断等级
    grade = get_grade(score)

    # 根据等级获取中文说明
    grade_desc = grade_descriptions.get(grade, "未知等级")

    # 判断是否及格
    passed = is_pass(score)
    pass_status = "及格" if passed else "不及格"

    # 获取学习建议
    advice = get_advice(grade)

    # 使用集合保存标签，演示“唯一性”和“自动去重”
    tags = set()

    # 根据分数添加标签
    if score >= 90:
        tags.add("高分")
        tags.add("基础扎实")
    elif score >= 80:
        tags.add("良好")
        tags.add("继续提升")
    elif score >= 70:
        tags.add("中等")
        tags.add("需要练习")
    elif score >= 60:
        tags.add("及格")
        tags.add("基础不稳")
    else:
        tags.add("不及格")
        tags.add("需要补基础")

    # 故意重复添加一次标签，演示集合不会重复
    tags.add("继续提升")
    tags.add("继续提升")

    # 创建学生信息字典
    student = {
        "name": name,
        "score": score,
        "grade": grade,
        "grade_desc": grade_desc,
        "pass_status": pass_status,
        "advice": advice,
        # sorted() 把集合转成列表并排序，方便稳定展示
        "tags": sorted(tags)
    }

    # 把学生信息加入列表
    students.append(student)

    print(f"\n✅ {name} 的成绩分析已完成！")
    display_student_result(student)

# --------------------------------------------
# 3.6 显示所有学生汇总
# --------------------------------------------

def show_all_students():
    """
    显示所有已录入学生的成绩汇总
    """
    print("\n" + "=" * 60)
    print("所有学生成绩汇总")
    print("=" * 60)

    if not students:
        print("❌ 还没有录入任何学生信息")
        return

    # 遍历列表，逐个显示学生信息
    for index, student in enumerate(students, start=1):
        print(f"\n第 {index} 位学生：")
        display_student_result(student)

    # 统计平均分
    total_score = 0
    for student in students:
        total_score += student["score"]

    average_score = total_score / len(students)
    print(f"\n班级平均分：{average_score:.2f} 分")

# --------------------------------------------
# 3.7 统计等级分布
# --------------------------------------------

def show_grade_statistics():
    """
    统计各等级人数
    """
    print("\n" + "=" * 60)
    print("成绩等级统计")
    print("=" * 60)

    if not students:
        print("❌ 还没有学生数据，无法统计")
        return

    grade_count = {"A": 0, "B": 0, "C": 0, "D": 0, "F": 0}

    # 遍历所有学生，统计各等级人数
    for student in students:
        grade_count[student["grade"]] += 1

    for grade, count in grade_count.items():
        print(f"等级 {grade}（{grade_descriptions[grade]}）：{count} 人")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
1. 错误：先判断 >= 60，再判断 >= 90

❌ 错误代码：
if score >= 60:
    grade = 'D'
elif score >= 90:
    grade = 'A'

原因：90分也满足 >= 60，会被前面拦住

✅ 正确做法：
先判断 >= 90，再逐步往下判断
""")

print("""
2. 错误：忘记把 input() 转成数字

❌ 错误代码：
score = input('请输入成绩：')
if score >= 60:
    print('及格')

原因：input() 返回字符串，不能直接和数字比较

✅ 正确做法：
score = float(input('请输入成绩：'))
""")

print("""
3. 错误：直接用字典[key] 获取未知键

❌ 错误代码：
advice = advice_map[grade]

原因：如果 grade 不存在，会报 KeyError

✅ 正确做法：
advice = advice_map.get(grade, '默认建议')
""")

print("""
4. 错误：以为集合会保留重复标签

❌ 错误理解：
“我 add 两次，集合里就会有两个一样的标签。”

原因：集合自动去重

✅ 正确理解：
集合适合保存“唯一标签”
""")

print("""
5. 错误：不验证输入范围

❌ 错误情况：
用户输入 -10 或 999，也直接当成成绩

原因：程序逻辑不严谨

✅ 正确做法：
判断成绩是否在 0 到 100 之间
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【函数1：get_grade(score)】

语法：get_grade(score)
参数：
  - score：学生成绩
返回：A/B/C/D/F
作用：根据成绩判断等级

────────────────────────────────────

【函数2：is_pass(score)】

语法：is_pass(score)
参数：
  - score：学生成绩
返回：True 或 False
作用：判断是否及格

────────────────────────────────────

【函数3：get_advice(grade)】

语法：get_advice(grade)
参数：
  - grade：成绩等级
返回：学习建议字符串
作用：根据等级给出建议

────────────────────────────────────

【函数4：display_student_result(student)】

语法：display_student_result(student)
参数：
  - student：学生信息字典
返回：无
作用：展示单个学生的成绩分析结果

────────────────────────────────────

【函数5：add_student_score()】

语法：add_student_score()
参数：无
返回：无
作用：输入并分析一个学生的成绩
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题")
print("=" * 60)

print("""
1. 修改等级规则练习
   要求：
   - 把A等级改为95分及以上
   - 观察程序逻辑需要改哪里
   提示：修改 get_grade() 函数

2. 添加评语练习
   要求：
   - 为每个等级增加一句评语
   - 例如：A → “表现非常出色”
   提示：可以新增一个字典保存评语

3. 添加优秀判断练习
   要求：
   - 如果分数大于等于85，额外显示“优秀学员”
   提示：在 display_student_result() 或 add_student_score() 中添加条件判断

4. 多人录入练习
   要求：
   - 连续添加3名学生信息
   - 然后查看汇总和等级统计
   提示：运行主菜单，多操作几次

5. 挑战练习：增加科目分类
   要求：
   - 除了总分，还输入课程名
   - 显示“Python课程成绩：88分”这样的结果
   提示：可以把 student 字典扩展得更丰富
""")

# ============================================
# 第七部分：主程序入口
# ============================================


def main():
    """
    主菜单函数

    作用：
    - 显示菜单
    - 根据用户选择调用不同功能
    - 让整个程序可以重复操作
    """
    while True:
        print("\n" + "=" * 60)
        print("成绩等级评定系统")
        print("=" * 60)
        print("1. 添加学生成绩")
        print("2. 显示所有学生成绩")
        print("3. 查看等级统计")
        print("4. 退出系统")

        choice = input("请输入选项（1-4）：").strip()

        if choice == "1":
            add_student_score()
        elif choice == "2":
            show_all_students()
        elif choice == "3":
            show_grade_statistics()
        elif choice == "4":
            print("\n感谢使用成绩等级评定系统，继续修炼，必有大成！")
            break
        else:
            print("❌ 选项无效，请输入1到4之间的数字")


print("\n" + "=" * 60)
print("🎉 第5天项目材料已创建完成！")
print("建议：运行本文件，录入几名学生，观察等级判断和统计结果")
print("=" * 60)

# 当文件被直接运行时，启动主程序
if __name__ == "__main__":
    main()
