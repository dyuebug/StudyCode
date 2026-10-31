// ============================================================================
//  file-manager · 第 14 周「CLI 文件管理器」的补做
// ============================================================================
//  日期：2026-11-09（第45天）
//  难度：⭐⭐⭐
//
//  这个项目的价值不在"能列目录"，而在三条**必须做对**的工程细节：
//
//  1. **不跟随符号链接**：`symlink_metadata` 而不是 `metadata`。
//     用 `metadata` 会把链接解析成目标，遇到 `a -> b -> a` 直接无限递归。
//     而且这个 bug 在 Linux 上"看起来能用"，只有真遇到环才炸。
//
//  2. **错误累积而不是中断**：撞到没权限的子目录时，继续走完剩下的，
//     最后统一报告。`?` 提前返回会让"有一个目录没权限"变成"整次扫描失败"。
//
//  3. **流式复制**：64 KiB 缓冲区 + 循环读写，内存 O(1)。
//     `fs::read` + `fs::write` 在 4 GB 文件上会把内存吃光——而它"成功过"。
//
//  自测方式见 tests/，断言的是行为（计数、字节数、链接不被展开），不是输出文案。
// ============================================================================

use std::fs;
use std::io::{self, Read, Write};
use std::path::{Path, PathBuf};

/// 参数解析单独成模块，好处是它能被单元测试直接喂参数
pub mod cli;

/// 遍历结果中的一条。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Entry {
    pub path: PathBuf,
    /// 相对 `root` 的层级，`root` 的直接子项为 1
    pub depth: usize,
    pub is_dir: bool,
    pub len: u64,
    /// 是否为符号链接（被列出但**不展开**）
    pub is_symlink: bool,
}

/// 一次遍历的汇总。错误单独收集，不打断遍历。
#[derive(Debug, Default, Clone, PartialEq, Eq)]
pub struct WalkStats {
    pub files: usize,
    pub dirs: usize,
    pub bytes: u64,
    pub symlinks_skipped: usize,
    /// `(出错的路径, 原因)`：让调用方决定是报告还是忽略
    pub errors: Vec<(PathBuf, String)>,
}

/// 递归遍历 `root`，最多下钻 `max_depth` 层（`1` = 只看直接子项）。
///
/// 返回 `(按路径排序的条目, 汇总)`，顺序固定便于断言。
pub fn walk(root: &Path, max_depth: usize) -> (Vec<Entry>, WalkStats) {
    let mut entries = Vec::new();
    let mut stats = WalkStats::default();

    // 根目录本身读不到时也要留痕，而不是静默返回空列表
    if let Err(e) = fs::read_dir(root) {
        stats.errors.push((root.to_path_buf(), e.to_string()));
        return (entries, stats);
    }

    walk_inner(root, 0, max_depth, &mut entries, &mut stats);
    entries.sort_by(|a, b| a.path.cmp(&b.path));
    (entries, stats)
}

fn walk_inner(
    dir: &Path,
    depth: usize,
    max_depth: usize,
    out: &mut Vec<Entry>,
    stats: &mut WalkStats,
) {
    let read_dir = match fs::read_dir(dir) {
        Ok(rd) => rd,
        Err(e) => {
            stats.errors.push((dir.to_path_buf(), e.to_string()));
            return;
        }
    };

    for item in read_dir {
        let item = match item {
            Ok(i) => i,
            Err(e) => {
                stats.errors.push((dir.to_path_buf(), e.to_string()));
                continue;
            }
        };

        let path = item.path();

        // 关键：symlink_metadata **不跟随**链接
        let meta = match fs::symlink_metadata(&path) {
            Ok(m) => m,
            Err(e) => {
                stats.errors.push((path, e.to_string()));
                continue;
            }
        };

        let file_type = meta.file_type();

        if file_type.is_symlink() {
            // 列出但不进入：进入就可能成环
            stats.symlinks_skipped += 1;
            out.push(Entry {
                path,
                depth: depth + 1,
                is_dir: false,
                len: 0,
                is_symlink: true,
            });
            continue;
        }

        if file_type.is_dir() {
            stats.dirs += 1;
            out.push(Entry {
                path: path.clone(),
                depth: depth + 1,
                is_dir: true,
                len: 0,
                is_symlink: false,
            });
            if depth + 1 < max_depth {
                walk_inner(&path, depth + 1, max_depth, out, stats);
            }
            continue;
        }

        stats.files += 1;
        stats.bytes += meta.len();
        out.push(Entry {
            path,
            depth: depth + 1,
            is_dir: false,
            len: meta.len(),
            is_symlink: false,
        });
    }
}

/// 按扩展名筛选（大小写不敏感，不含点）。
pub fn find_by_ext(root: &Path, ext: &str) -> (Vec<PathBuf>, WalkStats) {
    let (entries, stats) = walk(root, usize::MAX);
    let want = ext.trim_start_matches('.').to_ascii_lowercase();
    let hits = entries
        .into_iter()
        .filter(|e| {
            !e.is_dir
                && !e.is_symlink
                && e.path
                    .extension()
                    .map(|x| x.to_string_lossy().to_ascii_lowercase() == want)
                    .unwrap_or(false)
        })
        .map(|e| e.path)
        .collect();
    (hits, stats)
}

/// 流式复制，固定 64 KiB 缓冲区，内存占用与文件大小无关。
///
/// `dry_run` 时只统计**将会**复制多少字节，不写任何文件。
pub fn copy_streaming(src: &Path, dst: &Path, dry_run: bool) -> io::Result<u64> {
    let src_len = fs::metadata(src)?.len();

    if dry_run {
        return Ok(src_len);
    }

    if let Some(parent) = dst.parent() {
        if !parent.as_os_str().is_empty() {
            fs::create_dir_all(parent)?;
        }
    }

    let mut input = fs::File::open(src)?;
    let mut output = fs::File::create(dst)?;
    let mut buf = vec![0u8; 64 * 1024];
    let mut total = 0u64;

    loop {
        let n = input.read(&mut buf)?;
        if n == 0 {
            break;
        }
        output.write_all(&buf[..n])?;
        total += n as u64;
    }
    output.flush()?;

    // 产物断言：复制"成功"不等于字节数对得上
    if total != src_len {
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!("字节数不一致：写入 {total}，源 {src_len}"),
        ));
    }
    Ok(total)
}

/// 人类可读体积。写成独立函数是为了能被测试直接断言。
pub fn human(bytes: u64) -> String {
    const UNITS: [&str; 5] = ["B", "KB", "MB", "GB", "TB"];
    let mut value = bytes as f64;
    let mut unit = 0;
    while value >= 1024.0 && unit + 1 < UNITS.len() {
        value /= 1024.0;
        unit += 1;
    }
    if unit == 0 {
        format!("{bytes} B")
    } else {
        format!("{value:.1} {}", UNITS[unit])
    }
}
