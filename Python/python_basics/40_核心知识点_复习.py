# ============================================
# 核心知识点复习 - 前16天知识总结
# 日期：2026-06-03（第17天）
# 难度：⭐⭐⭐⭐ 综合复习
# ============================================

"""
【复习目标】

通过系统化的复习，将前16天学到的知识串联起来：
1. 回顾核心概念和原理
2. 理清知识点之间的关系
3. 查漏补缺，巩固理解
4. 建立完整的知识体系

复习方法：
- 不是重新学习，而是快速回顾
- 重点关注核心概念和应用场景
- 找出知识点之间的联系
- 回顾常见错误和最佳实践

复习顺序：
1. 基础语法（第1-2天）
2. 数据结构（第3-5天）
3. 控制流程（第5-6天）
4. 函数和模块（第7-8天）
5. 面向对象编程（第9-11天）
6. 异常处理（第12天）
7. 标准库（第13天）
8. 调试技巧（第14天）
9. 第三方库（第15天）
10. 命令行工具开发（第16天）
"""

# ============================================
# 第一部分：基础语法复习（第1-2天）
# ============================================

print("=" * 60)
print("第一部分：基础语法复习（第1-2天）")
print("=" * 60)

print("""
【核心概念】

1. 变量和数据类型
   - 变量：存储数据的容器
   - 数据类型：int（整数）、float（浮点数）、str（字符串）、bool（布尔值）
   - 类型转换：int()、float()、str()、bool()

2. 输入输出
   - print()：输出到控制台
   - input()：从用户获取输入（返回字符串）
   - f-string：格式化字符串（f"{变量}"）

3. 字符串操作
   - 索引：str[0]（第一个字符）
   - 切片：str[start:end:step]
   - 常用方法：upper()、lower()、strip()、split()、join()、replace()

4. 运算符
   - 算术：+、-、*、/、//（整除）、%（取余）、**（幂）
   - 比较：==、!=、>、<、>=、<=
   - 逻辑：and、or、not
   - 赋值：=、+=、-=、*=、/=

【关键理解】

- 变量是引用，不是值本身
- 字符串是不可变的
- 类型转换的必要性
- 运算符优先级

【常见错误】

1. 忘记类型转换
   age = input("年龄：")  # 返回字符串
   age = int(age)  # 需要转换为整数

2. 字符串拼接类型错误
   "年龄：" + 18  # 错误
   "年龄：" + str(18)  # 正确

3. 除法结果类型
   10 / 3  # 3.333...（浮点数）
   10 // 3  # 3（整数）
""")

# 快速示例
print("\n快速示例：")
print("-" * 60)

# 变量和类型转换
name = "张三"
age = 25
score = 85.5
is_student = True

print(f"姓名：{name}，年龄：{age}，成绩：{score}，是学生：{is_student}")

# 字符串操作
text = "  Hello Python  "
print(f"原始：'{text}'")
print(f"去空格：'{text.strip()}'")
print(f"大写：'{text.upper()}'")
print(f"分割：{text.strip().split()}")

# 运算符
print(f"10 / 3 = {10 / 3}")  # 3.333...
print(f"10 // 3 = {10 // 3}")  # 3
print(f"10 % 3 = {10 % 3}")  # 1
print(f"2 ** 3 = {2 ** 3}")  # 8

# ============================================
# 第二部分：数据结构复习（第3-5天）
# ============================================

print("\n" + "=" * 60)
print("第二部分：数据结构复习（第3-5天）")
print("=" * 60)

print("""
【核心概念】

1. 列表（List）- 可变的有序集合
   - 创建：[1, 2, 3] 或 list()
   - 索引和切片：list[0]、list[1:3]
   - 增删改查：append()、insert()、remove()、pop()
   - 排序：sort()（原地排序）、sorted()（返回新列表）
   - 特点：可变、有序、可重复

2. 字典（Dict）- 键值对集合
   - 创建：{"key": "value"} 或 dict()
   - 访问：dict["key"] 或 dict.get("key")
   - 遍历：keys()、values()、items()
   - 常用方法：pop()、update()、clear()、setdefault()
   - 特点：可变、无序（Python 3.7+保持插入顺序）、键唯一

3. 元组（Tuple）- 不可变的有序集合
   - 创建：(1, 2, 3) 或 tuple()
   - 索引和切片：tuple[0]、tuple[1:3]
   - 解包：a, b, c = (1, 2, 3)
   - 特点：不可变、有序、可重复
   - 使用场景：函数返回多个值、字典的键

4. 集合（Set）- 无序不重复集合
   - 创建：{1, 2, 3} 或 set()
   - 增删：add()、remove()、discard()
   - 数学运算：交集（&）、并集（|）、差集（-）
   - 特点：可变、无序、不重复
   - 使用场景：去重、成员测试、数学运算

【关键理解】

- 可变 vs 不可变
- 有序 vs 无序
- 引用 vs 拷贝
- 选择合适的数据结构

【常见错误】

1. 列表拷贝陷阱
   list1 = [1, 2, 3]
   list2 = list1  # 引用，不是拷贝
   list2.append(4)  # list1 也会改变
   list2 = list1.copy()  # 正确的拷贝方式

2. 字典访问不存在的键
   dict["key"]  # KeyError
   dict.get("key", "默认值")  # 安全的方式

3. 元组单元素定义
   tuple1 = (1)  # 这是整数，不是元组
   tuple1 = (1,)  # 正确的单元素元组
""")

# 快速示例
print("\n快速示例：")
print("-" * 60)

# 列表
fruits = ["苹果", "香蕉", "橙子"]
fruits.append("葡萄")
print(f"列表：{fruits}")
print(f"排序：{sorted(fruits)}")

# 字典
student = {"name": "张三", "age": 20, "score": 85}
print(f"字典：{student}")
print(f"姓名：{student.get('name')}")
print(f"遍历：", end="")
for key, value in student.items():
    print(f"{key}={value}", end=" ")
print()

# 元组
point = (10, 20)
x, y = point  # 解包
print(f"元组：{point}，x={x}，y={y}")

# 集合
set1 = {1, 2, 3, 4}
set2 = {3, 4, 5, 6}
print(f"集合1：{set1}")
print(f"集合2：{set2}")
print(f"交集：{set1 & set2}")
print(f"并集：{set1 | set2}")
print(f"差集：{set1 - set2}")

# ============================================
# 第三部分：控制流程复习（第5-6天）
# ============================================

print("\n" + "=" * 60)
print("第三部分：控制流程复习（第5-6天）")
print("=" * 60)

print("""
【核心概念】

1. 条件判断（if-elif-else）
   - if 条件：执行代码
   - elif 条件：否则如果
   - else：否则
   - 嵌套if：if中包含if
   - 条件表达式：value = a if condition else b

2. for循环
   - 遍历序列：for item in sequence
   - range()：生成数字序列
   - enumerate()：同时获取索引和值
   - 循环嵌套：for中包含for
   - break：跳出循环
   - continue：跳过本次循环

3. while循环
   - 条件循环：while 条件
   - 无限循环：while True
   - break和continue
   - 循环计数器

【关键理解】

- 条件判断的逻辑
- 循环的选择（for vs while）
- break和continue的区别
- 循环嵌套的应用

【常见错误】

1. 条件判断中的赋值
   if x = 10:  # 错误，应该用 ==
   if x == 10:  # 正确

2. 无限循环
   while True:
       # 忘记break，导致无限循环
       pass

3. 循环中修改列表
   for item in list:
       list.remove(item)  # 可能导致跳过元素
""")

# 快速示例
print("\n快速示例：")
print("-" * 60)

# 条件判断
score = 85
if score >= 90:
    grade = "优秀"
elif score >= 80:
    grade = "良好"
elif score >= 60:
    grade = "及格"
else:
    grade = "不及格"
print(f"成绩：{score}，等级：{grade}")

# 条件表达式
status = "成年" if age >= 18 else "未成年"
print(f"年龄：{age}，状态：{status}")

# for循环
print("for循环示例：")
for i in range(1, 6):
    print(f"  {i} * 2 = {i * 2}")

# enumerate
fruits = ["苹果", "香蕉", "橙子"]
print("enumerate示例：")
for index, fruit in enumerate(fruits, start=1):
    print(f"  {index}. {fruit}")

# while循环
print("while循环示例：")
count = 0
while count < 3:
    print(f"  计数：{count}")
    count += 1

# ============================================
# 第四部分：函数和模块复习（第7-8天）
# ============================================

print("\n" + "=" * 60)
print("第四部分：函数和模块复习（第7-8天）")
print("=" * 60)

print("""
【核心概念】

1. 函数定义
   - def 函数名(参数)：定义函数
   - return：返回值
   - 作用域：局部变量 vs 全局变量
   - 文档字符串：函数说明

2. 参数类型
   - 位置参数：按顺序传递
   - 关键字参数：指定参数名
   - 默认参数：提供默认值
   - 可变参数：*args（元组）、**kwargs（字典）

3. 模块导入
   - import 模块名
   - from 模块名 import 函数名
   - import 模块名 as 别名
   - 常用标准库：os、math、random、datetime、json

4. 文件操作
   - open()：打开文件
   - 模式：r（读）、w（写）、a（追加）
   - with语句：自动关闭文件
   - encoding：指定编码（utf-8）

【关键理解】

- 函数的作用：代码复用、模块化
- 参数传递机制
- 作用域规则
- 模块化思维
- 文件操作的安全性

【常见错误】

1. 忘记return
   def add(a, b):
       a + b  # 没有return，返回None

2. 默认参数是可变对象
   def func(list=[]):  # 危险
       list.append(1)
       return list

3. 文件忘记关闭
   f = open("file.txt")  # 忘记close()
   # 应该使用with语句

4. 编码问题
   open("file.txt")  # 可能编码错误
   open("file.txt", encoding="utf-8")  # 正确
""")

# 快速示例
print("\n快速示例：")
print("-" * 60)

# 函数定义
def greet(name, greeting="你好"):
    """问候函数"""
    return f"{greeting}，{name}！"

print(greet("张三"))
print(greet("李四", "早上好"))

# 可变参数
def sum_all(*numbers):
    """计算所有数字的和"""
    return sum(numbers)

print(f"求和：{sum_all(1, 2, 3, 4, 5)}")

# 文件操作示例（模拟）
print("\n文件操作示例（概念）：")
print("""
# 读取文件
with open("data.txt", "r", encoding="utf-8") as f:
    content = f.read()

# 写入文件
with open("output.txt", "w", encoding="utf-8") as f:
    f.write("Hello World")

# 追加文件
with open("log.txt", "a", encoding="utf-8") as f:
    f.write("新日志\\n")
""")

# ============================================
# 第五部分：面向对象编程复习（第9-11天）
# ============================================

print("\n" + "=" * 60)
print("第五部分：面向对象编程复习（第9-11天）")
print("=" * 60)

print("""
【核心概念】

1. 类和对象
   - class：定义类
   - __init__()：初始化方法
   - self：实例引用
   - 属性：对象的数据
   - 方法：对象的行为

2. 继承
   - 父类（基类）：被继承的类
   - 子类（派生类）：继承的类
   - super()：调用父类方法
   - 方法重写：子类重新定义父类方法
   - 多态：不同对象调用相同方法，表现不同

3. 封装
   - 私有属性：__属性名
   - 私有方法：__方法名
   - getter/setter：访问和修改私有属性
   - 数据保护：隐藏内部实现

【关键理解】

- 面向对象的思维方式
- 类是模板，对象是实例
- 继承实现代码复用
- 封装保护数据
- 多态提供灵活性

【常见错误】

1. 忘记self参数
   class MyClass:
       def method():  # 错误，缺少self
           pass

2. __init__拼写错误
   def _init_(self):  # 错误，应该是__init__
       pass

3. 访问私有属性
   obj.__private  # 错误，无法直接访问
   obj._ClassName__private  # 可以但不推荐
""")

# 快速示例
print("\n快速示例：")
print("-" * 60)

# 定义类
class Student:
    """学生类"""

    def __init__(self, name, age):
        self.name = name
        self.age = age
        self.__score = 0  # 私有属性

    def set_score(self, score):
        """设置成绩"""
        if 0 <= score <= 100:
            self.__score = score
        else:
            print("成绩必须在0-100之间")

    def get_score(self):
        """获取成绩"""
        return self.__score

    def info(self):
        """显示信息"""
        return f"{self.name}，{self.age}岁，成绩：{self.__score}"

# 创建对象
student = Student("张三", 20)
student.set_score(85)
print(student.info())

# 继承示例
class GraduateStudent(Student):
    """研究生类（继承学生类）"""

    def __init__(self, name, age, research):
        super().__init__(name, age)
        self.research = research

    def info(self):
        """重写info方法"""
        base_info = super().info()
        return f"{base_info}，研究方向：{self.research}"

grad = GraduateStudent("李四", 25, "人工智能")
grad.set_score(90)
print(grad.info())

# __CONTINUE_HERE__

# ============================================
# 第六部分：异常处理复习（第12天）
# ============================================

print("\n" + "=" * 60)
print("第六部分：异常处理复习（第12天）")
print("=" * 60)

print("""
【核心概念】

1. 异常处理基本语法
   - try：尝试执行的代码
   - except：捕获异常
   - else：没有异常时执行
   - finally：无论如何都执行
   - raise：主动抛出异常

2. 常见异常类型
   - ValueError：值错误
   - TypeError：类型错误
   - KeyError：字典键不存在
   - IndexError：索引超出范围
   - FileNotFoundError：文件不存在
   - ZeroDivisionError：除零错误

3. 异常处理最佳实践
   - 只捕获预期的异常
   - 提供友好的错误提示
   - 记录错误日志
   - 不要忽略异常

【关键理解】

- 异常是程序运行时的错误
- 异常处理提高程序健壮性
- 预期错误要处理
- 不要过度使用异常处理

【常见错误】

1. 捕获所有异常
   try:
       code
   except:  # 不推荐，太宽泛
       pass

2. 忽略异常
   try:
       code
   except Exception:
       pass  # 错误被忽略

3. 异常信息不明确
   except Exception as e:
       print("错误")  # 应该打印具体错误信息
""")

# 快速示例
print("\n快速示例：")
print("-" * 60)

# 基本异常处理
def divide(a, b):
    """除法函数"""
    try:
        result = a / b
        return result
    except ZeroDivisionError:
        print("错误：除数不能为0")
        return None
    except TypeError:
        print("错误：参数必须是数字")
        return None

print(f"10 / 2 = {divide(10, 2)}")
print(f"10 / 0 = {divide(10, 0)}")

# try-except-else-finally
def read_file(filename):
    """读取文件"""
    try:
        # 模拟文件操作
        print(f"尝试读取文件：{filename}")
        if filename == "error.txt":
            raise FileNotFoundError("文件不存在")
        content = "文件内容"
    except FileNotFoundError as e:
        print(f"错误：{e}")
        content = None
    else:
        print("文件读取成功")
    finally:
        print("清理资源")
    return content

read_file("data.txt")
print()
read_file("error.txt")

# ============================================
# 第七部分：标准库复习（第13天）
# ============================================

print("\n" + "=" * 60)
print("第七部分：标准库复习（第13天）")
print("=" * 60)

print("""
【核心概念】

1. os模块 - 操作系统接口
   - os.path.join()：拼接路径
   - os.path.exists()：检查路径是否存在
   - os.makedirs()：创建目录
   - os.listdir()：列出目录内容
   - os.remove()：删除文件

2. datetime模块 - 日期时间处理
   - datetime.now()：当前时间
   - strftime()：格式化日期
   - strptime()：解析日期字符串
   - timedelta：时间差计算

3. json模块 - JSON数据处理
   - json.dumps()：序列化（Python → JSON字符串）
   - json.loads()：反序列化（JSON字符串 → Python）
   - json.dump()：序列化到文件
   - json.load()：从文件反序列化

【关键理解】

- 标准库是Python自带的工具箱
- 优先使用标准库
- 标准库功能强大且稳定
- 学会查阅官方文档

【常见错误】

1. 路径拼接用字符串
   path = "dir" + "/" + "file.txt"  # 不推荐
   path = os.path.join("dir", "file.txt")  # 推荐

2. JSON编码问题
   json.dumps(data)  # 中文变成\\uXXXX
   json.dumps(data, ensure_ascii=False)  # 保留中文

3. 日期格式错误
   strftime("%Y-%m-%d")  # 正确
   strftime("%y-%M-%D")  # 错误的格式
""")

# 快速示例
print("\n快速示例：")
print("-" * 60)

import os
from datetime import datetime, timedelta
import json

# os模块
print("os模块示例：")
path = os.path.join("data", "file.txt")
print(f"  路径拼接：{path}")
print(f"  当前目录：{os.getcwd()}")

# datetime模块
print("\ndatetime模块示例：")
now = datetime.now()
print(f"  当前时间：{now.strftime('%Y-%m-%d %H:%M:%S')}")
tomorrow = now + timedelta(days=1)
print(f"  明天：{tomorrow.strftime('%Y-%m-%d')}")

# json模块
print("\njson模块示例：")
data = {"name": "张三", "age": 25, "score": 85}
json_str = json.dumps(data, ensure_ascii=False)
print(f"  序列化：{json_str}")
data_back = json.loads(json_str)
print(f"  反序列化：{data_back}")

# ============================================
# 第八部分：调试技巧复习（第14天）
# ============================================

print("\n" + "=" * 60)
print("第八部分：调试技巧复习（第14天）")
print("=" * 60)

print("""
【核心概念】

1. 调试的黄金法则
   - 重现：能稳定重现问题
   - 定位：找到问题所在
   - 理解：理解问题原因
   - 修复：修改代码
   - 验证：确认问题解决

2. print调试技巧
   - 打印变量值
   - 打印变量类型
   - 使用分隔线标记
   - 打印函数调用栈

3. logging模块
   - 日志级别：DEBUG、INFO、WARNING、ERROR、CRITICAL
   - basicConfig()：配置日志
   - 日志格式化
   - 输出到文件和控制台

4. 常见bug类型
   - 语法错误：代码写错
   - 逻辑错误：逻辑不对
   - 运行时错误：运行时出错
   - 性能问题：运行太慢

【关键理解】

- 调试是编程的重要技能
- 系统化的调试方法
- 日志记录的重要性
- 预防胜于调试

【常见错误】

1. 只用print调试
   # 应该使用logging模块

2. 日志级别不当
   logging.debug("重要信息")  # 应该用INFO或WARNING

3. 没有记录错误日志
   except Exception:
       pass  # 应该记录日志
""")

# 快速示例
print("\n快速示例：")
print("-" * 60)

import logging

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

# 使用日志
logging.debug("调试信息")
logging.info("普通信息")
logging.warning("警告信息")
logging.error("错误信息")

# 调试示例
def calculate(a, b, operation):
    """计算函数"""
    logging.info(f"计算：{a} {operation} {b}")
    try:
        if operation == "+":
            result = a + b
        elif operation == "-":
            result = a - b
        elif operation == "*":
            result = a * b
        elif operation == "/":
            result = a / b
        else:
            raise ValueError(f"不支持的操作：{operation}")
        logging.info(f"结果：{result}")
        return result
    except Exception as e:
        logging.error(f"计算失败：{e}")
        return None

calculate(10, 5, "+")
calculate(10, 0, "/")

# ============================================
# 第九部分：第三方库复习（第15天）
# ============================================

print("\n" + "=" * 60)
print("第九部分：第三方库复习（第15天）")
print("=" * 60)

print("""
【核心概念】

1. 第三方库和PyPI
   - 第三方库：社区开发的扩展包
   - PyPI：Python Package Index（包索引）
   - 托管了40万+个包

2. pip包管理工具
   - pip install：安装包
   - pip uninstall：卸载包
   - pip list：查看已安装的包
   - pip freeze：导出依赖列表
   - 国内镜像源：加速下载

3. 虚拟环境（venv）
   - 为每个项目创建独立环境
   - 隔离依赖，避免版本冲突
   - python -m venv venv：创建
   - venv\\Scripts\\activate：激活（Windows）

4. 常用第三方库
   - requests：HTTP请求
   - pandas：数据分析
   - numpy：科学计算
   - flask：Web框架

【关键理解】

- 第三方库是Python生态的核心优势
- pip是必须掌握的工具
- 虚拟环境隔离项目依赖
- 学会查阅库的文档

【常见错误】

1. 全局安装所有包
   # 应该为每个项目创建虚拟环境

2. 忘记导出依赖
   # 应该使用pip freeze > requirements.txt

3. 网络请求没有异常处理
   response = requests.get(url)  # 可能失败
   # 应该使用try-except
""")

# 快速示例（概念）
print("\n快速示例（概念）：")
print("-" * 60)

print("""
# pip命令
$ pip install requests
$ pip list
$ pip freeze > requirements.txt
$ pip install -r requirements.txt

# 虚拟环境
$ python -m venv venv
$ venv\\Scripts\\activate  # Windows
$ pip install requests
$ deactivate

# requests库使用
import requests

try:
    response = requests.get('https://api.github.com', timeout=5)
    response.raise_for_status()
    data = response.json()
    print(data)
except requests.exceptions.RequestException as e:
    print(f"请求失败：{e}")

# pandas库使用
import pandas as pd

data = {'name': ['张三', '李四'], 'age': [25, 30]}
df = pd.DataFrame(data)
print(df)
print(df[df['age'] > 25])
""")

# ============================================
# 第十部分：命令行工具开发复习（第16天）
# ============================================

print("\n" + "=" * 60)
print("第十部分：命令行工具开发复习（第16天）")
print("=" * 60)

print("""
【核心概念】

1. 命令行工具
   - 通过命令行与用户交互的程序
   - 轻量高效，适合自动化任务
   - 常见示例：git、pip、docker

2. argparse模块
   - 命令行参数解析
   - 创建解析器：ArgumentParser()
   - 添加参数：add_argument()
   - 位置参数：必需参数
   - 可选参数：--开头，可选
   - 子命令：subparsers

3. 项目结构设计
   - 单一职责：每个模块只做一件事
   - 高内聚：相关功能放在一起
   - 低耦合：模块之间依赖少
   - 推荐结构：main.py、models.py、utils.py、config.py

4. 日志记录和错误处理
   - 使用logging模块，不用print
   - 配置日志级别和格式
   - 预期错误要处理
   - 提供友好的错误提示

【关键理解】

- 命令行工具是实用的项目类型
- argparse是标准库，功能强大
- 项目结构影响可维护性
- 良好的错误处理提升用户体验

【常见错误】

1. 所有代码写在一个文件
   # 应该按功能模块拆分

2. 没有使用日志
   print("错误")  # 应该用logging

3. 路径硬编码
   open("C:\\Users\\...")  # 应该用相对路径

4. 没有提供帮助信息
   # 应该使用description和help参数
""")

# 快速示例（概念）
print("\n快速示例（概念）：")
print("-" * 60)

print("""
# argparse使用
import argparse

parser = argparse.ArgumentParser(description='计算器')
parser.add_argument('num1', type=int, help='第一个数字')
parser.add_argument('num2', type=int, help='第二个数字')
parser.add_argument('--op', default='add', help='操作类型')

args = parser.parse_args()

if args.op == 'add':
    result = args.num1 + args.num2
    print(f'{args.num1} + {args.num2} = {result}')

# 使用
$ python calc.py 10 20
$ python calc.py 10 20 --op sub

# 项目结构
my_cli_tool/
├── main.py          # 主程序入口
├── models.py        # 数据模型
├── utils.py         # 工具函数
├── config.py        # 配置常量
├── data/            # 数据文件
└── logs/            # 日志文件
""")

# ============================================
# 总结：知识点关系图
# ============================================

print("\n" + "=" * 60)
print("总结：知识点关系图")
print("=" * 60)

print("""
【Python知识体系】

1. 基础层（第1-2天）
   ├─ 变量和数据类型
   ├─ 输入输出
   ├─ 字符串操作
   └─ 运算符

2. 数据结构层（第3-5天）
   ├─ 列表（可变、有序）
   ├─ 字典（键值对）
   ├─ 元组（不可变）
   └─ 集合（不重复）

3. 控制流程层（第5-6天）
   ├─ 条件判断（if-elif-else）
   ├─ for循环（遍历序列）
   └─ while循环（条件循环）

4. 函数和模块层（第7-8天）
   ├─ 函数定义和调用
   ├─ 参数传递
   ├─ 模块导入
   └─ 文件操作

5. 面向对象层（第9-11天）
   ├─ 类和对象
   ├─ 继承（代码复用）
   └─ 封装（数据保护）

6. 异常处理层（第12天）
   ├─ try-except
   ├─ 异常类型
   └─ 错误处理最佳实践

7. 标准库层（第13天）
   ├─ os模块（系统操作）
   ├─ datetime模块（日期时间）
   └─ json模块（数据交换）

8. 调试和测试层（第14天）
   ├─ print调试
   ├─ logging模块
   └─ 调试方法论

9. 第三方库层（第15天）
   ├─ pip包管理
   ├─ 虚拟环境
   ├─ requests库
   └─ pandas库

10. 项目开发层（第16天）
    ├─ argparse模块
    ├─ 项目结构设计
    ├─ 日志记录
    └─ 错误处理

【知识点之间的关系】

基础语法 → 数据结构 → 控制流程 → 函数 → 面向对象
    ↓         ↓          ↓        ↓        ↓
  所有知识点的基础，层层递进，相互依赖

异常处理 ← 贯穿所有层次 → 调试技巧
    ↓                        ↓
  提高程序健壮性      提高开发效率

标准库 + 第三方库 = 扩展Python能力
    ↓
项目开发 = 综合运用所有知识

【学习路径】

1. 打好基础：变量、数据类型、运算符
2. 掌握数据结构：列表、字典、元组、集合
3. 学会控制流程：条件判断、循环
4. 理解函数：代码复用、模块化
5. 掌握面向对象：类、继承、封装
6. 学会异常处理：提高健壮性
7. 使用标准库：扩展能力
8. 掌握调试技巧：提高效率
9. 使用第三方库：站在巨人的肩膀上
10. 综合项目实战：融会贯通
""")

print("\n" + "=" * 60)
print("复习完成！")
print("=" * 60)
print("""
恭喜你完成了前16天知识点的系统复习！

你已经回顾了：
✅ 基础语法（变量、数据类型、字符串、运算符）
✅ 数据结构（列表、字典、元组、集合）
✅ 控制流程（条件判断、循环）
✅ 函数和模块（函数定义、模块导入、文件操作）
✅ 面向对象编程（类、继承、封装）
✅ 异常处理（try-except、异常类型）
✅ 标准库（os、datetime、json）
✅ 调试技巧（print、logging）
✅ 第三方库（pip、venv、requests、pandas）
✅ 命令行工具开发（argparse、项目结构）

下一步：
- 整理知识体系和思维导图
- 完成综合练习题
- 总结学习经验

记住：
- 理解概念比记住语法更重要
- 知识点之间是相互关联的
- 实践是最好的学习方式
- 持续学习，不断提升
""")

