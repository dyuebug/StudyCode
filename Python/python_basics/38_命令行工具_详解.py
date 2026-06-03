# ============================================
# 命令行工具开发 - 超详细注释版
# 日期：2026-06-02（第16天）
# 难度：⭐⭐⭐⭐ 中高级
# ============================================

"""
【核心概念】命令行工具（CLI - Command Line Interface）

是什么？
- 通过命令行（终端）与用户交互的程序
- 用户输入命令和参数，程序执行相应操作
- 不需要图形界面，轻量高效

为什么需要？
- 自动化任务：批量处理、定时任务
- 服务器环境：服务器通常没有图形界面
- 开发效率：命令行操作更快捷
- 脚本集成：易于集成到其他脚本中

常见示例？
- git：版本控制工具
- pip：Python 包管理工具
- npm：Node.js 包管理工具
- docker：容器管理工具
"""

# ============================================
# 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
# ============================================

print("=" * 60)
print("第一部分：核心概念详解")
print("=" * 60)

# --------------------------------------------
# 1.1 什么是命令行工具？
# --------------------------------------------

print("\n1.1 什么是命令行工具？")
print("-" * 60)

print("""
【命令行工具的定义】

命令行工具 = 通过命令行交互的程序

比喻：
- 图形界面 = 餐厅点餐（看菜单，点击选择）
- 命令行工具 = 电话点餐（说出菜名和数量）

命令行工具的特点：
1. 文本交互：通过文本命令与程序交互
2. 参数化：通过参数控制程序行为
3. 可脚本化：可以写入脚本自动执行
4. 轻量高效：不需要图形界面，资源占用少

常见的命令行工具：
- git add file.txt：添加文件到暂存区
- pip install requests：安装 Python 包
- python script.py：运行 Python 脚本
- ls -la：列出目录内容（Linux/Mac）
- dir：列出目录内容（Windows）
""")

# --------------------------------------------
# 1.2 命令行工具的应用场景
# --------------------------------------------

print("\n1.2 命令行工具的应用场景")
print("-" * 60)

print("""
【应用场景】

1. 自动化任务
   - 批量处理文件
   - 定时任务执行
   - 数据备份和同步

2. 开发工具
   - 代码生成器
   - 项目脚手架
   - 构建和部署工具

3. 系统管理
   - 服务器管理
   - 日志分析
   - 性能监控

4. 数据处理
   - 数据清洗
   - 数据转换
   - 数据分析

5. 个人工具
   - 待办事项管理
   - 笔记管理
   - 密码管理

示例：
- git：版本控制
- docker：容器管理
- kubectl：Kubernetes 管理
- aws-cli：AWS 云服务管理
""")

# ============================================
# 第二部分：argparse 模块基础 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第二部分：argparse 模块基础")
print("=" * 60)

# --------------------------------------------
# 2.1 什么是 argparse？
# --------------------------------------------

print("\n2.1 什么是 argparse？")
print("-" * 60)

print("""
【argparse 的定义】

argparse = Python 标准库中的命令行参数解析模块

是什么？
- 用于解析命令行参数的标准库
- 自动生成帮助信息
- 支持位置参数和可选参数
- 支持子命令

作用：
- 解析用户输入的命令行参数
- 验证参数类型和值
- 生成帮助文档
- 提供友好的错误提示

比喻：
- argparse = 餐厅服务员（理解你的点餐需求）
- 命令行参数 = 你的点餐内容
""")

# --------------------------------------------
# 2.2 argparse 基本使用
# --------------------------------------------

print("\n2.2 argparse 基本使用")
print("-" * 60)

print("""
【基本步骤】

1. 创建解析器
   parser = argparse.ArgumentParser(description='程序描述')

2. 添加参数
   parser.add_argument('name', help='参数说明')  # 位置参数
   parser.add_argument('--option', help='参数说明')  # 可选参数

3. 解析参数
   args = parser.parse_args()

4. 使用参数
   print(args.name)
   print(args.option)

示例：
import argparse

# 创建解析器
parser = argparse.ArgumentParser(description='简单的计算器')

# 添加参数
parser.add_argument('num1', type=int, help='第一个数字')
parser.add_argument('num2', type=int, help='第二个数字')
parser.add_argument('--operation', default='add', help='操作类型')

# 解析参数
args = parser.parse_args()

# 使用参数
if args.operation == 'add':
    result = args.num1 + args.num2
    print(f'{args.num1} + {args.num2} = {result}')
""")

# --------------------------------------------
# 2.3 位置参数 vs 可选参数
# --------------------------------------------

print("\n2.3 位置参数 vs 可选参数")
print("-" * 60)

print("""
【位置参数】

特点：
- 必须提供
- 按顺序匹配
- 不需要参数名

示例：
parser.add_argument('filename', help='文件名')
parser.add_argument('count', type=int, help='数量')

使用：
python script.py data.txt 10

【可选参数】

特点：
- 可以不提供（有默认值）
- 使用 -- 或 - 开头
- 需要参数名

示例：
parser.add_argument('--verbose', action='store_true', help='详细输出')
parser.add_argument('--output', default='output.txt', help='输出文件')

使用：
python script.py --verbose --output result.txt

【参数类型】

type 参数：
- type=int：整数
- type=float：浮点数
- type=str：字符串（默认）

default 参数：
- 默认值

action 参数：
- action='store_true'：布尔标志（存在为 True）
- action='store_false'：布尔标志（存在为 False）
""")

# ============================================
# 第三部分：项目结构设计 ⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第三部分：项目结构设计")
print("=" * 60)

# --------------------------------------------
# 3.1 为什么需要好的项目结构？
# --------------------------------------------

print("\n3.1 为什么需要好的项目结构？")
print("-" * 60)

print("""
【项目结构的重要性】

好的项目结构：
1. 易于理解：新人能快速上手
2. 易于维护：修改代码不会影响其他部分
3. 易于扩展：添加新功能很容易
4. 易于测试：可以单独测试每个模块

比喻：
- 好的项目结构 = 整洁的房间（东西分类摆放）
- 差的项目结构 = 杂乱的房间（东西随意堆放）

常见问题：
- 所有代码写在一个文件里
- 函数职责不清晰
- 代码重复
- 难以测试
""")

# --------------------------------------------
# 3.2 推荐的项目结构
# --------------------------------------------

print("\n3.2 推荐的项目结构")
print("-" * 60)

print("""
【简单项目结构】

my_cli_tool/
├── main.py          # 主程序入口
├── config.py        # 配置管理
├── models.py        # 数据模型（类定义）
├── utils.py         # 工具函数
├── data/            # 数据文件目录
│   └── tasks.json
└── logs/            # 日志文件目录
    └── app.log

【模块职责】

main.py：
- 命令行参数解析
- 调用其他模块
- 程序入口

models.py：
- 定义数据模型类
- 业务逻辑

utils.py：
- 通用工具函数
- 文件操作、日期处理等

config.py：
- 配置常量
- 配置文件读取

【设计原则】

1. 单一职责：每个模块只做一件事
2. 高内聚：相关功能放在一起
3. 低耦合：模块之间依赖少
4. 易于测试：可以单独测试每个模块
""")

# ============================================
# 第四部分：日志记录和错误处理 ⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第四部分：日志记录和错误处理")
print("=" * 60)

# --------------------------------------------
# 4.1 为什么需要日志记录？
# --------------------------------------------

print("\n4.1 为什么需要日志记录？")
print("-" * 60)

print("""
【日志记录的价值】

日志记录的作用：
1. 调试：记录程序运行状态，方便调试
2. 监控：监控程序运行情况
3. 审计：记录用户操作，追溯问题
4. 分析：分析程序性能和使用情况

比喻：
- 日志 = 飞机的黑匣子（记录飞行数据）
- 没有日志 = 盲飞（出问题不知道原因）

日志 vs print：
- print：临时调试，输出到控制台
- 日志：长期记录，可以输出到文件，有级别控制
""")

# --------------------------------------------
# 4.2 logging 模块使用
# --------------------------------------------

print("\n4.2 logging 模块使用")
print("-" * 60)

print("""
【logging 基本使用】

import logging

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    filename='app.log'
)

# 记录日志
logging.debug('调试信息')
logging.info('普通信息')
logging.warning('警告信息')
logging.error('错误信息')
logging.critical('严重错误')

【日志级别】

DEBUG：详细的调试信息
INFO：普通信息
WARNING：警告信息
ERROR：错误信息
CRITICAL：严重错误

【日志格式】

常用格式化字符串：
- %(asctime)s：时间
- %(levelname)s：日志级别
- %(message)s：日志消息
- %(filename)s：文件名
- %(lineno)d：行号
""")

# --------------------------------------------
# 4.3 错误处理最佳实践
# --------------------------------------------

print("\n4.3 错误处理最佳实践")
print("-" * 60)

print("""
【错误处理原则】

1. 预期错误要处理
   - 文件不存在
   - 网络连接失败
   - 用户输入错误

2. 提供友好的错误提示
   - 不要直接显示技术错误信息
   - 告诉用户发生了什么
   - 告诉用户如何解决

3. 记录错误日志
   - 记录错误详情
   - 方便后续排查

示例：
try:
    with open('data.json', 'r') as f:
        data = json.load(f)
except FileNotFoundError:
    logging.error('数据文件不存在')
    print('错误：数据文件不存在，请先初始化')
    sys.exit(1)
except json.JSONDecodeError:
    logging.error('数据文件格式错误')
    print('错误：数据文件格式错误，请检查文件内容')
    sys.exit(1)
""")

# ============================================
# 第五部分：代码组织最佳实践 ⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第五部分：代码组织最佳实践")
print("=" * 60)

print("""
【最佳实践总结】

1. 函数职责单一
   - 每个函数只做一件事
   - 函数名清晰表达功能
   - 函数长度不超过 50 行

2. 类的设计原则
   - 单一职责原则
   - 封装数据和行为
   - 提供清晰的接口

3. 代码复用
   - 提取公共代码为函数
   - 避免重复代码
   - 使用继承和组合

4. 命名规范
   - 变量名：小写字母，下划线分隔（user_name）
   - 函数名：小写字母，下划线分隔（get_user）
   - 类名：大驼峰命名（UserManager）
   - 常量：大写字母，下划线分隔（MAX_SIZE）

5. 注释和文档
   - 复杂逻辑要注释
   - 函数要有文档字符串
   - 注释要说明"为什么"，而不是"是什么"

6. 错误处理
   - 预期错误要处理
   - 提供友好的错误提示
   - 记录错误日志
""")

# ============================================
# 第六部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
# ============================================

print("\n" + "=" * 60)
print("第六部分：常见错误和陷阱 ⭐⭐⭐⭐⭐")
print("=" * 60)

print("""
错误1：忘记处理用户输入错误

❌ 错误代码：
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('age', type=int)
args = parser.parse_args()

print(f"年龄：{args.age}")

问题：
- 用户输入非数字时程序崩溃
- 没有友好的错误提示
- 用户体验差

✅ 正确代码：
import argparse
import sys

parser = argparse.ArgumentParser()
parser.add_argument('age', type=int, help='年龄（整数）')

try:
    args = parser.parse_args()
    if args.age < 0 or args.age > 150:
        print("错误：年龄必须在 0-150 之间")
        sys.exit(1)
    print(f"年龄：{args.age}")
except SystemExit:
    print("提示：请输入有效的年龄（整数）")
    sys.exit(1)

记忆技巧：
- 用户输入总是不可信的
- 使用 type 参数进行类型验证
- 添加范围检查
- 提供友好的错误提示
""")

print("""
错误2：数据文件路径硬编码

❌ 错误代码：
import json

# 硬编码路径
with open('C:\\\\Users\\\\user\\\\data.json', 'r') as f:
    data = json.load(f)

问题：
- 路径在其他电脑上不存在
- 不同操作系统路径格式不同
- 难以维护和修改

✅ 正确代码：
import json
import os

# 使用相对路径或配置文件
DATA_DIR = os.path.join(os.path.dirname(__file__), 'data')
DATA_FILE = os.path.join(DATA_DIR, 'tasks.json')

# 确保目录存在
os.makedirs(DATA_DIR, exist_ok=True)

# 使用配置的路径
try:
    with open(DATA_FILE, 'r', encoding='utf-8') as f:
        data = json.load(f)
except FileNotFoundError:
    data = []

记忆技巧：
- 使用相对路径，不用绝对路径
- 使用 os.path.join() 拼接路径
- 使用 os.makedirs() 确保目录存在
- 将路径定义为常量，方便修改
""")

# __CONTINUE_HERE__

print("""
错误3：所有代码写在一个文件里

❌ 错误代码：
# main.py（1000+ 行代码）
import argparse
import json

# 数据模型类
class Task:
    pass

# 业务逻辑类
class TodoManager:
    pass

# 工具函数
def format_date():
    pass

# 命令行参数解析
parser = argparse.ArgumentParser()
# ... 100 行参数定义

# 主程序逻辑
if __name__ == '__main__':
    # ... 500 行代码

问题：
- 代码难以阅读和维护
- 难以测试单个功能
- 难以复用代码
- 团队协作困难

✅ 正确代码：
# 项目结构
my_todo/
├── main.py          # 主程序入口（50 行）
├── models.py        # 数据模型（100 行）
├── manager.py       # 业务逻辑（200 行）
├── utils.py         # 工具函数（50 行）
└── config.py        # 配置常量（20 行）

# main.py
from models import Task
from manager import TodoManager
import argparse

def main():
    parser = argparse.ArgumentParser()
    # ... 参数定义
    args = parser.parse_args()

    manager = TodoManager()
    # ... 调用业务逻辑

if __name__ == '__main__':
    main()

记忆技巧：
- 单个文件不超过 300 行
- 按功能模块拆分文件
- 主程序只负责调用，不写业务逻辑
- 遵循单一职责原则
""")

print("""
错误4：没有使用日志，只用 print

❌ 错误代码：
def delete_task(task_id):
    print(f"删除任务：{task_id}")
    # ... 删除逻辑
    print("删除成功")

问题：
- print 输出无法控制级别
- 无法保存到文件
- 生产环境难以调试
- 无法区分普通输出和调试信息

✅ 正确代码：
import logging

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('app.log'),
        logging.StreamHandler()
    ]
)

def delete_task(task_id):
    logging.info(f"删除任务：{task_id}")
    try:
        # ... 删除逻辑
        logging.info("删除成功")
    except Exception as e:
        logging.error(f"删除失败：{e}")

记忆技巧：
- 使用 logging 模块，不用 print
- 配置日志级别和格式
- 同时输出到文件和控制台
- 区分 info、warning、error 级别
""")

print("""
错误5：没有提供帮助信息

❌ 错误代码：
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('command')
parser.add_argument('value')
args = parser.parse_args()

问题：
- 用户不知道如何使用
- 没有参数说明
- 没有示例

✅ 正确代码：
import argparse

parser = argparse.ArgumentParser(
    description='待办事项管理工具',
    epilog='示例：python todo.py add "买菜"'
)

parser.add_argument(
    'command',
    choices=['add', 'list', 'done', 'delete'],
    help='命令：add（添加）、list（列表）、done（完成）、delete（删除）'
)

parser.add_argument(
    'value',
    nargs='?',
    help='任务内容或任务ID'
)

args = parser.parse_args()

记忆技巧：
- 使用 description 参数说明程序功能
- 使用 help 参数说明每个参数
- 使用 epilog 参数提供使用示例
- 使用 choices 参数限制可选值
""")

print("""
错误6：JSON 文件编码问题

❌ 错误代码：
import json

# 保存中文数据
data = {'name': '张三', 'task': '学习Python'}
with open('data.json', 'w') as f:
    json.dump(data, f)

# 结果：{"name": "\\u5f20\\u4e09", "task": "\\u5b66\\u4e60Python"}

问题：
- 中文被转义为 Unicode
- 文件难以阅读
- 不符合预期

✅ 正确代码：
import json

# 保存中文数据
data = {'name': '张三', 'task': '学习Python'}
with open('data.json', 'w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False, indent=2)

# 结果：
# {
#   "name": "张三",
#   "task": "学习Python"
# }

记忆技巧：
- 使用 encoding='utf-8' 指定编码
- 使用 ensure_ascii=False 保留中文
- 使用 indent=2 格式化输出
- 读取时也要指定 encoding='utf-8'
""")

print("""
错误7：没有验证数据完整性

❌ 错误代码：
import json

with open('tasks.json', 'r') as f:
    tasks = json.load(f)

# 直接使用，没有验证
for task in tasks:
    print(task['id'], task['title'])  # 如果缺少字段会报错

问题：
- 数据文件可能被手动修改
- 缺少必需字段会导致程序崩溃
- 没有数据验证

✅ 正确代码：
import json
import logging

def load_tasks():
    try:
        with open('tasks.json', 'r', encoding='utf-8') as f:
            tasks = json.load(f)

        # 验证数据结构
        if not isinstance(tasks, list):
            logging.error("数据格式错误：不是列表")
            return []

        # 验证每个任务
        valid_tasks = []
        for task in tasks:
            if isinstance(task, dict) and 'id' in task and 'title' in task:
                valid_tasks.append(task)
            else:
                logging.warning(f"跳过无效任务：{task}")

        return valid_tasks

    except FileNotFoundError:
        logging.info("数据文件不存在，创建新文件")
        return []
    except json.JSONDecodeError:
        logging.error("数据文件格式错误")
        return []

记忆技巧：
- 读取数据后要验证结构
- 检查必需字段是否存在
- 处理数据损坏的情况
- 记录错误日志
""")

# ============================================
# 第七部分：工具速查
# ============================================

print("\n" + "=" * 60)
print("第七部分：工具速查")
print("=" * 60)

print("""
【argparse 速查】

创建解析器：
parser = argparse.ArgumentParser(description='程序描述')

添加位置参数：
parser.add_argument('name', help='参数说明')

添加可选参数：
parser.add_argument('--option', default='默认值', help='参数说明')

参数类型：
parser.add_argument('num', type=int)

布尔标志：
parser.add_argument('--verbose', action='store_true')

限制选项：
parser.add_argument('cmd', choices=['add', 'list', 'delete'])

解析参数：
args = parser.parse_args()

使用参数：
print(args.name)
print(args.option)

────────────────────────────────────

【logging 速查】

基本配置：
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    filename='app.log'
)

记录日志：
logging.debug('调试信息')
logging.info('普通信息')
logging.warning('警告信息')
logging.error('错误信息')
logging.critical('严重错误')

────────────────────────────────────

【项目结构速查】

推荐结构：
my_cli_tool/
├── main.py          # 主程序入口
├── config.py        # 配置管理
├── models.py        # 数据模型
├── utils.py         # 工具函数
├── data/            # 数据文件
│   └── tasks.json
└── logs/            # 日志文件
    └── app.log

────────────────────────────────────

【文件操作速查】

相对路径：
DATA_DIR = os.path.join(os.path.dirname(__file__), 'data')

确保目录存在：
os.makedirs(DATA_DIR, exist_ok=True)

读取 JSON：
with open(file, 'r', encoding='utf-8') as f:
    data = json.load(f)

写入 JSON：
with open(file, 'w', encoding='utf-8') as f:
    json.dump(data, f, ensure_ascii=False, indent=2)
""")

# ============================================
# 第八部分：练习题
# ============================================

print("\n" + "=" * 60)
print("第八部分：练习题（自己动手！）")
print("=" * 60)

print("""
练习1：简单计算器命令行工具
  要求：
  - 使用 argparse 解析命令行参数
  - 支持加减乘除四种运算
  - 参数：两个数字 + 运算符
  - 示例：python calc.py 10 20 --op add

  提示：
  - 使用位置参数接收数字
  - 使用可选参数接收运算符
  - 使用 choices 限制运算符选项

练习2：文件批量重命名工具
  要求：
  - 批量重命名指定目录下的文件
  - 支持添加前缀或后缀
  - 支持替换文件名中的字符
  - 使用日志记录操作

  提示：
  - 使用 os.listdir() 列出文件
  - 使用 os.rename() 重命名文件
  - 使用 logging 记录每次重命名

练习3：学生成绩管理命令行工具
  要求：
  - 添加学生成绩
  - 查看所有成绩
  - 计算平均分
  - 查找最高分和最低分
  - 数据保存到 JSON 文件

  提示：
  - 使用子命令（subparsers）
  - 设计 Student 类
  - 使用 JSON 持久化数据

练习4：日志分析工具
  要求：
  - 读取日志文件
  - 统计不同级别的日志数量
  - 查找包含特定关键词的日志
  - 生成分析报告

  提示：
  - 使用正则表达式解析日志
  - 使用字典统计数量
  - 格式化输出报告

练习5：待办事项管理工具（综合练习）
  要求：
  - 添加任务（add）
  - 列出任务（list）
  - 完成任务（done）
  - 删除任务（delete）
  - 数据持久化
  - 完整的错误处理
  - 日志记录

  提示：
  - 设计 Task 类和 TodoManager 类
  - 使用 argparse 子命令
  - 使用 JSON 保存数据
  - 使用 logging 记录操作
  - 参考明天的综合项目实战
""")

print("\n" + "=" * 60)
print("学习完成！")
print("=" * 60)
print("""
恭喜你完成了命令行工具开发的学习！

你已经掌握了：
✅ 命令行工具的概念和应用场景
✅ argparse 模块的使用
✅ 项目结构设计原则
✅ 日志记录和错误处理
✅ 代码组织最佳实践

下一步：
- 完成练习题
- 运行 39_综合项目_实战.py 进行综合练习
- 开发自己的命令行工具

记住：
- 命令行工具是实用的项目类型
- argparse 是标准库，功能强大
- 项目结构设计影响代码可维护性
- 良好的错误处理提升用户体验
- 日志记录是调试和监控的关键
""")





