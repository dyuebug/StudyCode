# ============================================
# 魔法方法详解
# 日期：2026-06-04（第18天）
# 难度：⭐⭐⭐⭐ 面向对象高级特性
# ============================================

"""
【学习目标】

通过本节学习，你将掌握：
1. 魔法方法的概念和作用
2. 字符串表示方法（__str__、__repr__）
3. 容器类型方法（__len__、__getitem__等）
4. 运算符重载（__add__、__sub__等）
5. 比较运算符（__eq__、__lt__等）
6. 其他常用魔法方法

【为什么需要魔法方法？】

魔法方法（Magic Methods）也叫特殊方法（Special Methods）：
- 让自定义类像内置类型一样使用
- 支持运算符操作（+、-、*、/等）
- 支持内置函数（len()、str()等）
- 让代码更加Pythonic和优雅

例如：
- 使用 + 运算符：需要实现 __add__()
- 使用 len() 函数：需要实现 __len__()
- 使用 print() 打印：需要实现 __str__()
"""

# ============================================
# 第一部分：字符串表示方法
# ============================================

print("=" * 60)
print("第一部分：字符串表示方法")
print("=" * 60)

print("""
【__str__() 和 __repr__()】

__str__()：
- 用户友好的字符串表示
- print() 函数调用
- str() 函数调用
- 目标：可读性

__repr__()：
- 开发者友好的字符串表示
- 交互式环境中显示
- repr() 函数调用
- 目标：明确性、可重建对象

区别：
- __str__() 给用户看
- __repr__() 给开发者看
- 如果只定义一个，定义 __repr__()
- 如果没有 __str__()，会使用 __repr__()
""")

# 示例1：__str__() 和 __repr__()
print("\n示例1：__str__() 和 __repr__()")
print("-" * 60)

class Book:
    """书籍类：演示字符串表示方法"""

    def __init__(self, title, author, price):
        """初始化书籍

        参数：
            title: 书名
            author: 作者
            price: 价格
        """
        self.title = title
        self.author = author
        self.price = price

    def __str__(self):
        """用户友好的字符串表示

        返回：
            格式化的字符串，适合展示给用户
        """
        return f"《{self.title}》 - {self.author}（¥{self.price}）"

    def __repr__(self):
        """开发者友好的字符串表示

        返回：
            可以重建对象的字符串表示
        """
        return f"Book('{self.title}', '{self.author}', {self.price})"

# 测试字符串表示
book = Book("Python编程", "张三", 89.0)

print("使用 print()（调用 __str__()）：")
print(book)

print("\n使用 str()（调用 __str__()）：")
print(str(book))

print("\n使用 repr()（调用 __repr__()）：")
print(repr(book))

print("\n在交互式环境中直接输入变量名（调用 __repr__()）：")
print(f">>> book")
print(repr(book))

# ============================================
# 第二部分：容器类型方法
# ============================================

print("\n" + "=" * 60)
print("第二部分：容器类型方法")
print("=" * 60)

print("""
【容器类型魔法方法】

__len__()：
- 返回容器长度
- len() 函数调用

__getitem__(key)：
- 索引访问：obj[key]
- 支持切片

__setitem__(key, value)：
- 索引赋值：obj[key] = value

__delitem__(key)：
- 索引删除：del obj[key]

__contains__(item)：
- 成员测试：item in obj

__iter__()：
- 返回迭代器
- for 循环调用
""")

# 示例1：自定义列表类
print("\n示例1：自定义列表类")
print("-" * 60)

class MyList:
    """自定义列表类：演示容器类型方法"""

    def __init__(self):
        """初始化空列表"""
        self._items = []

    def __len__(self):
        """返回列表长度

        返回：
            列表中元素的数量
        """
        print(f"  [调用 __len__()]")
        return len(self._items)

    def __getitem__(self, index):
        """索引访问

        参数：
            index: 索引或切片

        返回：
            对应位置的元素
        """
        print(f"  [调用 __getitem__({index})]")
        return self._items[index]

    def __setitem__(self, index, value):
        """索引赋值

        参数：
            index: 索引
            value: 新值
        """
        print(f"  [调用 __setitem__({index}, {value})]")
        self._items[index] = value

    def __delitem__(self, index):
        """索引删除

        参数：
            index: 索引
        """
        print(f"  [调用 __delitem__({index})]")
        del self._items[index]

    def __contains__(self, item):
        """成员测试

        参数：
            item: 要查找的元素

        返回：
            True 如果元素存在，否则 False
        """
        print(f"  [调用 __contains__({item})]")
        return item in self._items

    def __iter__(self):
        """返回迭代器

        返回：
            迭代器对象
        """
        print(f"  [调用 __iter__()]")
        return iter(self._items)

    def append(self, item):
        """添加元素

        参数：
            item: 要添加的元素
        """
        self._items.append(item)

    def __str__(self):
        """字符串表示"""
        return f"MyList({self._items})"

# 测试容器类型方法
print("创建自定义列表：")
my_list = MyList()
my_list.append(10)
my_list.append(20)
my_list.append(30)
print(my_list)

print("\n使用 len()：")
print(f"长度：{len(my_list)}")

print("\n使用索引访问：")
print(f"my_list[0] = {my_list[0]}")
print(f"my_list[1] = {my_list[1]}")

print("\n使用切片：")
print(f"my_list[0:2] = {my_list[0:2]}")

print("\n使用索引赋值：")
my_list[1] = 25
print(my_list)

print("\n使用 in 运算符：")
print(f"10 in my_list: {10 in my_list}")
print(f"100 in my_list: {100 in my_list}")

print("\n使用 for 循环：")
for item in my_list:
    print(f"  {item}")

# ============================================
# 第三部分：运算符重载
# ============================================

print("\n" + "=" * 60)
print("第三部分：运算符重载")
print("=" * 60)

print("""
【运算符重载魔法方法】

算术运算符：
__add__(self, other)      # +
__sub__(self, other)      # -
__mul__(self, other)      # *
__truediv__(self, other)  # /
__floordiv__(self, other) # //
__mod__(self, other)      # %
__pow__(self, other)      # **

反向运算符（当左操作数不支持时）：
__radd__(self, other)     # +
__rsub__(self, other)     # -
__rmul__(self, other)     # *

增强赋值运算符：
__iadd__(self, other)     # +=
__isub__(self, other)     # -=
__imul__(self, other)     # *=
""")

# 示例1：向量类
print("\n示例1：向量类")
print("-" * 60)

class Vector:
    """二维向量类：演示运算符重载"""

    def __init__(self, x, y):
        """初始化向量

        参数：
            x: x坐标
            y: y坐标
        """
        self.x = x
        self.y = y

    def __add__(self, other):
        """向量加法：v1 + v2

        参数：
            other: 另一个向量

        返回：
            新的向量对象
        """
        if not isinstance(other, Vector):
            raise TypeError("只能与Vector相加")
        return Vector(self.x + other.x, self.y + other.y)

    def __sub__(self, other):
        """向量减法：v1 - v2

        参数：
            other: 另一个向量

        返回：
            新的向量对象
        """
        if not isinstance(other, Vector):
            raise TypeError("只能与Vector相减")
        return Vector(self.x - other.x, self.y - other.y)

    def __mul__(self, scalar):
        """向量数乘：v * scalar

        参数：
            scalar: 标量（数字）

        返回：
            新的向量对象
        """
        if not isinstance(scalar, (int, float)):
            raise TypeError("只能与数字相乘")
        return Vector(self.x * scalar, self.y * scalar)

    def __rmul__(self, scalar):
        """反向数乘：scalar * v

        参数：
            scalar: 标量（数字）

        返回：
            新的向量对象
        """
        return self.__mul__(scalar)

    def __truediv__(self, scalar):
        """向量除法：v / scalar

        参数：
            scalar: 标量（数字）

        返回：
            新的向量对象
        """
        if not isinstance(scalar, (int, float)):
            raise TypeError("只能除以数字")
        if scalar == 0:
            raise ValueError("不能除以0")
        return Vector(self.x / scalar, self.y / scalar)

    def __neg__(self):
        """向量取负：-v

        返回：
            新的向量对象
        """
        return Vector(-self.x, -self.y)

    def __abs__(self):
        """向量的模（长度）：abs(v)

        返回：
            向量的长度
        """
        return (self.x ** 2 + self.y ** 2) ** 0.5

    def __str__(self):
        """字符串表示"""
        return f"Vector({self.x}, {self.y})"

    def __repr__(self):
        """开发者表示"""
        return f"Vector({self.x}, {self.y})"

# 测试运算符重载
print("创建向量：")
v1 = Vector(3, 4)
v2 = Vector(1, 2)
print(f"v1 = {v1}")
print(f"v2 = {v2}")

print("\n向量加法：")
v3 = v1 + v2
print(f"v1 + v2 = {v3}")

print("\n向量减法：")
v4 = v1 - v2
print(f"v1 - v2 = {v4}")

print("\n向量数乘：")
v5 = v1 * 2
print(f"v1 * 2 = {v5}")

print("\n反向数乘：")
v6 = 3 * v1
print(f"3 * v1 = {v6}")

print("\n向量除法：")
v7 = v1 / 2
print(f"v1 / 2 = {v7}")

print("\n向量取负：")
v8 = -v1
print(f"-v1 = {v8}")

print("\n向量的模：")
print(f"abs(v1) = {abs(v1)}")

# ============================================
# 第四部分：比较运算符
# ============================================

print("\n" + "=" * 60)
print("第四部分：比较运算符")
print("=" * 60)

print("""
【比较运算符魔法方法】

__eq__(self, other)  # ==（等于）
__ne__(self, other)  # !=（不等于）
__lt__(self, other)  # <（小于）
__le__(self, other)  # <=（小于等于）
__gt__(self, other)  # >（大于）
__ge__(self, other)  # >=（大于等于）

注意：
- 定义了 __eq__()，通常也要定义 __ne__()
- 定义了 __lt__()，通常也要定义 __le__()、__gt__()、__ge__()
- 或者使用 functools.total_ordering 装饰器
""")

# 示例1：学生类
print("\n示例1：学生类")
print("-" * 60)

class Student:
    """学生类：演示比较运算符"""

    def __init__(self, name, score):
        """初始化学生

        参数：
            name: 姓名
            score: 分数
        """
        self.name = name
        self.score = score

    def __eq__(self, other):
        """相等比较：s1 == s2

        参数：
            other: 另一个学生

        返回：
            True 如果分数相等
        """
        if not isinstance(other, Student):
            return False
        return self.score == other.score

    def __ne__(self, other):
        """不等比较：s1 != s2

        参数：
            other: 另一个学生

        返回：
            True 如果分数不等
        """
        return not self.__eq__(other)

    def __lt__(self, other):
        """小于比较：s1 < s2

        参数：
            other: 另一个学生

        返回：
            True 如果分数小于
        """
        if not isinstance(other, Student):
            raise TypeError("只能与Student比较")
        return self.score < other.score

    def __le__(self, other):
        """小于等于比较：s1 <= s2

        参数：
            other: 另一个学生

        返回：
            True 如果分数小于等于
        """
        return self.score <= other.score

    def __gt__(self, other):
        """大于比较：s1 > s2

        参数：
            other: 另一个学生

        返回：
            True 如果分数大于
        """
        if not isinstance(other, Student):
            raise TypeError("只能与Student比较")
        return self.score > other.score

    def __ge__(self, other):
        """大于等于比较：s1 >= s2

        参数：
            other: 另一个学生

        返回：
            True 如果分数大于等于
        """
        return self.score >= other.score

    def __str__(self):
        """字符串表示"""
        return f"{self.name}（{self.score}分）"

# 测试比较运算符
print("创建学生：")
s1 = Student("张三", 85)
s2 = Student("李四", 90)
s3 = Student("王五", 85)

print(f"s1 = {s1}")
print(f"s2 = {s2}")
print(f"s3 = {s3}")

print("\n相等比较：")
print(f"s1 == s2: {s1 == s2}")
print(f"s1 == s3: {s1 == s3}")

print("\n不等比较：")
print(f"s1 != s2: {s1 != s2}")

print("\n大小比较：")
print(f"s1 < s2: {s1 < s2}")
print(f"s1 > s2: {s1 > s2}")
print(f"s1 <= s3: {s1 <= s3}")
print(f"s1 >= s3: {s1 >= s3}")

print("\n排序：")
students = [s1, s2, s3]
students.sort()  # 使用 __lt__() 排序
print("按分数排序：")
for s in students:
    print(f"  {s}")

# ============================================
# 第五部分：其他常用魔法方法
# ============================================

print("\n" + "=" * 60)
print("第五部分：其他常用魔法方法")
print("=" * 60)

print("""
【其他常用魔法方法】

__init__(self, ...)：
- 初始化方法
- 创建对象时调用

__del__(self)：
- 析构方法
- 对象被销毁时调用

__call__(self, ...)：
- 让对象可调用
- obj() 调用

__bool__(self)：
- 布尔值转换
- bool(obj) 或 if obj 调用

__hash__(self)：
- 返回哈希值
- hash(obj) 调用
- 用于字典的键和集合

__format__(self, format_spec)：
- 格式化字符串
- format(obj, spec) 调用
""")

# 示例1：可调用对象
print("\n示例1：可调用对象")
print("-" * 60)

class Multiplier:
    """乘法器类：演示 __call__()"""

    def __init__(self, factor):
        """初始化乘法器

        参数：
            factor: 乘数
        """
        self.factor = factor

    def __call__(self, x):
        """让对象可调用

        参数：
            x: 被乘数

        返回：
            x * factor
        """
        return x * self.factor

    def __str__(self):
        return f"Multiplier(factor={self.factor})"

# 测试可调用对象
print("创建乘法器：")
double = Multiplier(2)
triple = Multiplier(3)

print(f"double = {double}")
print(f"triple = {triple}")

print("\n调用对象（像函数一样）：")
print(f"double(5) = {double(5)}")
print(f"triple(5) = {triple(5)}")

# 示例2：布尔值转换
print("\n示例2：布尔值转换")
print("-" * 60)

class ShoppingCart:
    """购物车类：演示 __bool__()"""

    def __init__(self):
        """初始化购物车"""
        self._items = []

    def add(self, item):
        """添加商品

        参数：
            item: 商品
        """
        self._items.append(item)

    def __bool__(self):
        """布尔值转换

        返回：
            True 如果购物车不为空
        """
        return len(self._items) > 0

    def __len__(self):
        """返回商品数量"""
        return len(self._items)

    def __str__(self):
        return f"ShoppingCart({len(self._items)} items)"

# 测试布尔值转换
print("创建购物车：")
cart = ShoppingCart()

print(f"\n空购物车：{cart}")
print(f"bool(cart) = {bool(cart)}")
if cart:
    print("购物车有商品")
else:
    print("购物车是空的")

print("\n添加商品：")
cart.add("苹果")
cart.add("香蕉")
print(f"购物车：{cart}")
print(f"bool(cart) = {bool(cart)}")
if cart:
    print("购物车有商品")
else:
    print("购物车是空的")

# 示例3：哈希值
print("\n示例3：哈希值")
print("-" * 60)

class Point:
    """点类：演示 __hash__()"""

    def __init__(self, x, y):
        """初始化点

        参数：
            x: x坐标
            y: y坐标
        """
        self.x = x
        self.y = y

    def __eq__(self, other):
        """相等比较"""
        if not isinstance(other, Point):
            return False
        return self.x == other.x and self.y == other.y

    def __hash__(self):
        """返回哈希值

        返回：
            基于坐标的哈希值
        """
        return hash((self.x, self.y))

    def __str__(self):
        return f"Point({self.x}, {self.y})"

# 测试哈希值
print("创建点：")
p1 = Point(1, 2)
p2 = Point(3, 4)
p3 = Point(1, 2)

print(f"p1 = {p1}, hash = {hash(p1)}")
print(f"p2 = {p2}, hash = {hash(p2)}")
print(f"p3 = {p3}, hash = {hash(p3)}")

print("\n使用点作为字典的键：")
point_dict = {p1: "点1", p2: "点2"}
print(f"point_dict[p1] = {point_dict[p1]}")
print(f"point_dict[p3] = {point_dict[p3]}")  # p3 和 p1 相等

print("\n使用点作为集合元素：")
point_set = {p1, p2, p3}
print(f"集合大小：{len(point_set)}")  # p1 和 p3 相等，只保留一个

# ============================================
# 第六部分：常见错误和陷阱
# ============================================

print("\n" + "=" * 60)
print("第六部分：常见错误和陷阱")
print("=" * 60)

print("""
【常见错误】

1. __str__() 返回非字符串
   ❌ 错误：
   def __str__(self):
       return 123  # 错误！必须返回字符串

   ✅ 正确：
   def __str__(self):
       return "123"

2. __len__() 返回负数
   ❌ 错误：
   def __len__(self):
       return -1  # 错误！长度不能为负

   ✅ 正确：
   def __len__(self):
       return 0  # 或正整数

3. 运算符重载不返回新对象
   ❌ 错误：
   def __add__(self, other):
       self.value += other.value  # 错误！修改了self
       return self

   ✅ 正确：
   def __add__(self, other):
       return MyClass(self.value + other.value)  # 返回新对象

4. 比较运算符类型检查不完整
   ❌ 错误：
   def __eq__(self, other):
       return self.value == other.value  # 如果other不是同类型会报错

   ✅ 正确：
   def __eq__(self, other):
       if not isinstance(other, MyClass):
           return False
       return self.value == other.value

5. 定义了 __eq__() 但没定义 __hash__()
   ❌ 问题：
   class MyClass:
       def __eq__(self, other):
           return self.value == other.value
       # 没有定义 __hash__()，对象不能用作字典键

   ✅ 正确：
   class MyClass:
       def __eq__(self, other):
           if not isinstance(other, MyClass):
               return False
           return self.value == other.value

       def __hash__(self):
           return hash(self.value)

【最佳实践】

1. 字符串表示
   - __str__() 给用户看，简洁友好
   - __repr__() 给开发者看，明确完整
   - 至少实现 __repr__()

2. 运算符重载
   - 返回新对象，不修改原对象
   - 检查参数类型
   - 提供友好的错误信息

3. 比较运算符
   - 实现 __eq__() 时也实现 __ne__()
   - 实现 __lt__() 时也实现其他比较运算符
   - 或使用 @functools.total_ordering 装饰器

4. 哈希值
   - 定义了 __eq__() 通常也要定义 __hash__()
   - 不可变对象才应该定义 __hash__()
   - 相等的对象必须有相同的哈希值

5. 容器类型
   - 实现 __len__() 返回非负整数
   - 实现 __getitem__() 支持索引和切片
   - 实现 __iter__() 支持迭代
""")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结")
print("=" * 60)

print("""
【本节重点】

1. 字符串表示
   - __str__()：用户友好，print() 调用
   - __repr__()：开发者友好，repr() 调用

2. 容器类型
   - __len__()：返回长度，len() 调用
   - __getitem__()：索引访问，obj[key]
   - __setitem__()：索引赋值，obj[key] = value
   - __contains__()：成员测试，item in obj
   - __iter__()：迭代器，for 循环

3. 运算符重载
   - __add__()、__sub__()、__mul__()：算术运算
   - __radd__()、__rmul__()：反向运算
   - __neg__()、__abs__()：一元运算

4. 比较运算符
   - __eq__()、__ne__()：相等、不等
   - __lt__()、__le__()、__gt__()、__ge__()：大小比较

5. 其他常用方法
   - __call__()：让对象可调用
   - __bool__()：布尔值转换
   - __hash__()：哈希值，用于字典和集合

【下一步】

- 完成综合练习
- 设计优雅的类
- 让自定义类更加Pythonic

记住：
- 魔法方法让类更加Pythonic
- 运算符重载返回新对象
- 比较运算符要检查类型
- 定义 __eq__() 通常也要定义 __hash__()
""")
