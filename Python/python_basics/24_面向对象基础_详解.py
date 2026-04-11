# ============================================
# 面向对象基础 - 超详细注释版
# 日期：2026-04-08（第9天）
# 难度：⭐⭐ 中级
# ============================================

"""
【核心概念】面向对象编程（OOP）

是什么？
- 面向对象是一种组织代码的方式，把“数据”和“操作数据的方法”放在一起
- 在 Python 里，通常通过“类（class）”和“对象（object）”来实现

为什么需要？
- 现实世界很多东西本来就适合描述成“对象”
- 代码更容易组织、复用、扩展
- 为后面的继承、封装和项目开发打基础

核心特点？
- 类是模板，对象是实例
- 属性保存数据，方法描述行为
- `self` 表示“当前对象自己”
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

print("""
【类和对象】

什么是类？
- 类（class）可以理解成“设计图”或“模板”
- 它规定了：这个对象应该有哪些属性、能做哪些事情

什么是对象？
- 对象（object）是根据类创建出来的“具体实例”
- 同一个类，可以创建很多个不同对象

生活例子：
- 类：汽车设计图
- 对象：一辆具体的红色汽车、一辆具体的蓝色汽车

在代码里：
- 类定义通用规则
- 对象保存自己的具体数据
""")

print("""
【class 关键字】

语法：
class 类名:
    代码块

命名规范：
- 类名通常用大驼峰命名法（PascalCase）
- 例如：Student、Car、Dog

为什么类名首字母通常大写？
- 这样一眼就能和普通变量、函数区分开
""")

print("""
【__init__() 函数卡片】

函数名：__init__()
作用：初始化对象，在创建对象时自动执行
类型：类中的特殊方法（魔术方法）

完整语法：
class 类名:
    def __init__(self, 参数1, 参数2):
        self.属性1 = 参数1
        self.属性2 = 参数2

参数详解：
1. self（必需）
   - 类型：对象本身
   - 作用：代表“当前创建出来的对象”
   - 注意：调用时不用手动传，Python 会自动传入

2. 其他参数（可选/必需）
   - 类型：由你自己决定
   - 作用：给对象的属性赋初始值
   - 示例：name、age、color

返回值：
- 不需要写 return
- 它的任务不是返回结果，而是“初始化对象”
""")

print("""
【self 函数卡片】

self 不是关键字，只是约定俗成的名字。

为什么必须写 self？
- 因为类里定义的方法要知道：
  “现在操作的是哪个对象？”

self 的作用：
- 访问当前对象的属性：self.name
- 调用当前对象的方法：self.say_hello()

记忆方法：
- self ≈ “我自己”
""")

# ============================================
# 第二部分：基础操作实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 最简单的类
# --------------------------------------------

print("\n2.1 最简单的类")
print("-" * 60)

class Dog:
    """最简单的狗类：先只放一个方法，帮助理解类的基本结构"""

    def bark(self):
        """实例方法：表示狗会叫"""
        print("汪汪汪！")

# 创建对象：Dog() 会根据 Dog 这个类创建一个具体对象
my_dog = Dog()

print("已创建对象 my_dog")
# 调用对象的方法
my_dog.bark()

# --------------------------------------------
# 2.2 带属性的类
# --------------------------------------------

print("\n2.2 带属性的类")
print("-" * 60)

class Student:
    """学生类：演示属性和方法怎么配合使用"""

    def __init__(self, name, age, score):
        # self.name 表示“当前对象自己的 name 属性”
        # name 是外部传入的参数
        self.name = name
        self.age = age
        self.score = score

    def introduce(self):
        # 这里用 self.属性 读取当前对象保存的数据
        print(f"大家好，我叫{self.name}，今年{self.age}岁，成绩是{self.score}分。")

    def is_passing(self):
        # 方法里也可以做判断并返回结果
        return self.score >= 60

# 创建两个不同对象
student1 = Student("小明", 18, 88)
student2 = Student("小红", 17, 56)

print(f"student1.name = {student1.name}")
print(f"student2.name = {student2.name}")

student1.introduce()
student2.introduce()

print(f"{student1.name}是否及格：{student1.is_passing()}")
print(f"{student2.name}是否及格：{student2.is_passing()}")

# --------------------------------------------
# 2.3 修改对象属性
# --------------------------------------------

print("\n2.3 修改对象属性")
print("-" * 60)

print(f"修改前：{student2.name} 的成绩 = {student2.score}")
# 直接修改对象属性
student2.score = 66
print(f"修改后：{student2.name} 的成绩 = {student2.score}")
print(f"修改后是否及格：{student2.is_passing()}")

# --------------------------------------------
# 2.4 一个更贴近生活的例子：汽车类
# --------------------------------------------

print("\n2.4 汽车类")
print("-" * 60)

class Car:
    """汽车类：更清楚地体会“属性是数据，方法是行为”"""

    def __init__(self, brand, color, speed=0):
        self.brand = brand
        self.color = color
        self.speed = speed

    def accelerate(self, value):
        """加速：让速度增加"""
        self.speed += value
        print(f"{self.color}{self.brand}加速后，当前速度：{self.speed} km/h")

    def brake(self, value):
        """减速：速度不能小于 0"""
        self.speed -= value
        if self.speed < 0:
            self.speed = 0
        print(f"{self.color}{self.brand}减速后，当前速度：{self.speed} km/h")

    def show_info(self):
        print(f"品牌：{self.brand}，颜色：{self.color}，速度：{self.speed} km/h")

car1 = Car("比亚迪", "白色")
car2 = Car("特斯拉", "黑色", 20)

car1.show_info()
car1.accelerate(30)
car1.brake(10)

car2.show_info()
car2.accelerate(40)
car2.brake(100)

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("\n" + "=" * 60)
print("第三部分：深入理解（选学，可跳过）")
print("=" * 60)

print("""
🔍 深入理解：为什么方法里第一个参数通常是 self？

如果你觉得这部分有点抽象，可以跳过，不影响继续学习。

当你写：
    student1.introduce()

表面上看，你没有传参数。
但 Python 实际会偷偷把 student1 这个对象传进去，等价于：
    Student.introduce(student1)

所以：
- self 就是“当前调用这个方法的对象”
- 谁调用方法，self 就是谁

这就是为什么不同对象调用同一个方法，却能打印出不同结果。
""")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
错误1：定义方法时忘记写 self

❌ 错误代码：
class Dog:
    def bark():
        print('汪汪')

d = Dog()
d.bark()   # TypeError

原因：
- 对象调用方法时，Python 会自动传入当前对象
- 你没写 self，就没有位置接收这个参数

✅ 正确代码：
class Dog:
    def bark(self):
        print('汪汪')
""")

print("""
错误2：把类当对象用

❌ 错误代码：
class Student:
    pass

Student.name = '小明'   # 这不是在创建具体学生对象

原因：
- 类是模板，不是具体实例
- 一般应该先创建对象，再给对象设置数据

✅ 正确代码：
student = Student()
student.name = '小明'
""")

print("""
错误3：创建对象时漏传参数

❌ 错误代码：
class Student:
    def __init__(self, name, age):
        self.name = name
        self.age = age

s = Student('小明')   # 少传了 age

✅ 正确代码：
s = Student('小明', 18)
""")

print("""
错误4：在类的方法里直接用变量名，不写 self.

❌ 错误代码：
class Student:
    def __init__(self, name):
        self.name = name

    def hello(self):
        print(name)   # NameError

原因：
- name 只是 __init__() 里的局部参数
- 保存到对象里的属性要通过 self.name 访问

✅ 正确代码：
    def hello(self):
        print(self.name)
""")

print("""
错误5：误以为 __init__() 需要手动调用

❌ 错误理解：
student = Student()
student.__init__()   # 不需要这样做

✅ 正确理解：
- 创建对象时，__init__() 会自动执行
- 你只需要写：Student(...)
""")

print("""
错误6：多个对象的数据会互相共享

❌ 错误理解：
- 以为修改 student1.name 会影响 student2.name

✅ 正确认识：
- 每个对象通常都有自己独立的实例属性
- student1 和 student2 保存的是各自的数据
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【class】
语法：
class 类名:
    代码块

作用：定义一个类

────────────────────────────────────

【__init__()】
语法：
def __init__(self, 参数...):
    self.属性 = 参数

作用：初始化对象，创建对象时自动执行

────────────────────────────────────

【实例方法】
语法：
def 方法名(self, 参数...):
    代码

作用：描述对象能做什么
注意：第一个参数通常必须是 self

────────────────────────────────────

【创建对象】
语法：
对象名 = 类名(参数...)

示例：
student = Student('小明', 18, 90)

────────────────────────────────────

【访问属性】
语法：
对象名.属性名

示例：
print(student.name)
student.score = 100
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题（自己动手！）")
print("=" * 60)

print("""
练习1：创建一个 Cat 类
  要求：
  - 有 name、color 两个属性
  - 有一个 meow() 方法，打印“某某猫在喵喵叫”
  提示：先写 __init__()，再写普通方法

练习2：创建一个 Book 类
  要求：
  - 有 title、author、price 三个属性
  - 有一个 show_info() 方法，输出图书信息
  - 创建两个不同的图书对象并分别调用方法
  提示：体会“同一个类可以创建多个对象”

练习3：创建一个 BankAccount 类
  要求：
  - 有 owner、balance 属性
  - 有 deposit() 存款方法
  - 有 withdraw() 取款方法
  - 余额不能变成负数
  提示：withdraw() 里要加判断

练习4：创建一个 Rectangle 类
  要求：
  - 有 width、height 属性
  - 有 area() 方法计算面积
  - 有 perimeter() 方法计算周长
  提示：属性保存数据，方法负责计算
""")
