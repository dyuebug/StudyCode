# ============================================
# 文本格式化工具 - 综合项目
# 日期：2026-02-17（第2天）
# 难度：⭐⭐ 中级
# ============================================

"""
【项目简介】文本格式化工具

功能：
- 文本大小写转换
- 文本对齐和填充
- 文本统计分析
- 简单的文本加密（凯撒密码）

涉及知识点：
- 字符串方法（upper、lower、strip、center等）
- 字符串索引和切片
- 运算符（算术、比较、逻辑）
- 输入输出
- 条件判断（简单使用）
"""

print("=" * 60)
print("文本格式化工具")
print("=" * 60)

# ============================================
# 功能1：文本大小写转换
# ============================================

print("\n" + "=" * 60)
print("功能1：文本大小写转换")
print("=" * 60)

# 获取用户输入
text = input("请输入一段文本：")

# 显示原文
print(f"\n原文：{text}")

# 转换为大写
upper_text = text.upper()
print(f"全大写：{upper_text}")

# 转换为小写
lower_text = text.lower()
print(f"全小写：{lower_text}")

# 首字母大写
capitalized_text = text.capitalize()
print(f"首字母大写：{capitalized_text}")

# 每个单词首字母大写
title_text = text.title()
print(f"标题格式：{title_text}")

# 大小写互换
swapped_text = text.swapcase()
print(f"大小写互换：{swapped_text}")

# ============================================
# 功能2：文本对齐和填充
# ============================================

print("\n" + "=" * 60)
print("功能2：文本对齐和填充")
print("=" * 60)

# 获取用户输入
text = input("请输入要对齐的文本：")
width = int(input("请输入总宽度："))

# 左对齐
left_aligned = text.ljust(width)
print(f"左对齐：|{left_aligned}|")

# 右对齐
right_aligned = text.rjust(width)
print(f"右对齐：|{right_aligned}|")

# 居中对齐
centered = text.center(width)
print(f"居中：|{centered}|")

# 使用指定字符填充
centered_with_char = text.center(width, '*')
print(f"居中（*填充）：|{centered_with_char}|")

# 数字补零（常用于格式化）
number = "42"
padded_number = number.zfill(5)  # 补零到5位
print(f"数字补零：{padded_number}")  # 00042

# ============================================
# 功能3：文本统计分析
# ============================================

print("\n" + "=" * 60)
print("功能3：文本统计分析")
print("=" * 60)

# 获取用户输入
text = input("请输入一段文本：")

# 统计总字符数（包括空格）
total_chars = len(text)
print(f"总字符数：{total_chars}")

# 去除空格后的字符数
chars_without_spaces = len(text.replace(" ", ""))
print(f"不含空格的字符数：{chars_without_spaces}")

# 统计单词数（按空格分割）
words = text.split()
word_count = len(words)
print(f"单词数：{word_count}")

# 统计特定字符出现次数
char_to_count = input("请输入要统计的字符：")
char_count = text.count(char_to_count)
print(f"字符 '{char_to_count}' 出现了 {char_count} 次")

# 统计空格数
space_count = text.count(" ")
print(f"空格数：{space_count}")

# 查找子串位置
substring = input("请输入要查找的子串：")
position = text.find(substring)
if position != -1:
    print(f"子串 '{substring}' 在位置 {position}")
else:
    print(f"未找到子串 '{substring}'")

# 检查文本类型
print(f"\n文本类型分析：")
print(f"是否全是数字：{text.isdigit()}")
print(f"是否全是字母：{text.isalpha()}")
print(f"是否全是字母或数字：{text.isalnum()}")
print(f"是否全是小写：{text.islower()}")
print(f"是否全是大写：{text.isupper()}")

# ============================================
# 功能4：文本清理和格式化
# ============================================

print("\n" + "=" * 60)
print("功能4：文本清理和格式化")
print("=" * 60)

# 获取用户输入（可能包含多余空格）
text = input("请输入文本（可能包含多余空格）：")

# 显示原文（用引号标记边界）
print(f"原文：'{text}'")

# 去除两端空格
trimmed = text.strip()
print(f"去除两端空格：'{trimmed}'")

# 去除左端空格
left_trimmed = text.lstrip()
print(f"去除左端空格：'{left_trimmed}'")

# 去除右端空格
right_trimmed = text.rstrip()
print(f"去除右端空格：'{right_trimmed}'")

# 替换多余空格为单个空格
# 方法：split()会按空白分割，join()用单个空格连接
cleaned = " ".join(text.split())
print(f"清理多余空格：'{cleaned}'")

# 移除特定字符
chars_to_remove = input("请输入要移除的字符：")
removed = text.replace(chars_to_remove, "")
print(f"移除 '{chars_to_remove}'：'{removed}'")

# ============================================
# 功能5：简单文本加密（凯撒密码）
# ============================================

print("\n" + "=" * 60)
print("功能5：简单文本加密（凯撒密码）")
print("=" * 60)

print("""
凯撒密码说明：
- 将每个字母向后移动固定位数
- 例如：移动3位，A→D, B→E, C→F
- 只加密字母，数字和符号不变
""")

# 获取用户输入
text = input("请输入要加密的文本（仅英文）：")
shift = int(input("请输入移动位数（1-25）："))

# 加密函数
encrypted = ""
for char in text:
    if char.isalpha():  # 只加密字母
        # 判断大小写
        if char.isupper():
            # 大写字母：A=65, Z=90
            # 公式：((字符码 - 65 + 移动位数) % 26) + 65
            encrypted_char = chr((ord(char) - 65 + shift) % 26 + 65)
        else:
            # 小写字母：a=97, z=122
            # 公式：((字符码 - 97 + 移动位数) % 26) + 97
            encrypted_char = chr((ord(char) - 97 + shift) % 26 + 97)
        encrypted += encrypted_char
    else:
        # 非字母字符保持不变
        encrypted += char

print(f"\n原文：{text}")
print(f"密文：{encrypted}")

# 解密（向相反方向移动）
decrypted = ""
for char in encrypted:
    if char.isalpha():
        if char.isupper():
            decrypted_char = chr((ord(char) - 65 - shift) % 26 + 65)
        else:
            decrypted_char = chr((ord(char) - 97 - shift) % 26 + 97)
        decrypted += decrypted_char
    else:
        decrypted += char

print(f"解密：{decrypted}")

# ============================================
# 🔍 深入理解：凯撒密码的原理（选学）
# ============================================

print("\n" + "=" * 60)
print("🔍 深入理解：凯撒密码的原理（选学）")
print("=" * 60)

print("""
如果你想知道"凯撒密码是如何工作的"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【ASCII码】

每个字符都有一个数字编码（ASCII码）：
- 'A' = 65, 'B' = 66, ..., 'Z' = 90
- 'a' = 97, 'b' = 98, ..., 'z' = 122

【ord()和chr()函数】

ord(char)：字符 → ASCII码
chr(code)：ASCII码 → 字符

示例：
ord('A')  # 65
chr(65)   # 'A'

【加密步骤】

以 'A' 移动3位为例：

1. 获取ASCII码：ord('A') = 65
2. 转换为0-25范围：65 - 65 = 0
3. 加上移动位数：0 + 3 = 3
4. 取模26（循环）：3 % 26 = 3
5. 转换回ASCII码：3 + 65 = 68
6. 转换为字符：chr(68) = 'D'

【为什么要取模26？】

因为英文字母只有26个，移动超过26位会循环：
- 'Z' 移动1位 → 'A'（循环回开头）
- 'Y' 移动3位 → 'B'

公式中的 % 26 确保结果在0-25范围内。

【实际应用】

凯撒密码很容易破解（只有25种可能），但它展示了：
- 字符编码的概念
- 模运算的应用
- 加密解密的对称性

现代加密算法（如AES、RSA）原理类似但复杂得多。
""")

# ============================================
# 功能6：文本格式化综合应用
# ============================================

print("\n" + "=" * 60)
print("功能6：文本格式化综合应用 - 名片生成器")
print("=" * 60)

# 获取用户信息
name = input("请输入姓名：")
title = input("请输入职位：")
company = input("请输入公司：")
phone = input("请输入电话：")
email = input("请输入邮箱：")

# 生成名片
card_width = 50
border = "=" * card_width

print(f"\n{border}")
print(name.center(card_width))
print(title.center(card_width))
print("-" * card_width)
print(company.center(card_width))
print("-" * card_width)
print(f"电话：{phone}".center(card_width))
print(f"邮箱：{email}".center(card_width))
print(border)

# ============================================
# 功能7：文本模板填充
# ============================================

print("\n" + "=" * 60)
print("功能7：文本模板填充")
print("=" * 60)

# 定义模板
template = """
尊敬的 {name}：

您好！

感谢您购买我们的产品 {product}。
您的订单号是：{order_id}
订单金额：{amount} 元

我们将在 {days} 个工作日内发货。

祝您生活愉快！

{company}
{date}
"""

# 获取用户输入
name = input("请输入客户姓名：")
product = input("请输入产品名称：")
order_id = input("请输入订单号：")
amount = input("请输入金额：")
days = input("请输入发货天数：")
company = input("请输入公司名称：")
date = input("请输入日期：")

# 填充模板
filled_template = template.format(
    name=name,
    product=product,
    order_id=order_id,
    amount=amount,
    days=days,
    company=company,
    date=date
)

print("\n生成的邮件：")
print(filled_template)

# ============================================
# 常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("项目中的常见错误和陷阱")
print("=" * 60)

print("""
1. 忘记检查find()的返回值
   ❌ 错误：
   index = text.find("xxx")
   print(text[index])  # 如果找不到，index=-1，会打印最后一个字符

   ✅ 正确：
   index = text.find("xxx")
   if index != -1:
       print(text[index])

2. 字符串方法不会修改原字符串
   ❌ 错误：
   text = "hello"
   text.upper()  # 没有保存结果
   print(text)   # 还是 "hello"

   ✅ 正确：
   text = "hello"
   text = text.upper()  # 保存结果
   print(text)  # "HELLO"

3. split()产生的空字符串
   ❌ 问题：
   text = "a,,b"
   parts = text.split(',')  # ['a', '', 'b']

   ✅ 过滤空字符串：
   parts = [p for p in text.split(',') if p]

4. 凯撒密码的边界情况
   ❌ 问题：
   'Z' 移动1位应该是 'A'，但如果不用%会变成 '['

   ✅ 使用模运算：
   (ord('Z') - 65 + 1) % 26 + 65  # 正确循环到 'A'

5. input()返回字符串
   ❌ 错误：
   width = input("输入宽度：")
   text.center(width)  # TypeError

   ✅ 正确：
   width = int(input("输入宽度："))
   text.center(width)
""")

# ============================================
# 练习题
# ============================================

print("\n" + "=" * 60)
print("练习题")
print("=" * 60)

print("""
1. 文本统计增强
   扩展文本统计功能，增加：
   - 统计每个单词出现的次数
   - 找出最长的单词
   - 统计元音字母（a,e,i,o,u）的数量
   提示：使用字典统计单词频率

2. 文本清理工具
   编写工具清理文本：
   - 移除所有标点符号
   - 移除所有数字
   - 移除多余空格
   提示：使用replace()或字符串方法

3. 密码强度检查
   编写工具检查密码强度：
   - 长度至少8位
   - 包含大写字母
   - 包含小写字母
   - 包含数字
   - 包含特殊字符
   提示：使用isupper()、islower()、isdigit()

4. 文本对比工具
   比较两段文本：
   - 是否完全相同
   - 忽略大小写是否相同
   - 忽略空格是否相同
   提示：使用lower()、replace()

5. 简易文本编辑器
   实现基本编辑功能：
   - 查找并替换
   - 插入文本
   - 删除文本
   提示：使用replace()、切片

6. ROT13加密
   实现ROT13加密（移动13位的凯撒密码）：
   - 特点：加密和解密使用相同操作
   - 因为26 / 2 = 13
   提示：修改凯撒密码代码，固定shift=13

7. 文本格式转换
   实现格式转换：
   - 驼峰命名 → 下划线命名（myName → my_name）
   - 下划线命名 → 驼峰命名（my_name → myName）
   提示：使用split()、join()、title()

8. 综合项目：文本分析报告
   生成完整的文本分析报告：
   - 基本统计（字符数、单词数、行数）
   - 词频统计（前10个最常见单词）
   - 格式分析（大小写分布）
   - 生成格式化的报告
   提示：综合使用所有学过的方法
""")

print("\n" + "=" * 60)
print("项目完成！")
print("=" * 60)
print("""
🎉 恭喜你完成了文本格式化工具项目！

你已经掌握：
✅ 字符串的各种操作方法
✅ 文本统计和分析
✅ 文本格式化和对齐
✅ 简单的加密算法
✅ 综合运用字符串和运算符

项目亮点：
⭐ 实用的文本处理功能
⭐ 有趣的凯撒密码实现
⭐ 专业的名片生成器
⭐ 灵活的模板填充系统

下一步：
→ 尝试完成练习题，扩展项目功能
→ 思考如何改进用户体验
→ 准备学习第3天的内容

你做得很棒！继续加油！💪
""")
