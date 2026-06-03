# ============================================
# 数据备份工具 - 实战项目
# 日期：2026-05-31（第23天）
# 难度：⭐⭐⭐⭐ 综合应用
# ============================================

"""
【项目目标】

开发一个数据备份工具，能够：
1. 完整备份（压缩所有文件）
2. 增量备份（只备份修改的文件）
3. 备份恢复
4. 文件校验（哈希值）

【用到的知识点】
- zipfile 模块（文件压缩）
- hashlib 模块（文件哈希）
- pathlib 模块（路径操作）
- json 模块（存储备份元数据）
"""

import zipfile
import hashlib
import json
from pathlib import Path
from datetime import datetime

# ============================================
# 第一部分：备份工具核心类
# ============================================

print("=" * 60)
print("数据备份工具 - 实战项目")
print("=" * 60)

class BackupTool:
    """
    数据备份工具

    支持：
    1. 完整备份
    2. 增量备份
    3. 备份恢复
    4. 文件校验
    """

    def __init__(self, source_dir, backup_dir):
        """
        初始化备份工具

        参数：
            source_dir - 源目录（要备份的目录）
            backup_dir - 备份目录（存放备份文件）
        """
        self.source_dir = Path(source_dir)
        self.backup_dir = Path(backup_dir)
        self.backup_dir.mkdir(parents=True, exist_ok=True)

        # 元数据文件（存储文件哈希信息）
        self.metadata_file = self.backup_dir / "backup_metadata.json"
        self.metadata = self._load_metadata()

    def _load_metadata(self):
        """加载备份元数据"""
        if self.metadata_file.exists():
            with open(self.metadata_file, 'r', encoding='utf-8') as f:
                return json.load(f)
        return {}

    def _save_metadata(self):
        """保存备份元数据"""
        with open(self.metadata_file, 'w', encoding='utf-8') as f:
            json.dump(self.metadata, f, indent=2, ensure_ascii=False)

    def _calculate_file_hash(self, file_path):
        """
        计算文件的 SHA-256 哈希值

        参数：
            file_path - 文件路径

        返回：
            哈希值（十六进制字符串）
        """
        h = hashlib.sha256()
        with open(file_path, 'rb') as f:
            while chunk := f.read(8192):
                h.update(chunk)
        return h.hexdigest()

    def full_backup(self):
        """
        完整备份

        压缩源目录下的所有文件
        """
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_name = f"full_backup_{timestamp}.zip"
        backup_path = self.backup_dir / backup_name

        print(f"\n开始完整备份...")
        print(f"源目录：{self.source_dir}")
        print(f"备份文件：{backup_path}")
        print("-" * 60)

        file_count = 0
        total_size = 0

        with zipfile.ZipFile(backup_path, 'w', zipfile.ZIP_DEFLATED) as zf:
            for file in self.source_dir.rglob('*'):
                if file.is_file():
                    # 计算相对路径
                    rel_path = file.relative_to(self.source_dir)

                    # 添加到压缩包
                    zf.write(file, arcname=rel_path)

                    # 计算哈希值
                    file_hash = self._calculate_file_hash(file)

                    # 保存元数据
                    self.metadata[str(rel_path)] = {
                        'hash': file_hash,
                        'size': file.stat().st_size,
                        'mtime': file.stat().st_mtime,
                        'backup': backup_name
                    }

                    file_count += 1
                    total_size += file.stat().st_size
                    print(f"  备份：{rel_path}")

        # 保存元数据
        self._save_metadata()

        backup_size = backup_path.stat().st_size
        ratio = (1 - backup_size / total_size) * 100 if total_size > 0 else 0

        print("-" * 60)
        print(f"完整备份完成！")
        print(f"  文件数：{file_count}")
        print(f"  原始大小：{total_size} 字节")
        print(f"  压缩后：{backup_size} 字节")
        print(f"  压缩率：{ratio:.1f}%")

        return backup_path

    def incremental_backup(self):
        """
        增量备份

        只备份修改过的文件（通过哈希值判断）
        """
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_name = f"incremental_backup_{timestamp}.zip"
        backup_path = self.backup_dir / backup_name

        print(f"\n开始增量备份...")
        print(f"源目录：{self.source_dir}")
        print(f"备份文件：{backup_path}")
        print("-" * 60)

        changed_files = []
        new_files = []

        # 检查文件变化
        for file in self.source_dir.rglob('*'):
            if file.is_file():
                rel_path = str(file.relative_to(self.source_dir))
                current_hash = self._calculate_file_hash(file)

                if rel_path in self.metadata:
                    # 文件已存在，检查是否修改
                    if self.metadata[rel_path]['hash'] != current_hash:
                        changed_files.append((file, rel_path))
                else:
                    # 新文件
                    new_files.append((file, rel_path))

        # 如果没有变化，不创建备份
        if not changed_files and not new_files:
            print("没有文件变化，无需备份")
            return None

        # 创建增量备份
        file_count = 0
        total_size = 0

        with zipfile.ZipFile(backup_path, 'w', zipfile.ZIP_DEFLATED) as zf:
            # 备份修改的文件
            for file, rel_path in changed_files:
                zf.write(file, arcname=rel_path)

                # 更新元数据
                file_hash = self._calculate_file_hash(file)
                self.metadata[rel_path] = {
                    'hash': file_hash,
                    'size': file.stat().st_size,
                    'mtime': file.stat().st_mtime,
                    'backup': backup_name
                }

                file_count += 1
                total_size += file.stat().st_size
                print(f"  修改：{rel_path}")

            # 备份新文件
            for file, rel_path in new_files:
                zf.write(file, arcname=rel_path)

                # 添加元数据
                file_hash = self._calculate_file_hash(file)
                self.metadata[rel_path] = {
                    'hash': file_hash,
                    'size': file.stat().st_size,
                    'mtime': file.stat().st_mtime,
                    'backup': backup_name
                }

                file_count += 1
                total_size += file.stat().st_size
                print(f"  新增：{rel_path}")

        # 保存元数据
        self._save_metadata()

        backup_size = backup_path.stat().st_size
        ratio = (1 - backup_size / total_size) * 100 if total_size > 0 else 0

        print("-" * 60)
        print(f"增量备份完成！")
        print(f"  修改文件：{len(changed_files)}")
        print(f"  新增文件：{len(new_files)}")
        print(f"  总文件数：{file_count}")
        print(f"  原始大小：{total_size} 字节")
        print(f"  压缩后：{backup_size} 字节")
        print(f"  压缩率：{ratio:.1f}%")

        return backup_path

    def restore(self, backup_file, restore_dir):
        """
        恢复备份

        参数：
            backup_file - 备份文件路径
            restore_dir - 恢复目录
        """
        backup_path = Path(backup_file)
        restore_path = Path(restore_dir)
        restore_path.mkdir(parents=True, exist_ok=True)

        print(f"\n开始恢复备份...")
        print(f"备份文件：{backup_path}")
        print(f"恢复目录：{restore_path}")
        print("-" * 60)

        file_count = 0

        with zipfile.ZipFile(backup_path, 'r') as zf:
            for name in zf.namelist():
                zf.extract(name, restore_path)
                file_count += 1
                print(f"  恢复：{name}")

        print("-" * 60)
        print(f"恢复完成！共恢复 {file_count} 个文件")

    def verify(self):
        """
        验证文件完整性

        对比当前文件与元数据中的哈希值
        """
        print(f"\n开始验证文件完整性...")
        print(f"源目录：{self.source_dir}")
        print("-" * 60)

        verified = 0
        modified = 0
        missing = 0

        # 检查元数据中的文件
        for rel_path, info in self.metadata.items():
            file_path = self.source_dir / rel_path

            if not file_path.exists():
                print(f"  [缺失] {rel_path}")
                missing += 1
                continue

            current_hash = self._calculate_file_hash(file_path)
            if current_hash == info['hash']:
                verified += 1
            else:
                print(f"  [修改] {rel_path}")
                modified += 1

        print("-" * 60)
        print(f"验证完成！")
        print(f"  完整：{verified}")
        print(f"  修改：{modified}")
        print(f"  缺失：{missing}")

        return verified, modified, missing


# ============================================
# 第二部分：命令行接口
# ============================================

def main():
    """主函数：命令行入口"""
    import argparse

    parser = argparse.ArgumentParser(
        description='数据备份工具 - 完整备份、增量备份、恢复'
    )

    subparsers = parser.add_subparsers(dest='command', help='子命令')

    # 完整备份命令
    full_parser = subparsers.add_parser('full', help='完整备份')
    full_parser.add_argument('source', help='源目录')
    full_parser.add_argument('backup', help='备份目录')

    # 增量备份命令
    incr_parser = subparsers.add_parser('incremental', help='增量备份')
    incr_parser.add_argument('source', help='源目录')
    incr_parser.add_argument('backup', help='备份目录')

    # 恢复命令
    restore_parser = subparsers.add_parser('restore', help='恢复备份')
    restore_parser.add_argument('backup_file', help='备份文件')
    restore_parser.add_argument('restore_dir', help='恢复目录')

    # 验证命令
    verify_parser = subparsers.add_parser('verify', help='验证文件完整性')
    verify_parser.add_argument('source', help='源目录')
    verify_parser.add_argument('backup', help='备份目录')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return

    # 执行对应命令
    if args.command == 'full':
        tool = BackupTool(args.source, args.backup)
        tool.full_backup()

    elif args.command == 'incremental':
        tool = BackupTool(args.source, args.backup)
        tool.incremental_backup()

    elif args.command == 'restore':
        # 恢复不需要源目录，直接创建工具实例
        tool = BackupTool('.', Path(args.backup_file).parent)
        tool.restore(args.backup_file, args.restore_dir)

    elif args.command == 'verify':
        tool = BackupTool(args.source, args.backup)
        tool.verify()

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
        print("测试模式：演示数据备份工具")
        print("=" * 60)

        # 创建测试目录和文件
        test_source = Path("test_source")
        test_backup = Path("test_backup")

        test_source.mkdir(exist_ok=True)

        # 创建测试文件
        (test_source / "file1.txt").write_text("这是文件1的内容\n" * 10)
        (test_source / "file2.txt").write_text("这是文件2的内容\n" * 10)
        (test_source / "file3.txt").write_text("这是文件3的内容\n" * 10)

        print(f"\n创建测试目录：{test_source}")
        print(f"创建测试文件：3 个")

        # 测试1：完整备份
        print("\n" + "=" * 60)
        print("测试1：完整备份")
        tool = BackupTool(test_source, test_backup)
        backup1 = tool.full_backup()

        # 测试2：修改文件后增量备份
        print("\n" + "=" * 60)
        print("测试2：修改文件后增量备份")
        (test_source / "file1.txt").write_text("修改后的内容\n" * 10)
        (test_source / "file4.txt").write_text("新增的文件4\n" * 10)
        backup2 = tool.incremental_backup()

        # 测试3：验证文件完整性
        print("\n" + "=" * 60)
        print("测试3：验证文件完整性")
        tool.verify()

        # 测试4：恢复备份
        print("\n" + "=" * 60)
        print("测试4：恢复备份")
        test_restore = Path("test_restore")
        if backup1:
            tool.restore(backup1, test_restore)

        print("\n" + "=" * 60)
        print("测试完成！")
        print("\n使用方法：")
        print("  # 完整备份")
        print("  python 59_数据备份工具.py full <源目录> <备份目录>")
        print("\n  # 增量备份")
        print("  python 59_数据备份工具.py incremental <源目录> <备份目录>")
        print("\n  # 恢复备份")
        print("  python 59_数据备份工具.py restore <备份文件> <恢复目录>")
        print("\n  # 验证完整性")
        print("  python 59_数据备份工具.py verify <源目录> <备份目录>")
        print("=" * 60)

