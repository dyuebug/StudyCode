# ============================================
# 面向对象综合练习 - 银行账户系统
# 日期：2026-04-09（第11天）
# 难度：⭐⭐⭐ 中高级
# ============================================

"""
【综合练习目标】

通过“银行账户系统”综合练习前面学到的知识：
1. 类和对象
2. 属性和方法
3. 封装与私有属性
4. 公开接口设计
5. 合理的数据校验

这个练习重点不是“功能很多”，
而是体验：对象怎样设计才更安全、更合理。
"""

# ============================================
# 功能1：定义账户类
# ============================================

print("=" * 60)
print("功能1：定义 BankAccount 类")
print("=" * 60)

class BankAccount:
    """银行账户类：演示封装和公开接口设计"""

    def __init__(self, owner, password, balance=0):
        # 公开属性：账户主人姓名，可以直接查看
        self.owner = owner

        # 私有属性：密码和余额不应该被外部直接访问
        self.__password = password
        self.__balance = balance

    def __check_password(self, password):
        """私有方法：内部校验密码是否正确"""
        return self.__password == password

    def get_balance(self, password):
        """公开方法：验证密码后再返回余额"""
        if self.__check_password(password):
            return self.__balance
        else:
            print("密码错误，无法查看余额。")
            return None

    def deposit(self, amount):
        """存款：金额必须大于 0"""
        if amount > 0:
            self.__balance += amount
            print(f"存款成功，当前余额：{self.__balance}")
        else:
            print("存款金额必须大于 0")

    def withdraw(self, amount, password):
        """取款：要验证密码，还要检查余额是否足够"""
        if not self.__check_password(password):
            print("密码错误，取款失败。")
            return

        if amount <= 0:
            print("取款金额必须大于 0")
        elif amount > self.__balance:
            print("余额不足，取款失败。")
        else:
            self.__balance -= amount
            print(f"取款成功，当前余额：{self.__balance}")

    def change_password(self, old_password, new_password):
        """修改密码：必须先验证旧密码"""
        if not self.__check_password(old_password):
            print("旧密码错误，修改失败。")
            return

        if len(new_password) < 6:
            print("新密码太短，至少需要 6 位。")
            return

        self.__password = new_password
        print("密码修改成功。")

    def show_account_info(self):
        """显示账户基础信息（不暴露敏感数据）"""
        print(f"账户持有人：{self.owner}")
        print("余额和密码属于敏感信息，不直接公开显示。")

# ============================================
# 功能2：创建对象并测试
# ============================================

print("\n" + "=" * 60)
print("功能2：创建对象并测试")
print("=" * 60)

account = BankAccount("小李", "123456", 1000)

print("\n显示账户信息：")
account.show_account_info()

print("\n测试查看余额：")
print(f"正确密码查看余额：{account.get_balance('123456')}")
print(f"错误密码查看余额：{account.get_balance('000000')}")

print("\n测试存款：")
account.deposit(500)
account.deposit(-10)

print("\n测试取款：")
account.withdraw(300, "123456")
account.withdraw(5000, "123456")
account.withdraw(100, "000000")

print("\n测试修改密码：")
account.change_password("000000", "888888")
account.change_password("123456", "123")
account.change_password("123456", "888888")

print("\n用新密码查看余额：")
print(f"新密码查看余额：{account.get_balance('888888')}")

# ============================================
# 功能3：说明封装的价值
# ============================================

print("\n" + "=" * 60)
print("功能3：理解封装的价值")
print("=" * 60)

print("""
这个账户类为什么设计得比“直接改属性”更好？

原因：
1. 外部不能直接乱改余额
2. 查看余额需要密码验证
3. 取款需要检查余额是否足够
4. 修改密码需要验证旧密码
5. 所有关键操作都经过统一入口

这就是“封装”的实际价值：
- 保证对象状态合理
- 防止错误使用
- 让类的接口更清晰
""")

# ============================================
# 功能4：多个对象独立存在
# ============================================

print("\n" + "=" * 60)
print("功能4：多个对象独立存在")
print("=" * 60)

account1 = BankAccount("张三", "abc123", 500)
account2 = BankAccount("李四", "xyz789", 1200)

print("\n账户1操作：")
account1.deposit(100)
print(f"账户1余额：{account1.get_balance('abc123')}")

print("\n账户2操作：")
account2.withdraw(200, 'xyz789')
print(f"账户2余额：{account2.get_balance('xyz789')}")

print("\n说明：")
print("- account1 和 account2 是两个独立对象")
print("- 一个对象的数据变化，不会影响另一个对象")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结：今天练习学到了什么")
print("=" * 60)

print("""
通过今天的综合练习，你已经实际用到了：

1. class 定义类
2. __init__() 初始化对象
3. 私有属性和私有方法
4. 公开接口设计
5. 数据校验
6. 多个对象独立存在

现在你对面向对象的理解已经比“会写类”更进一步了：
你开始理解“类应该怎么设计才合理”。
""")

# ============================================
# 练习题（不提供答案）
# ============================================

print("\n" + "=" * 60)
print("附加练习题（自己动手）")
print("=" * 60)

print("""
练习1：设计一个 GameAccount 类
  要求：
  - 有用户名
  - 有私有属性 __level、__gold
  - 提供升级、加金币、消费金币的方法
  - 金币不能被扣成负数

练习2：设计一个 LibraryBook 类
  要求：
  - 有书名、作者
  - 有私有属性 __is_borrowed
  - 提供 borrow_book()、return_book()、show_status() 方法
  - 不允许重复借出未归还的书

练习3：设计一个 UserProfile 类
  要求：
  - 有 username
  - 有私有属性 __email、__phone
  - 提供安全的修改方法
  - 修改邮箱时要简单检查格式是否合理
""")
