# ============================================
# 面向对象进阶练习 - 动物继承系统
# 日期：2026-04-08（第10天）
# 难度：⭐⭐⭐ 中高级
# ============================================

"""
【综合练习目标】

通过“动物继承系统”练习：
1. 父类与子类的关系
2. 继承带来的代码复用
3. 方法重写
4. super() 的实际使用
5. 多态思想的初步体验
"""

# ============================================
# 功能1：定义父类
# ============================================

print("=" * 60)
print("功能1：定义 Animal 父类")
print("=" * 60)

class Animal:
    """动物父类：放所有动物共同拥有的数据和行为"""

    def __init__(self, name, age):
        self.name = name
        self.age = age

    def eat(self):
        print(f"{self.name} 正在吃东西。")

    def sleep(self):
        print(f"{self.name} 正在睡觉。")

    def speak(self):
        print(f"{self.name} 发出了声音。")

    def show_info(self):
        print(f"名字：{self.name}，年龄：{self.age}")

# ============================================
# 功能2：定义子类
# ============================================

print("\n" + "=" * 60)
print("功能2：定义 Cat 和 Dog 子类")
print("=" * 60)

class Cat(Animal):
    """猫类：继承 Animal，并增加自己的特有属性和行为"""

    def __init__(self, name, age, color):
        # 先调用父类初始化，复用 name 和 age 的赋值逻辑
        super().__init__(name, age)
        # 再补充子类自己的属性
        self.color = color

    def speak(self):
        # 方法重写：猫有自己的叫声
        print(f"{self.name}：喵喵喵！")

    def catch_mouse(self):
        print(f"{self.name} 正在抓老鼠。")

    def show_info(self):
        # 重写 show_info，让输出更完整
        print(f"猫咪名字：{self.name}，年龄：{self.age}，颜色：{self.color}")

class Dog(Animal):
    """狗类：继承 Animal，并增加自己的特有属性和行为"""

    def __init__(self, name, age, breed):
        super().__init__(name, age)
        self.breed = breed

    def speak(self):
        print(f"{self.name}：汪汪汪！")

    def guard_home(self):
        print(f"{self.name} 正在看家。")

    def show_info(self):
        print(f"小狗名字：{self.name}，年龄：{self.age}，品种：{self.breed}")

# ============================================
# 功能3：创建对象并测试
# ============================================

print("\n" + "=" * 60)
print("功能3：创建对象并测试继承")
print("=" * 60)

cat1 = Cat("咪咪", 2, "橘色")
dog1 = Dog("旺财", 3, "金毛")

print("\n测试 show_info()：")
cat1.show_info()
dog1.show_info()

print("\n测试继承来的方法：")
cat1.eat()
dog1.sleep()

print("\n测试方法重写：")
cat1.speak()
dog1.speak()

print("\n测试子类自己的方法：")
cat1.catch_mouse()
dog1.guard_home()

# ============================================
# 功能4：多态体验
# ============================================

print("\n" + "=" * 60)
print("功能4：多态体验")
print("=" * 60)

# 这个函数不关心传入的是猫还是狗
# 只要这个对象有 speak() 方法，就能正常工作
def animal_show(animal):
    animal.show_info()
    animal.speak()
    animal.eat()
    print("-" * 40)

animal_list = [
    Cat("小花", 1, "白色"),
    Dog("大黑", 4, "拉布拉多"),
    Cat("奶糖", 3, "灰色"),
]

print("\n循环处理不同动物对象：")
for animal in animal_list:
    animal_show(animal)

# ============================================
# 功能5：理解 super() 的价值
# ============================================

print("\n" + "=" * 60)
print("功能5：理解 super() 的价值")
print("=" * 60)

print("""
super() 的意义：
- 父类已经写好的初始化逻辑，子类直接复用
- 子类只写自己新增的属性
- 这样可以减少重复代码

例如：
Cat 和 Dog 都不需要重新写 name、age 的赋值过程
因为父类 Animal 已经处理好了。
""")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结：今天练习学到了什么")
print("=" * 60)

print("""
通过今天的练习，你已经实际用到了：

1. 父类和子类
2. 继承复用父类代码
3. super() 调用父类初始化
4. 方法重写
5. 多态思想的初步体验

下一步：可以继续学习封装、私有属性，或者进一步做更完整的小项目。
""")

# ============================================
# 练习题（不提供答案）
# ============================================

print("\n" + "=" * 60)
print("附加练习题（自己动手）")
print("=" * 60)

print("""
练习1：创建 Person 父类 和 Teacher 子类
  要求：
  - Person 有 name、age
  - Teacher 增加 subject
  - 用 super() 初始化
  - 重写 show_info()

练习2：创建 Bird 父类 和 Sparrow、Eagle 子类
  要求：
  - 父类有 fly() 方法
  - 两个子类分别重写 fly()
  - 创建对象并观察不同输出

练习3：创建 Device 父类 和 Phone、Laptop 子类
  要求：
  - 父类有 brand 属性和 start() 方法
  - 子类重写 start()
  - 用列表统一调用 start()
  提示：练习多态思想
""")
