# ============================================
# 面向对象进阶 - 超详细注释版
# 日期：2026-04-08（第10天）
# 难度：⭐⭐⭐ 中高级
# ============================================

"""
【核心概念】面向对象进阶（继承、方法重写、多态）

是什么？
- 面向对象进阶主要研究：类和类之间如何复用、扩展和协作
- 在 Python 中，最常见的进阶内容就是继承、方法重写和多态

为什么需要？
- 避免重复代码
- 让相似对象共享通用能力
- 让不同对象可以表现出不同的行为

核心特点？
- 子类可以继承父类的属性和方法
- 子类可以重写父类的方法
- `super()` 可以调用父类中的方法
- 多态强调“同一个方法，不同对象表现不同”
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

print("""
【继承】

什么是继承？
- 继承就是：子类直接获得父类已有的属性和方法
- 这样就不用在子类里重复写一遍相同代码

生活例子：
- 父类：动物
- 子类：猫、狗

为什么猫和狗适合继承“动物”？
- 因为它们都有名字、年龄、会吃东西
- 这些共同点可以放在父类里
- 各自独特的行为，再放到子类里

语法：
class 子类名(父类名):
    代码块
""")

print("""
【方法重写】

什么是方法重写？
- 子类重新定义父类中同名的方法
- 这样子类就能有“自己的实现方式”

为什么需要方法重写？
- 因为虽然同属一个父类，但不同子类的行为可能不同

例子：
- 父类 Animal 有 speak() 方法
- 子类 Dog 的 speak() 是“汪汪”
- 子类 Cat 的 speak() 是“喵喵”
""")

print("""
【super() 函数卡片】

函数名：super()
作用：调用父类的方法
类型：内置函数，常用于继承场景

完整语法：
super().方法名(参数)

常见用途：
1. 调用父类的 __init__()，复用父类初始化逻辑
2. 在子类中扩展父类已有方法，而不是完全重写

为什么推荐用 super()？
- 避免把父类类名写死
- 代码更灵活，后续更容易维护

示例：
class Animal:
    def __init__(self, name):
        self.name = name

class Dog(Animal):
    def __init__(self, name, breed):
        super().__init__(name)
        self.breed = breed
""")

print("""
【多态】

什么是多态？
- 多态强调：同一个方法名，不同对象调用时，结果不同

先不要把它想得太复杂。
你可以先记住：
- 狗对象调用 speak()，输出“汪汪”
- 猫对象调用 speak()，输出“喵喵”

虽然都是 speak()，但不同对象响应不同，这就是多态思想。
""")

# ============================================
# 第二部分：基础操作实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 最基础的继承
# --------------------------------------------

print("\n2.1 最基础的继承")
print("-" * 60)

class Animal:
    """动物父类：放所有动物的共同属性和共同方法"""

    def __init__(self, name, age):
        # 所有动物都有名字和年龄
        self.name = name
        self.age = age

    def eat(self):
        # 共同方法：所有动物都可以吃东西
        print(f"{self.name} 正在吃东西。")

    def sleep(self):
        # 共同方法：所有动物都可以睡觉
        print(f"{self.name} 正在睡觉。")

class Dog(Animal):
    """狗类：继承 Animal，但这里先不加自己的新内容"""
    pass

# Dog 虽然自己什么都没写，但因为继承了 Animal，
# 所以自动拥有 __init__()、eat()、sleep()
dog1 = Dog("旺财", 3)
print(f"狗的名字：{dog1.name}，年龄：{dog1.age}")
dog1.eat()
dog1.sleep()

# --------------------------------------------
# 2.2 子类扩展自己的方法
# --------------------------------------------

print("\n2.2 子类扩展自己的方法")
print("-" * 60)

class Cat(Animal):
    """猫类：继承 Animal，并新增自己的方法"""

    def catch_mouse(self):
        print(f"{self.name} 正在抓老鼠。")

cat1 = Cat("咪咪", 2)
cat1.eat()          # 从父类继承来的
cat1.sleep()        # 从父类继承来的
cat1.catch_mouse()  # 子类自己新增的

# --------------------------------------------
# 2.3 方法重写
# --------------------------------------------

print("\n2.3 方法重写")
print("-" * 60)

class Bird(Animal):
    """鸟类：继承 Animal，并重写 eat() 方法"""

    def eat(self):
        # 这里的方法名和父类一样，所以会覆盖父类版本
        print(f"{self.name} 正在啄食小米。")

bird1 = Bird("小黄", 1)
# 调用的是子类重写后的 eat()，不是父类的 eat()
bird1.eat()
bird1.sleep()  # sleep() 没重写，所以还是父类的方法

# --------------------------------------------
# 2.4 super() 调用父类初始化
# --------------------------------------------

print("\n2.4 super() 调用父类初始化")
print("-" * 60)

class Fish(Animal):
    """鱼类：除了名字和年龄，还多一个 water_type 属性"""

    def __init__(self, name, age, water_type):
        # 先调用父类的 __init__()，复用名字和年龄的初始化代码
        super().__init__(name, age)
        # 再补充子类自己特有的属性
        self.water_type = water_type

    def show_info(self):
        print(f"名字：{self.name}，年龄：{self.age}，水域类型：{self.water_type}")

fish1 = Fish("小金", 1, "淡水")
fish1.show_info()
fish1.eat()

# --------------------------------------------
# 2.5 多态思想演示
# --------------------------------------------

print("\n2.5 多态思想演示")
print("-" * 60)

class AnimalWithVoice:
    """带叫声的动物父类"""

    def __init__(self, name):
        self.name = name

    def speak(self):
        print(f"{self.name} 发出了声音。")

class DogWithVoice(AnimalWithVoice):
    def speak(self):
        print(f"{self.name}：汪汪汪！")

class CatWithVoice(AnimalWithVoice):
    def speak(self):
        print(f"{self.name}：喵喵喵！")

class DuckWithVoice(AnimalWithVoice):
    def speak(self):
        print(f"{self.name}：嘎嘎嘎！")

# 这个函数不关心传进来的是狗、猫还是鸭子
# 只要对象有 speak() 方法，就可以工作
# 这就是多态思想最直观的体验
def make_animal_speak(animal):
    animal.speak()

animals = [
    DogWithVoice("大黄"),
    CatWithVoice("小白"),
    DuckWithVoice("唐老鸭")
]

for animal in animals:
    make_animal_speak(animal)

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("\n" + "=" * 60)
print("第三部分：深入理解（选学，可跳过）")
print("=" * 60)

print("""
🔍 深入理解：为什么继承能减少重复代码？

如果你觉得这部分抽象，可以先跳过，不影响继续学习。

假设没有继承：
- Dog 类自己写 name、age、eat()、sleep()
- Cat 类也自己写 name、age、eat()、sleep()
- Bird 类也自己写 name、age、eat()、sleep()

问题：
- 大量重复代码
- 如果以后想改 sleep() 的逻辑，要改很多个类

有了继承：
- 把共同内容放到父类 Animal
- 子类只写“自己独有的部分”

这符合一个很重要的编程原则：
- DRY（Don't Repeat Yourself）
- 不要写重复代码
""")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
错误1：子类写了 __init__()，却忘记调用父类初始化

❌ 错误代码：
class Dog(Animal):
    def __init__(self, name, age, breed):
        self.breed = breed

问题：
- name 和 age 没有被初始化
- 后面访问 self.name 可能报错

✅ 正确代码：
class Dog(Animal):
    def __init__(self, name, age, breed):
        super().__init__(name, age)
        self.breed = breed
""")

print("""
错误2：以为继承就是复制粘贴

❌ 错误理解：
- 继承 = 把父类代码复制到子类

✅ 正确认识：
- 继承不是复制，而是“子类可以直接使用父类能力”
- 父类修改后，子类也会受到影响
""")

print("""
错误3：方法重写后，以为父类方法还能自动执行

❌ 错误理解：
class Bird(Animal):
    def eat(self):
        print('鸟在吃')

- 以为调用 Bird.eat() 时，父类 eat() 也会自动执行

✅ 正确认识：
- 子类重写后，默认只执行子类版本
- 如果你还想执行父类版本，要手动写 super().eat()
""")

print("""
错误4：super() 后面忘记加括号

❌ 错误代码：
super.__init__(name, age)

原因：
- super 是函数，必须写成 super()

✅ 正确代码：
super().__init__(name, age)
""")

print("""
错误5：把多态理解得过于复杂

❌ 错误想法：
- 多态一定是特别高级、很难的语法

✅ 正确理解：
- 先把多态理解成：
  “同一个方法，不同对象调用时效果不同”
- 现阶段先理解思想，不要求你记特别复杂的定义
""")

print("""
错误6：父类设计过大，什么都往里放

❌ 不推荐：
- 把很多只属于某个子类的方法也塞进父类

✅ 推荐：
- 父类只放“共同的东西”
- 子类负责“自己的个性化能力”
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【继承语法】
语法：
class 子类名(父类名):
    代码块

作用：让子类继承父类的属性和方法

────────────────────────────────────

【方法重写】
语法：
class 子类名(父类名):
    def 同名方法(self):
        新实现

作用：让子类拥有自己的方法实现

────────────────────────────────────

【super()】
语法：
super().方法名(参数)

作用：调用父类的方法
常见场景：调用父类 __init__()

────────────────────────────────────

【多态思想】
核心：
- 同一个方法
- 不同对象
- 不同表现

示例：
dog.speak()  → 汪汪
cat.speak()  → 喵喵
duck.speak() → 嘎嘎
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题（自己动手！）")
print("=" * 60)

print("""
练习1：创建 Person 父类 和 Student 子类
  要求：
  - Person 有 name、age 属性
  - Student 增加 school 属性
  - 用 super() 调用父类初始化
  - 写一个 show_info() 方法输出完整信息
  提示：先写父类，再写子类

练习2：创建 Shape 父类 和 Rectangle、Circle 子类
  要求：
  - Shape 有 area() 方法
  - Rectangle 和 Circle 各自重写 area()
  - 创建对象并分别调用 area()
  提示：体会“同一个方法，不同对象结果不同”

练习3：创建 Employee 父类 和 Manager 子类
  要求：
  - Employee 有 name、salary 属性
  - Manager 增加 bonus 属性
  - 写 total_income() 返回总收入
  提示：Manager 可以在父类基础上扩展自己的数据

练习4：创建 Vehicle 父类 和 Car、Bike 子类
  要求：
  - Vehicle 有 start() 方法
  - Car 和 Bike 分别重写 start()
  - 用列表存多个对象，循环调用 start()
  提示：这就是多态的简单练习
""")
