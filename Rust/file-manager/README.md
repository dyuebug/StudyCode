# file-manager

第 45 天补做项：**第 14 周的 CLI 文件管理器项目**（原路线图点名但一直未落地）。
零依赖，只用 std。

目标不是"能列目录"，而是三条在真实 CLI 里必须做对的工程细节：
**不跟随符号链接**、**错误累积而不是中断**、**流式复制**。

---

## 1. 当前状态（2026-11-09）

| 项 | 状态 | 依据 |
|---|---|---|
| 编译 | ✅ 零警告 | `cargo build --offline --all-targets` |
| 测试 | ✅ 16 条通过 | 7 个单测（cli）+ 9 个集成测试 |
| 格式 / 静态检查 | ✅ | `cargo fmt --check`、`clippy -D warnings` 退出码 0 |
| 依赖 | ✅ **零外部依赖** | 见 §6，clap 在本机不可用 |
| 退出码 | ✅ 用法错误为 1 | `./file-manager tree` → `exit=1` |
| 符号链接保护 | ✅ **已验证**（junction） | 见 §3.1：`mklink /J` 免特权，且改回 `metadata` 后测试立刻变红 |

---

## 2. 命令

```bash
file-manager tree <路径> [--depth N]        # 递归列出目录树，默认 N=3
file-manager find <路径> --ext <扩展名>      # 按扩展名查找（大小写不敏感）
file-manager du <路径>                      # 统计体积
file-manager copy <源> <目标> [--dry-run]    # 流式复制
file-manager help
```

实测输出：

```
$ ./file-manager tree src --depth 2
cli.rs  6.8 KB
lib.rs  6.7 KB
main.rs  3.3 KB

$ ./file-manager du src
目录：src
文件 3 个 / 子目录 0 个
合计 16.9 KB

$ ./file-manager tree            # 缺参数
用法错误：tree 需要 <路径>
...
$ echo $?
1
```

---

## 3. 三条实测结论

1. **`symlink_metadata` 不是 `metadata` 的别名**。
   `metadata` 会跟随链接去读目标，遇到 `a -> b -> a` 直接无限递归。
   本项目用 `symlink_metadata`，把链接**列出但不展开**，并计入
   `WalkStats.symlinks_skipped`。

   这条保护**已验证**，而且是在没有管理员权限的 Windows 上：
   `symlink_dir` 会失败（需开发者模式），但目录 **junction**（`mklink /J`）不需要特权，
   而 std 把 junction 也报告为 `is_symlink == true`。
   测试先试 symlink、失败再退到 junction，并单独用
   `junction_被识别为符号链接` 锁定这个前提（哪天 std 改了行为，这条先红）。

   可证伪性验证：把 `lib.rs` 里的 `symlink_metadata` 换回 `metadata`，
   `符号链接被列出但不展开` 立刻失败于
   `assert_eq!(stats.symlinks_skipped, 1)`（8 passed; 1 failed）。换回后恢复 9 passed。

2. **扫描出错不该让整条命令失败**。
   撞到没权限的子目录时，`?` 会让"一个目录读不了"变成"整次扫描失败"。
   本项目把错误收集进 `errors: Vec<(PathBuf, String)>`，
   遍历继续，最后统一打到 stderr —— 但**不影响退出码**。
   这就是"不能静默失败"和"不能过度失败"之间的那条线。

3. **管道会吃掉退出码**。
   `./file-manager tree | head -3; echo $?` 得到 `0`（head 的退出码），
   而 `./file-manager tree >/dev/null 2>&1; echo $?` 得到真实的 `1`。
   加 `set -o pipefail` 后管道恢复为 `1`。
   这不是本项目独有的坑，是任何 CLI 自测都会踩的。

---

## 4. 目录结构

```
file-manager/
├── src/
│   ├── lib.rs   221 行  walk / find_by_ext / copy_streaming / human
│   ├── cli.rs   228 行  手写子命令解析（含 7 个单测）
│   └── main.rs  100 行  只做三件事：解析 → 调 lib → 打印
└── tests/
    └── file_manager.rs  194 行  9 条集成测试（临时目录 + Drop 清理）
```

边界：`main.rs` 不含业务逻辑，所以 `tests/` 能直接测 `lib` 而不必跑进程。
解析器写成纯函数 `parse<I: IntoIterator<Item=String>>`，测试直接喂 `["tree", "."]`。

---

## 5. 测试断言的是行为，不是文案

| 测试 | 断言的产物 |
|---|---|
| `遍历计数与字节数正确` | `files=3 / dirs=2 / bytes=10` |
| `深度限制生效` | 深度 1 时深层文件不出现 |
| `符号链接被列出但不展开` | `b.txt` 只应出现 1 次（跟随链接会变 2） |
| `junction_被识别为符号链接` | 退路前提：junction 必须被 std 报成 symlink |
| `流式复制字节一致` | 5 MiB 文件复制后**逐字节相同** |
| `dry_run_不创建任何文件` | 目标与其父目录**都不存在** |
| `根目录不存在时记录错误而不是崩溃` | `errors.len() == 1` |

`流式复制` 用 5 MiB（远大于 64 KiB 缓冲）确保走多轮循环；
复制完成后再比对 `total == src_len`，因为"没报错"不等于"字节数对得上"。

---

## 6. 为什么零依赖（如实记录）

原本计划用 `clap`。首次 `cargo test --offline` 直接失败：

```
error: failed to download `clap v4.6.1`
```

本机 crates.io 不可达（`curl` 返回 `000`），且本地注册表缓存里
只有 `clap` 的传递依赖片段（anstyle / strsim），**没有 clap 本体**。

于是改成零依赖方案，并手写 `cli.rs`。这个取舍记在 `src/cli.rs` 头部：
手写版**故意没有**实现短选项合并、选项与位置参数混排、自动 help 排版。

其中 `--dry-run` 这类**开关**最容易写错——如果把它当成"带值的选项"，
`copy a.txt b.txt --dry-run` 会把 `b.txt` 吞成它的值，位置参数就少一个。
`cli.rs::开关不会吃掉后面的位置参数` 就是为这个 bug 写的回归测试。

---

## 7. 未验证 / 已知局限

- 符号链接保护只在 **Windows + junction** 上验证过；Unix 真 symlink 与符号链接环
  （`a -> b -> a`）未实测，`is_symlink` 对 junction 与 symlink 的行为可能不同
- 未处理硬链接、未处理 ACL / 权限位保留（`copy` 只复制内容）
- `copy` 不支持目录递归复制
- 未测超大目录（10 万文件）下的性能与内存
- 无进度条、无并发遍历（`walk` 是单线程递归）
- 帮助手写，未随参数自动生成，改选项时容易忘记同步

---

## 8. 下一步（若继续）

1. 在 WSL 或 Linux 上跑一次真实的 `symlink()` 用例，并构造 `a -> b -> a` 环，
   确认深度受限与不跟随两条保护都生效（junction 无法构造环）
2. `copy` 支持目录 + 保留修改时间
3. 用 `walkdir` 或自写迭代器替掉递归，避免深目录爆栈
