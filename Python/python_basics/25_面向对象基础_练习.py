# ============================================
# 面向对象基础练习 - 学生类与汽车类
# 日期：2026-04-08（第9天）
# 难度：⭐⭐ 中级
# ============================================

"""
【综合练习目标】

通过两个小项目，把“类、对象、属性、方法、self、__init__()”真正用起来。

功能：
1. 创建学生类，管理学生信息和成绩状态
2. 创建汽车类，模拟汽车加速、减速和显示信息
3. 通过多个对象对比，理解“同一个类可以创建多个实例”
"""

# ============================================
# 功能1：学生类练习
# ============================================

print("=" * 60)
print("功能1：学生类练习")
print("=" * 60)

class Student:
    """学生类：保存学生信息，并提供常见操作方法"""

    def __init__(self, name, age, score):
        # 初始化对象属性
        self.name = name
        self.age = age
        self.score = score

    def show_info(self):
        """显示学生基本信息"""
        print(f"姓名：{self.name}，年龄：{self.age}，成绩：{self.score}")

    def get_grade(self):
        """根据成绩返回等级"""
        if self.score >= 90:
            return "A"
        elif self.score >= 80:
            return "B"
        elif self.score >= 70:
            return "C"
        elif self.score >= 60:
            return "D"
        else:
            return "F"

    def is_passing(self):
        """判断是否及格"""
        return self.score >= 60

    def update_score(self, new_score):
        """修改成绩"""
        self.score = new_score
        print(f"{self.name} 的成绩已更新为 {self.score} 分")

# 创建多个学生对象
student1 = Student("小明", 18, 92)
student2 = Student("小红", 17, 76)
student3 = Student("小刚", 18, 58)

students = [student1, student2, student3]

print("\n学生信息列表：")
for student in students:
    student.show_info()
    print(f"等级：{student.get_grade()}，是否及格：{student.is_passing()}")
    print("-" * 40)

# 演示修改属性后的效果
print("\n修改成绩演示：")
student3.update_score(68)
print(f"更新后等级：{student3.get_grade()}，是否及格：{student3.is_passing()}")

# ============================================
# 功能2：汽车类练习
# ============================================

print("\n" + "=" * 60)
print("功能2：汽车类练习")
print("=" * 60)

class Car:
    """汽车类：模拟汽车的基础状态和行为"""

    def __init__(self, brand, color, speed=0):
        self.brand = brand
        self.color = color
        self.speed = speed

    def show_info(self):
        """显示汽车信息"""
        print(f"品牌：{self.brand}，颜色：{self.color}，当前速度：{self.speed} km/h")

    def accelerate(self, amount):
        """加速"""
        self.speed += amount
        print(f"{self.color}{self.brand} 加速 {amount} km/h，当前速度：{self.speed} km/h")

    def brake(self, amount):
        """减速，速度最小为 0"""
        self.speed -= amount
        if self.speed < 0:
            self.speed = 0
        print(f"{self.color}{self.brand} 减速 {amount} km/h，当前速度：{self.speed} km/h")

# 创建多个汽车对象
car1 = Car("比亚迪", "白色")
car2 = Car("特斯拉", "黑色", 20)

print("\n汽车初始信息：")
car1.show_info()
car2.show_info()

print("\n汽车操作演示：")
car1.accelerate(30)
car1.brake(5)
car2.accelerate(50)
car2.brake(100)

# ============================================
# 功能3：对比多个对象
# ============================================

print("\n" + "=" * 60)
print("功能3：理解“同类不同对象”")
print("=" * 60)

print("""
观察：
- student1、student2、student3 都是 Student 类创建出来的对象
- car1、car2 都是 Car 类创建出来的对象
- 它们的方法结构一样，但保存的数据各不相同

这说明：
- 类决定“对象有什么能力”
- 对象决定“具体数据是什么”
""")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结：今天练习学到了什么")
print("=" * 60)

print("""
通过今天的练习，你已经实际用到了：

1. class 定义类
2. __init__() 初始化对象
3. self 访问当前对象自己的属性
4. 属性保存数据
5. 方法描述行为
6. 一个类创建多个对象

下一步：继续学习面向对象的进阶内容，比如继承和封装。
""")

# ============================================
# 练习题（不提供答案）
# ============================================

print("\n" + "=" * 60)
print("附加练习题（自己动手）")
print("=" * 60)

print("""
练习1：手机类 Phone
  要求：
  - 属性：brand、model、battery
  - 方法：call()、charge()
  提示：charge() 可以让 battery 增加，但不要超过 100

练习2：商品类 Product
  要求：
  - 属性：name、price、stock
  - 方法：sell(count)
  - 卖出时库存不能小于 0
  提示：在 sell() 里先判断库存够不够

练习3：电影类 Movie
  要求：
  - 属性：title、director、rating
  - 方法：show_info()
  - 创建 3 个电影对象输出信息
  提示：重点练习“一个类创建多个对象”
""")
