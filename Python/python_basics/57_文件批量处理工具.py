# ============================================
# 文件批量处理工具 - 实战项目
# 日期：2026-05-31（第22天）
# 难度：⭐⭐⭐⭐ 综合应用
# ============================================

"""
【项目目标】

开发一个文件批量处理工具，能够：
1. 批量重命名文件
2. 批量替换文件内容
3. 文件筛选和过滤
4. 安全的批量操作（预览、确认）

【用到的知识点】
- pathlib 模块（路径操作）
- 正则表达式（文件名匹配、内容替换）
- 迭代器（处理大量文件）
- 命令行参数（argparse）
"""

import re
import argparse
from pathlib import Path
from datetime import datetime

# ============================================
# 第一部分：文件批量重命名
# ============================================

print("=" * 60)
print("文件批量处理工具 - 实战项目")
print("=" * 60)

class FileRenamer:
    """
    文件批量重命名工具

    支持的重命名模式：
    1. 前缀/后缀添加
    2. 正则表达式替换
    3. 序号重命名
    """

    def __init__(self, directory, pattern='*', dry_run=True):
        """
        初始化重命名工具

        参数：
            directory - 目标目录
            pattern   - 文件匹配模式（如 '*.txt'）
            dry_run   - 是否预览模式（True=只预览，False=实际执行）
        """
        self.directory = Path(directory)
        self.pattern = pattern
        self.dry_run = dry_run

    def add_prefix(self, prefix):
        """
        添加前缀

        示例：'file.txt' -> 'prefix_file.txt'
        """
        files = list(self.directory.glob(self.pattern))
        changes = []

        for file in files:
            if file.is_file():
                new_name = f"{prefix}{file.name}"
                new_path = file.parent / new_name
                changes.append((file, new_path))

        return self._apply_changes(changes)

    def add_suffix(self, suffix):
        """
        添加后缀（在扩展名前）

        示例：'file.txt' -> 'file_suffix.txt'
        """
        files = list(self.directory.glob(self.pattern))
        changes = []

        for file in files:
            if file.is_file():
                new_name = f"{file.stem}{suffix}{file.suffix}"
                new_path = file.parent / new_name
                changes.append((file, new_path))

        return self._apply_changes(changes)

    def replace_pattern(self, old_pattern, new_pattern):
        """
        正则表达式替换

        示例：'file_001.txt' -> 'doc_001.txt'
              (old_pattern='file', new_pattern='doc')
        """
        files = list(self.directory.glob(self.pattern))
        changes = []

        for file in files:
            if file.is_file():
                new_name = re.sub(old_pattern, new_pattern, file.name)
                if new_name != file.name:  # 只有名称改变时才添加
                    new_path = file.parent / new_name
                    changes.append((file, new_path))

        return self._apply_changes(changes)

    def number_files(self, prefix='file', start=1, digits=3):
        """
        序号重命名

        示例：'file_001.txt', 'file_002.txt', ...
        """
        files = sorted(self.directory.glob(self.pattern))
        changes = []

        for i, file in enumerate(files, start):
            if file.is_file():
                number = str(i).zfill(digits)  # 补零
                new_name = f"{prefix}_{number}{file.suffix}"
                new_path = file.parent / new_name
                changes.append((file, new_path))

        return self._apply_changes(changes)

    def _apply_changes(self, changes):
        """
        应用重命名变更

        返回：成功重命名的文件数
        """
        if not changes:
            print("没有文件需要重命名")
            return 0

        # 显示预览
        print(f"\n{'预览模式' if self.dry_run else '执行模式'}：")
        print("-" * 60)
        for old_path, new_path in changes:
            print(f"  {old_path.name} -> {new_path.name}")

        # 如果是预览模式，不实际执行
        if self.dry_run:
            print(f"\n预览：将重命名 {len(changes)} 个文件")
            print("提示：使用 dry_run=False 实际执行")
            return 0

        # 实际执行重命名
        success_count = 0
        for old_path, new_path in changes:
            try:
                old_path.rename(new_path)
                success_count += 1
            except Exception as e:
                print(f"错误：无法重命名 {old_path.name} - {e}")

        print(f"\n成功重命名 {success_count}/{len(changes)} 个文件")
        return success_count

# ============================================
# 第二部分：文件内容批量替换
# ============================================

class FileContentReplacer:
    """
    文件内容批量替换工具

    支持：
    1. 简单字符串替换
    2. 正则表达式替换
    3. 多文件批量处理
    """

    def __init__(self, directory, pattern='*.txt', dry_run=True):
        """
        初始化替换工具

        参数：
            directory - 目标目录
            pattern   - 文件匹配模式
            dry_run   - 是否预览模式
        """
        self.directory = Path(directory)
        self.pattern = pattern
        self.dry_run = dry_run

    def replace_text(self, old_text, new_text, use_regex=False):
        """
        批量替换文本

        参数：
            old_text  - 要替换的文本（或正则表达式）
            new_text  - 替换后的文本
            use_regex - 是否使用正则表达式
        """
        files = list(self.directory.rglob(self.pattern))
        changes = []

        for file in files:
            if file.is_file():
                try:
                    # 读取文件内容
                    content = file.read_text(encoding='utf-8')

                    # 执行替换
                    if use_regex:
                        new_content = re.sub(old_text, new_text, content)
                    else:
                        new_content = content.replace(old_text, new_text)

                    # 如果内容有变化，记录
                    if new_content != content:
                        count = content.count(old_text) if not use_regex else len(re.findall(old_text, content))
                        changes.append((file, content, new_content, count))

                except Exception as e:
                    print(f"警告：无法处理 {file.name} - {e}")

        return self._apply_changes(changes, old_text, new_text)

    def _apply_changes(self, changes, old_text, new_text):
        """
        应用内容替换

        返回：成功替换的文件数
        """
        if not changes:
            print("没有文件需要替换")
            return 0

        # 显示预览
        print(f"\n{'预览模式' if self.dry_run else '执行模式'}：")
        print(f"替换：'{old_text}' -> '{new_text}'")
        print("-" * 60)

        total_replacements = 0
        for file, old_content, new_content, count in changes:
            print(f"  {file.name}: {count} 处替换")
            total_replacements += count

        print(f"\n总计：{len(changes)} 个文件，{total_replacements} 处替换")

        # 如果是预览模式，不实际执行
        if self.dry_run:
            print("提示：使用 dry_run=False 实际执行")
            return 0

        # 实际执行替换
        success_count = 0
        for file, old_content, new_content, count in changes:
            try:
                file.write_text(new_content, encoding='utf-8')
                success_count += 1
            except Exception as e:
                print(f"错误：无法写入 {file.name} - {e}")

        print(f"\n成功替换 {success_count}/{len(changes)} 个文件")
        return success_count

# ============================================
# 第三部分：文件筛选工具
# ============================================

class FileFilter:
    """
    文件筛选工具

    支持按：
    1. 文件大小
    2. 修改时间
    3. 文件名模式
    """

    def __init__(self, directory):
        self.directory = Path(directory)

    def filter_by_size(self, min_size=0, max_size=float('inf')):
        """
        按文件大小筛选

        参数：
            min_size - 最小大小（字节）
            max_size - 最大大小（字节）
        """
        files = []
        for file in self.directory.rglob('*'):
            if file.is_file():
                size = file.stat().st_size
                if min_size <= size <= max_size:
                    files.append((file, size))

        return sorted(files, key=lambda x: x[1], reverse=True)

    def filter_by_extension(self, extensions):
        """
        按扩展名筛选

        参数：
            extensions - 扩展名列表（如 ['.txt', '.py']）
        """
        files = []
        for file in self.directory.rglob('*'):
            if file.is_file() and file.suffix in extensions:
                files.append(file)

        return sorted(files)

    def filter_by_pattern(self, pattern):
        """
        按正则表达式筛选文件名

        参数：
            pattern - 正则表达式模式
        """
        files = []
        regex = re.compile(pattern)

        for file in self.directory.rglob('*'):
            if file.is_file() and regex.search(file.name):
                files.append(file)

        return sorted(files)

# ============================================
# 第四部分：命令行接口
# ============================================

def main():
    """主函数：命令行入口"""

    parser = argparse.ArgumentParser(
        description='文件批量处理工具 - 重命名、替换、筛选'
    )

    subparsers = parser.add_subparsers(dest='command', help='子命令')

    # 重命名命令
    rename_parser = subparsers.add_parser('rename', help='批量重命名文件')
    rename_parser.add_argument('directory', help='目标目录')
    rename_parser.add_argument('--pattern', default='*', help='文件匹配模式')
    rename_parser.add_argument('--prefix', help='添加前缀')
    rename_parser.add_argument('--suffix', help='添加后缀')
    rename_parser.add_argument('--replace', nargs=2, metavar=('OLD', 'NEW'), help='替换模式')
    rename_parser.add_argument('--number', nargs='?', const='file', help='序号重命名')
    rename_parser.add_argument('--execute', action='store_true', help='实际执行（默认预览）')

    # 替换命令
    replace_parser = subparsers.add_parser('replace', help='批量替换文件内容')
    replace_parser.add_argument('directory', help='目标目录')
    replace_parser.add_argument('old', help='要替换的文本')
    replace_parser.add_argument('new', help='替换后的文本')
    replace_parser.add_argument('--pattern', default='*.txt', help='文件匹配模式')
    replace_parser.add_argument('--regex', action='store_true', help='使用正则表达式')
    replace_parser.add_argument('--execute', action='store_true', help='实际执行（默认预览）')

    # 筛选命令
    filter_parser = subparsers.add_parser('filter', help='筛选文件')
    filter_parser.add_argument('directory', help='目标目录')
    filter_parser.add_argument('--size', nargs=2, type=int, metavar=('MIN', 'MAX'), help='文件大小范围（字节）')
    filter_parser.add_argument('--ext', nargs='+', help='扩展名列表')
    filter_parser.add_argument('--pattern', help='文件名正则表达式')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return

    # 执行对应命令
    if args.command == 'rename':
        renamer = FileRenamer(args.directory, args.pattern, dry_run=not args.execute)

        if args.prefix:
            renamer.add_prefix(args.prefix)
        elif args.suffix:
            renamer.add_suffix(args.suffix)
        elif args.replace:
            renamer.replace_pattern(args.replace[0], args.replace[1])
        elif args.number:
            renamer.number_files(prefix=args.number)
        else:
            print("错误：请指定重命名操作（--prefix, --suffix, --replace, --number）")

    elif args.command == 'replace':
        replacer = FileContentReplacer(args.directory, args.pattern, dry_run=not args.execute)
        replacer.replace_text(args.old, args.new, use_regex=args.regex)

    elif args.command == 'filter':
        filter_tool = FileFilter(args.directory)

        if args.size:
            files = filter_tool.filter_by_size(args.size[0], args.size[1])
            print(f"\n找到 {len(files)} 个文件：")
            for file, size in files:
                print(f"  {file.name}: {size} 字节")

        elif args.ext:
            files = filter_tool.filter_by_extension(args.ext)
            print(f"\n找到 {len(files)} 个文件：")
            for file in files:
                print(f"  {file}")

        elif args.pattern:
            files = filter_tool.filter_by_pattern(args.pattern)
            print(f"\n找到 {len(files)} 个文件：")
            for file in files:
                print(f"  {file}")

        else:
            print("错误：请指定筛选条件（--size, --ext, --pattern）")

# ============================================
# 第五部分：测试代码
# ============================================

if __name__ == '__main__':
    import sys

    # 检查是否提供了命令行参数
    if len(sys.argv) > 1:
        # 有参数，执行命令行模式
        main()
    else:
        # 无参数，执行测试模式
        print("测试模式：演示文件批量处理")
        print("=" * 60)

        # 创建测试目录和文件
        test_dir = Path("test_batch")
        test_dir.mkdir(exist_ok=True)

        # 创建测试文件
        test_files = [
            "file_001.txt",
            "file_002.txt",
            "document_001.txt",
            "report.txt"
        ]

        for filename in test_files:
            file_path = test_dir / filename
            file_path.write_text(f"这是 {filename} 的内容\n包含一些测试文本。")

        print(f"\n创建测试目录：{test_dir}")
        print(f"创建测试文件：{len(test_files)} 个")

        # 测试1：重命名（添加前缀）
        print("\n" + "=" * 60)
        print("测试1：添加前缀")
        renamer = FileRenamer(test_dir, '*.txt', dry_run=True)
        renamer.add_prefix('backup_')

        # 测试2：内容替换
        print("\n" + "=" * 60)
        print("测试2：内容替换")
        replacer = FileContentReplacer(test_dir, '*.txt', dry_run=True)
        replacer.replace_text('测试', '示例')

        # 测试3：文件筛选
        print("\n" + "=" * 60)
        print("测试3：文件筛选")
        filter_tool = FileFilter(test_dir)
        files = filter_tool.filter_by_extension(['.txt'])
        print(f"找到 {len(files)} 个 .txt 文件")

        print("\n" + "=" * 60)
        print("测试完成！")
        print("\n使用方法：")
        print("  # 重命名")
        print("  python 57_文件批量处理工具.py rename <目录> --prefix backup_")
        print("  python 57_文件批量处理工具.py rename <目录> --number --execute")
        print("\n  # 替换")
        print("  python 57_文件批量处理工具.py replace <目录> '旧文本' '新文本' --execute")
        print("\n  # 筛选")
        print("  python 57_文件批量处理工具.py filter <目录> --ext .txt .py")
        print("=" * 60)
