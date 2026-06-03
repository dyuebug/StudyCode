# ============================================
# pathlib 模块 - 超详细注释版
# 日期：2026-05-31（第22天）
# 难度：⭐⭐⭐ 高级
# ============================================

"""
【核心概念】pathlib 模块

是什么？
- pathlib 是 Python 3.4+ 引入的面向对象的路径操作模块
- 提供了比 os.path 更优雅、更直观的路径操作方式

为什么需要？
- os.path 是字符串操作，容易出错
- pathlib 是对象操作，更安全、更易读
- 支持链式调用，代码更简洁

核心特点：
- 面向对象：Path 对象封装了所有路径操作
- 跨平台：自动处理 Windows/Linux/Mac 的路径差异
- 功能丰富：文件操作、目录遍历、路径拼接等
"""

from pathlib import Path
import os

# ============================================
# 第一部分：核心概念详解
# ============================================

print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 Path 对象基础
# --------------------------------------------

print("\n1.1 Path 对象基础")
print("-" * 60)

print("""
【Path 对象】

创建 Path 对象：
  Path('.')           # 当前目录
  Path('/usr/bin')    # 绝对路径
  Path('data/file.txt')  # 相对路径
  Path.cwd()          # 当前工作目录
  Path.home()         # 用户主目录

Path 对象的属性：
  path.name           # 文件名（包含扩展名）
  path.stem           # 文件名（不含扩展名）
  path.suffix         # 扩展名
  path.parent         # 父目录
  path.parts          # 路径各部分组成的元组
  path.is_absolute()  # 是否是绝对路径
""")

# 示例：Path 对象的基本使用
file_path = Path("data/reports/2024/report.txt")

print(f"完整路径：{file_path}")
print(f"文件名：{file_path.name}")
print(f"文件名（无扩展名）：{file_path.stem}")
print(f"扩展名：{file_path.suffix}")
print(f"父目录：{file_path.parent}")
print(f"路径各部分：{file_path.parts}")

print()

# --------------------------------------------
# 1.2 路径拼接
# --------------------------------------------

print("\n1.2 路径拼接")
print("-" * 60)

print("""
【路径拼接】

使用 / 运算符（推荐）：
  base = Path('data')
  file = base / 'reports' / 'report.txt'
  # 结果：data/reports/report.txt

使用 joinpath() 方法：
  file = base.joinpath('reports', 'report.txt')

优势：
  - 自动处理路径分隔符（Windows 用 \\，Linux 用 /）
  - 代码更简洁易读
  - 避免字符串拼接错误
""")

# 示例：路径拼接
base_dir = Path("data")
report_dir = base_dir / "reports" / "2024"
report_file = report_dir / "report.txt"

print(f"基础目录：{base_dir}")
print(f"报告目录：{report_dir}")
print(f"报告文件：{report_file}")

print()

# --------------------------------------------
# 1.3 文件和目录判断
# --------------------------------------------

print("\n1.3 文件和目录判断")
print("-" * 60)

print("""
【文件和目录判断】

path.exists()       # 路径是否存在
path.is_file()      # 是否是文件
path.is_dir()       # 是否是目录
path.is_symlink()   # 是否是符号链接
""")

# 示例：判断路径类型
current_file = Path(__file__)  # 当前脚本文件
current_dir = Path.cwd()       # 当前工作目录

print(f"当前文件：{current_file}")
print(f"  存在：{current_file.exists()}")
print(f"  是文件：{current_file.is_file()}")
print(f"  是目录：{current_file.is_dir()}")

print(f"\n当前目录：{current_dir}")
print(f"  存在：{current_dir.exists()}")
print(f"  是文件：{current_dir.is_file()}")
print(f"  是目录：{current_dir.is_dir()}")

print()

# ============================================
# 第二部分：基础操作实践
# ============================================

print("=" * 60)
print("第二部分：基础操作实践")
print("=" * 60)

# --------------------------------------------
# 2.1 目录遍历
# --------------------------------------------

print("\n2.1 目录遍历")
print("-" * 60)

print("""
【目录遍历方法】

iterdir()           # 遍历直接子项（不递归）
glob(pattern)       # 模式匹配（支持 * 和 ?）
rglob(pattern)      # 递归模式匹配（支持 **）

示例：
  # 遍历当前目录
  for item in Path('.').iterdir():
      print(item)

  # 查找所有 .py 文件
  for file in Path('.').glob('*.py'):
      print(file)

  # 递归查找所有 .txt 文件
  for file in Path('.').rglob('*.txt'):
      print(file)
""")

# 示例：遍历当前目录的 Python 文件
print("当前目录的 Python 文件（前5个）：")
py_files = list(Path('.').glob('*.py'))
for i, file in enumerate(py_files[:5], 1):
    print(f"  {i}. {file.name}")

print()

# --------------------------------------------
# 2.2 文件操作
# --------------------------------------------

print("\n2.2 文件操作")
print("-" * 60)

print("""
【文件操作方法】

读取文件：
  path.read_text()        # 读取文本文件
  path.read_bytes()       # 读取二进制文件

写入文件：
  path.write_text(text)   # 写入文本
  path.write_bytes(data)  # 写入二进制

打开文件：
  path.open('r')          # 打开文件（返回文件对象）

文件信息：
  path.stat()             # 文件状态（大小、修改时间等）
  path.stat().st_size     # 文件大小（字节）
  path.stat().st_mtime    # 最后修改时间
""")

# 示例：创建临时文件并读取
temp_file = Path("temp_test.txt")
temp_file.write_text("Hello, pathlib!")
content = temp_file.read_text()
print(f"临时文件内容：{content}")

# 获取文件信息
if temp_file.exists():
    stat = temp_file.stat()
    print(f"文件大小：{stat.st_size} 字节")

# 清理临时文件
temp_file.unlink()  # 删除文件
print("临时文件已删除")

print()

# --------------------------------------------
# 2.3 目录操作
# --------------------------------------------

print("\n2.3 目录操作")
print("-" * 60)

print("""
【目录操作方法】

创建目录：
  path.mkdir()                    # 创建目录
  path.mkdir(parents=True)        # 创建多级目录
  path.mkdir(exist_ok=True)       # 目录存在时不报错

删除目录：
  path.rmdir()                    # 删除空目录
  path.unlink()                   # 删除文件

重命名：
  path.rename(new_path)           # 重命名/移动

复制（需要 shutil）：
  import shutil
  shutil.copy(src, dst)           # 复制文件
  shutil.copytree(src, dst)       # 复制目录树
""")

# 示例：创建和删除目录
test_dir = Path("test_directory")

# 创建目录
if not test_dir.exists():
    test_dir.mkdir()
    print(f"创建目录：{test_dir}")

# 删除目录
if test_dir.exists():
    test_dir.rmdir()
    print(f"删除目录：{test_dir}")

print()

# ============================================
# 第三部分：深入理解（可选）
# ============================================

print("""
==============================================
深入理解：pathlib vs os.path（选学）
==============================================

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【对比：pathlib vs os.path】

获取文件名：
  # os.path
  filename = os.path.basename('/data/file.txt')

  # pathlib
  filename = Path('/data/file.txt').name

路径拼接：
  # os.path
  path = os.path.join('data', 'reports', 'file.txt')

  # pathlib
  path = Path('data') / 'reports' / 'file.txt'

判断文件是否存在：
  # os.path
  if os.path.exists(path) and os.path.isfile(path):
      ...

  # pathlib
  if Path(path).is_file():
      ...

【pathlib 的优势】

1. 面向对象：所有操作都是方法调用
2. 链式调用：path.parent.parent.name
3. 跨平台：自动处理路径分隔符
4. 类型安全：Path 对象而非字符串
5. 功能丰富：集成了文件操作

【何时使用 pathlib】

推荐：所有新项目都使用 pathlib
兼容：需要兼容 Python 3.3 及以下时用 os.path

==============================================
""")

# 示例：pathlib vs os.path 对比
print("pathlib vs os.path 对比：")

# 使用 os.path
import os
os_path = os.path.join('data', 'reports', 'file.txt')
os_name = os.path.basename(os_path)
print(f"os.path 方式：{os_path} -> {os_name}")

# 使用 pathlib
path_obj = Path('data') / 'reports' / 'file.txt'
path_name = path_obj.name
print(f"pathlib 方式：{path_obj} -> {path_name}")

print()

# ============================================
# 第四部分：常见错误和陷阱
# ============================================

print("=" * 60)
print("第四部分：常见错误和陷阱")
print("=" * 60)

print("""
[X] 错误1：混用 Path 对象和字符串

错误代码：
  path = Path('data')
  full_path = path + '/file.txt'  # TypeError

原因：Path 对象不支持 + 运算符

[OK] 正确做法：
  full_path = path / 'file.txt'

记忆技巧：Path 对象用 / 拼接，不用 +

────────────────────────────────────

[X] 错误2：mkdir() 创建多级目录失败

错误代码：
  Path('data/reports/2024').mkdir()  # FileNotFoundError

原因：父目录不存在

[OK] 正确做法：
  Path('data/reports/2024').mkdir(parents=True, exist_ok=True)

记忆技巧：parents=True 创建父目录，exist_ok=True 避免已存在错误

────────────────────────────────────

[X] 错误3：删除非空目录

错误代码：
  Path('data').rmdir()  # OSError: Directory not empty

原因：rmdir() 只能删除空目录

[OK] 正确做法：
  import shutil
  shutil.rmtree('data')  # 递归删除

记忆技巧：rmdir 删空目录，rmtree 删目录树

────────────────────────────────────

[X] 错误4：忘记检查路径是否存在

错误代码：
  path = Path('file.txt')
  content = path.read_text()  # FileNotFoundError

[OK] 正确做法：
  if path.exists():
      content = path.read_text()
  else:
      print("文件不存在")

记忆技巧：操作文件前先检查 exists()

────────────────────────────────────

[X] 错误5：iterdir() 和 glob() 混淆

错误代码：
  # 想递归查找所有 .txt 文件
  for file in Path('.').glob('*.txt'):  # 只查找当前目录
      print(file)

[OK] 正确做法：
  for file in Path('.').rglob('*.txt'):  # 递归查找
      print(file)

记忆技巧：
  glob  - 当前目录
  rglob - 递归（r = recursive）
""")

print()

# ============================================
# 第五部分：常用操作速查
# ============================================

print("=" * 60)
print("第五部分：常用操作速查")
print("=" * 60)

print("""
【创建 Path 对象】

Path('.')                   # 当前目录
Path('/usr/bin')            # 绝对路径
Path.cwd()                  # 当前工作目录
Path.home()                 # 用户主目录

【路径属性】

path.name                   # 文件名
path.stem                   # 文件名（无扩展名）
path.suffix                 # 扩展名
path.parent                 # 父目录
path.parts                  # 路径各部分

【路径操作】

path1 / path2               # 路径拼接
path.resolve()              # 转为绝对路径
path.relative_to(base)      # 相对路径

【判断】

path.exists()               # 是否存在
path.is_file()              # 是否是文件
path.is_dir()               # 是否是目录

【遍历】

path.iterdir()              # 遍历直接子项
path.glob('*.txt')          # 模式匹配
path.rglob('*.txt')         # 递归匹配

【文件操作】

path.read_text()            # 读取文本
path.write_text(text)       # 写入文本
path.open('r')              # 打开文件
path.unlink()               # 删除文件

【目录操作】

path.mkdir(parents=True, exist_ok=True)  # 创建目录
path.rmdir()                             # 删除空目录
path.rename(new_path)                    # 重命名
""")

print()

# ============================================
# 第六部分：练习题
# ============================================

print("=" * 60)
print("第六部分：练习题")
print("=" * 60)

print("""
1. 编写函数，统计指定目录下所有 .py 文件的总行数
   提示：使用 rglob('*.py') 和 read_text()

2. 编写函数，查找指定目录下最大的文件
   提示：使用 stat().st_size

3. 编写函数，将目录下所有 .txt 文件移动到 backup 子目录
   提示：使用 glob('*.txt') 和 rename()

4. 编写函数，递归列出目录树结构（类似 tree 命令）
   提示：使用 iterdir() 和递归

5. 思考题：
   - pathlib 和 os.path 有什么区别？
   - 什么时候用 glob，什么时候用 rglob？
   - 如何安全地删除非空目录？
""")

print("=" * 60)
print("第22天 - pathlib 模块详解 学习完成！")
print("=" * 60)
