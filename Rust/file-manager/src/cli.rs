// ============================================================================
//  cli.rs · 手写子命令解析（60 行看完 clap 替你做的事）
// ============================================================================
//  写成「纯函数 + 可测」而不是在 main 里直接用 env::args()：
//  解析器接收一个迭代器，测试就能直接喂 `["tree", ".", "--depth", "9"]`。
//
//  这里刻意**没有**做的（正是 clap 的卖点）：
//    - 短选项合并（`-n5`）
//    - 选项与位置参数混排（`tree --depth 2 .` 这种顺序）
//    - 自动 help 排版、颜色、`--version`
//    - 类型错误信息里的「最相近的合法值」提示
// ============================================================================

use std::path::PathBuf;

const HELP: &str = "\
file-manager —— 第14周项目

用法：
  file-manager tree <路径> [--depth N]     递归列出目录树（默认 N=3）
  file-manager find <路径> --ext <扩展名>  按扩展名查找
  file-manager du <路径>                   统计目录体积
  file-manager copy <源> <目标> [--dry-run] 流式复制
  file-manager help                        显示本帮助

退出码：0 成功；1 用法错误或复制失败
";

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Cmd {
    Tree {
        path: PathBuf,
        depth: usize,
    },
    Find {
        path: PathBuf,
        ext: String,
    },
    Du {
        path: PathBuf,
    },
    Copy {
        src: PathBuf,
        dst: PathBuf,
        dry_run: bool,
    },
    Help,
}

/// 解析参数。`args` 不含程序名本身。
///
/// 返回 `Err(String)` 而不是直接打印：错误文案由调用方决定去哪（stdout/stderr）。
pub fn parse<I: IntoIterator<Item = String>>(args: I) -> Result<Cmd, String> {
    let mut it = args.into_iter();
    let head = it.next().ok_or_else(|| "缺少子命令".to_string())?;
    let rest: Vec<String> = it.collect();

    match head.as_str() {
        "help" | "-h" | "--help" => Ok(Cmd::Help),

        "tree" => {
            let (positional, flags) = split(&rest);
            let path = positional
                .first()
                .ok_or_else(|| "tree 需要 <路径>".to_string())?;
            let depth = match flag_value(&flags, "--depth") {
                Some(v) => v
                    .parse::<usize>()
                    .map_err(|_| format!("--depth 需要非负整数，实际：{v:?}"))?,
                None => 3,
            };
            Ok(Cmd::Tree {
                path: PathBuf::from(path),
                depth,
            })
        }

        "find" => {
            let (positional, flags) = split(&rest);
            let path = positional
                .first()
                .ok_or_else(|| "find 需要 <路径>".to_string())?;
            let ext = flag_value(&flags, "--ext")
                .ok_or_else(|| "find 需要 --ext <扩展名>".to_string())?;
            Ok(Cmd::Find {
                path: PathBuf::from(path),
                ext: ext.to_string(),
            })
        }

        "du" => {
            let (positional, _) = split(&rest);
            let path = positional
                .first()
                .ok_or_else(|| "du 需要 <路径>".to_string())?;
            Ok(Cmd::Du {
                path: PathBuf::from(path),
            })
        }

        "copy" => {
            let (positional, flags) = split(&rest);
            if positional.len() < 2 {
                return Err("copy 需要 <源> 与 <目标>".to_string());
            }
            Ok(Cmd::Copy {
                src: PathBuf::from(&positional[0]),
                dst: PathBuf::from(&positional[1]),
                dry_run: flags.iter().any(|(k, _)| k.as_str() == "--dry-run"),
            })
        }

        other => Err(format!("未知子命令：{other}（试试 help）")),
    }
}

/// 把 `--key value` 拆成「位置参数」与「(key, Option<value>)」两组。
///
/// 这是手写解析最容易出错的地方：`--dry-run` 是开关，不该吃掉后面的参数。
fn split(args: &[String]) -> (Vec<&String>, Vec<(&String, Option<&String>)>) {
    const SWITCHES: [&str; 1] = ["--dry-run"];

    let mut positional = Vec::new();
    let mut flags = Vec::new();
    let mut i = 0;

    while i < args.len() {
        let arg = &args[i];
        if arg.starts_with("--") {
            if SWITCHES.contains(&arg.as_str()) {
                flags.push((arg, None));
                i += 1;
            } else if i + 1 < args.len() {
                flags.push((arg, Some(&args[i + 1])));
                i += 2;
            } else {
                flags.push((arg, None));
                i += 1;
            }
        } else {
            positional.push(arg);
            i += 1;
        }
    }

    (positional, flags)
}

fn flag_value<'a>(flags: &'a [(&String, Option<&'a String>)], key: &str) -> Option<&'a str> {
    flags
        .iter()
        .find(|(k, _)| k.as_str() == key)
        .and_then(|(_, v)| v.map(|s| s.as_str()))
}

pub fn help_text() -> &'static str {
    HELP
}

#[cfg(test)]
mod tests {
    use super::*;

    fn p(args: &[&str]) -> Result<Cmd, String> {
        parse(args.iter().map(|s| s.to_string()))
    }

    #[test]
    fn tree_默认深度_3() {
        assert_eq!(
            p(&["tree", "."]).unwrap(),
            Cmd::Tree {
                path: PathBuf::from("."),
                depth: 3
            }
        );
    }

    #[test]
    fn tree_指定深度() {
        assert_eq!(
            p(&["tree", ".", "--depth", "1"]).unwrap(),
            Cmd::Tree {
                path: PathBuf::from("."),
                depth: 1
            }
        );
    }

    #[test]
    fn 深度不是数字时报错() {
        let err = p(&["tree", ".", "--depth", "abc"]).unwrap_err();
        assert!(err.contains("非负整数"), "实际：{err}");
    }

    #[test]
    fn 开关不会吃掉后面的位置参数() {
        // 手写解析最典型的 bug：把 --dry-run 当成带值选项，
        // 于是 dst 被当成它的值，位置参数少一个
        assert_eq!(
            p(&["copy", "a.txt", "b.txt", "--dry-run"]).unwrap(),
            Cmd::Copy {
                src: PathBuf::from("a.txt"),
                dst: PathBuf::from("b.txt"),
                dry_run: true
            }
        );
    }

    #[test]
    fn 缺少参数时报出缺哪个() {
        assert!(p(&["tree"]).unwrap_err().contains("需要 <路径>"));
        assert!(p(&["copy", "only-one"]).unwrap_err().contains("需要 <源>"));
        assert!(p(&["find", "."]).unwrap_err().contains("--ext"));
    }

    #[test]
    fn 未知子命令给提示() {
        assert!(p(&["tre"]).unwrap_err().contains("未知子命令"));
    }

    #[test]
    fn help_三种写法等价() {
        assert_eq!(p(&["help"]).unwrap(), Cmd::Help);
        assert_eq!(p(&["-h"]).unwrap(), Cmd::Help);
        assert_eq!(p(&["--help"]).unwrap(), Cmd::Help);
    }
}
