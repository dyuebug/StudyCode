# ============================================
# 日志分析工具 - 实战项目
# 日期：2026-05-31（第21天）
# 难度：⭐⭐⭐⭐ 综合应用
# ============================================

"""
【项目目标】

开发一个命令行日志分析工具，能够：
1. 读取日志文件
2. 解析日志格式（日期、时间、级别、消息）
3. 统计分析（错误数量、时间分布）
4. 生成分析报告

【用到的知识点】
- 正则表达式（日志解析）
- 文件操作（逐行读取）
- 迭代器（处理大文件）
- 字典统计（数据聚合）
- 命令行参数（argparse）
- 上下文管理器（文件管理）
"""

import re
import argparse
from collections import defaultdict, Counter
from datetime import datetime

# ============================================
# 第一部分：日志解析器
# ============================================

print("=" * 60)
print("日志分析工具 - 实战项目")
print("=" * 60)

class LogParser:
    """
    日志解析器

    支持的日志格式：
    YYYY-MM-DD HH:MM:SS [LEVEL] message
    """

    # 日志格式的正则表达式
    # 分组1：日期，分组2：时间，分组3：级别，分组4：消息
    LOG_PATTERN = r'(\d{4}-\d{2}-\d{2}) (\d{2}:\d{2}:\d{2}) \[(\w+)\] (.+)'

    def __init__(self):
        self.pattern = re.compile(self.LOG_PATTERN)

    def parse_line(self, line):
        """
        解析单行日志

        参数：
            line - 日志行字符串

        返回：
            字典 {'date': ..., 'time': ..., 'level': ..., 'message': ...}
            或 None（解析失败）
        """
        match = self.pattern.match(line.strip())
        if match:
            return {
                'date': match.group(1),
                'time': match.group(2),
                'level': match.group(3),
                'message': match.group(4)
            }
        return None

    def parse_file(self, filename):
        """
        解析整个日志文件（生成器）

        参数：
            filename - 日志文件路径

        返回：
            生成器，逐行返回解析后的日志字典
        """
        with open(filename, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                parsed = self.parse_line(line)
                if parsed:
                    parsed['line_num'] = line_num
                    yield parsed
                else:
                    # 解析失败，记录警告
                    print(f"警告：第{line_num}行格式不正确")

# ============================================
# 第二部分：日志分析器
# ============================================

class LogAnalyzer:
    """
    日志分析器

    统计分析日志数据
    """

    def __init__(self):
        self.total_count = 0
        self.level_count = Counter()  # 按级别统计
        self.date_count = Counter()   # 按日期统计
        self.hour_count = Counter()   # 按小时统计
        self.error_messages = []      # 错误消息列表

    def analyze(self, log_entries):
        """
        分析日志条目

        参数：
            log_entries - 日志条目的可迭代对象
        """
        for entry in log_entries:
            self.total_count += 1

            # 统计级别
            level = entry['level']
            self.level_count[level] += 1

            # 统计日期
            date = entry['date']
            self.date_count[date] += 1

            # 统计小时
            hour = entry['time'][:2]  # 提取小时部分
            self.hour_count[hour] += 1

            # 收集错误消息
            if level == 'ERROR':
                self.error_messages.append({
                    'line': entry['line_num'],
                    'time': f"{entry['date']} {entry['time']}",
                    'message': entry['message']
                })

    def generate_report(self):
        """
        生成分析报告

        返回：
            报告字符串
        """
        lines = []
        lines.append("=" * 60)
        lines.append("日志分析报告")
        lines.append("=" * 60)

        # 总体统计
        lines.append(f"\n【总体统计】")
        lines.append(f"总日志条数：{self.total_count}")

        # 按级别统计
        lines.append(f"\n【按级别统计】")
        for level, count in self.level_count.most_common():
            percentage = (count / self.total_count) * 100
            lines.append(f"  {level:8s}: {count:5d} ({percentage:5.1f}%)")

        # 按日期统计
        lines.append(f"\n【按日期统计】")
        for date, count in sorted(self.date_count.items()):
            lines.append(f"  {date}: {count:5d}")

        # 按小时统计
        lines.append(f"\n【按小时统计】")
        for hour in sorted(self.hour_count.keys()):
            count = self.hour_count[hour]
            bar = '█' * (count // 10)  # 简单的柱状图
            lines.append(f"  {hour}:00 - {count:5d} {bar}")

        # 错误详情
        if self.error_messages:
            lines.append(f"\n【错误详情】（前10条）")
            for i, error in enumerate(self.error_messages[:10], 1):
                lines.append(f"\n  {i}. 行号：{error['line']}")
                lines.append(f"     时间：{error['time']}")
                lines.append(f"     消息：{error['message']}")

        lines.append("\n" + "=" * 60)

        return '\n'.join(lines)

# ============================================
# 第三部分：命令行接口
# ============================================

def main():
    """主函数：命令行入口"""

    # 创建命令行参数解析器
    parser = argparse.ArgumentParser(
        description='日志分析工具 - 分析日志文件并生成报告'
    )

    # 添加参数
    parser.add_argument(
        'logfile',
        help='日志文件路径'
    )

    parser.add_argument(
        '-o', '--output',
        help='输出报告文件路径（可选，默认输出到屏幕）',
        default=None
    )

    parser.add_argument(
        '-l', '--level',
        help='只分析指定级别的日志（如 ERROR）',
        default=None
    )

    # 解析参数
    args = parser.parse_args()

    # 创建解析器和分析器
    parser_obj = LogParser()
    analyzer = LogAnalyzer()

    print(f"正在分析日志文件：{args.logfile}")
    print("-" * 60)

    try:
        # 解析日志文件
        log_entries = parser_obj.parse_file(args.logfile)

        # 如果指定了级别，过滤
        if args.level:
            log_entries = (
                entry for entry in log_entries
                if entry['level'] == args.level.upper()
            )

        # 分析日志
        analyzer.analyze(log_entries)

        # 生成报告
        report = analyzer.generate_report()

        # 输出报告
        if args.output:
            with open(args.output, 'w', encoding='utf-8') as f:
                f.write(report)
            print(f"\n报告已保存到：{args.output}")
        else:
            print(report)

    except FileNotFoundError:
        print(f"错误：文件不存在 - {args.logfile}")
    except Exception as e:
        print(f"错误：{e}")

# ============================================
# 第四部分：测试代码
# ============================================

if __name__ == '__main__':
    # 如果直接运行此文件，执行测试
    import sys

    # 检查是否提供了命令行参数
    if len(sys.argv) > 1:
        # 有参数，执行命令行模式
        main()
    else:
        # 无参数，执行测试模式
        print("测试模式：演示日志解析和分析")
        print("=" * 60)

        # 创建测试日志数据
        test_logs = [
            "2024-05-31 10:00:00 [INFO] Server started",
            "2024-05-31 10:05:23 [ERROR] Connection timeout",
            "2024-05-31 10:10:45 [ERROR] Database error",
            "2024-05-31 10:15:12 [WARNING] High memory usage",
            "2024-05-31 10:20:33 [INFO] Request processed",
            "2024-05-31 11:00:00 [INFO] Backup completed",
            "2024-05-31 11:30:22 [ERROR] File not found",
        ]

        # 测试解析器
        parser = LogParser()
        print("\n【测试日志解析】")
        parsed_logs = []
        for i, log in enumerate(test_logs, 1):
            parsed = parser.parse_line(log)
            if parsed:
                parsed['line_num'] = i
                parsed_logs.append(parsed)
                print(f"[OK] 解析成功：{parsed['level']} - {parsed['message'][:30]}")

        # 测试分析器
        print("\n【测试日志分析】")
        analyzer = LogAnalyzer()
        analyzer.analyze(parsed_logs)

        # 生成报告
        report = analyzer.generate_report()
        print(report)

        print("\n" + "=" * 60)
        print("测试完成！")
        print("\n使用方法：")
        print("  python 55_日志分析工具.py <日志文件路径>")
        print("  python 55_日志分析工具.py <日志文件路径> -o report.txt")
        print("  python 55_日志分析工具.py <日志文件路径> -l ERROR")
        print("=" * 60)
