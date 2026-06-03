# ============================================
# 系统监控工具 - 实战项目
# 日期：2026-05-31（第24天）
# 难度：⭐⭐⭐⭐ 综合应用
# ============================================

"""
【项目目标】

开发一个系统监控工具，能够：
1. 实时监控 CPU、内存、磁盘
2. 进程管理（列出、搜索、终止）
3. 告警功能（超过阈值时告警）
4. 历史记录（保存监控数据）

【用到的知识点】
- psutil 模块（系统监控）
- json 模块（保存数据）
- datetime 模块（时间戳）
- 命令行参数（argparse）
"""

import psutil
import json
import time
from datetime import datetime
from pathlib import Path

# ============================================
# 第一部分：系统监控核心类
# ============================================

print("=" * 60)
print("系统监控工具 - 实战项目")
print("=" * 60)

class SystemMonitor:
    """
    系统监控工具

    支持：
    1. 实时监控
    2. 告警功能
    3. 历史记录
    4. 进程管理
    """

    def __init__(self, log_dir="monitor_logs"):
        """
        初始化监控工具

        参数：
            log_dir - 日志目录
        """
        self.log_dir = Path(log_dir)
        self.log_dir.mkdir(exist_ok=True)

        # 告警阈值
        self.thresholds = {
            'cpu': 80.0,      # CPU 使用率 > 80% 告警
            'memory': 85.0,   # 内存使用率 > 85% 告警
            'disk': 90.0      # 磁盘使用率 > 90% 告警
        }

    def get_cpu_info(self):
        """获取 CPU 信息"""
        return {
            'percent': psutil.cpu_percent(interval=1),
            'count': psutil.cpu_count(),
            'freq': psutil.cpu_freq().current if psutil.cpu_freq() else 0
        }

    def get_memory_info(self):
        """获取内存信息"""
        mem = psutil.virtual_memory()
        return {
            'total': mem.total,
            'available': mem.available,
            'used': mem.used,
            'percent': mem.percent
        }

    def get_disk_info(self):
        """获取磁盘信息"""
        disks = []
        for partition in psutil.disk_partitions():
            try:
                usage = psutil.disk_usage(partition.mountpoint)
                disks.append({
                    'device': partition.device,
                    'mountpoint': partition.mountpoint,
                    'total': usage.total,
                    'used': usage.used,
                    'free': usage.free,
                    'percent': usage.percent
                })
            except PermissionError:
                pass
        return disks

    def get_network_info(self):
        """获取网络信息"""
        net = psutil.net_io_counters()
        return {
            'bytes_sent': net.bytes_sent,
            'bytes_recv': net.bytes_recv,
            'packets_sent': net.packets_sent,
            'packets_recv': net.packets_recv
        }

    def check_alerts(self, cpu_percent, mem_percent, disk_info):
        """
        检查告警

        参数：
            cpu_percent - CPU 使用率
            mem_percent - 内存使用率
            disk_info - 磁盘信息列表

        返回：
            告警列表
        """
        alerts = []

        # CPU 告警
        if cpu_percent > self.thresholds['cpu']:
            alerts.append(f"⚠️ CPU 使用率过高：{cpu_percent}%")

        # 内存告警
        if mem_percent > self.thresholds['memory']:
            alerts.append(f"⚠️ 内存使用率过高：{mem_percent}%")

        # 磁盘告警
        for disk in disk_info:
            if disk['percent'] > self.thresholds['disk']:
                alerts.append(f"⚠️ 磁盘 {disk['mountpoint']} 使用率过高：{disk['percent']}%")

        return alerts

    def monitor_once(self):
        """
        执行一次监控

        返回：
            监控数据字典
        """
        # 获取系统信息
        cpu_info = self.get_cpu_info()
        mem_info = self.get_memory_info()
        disk_info = self.get_disk_info()
        net_info = self.get_network_info()

        # 检查告警
        alerts = self.check_alerts(
            cpu_info['percent'],
            mem_info['percent'],
            disk_info
        )

        # 组装数据
        data = {
            'timestamp': datetime.now().isoformat(),
            'cpu': cpu_info,
            'memory': mem_info,
            'disk': disk_info,
            'network': net_info,
            'alerts': alerts
        }

        return data

    def monitor_realtime(self, interval=5, duration=60):
        """
        实时监控

        参数：
            interval - 监控间隔（秒）
            duration - 监控时长（秒）
        """
        print(f"\n开始实时监控...")
        print(f"监控间隔：{interval} 秒")
        print(f"监控时长：{duration} 秒")
        print("=" * 60)

        start_time = time.time()
        count = 0

        while time.time() - start_time < duration:
            count += 1
            data = self.monitor_once()

            # 显示监控信息
            print(f"\n[{count}] {data['timestamp']}")
            print(f"CPU: {data['cpu']['percent']}%")
            print(f"内存: {data['memory']['percent']}% "
                  f"({data['memory']['used'] / (1024**3):.2f} GB / "
                  f"{data['memory']['total'] / (1024**3):.2f} GB)")

            for disk in data['disk']:
                print(f"磁盘 {disk['mountpoint']}: {disk['percent']}% "
                      f"({disk['used'] / (1024**3):.2f} GB / "
                      f"{disk['total'] / (1024**3):.2f} GB)")

            # 显示告警
            if data['alerts']:
                print("\n告警：")
                for alert in data['alerts']:
                    print(f"  {alert}")

            # 保存到日志
            self.save_log(data)

            # 等待下一次监控
            time.sleep(interval)

        print("\n" + "=" * 60)
        print(f"监控完成！共监控 {count} 次")

    def save_log(self, data):
        """
        保存监控日志

        参数：
            data - 监控数据
        """
        # 按日期保存
        date = datetime.now().strftime("%Y-%m-%d")
        log_file = self.log_dir / f"monitor_{date}.json"

        # 读取已有日志
        if log_file.exists():
            with open(log_file, 'r', encoding='utf-8') as f:
                logs = json.load(f)
        else:
            logs = []

        # 添加新数据
        logs.append(data)

        # 保存
        with open(log_file, 'w', encoding='utf-8') as f:
            json.dump(logs, f, indent=2, ensure_ascii=False)

    def get_top_processes(self, n=10, sort_by='memory'):
        """
        获取资源占用最高的进程

        参数：
            n - 返回前 n 个进程
            sort_by - 排序依据（'memory' 或 'cpu'）

        返回：
            进程列表
        """
        processes = []

        for proc in psutil.process_iter(['pid', 'name', 'cpu_percent', 'memory_info']):
            try:
                info = proc.info
                processes.append({
                    'pid': info['pid'],
                    'name': info['name'],
                    'cpu': info['cpu_percent'],
                    'memory': info['memory_info'].rss if info['memory_info'] else 0
                })
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass

        # 排序
        if sort_by == 'memory':
            processes.sort(key=lambda x: x['memory'], reverse=True)
        else:
            processes.sort(key=lambda x: x['cpu'], reverse=True)

        return processes[:n]

    def search_process(self, keyword):
        """
        搜索进程

        参数：
            keyword - 关键词

        返回：
            匹配的进程列表
        """
        processes = []

        for proc in psutil.process_iter(['pid', 'name', 'cpu_percent', 'memory_info']):
            try:
                info = proc.info
                if keyword.lower() in info['name'].lower():
                    processes.append({
                        'pid': info['pid'],
                        'name': info['name'],
                        'cpu': info['cpu_percent'],
                        'memory': info['memory_info'].rss if info['memory_info'] else 0
                    })
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass

        return processes

    def kill_process(self, pid):
        """
        终止进程

        参数：
            pid - 进程 ID

        返回：
            是否成功
        """
        try:
            proc = psutil.Process(pid)
            proc.terminate()
            proc.wait(timeout=3)
            return True
        except psutil.NoSuchProcess:
            print(f"进程 {pid} 不存在")
            return False
        except psutil.AccessDenied:
            print(f"权限不足，无法终止进程 {pid}")
            return False
        except psutil.TimeoutExpired:
            # 强制终止
            proc.kill()
            return True

# ============================================
# 第二部分：命令行接口
# ============================================

def main():
    """主函数：命令行入口"""
    import argparse

    parser = argparse.ArgumentParser(
        description='系统监控工具 - 实时监控、进程管理、告警'
    )

    subparsers = parser.add_subparsers(dest='command', help='子命令')

    # 实时监控命令
    monitor_parser = subparsers.add_parser('monitor', help='实时监控')
    monitor_parser.add_argument('--interval', type=int, default=5, help='监控间隔（秒）')
    monitor_parser.add_argument('--duration', type=int, default=60, help='监控时长（秒）')

    # 快照命令
    snapshot_parser = subparsers.add_parser('snapshot', help='系统快照')

    # 进程列表命令
    top_parser = subparsers.add_parser('top', help='资源占用最高的进程')
    top_parser.add_argument('-n', type=int, default=10, help='显示前 n 个进程')
    top_parser.add_argument('--sort', choices=['memory', 'cpu'], default='memory', help='排序依据')

    # 搜索进程命令
    search_parser = subparsers.add_parser('search', help='搜索进程')
    search_parser.add_argument('keyword', help='关键词')

    # 终止进程命令
    kill_parser = subparsers.add_parser('kill', help='终止进程')
    kill_parser.add_argument('pid', type=int, help='进程 ID')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return

    monitor = SystemMonitor()

    # 执行对应命令
    if args.command == 'monitor':
        monitor.monitor_realtime(args.interval, args.duration)

    elif args.command == 'snapshot':
        data = monitor.monitor_once()
        print(f"\n系统快照 - {data['timestamp']}")
        print("=" * 60)
        print(f"CPU: {data['cpu']['percent']}%")
        print(f"内存: {data['memory']['percent']}%")
        for disk in data['disk']:
            print(f"磁盘 {disk['mountpoint']}: {disk['percent']}%")
        if data['alerts']:
            print("\n告警：")
            for alert in data['alerts']:
                print(f"  {alert}")

    elif args.command == 'top':
        processes = monitor.get_top_processes(args.n, args.sort)
        print(f"\n资源占用最高的 {args.n} 个进程（按 {args.sort} 排序）")
        print("=" * 60)
        print(f"{'PID':<10} {'名称':<30} {'CPU%':<10} {'内存(MB)':<10}")
        print("-" * 60)
        for proc in processes:
            print(f"{proc['pid']:<10} {proc['name']:<30} "
                  f"{proc['cpu']:<10.1f} {proc['memory']/(1024**2):<10.2f}")

    elif args.command == 'search':
        processes = monitor.search_process(args.keyword)
        print(f"\n搜索结果：'{args.keyword}'")
        print("=" * 60)
        if processes:
            print(f"{'PID':<10} {'名称':<30} {'CPU%':<10} {'内存(MB)':<10}")
            print("-" * 60)
            for proc in processes:
                print(f"{proc['pid']:<10} {proc['name']:<30} "
                      f"{proc['cpu']:<10.1f} {proc['memory']/(1024**2):<10.2f}")
        else:
            print("未找到匹配的进程")

    elif args.command == 'kill':
        if monitor.kill_process(args.pid):
            print(f"成功终止进程 {args.pid}")
        else:
            print(f"终止进程 {args.pid} 失败")

# ============================================
# 第三部分：测试代码
# ============================================

if __name__ == '__main__':
    import sys

    # 检查是否提供了命令行参数
    if len(sys.argv) > 1:
        # 有参数，执行命令行模式
        main()
    else:
        # 无参数，执行测试模式
        print("测试模式：演示系统监控工具")
        print("=" * 60)

        monitor = SystemMonitor()

        # 测试1：系统快照
        print("\n测试1：系统快照")
        print("-" * 60)
        data = monitor.monitor_once()
        print(f"时间：{data['timestamp']}")
        print(f"CPU：{data['cpu']['percent']}%")
        print(f"内存：{data['memory']['percent']}%")
        print(f"磁盘数：{len(data['disk'])}")
        if data['alerts']:
            print("告警：")
            for alert in data['alerts']:
                print(f"  {alert}")

        # 测试2：资源占用最高的进程
        print("\n测试2：资源占用最高的 5 个进程")
        print("-" * 60)
        processes = monitor.get_top_processes(5, 'memory')
        for i, proc in enumerate(processes, 1):
            print(f"{i}. {proc['name']} (PID: {proc['pid']})")
            print(f"   CPU: {proc['cpu']}%, 内存: {proc['memory']/(1024**2):.2f} MB")

        # 测试3：搜索进程
        print("\n测试3：搜索 Python 进程")
        print("-" * 60)
        processes = monitor.search_process('python')
        print(f"找到 {len(processes)} 个 Python 进程")
        for proc in processes[:3]:
            print(f"  {proc['name']} (PID: {proc['pid']})")

        print("\n" + "=" * 60)
        print("测试完成！")
        print("\n使用方法：")
        print("  # 实时监控")
        print("  python 61_系统监控工具.py monitor --interval 5 --duration 60")
        print("\n  # 系统快照")
        print("  python 61_系统监控工具.py snapshot")
        print("\n  # 资源占用最高的进程")
        print("  python 61_系统监控工具.py top -n 10 --sort memory")
        print("\n  # 搜索进程")
        print("  python 61_系统监控工具.py search python")
        print("\n  # 终止进程")
        print("  python 61_系统监控工具.py kill <PID>")
        print("=" * 60)
