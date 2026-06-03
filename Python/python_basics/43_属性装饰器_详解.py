# ============================================
# 属性装饰器详解
# 日期：2026-06-04（第18天）
# 难度：⭐⭐⭐⭐ 面向对象高级特性
# ============================================

"""
【学习目标】

通过本节学习,你将掌握：
1. @property装饰器的作用和使用
2. getter、setter、deleter的概念
3. 只读属性和可读写属性
4. 属性验证的实现
5. 优雅的属性访问方式

【为什么需要属性装饰器？】

在面向对象编程中，我们经常需要：
- 控制属性的访问和修改
- 在设置属性时进行验证
- 计算属性值（而不是存储）
- 保持接口简洁（像访问属性一样调用方法）

@property装饰器让我们能够：
- 将方法转换为属性
- 提供优雅的属性访问方式
- 在背后添加验证逻辑
- 保持代码的封装性
"""

# ============================================
# 第一部分：@property基础
# ============================================

print("=" * 60)
print("第一部分：@property基础")
print("=" * 60)

print("""
【@property装饰器】

是什么？
- 一个内置装饰器
- 将方法转换为属性
- 提供优雅的属性访问方式

为什么需要？
- 替代getter方法
- 保持接口简洁
- 在背后添加逻辑

核心特点：
- 方法变成属性
- 访问时不需要括号
- 可以添加setter和deleter
""")

# 示例1：基本使用
print("\n示例1：基本使用")
print("-" * 60)

class Circle:
    """圆形类：演示@property基础用法"""

    def __init__(self, radius):
        """初始化圆形

        参数：
            radius: 半径
        """
        self._radius = radius  # 使用下划线表示"内部属性"

    # 使用@property装饰器，将方法转换为属性
    @property
    def radius(self):
        """获取半径（getter）"""
        print("  [调用getter]")
        return self._radius

    @property
    def diameter(self):
        """计算直径（只读属性）"""
        print("  [计算直径]")
        return self._radius * 2

    @property
    def area(self):
        """计算面积（只读属性）"""
        print("  [计算面积]")
        return 3.14159 * self._radius ** 2

# 测试@property
print("创建圆形：")
circle = Circle(5)

print("\n访问半径（像访问属性一样，不需要括号）：")
print(f"半径：{circle.radius}")  # 调用radius()方法，但看起来像访问属性

print("\n访问直径（自动计算）：")
print(f"直径：{circle.diameter}")  # 调用diameter()方法

print("\n访问面积（自动计算）：")
print(f"面积：{circle.area}")  # 调用area()方法

# ============================================
# 第二部分：setter和deleter
# ============================================

print("\n" + "=" * 60)
print("第二部分：setter和deleter")
print("=" * 60)

print("""
【@property.setter装饰器】

作用：
- 设置属性值
- 可以添加验证逻辑
- 替代setter方法

语法：
@属性名.setter
def 属性名(self, value):
    # 验证和设置逻辑
    pass

【@property.deleter装饰器】

作用：
- 删除属性
- 清理资源
- 执行清理逻辑

语法：
@属性名.deleter
def 属性名(self):
    # 清理逻辑
    pass
""")

# 示例1：完整的属性（getter + setter + deleter）
print("\n示例1：完整的属性（getter + setter + deleter）")
print("-" * 60)

class Person:
    """人类：演示完整的属性"""

    def __init__(self, name, age):
        """初始化人

        参数：
            name: 姓名
            age: 年龄
        """
        self._name = name
        self._age = age

    # getter：获取姓名
    @property
    def name(self):
        """获取姓名"""
        print("  [获取姓名]")
        return self._name

    # setter：设置姓名
    @name.setter
    def name(self, value):
        """设置姓名

        参数：
            value: 新姓名
        """
        print(f"  [设置姓名：{value}]")
        if not value:
            raise ValueError("姓名不能为空")
        self._name = value

    # deleter：删除姓名
    @name.deleter
    def name(self):
        """删除姓名"""
        print("  [删除姓名]")
        del self._name

    # getter：获取年龄
    @property
    def age(self):
        """获取年龄"""
        print("  [获取年龄]")
        return self._age

    # setter：设置年龄（带验证）
    @age.setter
    def age(self, value):
        """设置年龄

        参数：
            value: 新年龄
        """
        print(f"  [设置年龄：{value}]")
        if not isinstance(value, int):
            raise TypeError("年龄必须是整数")
        if value < 0 or value > 150:
            raise ValueError("年龄必须在0-150之间")
        self._age = value

# 测试完整属性
print("创建人：")
person = Person("张三", 25)

print("\n获取属性：")
print(f"姓名：{person.name}")
print(f"年龄：{person.age}")

print("\n修改属性：")
person.name = "李四"
print(f"新姓名：{person.name}")

person.age = 30
print(f"新年龄：{person.age}")

print("\n测试验证：")
try:
    person.age = -5  # 无效年龄
except ValueError as e:
    print(f"错误：{e}")

try:
    person.age = "abc"  # 错误类型
except TypeError as e:
    print(f"错误：{e}")

# ============================================
# 第三部分：只读属性
# ============================================

print("\n" + "=" * 60)
print("第三部分：只读属性")
print("=" * 60)

print("""
【只读属性】

是什么？
- 只定义@property（getter）
- 不定义setter
- 属性不可修改

为什么需要？
- 保护数据
- 计算属性
- 派生属性

使用场景：
- 根据其他属性计算的值
- 不应该被外部修改的属性
- 系统生成的属性
""")

# 示例1：只读属性
print("\n示例1：只读属性")
print("-" * 60)

class Rectangle:
    """矩形类：演示只读属性"""

    def __init__(self, width, height):
        """初始化矩形

        参数：
            width: 宽度
            height: 高度
        """
        self._width = width
        self._height = height

    # 可读写属性：宽度
    @property
    def width(self):
        """获取宽度"""
        return self._width

    @width.setter
    def width(self, value):
        """设置宽度"""
        if value <= 0:
            raise ValueError("宽度必须大于0")
        self._width = value

    # 可读写属性：高度
    @property
    def height(self):
        """获取高度"""
        return self._height

    @height.setter
    def height(self, value):
        """设置高度"""
        if value <= 0:
            raise ValueError("高度必须大于0")
        self._height = value

    # 只读属性：面积（根据宽度和高度计算）
    @property
    def area(self):
        """计算面积（只读）"""
        return self._width * self._height

    # 只读属性：周长（根据宽度和高度计算）
    @property
    def perimeter(self):
        """计算周长（只读）"""
        return 2 * (self._width + self._height)

    # 只读属性：是否为正方形
    @property
    def is_square(self):
        """判断是否为正方形（只读）"""
        return self._width == self._height

# 测试只读属性
print("创建矩形：")
rect = Rectangle(10, 5)

print("\n访问属性：")
print(f"宽度：{rect.width}")
print(f"高度：{rect.height}")
print(f"面积：{rect.area}")  # 只读
print(f"周长：{rect.perimeter}")  # 只读
print(f"是否为正方形：{rect.is_square}")  # 只读

print("\n修改宽度和高度：")
rect.width = 8
rect.height = 8
print(f"新宽度：{rect.width}")
print(f"新高度：{rect.height}")
print(f"新面积：{rect.area}")  # 自动更新
print(f"是否为正方形：{rect.is_square}")  # 自动更新

print("\n尝试修改只读属性：")
try:
    rect.area = 100  # 错误！area是只读属性
except AttributeError as e:
    print(f"错误：{e}")

# ============================================
# 第四部分：属性验证
# ============================================

print("\n" + "=" * 60)
print("第四部分：属性验证")
print("=" * 60)

print("""
【属性验证】

为什么需要？
- 确保数据有效性
- 防止无效输入
- 提供友好的错误提示

验证类型：
1. 类型验证：检查数据类型
2. 范围验证：检查数值范围
3. 格式验证：检查字符串格式
4. 逻辑验证：检查业务逻辑

实现方式：
- 在setter中添加验证逻辑
- 验证失败时抛出异常
- 提供清晰的错误信息
""")

# 示例1：完整的属性验证
print("\n示例1：完整的属性验证")
print("-" * 60)

class BankAccount:
    """银行账户类：演示属性验证"""

    def __init__(self, account_number, owner, balance=0):
        """初始化账户

        参数：
            account_number: 账号
            owner: 账户所有者
            balance: 初始余额（默认0）
        """
        self.account_number = account_number  # 使用setter验证
        self.owner = owner  # 使用setter验证
        self.balance = balance  # 使用setter验证

    # 账号属性（只读）
    @property
    def account_number(self):
        """获取账号"""
        return self._account_number

    @account_number.setter
    def account_number(self, value):
        """设置账号（带验证）

        验证规则：
        - 必须是字符串
        - 长度为10位
        - 只包含数字
        """
        if not isinstance(value, str):
            raise TypeError("账号必须是字符串")
        if len(value) != 10:
            raise ValueError("账号必须是10位")
        if not value.isdigit():
            raise ValueError("账号只能包含数字")
        self._account_number = value

    # 所有者属性
    @property
    def owner(self):
        """获取所有者"""
        return self._owner

    @owner.setter
    def owner(self, value):
        """设置所有者（带验证）

        验证规则：
        - 必须是字符串
        - 不能为空
        - 长度在2-20之间
        """
        if not isinstance(value, str):
            raise TypeError("所有者姓名必须是字符串")
        if not value.strip():
            raise ValueError("所有者姓名不能为空")
        if len(value) < 2 or len(value) > 20:
            raise ValueError("所有者姓名长度必须在2-20之间")
        self._owner = value

    # 余额属性
    @property
    def balance(self):
        """获取余额"""
        return self._balance

    @balance.setter
    def balance(self, value):
        """设置余额（带验证）

        验证规则：
        - 必须是数字（int或float）
        - 不能为负数
        """
        if not isinstance(value, (int, float)):
            raise TypeError("余额必须是数字")
        if value < 0:
            raise ValueError("余额不能为负数")
        self._balance = value

    def deposit(self, amount):
        """存款

        参数：
            amount: 存款金额
        """
        if amount <= 0:
            raise ValueError("存款金额必须大于0")
        self.balance += amount  # 使用setter验证
        print(f"存款成功：{amount} 元，当前余额：{self.balance} 元")

    def withdraw(self, amount):
        """取款

        参数：
            amount: 取款金额
        """
        if amount <= 0:
            raise ValueError("取款金额必须大于0")
        if amount > self.balance:
            raise ValueError("余额不足")
        self.balance -= amount  # 使用setter验证
        print(f"取款成功：{amount} 元，当前余额：{self.balance} 元")

    def __str__(self):
        return f"账号：{self.account_number}，所有者：{self.owner}，余额：{self.balance} 元"

# 测试属性验证
print("测试有效账户：")
try:
    account = BankAccount("1234567890", "张三", 1000)
    print(f"创建成功：{account}")
except (TypeError, ValueError) as e:
    print(f"创建失败：{e}")

print("\n测试无效账号：")
try:
    account = BankAccount("123", "张三", 1000)  # 账号太短
except ValueError as e:
    print(f"错误：{e}")

print("\n测试无效所有者：")
try:
    account = BankAccount("1234567890", "", 1000)  # 所有者为空
except ValueError as e:
    print(f"错误：{e}")

print("\n测试无效余额：")
try:
    account = BankAccount("1234567890", "张三", -100)  # 余额为负
except ValueError as e:
    print(f"错误：{e}")

print("\n测试存取款：")
account = BankAccount("1234567890", "张三", 1000)
account.deposit(500)
account.withdraw(300)

print("\n测试余额不足：")
try:
    account.withdraw(2000)  # 余额不足
except ValueError as e:
    print(f"错误：{e}")

# ============================================
# 第五部分：property()函数
# ============================================

print("\n" + "=" * 60)
print("第五部分：property()函数")
print("=" * 60)

print("""
【property()函数】

是什么？
- property装饰器的函数形式
- 更传统的写法
- 功能完全相同

语法：
属性名 = property(fget, fset, fdel, doc)

参数：
- fget: getter函数
- fset: setter函数（可选）
- fdel: deleter函数（可选）
- doc: 文档字符串（可选）

对比：
装饰器形式（推荐）：
@property
def name(self):
    return self._name

@name.setter
def name(self, value):
    self._name = value

函数形式（传统）：
def get_name(self):
    return self._name

def set_name(self, value):
    self._name = value

name = property(get_name, set_name)
""")

# 示例：property()函数形式
print("\n示例：property()函数形式")
print("-" * 60)

class Temperature:
    """温度类：演示property()函数形式"""

    def __init__(self, celsius):
        """初始化温度

        参数：
            celsius: 摄氏温度
        """
        self._celsius = celsius

    # 使用property()函数定义属性
    def get_celsius(self):
        """获取摄氏温度"""
        return self._celsius

    def set_celsius(self, value):
        """设置摄氏温度"""
        if value < -273.15:
            raise ValueError("温度不能低于绝对零度（-273.15°C）")
        self._celsius = value

    def del_celsius(self):
        """删除摄氏温度"""
        del self._celsius

    # 使用property()函数创建属性
    celsius = property(get_celsius, set_celsius, del_celsius, "摄氏温度")

    # 华氏温度（只读，使用装饰器形式）
    @property
    def fahrenheit(self):
        """获取华氏温度（只读）"""
        return self._celsius * 9/5 + 32

# 测试property()函数
print("创建温度对象：")
temp = Temperature(25)
print(f"摄氏温度：{temp.celsius}°C")
print(f"华氏温度：{temp.fahrenheit}°F")

print("\n修改温度：")
temp.celsius = 30
print(f"新摄氏温度：{temp.celsius}°C")
print(f"新华氏温度：{temp.fahrenheit}°F")

# ============================================
# 第六部分：常见错误和陷阱
# ============================================

print("\n" + "=" * 60)
print("第六部分：常见错误和陷阱")
print("=" * 60)

print("""
【常见错误】

1. 忘记使用下划线命名内部属性
   ❌ 错误：
   @property
   def name(self):
       return self.name  # 无限递归！

   ✅ 正确：
   @property
   def name(self):
       return self._name  # 使用下划线

2. setter装饰器名称错误
   ❌ 错误：
   @property
   def name(self):
       return self._name

   @name_setter  # 错误！应该是@name.setter
   def name(self, value):
       self._name = value

   ✅ 正确：
   @name.setter  # 正确
   def name(self, value):
       self._name = value

3. 在__init__中直接设置内部属性
   ❌ 不好：
   def __init__(self, age):
       self._age = age  # 绕过了setter验证

   ✅ 更好：
   def __init__(self, age):
       self.age = age  # 使用setter验证

4. 只读属性尝试设置setter
   ❌ 错误：
   @property
   def area(self):
       return self.width * self.height

   @area.setter  # 错误！area应该是只读的
   def area(self, value):
       pass

   ✅ 正确：
   @property
   def area(self):
       return self.width * self.height
   # 不定义setter，保持只读

5. 验证逻辑不完整
   ❌ 不好：
   @age.setter
   def age(self, value):
       if value < 0:
           raise ValueError("年龄不能为负")
       self._age = value  # 没有检查类型和上限

   ✅ 更好：
   @age.setter
   def age(self, value):
       if not isinstance(value, int):
           raise TypeError("年龄必须是整数")
       if value < 0 or value > 150:
           raise ValueError("年龄必须在0-150之间")
       self._age = value

【最佳实践】

1. 使用下划线命名内部属性
   - self._name 表示内部属性
   - self.name 是对外的属性接口

2. 在__init__中使用属性（而不是内部属性）
   - 确保验证逻辑被执行
   - 保持一致性

3. 只读属性不定义setter
   - 计算属性应该是只读的
   - 派生属性应该是只读的

4. 验证逻辑要完整
   - 检查类型
   - 检查范围
   - 检查格式
   - 提供清晰的错误信息

5. 文档字符串
   - 说明属性的作用
   - 说明验证规则
   - 说明可能的异常
""")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结")
print("=" * 60)

print("""
【本节重点】

1. @property装饰器
   - 将方法转换为属性
   - 提供优雅的属性访问方式
   - 替代getter方法

2. @属性名.setter装饰器
   - 设置属性值
   - 添加验证逻辑
   - 替代setter方法

3. @属性名.deleter装饰器
   - 删除属性
   - 清理资源

4. 只读属性
   - 只定义@property
   - 不定义setter
   - 用于计算属性和派生属性

5. 属性验证
   - 在setter中添加验证逻辑
   - 确保数据有效性
   - 提供友好的错误提示

6. property()函数
   - 传统的函数形式
   - 功能与装饰器相同
   - 推荐使用装饰器形式

【下一步】

- 学习魔法方法
- 完成综合练习
- 设计优雅的类接口

记住：
- @property让属性访问更优雅
- setter中添加验证逻辑
- 只读属性用于计算和派生
- 保持接口简洁和一致
""")
