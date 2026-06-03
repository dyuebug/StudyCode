# ============================================
# 综合项目实战：待办事项管理工具（TODO CLI）
# 日期：2026-06-02（第16天）
# 难度：⭐⭐⭐⭐ 中高级
# ============================================

"""
【项目简介】

项目名称：待办事项管理工具（TODO CLI）

功能：
- 添加任务（add）
- 查看任务（list）
- 完成任务（done）
- 删除任务（delete）
- 数据持久化（JSON）
- 命令行参数解析（argparse）
- 日志记录（logging）
- 完整的错误处理

技术栈：
- 面向对象设计（Task 类、TodoManager 类）
- 文件操作（JSON 读写）
- 异常处理（try-except）
- 日志记录（logging 模块）
- 命令行参数解析（argparse 模块）

学习目标：
- 综合运用前15天所学知识
- 掌握项目结构设计
- 实践模块化开发
- 写出高质量的代码
"""

# ============================================
# 第一部分：导入模块和配置
# ============================================

import argparse  # 命令行参数解析
import json      # JSON 数据处理
import os        # 文件和目录操作
import logging   # 日志记录
from datetime import datetime  # 日期时间处理

# --------------------------------------------
# 配置常量
# --------------------------------------------

# 数据文件路径（使用相对路径）
# os.path.dirname(__file__) 获取当前文件所在目录
# os.path.join() 拼接路径，跨平台兼容
DATA_DIR = os.path.join(os.path.dirname(__file__), 'data')
DATA_FILE = os.path.join(DATA_DIR, 'tasks.json')

# 日志文件路径
LOG_DIR = os.path.join(os.path.dirname(__file__), 'logs')
LOG_FILE = os.path.join(LOG_DIR, 'todo.log')

# --------------------------------------------
# 配置日志
# --------------------------------------------

# 确保日志目录存在
# exist_ok=True 表示目录已存在时不报错
os.makedirs(LOG_DIR, exist_ok=True)

# 配置日志格式和输出
logging.basicConfig(
    level=logging.INFO,  # 日志级别：INFO 及以上
    format='%(asctime)s - %(levelname)s - %(message)s',  # 日志格式
    handlers=[
        logging.FileHandler(LOG_FILE, encoding='utf-8'),  # 输出到文件
        logging.StreamHandler()  # 同时输出到控制台
    ]
)

# ============================================
# 第二部分：数据模型类
# ============================================

class Task:
    """
    任务类：表示一个待办事项

    属性：
        id (int): 任务ID（唯一标识）
        title (str): 任务标题
        created_at (str): 创建时间
        completed (bool): 是否完成
        completed_at (str): 完成时间
    """

    def __init__(self, task_id, title, created_at=None, completed=False, completed_at=None):
        """
        初始化任务对象

        参数：
            task_id (int): 任务ID
            title (str): 任务标题
            created_at (str, optional): 创建时间，默认为当前时间
            completed (bool, optional): 是否完成，默认为 False
            completed_at (str, optional): 完成时间，默认为 None
        """
        self.id = task_id
        self.title = title
        # 如果没有提供创建时间，使用当前时间
        self.created_at = created_at or datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        self.completed = completed
        self.completed_at = completed_at

    def mark_completed(self):
        """
        标记任务为已完成

        设置 completed 为 True，并记录完成时间
        """
        self.completed = True
        self.completed_at = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    def to_dict(self):
        """
        将任务对象转换为字典（用于 JSON 序列化）

        返回：
            dict: 包含任务所有属性的字典
        """
        return {
            'id': self.id,
            'title': self.title,
            'created_at': self.created_at,
            'completed': self.completed,
            'completed_at': self.completed_at
        }

    @staticmethod
    def from_dict(data):
        """
        从字典创建任务对象（用于 JSON 反序列化）

        参数：
            data (dict): 包含任务数据的字典

        返回：
            Task: 任务对象
        """
        return Task(
            task_id=data['id'],
            title=data['title'],
            created_at=data.get('created_at'),
            completed=data.get('completed', False),
            completed_at=data.get('completed_at')
        )

    def __str__(self):
        """
        返回任务的字符串表示（用于打印）

        返回：
            str: 格式化的任务信息
        """
        status = '✓' if self.completed else '○'
        return f"[{status}] {self.id}. {self.title} (创建于: {self.created_at})"

# ============================================
# 第三部分：业务逻辑类
# ============================================

class TodoManager:
    """
    待办事项管理器：负责任务的增删改查和数据持久化

    属性：
        data_file (str): 数据文件路径
        tasks (list): 任务列表
    """

    def __init__(self, data_file):
        """
        初始化管理器

        参数：
            data_file (str): 数据文件路径
        """
        self.data_file = data_file
        self.tasks = []
        # 确保数据目录存在
        os.makedirs(os.path.dirname(data_file), exist_ok=True)
        # 加载数据
        self.load_tasks()

    def load_tasks(self):
        """
        从 JSON 文件加载任务数据

        处理：
        - 文件不存在：创建空列表
        - JSON 格式错误：记录错误，创建空列表
        - 数据验证：跳过无效任务
        """
        try:
            # 尝试打开并读取文件
            with open(self.data_file, 'r', encoding='utf-8') as f:
                data = json.load(f)

            # 验证数据格式
            if not isinstance(data, list):
                logging.error("数据格式错误：不是列表")
                self.tasks = []
                return

            # 将字典转换为 Task 对象
            self.tasks = []
            for item in data:
                try:
                    # 验证必需字段
                    if isinstance(item, dict) and 'id' in item and 'title' in item:
                        task = Task.from_dict(item)
                        self.tasks.append(task)
                    else:
                        logging.warning(f"跳过无效任务：{item}")
                except Exception as e:
                    logging.warning(f"解析任务失败：{e}")

            logging.info(f"成功加载 {len(self.tasks)} 个任务")

        except FileNotFoundError:
            # 文件不存在，创建空列表
            logging.info("数据文件不存在，创建新文件")
            self.tasks = []
            self.save_tasks()

        except json.JSONDecodeError as e:
            # JSON 格式错误
            logging.error(f"数据文件格式错误：{e}")
            self.tasks = []

        except Exception as e:
            # 其他错误
            logging.error(f"加载数据失败：{e}")
            self.tasks = []

    def save_tasks(self):
        """
        将任务数据保存到 JSON 文件

        处理：
        - 将 Task 对象转换为字典
        - 使用 ensure_ascii=False 保留中文
        - 使用 indent=2 格式化输出
        """
        try:
            # 将 Task 对象转换为字典列表
            data = [task.to_dict() for task in self.tasks]

            # 写入文件
            with open(self.data_file, 'w', encoding='utf-8') as f:
                json.dump(data, f, ensure_ascii=False, indent=2)

            logging.info(f"成功保存 {len(self.tasks)} 个任务")

        except Exception as e:
            logging.error(f"保存数据失败：{e}")
            print(f"错误：保存数据失败 - {e}")

    def get_next_id(self):
        """
        获取下一个可用的任务ID

        返回：
            int: 下一个任务ID
        """
        if not self.tasks:
            return 1
        # 找到最大的 ID，然后加 1
        return max(task.id for task in self.tasks) + 1

    def add_task(self, title):
        """
        添加新任务

        参数：
            title (str): 任务标题

        返回：
            Task: 新创建的任务对象
        """
        # 验证标题
        if not title or not title.strip():
            logging.warning("任务标题为空")
            print("错误：任务标题不能为空")
            return None

        # 创建新任务
        task_id = self.get_next_id()
        task = Task(task_id, title.strip())

        # 添加到列表
        self.tasks.append(task)

        # 保存到文件
        self.save_tasks()

        logging.info(f"添加任务：{task.title}")
        print(f"✓ 成功添加任务：{task.title}")

        return task

    def list_tasks(self, show_all=True):
        """
        列出任务

        参数：
            show_all (bool): True 显示所有任务，False 只显示未完成任务
        """
        if not self.tasks:
            print("暂无任务")
            return

        # 筛选任务
        if show_all:
            tasks_to_show = self.tasks
            print("\n所有任务：")
        else:
            tasks_to_show = [task for task in self.tasks if not task.completed]
            print("\n未完成任务：")

        if not tasks_to_show:
            print("暂无任务")
            return

        # 打印任务
        print("-" * 60)
        for task in tasks_to_show:
            print(task)
            if task.completed and task.completed_at:
                print(f"  完成于: {task.completed_at}")
        print("-" * 60)

        # 统计信息
        total = len(self.tasks)
        completed = len([t for t in self.tasks if t.completed])
        print(f"\n总计：{total} 个任务，已完成 {completed} 个，未完成 {total - completed} 个")

    def find_task(self, task_id):
        """
        根据ID查找任务

        参数：
            task_id (int): 任务ID

        返回：
            Task: 找到的任务对象，未找到返回 None
        """
        for task in self.tasks:
            if task.id == task_id:
                return task
        return None

    def complete_task(self, task_id):
        """
        完成任务

        参数：
            task_id (int): 任务ID
        """
        task = self.find_task(task_id)

        if not task:
            logging.warning(f"任务不存在：ID={task_id}")
            print(f"错误：任务 {task_id} 不存在")
            return

        if task.completed:
            print(f"任务 {task_id} 已经完成")
            return

        # 标记为完成
        task.mark_completed()

        # 保存到文件
        self.save_tasks()

        logging.info(f"完成任务：{task.title}")
        print(f"✓ 成功完成任务：{task.title}")

    def delete_task(self, task_id):
        """
        删除任务

        参数：
            task_id (int): 任务ID
        """
        task = self.find_task(task_id)

        if not task:
            logging.warning(f"任务不存在：ID={task_id}")
            print(f"错误：任务 {task_id} 不存在")
            return

        # 从列表中移除
        self.tasks.remove(task)

        # 保存到文件
        self.save_tasks()

        logging.info(f"删除任务：{task.title}")
        print(f"✓ 成功删除任务：{task.title}")

# ============================================
# 第四部分：命令行参数解析
# ============================================

def create_parser():
    """
    创建命令行参数解析器

    返回：
        ArgumentParser: 配置好的参数解析器
    """
    # 创建主解析器
    parser = argparse.ArgumentParser(
        description='待办事项管理工具（TODO CLI）',
        epilog='示例：python todo.py add "学习Python"'
    )

    # 创建子命令解析器
    subparsers = parser.add_subparsers(
        dest='command',
        help='可用命令'
    )

    # add 命令：添加任务
    parser_add = subparsers.add_parser('add', help='添加新任务')
    parser_add.add_argument('title', help='任务标题')

    # list 命令：列出任务
    parser_list = subparsers.add_parser('list', help='列出任务')
    parser_list.add_argument(
        '--all',
        action='store_true',
        help='显示所有任务（包括已完成）'
    )

    # done 命令：完成任务
    parser_done = subparsers.add_parser('done', help='完成任务')
    parser_done.add_argument('id', type=int, help='任务ID')

    # delete 命令：删除任务
    parser_delete = subparsers.add_parser('delete', help='删除任务')
    parser_delete.add_argument('id', type=int, help='任务ID')

    return parser

# ============================================
# 第五部分：主程序
# ============================================

def main():
    """
    主程序入口

    流程：
    1. 解析命令行参数
    2. 创建 TodoManager 实例
    3. 根据命令执行相应操作
    """
    # 创建参数解析器
    parser = create_parser()

    # 解析参数
    args = parser.parse_args()

    # 如果没有提供命令，显示帮助信息
    if not args.command:
        parser.print_help()
        return

    # 创建管理器
    manager = TodoManager(DATA_FILE)

    # 根据命令执行操作
    try:
        if args.command == 'add':
            # 添加任务
            manager.add_task(args.title)

        elif args.command == 'list':
            # 列出任务
            manager.list_tasks(show_all=args.all)

        elif args.command == 'done':
            # 完成任务
            manager.complete_task(args.id)

        elif args.command == 'delete':
            # 删除任务
            manager.delete_task(args.id)

        else:
            print(f"未知命令：{args.command}")
            parser.print_help()

    except Exception as e:
        logging.error(f"执行命令失败：{e}")
        print(f"错误：{e}")

# ============================================
# 程序入口
# ============================================

if __name__ == '__main__':
    main()

# ============================================
# 使用说明
# ============================================

"""
【使用方法】

1. 添加任务
   python 39_综合项目_实战.py add "学习Python"
   python 39_综合项目_实战.py add "写作业"

2. 列出未完成任务
   python 39_综合项目_实战.py list

3. 列出所有任务
   python 39_综合项目_实战.py list --all

4. 完成任务
   python 39_综合项目_实战.py done 1

5. 删除任务
   python 39_综合项目_实战.py delete 2

6. 查看帮助
   python 39_综合项目_实战.py --help
   python 39_综合项目_实战.py add --help

【文件说明】

运行后会自动创建以下文件和目录：

python_basics/
├── 39_综合项目_实战.py  # 主程序
├── data/                 # 数据目录
│   └── tasks.json        # 任务数据文件
└── logs/                 # 日志目录
    └── todo.log          # 日志文件

【项目特点】

1. 面向对象设计
   - Task 类：封装任务数据和行为
   - TodoManager 类：封装业务逻辑

2. 数据持久化
   - 使用 JSON 文件存储数据
   - 自动保存和加载

3. 完整的错误处理
   - 文件不存在
   - JSON 格式错误
   - 数据验证
   - 用户输入错误

4. 日志记录
   - 记录所有操作
   - 同时输出到文件和控制台
   - 方便调试和追踪

5. 命令行参数解析
   - 使用 argparse 模块
   - 支持子命令
   - 自动生成帮助信息

【扩展建议】

如果你想进一步改进这个项目，可以尝试：

1. 添加任务优先级（高、中、低）
2. 添加任务截止日期
3. 添加任务分类（工作、学习、生活）
4. 添加任务搜索功能
5. 添加任务编辑功能
6. 添加彩色输出（使用 colorama 库）
7. 添加任务统计功能
8. 添加数据导出功能（CSV、Excel）

【学习总结】

通过这个项目，你已经综合运用了：

✅ 变量和数据类型
✅ 字符串操作
✅ 列表和字典
✅ 条件判断和循环
✅ 函数定义和调用
✅ 类和对象
✅ 继承和封装
✅ 异常处理
✅ 文件操作
✅ JSON 数据处理
✅ 日志记录
✅ 命令行参数解析
✅ 项目结构设计
✅ 模块化开发

恭喜你完成了第16天的学习！
这是一个完整的、可以实际使用的命令行工具。
你已经具备了开发实用工具的能力！
"""
