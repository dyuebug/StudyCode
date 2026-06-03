# ============================================
# 类方法和静态方法详解
# 日期：2026-06-04（第18天）
# 难度：⭐⭐⭐⭐ 面向对象高级特性
# ============================================

"""
【学习目标】

通过本节学习，你将掌握：
1. 实例方法、类方法、静态方法的区别
2. @classmethod装饰器的使用
3. @staticmethod装饰器的使用
4. 三种方法的使用场景
5. 工厂方法模式

【为什么需要类方法和静态方法？】

在面向对象编程中，我们有三种方法：
- 实例方法：操作实例数据（最常用）
- 类方法：操作类数据，或作为工厂方法
- 静态方法：不操作实例或类数据，但逻辑上属于这个类

合理使用这三种方法，可以让代码更加清晰和优雅。
"""

# ============================================
# 第一部分：三种方法的概念
# ============================================

print("=" * 60)
print("第一部分：三种方法的概念")
print("=" * 60)

print("""
【三种方法的定义】

1. 实例方法（Instance Method）
   - 第一个参数是 self（实例本身）
   - 可以访问实例属性和类属性
   - 通过实例调用：obj.method()
   - 最常用的方法类型

2. 类方法（Class Method）
   - 使用 @classmethod 装饰器
   - 第一个参数是 cls（类本身）
   - 可以访问类属性，不能访问实例属性
   - 可以通过类或实例调用：Class.method() 或 obj.method()
   - 常用于工厂方法

3. 静态方法（Static Method）
   - 使用 @staticmethod 装饰器
   - 没有 self 或 cls 参数
   - 不能访问实例属性或类属性
   - 可以通过类或实例调用：Class.method() 或 obj.method()
   - 常用于工具函数

【三种方法的对比】

| 特性 | 实例方法 | 类方法 | 静态方法 |
|------|---------|--------|---------|
| 装饰器 | 无 | @classmethod | @staticmethod |
| 第一个参数 | self | cls | 无 |
| 访问实例属性 | ✅ | ❌ | ❌ |
| 访问类属性 | ✅ | ✅ | ❌ |
| 调用方式 | obj.method() | Class.method() 或 obj.method() | Class.method() 或 obj.method() |
| 使用场景 | 操作实例数据 | 操作类数据、工厂方法 | 工具函数 |
""")

# 示例：三种方法的定义
print("\n示例：三种方法的定义")
print("-" * 60)

class MyClass:
    """演示三种方法的类"""

    class_variable = "我是类属性"  # 类属性

    def __init__(self, value):
        self.instance_variable = value  # 实例属性

    # 实例方法
    def instance_method(self):
        """实例方法：可以访问实例属性和类属性"""
        print(f"实例方法被调用")
        print(f"  实例属性：{self.instance_variable}")
        print(f"  类属性：{self.class_variable}")
        print(f"  可以访问 self 和类属性")

    # 类方法
    @classmethod
    def class_method(cls):
        """类方法：只能访问类属性"""
        print(f"类方法被调用")
        print(f"  类属性：{cls.class_variable}")
        print(f"  可以访问 cls，但不能访问实例属性")
        # print(self.instance_variable)  # 错误！没有 self

    # 静态方法
    @staticmethod
    def static_method():
        """静态方法：不能访问实例属性或类属性"""
        print(f"静态方法被调用")
        print(f"  不能访问 self 或 cls")
        print(f"  只能使用传入的参数或全局变量")
        # print(self.instance_variable)  # 错误！没有 self
        # print(cls.class_variable)  # 错误！没有 cls

# 测试三种方法
print("\n测试三种方法：")
obj = MyClass("实例值")

print("\n1. 调用实例方法：")
obj.instance_method()

print("\n2. 调用类方法（通过类）：")
MyClass.class_method()

print("\n3. 调用类方法（通过实例）：")
obj.class_method()

print("\n4. 调用静态方法（通过类）：")
MyClass.static_method()

print("\n5. 调用静态方法（通过实例）：")
obj.static_method()

# ============================================
# 第二部分：类方法详解
# ============================================

print("\n" + "=" * 60)
print("第二部分：类方法详解")
print("=" * 60)

print("""
【类方法的特点】

1. 使用 @classmethod 装饰器
2. 第一个参数是 cls（类本身）
3. 可以访问类属性
4. 不能访问实例属性
5. 可以创建类的实例（工厂方法）

【类方法的使用场景】

1. 工厂方法（Factory Method）
   - 提供多种创建实例的方式
   - 比 __init__ 更灵活

2. 操作类属性
   - 统计实例数量
   - 修改类级别的配置

3. 替代构造函数
   - 从不同格式的数据创建实例
   - 例如：从字符串、从字典、从文件等
""")

# 示例1：工厂方法
print("\n示例1：工厂方法")
print("-" * 60)

class Date:
    """日期类：演示工厂方法"""

    def __init__(self, year, month, day):
        """标准构造函数"""
        self.year = year
        self.month = month
        self.day = day

    @classmethod
    def from_string(cls, date_string):
        """工厂方法：从字符串创建日期对象

        参数：
            date_string: 格式为 "YYYY-MM-DD" 的字符串

        返回：
            Date对象
        """
        year, month, day = map(int, date_string.split('-'))
        return cls(year, month, day)  # 调用 __init__

    @classmethod
    def today(cls):
        """工厂方法：创建今天的日期对象"""
        # 这里简化处理，实际应该使用 datetime 模块
        return cls(2026, 6, 4)

    @classmethod
    def from_timestamp(cls, timestamp):
        """工厂方法：从时间戳创建日期对象"""
        # 这里简化处理
        return cls(2026, 1, 1)

    def __str__(self):
        return f"{self.year}-{self.month:02d}-{self.day:02d}"

# 测试工厂方法
print("使用不同的工厂方法创建日期对象：")

# 方式1：标准构造函数
date1 = Date(2026, 6, 4)
print(f"标准构造：{date1}")

# 方式2：从字符串创建
date2 = Date.from_string("2026-12-25")
print(f"从字符串：{date2}")

# 方式3：创建今天的日期
date3 = Date.today()
print(f"今天日期：{date3}")

# 方式4：从时间戳创建
date4 = Date.from_timestamp(1234567890)
print(f"从时间戳：{date4}")

# 示例2：统计实例数量
print("\n示例2：统计实例数量")
print("-" * 60)

class Person:
    """人类：演示类属性和类方法"""

    count = 0  # 类属性：统计实例数量

    def __init__(self, name):
        self.name = name
        Person.count += 1  # 每创建一个实例，计数加1

    @classmethod
    def get_count(cls):
        """类方法：获取实例数量"""
        return cls.count

    @classmethod
    def reset_count(cls):
        """类方法：重置计数"""
        cls.count = 0

# 测试实例计数
print(f"初始人数：{Person.get_count()}")

p1 = Person("张三")
print(f"创建张三后：{Person.get_count()}")

p2 = Person("李四")
print(f"创建李四后：{Person.get_count()}")

p3 = Person("王五")
print(f"创建王五后：{Person.get_count()}")

Person.reset_count()
print(f"重置后：{Person.get_count()}")

# ============================================
# 第三部分：静态方法详解
# ============================================

print("\n" + "=" * 60)
print("第三部分：静态方法详解")
print("=" * 60)

print("""
【静态方法的特点】

1. 使用 @staticmethod 装饰器
2. 没有 self 或 cls 参数
3. 不能访问实例属性或类属性
4. 只能使用传入的参数
5. 逻辑上属于这个类，但不依赖类或实例的状态

【静态方法的使用场景】

1. 工具函数
   - 验证数据格式
   - 数据转换
   - 辅助计算

2. 不需要访问类或实例数据的函数
   - 逻辑上属于这个类
   - 但不依赖类的状态

3. 命名空间
   - 将相关的函数组织在一起
   - 避免全局函数污染
""")

# 示例1：验证函数
print("\n示例1：验证函数")
print("-" * 60)

class BankAccount:
    """银行账户类：演示静态方法"""

    def __init__(self, account_number, balance):
        # 验证账号格式
        if not self.is_valid_account_number(account_number):
            raise ValueError("账号格式错误")

        self.account_number = account_number
        self.balance = balance

    @staticmethod
    def is_valid_account_number(account_number):
        """静态方法：验证账号格式

        账号规则：
        - 长度为10位
        - 只包含数字
        """
        if len(account_number) != 10:
            return False
        if not account_number.isdigit():
            return False
        return True

    @staticmethod
    def format_currency(amount):
        """静态方法：格式化货币"""
        return f"¥{amount:,.2f}"

    def __str__(self):
        return f"账号：{self.account_number}，余额：{self.format_currency(self.balance)}"

# 测试静态方法
print("测试账号验证：")
print(f"1234567890 是否有效：{BankAccount.is_valid_account_number('1234567890')}")
print(f"123 是否有效：{BankAccount.is_valid_account_number('123')}")
print(f"12345abcde 是否有效：{BankAccount.is_valid_account_number('12345abcde')}")

print("\n测试货币格式化：")
print(f"1000 格式化：{BankAccount.format_currency(1000)}")
print(f"1234567.89 格式化：{BankAccount.format_currency(1234567.89)}")

print("\n创建账户：")
try:
    account1 = BankAccount("1234567890", 1000)
    print(f"成功：{account1}")
except ValueError as e:
    print(f"失败：{e}")

try:
    account2 = BankAccount("123", 1000)
    print(f"成功：{account2}")
except ValueError as e:
    print(f"失败：{e}")

# 示例2：数学工具类
print("\n示例2：数学工具类")
print("-" * 60)

class MathUtils:
    """数学工具类：演示静态方法作为工具函数"""

    @staticmethod
    def is_prime(n):
        """判断是否为质数"""
        if n < 2:
            return False
        for i in range(2, int(n ** 0.5) + 1):
            if n % i == 0:
                return False
        return True

    @staticmethod
    def factorial(n):
        """计算阶乘"""
        if n <= 1:
            return 1
        return n * MathUtils.factorial(n - 1)

    @staticmethod
    def gcd(a, b):
        """计算最大公约数"""
        while b:
            a, b = b, a % b
        return a

# 测试数学工具
print("测试质数判断：")
for num in [2, 3, 4, 5, 10, 17]:
    print(f"  {num} 是质数：{MathUtils.is_prime(num)}")

print("\n测试阶乘：")
for num in [0, 1, 5, 10]:
    print(f"  {num}! = {MathUtils.factorial(num)}")

print("\n测试最大公约数：")
print(f"  gcd(12, 18) = {MathUtils.gcd(12, 18)}")
print(f"  gcd(100, 50) = {MathUtils.gcd(100, 50)}")

# ============================================
# 第四部分：三种方法的选择
# ============================================

print("\n" + "=" * 60)
print("第四部分：三种方法的选择")
print("=" * 60)

print("""
【如何选择使用哪种方法？】

1. 需要访问实例属性？
   → 使用实例方法

2. 需要访问类属性，或创建实例？
   → 使用类方法

3. 不需要访问实例或类属性，但逻辑上属于这个类？
   → 使用静态方法

4. 完全独立的函数，与类无关？
   → 使用普通函数（不放在类里）

【决策流程图】

需要访问实例数据？
├─ 是 → 实例方法
└─ 否 → 需要访问类数据或创建实例？
        ├─ 是 → 类方法
        └─ 否 → 逻辑上属于这个类？
                ├─ 是 → 静态方法
                └─ 否 → 普通函数
""")

# 综合示例：完整的类设计
print("\n综合示例：完整的类设计")
print("-" * 60)

class Employee:
    """员工类：综合演示三种方法"""

    # 类属性
    company_name = "ABC公司"
    employee_count = 0

    def __init__(self, name, salary):
        """实例方法：初始化"""
        self.name = name
        self.salary = salary
        Employee.employee_count += 1

    # 实例方法：操作实例数据
    def get_annual_salary(self):
        """计算年薪"""
        return self.salary * 12

    def give_raise(self, amount):
        """加薪"""
        self.salary += amount
        print(f"{self.name} 加薪 {amount} 元，新工资：{self.salary} 元")

    # 类方法：工厂方法
    @classmethod
    def from_string(cls, emp_string):
        """从字符串创建员工对象

        格式："姓名,工资"
        """
        name, salary = emp_string.split(',')
        return cls(name, int(salary))

    # 类方法：操作类属性
    @classmethod
    def get_employee_count(cls):
        """获取员工数量"""
        return cls.employee_count

    @classmethod
    def set_company_name(cls, name):
        """设置公司名称"""
        cls.company_name = name

    # 静态方法：工具函数
    @staticmethod
    def is_valid_salary(salary):
        """验证工资是否有效"""
        return salary > 0 and salary < 1000000

    @staticmethod
    def calculate_tax(salary):
        """计算个人所得税（简化版）"""
        if salary <= 5000:
            return 0
        elif salary <= 8000:
            return (salary - 5000) * 0.03
        elif salary <= 17000:
            return (salary - 8000) * 0.1 + 90
        else:
            return (salary - 17000) * 0.2 + 990

    def __str__(self):
        return f"{self.name}（{self.company_name}），工资：{self.salary} 元"

# 测试综合示例
print("测试员工类：")

# 使用标准构造函数
emp1 = Employee("张三", 8000)
print(f"1. {emp1}")

# 使用工厂方法
emp2 = Employee.from_string("李四,12000")
print(f"2. {emp2}")

# 调用实例方法
print(f"\n3. {emp1.name} 的年薪：{emp1.get_annual_salary()} 元")
emp1.give_raise(2000)

# 调用类方法
print(f"\n4. 员工总数：{Employee.get_employee_count()}")
Employee.set_company_name("XYZ公司")
print(f"5. 公司名称已更改为：{Employee.company_name}")

# 调用静态方法
print(f"\n6. 工资 8000 是否有效：{Employee.is_valid_salary(8000)}")
print(f"7. 工资 8000 的税：{Employee.calculate_tax(8000):.2f} 元")
print(f"8. 工资 12000 的税：{Employee.calculate_tax(12000):.2f} 元")

# ============================================
# 第五部分：常见错误和陷阱
# ============================================

print("\n" + "=" * 60)
print("第五部分：常见错误和陷阱")
print("=" * 60)

print("""
【常见错误】

1. 在静态方法中访问实例或类属性
   ❌ 错误：
   @staticmethod
   def method():
       print(self.value)  # 错误！没有 self
       print(cls.value)   # 错误！没有 cls

   ✅ 正确：
   @staticmethod
   def method(value):
       print(value)  # 使用参数

2. 忘记装饰器
   ❌ 错误：
   def class_method(cls):  # 忘记 @classmethod
       print(cls.value)

   ✅ 正确：
   @classmethod
   def class_method(cls):
       print(cls.value)

3. 类方法中使用 self
   ❌ 错误：
   @classmethod
   def method(cls):
       print(self.value)  # 错误！应该用 cls

   ✅ 正确：
   @classmethod
   def method(cls):
       print(cls.value)

4. 静态方法应该是实例方法
   ❌ 不好：
   @staticmethod
   def get_name(name):  # 应该是实例方法
       return name

   ✅ 更好：
   def get_name(self):
       return self.name

5. 过度使用静态方法
   - 如果一个类全是静态方法，考虑使用模块级函数
   - 静态方法应该逻辑上属于这个类

【最佳实践】

1. 优先使用实例方法
   - 大多数情况下使用实例方法
   - 只在必要时使用类方法和静态方法

2. 类方法用于工厂方法
   - 提供多种创建实例的方式
   - 比 __init__ 更灵活

3. 静态方法用于工具函数
   - 验证、转换、计算等
   - 逻辑上属于这个类

4. 保持方法简单
   - 每个方法只做一件事
   - 方法名清晰表达功能

5. 合理组织代码
   - 相关的方法放在一起
   - 使用注释分组
""")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结")
print("=" * 60)

print("""
【本节重点】

1. 三种方法的区别
   - 实例方法：操作实例数据，第一个参数是 self
   - 类方法：操作类数据或工厂方法，第一个参数是 cls，使用 @classmethod
   - 静态方法：工具函数，没有 self 或 cls，使用 @staticmethod

2. 使用场景
   - 实例方法：需要访问实例属性
   - 类方法：需要访问类属性，或作为工厂方法
   - 静态方法：不需要访问实例或类属性，但逻辑上属于这个类

3. 工厂方法模式
   - 使用类方法提供多种创建实例的方式
   - 比 __init__ 更灵活

4. 选择原则
   - 优先使用实例方法
   - 必要时使用类方法和静态方法
   - 保持代码简单清晰

【下一步】

- 学习属性装饰器（@property）
- 学习魔法方法
- 完成综合练习

记住：
- 理解三种方法的区别和使用场景
- 合理选择方法类型
- 让代码更加清晰和优雅
""")
