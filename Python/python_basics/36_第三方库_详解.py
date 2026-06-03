# ============================================
# 第三方库基础 - 超详细注释版
# 日期：2026-06-01（第15天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【核心概念】第三方库（Third-Party Library）

是什么？
- 第三方库是由 Python 社区开发的扩展包
- 不是 Python 自带的，需要额外安装
- 托管在 PyPI（Python Package Index）上

为什么需要？
- 标准库功能有限，无法满足所有需求
- 第三方库提供了丰富的功能扩展
- 避免重复造轮子，提高开发效率
- Python 生态的核心优势

核心特点？
- 需要安装：使用 pip 工具安装
- 功能丰富：几乎任何需求都有对应的库
- 社区驱动：由全球开发者贡献和维护
- 版本管理：可以指定版本，避免冲突
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 什么是第三方库？
# --------------------------------------------

print("\n1.1 什么是第三方库？")
print("-" * 60)

print("""
【第三方库的定义】

第三方库 = 社区开发的 Python 扩展包

比喻：
- 标准库 = 手机自带的应用（电话、短信、相机）
- 第三方库 = 应用商店下载的应用（微信、抖音、游戏）

Python 的三层结构：
1. Python 核心：基本语法、数据类型、控制流程
2. 标准库：Python 自带的模块（os、datetime、json）
3. 第三方库：社区开发的扩展包（requests、pandas、numpy）

为什么需要第三方库？
- 标准库功能有限，无法满足所有需求
- 第三方库提供了专业的解决方案
- 避免重复造轮子，提高开发效率
- Python 生态的核心优势
""")

# --------------------------------------------
# 1.2 PyPI - Python 包索引
# --------------------------------------------

print("\n1.2 PyPI - Python 包索引")
print("-" * 60)

print("""
【PyPI 的定义】

PyPI = Python Package Index（Python 包索引）
官网：https://pypi.org/

是什么？
- Python 第三方库的官方仓库
- 类似于应用商店
- 托管了超过 40 万个 Python 包

作用：
- 开发者上传自己开发的包
- 用户搜索和下载需要的包
- pip 工具从 PyPI 下载包

常用的第三方库：
- requests：HTTP 请求库（每月下载量 1 亿+）
- pandas：数据分析库
- numpy：科学计算库
- flask：Web 框架
- django：Web 框架
- pytest：测试框架
""")

# --------------------------------------------
# 1.3 第三方库 vs 标准库
# --------------------------------------------

print("\n1.3 第三方库 vs 标准库")
print("-" * 60)

print("""
【对比】

标准库：
- Python 自带，无需安装
- 功能基础，覆盖常见需求
- 官方维护，稳定可靠
- 示例：os、datetime、json、math

第三方库：
- 需要安装（pip install）
- 功能专业，针对特定领域
- 社区维护，更新频繁
- 示例：requests、pandas、numpy、flask

如何选择？
- 优先使用标准库（无需安装，稳定）
- 标准库不够用时，使用第三方库
- 选择流行的、维护活跃的第三方库
""")

# ============================================
# 第二部分：pip 包管理工具 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第二部分：pip 包管理工具")
print("=" * 60)

# --------------------------------------------
# 2.1 什么是 pip？
# --------------------------------------------

print("\n2.1 什么是 pip？")
print("-" * 60)

print("""
【pip 的定义】

pip = Package Installer for Python（Python 包安装器）

是什么？
- Python 的包管理工具
- 用于安装、卸载、管理第三方库
- Python 3.4+ 自带 pip

作用：
- 安装第三方库
- 卸载第三方库
- 查看已安装的库
- 更新第三方库
- 导出依赖列表

比喻：
- pip = 应用商店的下载管理器
- 第三方库 = 应用
""")

# --------------------------------------------
# 2.2 pip 常用命令
# --------------------------------------------

print("\n2.2 pip 常用命令")
print("-" * 60)

print("""
【pip 命令速查】

1. 安装包
   pip install 包名
   pip install requests
   pip install pandas==1.5.0  # 安装指定版本

2. 卸载包
   pip uninstall 包名
   pip uninstall requests

3. 查看已安装的包
   pip list  # 列出所有包
   pip show 包名  # 查看包详情

4. 更新包
   pip install --upgrade 包名
   pip install --upgrade requests

5. 导出依赖列表
   pip freeze > requirements.txt

6. 安装依赖列表
   pip install -r requirements.txt

注意：
- 在命令行（终端）中执行，不是在 Python 中
- Windows: 打开 cmd 或 PowerShell
- Mac/Linux: 打开 Terminal
""")

print("\n示例：pip 命令演示（仅展示，不实际执行）")
print("-" * 60)

print("""
# 安装 requests 库
$ pip install requests

# 查看 requests 详情
$ pip show requests

# 列出所有已安装的包
$ pip list

# 卸载 requests
$ pip uninstall requests

# 导出依赖列表
$ pip freeze > requirements.txt
""")

# ============================================
# 第三部分：虚拟环境（venv）⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第三部分：虚拟环境（venv）")
print("=" * 60)

# --------------------------------------------
# 3.1 为什么需要虚拟环境？
# --------------------------------------------

print("\n3.1 为什么需要虚拟环境？")
print("-" * 60)

print("""
【虚拟环境的概念】

虚拟环境 = 独立的 Python 运行环境

问题场景：
- 项目A需要 requests 2.28.0
- 项目B需要 requests 2.31.0
- 全局只能安装一个版本，怎么办？

解决方案：
- 为每个项目创建独立的虚拟环境
- 每个虚拟环境有自己的包
- 互不干扰，版本隔离

比喻：
- 全局环境 = 公共图书馆（所有人共享）
- 虚拟环境 = 私人书房（每个项目独立）

优势：
- 版本隔离：不同项目使用不同版本的库
- 依赖管理：清晰记录项目依赖
- 环境干净：不污染全局环境
- 易于部署：导出依赖列表，快速重建环境
""")

# --------------------------------------------
# 3.2 虚拟环境的使用
# --------------------------------------------

print("\n3.2 虚拟环境的使用")
print("-" * 60)

print("""
【虚拟环境命令】

1. 创建虚拟环境
   python -m venv 环境名
   python -m venv venv  # 通常命名为 venv

2. 激活虚拟环境
   Windows: venv\\Scripts\\activate
   Mac/Linux: source venv/bin/activate

3. 退出虚拟环境
   deactivate

4. 删除虚拟环境
   直接删除 venv 文件夹

使用流程：
1. 创建项目目录
2. 创建虚拟环境
3. 激活虚拟环境
4. 安装需要的包
5. 开发项目
6. 导出依赖列表（pip freeze > requirements.txt）
7. 退出虚拟环境
""")

# ============================================
# 第四部分：requests 库基础 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：requests 库基础")
print("=" * 60)

# --------------------------------------------
# 4.1 什么是 requests？
# --------------------------------------------

print("\n4.1 什么是 requests？")
print("-" * 60)

print("""
【requests 库的定义】

requests = Python 的 HTTP 请求库

是什么？
- 用于发送 HTTP 请求的第三方库
- 比标准库 urllib 更简单易用
- Python 中最受欢迎的 HTTP 库

作用：
- 获取网页内容
- 调用 API 接口
- 下载文件
- 提交表单数据

安装：
pip install requests

官网：https://requests.readthedocs.io/
""")

# --------------------------------------------
# 4.2 requests 基本使用（示例代码）
# --------------------------------------------

print("\n4.2 requests 基本使用（示例代码）")
print("-" * 60)

print("""
【GET 请求示例】

import requests

# 发送 GET 请求
response = requests.get('https://api.github.com')

# 查看响应状态码
print(response.status_code)  # 200 表示成功

# 查看响应内容（文本）
print(response.text)

# 查看响应内容（JSON）
data = response.json()
print(data)

【POST 请求示例】

import requests

# 发送 POST 请求
data = {'username': 'test', 'password': '123456'}
response = requests.post('https://example.com/login', data=data)

# 查看响应
print(response.status_code)
print(response.text)

【常用参数】

# 添加请求头
headers = {'User-Agent': 'My App'}
response = requests.get(url, headers=headers)

# 添加查询参数
params = {'page': 1, 'size': 10}
response = requests.get(url, params=params)

# 设置超时
response = requests.get(url, timeout=5)
""")

# ============================================
# 第五部分：pandas 库入门 ⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第五部分：pandas 库入门")
print("=" * 60)

# --------------------------------------------
# 5.1 什么是 pandas？
# --------------------------------------------

print("\n5.1 什么是 pandas？")
print("-" * 60)

print("""
【pandas 库的定义】

pandas = Python 的数据分析库

是什么？
- 用于数据处理和分析的第三方库
- 提供了类似 Excel 的表格操作
- 数据科学的必备工具

作用：
- 读取和写入数据（CSV、Excel、JSON）
- 数据清洗和转换
- 数据筛选和聚合
- 数据分析和统计

安装：
pip install pandas

官网：https://pandas.pydata.org/
""")

# --------------------------------------------
# 5.2 pandas 核心数据结构
# --------------------------------------------

print("\n5.2 pandas 核心数据结构")
print("-" * 60)

print("""
【Series - 一维数据】

Series = 带标签的一维数组

示例：
import pandas as pd

# 创建 Series
s = pd.Series([10, 20, 30, 40])
print(s)

# 输出：
# 0    10
# 1    20
# 2    30
# 3    40
# dtype: int64

# 自定义索引
s = pd.Series([10, 20, 30], index=['a', 'b', 'c'])
print(s['a'])  # 10

【DataFrame - 二维表格】

DataFrame = 带标签的二维表格

示例：
import pandas as pd

# 创建 DataFrame
data = {
    'name': ['张三', '李四', '王五'],
    'age': [25, 30, 28],
    'score': [85, 92, 78]
}
df = pd.DataFrame(data)
print(df)

# 输出：
#   name  age  score
# 0  张三   25     85
# 1  李四   30     92
# 2  王五   28     78

# 访问列
print(df['name'])

# 访问行
print(df.loc[0])

# 筛选数据
print(df[df['age'] > 25])
""")

# ============================================
# 第六部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第六部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
错误1：忘记安装第三方库就使用

❌ 错误代码：
import requests  # ModuleNotFoundError: No module named 'requests'

问题：
- 第三方库需要先安装才能使用
- 标准库无需安装，第三方库必须安装

✅ 正确做法：
# 先在命令行安装
$ pip install requests

# 然后在 Python 中导入
import requests

记忆技巧：
- 第一次使用某个库，先检查是否已安装
- 使用 pip list 查看已安装的库
- 遇到 ModuleNotFoundError，先用 pip install 安装
""")

print("""
错误2：在 Python 代码中执行 pip 命令

❌ 错误代码：
pip install requests  # SyntaxError

问题：
- pip 命令在命令行（终端）中执行
- 不是在 Python 代码中执行

✅ 正确做法：
# 在命令行（cmd、PowerShell、Terminal）中执行
$ pip install requests

# 不是在 Python 中执行
# 不是在 .py 文件中写 pip install

记忆技巧：
- pip 命令 = 命令行工具
- Python 代码 = .py 文件或交互式环境
- 两者不要混淆
""")

print("""
错误3：全局安装所有包，不使用虚拟环境

❌ 错误做法：
# 所有项目都在全局环境安装包
$ pip install requests
$ pip install pandas
$ pip install flask
# ... 安装了几十个包

问题：
- 不同项目可能需要不同版本的库
- 全局环境会越来越乱
- 版本冲突难以解决

✅ 正确做法：
# 为每个项目创建虚拟环境
$ python -m venv venv
$ venv\\Scripts\\activate  # Windows
$ pip install requests  # 只在当前项目安装

记忆技巧：
- 一个项目 = 一个虚拟环境
- 虚拟环境隔离依赖
- 保持全局环境干净
""")

print("""
错误4：requests 请求没有处理异常

❌ 错误代码：
import requests
response = requests.get('https://example.com')
data = response.json()  # 如果网络错误会崩溃

问题：
- 网络请求可能失败
- 服务器可能返回错误
- 没有异常处理会导致程序崩溃

✅ 正确代码：
import requests

try:
    response = requests.get('https://example.com', timeout=5)
    response.raise_for_status()  # 检查状态码
    data = response.json()
except requests.exceptions.RequestException as e:
    print(f"请求失败：{e}")

记忆技巧：
- 网络请求总是可能失败
- 使用 try-except 处理异常
- 设置 timeout 避免长时间等待
""")

print("""
错误5：pandas 读取数据时编码错误

❌ 错误代码：
import pandas as pd
df = pd.read_csv('data.csv')  # UnicodeDecodeError

问题：
- CSV 文件可能是 GBK 编码
- pandas 默认使用 UTF-8 编码
- 编码不匹配导致错误

✅ 正确代码：
import pandas as pd
df = pd.read_csv('data.csv', encoding='gbk')  # 指定编码

记忆技巧：
- 中文 CSV 文件常用 GBK 编码
- 遇到编码错误，尝试 encoding='gbk'
- 或者用 encoding='utf-8-sig'
""")

print("""
错误6：pip 安装速度慢

❌ 问题：
$ pip install pandas  # 下载很慢，甚至超时

原因：
- 默认从国外服务器下载
- 网络速度慢

✅ 解决方案：
# 使用国内镜像源
$ pip install pandas -i https://pypi.tuna.tsinghua.edu.cn/simple

# 或者永久配置镜像源
$ pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple

常用镜像源：
- 清华：https://pypi.tuna.tsinghua.edu.cn/simple
- 阿里云：https://mirrors.aliyun.com/pypi/simple/
- 豆瓣：https://pypi.douban.com/simple/

记忆技巧：
- 国内用户建议配置镜像源
- 下载速度快很多
""")

print("""
错误7：pandas DataFrame 修改数据时的警告

❌ 错误代码：
import pandas as pd
df = pd.DataFrame({'a': [1, 2, 3]})
df[df['a'] > 1]['a'] = 10  # SettingWithCopyWarning

问题：
- 链式索引可能不会真正修改数据
- pandas 会发出警告

✅ 正确代码：
import pandas as pd
df = pd.DataFrame({'a': [1, 2, 3]})
df.loc[df['a'] > 1, 'a'] = 10  # 使用 loc

记忆技巧：
- 修改 DataFrame 使用 loc 或 iloc
- 避免链式索引
""")

# ============================================
# 第七部分：工具速查
# ============================================

print("\n" + "=" * 60)
print("第七部分：工具速查")
print("=" * 60)

print("""
【pip 命令速查】

安装：pip install 包名
卸载：pip uninstall 包名
列表：pip list
详情：pip show 包名
更新：pip install --upgrade 包名
导出：pip freeze > requirements.txt
安装：pip install -r requirements.txt

────────────────────────────────────

【虚拟环境命令】

创建：python -m venv venv
激活：venv\\Scripts\\activate (Windows)
激活：source venv/bin/activate (Mac/Linux)
退出：deactivate

────────────────────────────────────

【requests 常用方法】

GET：requests.get(url)
POST：requests.post(url, data=data)
状态码：response.status_code
文本：response.text
JSON：response.json()
异常处理：try-except requests.exceptions.RequestException

────────────────────────────────────

【pandas 常用操作】

创建：pd.DataFrame(data)
读取：pd.read_csv('file.csv')
查看：df.head()
筛选：df[df['col'] > 10]
访问列：df['col']
访问行：df.loc[0]
统计：df.describe()
""")

# ============================================
# 第八部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第八部分：练习题（自己动手！）")
print("=" * 60)

print("""
练习1：安装和使用 requests
  要求：
  - 使用 pip 安装 requests 库
  - 编写代码访问 https://api.github.com
  - 打印响应状态码和内容
  - 处理可能的异常

  提示：
  - 先在命令行执行 pip install requests
  - 使用 try-except 处理异常
  - 设置 timeout 参数

练习2：创建虚拟环境
  要求：
  - 创建一个名为 myproject 的项目目录
  - 在项目目录中创建虚拟环境
  - 激活虚拟环境
  - 安装 requests 库
  - 导出依赖列表到 requirements.txt

  提示：
  - python -m venv venv
  - venv\\Scripts\\activate
  - pip freeze > requirements.txt

练习3：pandas 数据处理
  要求：
  - 创建一个包含学生信息的 DataFrame
  - 包含：姓名、年龄、成绩
  - 筛选出成绩大于 80 的学生
  - 计算平均成绩

  提示：
  - pd.DataFrame(data)
  - df[df['score'] > 80]
  - df['score'].mean()

练习4：requests + pandas 综合
  要求：
  - 使用 requests 获取 JSON 数据（模拟）
  - 将 JSON 数据转换为 pandas DataFrame
  - 进行数据分析

  提示：
  - response.json() 获取 JSON 数据
  - pd.DataFrame(json_data) 转换为 DataFrame

练习5：错误处理
  要求：
  - 编写一个函数 fetch_data(url)
  - 使用 requests 获取数据
  - 处理网络错误、超时、状态码错误
  - 返回数据或 None

  提示：
  - try-except 处理异常
  - response.raise_for_status() 检查状态码
  - timeout 参数设置超时
""")

print("\n" + "=" * 60)
print("学习完成！")
print("=" * 60)
print("""
恭喜你完成了第三方库基础的学习！

你已经掌握了：
✅ 第三方库的概念和 PyPI
✅ pip 包管理工具的使用
✅ 虚拟环境的创建和使用
✅ requests 库的基本使用
✅ pandas 库的基本概念

下一步：
- 完成练习题
- 运行 37_第三方库_练习.py 进行综合练习
- 在实际项目中应用第三方库

记住：
- 第三方库是 Python 生态的核心优势
- pip 是必须掌握的工具
- 虚拟环境隔离项目依赖
- requests 和 pandas 是最常用的库
""")

