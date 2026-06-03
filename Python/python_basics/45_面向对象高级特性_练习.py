# ============================================
# 面向对象高级特性综合练习
# 日期：2026-06-04（第18天）
# 难度：⭐⭐⭐⭐⭐ 综合实战
# ============================================

"""
【练习目标】

通过本练习，你将综合运用：
1. 类方法和静态方法
2. @property装饰器
3. 魔法方法
4. 面向对象设计思想

【练习项目】

项目1：日期类（Date）
- 使用类方法作为工厂方法
- 使用@property实现属性访问
- 实现魔法方法（__str__、__repr__、__eq__、__lt__等）
- 实现日期比较和运算

项目2：向量类（Vector）
- 实现向量的加减乘除
- 实现向量的长度和点积
- 使用魔法方法重载运算符
- 使用@property实现只读属性

项目3：银行账户类（增强版）
- 使用@property实现余额访问
- 使用类方法统计账户数量
- 使用静态方法验证账号格式
- 实现魔法方法（__str__、__repr__）
"""

# ============================================
# 项目1：日期类（Date）
# ============================================

print("=" * 60)
print("项目1：日期类（Date）")
print("=" * 60)

class Date:
    """日期类：综合演示面向对象高级特性"""

    # 类属性：月份天数
    _days_in_month = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

    def __init__(self, year, month, day):
        """初始化日期

        参数：
            year: 年份
            month: 月份（1-12）
            day: 日期（1-31）
        """
        # 使用属性setter进行验证
        self.year = year
        self.month = month
        self.day = day

    # ========== 属性（@property） ==========

    @property
    def year(self):
        """获取年份"""
        return self._year

    @year.setter
    def year(self, value):
        """设置年份（带验证）"""
        if not isinstance(value, int):
            raise TypeError("年份必须是整数")
        if value < 1:
            raise ValueError("年份必须大于0")
        self._year = value

    @property
    def month(self):
        """获取月份"""
        return self._month

    @month.setter
    def month(self, value):
        """设置月份（带验证）"""
        if not isinstance(value, int):
            raise TypeError("月份必须是整数")
        if value < 1 or value > 12:
            raise ValueError("月份必须在1-12之间")
        self._month = value

    @property
    def day(self):
        """获取日期"""
        return self._day

    @day.setter
    def day(self, value):
        """设置日期（带验证）"""
        if not isinstance(value, int):
            raise TypeError("日期必须是整数")
        max_day = self._get_days_in_month(self._year, self._month)
        if value < 1 or value > max_day:
            raise ValueError(f"日期必须在1-{max_day}之间")
        self._day = value

    # ========== 类方法（工厂方法） ==========

    @classmethod
    def from_string(cls, date_string):
        """从字符串创建日期对象

        参数：
            date_string: 格式为 "YYYY-MM-DD" 的字符串

        返回：
            Date对象

        示例：
            date = Date.from_string("2026-06-04")
        """
        try:
            year, month, day = map(int, date_string.split('-'))
            return cls(year, month, day)
        except ValueError:
            raise ValueError("日期字符串格式错误，应为 YYYY-MM-DD")

    @classmethod
    def today(cls):
        """创建今天的日期对象

        返回：
            Date对象（简化版，实际应使用datetime模块）
        """
        # 这里简化处理，实际应该使用 datetime.date.today()
        return cls(2026, 6, 4)

    @classmethod
    def from_timestamp(cls, timestamp):
        """从时间戳创建日期对象

        参数：
            timestamp: Unix时间戳

        返回：
            Date对象（简化版）
        """
        # 这里简化处理，实际应该使用 datetime.fromtimestamp()
        return cls(2026, 1, 1)

    # ========== 静态方法（工具函数） ==========

    @staticmethod
    def is_leap_year(year):
        """判断是否为闰年

        参数：
            year: 年份

        返回：
            True 如果是闰年

        规则：
            - 能被4整除但不能被100整除
            - 或能被400整除
        """
        return (year % 4 == 0 and year % 100 != 0) or (year % 400 == 0)

    @staticmethod
    def _get_days_in_month(year, month):
        """获取指定月份的天数

        参数：
            year: 年份
            month: 月份

        返回：
            该月的天数
        """
        days = Date._days_in_month[month]
        # 如果是2月且是闰年，返回29天
        if month == 2 and Date.is_leap_year(year):
            return 29
        return days

    # ========== 魔法方法 ==========

    def __str__(self):
        """用户友好的字符串表示"""
        return f"{self._year}-{self._month:02d}-{self._day:02d}"

    def __repr__(self):
        """开发者友好的字符串表示"""
        return f"Date({self._year}, {self._month}, {self._day})"

    def __eq__(self, other):
        """相等比较：date1 == date2"""
        if not isinstance(other, Date):
            return False
        return (self._year == other._year and
                self._month == other._month and
                self._day == other._day)

    def __ne__(self, other):
        """不等比较：date1 != date2"""
        return not self.__eq__(other)

    def __lt__(self, other):
        """小于比较：date1 < date2"""
        if not isinstance(other, Date):
            raise TypeError("只能与Date比较")
        if self._year != other._year:
            return self._year < other._year
        if self._month != other._month:
            return self._month < other._month
        return self._day < other._day

    def __le__(self, other):
        """小于等于比较：date1 <= date2"""
        return self == other or self < other

    def __gt__(self, other):
        """大于比较：date1 > date2"""
        return not self <= other

    def __ge__(self, other):
        """大于等于比较：date1 >= date2"""
        return not self < other

    def __hash__(self):
        """返回哈希值（用于字典和集合）"""
        return hash((self._year, self._month, self._day))

# 测试日期类
print("\n测试日期类：")
print("-" * 60)

print("1. 创建日期对象：")
date1 = Date(2026, 6, 4)
print(f"   date1 = {date1}")

print("\n2. 使用工厂方法：")
date2 = Date.from_string("2026-12-25")
print(f"   从字符串：{date2}")

date3 = Date.today()
print(f"   今天：{date3}")

print("\n3. 测试属性验证：")
try:
    invalid_date = Date(2026, 13, 1)  # 无效月份
except ValueError as e:
    print(f"   错误：{e}")

print("\n4. 测试静态方法：")
print(f"   2024是闰年：{Date.is_leap_year(2024)}")
print(f"   2026是闰年：{Date.is_leap_year(2026)}")

print("\n5. 测试比较运算符：")
print(f"   date1 == date2: {date1 == date2}")
print(f"   date1 < date2: {date1 < date2}")
print(f"   date1 > date2: {date1 > date2}")

print("\n6. 测试排序：")
dates = [date2, date1, date3]
dates.sort()
print("   排序后：")
for d in dates:
    print(f"     {d}")

print("\n7. 测试哈希（用作字典键）：")
date_dict = {date1: "日期1", date2: "日期2"}
print(f"   date_dict[date1] = {date_dict[date1]}")

# ============================================
# 项目2：向量类（Vector）
# ============================================

print("\n" + "=" * 60)
print("项目2：向量类（Vector）")
print("=" * 60)

class Vector:
    """二维向量类：综合演示运算符重载和属性"""

    def __init__(self, x, y):
        """初始化向量

        参数：
            x: x坐标
            y: y坐标
        """
        self._x = x
        self._y = y

    # ========== 属性（只读） ==========

    @property
    def x(self):
        """获取x坐标（只读）"""
        return self._x

    @property
    def y(self):
        """获取y坐标（只读）"""
        return self._y

    @property
    def magnitude(self):
        """获取向量的模（长度）（只读）"""
        return (self._x ** 2 + self._y ** 2) ** 0.5

    # ========== 静态方法 ==========

    @staticmethod
    def dot_product(v1, v2):
        """计算两个向量的点积

        参数：
            v1: 向量1
            v2: 向量2

        返回：
            点积（标量）

        公式：
            v1 · v2 = v1.x * v2.x + v1.y * v2.y
        """
        if not isinstance(v1, Vector) or not isinstance(v2, Vector):
            raise TypeError("参数必须是Vector对象")
        return v1.x * v2.x + v1.y * v2.y

    @staticmethod
    def angle_between(v1, v2):
        """计算两个向量的夹角（弧度）

        参数：
            v1: 向量1
            v2: 向量2

        返回：
            夹角（弧度）

        公式：
            cos(θ) = (v1 · v2) / (|v1| * |v2|)
        """
        import math
        dot = Vector.dot_product(v1, v2)
        mag_product = v1.magnitude * v2.magnitude
        if mag_product == 0:
            raise ValueError("零向量没有夹角")
        return math.acos(dot / mag_product)

    # ========== 魔法方法（运算符重载） ==========

    def __add__(self, other):
        """向量加法：v1 + v2"""
        if not isinstance(other, Vector):
            raise TypeError("只能与Vector相加")
        return Vector(self._x + other._x, self._y + other._y)

    def __sub__(self, other):
        """向量减法：v1 - v2"""
        if not isinstance(other, Vector):
            raise TypeError("只能与Vector相减")
        return Vector(self._x - other._x, self._y - other._y)

    def __mul__(self, scalar):
        """向量数乘：v * scalar"""
        if not isinstance(scalar, (int, float)):
            raise TypeError("只能与数字相乘")
        return Vector(self._x * scalar, self._y * scalar)

    def __rmul__(self, scalar):
        """反向数乘：scalar * v"""
        return self.__mul__(scalar)

    def __truediv__(self, scalar):
        """向量除法：v / scalar"""
        if not isinstance(scalar, (int, float)):
            raise TypeError("只能除以数字")
        if scalar == 0:
            raise ValueError("不能除以0")
        return Vector(self._x / scalar, self._y / scalar)

    def __neg__(self):
        """向量取负：-v"""
        return Vector(-self._x, -self._y)

    def __abs__(self):
        """向量的模：abs(v)"""
        return self.magnitude

    def __eq__(self, other):
        """相等比较：v1 == v2"""
        if not isinstance(other, Vector):
            return False
        return self._x == other._x and self._y == other._y

    def __str__(self):
        """用户友好的字符串表示"""
        return f"({self._x}, {self._y})"

    def __repr__(self):
        """开发者友好的字符串表示"""
        return f"Vector({self._x}, {self._y})"

    def __len__(self):
        """返回向量维度（总是2）"""
        return 2

    def __getitem__(self, index):
        """索引访问：v[0] 返回 x，v[1] 返回 y"""
        if index == 0:
            return self._x
        elif index == 1:
            return self._y
        else:
            raise IndexError("向量索引超出范围（0-1）")

    def __iter__(self):
        """迭代器：支持 for x, y in vector"""
        return iter([self._x, self._y])

    def __hash__(self):
        """返回哈希值"""
        return hash((self._x, self._y))

# 测试向量类
print("\n测试向量类：")
print("-" * 60)

print("1. 创建向量：")
v1 = Vector(3, 4)
v2 = Vector(1, 2)
print(f"   v1 = {v1}")
print(f"   v2 = {v2}")

print("\n2. 测试只读属性：")
print(f"   v1.x = {v1.x}")
print(f"   v1.y = {v1.y}")
print(f"   v1.magnitude = {v1.magnitude}")

print("\n3. 测试运算符重载：")
print(f"   v1 + v2 = {v1 + v2}")
print(f"   v1 - v2 = {v1 - v2}")
print(f"   v1 * 2 = {v1 * 2}")
print(f"   3 * v1 = {3 * v1}")
print(f"   v1 / 2 = {v1 / 2}")
print(f"   -v1 = {-v1}")
print(f"   abs(v1) = {abs(v1)}")

print("\n4. 测试静态方法：")
print(f"   点积 v1 · v2 = {Vector.dot_product(v1, v2)}")
import math
angle = Vector.angle_between(v1, v2)
print(f"   夹角（弧度）= {angle:.4f}")
print(f"   夹角（度）= {math.degrees(angle):.2f}°")

print("\n5. 测试索引访问：")
print(f"   v1[0] = {v1[0]}")
print(f"   v1[1] = {v1[1]}")

print("\n6. 测试迭代：")
print("   解包：", end="")
x, y = v1
print(f"x={x}, y={y}")

print("\n7. 测试哈希（用作集合元素）：")
vector_set = {v1, v2, Vector(3, 4)}
print(f"   集合大小：{len(vector_set)}")  # v1 和 Vector(3, 4) 相等

# ============================================
# 项目3：银行账户类（增强版）
# ============================================

print("\n" + "=" * 60)
print("项目3：银行账户类（增强版）")
print("=" * 60)

class BankAccount:
    """银行账户类：综合演示类方法、静态方法、属性"""

    # 类属性：账户计数
    _account_count = 0

    def __init__(self, account_number, owner, balance=0):
        """初始化账户

        参数：
            account_number: 账号
            owner: 账户所有者
            balance: 初始余额（默认0）
        """
        self.account_number = account_number
        self.owner = owner
        self._balance = balance
        BankAccount._account_count += 1

    # ========== 属性 ==========

    @property
    def account_number(self):
        """获取账号（只读）"""
        return self._account_number

    @account_number.setter
    def account_number(self, value):
        """设置账号（带验证）"""
        if not self.is_valid_account_number(value):
            raise ValueError("账号格式错误（必须是10位数字）")
        self._account_number = value

    @property
    def owner(self):
        """获取所有者"""
        return self._owner

    @owner.setter
    def owner(self, value):
        """设置所有者（带验证）"""
        if not isinstance(value, str) or not value.strip():
            raise ValueError("所有者姓名不能为空")
        if len(value) < 2 or len(value) > 20:
            raise ValueError("所有者姓名长度必须在2-20之间")
        self._owner = value

    @property
    def balance(self):
        """获取余额（只读）"""
        return self._balance

    # ========== 类方法 ==========

    @classmethod
    def get_account_count(cls):
        """获取账户总数

        返回：
            账户数量
        """
        return cls._account_count

    @classmethod
    def reset_account_count(cls):
        """重置账户计数（用于测试）"""
        cls._account_count = 0

    # ========== 静态方法 ==========

    @staticmethod
    def is_valid_account_number(account_number):
        """验证账号格式

        参数：
            account_number: 账号

        返回：
            True 如果格式有效

        规则：
            - 必须是字符串
            - 长度为10位
            - 只包含数字
        """
        if not isinstance(account_number, str):
            return False
        if len(account_number) != 10:
            return False
        if not account_number.isdigit():
            return False
        return True

    @staticmethod
    def format_currency(amount):
        """格式化货币

        参数：
            amount: 金额

        返回：
            格式化的字符串
        """
        return f"¥{amount:,.2f}"

    # ========== 实例方法 ==========

    def deposit(self, amount):
        """存款

        参数：
            amount: 存款金额
        """
        if amount <= 0:
            raise ValueError("存款金额必须大于0")
        self._balance += amount
        return self._balance

    def withdraw(self, amount):
        """取款

        参数：
            amount: 取款金额
        """
        if amount <= 0:
            raise ValueError("取款金额必须大于0")
        if amount > self._balance:
            raise ValueError("余额不足")
        self._balance -= amount
        return self._balance

    def transfer(self, other, amount):
        """转账

        参数：
            other: 目标账户
            amount: 转账金额
        """
        if not isinstance(other, BankAccount):
            raise TypeError("只能向BankAccount转账")
        self.withdraw(amount)  # 可能抛出异常
        other.deposit(amount)

    # ========== 魔法方法 ==========

    def __str__(self):
        """用户友好的字符串表示"""
        return f"账号：{self._account_number}，所有者：{self._owner}，余额：{self.format_currency(self._balance)}"

    def __repr__(self):
        """开发者友好的字符串表示"""
        return f"BankAccount('{self._account_number}', '{self._owner}', {self._balance})"

    def __eq__(self, other):
        """相等比较（基于账号）"""
        if not isinstance(other, BankAccount):
            return False
        return self._account_number == other._account_number

    def __hash__(self):
        """返回哈希值（基于账号）"""
        return hash(self._account_number)

    def __lt__(self, other):
        """小于比较（基于余额）"""
        if not isinstance(other, BankAccount):
            raise TypeError("只能与BankAccount比较")
        return self._balance < other._balance

    def __bool__(self):
        """布尔值转换（余额是否大于0）"""
        return self._balance > 0

# 测试银行账户类
print("\n测试银行账户类：")
print("-" * 60)

print("1. 创建账户：")
account1 = BankAccount("1234567890", "张三", 1000)
account2 = BankAccount("9876543210", "李四", 2000)
print(f"   {account1}")
print(f"   {account2}")

print("\n2. 测试类方法：")
print(f"   账户总数：{BankAccount.get_account_count()}")

print("\n3. 测试静态方法：")
print(f"   验证账号 '1234567890'：{BankAccount.is_valid_account_number('1234567890')}")
print(f"   验证账号 '123'：{BankAccount.is_valid_account_number('123')}")
print(f"   格式化金额 1234567.89：{BankAccount.format_currency(1234567.89)}")

print("\n4. 测试存取款：")
account1.deposit(500)
print(f"   存款后：{account1}")
account1.withdraw(300)
print(f"   取款后：{account1}")

print("\n5. 测试转账：")
print(f"   转账前：")
print(f"     {account1}")
print(f"     {account2}")
account1.transfer(account2, 200)
print(f"   转账后：")
print(f"     {account1}")
print(f"     {account2}")

print("\n6. 测试比较运算符：")
print(f"   account1 < account2: {account1 < account2}")
print(f"   account1 == account2: {account1 == account2}")

print("\n7. 测试布尔值转换：")
empty_account = BankAccount("0000000000", "王五", 0)
print(f"   bool(account1): {bool(account1)}")
print(f"   bool(empty_account): {bool(empty_account)}")

print("\n8. 测试排序：")
accounts = [account2, account1, empty_account]
accounts.sort()  # 按余额排序
print("   按余额排序：")
for acc in accounts:
    print(f"     {acc}")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结")
print("=" * 60)

print("""
【练习总结】

通过这三个项目，我们综合运用了：

1. 类方法（@classmethod）
   - 工厂方法：Date.from_string()、Date.today()
   - 类属性操作：BankAccount.get_account_count()

2. 静态方法（@staticmethod）
   - 工具函数：Date.is_leap_year()、Vector.dot_product()
   - 验证函数：BankAccount.is_valid_account_number()

3. 属性装饰器（@property）
   - 只读属性：Vector.magnitude、BankAccount.balance
   - 可读写属性：Date.year、Date.month、Date.day
   - 属性验证：在setter中验证数据

4. 魔法方法
   - 字符串表示：__str__()、__repr__()
   - 运算符重载：__add__()、__sub__()、__mul__()
   - 比较运算符：__eq__()、__lt__()
   - 容器类型：__len__()、__getitem__()、__iter__()
   - 其他：__hash__()、__bool__()

【设计原则】

1. 单一职责
   - 每个方法只做一件事
   - 每个类只负责一个概念

2. 封装
   - 使用下划线命名内部属性
   - 通过属性提供访问接口
   - 在setter中验证数据

3. 优雅的接口
   - 使用@property让属性访问更自然
   - 使用魔法方法让类更Pythonic
   - 使用类方法提供多种创建方式

4. 类型安全
   - 在方法中检查参数类型
   - 提供友好的错误信息
   - 使用isinstance()而不是type()

【下一步】

- 继续学习更多面向对象特性
- 学习设计模式
- 实践更多项目

记住：
- 面向对象是一种思维方式
- 好的设计让代码更优雅
- 实践是最好的学习方式
""")
