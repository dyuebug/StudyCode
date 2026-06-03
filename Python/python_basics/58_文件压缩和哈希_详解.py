# ============================================
# zipfile 和 hashlib 模块 - 超详细注释版
# 日期：2026-05-31（第23天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【核心概念】zipfile 和 hashlib 模块

zipfile 模块：
- 用于创建、读取、写入、追加 ZIP 压缩文件
- 支持压缩和解压缩
- 跨平台，Windows/Linux/Mac 通用

hashlib 模块：
- 提供多种哈希算法（MD5、SHA1、SHA256 等）
- 用于计算文件的哈希值（指纹）
- 检测文件是否被修改

为什么需要？
- 备份：压缩文件节省空间
- 校验：哈希值确保文件完整性
- 增量备份：通过哈希值检测文件变化
"""

import zipfile
import hashlib
from pathlib import Path
import os

# ============================================
# 第一部分：zipfile 模块详解
# ============================================

print("第一部分：zipfile 模块详解")
print("=" * 60)

# --------------------------------------------
# 1.1 创建 ZIP 文件
# --------------------------------------------

print("\n1.1 创建 ZIP 文件")
print("-" * 60)

print("""
【创建 ZIP 文件】

基本语法：
  with zipfile.ZipFile('archive.zip', 'w') as zf:
      zf.write('file.txt')

模式：
  'w' - 写入模式（创建新文件，覆盖已存在的）
  'a' - 追加模式（添加到已存在的 ZIP）
  'r' - 读取模式（只读）

压缩级别：
  zipfile.ZIP_STORED    # 不压缩（只打包）
  zipfile.ZIP_DEFLATED  # 标准压缩（推荐）
""")

# 示例：创建 ZIP 文件
print("示例：创建 ZIP 文件")

# 创建测试文件
test_dir = Path("test_zip")
test_dir.mkdir(exist_ok=True)

file1 = test_dir / "file1.txt"
file2 = test_dir / "file2.txt"
file1.write_text("这是文件1的内容\n" * 10)
file2.write_text("这是文件2的内容\n" * 10)

# 创建 ZIP 文件
zip_path = Path("test_archive.zip")
with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
    zf.write(file1, arcname=file1.name)  # arcname 指定压缩包内的文件名
    zf.write(file2, arcname=file2.name)

print(f"创建 ZIP 文件：{zip_path}")
print(f"原始大小：{file1.stat().st_size + file2.stat().st_size} 字节")
print(f"压缩后大小：{zip_path.stat().st_size} 字节")

print()

# --------------------------------------------
# 1.2 读取 ZIP 文件
# --------------------------------------------

print("\n1.2 读取 ZIP 文件")
print("-" * 60)

print("""
【读取 ZIP 文件】

列出文件：
  with zipfile.ZipFile('archive.zip', 'r') as zf:
      for name in zf.namelist():
          print(name)

获取文件信息：
  info = zf.getinfo('file.txt')
  print(info.file_size)      # 原始大小
  print(info.compress_size)  # 压缩后大小

读取文件内容：
  content = zf.read('file.txt')  # 返回字节
""")

# 示例：读取 ZIP 文件
print("示例：读取 ZIP 文件")

with zipfile.ZipFile(zip_path, 'r') as zf:
    print("压缩包内的文件：")
    for name in zf.namelist():
        info = zf.getinfo(name)
        ratio = (1 - info.compress_size / info.file_size) * 100
        print(f"  {name}: {info.file_size} -> {info.compress_size} 字节 (压缩率 {ratio:.1f}%)")

print()

# --------------------------------------------
# 1.3 解压 ZIP 文件
# --------------------------------------------

print("\n1.3 解压 ZIP 文件")
print("-" * 60)

print("""
【解压 ZIP 文件】

解压所有文件：
  with zipfile.ZipFile('archive.zip', 'r') as zf:
      zf.extractall('output_dir')

解压单个文件：
  zf.extract('file.txt', 'output_dir')

解压到内存：
  content = zf.read('file.txt')
""")

# 示例：解压 ZIP 文件
print("示例：解压 ZIP 文件")

extract_dir = Path("test_extract")
extract_dir.mkdir(exist_ok=True)

with zipfile.ZipFile(zip_path, 'r') as zf:
    zf.extractall(extract_dir)

print(f"解压到：{extract_dir}")
print("解压的文件：")
for file in extract_dir.iterdir():
    print(f"  {file.name}")

print()

# ============================================
# 第二部分：hashlib 模块详解
# ============================================

print("=" * 60)
print("第二部分：hashlib 模块详解")
print("=" * 60)

# --------------------------------------------
# 2.1 计算字符串哈希
# --------------------------------------------

print("\n2.1 计算字符串哈希")
print("-" * 60)

print("""
【哈希算法】

常用算法：
  hashlib.md5()      # MD5（128位，不安全，但快）
  hashlib.sha1()     # SHA-1（160位，不安全）
  hashlib.sha256()   # SHA-256（256位，推荐）
  hashlib.sha512()   # SHA-512（512位，更安全）

基本用法：
  h = hashlib.sha256()
  h.update(b'data')
  hash_value = h.hexdigest()  # 十六进制字符串
""")

# 示例：计算字符串哈希
print("示例：计算字符串哈希")

text = "Hello, World!"
data = text.encode('utf-8')  # 转为字节

# MD5
md5_hash = hashlib.md5(data).hexdigest()
print(f"MD5:    {md5_hash}")

# SHA-256
sha256_hash = hashlib.sha256(data).hexdigest()
print(f"SHA256: {sha256_hash}")

print()

# --------------------------------------------
# 2.2 计算文件哈希
# --------------------------------------------

print("\n2.2 计算文件哈希")
print("-" * 60)

print("""
【计算文件哈希】

分块读取（适合大文件）：
  h = hashlib.sha256()
  with open('file.txt', 'rb') as f:
      while chunk := f.read(8192):  # 每次读取 8KB
          h.update(chunk)
  hash_value = h.hexdigest()

为什么分块？
- 大文件一次性读入内存会占用大量内存
- 分块读取内存友好
""")

# 示例：计算文件哈希
print("示例：计算文件哈希")

def calculate_file_hash(file_path, algorithm='sha256'):
    """
    计算文件的哈希值

    参数：
        file_path - 文件路径
        algorithm - 哈希算法（'md5', 'sha256', 'sha512'）

    返回：
        哈希值（十六进制字符串）
    """
    h = hashlib.new(algorithm)

    with open(file_path, 'rb') as f:
        while chunk := f.read(8192):
            h.update(chunk)

    return h.hexdigest()

# 计算测试文件的哈希
file_hash = calculate_file_hash(file1)
print(f"文件：{file1.name}")
print(f"SHA256: {file_hash}")

print()

# --------------------------------------------
# 2.3 哈希的应用场景
# --------------------------------------------

print("\n2.3 哈希的应用场景")
print("-" * 60)

print("""
【哈希的应用】

1. 文件完整性校验
   - 下载文件后，对比哈希值
   - 确保文件未被篡改

2. 检测文件变化
   - 备份前计算哈希
   - 备份后对比哈希
   - 哈希不同 = 文件已修改

3. 去重
   - 相同内容的文件哈希相同
   - 通过哈希快速识别重复文件

4. 密码存储
   - 不存储明文密码
   - 存储密码的哈希值
""")

# 示例：检测文件变化
print("示例：检测文件变化")

# 原始哈希
original_hash = calculate_file_hash(file1)
print(f"原始哈希：{original_hash[:16]}...")

# 修改文件
file1.write_text("修改后的内容\n")

# 新哈希
new_hash = calculate_file_hash(file1)
print(f"新哈希：  {new_hash[:16]}...")

if original_hash != new_hash:
    print("检测到文件已修改！")

print()

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("""
==============================================
深入理解：哈希算法的选择（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【哈希算法对比】

MD5（128位）：
  优点：速度快
  缺点：已被破解，不安全
  用途：文件校验（非安全场景）

SHA-1（160位）：
  优点：比 MD5 安全
  缺点：也已被破解
  用途：Git 等版本控制（逐步淘汰）

SHA-256（256位）：
  优点：安全性高，速度适中
  缺点：比 MD5 慢
  用途：推荐用于文件校验和安全场景

SHA-512（512位）：
  优点：最安全
  缺点：最慢，哈希值长
  用途：高安全要求场景

【选择建议】

文件校验：SHA-256（推荐）
快速去重：MD5（速度优先）
安全场景：SHA-256 或 SHA-512
密码存储：bcrypt 或 argon2（专用算法）

==============================================
""")

# 示例：算法性能对比
print("算法性能对比：")

import time

test_data = b"x" * 1000000  # 1MB 数据

for algo in ['md5', 'sha1', 'sha256', 'sha512']:
    start = time.time()
    hashlib.new(algo, test_data).hexdigest()
    elapsed = time.time() - start
    print(f"  {algo:8s}: {elapsed*1000:.2f} ms")

print()

# ============================================
# 第四部分：常见错误和陷阱
# ============================================

print("=" * 60)
print("第四部分：常见错误和陷阱")
print("=" * 60)

print("""
[X] 错误1：忘记使用二进制模式

错误代码：
  with open('file.txt', 'r') as f:  # 文本模式
      h.update(f.read())  # TypeError

原因：哈希需要字节数据，不是字符串

[OK] 正确做法：
  with open('file.txt', 'rb') as f:  # 二进制模式
      h.update(f.read())

记忆技巧：哈希用 'rb'，文本用 'r'

────────────────────────────────────

[X] 错误2：大文件一次性读入内存

错误代码：
  with open('large_file.bin', 'rb') as f:
      h.update(f.read())  # 内存溢出

原因：大文件占用大量内存

[OK] 正确做法：
  with open('large_file.bin', 'rb') as f:
      while chunk := f.read(8192):
          h.update(chunk)

记忆技巧：分块读取，8192 字节（8KB）

────────────────────────────────────

[X] 错误3：ZIP 文件路径问题

错误代码：
  zf.write('/full/path/file.txt')  # 压缩包内包含完整路径

结果：解压时创建多级目录

[OK] 正确做法：
  zf.write('/full/path/file.txt', arcname='file.txt')

记忆技巧：用 arcname 指定压缩包内的文件名

────────────────────────────────────

[X] 错误4：忘记关闭 ZIP 文件

错误代码：
  zf = zipfile.ZipFile('archive.zip', 'w')
  zf.write('file.txt')
  # 忘记 zf.close()

结果：文件可能损坏

[OK] 正确做法：
  with zipfile.ZipFile('archive.zip', 'w') as zf:
      zf.write('file.txt')

记忆技巧：总是用 with 语句

────────────────────────────────────

[X] 错误5：哈希算法拼写错误

错误代码：
  h = hashlib.new('sha-256')  # 错误：有连字符

结果：ValueError: unsupported hash type

[OK] 正确做法：
  h = hashlib.new('sha256')  # 正确：无连字符

记忆技巧：算法名无连字符（md5, sha1, sha256）
""")

print()

# ============================================
# 第五部分：常用操作速查
# ============================================

print("=" * 60)
print("第五部分：常用操作速查")
print("=" * 60)

print("""
【zipfile 操作】

创建 ZIP：
  with zipfile.ZipFile('archive.zip', 'w', zipfile.ZIP_DEFLATED) as zf:
      zf.write('file.txt', arcname='file.txt')

读取 ZIP：
  with zipfile.ZipFile('archive.zip', 'r') as zf:
      names = zf.namelist()
      info = zf.getinfo('file.txt')
      content = zf.read('file.txt')

解压 ZIP：
  with zipfile.ZipFile('archive.zip', 'r') as zf:
      zf.extractall('output_dir')

【hashlib 操作】

计算字符串哈希：
  h = hashlib.sha256(b'data')
  hash_value = h.hexdigest()

计算文件哈希：
  h = hashlib.sha256()
  with open('file.txt', 'rb') as f:
      while chunk := f.read(8192):
          h.update(chunk)
  hash_value = h.hexdigest()

【常用算法】

hashlib.md5()      # 快速，不安全
hashlib.sha256()   # 推荐
hashlib.sha512()   # 最安全
""")

print()

# ============================================
# 第六部分：练习题
# ============================================

print("=" * 60)
print("第六部分：练习题")
print("=" * 60)

print("""
1. 编写函数，压缩指定目录下的所有文件
   提示：使用 Path.rglob('*') 和 zipfile

2. 编写函数，对比两个文件是否相同（通过哈希）
   提示：计算两个文件的 SHA-256 哈希值

3. 编写函数，查找目录中的重复文件
   提示：用字典存储 {哈希值: [文件列表]}

4. 编写函数，验证 ZIP 文件的完整性
   提示：解压后对比文件哈希

5. 思考题：
   - 为什么备份要用压缩？
   - 为什么增量备份需要哈希值？
   - MD5 和 SHA-256 有什么区别？
""")

print("=" * 60)
print("第23天 - zipfile 和 hashlib 模块详解 学习完成！")
print("=" * 60)

# 清理测试文件
import shutil
if test_dir.exists():
    shutil.rmtree(test_dir)
if extract_dir.exists():
    shutil.rmtree(extract_dir)
if zip_path.exists():
    zip_path.unlink()
print("\n测试文件已清理")
