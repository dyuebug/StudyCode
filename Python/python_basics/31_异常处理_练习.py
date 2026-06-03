# ============================================
# 异常处理综合练习 - 实战项目
# 日期：2026-05-29（第12天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【综合练习目标】

通过实际项目综合练习异常处理：
1. try-except 基本用法
2. 捕获多个异常
3. else 和 finally 子句
4. raise 主动抛出异常
5. 设计健壮的程序

这个练习重点是体验：
- 异常处理如何让程序更健壮
- 如何提供友好的错误提示
- 如何保证资源正确释放
"""

# ============================================
# 项目1：安全的除法计算器
# ============================================

print("=" * 60)
print("项目1：安全的除法计算器")
print("=" * 60)

def safe_calculator():
    """安全的除法计算器：处理各种异常"""

    print("\n欢迎使用安全除法计算器")

    # 模拟用户输入（实际项目中用 input()）
    test_cases = [
        ("10", "2"),      # 正常情况
        ("10", "0"),      # 除零错误
        ("abc", "2"),     # 值错误
        ("10", "xyz"),    # 值错误
    ]

    for num1_str, num2_str in test_cases:
        print(f"\n计算：{num1_str} / {num2_str}")

        try:
            # 尝试转换为数字
            num1 = float(num1_str)
            num2 = float(num2_str)

            # 尝试执行除法
            result = num1 / num2

        except ValueError as e:
            # 捕获值错误（无法转换为数字）
            print(f"输入错误：请输入有效的数字（{e}）")

        except ZeroDivisionError:
            # 捕获除零错误
            print("计算错误：除数不能为零")

        except Exception as e:
            # 捕获其他未预料的异常
            print(f"未知错误：{e}")

        else:
            # 只有在没有异常时才执行
            print(f"计算成功：结果 = {result}")

        finally:
            # 无论如何都执行
            print("-" * 40)

safe_calculator()

# ============================================
# 项目2：文件读取工具
# ============================================

print("\n" + "=" * 60)
print("项目2：文件读取工具")
print("=" * 60)

def read_file_safe(filename):
    """安全的文件读取：处理文件异常"""

    print(f"\n尝试读取文件：{filename}")

    file_handle = None  # 初始化文件句柄

    try:
        # 尝试打开文件
        file_handle = open(filename, 'r', encoding='utf-8')

        # 尝试读取内容
        content = file_handle.read()

        # 如果成功，返回内容
        return content

    except FileNotFoundError:
        # 文件不存在
        print(f"错误：文件 '{filename}' 不存在")
        return None

    except PermissionError:
        # 没有读取权限
        print(f"错误：没有权限读取文件 '{filename}'")
        return None

    except Exception as e:
        # 其他未预料的错误
        print(f"读取文件时发生错误：{e}")
        return None

    else:
        # 只有在成功读取时执行
        print(f"文件读取成功，内容长度：{len(content)} 字符")

    finally:
        # 无论如何都要关闭文件
        if file_handle is not None:
            file_handle.close()
            print("文件已关闭")

# 测试文件读取
# 注意：这些文件可能不存在，这正是我们要测试的
read_file_safe("存在的文件.txt")
read_file_safe("不存在的文件.txt")

print("\n说明：")
print("- finally 保证文件一定会被关闭")
print("- 即使发生异常，资源也不会泄漏")

# ============================================
# 项目3：用户输入验证器
# ============================================

print("\n" + "=" * 60)
print("项目3：用户输入验证器")
print("=" * 60)

class AgeValidator:
    """年龄验证器：使用 raise 主动抛出异常"""

    def __init__(self):
        self.min_age = 0
        self.max_age = 150

    def validate(self, age):
        """验证年龄是否合法"""

        # 检查类型
        if not isinstance(age, int):
            raise TypeError(f"年龄必须是整数，而不是 {type(age).__name__}")

        # 检查范围
        if age < self.min_age:
            raise ValueError(f"年龄不能小于 {self.min_age}")

        if age > self.max_age:
            raise ValueError(f"年龄不能大于 {self.max_age}")

        # 验证通过
        return True

    def set_age_safe(self, age):
        """安全地设置年龄"""
        try:
            # 尝试验证年龄
            self.validate(age)

        except TypeError as e:
            # 捕获类型错误
            print(f"类型错误：{e}")
            return False

        except ValueError as e:
            # 捕获值错误
            print(f"值错误：{e}")
            return False

        else:
            # 验证成功
            print(f"年龄验证通过：{age}岁")
            return True

# 测试年龄验证器
validator = AgeValidator()

print("\n测试各种输入：")
test_ages = [25, -5, 200, "abc", 3.14, 100]

for age in test_ages:
    print(f"\n验证年龄：{age}")
    validator.set_age_safe(age)

# ============================================
# 项目4：列表和字典安全访问
# ============================================

print("\n" + "=" * 60)
print("项目4：列表和字典安全访问")
print("=" * 60)

class SafeDataAccess:
    """安全的数据访问工具"""

    @staticmethod
    def safe_list_get(lst, index, default=None):
        """安全地获取列表元素"""
        try:
            # 尝试访问列表元素
            return lst[index]

        except IndexError:
            # 索引越界
            print(f"警告：索引 {index} 超出范围，返回默认值")
            return default

        except TypeError:
            # 类型错误（不是列表或索引不是整数）
            print(f"错误：参数类型不正确")
            return default

    @staticmethod
    def safe_dict_get(d, key, default=None):
        """安全地获取字典值"""
        try:
            # 尝试访问字典值
            return d[key]

        except KeyError:
            # 键不存在
            print(f"警告：键 '{key}' 不存在，返回默认值")
            return default

        except TypeError:
            # 类型错误（不是字典）
            print(f"错误：参数类型不正确")
            return default

# 测试列表安全访问
print("\n测试列表安全访问：")
my_list = [10, 20, 30]

print(f"访问索引 1：{SafeDataAccess.safe_list_get(my_list, 1)}")
print(f"访问索引 10：{SafeDataAccess.safe_list_get(my_list, 10, '默认值')}")

# 测试字典安全访问
print("\n测试字典安全访问：")
my_dict = {"name": "Tom", "age": 25}

print(f"访问键 'name'：{SafeDataAccess.safe_dict_get(my_dict, 'name')}")
print(f"访问键 'email'：{SafeDataAccess.safe_dict_get(my_dict, 'email', '未设置')}")

# ============================================
# 项目5：综合应用 - 学生成绩管理器
# ============================================

print("\n" + "=" * 60)
print("项目5：综合应用 - 学生成绩管理器")
print("=" * 60)

class ScoreManager:
    """学生成绩管理器：综合运用异常处理"""

    def __init__(self):
        self.scores = {}

    def add_score(self, name, score):
        """添加学生成绩"""
        try:
            # 验证姓名
            if not isinstance(name, str) or len(name) == 0:
                raise ValueError("姓名必须是非空字符串")

            # 验证成绩
            score = float(score)  # 尝试转换为浮点数
            if score < 0 or score > 100:
                raise ValueError("成绩必须在 0-100 之间")

            # 添加成绩
            self.scores[name] = score

        except ValueError as e:
            print(f"添加失败：{e}")
            return False

        except TypeError as e:
            print(f"类型错误：{e}")
            return False

        else:
            print(f"成功添加：{name} - {score}分")
            return True

    def get_score(self, name):
        """获取学生成绩"""
        try:
            # 尝试获取成绩
            score = self.scores[name]

        except KeyError:
            print(f"错误：找不到学生 '{name}' 的成绩")
            return None

        else:
            print(f"{name} 的成绩：{score}分")
            return score

    def calculate_average(self):
        """计算平均分"""
        try:
            # 检查是否有成绩
            if len(self.scores) == 0:
                raise ValueError("没有任何成绩数据")

            # 计算平均分
            total = sum(self.scores.values())
            average = total / len(self.scores)

        except ValueError as e:
            print(f"计算失败：{e}")
            return None

        except ZeroDivisionError:
            print("错误：除数为零")
            return None

        else:
            print(f"平均分：{average:.2f}")
            return average

# 测试成绩管理器
print("\n创建成绩管理器")
manager = ScoreManager()

print("\n添加学生成绩：")
manager.add_score("小明", 85)
manager.add_score("小红", 92)
manager.add_score("小刚", 78)
manager.add_score("小李", 150)  # 超出范围
manager.add_score("小王", "abc")  # 类型错误
manager.add_score("", 90)  # 姓名为空

print("\n查询学生成绩：")
manager.get_score("小明")
manager.get_score("小张")  # 不存在

print("\n计算平均分：")
manager.calculate_average()

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结：异常处理的价值")
print("=" * 60)

print("""
通过今天的综合练习，你已经实际用到了：

1. try-except 基本语法
   - 捕获特定异常
   - 捕获多个异常
   - 获取异常信息

2. else 子句
   - 只在没有异常时执行
   - 用于"成功后的操作"

3. finally 子句
   - 无论如何都执行
   - 用于资源清理

4. raise 主动抛出异常
   - 参数验证
   - 业务逻辑检查

5. 异常处理的最佳实践
   - 指定具体的异常类型
   - 提供友好的错误提示
   - 保证资源正确释放
   - 不要滥用异常处理

现在你对异常处理的理解已经从"会写语法"进步到：
你开始理解"如何设计健壮的程序"。
""")

# ============================================
# 附加练习题（不提供答案）
# ============================================

print("\n" + "=" * 60)
print("附加练习题（自己动手）")
print("=" * 60)

print("""
练习1：设计一个 BankAccount 类
  要求：
  - 有私有属性 __balance（余额）
  - 提供 deposit(amount) 存款方法
  - 提供 withdraw(amount) 取款方法
  - 金额必须大于 0，否则抛出 ValueError
  - 余额不足时抛出 ValueError
  - 使用异常处理保证操作安全

练习2：设计一个配置文件读取器
  要求：
  - 读取配置文件（如 config.txt）
  - 如果文件不存在，创建默认配置
  - 如果文件格式错误，提示用户
  - 使用 finally 确保文件关闭

练习3：设计一个数据转换工具
  要求：
  - 将字符串列表转换为整数列表
  - 如果某个元素无法转换，跳过并记录
  - 返回成功转换的结果和失败的元素
  - 提示：使用 try-except 处理每个元素

练习4：设计一个网络请求模拟器
  要求：
  - 模拟网络请求（可能成功或失败）
  - 失败时自动重试（最多3次）
  - 使用异常处理控制重试逻辑
  - 提示：用 raise 模拟网络错误

练习5：设计一个日志记录器
  要求：
  - 将日志写入文件
  - 如果文件写入失败，打印到控制台
  - 使用 finally 确保文件关闭
  - 捕获所有可能的异常
""")

print("\n" + "=" * 60)
print("学习完成！")
print("=" * 60)
print("""
恭喜你完成了异常处理的学习！

你已经掌握了：
✅ 异常的概念和常见类型
✅ try-except-else-finally 完整语法
✅ raise 主动抛出异常
✅ 异常处理的最佳实践
✅ 如何设计健壮的程序

下一步：
- 完成附加练习题
- 在实际项目中应用异常处理
- 让你的程序更加健壮和可靠
""")





