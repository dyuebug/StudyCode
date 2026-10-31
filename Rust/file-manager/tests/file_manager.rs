// 断言的是行为（计数 / 字节数 / 是否展开链接），不是日志文案。

use file_manager::{copy_streaming, find_by_ext, human, walk};
use std::fs;
use std::path::{Path, PathBuf};

/// 临时目录守卫：`Drop` 里清理，测试 panic 也能删。
struct TempTree {
    root: PathBuf,
}

impl TempTree {
    fn new(tag: &str) -> Self {
        let root = std::env::temp_dir().join(format!("file-manager-test-{tag}"));
        // 上次失败可能留下残骸，先清空
        let _ = fs::remove_dir_all(&root);
        fs::create_dir_all(root.join("sub").join("deep")).unwrap();
        fs::write(root.join("a.txt"), b"hello").unwrap(); // 5 B
        fs::write(root.join("sub").join("b.txt"), b"abc").unwrap(); // 3 B
        fs::write(root.join("sub").join("deep").join("c.log"), b"xy").unwrap(); // 2 B
        Self { root }
    }

    fn path(&self) -> &Path {
        &self.root
    }
}

impl Drop for TempTree {
    fn drop(&mut self) {
        // Windows 上文件可能仍被占用，重试几次
        for _ in 0..5 {
            match fs::remove_dir_all(&self.root) {
                Ok(()) => return,
                Err(_) => std::thread::sleep(std::time::Duration::from_millis(20)),
            }
        }
    }
}

#[test]
fn 遍历计数与字节数正确() {
    let t = TempTree::new("walk");
    let (entries, stats) = walk(t.path(), usize::MAX);

    assert_eq!(stats.files, 3, "三个文件");
    assert_eq!(stats.dirs, 2, "sub 与 sub/deep");
    assert_eq!(stats.bytes, 10, "5 + 3 + 2");
    assert!(stats.errors.is_empty(), "不该有错误：{:?}", stats.errors);
    assert_eq!(entries.len(), 5);
}

#[test]
fn 深度限制生效() {
    let t = TempTree::new("depth");
    let (entries, stats) = walk(t.path(), 1);

    // 只看直接子项：a.txt + sub，不应出现 b.txt / c.log
    assert_eq!(stats.files, 1);
    assert_eq!(stats.dirs, 1);
    assert!(!entries.iter().any(|e| e.path.ends_with("b.txt")));
}

#[test]
fn 符号链接被列出但不展开() {
    let t = TempTree::new("symlink");
    let link = t.path().join("link-to-sub");

    let kind = create_dir_symlink(&t.root.join("sub"), &link);
    if kind.is_none() {
        // 只有 symlink 与 junction 两条路都失败才跳过（本机实测两条都可用）
        eprintln!("跳过：当前环境既不能创建符号链接也不能创建 junction");
        return;
    }
    eprintln!("链接创建方式：{}", kind.unwrap());

    let (entries, stats) = walk(t.path(), usize::MAX);
    assert_eq!(stats.symlinks_skipped, 1);
    let link_entry = entries
        .iter()
        .find(|e| e.is_symlink)
        .expect("应列出链接本身");
    assert!(!link_entry.is_dir, "链接不应被当作目录下钻");
    // 关键断言：链接目标里的 b.txt 只应出现一次（通过真实路径），不因链接重复
    let b_count = entries.iter().filter(|e| e.path.ends_with("b.txt")).count();
    assert_eq!(b_count, 1, "跟着链接走会变成 2 个");
}

/// 创建指向目录的链接，返回实际使用的机制（用于日志与文档）。
///
/// Windows 上 `symlink_dir` 需要管理员或开发者模式；失败了就退到 **junction**
/// （`mklink /J`），它不需要特权。
/// 退路成立的前提是 std 把 junction 也报成 `is_symlink == true` —— 这一点
/// 由 `junction_被识别为符号链接` 单独断言，不靠假设。
#[cfg(windows)]
fn create_dir_symlink(src: &Path, dst: &Path) -> Option<&'static str> {
    if std::os::windows::fs::symlink_dir(src, dst).is_ok() {
        return Some("symlink");
    }
    let ok = std::process::Command::new("cmd")
        .args(["/c", "mklink", "/J"])
        .arg(dst)
        .arg(src)
        .output()
        .map(|o| o.status.success())
        .unwrap_or(false);
    ok.then_some("junction")
}

#[cfg(not(windows))]
fn create_dir_symlink(src: &Path, dst: &Path) -> Option<&'static str> {
    std::os::unix::fs::symlink(src, dst)
        .ok()
        .map(|()| "symlink")
}

/// 单独锁定"junction 被 std 当作符号链接"这个前提。
/// 若哪天 std 改了行为，这条测试会先红，而不是让上一条测试悄悄失去意义。
#[cfg(windows)]
#[test]
fn junction_被识别为符号链接() {
    let t = TempTree::new("junction-probe");
    let link = t.path().join("j");
    let made = std::process::Command::new("cmd")
        .args(["/c", "mklink", "/J"])
        .arg(&link)
        .arg(t.path().join("sub"))
        .output()
        .map(|o| o.status.success())
        .unwrap_or(false);
    if !made {
        eprintln!("跳过：junction 也创建失败");
        return;
    }
    let meta = fs::symlink_metadata(&link).unwrap();
    assert!(
        meta.file_type().is_symlink(),
        "junction 未被识别为 symlink，遍历保护测试将失去意义"
    );
}

#[test]
fn 按扩展名查找大小写不敏感() {
    let t = TempTree::new("find");
    let (hits, _) = find_by_ext(t.path(), "TXT");
    assert_eq!(hits.len(), 2, "a.txt 与 b.txt，实际：{hits:?}");

    let (hits, _) = find_by_ext(t.path(), ".log");
    assert_eq!(hits.len(), 1, "带点的写法也应支持");
}

#[test]
fn 流式复制字节一致() {
    let t = TempTree::new("copy");
    let src = t.path().join("big.bin");
    // 5 MiB：超过 64 KiB 缓冲区，确保走多轮循环
    let data = vec![7u8; 5 * 1024 * 1024];
    fs::write(&src, &data).unwrap();

    let dst = t.path().join("out").join("big.bin");
    let n = copy_streaming(&src, &dst, false).unwrap();

    assert_eq!(n, data.len() as u64);
    assert_eq!(fs::metadata(&dst).unwrap().len(), data.len() as u64);
    assert_eq!(fs::read(&dst).unwrap(), data, "内容必须逐字节相同");
}

#[test]
fn dry_run_不创建任何文件() {
    let t = TempTree::new("dry");
    let src = t.path().join("a.txt");
    let dst = t.path().join("never-created").join("a.txt");

    let n = copy_streaming(&src, &dst, true).unwrap();
    assert_eq!(n, 5);
    assert!(!dst.exists(), "dry-run 不能落盘");
    assert!(!dst.parent().unwrap().exists(), "也不该创建父目录");
}

#[test]
fn 根目录不存在时记录错误而不是崩溃() {
    let (entries, stats) = walk(Path::new("绝对不存在的目录-xyz"), 3);
    assert!(entries.is_empty());
    assert_eq!(stats.errors.len(), 1, "必须留痕：{:?}", stats.errors);
}

#[test]
fn 体积格式化() {
    assert_eq!(human(0), "0 B");
    assert_eq!(human(512), "512 B");
    assert_eq!(human(1024), "1.0 KB");
    assert_eq!(human(1536), "1.5 KB");
    assert_eq!(human(5 * 1024 * 1024), "5.0 MB");
}
