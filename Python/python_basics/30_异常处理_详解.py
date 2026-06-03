# ============================================
# 异常处理基础 - 超详细注释版
# 日期：2026-05-29（第12天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【核心概念】异常处理（Exception Handling）

是什么？
- 异常是程序运行时发生的错误
- 异常处理是捕获和处理这些错误的机制
- 让程序在遇到错误时不会崩溃，而是优雅地处理

为什么需要？
- 避免程序因错误而崩溃
- 提供友好的错误提示
- 保证资源正确释放（如文件、数据库连接）
- 让程序更加健壮和可靠

核心特点？
- try：尝试执行可能出错的代码
- except：捕获并处理异常
- else：没有异常时执行
- finally：无论如何都执行（资源清理）
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 什么是异常？
# --------------------------------------------

print("\n1.1 什么是异常？")
print("-" * 60)

print("""
【异常的定义】

异常 = 程序运行时发生的错误

生活例子：
- 你去ATM取钱，但卡里余额不足 → ValueError
- 你要打开一个文件，但文件不存在 → FileNotFoundError
- 你要除以一个数，但这个数是0 → ZeroDivisionError

编程中的异常：
- 程序运行时遇到无法继续执行的情况
- 如果不处理，程序会崩溃并显示错误信息
- 通过异常处理，可以让程序优雅地处理错误
""")

# --------------------------------------------
# 1.2 常见异常类型
# --------------------------------------------

print("\n1.2 常见异常类型")
print("-" * 60)

print("""
【Python 常见异常类型】

1. ZeroDivisionError
   - 除零错误
   - 示例：10 / 0

2. ValueError
   - 值错误（类型正确但值不合适）
   - 示例：int("abc")

3. TypeError
   - 类型错误（操作不支持该类型）
   - 示例："hello" + 5

4. FileNotFoundError
   - 文件未找到
   - 示例：open("不存在的文件.txt")

5. IndexError
   - 索引越界
   - 示例：[1, 2, 3][10]

6. KeyError
   - 字典键不存在
   - 示例：{"name": "Tom"}["age"]

7. AttributeError
   - 属性不存在
   - 示例："hello".append("world")
""")

# --------------------------------------------
# 1.3 try-except 基本语法
# --------------------------------------------

print("\n1.3 try-except 基本语法")
print("-" * 60)

print("""
【try-except 函数卡片】

基本语法：
try:
    # 可能出错的代码
    代码块
except 异常类型:
    # 处理异常的代码
    代码块

作用：
- try：尝试执行可能出错的代码
- except：如果发生异常，执行这里的代码

参数详解：
- 异常类型（可选）：指定要捕获的异常类型
  - 如果不指定，捕获所有异常（不推荐）
  - 推荐指定具体的异常类型

使用示例：
try:
    result = 10 / 0
except ZeroDivisionError:
    print("不能除以零！")
""")

# --------------------------------------------
# 1.4 else 和 finally 子句
# --------------------------------------------

print("\n1.4 else 和 finally 子句")
print("-" * 60)

print("""
【else 子句】

语法：
try:
    代码块
except 异常类型:
    处理异常
else:
    # 没有异常时执行
    代码块

作用：
- 只有在 try 块没有发生异常时才执行
- 用于"成功后的操作"

────────────────────────────────────

【finally 子句】

语法：
try:
    代码块
except 异常类型:
    处理异常
finally:
    # 无论如何都执行
    代码块

作用：
- 无论是否发生异常都会执行
- 用于资源清理（关闭文件、释放连接）
- 保证资源一定会被释放

重要性：⭐⭐⭐⭐⭐
- 文件操作必须用 finally 关闭文件
- 数据库连接必须用 finally 关闭连接
- 这是编写健壮程序的关键
""")

# ============================================
# 第二部分：基础操作实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 没有异常处理的问题
# --------------------------------------------

print("\n2.1 没有异常处理的问题")
print("-" * 60)

print("示例：除法运算，不处理异常")
print("如果运行 10 / 0，程序会崩溃")
print("（这里用注释演示，不实际运行）")
print("""
# result = 10 / 0  # 这会导致程序崩溃
# print("这行代码永远不会执行")
""")

print("问题：程序直接崩溃，用户体验很差")

# --------------------------------------------
# 2.2 使用 try-except 处理异常
# --------------------------------------------

print("\n2.2 使用 try-except 处理异常")
print("-" * 60)

print("示例1：捕获除零异常")
try:
    # 尝试执行可能出错的代码
    result = 10 / 0
except ZeroDivisionError:
    # 如果发生除零异常，执行这里
    print("错误：不能除以零！")
    result = None

print(f"结果：{result}")
print("程序继续运行，没有崩溃")

print("\n" + "-" * 60)
print("示例2：捕获值错误")
try:
    # 尝试将非数字字符串转换为整数
    number = int("abc")
except ValueError:
    # 如果发生值错误，执行这里
    print("错误：无法将 'abc' 转换为整数！")
    number = 0

print(f"结果：{number}")

# --------------------------------------------
# 2.3 捕获多个异常
# --------------------------------------------

print("\n2.3 捕获多个异常")
print("-" * 60)

print("方法1：分别捕获不同的异常")
def safe_divide(a, b):
    """安全的除法函数"""
    try:
        # 尝试执行除法
        result = a / b
    except ZeroDivisionError:
        # 捕获除零异常
        print("错误：除数不能为零")
        return None
    except TypeError:
        # 捕获类型错误
        print("错误：参数必须是数字")
        return None
    else:
        # 没有异常时执行
        print(f"计算成功：{a} / {b} = {result}")
        return result

safe_divide(10, 2)
safe_divide(10, 0)
safe_divide(10, "abc")

print("\n" + "-" * 60)
print("方法2：用一个 except 捕获多个异常")
def safe_convert(value):
    """安全的类型转换"""
    try:
        # 尝试转换为整数
        return int(value)
    except (ValueError, TypeError):
        # 用元组捕获多个异常
        print(f"错误：无法将 {value} 转换为整数")
        return 0

safe_convert("123")
safe_convert("abc")
safe_convert(None)

# --------------------------------------------
# 2.4 使用 else 子句
# --------------------------------------------

print("\n2.4 使用 else 子句")
print("-" * 60)

print("else 只在没有异常时执行")

def read_number():
    """读取用户输入的数字"""
    user_input = "42"  # 模拟用户输入

    try:
        # 尝试转换为整数
        number = int(user_input)
    except ValueError:
        # 如果转换失败
        print("输入无效，请输入数字")
    else:
        # 只有转换成功时才执行
        print(f"成功读取数字：{number}")
        print(f"数字的平方是：{number ** 2}")

read_number()

# --------------------------------------------
# 2.5 使用 finally 子句
# --------------------------------------------

print("\n2.5 使用 finally 子句")
print("-" * 60)

print("finally 无论如何都会执行")

def process_file(filename):
    """处理文件（演示 finally 的作用）"""
    print(f"尝试打开文件：{filename}")

    try:
        # 尝试打开文件
        # 这里用模拟，实际会打开真实文件
        if filename == "不存在.txt":
            raise FileNotFoundError("文件不存在")
        print("文件打开成功")
        print("正在处理文件...")
    except FileNotFoundError as e:
        # 捕获文件未找到异常
        print(f"错误：{e}")
    finally:
        # 无论是否发生异常，都会执行
        print("清理资源：关闭文件")
        print("-" * 40)

process_file("存在.txt")
process_file("不存在.txt")

# --------------------------------------------
# 2.6 主动抛出异常（raise）
# --------------------------------------------

print("\n2.6 主动抛出异常（raise）")
print("-" * 60)

print("""
【raise 函数卡片】

语法：
raise 异常类型("错误信息")

作用：
- 主动抛出一个异常
- 用于在代码中检测到不合理的情况时

使用场景：
- 参数验证失败
- 业务逻辑不满足
- 数据不合法

示例：
if age < 0:
    raise ValueError("年龄不能为负数")
""")

print("\n示例：年龄验证函数")

def set_age(age):
    """设置年龄，必须是正整数"""
    if not isinstance(age, int):
        # 如果不是整数，抛出类型错误
        raise TypeError("年龄必须是整数")
    if age < 0:
        # 如果是负数，抛出值错误
        raise ValueError("年龄不能为负数")
    if age > 150:
        # 如果太大，抛出值错误
        raise ValueError("年龄不能超过150岁")

    print(f"年龄设置成功：{age}岁")
    return age

# 测试正常情况
try:
    set_age(25)
except (TypeError, ValueError) as e:
    print(f"错误：{e}")

# 测试异常情况
try:
    set_age(-5)
except (TypeError, ValueError) as e:
    print(f"错误：{e}")

try:
    set_age("abc")
except (TypeError, ValueError) as e:
    print(f"错误：{e}")

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("\n" + "=" * 60)
print("第三部分：深入理解（选学，可跳过）")
print("=" * 60)

print("""
🔍 深入理解：什么时候用 if 判断，什么时候用 try-except？

如果你觉得抽象，可以先跳过，不影响后续学习。

原则：
1. 能预防的错误，用 if 判断
   - 例如：检查列表是否为空
   - if len(my_list) > 0:

2. 无法预防的错误，用 try-except
   - 例如：文件是否存在（可能在检查后被删除）
   - 例如：网络请求（可能突然断网）

为什么？
- if 判断更清晰，性能更好
- try-except 用于"真正无法预知"的情况

实际建议：
- 优先用 if 判断
- 只在必要时用 try-except
- 不要用异常处理代替正常逻辑
""")

print("""
🔍 深入理解：为什么要指定具体的异常类型？

不推荐：
try:
    代码
except:  # 捕获所有异常
    处理

推荐：
try:
    代码
except ValueError:  # 只捕获值错误
    处理

原因：
1. 捕获所有异常可能隐藏真正的bug
2. 不同的异常需要不同的处理方式
3. 明确的异常类型让代码更易维护

例外：
- 在最外层可以捕获所有异常，记录日志
- 但内层代码应该捕获具体异常
""")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
错误1：捕获所有异常

❌ 错误代码：
try:
    result = 10 / 0
except:
    print("发生错误")

问题：
- 捕获了所有异常，包括系统异常
- 可能隐藏真正的bug
- 难以调试

✅ 正确代码：
try:
    result = 10 / 0
except ZeroDivisionError:
    print("不能除以零")

记忆技巧：
- 总是指定具体的异常类型
- 只在最外层捕获所有异常（用于日志记录）
""")

print("""
错误2：except 块中没有任何处理

❌ 错误代码：
try:
    result = 10 / 0
except ZeroDivisionError:
    pass  # 什么都不做

问题：
- 异常被忽略了
- 用户不知道发生了什么
- 难以调试

✅ 正确代码：
try:
    result = 10 / 0
except ZeroDivisionError:
    print("错误：不能除以零")
    result = None

记忆技巧：
- 至少要打印错误信息
- 或者提供默认值
- 或者记录日志
""")

print("""
错误3：finally 中使用 return

❌ 错误代码：
def bad_function():
    try:
        return "try"
    finally:
        return "finally"  # 会覆盖 try 的返回值

问题：
- finally 中的 return 会覆盖 try 中的 return
- 导致意外的返回值

✅ 正确代码：
def good_function():
    try:
        return "try"
    finally:
        print("清理资源")  # 不要用 return

记忆技巧：
- finally 只用于清理资源
- 不要在 finally 中使用 return
""")

print("""
错误4：用异常处理代替正常逻辑

❌ 错误代码：
try:
    value = my_dict["key"]
except KeyError:
    value = None

问题：
- 能用 if 判断的，不要用 try-except
- 性能较差

✅ 正确代码：
value = my_dict.get("key", None)
# 或者
if "key" in my_dict:
    value = my_dict["key"]

记忆技巧：
- 能预防的用 if
- 无法预防的用 try-except
""")

print("""
错误5：忘记使用 finally 清理资源

❌ 错误代码：
try:
    f = open("file.txt")
    content = f.read()
except FileNotFoundError:
    print("文件不存在")
# 如果发生异常，文件没有关闭

问题：
- 资源泄漏
- 文件可能没有正确关闭

✅ 正确代码：
try:
    f = open("file.txt")
    content = f.read()
except FileNotFoundError:
    print("文件不存在")
finally:
    if 'f' in locals():
        f.close()

# 更好的方式：使用 with 语句
with open("file.txt") as f:
    content = f.read()

记忆技巧：
- 文件操作优先用 with
- 其他资源用 finally 清理
""")

print("""
错误6：异常信息不够详细

❌ 错误代码：
except ValueError:
    print("错误")

问题：
- 不知道具体是什么错误
- 难以调试

✅ 正确代码：
except ValueError as e:
    print(f"值错误：{e}")

记忆技巧：
- 用 as e 获取异常对象
- 打印异常信息
""")

print("""
错误7：在循环中滥用异常处理

❌ 错误代码：
for i in range(1000):
    try:
        result = 10 / i
    except ZeroDivisionError:
        continue

问题：
- 性能差
- 能用 if 判断的不要用 try-except

✅ 正确代码：
for i in range(1000):
    if i == 0:
        continue
    result = 10 / i

记忆技巧：
- 循环中优先用 if 判断
- 避免频繁触发异常
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【try-except】
语法：
try:
    可能出错的代码
except 异常类型:
    处理异常

作用：捕获并处理异常

────────────────────────────────────

【捕获多个异常】
语法：
except (异常1, 异常2):
    处理

作用：用一个 except 捕获多个异常

────────────────────────────────────

【else 子句】
语法：
try:
    代码
except:
    处理
else:
    没有异常时执行

作用：只在没有异常时执行

────────────────────────────────────

【finally 子句】
语法：
try:
    代码
except:
    处理
finally:
    无论如何都执行

作用：资源清理，保证一定执行

────────────────────────────────────

【raise 抛出异常】
语法：
raise 异常类型("错误信息")

作用：主动抛出异常

────────────────────────────────────

【获取异常信息】
语法：
except 异常类型 as e:
    print(e)

作用：获取异常对象，打印详细信息

────────────────────────────────────

【常见异常类型】
- ZeroDivisionError：除零错误
- ValueError：值错误
- TypeError：类型错误
- FileNotFoundError：文件未找到
- IndexError：索引越界
- KeyError：键不存在
- AttributeError：属性不存在
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题（自己动手！）")
print("=" * 60)

print("""
练习1：安全的列表访问
  要求：
  - 写一个函数 safe_get(lst, index)
  - 如果索引有效，返回元素
  - 如果索引越界，返回 None 并打印提示
  提示：捕获 IndexError

练习2：安全的字典访问
  要求：
  - 写一个函数 safe_dict_get(d, key)
  - 如果键存在，返回值
  - 如果键不存在，返回 "键不存在" 并打印提示
  提示：捕获 KeyError

练习3：年龄验证器
  要求：
  - 写一个函数 validate_age(age)
  - 年龄必须是整数
  - 年龄必须在 0-150 之间
  - 不满足条件时用 raise 抛出异常
  提示：使用 TypeError 和 ValueError

练习4：文件读取器
  要求：
  - 写一个函数 read_file_safe(filename)
  - 尝试读取文件内容
  - 如果文件不存在，返回 None
  - 使用 finally 确保文件关闭
  提示：捕获 FileNotFoundError

练习5：除法计算器
  要求：
  - 写一个函数 safe_divide(a, b)
  - 处理除零异常
  - 处理类型错误
  - 使用 else 在成功时打印结果
  提示：捕获 ZeroDivisionError 和 TypeError
""")

print("\n" + "=" * 60)
print("学习完成！")
print("=" * 60)
print("""
今天学习了异常处理的核心知识：
1. 异常的概念和常见类型
2. try-except 基本语法
3. else 和 finally 子句
4. raise 主动抛出异常
5. 常见错误和最佳实践

下一步：
- 完成练习题
- 运行 31_异常处理_练习.py 进行综合练习
""")

