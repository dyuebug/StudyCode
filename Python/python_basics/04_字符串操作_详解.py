# ============================================
# 字符串操作 - 超详细注释版
# 日期：2026-02-17（第2天）
# 难度：⭐⭐ 中级
# ============================================

"""
【核心概念】字符串操作

是什么？
- 字符串是Python中最常用的数据类型之一，用于存储文本信息

为什么需要？
- 处理用户输入、文件内容、网页数据等都需要字符串操作
- 字符串方法让文本处理变得简单高效

核心特点？
- 字符串是不可变的（immutable）- 不能修改单个字符
- 支持索引和切片操作
- 提供了丰富的内置方法
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 字符串的本质
# --------------------------------------------

print("\n1.1 字符串的本质")
print("-" * 60)

print("""
【字符串】

是什么？
- 字符串是字符的序列（sequence of characters）
- 用引号包裹的文本：'单引号' 或 "双引号"

为什么需要？
- 存储和处理文本信息
- 与用户交互（输入输出）
- 处理文件、网页等文本数据

核心特点：
- 不可变性：创建后不能修改
- 有序性：每个字符都有固定位置（索引）
- 可迭代：可以遍历每个字符
""")

# --------------------------------------------
# 1.2 字符串的不可变性 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.2 字符串的不可变性")
print("-" * 60)

print("""
【不可变性（Immutability）】

是什么？
- 字符串创建后，不能修改其中的字符
- 任何"修改"操作都会创建新字符串

为什么这样设计？
- 安全性：防止意外修改
- 性能：可以优化内存使用
- 可哈希：可以作为字典的键

示例：
text = "Hello"
# text[0] = "h"  # ❌ 错误！不能修改
text = "hello"   # ✅ 正确！创建新字符串
""")

# --------------------------------------------
# 1.3 索引和切片的概念 ⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n1.3 索引和切片")
print("-" * 60)

print("""
【索引（Index）】

是什么？
- 索引是字符在字符串中的位置编号
- Python索引从0开始（不是1！）

正向索引：从左到右，0, 1, 2, ...
反向索引：从右到左，-1, -2, -3, ...

示例："Hello"
  H    e    l    l    o
  0    1    2    3    4    (正向索引)
 -5   -4   -3   -2   -1    (反向索引)

【切片（Slice）】

是什么？
- 切片是获取字符串的一部分（子串）
- 语法：[start:end:step]

参数详解：
- start：起始索引（包含），默认0
- end：结束索引（不包含！），默认到末尾
- step：步长，默认1

重要规则：
- 左闭右开：[start, end)
- 可以省略参数
- 负数索引表示从右往左
""")

# ============================================
# 第二部分：基础操作实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 索引操作
# --------------------------------------------

print("\n2.1 索引操作")
print("-" * 60)

# 创建一个字符串
text = "Python"

# 正向索引：从0开始
print(f"字符串：{text}")
print(f"第1个字符（索引0）：{text[0]}")  # P
print(f"第2个字符（索引1）：{text[1]}")  # y
print(f"第6个字符（索引5）：{text[5]}")  # n

# 反向索引：从-1开始
print(f"最后1个字符（索引-1）：{text[-1]}")  # n
print(f"最后2个字符（索引-2）：{text[-2]}")  # o
print(f"第1个字符（索引-6）：{text[-6]}")   # P

# --------------------------------------------
# 2.2 切片操作
# --------------------------------------------

print("\n2.2 切片操作")
print("-" * 60)

text = "Hello, World!"

# 基本切片：[start:end]
# 获取索引1到4的字符（不包含索引4）
print(f"text[1:5] = {text[1:5]}")  # ello

# 省略start：从开头开始
print(f"text[:5] = {text[:5]}")    # Hello

# 省略end：到末尾结束
print(f"text[7:] = {text[7:]}")    # World!

# 省略start和end：复制整个字符串
print(f"text[:] = {text[:]}")      # Hello, World!

# 使用负数索引
print(f"text[-6:-1] = {text[-6:-1]}")  # World

# 使用步长step
print(f"text[::2] = {text[::2]}")   # Hlo ol!（每隔1个字符取1个）
print(f"text[::3] = {text[::3]}")   # Hl r!（每隔2个字符取1个）

# 反转字符串（步长为-1）
print(f"text[::-1] = {text[::-1]}")  # !dlroW ,olleH

# --------------------------------------------
# 2.3 字符串拼接
# --------------------------------------------

print("\n2.3 字符串拼接")
print("-" * 60)

# 使用 + 运算符拼接
first_name = "Zhang"
last_name = "San"
full_name = first_name + " " + last_name
print(f"全名：{full_name}")

# 使用 * 运算符重复
line = "=" * 30
print(line)

# 使用 join() 方法拼接列表（后面会详细讲）
words = ["Hello", "World", "Python"]
sentence = " ".join(words)
print(f"句子：{sentence}")

# --------------------------------------------
# 2.4 常用字符串方法（15+个）⭐⭐⭐⭐⭐
# --------------------------------------------

print("\n2.4 常用字符串方法")
print("-" * 60)

text = "  Hello, Python World!  "

# 1. upper() - 转大写
print(f"upper(): {text.upper()}")

# 2. lower() - 转小写
print(f"lower(): {text.lower()}")

# 3. strip() - 去除两端空白
print(f"strip(): '{text.strip()}'")

# 4. lstrip() - 去除左端空白
print(f"lstrip(): '{text.lstrip()}'")

# 5. rstrip() - 去除右端空白
print(f"rstrip(): '{text.rstrip()}'")

# 6. replace() - 替换子串
print(f"replace(): {text.replace('Python', 'Java')}")

# 7. find() - 查找子串位置（返回索引，找不到返回-1）
print(f"find('Python'): {text.find('Python')}")
print(f"find('Java'): {text.find('Java')}")  # -1

# 8. count() - 统计子串出现次数
print(f"count('o'): {text.count('o')}")

# 9. startswith() - 判断是否以某字符串开头
print(f"startswith('  Hello'): {text.startswith('  Hello')}")

# 10. endswith() - 判断是否以某字符串结尾
print(f"endswith('!  '): {text.endswith('!  ')}")

# 11. split() - 分割字符串为列表
words = text.strip().split()  # 默认按空白分割
print(f"split(): {words}")

# 12. split(',') - 按指定分隔符分割
parts = text.strip().split(',')
print(f"split(','): {parts}")

# 13. capitalize() - 首字母大写
text2 = "python"
print(f"capitalize(): {text2.capitalize()}")

# 14. title() - 每个单词首字母大写
text3 = "hello world python"
print(f"title(): {text3.title()}")

# 15. isdigit() - 判断是否全是数字
print(f"'123'.isdigit(): {'123'.isdigit()}")
print(f"'12a'.isdigit(): {'12a'.isdigit()}")

# 16. isalpha() - 判断是否全是字母
print(f"'abc'.isalpha(): {'abc'.isalpha()}")
print(f"'ab1'.isalpha(): {'ab1'.isalpha()}")

# 17. isalnum() - 判断是否全是字母或数字
print(f"'abc123'.isalnum(): {'abc123'.isalnum()}")
print(f"'abc 123'.isalnum(): {'abc 123'.isalnum()}")

# --------------------------------------------
# 2.5 转义字符
# --------------------------------------------

print("\n2.5 转义字符")
print("-" * 60)

# \n - 换行
print("第一行\n第二行")

# \t - 制表符（Tab）
print("姓名\t年龄\t城市")
print("张三\t25\t北京")

# \\ - 反斜杠本身
print("路径：C:\\Users\\Documents")

# \' 和 \" - 引号
print('He said: "Hello!"')
print("It's a nice day")

# ============================================
# 🔍 深入理解：字符串的内存优化（选学）
# ============================================

print("\n" + "=" * 60)
print("🔍 深入理解：字符串的内存优化（选学）")
print("=" * 60)

print("""
如果你想知道"为什么字符串不可变"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【字符串驻留（String Interning）】

Python会自动优化字符串的内存使用：
- 相同的字符串可能共享同一块内存
- 这就是为什么字符串必须不可变

示例：
a = "hello"
b = "hello"
# a和b可能指向同一块内存（Python自动优化）

如果字符串可变：
- 修改a会影响b（因为共享内存）
- 这会导致难以预料的bug

所以Python选择了不可变设计：
- 安全：不会意外修改
- 高效：可以共享内存
- 简单：行为可预测
""")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

# --------------------------------------------
# 4.1 错误1：尝试修改字符串
# --------------------------------------------

print("\n4.1 错误1：尝试修改字符串")
print("-" * 60)

print("""
❌ 错误代码：
```python
text = "Hello"
text[0] = "h"  # 尝试修改第一个字符
```

结果：TypeError: 'str' object does not support item assignment

原因：字符串是不可变的，不能修改单个字符

✅ 正确代码：
```python
text = "Hello"
text = "h" + text[1:]  # 创建新字符串
# 或者
text = text.replace("H", "h")
```

结果：text = "hello"

记忆技巧：字符串不可变，只能创建新字符串
""")

# --------------------------------------------
# 4.2 错误2：索引越界
# --------------------------------------------

print("\n4.2 错误2：索引越界")
print("-" * 60)

print("""
❌ 错误代码：
```python
text = "Hello"
print(text[10])  # 索引超出范围
```

结果：IndexError: string index out of range

原因：字符串只有5个字符（索引0-4），索引10不存在

✅ 正确代码：
```python
text = "Hello"
if len(text) > 10:
    print(text[10])
else:
    print("索引超出范围")
```

预防措施：使用len()检查字符串长度
""")

# --------------------------------------------
# 4.3 错误3：切片的左闭右开
# --------------------------------------------

print("\n4.3 错误3：切片的左闭右开")
print("-" * 60)

print("""
❌ 常见误解：
```python
text = "Python"
# 想获取"Pyt"（前3个字符）
result = text[0:3]  # 有人以为这会包含索引3
```

结果：result = "Pyt"（正确，但容易误解）

原因：切片是左闭右开 [start, end)
- 包含start索引
- 不包含end索引

✅ 正确理解：
```python
text = "Python"
result = text[0:3]  # 获取索引0, 1, 2（不包含3）
# 等价于
result = text[:3]   # 省略start，默认从0开始
```

记忆技巧：end索引是"到哪里停止"，不包含该位置
""")

# --------------------------------------------
# 4.4 错误4：find()返回-1
# --------------------------------------------

print("\n4.4 错误4：find()返回-1")
print("-" * 60)

print("""
❌ 错误代码：
```python
text = "Hello World"
index = text.find("Python")
print(text[index])  # 使用-1作为索引
```

结果：打印最后一个字符"d"（因为-1是反向索引）

原因：find()找不到时返回-1，而-1是有效的反向索引

✅ 正确代码：
```python
text = "Hello World"
index = text.find("Python")
if index != -1:
    print(text[index])
else:
    print("未找到")
```

预防措施：使用find()后必须检查返回值是否为-1
""")

# --------------------------------------------
# 4.5 错误5：split()的空字符串
# --------------------------------------------

print("\n4.5 错误5：split()的空字符串")
print("-" * 60)

print("""
❌ 错误代码：
```python
text = "a,b,,d"  # 注意中间有两个逗号
parts = text.split(',')
print(parts)
```

结果：['a', 'b', '', 'd']（包含空字符串）

原因：split()会保留空字符串

✅ 正确代码（如果要过滤空字符串）：
```python
text = "a,b,,d"
parts = [p for p in text.split(',') if p]
# 或者
parts = list(filter(None, text.split(',')))
```

结果：['a', 'b', 'd']

记忆技巧：split()忠实地分割，不会自动过滤空字符串
""")

# --------------------------------------------
# 4.6 错误6：字符串和数字拼接
# --------------------------------------------

print("\n4.6 错误6：字符串和数字拼接")
print("-" * 60)

print("""
❌ 错误代码：
```python
age = 25
message = "我今年" + age + "岁"
```

结果：TypeError: can only concatenate str (not "int") to str

原因：不能直接用+拼接字符串和数字

✅ 正确代码：
```python
age = 25
# 方法1：转换为字符串
message = "我今年" + str(age) + "岁"
# 方法2：使用f-string（推荐）
message = f"我今年{age}岁"
```

记忆技巧：+只能拼接相同类型，使用f-string最方便
""")

# --------------------------------------------
# 4.7 错误7：strip()只去除两端
# --------------------------------------------

print("\n4.7 错误7：strip()只去除两端")
print("-" * 60)

print("""
❌ 常见误解：
```python
text = "  Hello  World  "
result = text.strip()
print(result)
```

结果："Hello  World"（中间的空格还在）

原因：strip()只去除两端的空白，不去除中间的

✅ 如果要去除所有空格：
```python
text = "  Hello  World  "
# 方法1：replace
result = text.replace(" ", "")
# 方法2：split + join
result = "".join(text.split())
```

结果："HelloWorld"

记忆技巧：strip是"剥离"两端，不是"清除"所有
""")

# ============================================
# 第五部分：函数卡片速查
# ============================================

print("\n" + "=" * 60)
print("第五部分：函数卡片速查")
print("=" * 60)

print("""
【索引和切片】

语法：text[index] 或 text[start:end:step]
参数：
  - index: 整数索引（0开始，-1表示最后一个）
  - start: 起始索引（包含），默认0
  - end: 结束索引（不包含），默认到末尾
  - step: 步长，默认1
返回：单个字符或子串

示例：
text = "Python"
text[0]      # 'P'
text[1:4]    # 'yth'
text[::-1]   # 'nohtyP'（反转）

────────────────────────────────────

【upper() / lower()】

语法：text.upper() 或 text.lower()
参数：无
返回：新字符串（全大写/全小写）

示例：
"Hello".upper()  # "HELLO"
"Hello".lower()  # "hello"

────────────────────────────────────

【strip() / lstrip() / rstrip()】

语法：text.strip([chars])
参数：
  - chars: 要去除的字符集（可选），默认空白字符
返回：新字符串（去除两端/左端/右端指定字符）

示例：
"  hello  ".strip()   # "hello"
"###hello###".strip('#')  # "hello"

────────────────────────────────────

【replace()】

语法：text.replace(old, new[, count])
参数：
  - old: 要替换的子串
  - new: 替换成的子串
  - count: 替换次数（可选），默认全部替换
返回：新字符串

示例：
"hello".replace('l', 'L')     # "heLLo"
"hello".replace('l', 'L', 1)  # "heLlo"

────────────────────────────────────

【find() / index()】

语法：text.find(sub[, start[, end]])
参数：
  - sub: 要查找的子串
  - start: 起始位置（可选）
  - end: 结束位置（可选）
返回：
  - find(): 子串的索引，找不到返回-1
  - index(): 子串的索引，找不到抛出异常

示例：
"hello".find('l')    # 2
"hello".find('x')    # -1
"hello".index('l')   # 2

────────────────────────────────────

【split() / join()】

语法：
  - text.split([sep[, maxsplit]])
  - sep.join(iterable)
参数：
  - sep: 分隔符（可选），默认空白字符
  - maxsplit: 最大分割次数（可选）
  - iterable: 可迭代对象（如列表）
返回：
  - split(): 列表
  - join(): 字符串

示例：
"a,b,c".split(',')        # ['a', 'b', 'c']
" ".join(['a', 'b', 'c']) # "a b c"

────────────────────────────────────

【startswith() / endswith()】

语法：text.startswith(prefix) 或 text.endswith(suffix)
参数：
  - prefix/suffix: 前缀/后缀字符串
返回：布尔值（True/False）

示例：
"hello.py".endswith('.py')  # True
"hello.py".startswith('he') # True

────────────────────────────────────

【count()】

语法：text.count(sub[, start[, end]])
参数：
  - sub: 要统计的子串
  - start: 起始位置（可选）
  - end: 结束位置（可选）
返回：整数（出现次数）

示例：
"hello".count('l')  # 2

────────────────────────────────────

【isdigit() / isalpha() / isalnum()】

语法：text.isdigit() / text.isalpha() / text.isalnum()
参数：无
返回：布尔值
  - isdigit(): 是否全是数字
  - isalpha(): 是否全是字母
  - isalnum(): 是否全是字母或数字

示例：
"123".isdigit()    # True
"abc".isalpha()    # True
"abc123".isalnum() # True
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("练习题")
print("=" * 60)

print("""
1. 字符串切片练习
   给定字符串 text = "Python Programming"
   要求：
   - 获取前6个字符
   - 获取最后11个字符
   - 反转整个字符串
   - 获取所有偶数位置的字符
   提示：使用切片 [start:end:step]

2. 字符串方法练习
   给定字符串 email = "  USER@EXAMPLE.COM  "
   要求：
   - 去除两端空格
   - 转换为小写
   - 检查是否包含"@"
   - 分割成用户名和域名两部分
   提示：使用strip(), lower(), find(), split()

3. 字符串替换练习
   给定字符串 text = "I love Java. Java is great!"
   要求：
   - 将所有"Java"替换为"Python"
   - 统计"Java"出现的次数
   提示：使用replace(), count()

4. 字符串验证练习
   编写代码验证用户输入：
   - 检查是否为纯数字
   - 检查是否为纯字母
   - 检查是否以".py"结尾
   提示：使用isdigit(), isalpha(), endswith()

5. 字符串格式化练习
   给定：name = "张三", age = 25, city = "北京"
   要求：
   - 使用f-string格式化输出
   - 使用+拼接输出
   - 使用join()拼接输出
   提示：三种方法都要尝试

6. 综合练习：密码验证
   编写代码检查密码是否符合要求：
   - 长度至少8个字符
   - 包含字母和数字
   - 不包含空格
   提示：使用len(), isalnum(), find()

7. 综合练习：文本统计
   给定一段文本，统计：
   - 总字符数
   - 单词数（按空格分割）
   - 某个单词出现的次数
   提示：使用len(), split(), count()
""")

print("\n" + "=" * 60)
print("学习完成！")
print("=" * 60)
print("""
恭喜你完成了字符串操作的学习！

你已经掌握：
✅ 字符串索引和切片
✅ 15+个常用字符串方法
✅ 字符串的不可变性
✅ 常见错误和陷阱

下一步：
→ 完成练习题，巩固所学
→ 继续学习运算符（05_运算符_详解.py）
→ 完成文本格式化工具项目

加油！💪
""")
