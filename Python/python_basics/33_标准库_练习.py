# ============================================
# 标准库综合练习 - 实战项目
# 日期：2026-05-30（第13天）
# 难度：⭐⭐⭐ 中级
# ============================================

"""
【综合练习目标】

通过实际项目综合练习标准库：
1. os 模块（文件和目录操作）
2. datetime 模块（日期和时间处理）
3. json 模块（JSON 数据处理）

这个练习重点是体验：
- 标准库如何提高开发效率
- 如何在实际项目中应用标准库
- 如何设计实用的工具函数
"""

import os
import json
from datetime import datetime, timedelta

# ============================================
# 项目1：文件管理工具
# ============================================

print("=" * 60)
print("项目1：文件管理工具")
print("=" * 60)

class FileManager:
    """文件管理工具：使用 os 模块管理文件和目录"""

    @staticmethod
    def list_files(directory=".", extension=None):
        """列出目录中的文件"""
        print(f"\n列出目录：{directory}")

        try:
            # 检查目录是否存在
            if not os.path.exists(directory):
                print(f"错误：目录 '{directory}' 不存在")
                return []

            # 列出所有文件
            items = os.listdir(directory)

            # 过滤文件（排除目录）
            files = [item for item in items if os.path.isfile(os.path.join(directory, item))]

            # 如果指定了扩展名，进行过滤
            if extension:
                files = [f for f in files if f.endswith(extension)]

            print(f"找到 {len(files)} 个文件：")
            for i, file in enumerate(files[:10], 1):  # 只显示前10个
                print(f"  {i}. {file}")

            if len(files) > 10:
                print(f"  ... 还有 {len(files) - 10} 个文件")

            return files

        except Exception as e:
            print(f"错误：{e}")
            return []

    @staticmethod
    def get_file_info(filepath):
        """获取文件信息"""
        print(f"\n获取文件信息：{filepath}")

        if not os.path.exists(filepath):
            print("错误：文件不存在")
            return None

        info = {
            "文件名": os.path.basename(filepath),
            "目录": os.path.dirname(filepath) or "当前目录",
            "绝对路径": os.path.abspath(filepath),
            "是否是文件": os.path.isfile(filepath),
            "是否是目录": os.path.isdir(filepath),
        }

        for key, value in info.items():
            print(f"  {key}: {value}")

        return info

# 测试文件管理工具
print("\n测试文件管理工具：")
manager = FileManager()

# 列出当前目录的 Python 文件
manager.list_files(".", ".py")

# 获取当前文件的信息
manager.get_file_info(__file__)

# ============================================
# 项目2：日志记录器
# ============================================

print("\n" + "=" * 60)
print("项目2：日志记录器")
print("=" * 60)

class Logger:
    """日志记录器：使用 datetime 模块记录日志"""

    def __init__(self, log_file="app.log"):
        self.log_file = log_file

    def log(self, message, level="INFO"):
        """记录日志"""
        # 获取当前时间
        now = datetime.now()
        timestamp = now.strftime("%Y-%m-%d %H:%M:%S")

        # 格式化日志信息
        log_entry = f"[{timestamp}] [{level}] {message}"

        # 打印到控制台
        print(log_entry)

        # 写入文件（实际项目中会真正写入）
        # with open(self.log_file, "a", encoding="utf-8") as f:
        #     f.write(log_entry + "\n")

    def info(self, message):
        """记录信息日志"""
        self.log(message, "INFO")

    def warning(self, message):
        """记录警告日志"""
        self.log(message, "WARNING")

    def error(self, message):
        """记录错误日志"""
        self.log(message, "ERROR")

# 测试日志记录器
print("\n测试日志记录器：")
logger = Logger()

logger.info("程序启动")
logger.info("正在加载配置文件")
logger.warning("配置文件不存在，使用默认配置")
logger.error("数据库连接失败")
logger.info("程序退出")

# ============================================
# 项目3：配置文件管理器
# ============================================

print("\n" + "=" * 60)
print("项目3：配置文件管理器")
print("=" * 60)

class ConfigManager:
    """配置文件管理器：使用 json 模块管理配置"""

    def __init__(self):
        self.config = {}

    def load_from_dict(self, config_dict):
        """从字典加载配置（模拟从文件加载）"""
        self.config = config_dict
        print("配置加载成功")

    def load(self, filename):
        """从 JSON 文件加载配置"""
        try:
            with open(filename, "r", encoding="utf-8") as f:
                self.config = json.load(f)
            print(f"从 {filename} 加载配置成功")
        except FileNotFoundError:
            print(f"配置文件 {filename} 不存在，使用空配置")
            self.config = {}
        except json.JSONDecodeError as e:
            print(f"配置文件格式错误：{e}")
            self.config = {}

    def save(self, filename):
        """保存配置到 JSON 文件"""
        try:
            with open(filename, "w", encoding="utf-8") as f:
                json.dump(self.config, f, ensure_ascii=False, indent=2)
            print(f"配置已保存到 {filename}")
        except Exception as e:
            print(f"保存配置失败：{e}")

    def get(self, key, default=None):
        """获取配置项"""
        return self.config.get(key, default)

    def set(self, key, value):
        """设置配置项"""
        self.config[key] = value
        print(f"配置项 '{key}' 已设置为 '{value}'")

    def show(self):
        """显示所有配置"""
        print("\n当前配置：")
        print(json.dumps(self.config, ensure_ascii=False, indent=2))

# 测试配置文件管理器
print("\n测试配置文件管理器：")
config = ConfigManager()

# 模拟加载配置
config.load_from_dict({
    "app_name": "我的应用",
    "version": "1.0.0",
    "debug": True,
    "database": {
        "host": "localhost",
        "port": 3306
    }
})

# 显示配置
config.show()

# 获取配置项
print(f"\n应用名称：{config.get('app_name')}")
print(f"版本：{config.get('version')}")
print(f"不存在的配置：{config.get('not_exist', '默认值')}")

# 修改配置
config.set("debug", False)
config.set("max_connections", 100)

# 显示修改后的配置
config.show()

# ============================================
# 项目4：数据导入导出工具
# ============================================

print("\n" + "=" * 60)
print("项目4：数据导入导出工具")
print("=" * 60)

class DataExporter:
    """数据导入导出工具：综合运用 os、datetime、json"""

    @staticmethod
    def export_to_json(data, filename):
        """导出数据到 JSON 文件"""
        try:
            # 确保目录存在
            directory = os.path.dirname(filename)
            if directory and not os.path.exists(directory):
                os.makedirs(directory)
                print(f"创建目录：{directory}")

            # 添加导出时间戳
            export_data = {
                "export_time": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                "data": data
            }

            # 写入文件
            with open(filename, "w", encoding="utf-8") as f:
                json.dump(export_data, f, ensure_ascii=False, indent=2)

            print(f"数据已导出到：{filename}")
            return True

        except Exception as e:
            print(f"导出失败：{e}")
            return False

    @staticmethod
    def import_from_json(filename):
        """从 JSON 文件导入数据"""
        try:
            # 检查文件是否存在
            if not os.path.exists(filename):
                print(f"文件不存在：{filename}")
                return None

            # 读取文件
            with open(filename, "r", encoding="utf-8") as f:
                export_data = json.load(f)

            print(f"数据已导入：{filename}")
            print(f"导出时间：{export_data.get('export_time', '未知')}")

            return export_data.get("data")

        except Exception as e:
            print(f"导入失败：{e}")
            return None

# 测试数据导入导出工具
print("\n测试数据导入导出工具：")

# 准备测试数据
students = [
    {"name": "张三", "age": 20, "score": 85},
    {"name": "李四", "age": 21, "score": 92},
    {"name": "王五", "age": 19, "score": 78}
]

# 导出数据（模拟）
print("\n导出学生数据：")
print(json.dumps(students, ensure_ascii=False, indent=2))

# 模拟导入数据
print("\n模拟导入数据：")
print("数据导入成功")

# ============================================
# 项目5：日期计算工具
# ============================================

print("\n" + "=" * 60)
print("项目5：日期计算工具")
print("=" * 60)

class DateCalculator:
    """日期计算工具：使用 datetime 模块进行日期计算"""

    @staticmethod
    def days_until(target_date_str):
        """计算距离目标日期还有多少天"""
        try:
            # 解析目标日期
            target_date = datetime.strptime(target_date_str, "%Y-%m-%d")

            # 获取当前日期
            now = datetime.now()

            # 计算时间差
            diff = target_date - now

            # 返回天数
            days = diff.days

            if days > 0:
                print(f"距离 {target_date_str} 还有 {days} 天")
            elif days == 0:
                print(f"今天就是 {target_date_str}")
            else:
                print(f"{target_date_str} 已经过去 {-days} 天")

            return days

        except ValueError as e:
            print(f"日期格式错误：{e}")
            return None

    @staticmethod
    def add_days(date_str, days):
        """给日期加上指定天数"""
        try:
            # 解析日期
            date_obj = datetime.strptime(date_str, "%Y-%m-%d")

            # 加上天数
            new_date = date_obj + timedelta(days=days)

            # 格式化输出
            result = new_date.strftime("%Y-%m-%d")
            print(f"{date_str} + {days}天 = {result}")

            return result

        except ValueError as e:
            print(f"日期格式错误：{e}")
            return None

    @staticmethod
    def get_weekday(date_str):
        """获取日期是星期几"""
        try:
            # 解析日期
            date_obj = datetime.strptime(date_str, "%Y-%m-%d")

            # 获取星期几（0=周一，6=周日）
            weekday = date_obj.weekday()
            weekday_names = ["周一", "周二", "周三", "周四", "周五", "周六", "周日"]

            result = weekday_names[weekday]
            print(f"{date_str} 是 {result}")

            return result

        except ValueError as e:
            print(f"日期格式错误：{e}")
            return None

# 测试日期计算工具
print("\n测试日期计算工具：")
calculator = DateCalculator()

# 计算距离某个日期还有多少天
calculator.days_until("2026-12-31")
calculator.days_until("2026-06-01")

# 日期加减
calculator.add_days("2026-05-30", 7)
calculator.add_days("2026-05-30", -7)

# 获取星期几
calculator.get_weekday("2026-05-30")
calculator.get_weekday("2026-06-01")

# ============================================
# 总结
# ============================================

print("\n" + "=" * 60)
print("总结：标准库的价值")
print("=" * 60)

print("""
通过今天的综合练习，你已经实际用到了：

1. os 模块
   - 文件和目录操作
   - 路径处理（跨平台兼容）
   - 文件信息获取

2. datetime 模块
   - 获取当前时间
   - 日期格式化和解析
   - 时间差计算
   - 日期加减运算

3. json 模块
   - Python 对象 ↔ JSON 字符串
   - JSON 文件读写
   - 处理中文数据

4. 综合应用
   - 文件管理工具
   - 日志记录器
   - 配置文件管理器
   - 数据导入导出工具
   - 日期计算工具

现在你对标准库的理解已经从"知道有这些模块"进步到：
你开始理解"如何在实际项目中应用标准库"。
""")

# ============================================
# 附加练习题（不提供答案）
# ============================================

print("\n" + "=" * 60)
print("附加练习题（自己动手）")
print("=" * 60)

print("""
练习1：文件备份工具
  要求：
  - 创建一个 BackupManager 类
  - 提供 backup(source, destination) 方法
  - 备份文件时添加时间戳到文件名
  - 例如：file.txt → file_20260530_143025.txt
  - 使用 os 和 datetime 模块

练习2：任务提醒器
  要求：
  - 创建一个 TaskReminder 类
  - 提供 add_task(name, due_date) 方法添加任务
  - 提供 check_tasks() 方法检查即将到期的任务
  - 如果任务在3天内到期，显示提醒
  - 使用 datetime 模块

练习3：数据统计工具
  要求：
  - 读取 JSON 文件中的学生成绩数据
  - 计算平均分、最高分、最低分
  - 统计各分数段的人数
  - 将统计结果保存到新的 JSON 文件
  - 使用 json 模块

练习4：日志分析器
  要求：
  - 读取日志文件
  - 统计每个日期的日志数量
  - 统计每个级别（INFO/WARNING/ERROR）的数量
  - 找出最早和最晚的日志时间
  - 使用 datetime 和文件操作

练习5：项目初始化工具
  要求：
  - 创建一个 ProjectInitializer 类
  - 自动创建项目目录结构（src、tests、docs等）
  - 生成默认的配置文件（config.json）
  - 生成 README.md 文件（包含项目名称和创建时间）
  - 使用 os、datetime、json 模块
""")

print("\n" + "=" * 60)
print("学习完成！")
print("=" * 60)
print("""
恭喜你完成了标准库的学习！

你已经掌握了：
✅ 标准库的概念和优势
✅ os 模块（文件和目录操作）
✅ datetime 模块（日期和时间处理）
✅ json 模块（JSON 数据处理）
✅ 如何在实际项目中应用标准库

下一步：
- 完成附加练习题
- 在实际项目中应用标准库
- 探索更多标准库模块（sys、re、collections等）
""")





