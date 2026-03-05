# ============================================
# 学生信息管理系统 - 综合项目
# 日期：2026-03-05（第4天）
# 难度：⭐⭐ 中级
# 综合运用：字典、列表、元组
# ============================================

"""
【项目简介】

这是一个简单的学生信息管理系统，综合运用了：
- 字典：存储学生信息（键值对）
- 列表：存储多个学生
- 元组：存储固定的学生信息

功能：
1. 添加学生信息
2. 查询学生信息
3. 修改学生信息
4. 删除学生信息
5. 显示所有学生信息
6. 统计学生成绩
"""

# ============================================
# 全局变量：学生数据库
# ============================================

# 使用列表存储多个学生字典
# 每个学生是一个字典：{"id": 学号, "name": 姓名, "age": 年龄, "scores": 成绩元组}
students = []

# 学号计数器（自动生成学号）
student_id_counter = 1

# ============================================
# 功能1：添加学生信息
# ============================================

def add_student():
    """
    添加学生信息

    流程：
    1. 输入学生姓名、年龄
    2. 输入三门课程成绩（语文、数学、英语）
    3. 自动生成学号
    4. 将学生信息存储为字典
    5. 添加到学生列表
    """
    global student_id_counter  # 使用全局变量

    print("\n" + "=" * 60)
    print("添加学生信息")
    print("=" * 60)

    # 输入学生基本信息
    name = input("请输入学生姓名：").strip()

    # 输入年龄（需要验证是否为数字）
    while True:
        age_str = input("请输入学生年龄：").strip()
        if age_str.isdigit():  # 判断是否为数字
            age = int(age_str)
            if 1 <= age <= 150:  # 年龄范围验证
                break
            else:
                print("❌ 年龄必须在1-150之间，请重新输入")
        else:
            print("❌ 年龄必须是数字，请重新输入")

    # 输入三门课程成绩
    print("\n请输入三门课程成绩（0-100分）：")
    scores = []  # 临时列表存储成绩

    subjects = ["语文", "数学", "英语"]  # 科目列表
    for subject in subjects:
        while True:
            score_str = input(f"  {subject}成绩：").strip()
            # 验证成绩是否为数字且在0-100之间
            try:
                score = float(score_str)
                if 0 <= score <= 100:
                    scores.append(score)
                    break
                else:
                    print("    ❌ 成绩必须在0-100之间，请重新输入")
            except ValueError:
                print("    ❌ 成绩必须是数字，请重新输入")

    # 将成绩列表转换为元组（不可变）
    scores_tuple = tuple(scores)

    # 创建学生字典
    student = {
        "id": student_id_counter,      # 学号
        "name": name,                   # 姓名
        "age": age,                     # 年龄
        "scores": scores_tuple          # 成绩（元组）
    }

    # 添加到学生列表
    students.append(student)

    # 学号自增
    student_id_counter += 1

    print(f"\n✅ 学生信息添加成功！学号：{student['id']}")

# ============================================
# 功能2：查询学生信息
# ============================================

def query_student():
    """
    查询学生信息

    流程：
    1. 输入学号或姓名
    2. 在学生列表中查找
    3. 显示学生详细信息
    """
    print("\n" + "=" * 60)
    print("查询学生信息")
    print("=" * 60)

    if not students:
        print("❌ 暂无学生信息")
        return

    # 选择查询方式
    print("请选择查询方式：")
    print("1. 按学号查询")
    print("2. 按姓名查询")

    choice = input("请输入选项（1或2）：").strip()

    if choice == "1":
        # 按学号查询
        id_str = input("请输入学号：").strip()
        if not id_str.isdigit():
            print("❌ 学号必须是数字")
            return

        student_id = int(id_str)

        # 在学生列表中查找
        found = False
        for student in students:
            if student["id"] == student_id:
                display_student_info(student)
                found = True
                break

        if not found:
            print(f"❌ 未找到学号为 {student_id} 的学生")

    elif choice == "2":
        # 按姓名查询
        name = input("请输入姓名：").strip()

        # 在学生列表中查找（可能有多个同名学生）
        found_students = []
        for student in students:
            if student["name"] == name:
                found_students.append(student)

        if found_students:
            print(f"\n找到 {len(found_students)} 个学生：")
            for student in found_students:
                display_student_info(student)
        else:
            print(f"❌ 未找到姓名为 {name} 的学生")

    else:
        print("❌ 无效的选项")

def display_student_info(student):
    """
    显示单个学生的详细信息

    参数：
        student (dict): 学生字典
    """
    print("\n" + "-" * 60)
    print(f"学号：{student['id']}")
    print(f"姓名：{student['name']}")
    print(f"年龄：{student['age']}")

    # 解包成绩元组
    chinese, math, english = student["scores"]
    print(f"成绩：")
    print(f"  语文：{chinese}分")
    print(f"  数学：{math}分")
    print(f"  英语：{english}分")

    # 计算平均分
    average = sum(student["scores"]) / len(student["scores"])
    print(f"  平均分：{average:.2f}分")
    print("-" * 60)

# ============================================
# 功能3：修改学生信息
# ============================================

def modify_student():
    """
    修改学生信息

    流程：
    1. 输入学号
    2. 查找学生
    3. 选择要修改的项目
    4. 输入新值
    5. 更新学生信息
    """
    print("\n" + "=" * 60)
    print("修改学生信息")
    print("=" * 60)

    if not students:
        print("❌ 暂无学生信息")
        return

    # 输入学号
    id_str = input("请输入要修改的学生学号：").strip()
    if not id_str.isdigit():
        print("❌ 学号必须是数字")
        return

    student_id = int(id_str)

    # 查找学生
    student = None
    for s in students:
        if s["id"] == student_id:
            student = s
            break

    if not student:
        print(f"❌ 未找到学号为 {student_id} 的学生")
        return

    # 显示当前信息
    print("\n当前学生信息：")
    display_student_info(student)

    # 选择要修改的项目
    print("\n请选择要修改的项目：")
    print("1. 姓名")
    print("2. 年龄")
    print("3. 成绩")

    choice = input("请输入选项（1-3）：").strip()

    if choice == "1":
        # 修改姓名
        new_name = input("请输入新姓名：").strip()
        student["name"] = new_name
        print("✅ 姓名修改成功")

    elif choice == "2":
        # 修改年龄
        while True:
            age_str = input("请输入新年龄：").strip()
            if age_str.isdigit():
                new_age = int(age_str)
                if 1 <= new_age <= 150:
                    student["age"] = new_age
                    print("✅ 年龄修改成功")
                    break
                else:
                    print("❌ 年龄必须在1-150之间")
            else:
                print("❌ 年龄必须是数字")

    elif choice == "3":
        # 修改成绩
        print("\n请输入新的三门课程成绩（0-100分）：")
        new_scores = []
        subjects = ["语文", "数学", "英语"]

        for subject in subjects:
            while True:
                score_str = input(f"  {subject}成绩：").strip()
                try:
                    score = float(score_str)
                    if 0 <= score <= 100:
                        new_scores.append(score)
                        break
                    else:
                        print("    ❌ 成绩必须在0-100之间")
                except ValueError:
                    print("    ❌ 成绩必须是数字")

        # 更新成绩（转换为元组）
        student["scores"] = tuple(new_scores)
        print("✅ 成绩修改成功")

    else:
        print("❌ 无效的选项")

# ============================================
# 功能4：删除学生信息
# ============================================

def delete_student():
    """
    删除学生信息

    流程：
    1. 输入学号
    2. 查找学生
    3. 确认删除
    4. 从列表中移除
    """
    print("\n" + "=" * 60)
    print("删除学生信息")
    print("=" * 60)

    if not students:
        print("❌ 暂无学生信息")
        return

    # 输入学号
    id_str = input("请输入要删除的学生学号：").strip()
    if not id_str.isdigit():
        print("❌ 学号必须是数字")
        return

    student_id = int(id_str)

    # 查找学生
    student = None
    for s in students:
        if s["id"] == student_id:
            student = s
            break

    if not student:
        print(f"❌ 未找到学号为 {student_id} 的学生")
        return

    # 显示学生信息
    print("\n要删除的学生信息：")
    display_student_info(student)

    # 确认删除
    confirm = input("\n确认删除？(y/n)：").strip().lower()
    if confirm == "y" or confirm == "yes":
        students.remove(student)
        print("✅ 学生信息删除成功")
    else:
        print("❌ 取消删除")

# ============================================
# 功能5：显示所有学生信息
# ============================================

def display_all_students():
    """
    显示所有学生信息

    流程：
    1. 检查是否有学生
    2. 遍历学生列表
    3. 显示每个学生的信息
    """
    print("\n" + "=" * 60)
    print("所有学生信息")
    print("=" * 60)

    if not students:
        print("❌ 暂无学生信息")
        return

    print(f"\n共有 {len(students)} 个学生：\n")

    # 表头
    print(f"{'学号':<6} {'姓名':<10} {'年龄':<6} {'语文':<8} {'数学':<8} {'英语':<8} {'平均分':<8}")
    print("-" * 60)

    # 遍历学生列表
    for student in students:
        # 解包成绩元组
        chinese, math, english = student["scores"]
        average = sum(student["scores"]) / len(student["scores"])

        # 格式化输出
        print(f"{student['id']:<6} {student['name']:<10} {student['age']:<6} "
              f"{chinese:<8.1f} {math:<8.1f} {english:<8.1f} {average:<8.2f}")

# ============================================
# 功能6：统计学生成绩
# ============================================

def statistics():
    """
    统计学生成绩

    统计内容：
    1. 总人数
    2. 各科平均分
    3. 各科最高分、最低分
    4. 总平均分最高的学生
    """
    print("\n" + "=" * 60)
    print("成绩统计")
    print("=" * 60)

    if not students:
        print("❌ 暂无学生信息")
        return

    # 统计总人数
    total_students = len(students)
    print(f"\n总人数：{total_students}")

    # 收集各科成绩
    chinese_scores = []
    math_scores = []
    english_scores = []

    for student in students:
        chinese, math, english = student["scores"]
        chinese_scores.append(chinese)
        math_scores.append(math)
        english_scores.append(english)

    # 统计各科成绩
    print("\n各科成绩统计：")
    print("-" * 60)

    subjects = ["语文", "数学", "英语"]
    scores_list = [chinese_scores, math_scores, english_scores]

    for subject, scores in zip(subjects, scores_list):
        avg = sum(scores) / len(scores)
        max_score = max(scores)
        min_score = min(scores)
        print(f"{subject}：平均分 {avg:.2f}，最高分 {max_score:.1f}，最低分 {min_score:.1f}")

    # 找出总平均分最高的学生
    print("\n总平均分排名（前3名）：")
    print("-" * 60)

    # 计算每个学生的平均分
    student_averages = []
    for student in students:
        average = sum(student["scores"]) / len(student["scores"])
        student_averages.append((student, average))

    # 按平均分降序排序
    student_averages.sort(key=lambda x: x[1], reverse=True)

    # 显示前3名
    for i, (student, average) in enumerate(student_averages[:3], 1):
        print(f"第{i}名：{student['name']}（学号：{student['id']}），平均分：{average:.2f}")

# ============================================
# 主菜单
# ============================================

def show_menu():
    """显示主菜单"""
    print("\n" + "=" * 60)
    print("学生信息管理系统")
    print("=" * 60)
    print("1. 添加学生信息")
    print("2. 查询学生信息")
    print("3. 修改学生信息")
    print("4. 删除学生信息")
    print("5. 显示所有学生信息")
    print("6. 成绩统计")
    print("0. 退出系统")
    print("=" * 60)

def main():
    """主程序"""
    print("\n" + "=" * 60)
    print("欢迎使用学生信息管理系统！")
    print("=" * 60)

    # 添加一些示例数据（可选）
    # add_sample_data()

    while True:
        show_menu()
        choice = input("\n请输入选项（0-6）：").strip()

        if choice == "1":
            add_student()
        elif choice == "2":
            query_student()
        elif choice == "3":
            modify_student()
        elif choice == "4":
            delete_student()
        elif choice == "5":
            display_all_students()
        elif choice == "6":
            statistics()
        elif choice == "0":
            print("\n感谢使用学生信息管理系统！再见！👋")
            break
        else:
            print("\n❌ 无效的选项，请重新输入")

        # 暂停，等待用户按回车继续
        input("\n按回车键继续...")

# ============================================
# 添加示例数据（用于测试）
# ============================================

def add_sample_data():
    """添加示例数据（用于测试）"""
    global student_id_counter

    sample_students = [
        {"name": "张三", "age": 18, "scores": (85, 90, 88)},
        {"name": "李四", "age": 19, "scores": (92, 88, 95)},
        {"name": "王五", "age": 18, "scores": (78, 85, 80)},
        {"name": "赵六", "age": 20, "scores": (88, 92, 90)},
        {"name": "钱七", "age": 19, "scores": (95, 98, 96)},
    ]

    for data in sample_students:
        student = {
            "id": student_id_counter,
            "name": data["name"],
            "age": data["age"],
            "scores": data["scores"]
        }
        students.append(student)
        student_id_counter += 1

    print("✅ 已添加5个示例学生数据")

# ============================================
# 程序入口
# ============================================

if __name__ == "__main__":
    # 运行主程序
    main()

# ============================================
# 学习总结
# ============================================

"""
【项目总结】

本项目综合运用了：

1. 字典（Dictionary）：
   - 存储学生信息（键值对）
   - 使用 get() 安全访问
   - 使用 [] 修改值

2. 列表（List）：
   - 存储多个学生
   - 使用 append() 添加学生
   - 使用 remove() 删除学生
   - 使用 for 循环遍历

3. 元组（Tuple）：
   - 存储学生成绩（不可变）
   - 使用元组解包获取各科成绩
   - 使用 sum() 计算总分

4. 函数：
   - 模块化设计
   - 每个功能一个函数
   - 提高代码可读性和可维护性

5. 输入验证：
   - 验证数字输入
   - 验证范围
   - 错误处理

6. 用户交互：
   - 菜单系统
   - 友好的提示信息
   - 确认操作

【扩展思路】

可以继续扩展的功能：
1. 数据持久化（保存到文件）
2. 按成绩排序
3. 成绩等级评定（优秀、良好、及格、不及格）
4. 导出成绩单
5. 批量导入学生信息
6. 搜索功能增强（模糊搜索）
7. 数据备份和恢复

【学习建议】

1. 运行程序，测试所有功能
2. 尝试添加、查询、修改、删除学生
3. 观察数据结构的使用
4. 理解字典、列表、元组的配合
5. 尝试扩展新功能
"""
