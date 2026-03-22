# ============================================
# 文件操作 - 超详细注释版
# 日期：2026-03-20（第8天）
# 难度：⭐⭐ 中级
# ============================================

"""
【核心概念】文件操作

是什么？
- 程序读写硬盘上文件的能力
- 让数据可以「持久化」：程序关闭后数据不丢失

为什么需要？
- 保存用户数据（如成绩、配置）
- 读取配置文件、数据文件
- 生成报告、日志

核心特点：
- open() 打开文件，用完必须 close()
- 推荐用 with 语句，自动关闭
- 必须指定编码：encoding='utf-8'
"""

import os

# ============================================
# 第一部分：open() 函数详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：open() 函数详解")
print("=" * 60)

print("""
【open() 函数卡片】

语法：open(file, mode='r', encoding=None)

参数详解：
  file      → 文件路径（字符串）
  mode      → 打开模式（见下表）
  encoding  → 文件编码，中文文件必须指定 'utf-8'

文件模式（mode）：
  'r'   → 读取（默认），文件不存在则报错
  'w'   → 写入（覆盖），文件不存在则创建，存在则清空
  'a'   → 追加，文件不存在则创建，存在则追加到末尾
  'r+'  → 读写
  'rb'  → 二进制读取（图片、音频等）
  'wb'  → 二进制写入

返回值：文件对象（file object）

⚠️ 重要：打开文件后必须关闭！推荐用 with 语句。
""")

# ============================================
# 第二部分：写文件实践
# ============================================

print("\n" + "=" * 60)
print("第二部分：写文件实践")
print("=" * 60)

# 测试文件路径（在当前目录创建）
test_file = "test_output.txt"

# --------------------------------------------
# 2.1 写文件（with 语句 - 推荐方式）
# --------------------------------------------

print("\n2.1 写文件（with 语句）")
print("-" * 60)

print("""
【with 语句】

语法：
  with open(文件, 模式, encoding='utf-8') as f:
      f.write(内容)

优点：
  - 自动关闭文件（即使发生错误）
  - 代码更简洁
  - 是 Python 推荐的标准写法

对比不用 with：
  f = open('file.txt', 'w')
  f.write('内容')
  f.close()  # 必须手动关闭，容易忘！
""")

# 用 with 语句写文件
# 'w' 模式：写入（覆盖），文件不存在则创建
# encoding='utf-8'：指定编码，支持中文
with open(test_file, 'w', encoding='utf-8') as f:
    # write()：写入字符串，不自动换行
    # 需要手动加 \n 换行
    f.write("第一行：Python文件操作\n")
    f.write("第二行：with语句真好用\n")
    f.write("第三行：自动关闭文件\n")

print(f"已写入文件：{test_file}")

# --------------------------------------------
# 2.2 追加写入
# --------------------------------------------

print("\n2.2 追加写入（'a' 模式）")
print("-" * 60)

# 'a' 模式：追加，不覆盖原有内容
with open(test_file, 'a', encoding='utf-8') as f:
    f.write("第四行：追加的内容\n")
    f.write("第五行：不会覆盖原来的内容\n")

print(f"已追加内容到：{test_file}")

# --------------------------------------------
# 2.3 写入多行（writelines）
# --------------------------------------------

print("\n2.3 writelines() 写入列表")
print("-" * 60)

lines = [
    "苹果\n",
    "香蕉\n",
    "橙子\n",
    "葡萄\n",
]

fruits_file = "fruits.txt"
# writelines()：把列表中每个字符串写入文件
# 注意：不自动加换行，需要在每个字符串末尾加 \n
with open(fruits_file, 'w', encoding='utf-8') as f:
    f.writelines(lines)

print(f"已写入水果列表到：{fruits_file}")

# ============================================
# 第三部分：读文件实践
# ============================================

print("\n" + "=" * 60)
print("第三部分：读文件实践")
print("=" * 60)

# --------------------------------------------
# 3.1 read() - 一次读取全部内容
# --------------------------------------------

print("\n3.1 read() - 读取全部内容")
print("-" * 60)

print("""
【三种读取方式】
  read()       → 一次读取全部，返回字符串
  readline()   → 读取一行，返回字符串
  readlines()  → 读取所有行，返回列表
""")

# read()：读取文件全部内容，返回一个字符串
with open(test_file, 'r', encoding='utf-8') as f:
    content = f.read()  # 返回字符串

print("read() 结果：")
print(content)

# --------------------------------------------
# 3.2 readlines() - 读取为列表
# --------------------------------------------

print("3.2 readlines() - 读取为列表")
print("-" * 60)

# readlines()：读取所有行，返回列表
# 每个元素是一行，末尾包含 \n
with open(test_file, 'r', encoding='utf-8') as f:
    lines = f.readlines()  # 返回列表

print(f"共 {len(lines)} 行：")
for i, line in enumerate(lines, 1):
    # strip() 去掉末尾的 \n
    print(f"  第{i}行：{line.strip()}")

# --------------------------------------------
# 3.3 逐行读取（最省内存）
# --------------------------------------------

print("\n3.3 逐行读取（推荐方式，最省内存）")
print("-" * 60)

# 直接用 for 循环遍历文件对象
# 每次只读一行，适合大文件
print("逐行读取：")
with open(fruits_file, 'r', encoding='utf-8') as f:
    for line in f:  # 直接遍历文件对象
        # strip() 去掉首尾空白（包括\n）
        print(f"  水果：{line.strip()}")

# ============================================
# 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
错误1：忘记指定编码导致中文乱码

❌ 错误代码：
  with open('中文.txt', 'w') as f:  # 没有 encoding
      f.write('你好')              # 可能乱码或报错

✅ 正确代码：
  with open('中文.txt', 'w', encoding='utf-8') as f:
      f.write('你好')              # 正常写入中文
""")

print("""
错误2：'w' 模式会覆盖文件

❌ 危险操作：
  # 文件已有100行重要数据
  with open('data.txt', 'w') as f:  # 'w' 会清空文件！
      f.write('新内容')             # 原来100行全没了！

✅ 追加用 'a' 模式：
  with open('data.txt', 'a', encoding='utf-8') as f:
      f.write('新内容')  # 保留原有内容，追加到末尾
""")

print("""
错误3：读不存在的文件

❌ 错误代码：
  with open('不存在.txt', 'r') as f:  # FileNotFoundError
      content = f.read()

✅ 正确代码（先判断是否存在）：
  import os
  if os.path.exists('文件.txt'):
      with open('文件.txt', 'r', encoding='utf-8') as f:
          content = f.read()
  else:
      print('文件不存在')
""")

print("""
错误4：write() 写入非字符串

❌ 错误代码：
  with open('data.txt', 'w') as f:
      f.write(123)      # TypeError！write只接受字符串
      f.write([1,2,3])  # TypeError！

✅ 正确代码：
  with open('data.txt', 'w', encoding='utf-8') as f:
      f.write(str(123))          # 转为字符串
      f.write(str([1,2,3]))      # 转为字符串
""")

print("""
错误5：readline() 末尾有 \\n

❌ 误解：
  line = f.readline()
  print(line)  # 输出："第一行\n"  多了一个换行

✅ 正确处理：
  line = f.readline().strip()  # strip() 去掉首尾空白和\n
  print(line)  # 输出："第一行"
""")

# ============================================
# 第五部分：速查卡
# ============================================

print("\n" + "=" * 60)
print("第五部分：文件操作速查卡")
print("=" * 60)

print("""
【文件模式速查】
  'r'  → 读取（默认，文件必须存在）
  'w'  → 写入（覆盖，不存在则创建）
  'a'  → 追加（不覆盖，不存在则创建）
  'rb' → 二进制读（图片/音频）
  'wb' → 二进制写（图片/音频）

【读取方法速查】
  f.read()       → 全部内容（字符串）
  f.readline()   → 一行（字符串，含\n）
  f.readlines()  → 全部行（列表，每项含\n）
  for line in f  → 逐行（推荐，省内存）

【写入方法速查】
  f.write(str)        → 写入字符串
  f.writelines(list)  → 写入字符串列表

【标准模板】
  # 写文件
  with open('file.txt', 'w', encoding='utf-8') as f:
      f.write('内容\n')

  # 读文件
  with open('file.txt', 'r', encoding='utf-8') as f:
      content = f.read()

  # 追加文件
  with open('file.txt', 'a', encoding='utf-8') as f:
      f.write('追加内容\n')
""")

# ============================================
# 第六部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第六部分：练习题（自己动手！）")
print("=" * 60)

print("""
练习1：日记本
  要求：创建一个 diary.txt
       写入今天的日期和一段文字
       再次运行时，追加新的日记（不覆盖之前的）
  提示：用 'a' 模式追加，用 datetime 获取当前日期

练习2：单词计数器
  要求：创建一个 words.txt，写入一段英文文章
       读取文件，统计每个单词出现的次数
       按出现次数从多到少排序输出
  提示：split() 分割，字典统计，sorted() 排序

练习3：数字求和
  要求：创建 numbers.txt，每行写一个数字（如1到10）
       读取文件，计算所有数字的总和和平均值
  提示：readlines() + int() 转换 + 注意 strip()

练习4：文件备份
  要求：读取一个文件的内容
       把内容写入一个新文件（文件名加 _backup 后缀）
  提示：os.path.splitext() 分离文件名和扩展名
""")

# 清理测试文件
for f in [test_file, fruits_file]:
    if os.path.exists(f):
        os.remove(f)
print("\n（测试文件已清理）")
