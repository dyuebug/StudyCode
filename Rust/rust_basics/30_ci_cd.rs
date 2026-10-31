// ============================================
// 持续集成（CI）与质量门禁 - 超详细注释版
// 日期：2026-11-07（第43天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】简要说明

是什么？
- CI ≈ 把"人肉检查"换成"机器检查"，并且**结果可复现**。
  它是四件事的组合，缺一不可：

    【固定工具链】  同样是 rustc，1.92 和 1.99 的检查结果可以不同
    【缓存】        没有缓存时，每次都要重编全部依赖（第 42 天实测 279 秒）
    【检查链】      fmt → clippy → test → build，每道闸门防的回归不一样
    【退出码】      非 0 即失败。这是 CI 与"跑个脚本看看"的唯一区别

为什么需要？
- 因为"我这能跑"是最常见的误判来源：
  换台机器、换个人、换一天，结论就可能不同。
  CI 的价值不是"更严格的代码"，而是**同一组判断，任何人跑都得到同一个结果**。

核心特点？
- **检查链是短路的**：第一道闸门失败就不往下走，省时间也省注意力
- **门禁必须能失败**：永远通过的检查等于没有检查
- **"没输出"不是"通过"**：这是第 42 天最贵的一课（stub 二进制静默成功）

Rust 特色？
- `cargo fmt --check` / `cargo clippy -- -D warnings` / `cargo test` 三条命令
  就覆盖了格式、惯用法、行为三层，且都能用退出码表达结果。
- `Cargo.lock` 天然是**依赖图的指纹**：用它做缓存键，比"按日期"精确得多。
- clippy 的结论**绑定工具链版本**：同一条代码在 1.92 干净、在 1.99 可能报警。
  所以"CI 里跑哪个版本"是配置问题，不是随便选一个最近的。

【本文件怎么跑】

```bash
rustc --edition 2021 30_ci_cd.rs -o 30_ci_cd.exe

./30_ci_cd.exe             # 走一遍门禁模拟，退出码 0
./30_ci_cd.exe --gate      # 只跑门禁：退出码即结论（0 通过 / 1 失败）
./30_ci_cd.exe --cache-key # 只打印 Cargo.lock 的缓存键（改一个字节再跑，键会变）
```

⚠️ 本文件用 std 实现了**缓存键计算**与**门禁逻辑**，
不依赖任何第三方 crate，也不真的去调用 cargo（那是 CI 平台的事）。
它要讲清的是"CI 里那些配置为什么那样写"，而不是复刻一个 CI 系统。
*/

use std::env;
use std::fs;
use std::path::Path;

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
1.1 五道闸门，各防各的回归

| 命令 | 防的是什么 | 成本 | 失败时的典型信息 |
|---|---|---|---|
| `cargo fmt --check` | 格式不一致（diff 里出现纯格式改动） | 秒级 | `Diff in src/main.rs at line 12` |
| `cargo clippy -- -D warnings` | 惯用法错误：多余转换、可简化的匹配、易错的写法 | 十秒级 | `error: useless conversion to the same type` |
| `cargo check` | 类型错误（不产出二进制，比 build 快） | 十秒级 | `error[E0308]: mismatched types` |
| `cargo test` | **行为**错误 | 分钟级 | `test xxx ... FAILED` |
| `cargo build --release` | 只在 release 下才暴露的问题（优化/断言差异） | 分钟级 | 编译期错误，或运行时行为变化 |

【为什么是这个顺序】
**从快到慢**。`fmt` 3 秒能发现的问题，不该等 3 分钟的测试跑完才告诉你。
CI 的时间成本是"每次推送都要付一次"的税。

【为什么 `fmt --check` 而不是直接 `fmt`】
CI 里绝不能**改代码**。`--check` 只判断"是否已格式化"，返回非 0 让人在本地改。
CI 改代码会造成一种错觉：这次通过的是"CI 改过的版本"，不是你提交的版本。

【clippy 与 rustc 警告的关系】
rustc 的 lint 是"语言层面"的（未使用变量、不可达代码），clippy 是"惯用法层面"的。
`-D warnings` 把警告升级为错误——**门禁的本质就是把"警告"变成"退出码"**。
没有 `-D warnings`，warnings 不会让 CI 失败，等于没检查。

────────────────────────────────────

1.2 退出码是唯一的契约

CI 平台不看日志内容，只看退出码：

    0    → 通过，继续下一步
    非 0 → 失败，停在这里，标红

所以"检查脚本"必须做到两件事：
1. **失败时退出码非 0**（Python/Rust/Go 都默认如此，Shell 脚本经常忘）
2. **别用 `; cmd` 之类的方式把真实退出码吃掉**（第 42 天的原话：
   `docker build ...; echo $?` 让一次失败的构建被汇报成"成功"）

在 Shell 里正确的写法：

```bash
set -e                 # 任何一步失败就整体失败
cargo fmt --check
cargo clippy --all-targets -- -D warnings
cargo test
```

`set -e` 是 Shell 版的"失败即停"，与 CI 的短路语义一致。

────────────────────────────────────

1.3 缓存键：为什么必须绑定 `Cargo.lock`

CI 每次跑都在**全新的机器**上。没有缓存，就要把 actix-web、tokio 这些依赖
从头编一遍——第 42 天在 Docker 里实测，这一步是 **279.6 秒**。

缓存能命中的前提是"依赖没变"。而"依赖有没有变"的准确判据不是时间，
是 **`Cargo.lock` 的内容**：它记录了每个依赖的精确版本与来源。

    key = hash(Cargo.lock)            // 依赖变了，键就变，缓存自然失效

【常见错误】用日期或分支名做缓存键：
- 按日期 → 每天第一次必然未命中，第二到第 N 次又可能命中**已过期的缓存**
- 按分支 → 同一分支上换了一个依赖版本，键没变，于是命中旧缓存 → **虚假通过**

【为什么不用 `Cargo.toml`】
`Cargo.toml` 写的是"允许的版本范围"（`actix-web = "4"`），
同一个 toml 在不同时间解析出的实际版本可能不同。`Cargo.lock` 才是锁定后的结果。

────────────────────────────────────

1.4 "没有输出"不等于"通过"

这是第 42 天最贵的一课，值得单独成节。

那次 Docker 构建里，"真源码"编译只花了 **0.4 秒** —— 而编一个 actix-web 应用
不可能这么快。真相是它根本没编：镜像里留下的是占位的 `fn main() {}`。
容器起来 **退出码 0、日志空白**，比崩溃还难发现。

映射到 CI 上，同一个陷阱有三种长相：

| 长相 | 看起来 | 实际 |
|---|---|---|
| 测试 0 个用例 | `0 passed` | 测试根本没被发现（路径/命名/feature 不对） |
| 检查命令秒完成 | 快 | 缓存毒化，或用错了目录 |
| 日志空白但退出 0 | 成功 | 脚本吞了输出，退出码来自最后一条 `echo` |

**通用的防御手段**：给关键步骤加"产物断言"或"数量断言"。
比如测试用例数不少于某个值，构建产物的体积/内容符合预期：

```bash
cargo test 2>&1 | tee test.log
grep -q "test result: ok" test.log || exit 1
grep -Eq "test result: ok\. [1-9][0-9]* passed" test.log || {
  echo "0 个用例通过：测试可能根本没被收集"; exit 1; }
```

把"静默失败"变成"响亮失败"，比多写十行断言更值钱。

⚠️ 但别把这条当成万能规则：**`cargo fmt --check` 干净时本来就一句话不说。**
"零输出=可疑"只在"这条命令成功时本该有输出"的前提下成立，
所以下面 2.3 的代码里给 `Gate` 加了一个 `expects_output` 开关。
一刀切的规则只会制造误报，而误报多了，真报警就没人看了。

────────────────────────────────────

1.5 工具链版本是构建输入的一部分

第 42 天实测的对照：

    本机 rustc 1.92.0  →  realtime-chat 编译零警告
    镜像 rustc 1.99.1  →  同一条代码报 `fetch_update` 已废弃

同源码、同依赖，**只是编译器换了**，结论就变了。
所以 CI 配置里必须显式钉住版本，而不是"用最新的"：

```yaml
- uses: dtolnay/rust-toolchain@stable
  with:
    toolchain: "1.92.0"      # 与本地一致，才谈得上"本地能过 CI 也能过"
```

更普遍的做法是用 `rust-toolchain.toml` 把版本进仓库：
任何人 `cargo` 都会自动切到同一个版本，不会出现"你 1.92 我 1.99"。
*/

/// 检查结果
///
/// 【为什么要三态而不是 bool】
/// "通过 / 失败"两态会丢掉一个真实存在的情况：**不知道**。
/// 比如检查命令没有任何输出、或者输出里既没有成功标记也没有失败标记——
/// 这时把它当"通过"就是第 42 天那个 bug（静默成功）。
/// 三态强制调用方回答"我到底凭什么说它通过了"。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum Outcome {
    Pass,
    Fail,
    /// 结果不明：命令跑了，但看不出结论
    Unknown,
}

impl Outcome {
    /// 退出码映射：只有 Pass 是 0
    ///
    /// 【为什么 Unknown 也返回非 0】
    /// CI 的语义是"没证明通过就是没通过"。Unknown 说明检查本身有问题，
    /// 放行它等于埋下一个静默通过的洞。
    fn exit_code(self) -> i32 {
        match self {
            Outcome::Pass => 0,
            Outcome::Fail | Outcome::Unknown => 1,
        }
    }
}

/// 一道闸门
struct Gate {
    /// 命令名（与 CI 里的写法一致，便于对照）
    cmd: &'static str,
    /// 它防的是什么回归
    catches: &'static str,
    /// 本次的结果
    outcome: Outcome,
    /// 本次的输出行数（0 行要特别小心）
    output_lines: usize,
    /// 这条命令**正常情况下会不会有输出**
    ///
    /// 「零输出=可疑」这条规则本身不成立：`cargo fmt --check` 干净时就是一句话不说。
    /// 所以判断前必须先知道"这个命令本来就该沉默吗"。
    /// 这一类"看似通用、实则依赖具体情况"的规则，是自动化检查里最常见的误报来源。
    expects_output: bool,
}

impl Gate {
    fn new(cmd: &'static str, catches: &'static str, outcome: Outcome, output_lines: usize) -> Self {
        Self {
            cmd,
            catches,
            outcome,
            output_lines,
            // 默认认为"成功时会有输出"，需要沉默的项显式声明
            expects_output: true,
        }
    }

    /// 标记"这条命令成功时本来就没有输出"
    fn silent_ok(mut self) -> Self {
        self.expects_output = false;
        self
    }

    /// 这一行日志里该显示什么
    ///
    /// 0 行输出 + 本该有输出 + 结果是 Pass —— 打上"可疑"标记。
    /// 这就是把知识点 1.4 落到代码里：**让异常自己喊出来**。
    fn status_line(&self) -> String {
        let mark = match self.outcome {
            Outcome::Pass => "✅ 通过",
            Outcome::Fail => "❌ 失败",
            Outcome::Unknown => "❓ 结果不明",
        };
        // 失败时把"它本来该拦住什么"一起打出来：
        // 日志的价值不是"红了"，而是"红的时候我该往哪看"
        let what = match self.outcome {
            Outcome::Pass => String::new(),
            _ => format!("  ← 防的是：{}", self.catches),
        };
        let suspicious =
            if self.outcome == Outcome::Pass && self.output_lines == 0 && self.expects_output {
                "   ⚠️ 通过但零输出，先确认检查真的执行了"
            } else {
                ""
            };
        format!(
            "{:<44} {:<10} 输出 {:>3} 行{}{}",
            self.cmd, mark, self.output_lines, what, suspicious
        )
    }
}

/// 按顺序跑闸门，第一道失败就停（短路）
///
/// 【为什么短路】
/// 1. 省时间：fmt 都不过的代码不值得跑 3 分钟的测试
/// 2. 省注意力：一次只让开发者面对一个问题
/// 3. 结果可解释：第一个失败通常就是根因，后面的失败往往是它的连带效应
fn run_gate_chain(gates: &[Gate]) -> Outcome {
    for g in gates {
        println!("  {}", g.status_line());
        match g.outcome {
            Outcome::Pass => continue,
            // 失败或不明，都停在这里
            other => return other,
        }
    }
    Outcome::Pass
}

// ============================================
// 第二部分：基础操作实践
// ============================================

fn main() {
    let arg = env::args().nth(1).unwrap_or_default();

    match arg.as_str() {
        // 只算缓存键：便于"改一个字节再跑一次"的对照实验
        "--cache-key" => {
            let (key, source) = lockfile_cache_key();
            println!("缓存键 = {key:016x}（来源：{source}）");
        }
        // 只跑门禁：退出码就是结论，可被 CI/脚本直接使用
        "--gate" => {
            let gates = demo_gates();
            let result = run_gate_chain(&gates);
            println!("结论：{result:?}，退出码 {}", result.exit_code());
            std::process::exit(result.exit_code());
        }
        _ => {
            section_concepts();
            section_cache_key();
            section_gates();
            section_empty_output();
        }
    }
}

/// FNV-1a 64 位哈希：把任意字节序列压成一个整数
///
/// 【为什么手写而不引入 sha2】
/// 本文件的交付目标是让读者**看见**"缓存键 = 内容指纹"这件事。
/// 为此引入一个需要联网下载的 crate 会喧宾夺主，而算法本身只有三行。
///
/// 【参数（FNV-1a 的两个标准常量）】
///   offset basis = 0xcbf29ce484222325
///   prime        = 0x100000001b3
/// 每一步：先异或一个字节，再乘质数。顺序不能反（反了是 FNV-1，分布更差）。
///
/// 【为什么用 `wrapping_mul`】
/// 这里是**故意溢出**的：乘法的低位是可预测的，高位丢掉正是算法设计的一部分。
/// 用普通 `*` 在 debug 下会 panic（第 42 天讲过的 `overflow-checks`），
/// 那样同一个程序在 debug/release 下行为不同 —— 哈希函数绝不能这样。
fn fnv1a64(bytes: &[u8]) -> u64 {
    let mut hash: u64 = 0xcbf2_9ce4_8422_2325;
    for b in bytes {
        hash ^= *b as u64;
        hash = hash.wrapping_mul(0x0000_0100_0000_01b3);
    }
    hash
}

/// 读一份 Cargo.lock 的内容，返回 (内容, 来源说明)
///
/// 【为什么要试多个路径】
/// 这个文件既可能从 `rust_basics/` 下运行，也可能从项目根运行，
/// 还可能从 `realtime-chat/` 下运行。写死一个路径会让"改了字节键却不变"
/// 这种实验结果取决于你在哪个目录敲的命令 —— 那是实验设计的问题，不是结论。
///
/// 【为什么读不到也不失败】
/// 本文件要证明的是"内容变 → 键变"，与读的是真文件还是内置样例无关。
/// 但**必须把来源打出来**：不标注来源的"实测"，等于没有证据。
fn lockfile_content() -> (String, String) {
    const CANDIDATES: [&str; 4] = [
        "Cargo.lock",
        "../Cargo.lock",
        "../realtime-chat/Cargo.lock",
        "../../realtime-chat/Cargo.lock",
    ];
    for path in CANDIDATES {
        if Path::new(path).exists() {
            if let Ok(content) = fs::read_to_string(path) {
                let note = format!("{path}（{} 字节）", content.len());
                return (content, note);
            }
        }
    }
    // 内置样例：格式与真实 Cargo.lock 一致，只是内容短
    let sample = "version = 4\n\n[[package]]\nname = \"realtime-chat\"\nversion = \"0.1.0\"\n";
    (sample.to_string(), "内置样例（未找到 Cargo.lock）".to_string())
}

/// 缓存键 = hash(锁文件内容)
fn lockfile_cache_key() -> (u64, String) {
    let (content, source) = lockfile_content();
    (fnv1a64(content.as_bytes()), source)
}

/// 第二部分 2.1：缓存键是怎么算出来的
fn section_cache_key() {
    println!("\n============================================================");
    println!("2.1 缓存键：hash(Cargo.lock)");
    println!("============================================================");

    let (content, source) = lockfile_content();
    let key = fnv1a64(content.as_bytes());
    println!("  锁文件来源 = {source}");
    println!("  缓存键     = {key:016x}");

    // 对照实验：在原内容末尾加一个空格，键必须变
    let mut mutated = content.clone();
    mutated.push(' ');
    let key2 = fnv1a64(mutated.as_bytes());
    println!("\n  演示“改一个字节，键就变”：");
    println!("    原内容         {key:016x}");
    println!("    末尾加一个空格 {key2:016x}");
    println!(
        "    是否变化：{}（必须为 true，否则缓存会把新依赖当成旧的）",
        key != key2
    );

    println!("\n  为什么用 Cargo.lock 而不是 Cargo.toml：");
    println!("    Cargo.toml 写的是版本范围 `actix-web = \"4\"`，同一份 toml 在不同时间");
    println!("    可能解析出不同的实际版本；Cargo.lock 记录的是**锁定后的精确版本**。");
}

/// 第二部分 2.2：门禁的两种跑法
fn section_gates() {
    println!("\n============================================================");
    println!("2.2 门禁：全通过 vs 中途失败");
    println!("============================================================");

    println!("\n【场景 A】全部通过");
    let all_pass = demo_gates();
    let r = run_gate_chain(&all_pass);
    println!("  结论：{r:?}");

    println!("\n【场景 B】第 2 道闸门失败（短路：后面的不再执行）");
    let mut fail_at_2 = demo_gates();
    fail_at_2[1].outcome = Outcome::Fail;
    fail_at_2[1].output_lines = 12;
    let r = run_gate_chain(&fail_at_2);
    println!("  结论：{r:?}（第 3–5 道没有被执行，这就是短路）");

    println!("\n【场景 C】结果不明（Unknown）也算失败");
    let mut unknown = demo_gates();
    unknown[2].outcome = Outcome::Unknown;
    unknown[2].output_lines = 0;
    let r = run_gate_chain(&unknown);
    println!("  结论：{r:?}，退出码 {}", r.exit_code());
    println!("  → “没证明通过”就是“没通过”");
}

/// 第二部分 2.3：零输出的通过要怀疑（但先分清"本来该沉默"与"疑似没执行"）
fn section_empty_output() {
    println!("\n============================================================");
    println!("2.3 “通过但零输出”要被标记出来");
    println!("============================================================");

    // 情形一：本来就该沉默的命令 —— 不该报警
    let fmt_clean = Gate::new("cargo fmt --check", "格式不一致", Outcome::Pass, 0).silent_ok();
    println!("  ① 干净时的 fmt（本来就该沉默）");
    println!("     {}", fmt_clean.status_line());

    // 情形二：本该有输出的命令零输出 —— 报警
    let quiet = Gate::new("cargo test", "行为回归", Outcome::Pass, 0);
    println!("\n  ② cargo test 零输出（可疑）");
    println!("     {}", quiet.status_line());

    // 情形三：正常情况，作为对照
    let normal = Gate::new("cargo test", "行为回归", Outcome::Pass, 18);
    println!("\n  ③ 正常情况");
    println!("     {}", normal.status_line());

    println!("\n  【结论】“零输出=可疑”这条规则本身不成立 ——");
    println!("  它是**依赖具体命令**的启发式，必须先知道“这条命令成功时该不该说话”。");
    println!("  一刀切的规则只会制造误报，而误报多了，真报警就没人看了。");

    println!("\n  现实中的三种“零输出的通过”（第 42 天实测过第一种）：");
    println!("    1. 构建产物是 stub —— 编译 0.4 秒，容器退出码 0、日志空白");
    println!("    2. 测试 0 个用例 —— 测试文件没被收集，但 cargo 仍返回 0");
    println!("    3. 脚本吞了退出码 —— `cmd; echo ok` 结果是 0");
}

/// 第一部分的总览打印
fn section_concepts() {
    println!("============================================================");
    println!("30_ci_cd —— 持续集成与质量门禁");
    println!("============================================================");

    println!("\n  CI 的四要素：");
    println!("    固定工具链 / 缓存 / 检查链 / 退出码");
    println!("\n  五道闸门（从快到慢）：");
    println!("    cargo fmt --check                 格式");
    println!("    cargo clippy -- -D warnings       惯用法");
    println!("    cargo check                       类型");
    println!("    cargo test                        行为");
    println!("    cargo build --release             只有 release 才暴露的问题");
    println!("\n  本文件的其余部分会逐个演示：缓存键、短路、零输出的可疑通过。");
    println!("  想直接看门禁结论：./30_ci_cd.exe --gate");
}

/// 演示用的一组闸门（真实的输出行数只是一个代表性数字）
fn demo_gates() -> Vec<Gate> {
    vec![
        // fmt 干净时本来就一句话不说，所以显式声明 silent_ok
        Gate::new("cargo fmt --check", "格式不一致", Outcome::Pass, 0).silent_ok(),
        Gate::new(
            "cargo clippy --all-targets -- -D warnings",
            "惯用法错误",
            Outcome::Pass,
            3,
        ),
        Gate::new("cargo check --all-targets", "类型错误", Outcome::Pass, 2),
        Gate::new("cargo test", "行为错误", Outcome::Pass, 18),
        Gate::new(
            "cargo build --release",
            "release 才暴露的问题",
            Outcome::Pass,
            6,
        ),
    ]
}

// ============================================
// 第三部分：深入理解（选学）
// ============================================

/*
🔍 深入理解：缓存、矩阵与"门禁的成本"

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【1. 缓存是用"可能过期"换"速度"】
缓存命中意味着"直接复用上次的结果"。前提是**输入完全相同**：
源码、依赖、工具链、编译参数、环境变量。少算任何一个，就可能命中过期缓存。
常见的四个"忘了算进键"的输入：
  - 工具链版本（只按 Cargo.lock 算键 → 升级 rustc 后还命中旧缓存）
  - 编译参数（`RUSTFLAGS` 变了但键没变）
  - feature 组合（`--features foo` 与不带 feature 用了同一份缓存）
  - 被 `include_str!` 编进去的静态资源（第 42 天项目里的 `static/chat.html`）

最后一条特别隐蔽：改了一个 HTML 文件，但缓存键只算 `Cargo.lock`，
于是 cargo 认为"没变" —— 这是第 42 天 mtime 陷阱的近亲。

【2. 为什么 0.4 秒比 300 秒更值得看】
人对"慢"敏感，对"快"不敏感。可"不该这么快"恰恰是更强的信号：
- 300 秒 → 正常，依赖很多
- 0.4 秒 → 不可能，编一个 actix-web 应用至少几秒

在 CI 里，如果一个"应该几十秒"的步骤只用了 0.2 秒，
先去看它是不是命中了一份不该命中的缓存、或者目录根本不对。

【3. 矩阵构建的成本是乘法的】
`{stable, beta} × {ubuntu, windows} × {default, all-features}` = 8 个任务，
每个都要编译一遍。矩阵的价值是覆盖平台差异，成本是成倍的时间与机器。
实践上通常：**主矩阵只放"最常见的一种"，其余放到每日/每周定时任务**。
（本项目没有实际配置矩阵——没有多人协作，也没有多平台发布需求，
这正是 YAGNI：不要因为"CI 一般都这么配"就加。）

【4. clippy 的结论为什么会随版本变】
clippy 的规则库随工具链演进：新规则加入、误报修正、API 废弃。
所以"CI 挂了但代码没改"的经典原因是**工具链被自动升级**。
解法不是 `clippy --allow-all`，而是钉住版本 + 定期集中升级。
第 42 天那条 `fetch_update` 废弃警告就是这个现象的实例：
同一条代码在 1.92 干净，在 1.99.1 报警，而它建议的替代 API 在 1.92 上还不稳定。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
4.1 用 `;` 连接命令，退出码被吃掉

❌ 错误写法：
```bash
docker build -t app . ; echo "构建完成"
```
现象：构建失败了，但你看到"构建完成"，以为成功了。
原因：Shell 的退出码是**最后一条命令**的退出码，`echo` 永远返回 0。

实测（第 42 天）：真实日志里是 `#12 ERROR ... exit code: 101`，
而我看到的退出码是 0。

✅ 正确写法：
```bash
set -e
docker build -t app .
echo "构建完成"        # 只有前面成功才会走到这里
```

记忆技巧：**`;` 是"接着做"，`&&` 是"成功才做"。CI 要的是后者。**

────────────────────────────────────

4.2 不加 `-D warnings`，clippy 检查等于没做

❌ 错误写法：
```yaml
- run: cargo clippy --all-targets
```
现象：满屏 `warning:`，但任务标绿。

原因：警告不改变退出码。CI 只看退出码。
✅ 正确写法：
```yaml
- run: cargo clippy --all-targets -- -D warnings
```
记忆技巧：**门禁 = 把警告变成退出码。**

────────────────────────────────────

4.3 把 `cargo fmt` 放进 CI

❌ 错误写法：
```yaml
- run: cargo fmt            # CI 直接改代码
```
现象：CI 通过，但你提交的文件和 CI 检查的文件不是同一个。

✅ 正确写法：
```bash
cargo fmt --check           # 只判断，不改
```
记忆技巧：**CI 不写代码，只做判断。**

────────────────────────────────────

4.4 `cargo test` 显示 `0 passed` 也算通过

❌ 现象（第 43 天实测：空 crate 里把测试文件改名成 `tests/ws.rs.bak`）：

```
running 0 tests
test result: ok. 0 passed; 0 failed; 0 ignored; 0 measured; 0 filtered out
cargo test 退出码=0
```
退出码 0，任务标绿 —— 但一个用例都没跑。

原因：测试文件没被收集（文件名不是 `*.rs`、不在 `tests/` 下、
或整个 crate 的 feature 关掉了测试）。

⚠️ 但断言**不能**写成"每一行都要有通过"：
本项目正常输出里天然有两段 `0 passed`（bin 单测 + doctest），
那样写会把正常情况判成失败。

✅ 正确的防御是**汇总**：

```bash
cargo test 2>&1 | tee test.log
total=$(grep -Eo 'test result: ok\. [0-9]+ passed' test.log \
        | grep -Eo '[0-9]+' | awk '{s += $1} END {print s + 0}')
[ "$total" -ge 10 ] || { echo "通过用例总数 $total < 10"; exit 1; }
```
实测 `total=10`（3 + 0 + 7 + 0）。

记忆技巧：**"0 个"和"全部"在退出码上长得一模一样。**
局限：这条断言证明"**至少**跑了 10 个"，不是"该跑的都跑了"。

────────────────────────────────────

4.5 `cargo test | tee` 也会吃掉退出码

❌ 错误写法：
```bash
cargo test 2>&1 | tee test.log
```
现象：测试失败了，但这一步的退出码是 0 —— 任务标绿。

原因：管道的退出码是**最右侧命令**的，也就是 `tee` 的，它永远成功。

实测（第 43 天）：
```
$ bash -c 'false | tee /dev/null >/dev/null; echo "退出码=$?"'
退出码=0                      ← 失败被吃掉

$ bash -c 'set -o pipefail; false | tee /dev/null >/dev/null; echo "退出码=$?"'
退出码=1                      ← 正确
```

✅ 正确写法：
```bash
set -o pipefail
cargo test 2>&1 | tee test.log
```
记忆技巧：**退出码来自你没想到的那条命令。**
这是"退出码被吃掉"的第二副面孔；第一副是 `cmd; echo $?`（4.1）。

────────────────────────────────────

4.6 用日期/分支名做缓存键

❌ 错误写法：
```yaml
key: cargo-${{ github.ref }}-${{ github.run_id }}
```
现象：缓存从不命中（每次 run_id 都不同），CI 一直慢；
或者换成按日期，则可能**命中过期缓存**导致虚假通过。

✅ 正确写法：
```yaml
key: cargo-${{ hashFiles('Cargo.lock') }}      # 单 crate 仓库够用
```
工作区（存在多个 Cargo.lock）时，改成递归通配：`hashFiles('<两个星号>/Cargo.lock')`。

⚠️ 写本段时的真实踩坑：`<两个星号>/` 这个序列写进 Rust 的块注释里，
会被当成**结束注释**（`星号斜杠` 出现在第 2、3 个字符上），
导致后面整段 YAML 被当成 Rust 代码编译，报 `prefix 'lock' is unknown` +
`unterminated character literal`。所以上面把它拆开写。

记忆技巧：**键要绑定"输入内容"，不是"运行时间"。**

────────────────────────────────────

4.7 让 CI 自动用最新工具链

❌ 错误写法：
```yaml
- uses: dtolnay/rust-toolchain@stable    # 每次都是当时的 latest
```
现象：某个周一早上，代码没改，CI 全红——因为稳定版升级了，新 lint 报警。

实测依据（第 42 天）：同一条代码，本机 1.92.0 零警告，
镜像 1.99.1 报 `fetch_update` 已废弃。

✅ 正确写法：
```toml
# rust-toolchain.toml（进仓库）
[toolchain]
channel = "1.92.0"
```
记忆技巧：**工具链是构建输入，不是环境背景。**

────────────────────────────────────

4.8 只跑 `cargo check` 就说"测试通过"

❌ 错误写法：
```yaml
- run: cargo check --all-targets
```
`check` 只做类型检查，**不运行任何测试**。它能证明"能编过"，
不能证明"行为正确"。

✅ 正确写法：`check` 用于快速反馈，`test` 必须单独跑。
记忆技巧：**类型正确 ≠ 逻辑正确。**
*/

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================

/*
【fnv1a64(bytes: &[u8]) -> u64】
作用：把任意字节序列压成一个 64 位整数（FNV-1a 哈希）。
参数：任意字节切片（这里是锁文件的全部内容）。
返回：64 位无符号整数。同样的输入永远得到同样的输出。
用途：本文件用它模拟"缓存键"。
注意：**它不是密码学哈希**（可被构造碰撞），只适合做"内容指纹"。
      真实的缓存键一般用 SHA-256（GitHub Actions 用 `hashFiles`）。

【fs::read_to_string(path) -> io::Result<String>】
作用：把文件读成字符串。
注意：非 UTF-8 内容会报错；大文件用 `BufReader` 更好。
本文件用它读 `Cargo.lock`，读不到就退回内置样例。

【Path::exists() -> bool】
作用：路径是否存在。
注意：它不区分"不存在"和"没有权限看"，这是常见的误判来源。

【std::process::exit(code: i32) -> !】
作用：以指定退出码立即结束进程。
注意：**不运行 Drop、不自动刷新缓冲**。退出前要 `flush()`。
      CI 脚本里，这是"把结论交给平台"的唯一方式。

【env::args() -> Args】
作用：命令行参数迭代器（第一个是程序名）。
注意：`args()` 在遇到非 UTF-8 参数时会 panic，稳健写法是 `args_os()`。

【哈希相关：为什么不能用 `DefaultHasher` 做缓存键】
`std::collections::hash_map::DefaultHasher` 的算法**不保证跨版本稳定**，
Rust 升级后同一输入的输出可能不同 —— 用它做缓存键会导致缓存永久失效。
这是"看起来能用、实际会坏"的典型。
*/

// ============================================
// 练习题
// ============================================

/*
1. 缓存键的稳定性
   要求：用 `./30_ci_cd.exe --cache-key` 跑两次，确认键相同；
         然后**故意**在 `Cargo.lock` 末尾加一个空格，再跑一次，确认键变了。
   提示：改完记得改回去；两个数字都要贴出来。
   注意：这就是"缓存键要绑定内容"的最小证明。

2. 制造一次短路
   要求：修改 `demo_gates()`，让第 1 道闸门失败，观察第 2–5 道是否执行。
   提示：数一下打印了几行"状态行"。
   注意：为什么"只打印 1 行"反而是对的？

3. 零用例的陷阱
   要求：在任一 cargo 项目里，把 `tests/` 下的文件改名成 `ws.rs.bak`，
         跑 `cargo test`，记录输出与退出码。
   提示：注意 `running 0 tests` 和退出码 0 同时出现。
   注意：这就是"0 个用例也算通过"的实证。

4. 写一条数量断言
   要求：给上面第 3 题的场景写一段 Shell，让"0 个用例"变成失败；
         再拿到 `realtime-chat` 里跑，确认正常情况**不会**误报。
   提示：用**汇总**而不是"每行都要有"——
         `grep -Eo 'test result: ok\. [0-9]+ passed' | grep -Eo '[0-9]+' | awk '{s+=$1} END{print s+0}'`。
         本项目正常输出是 `3 + 0 + 7 + 0 = 10`，其中两个 0 是合法的。
   注意：把两次输出都贴出来，并说明这条断言**证明不了**什么。

5. 让 clippy 真的拦下一次
   要求：在 `realtime-chat` 里加一行 `let s = String::from("x").to_string();`，
         跑 `cargo clippy --all-targets -- -D warnings`，记录退出码；
         再删掉，确认恢复 0。
   提示：clippy 的提示里会给出具体建议。
   注意：这一步实测过 `useless_conversion`（第 43 天），可以对照。

6. 工具链版本对照
   要求：用 `rustc --version` 记录本机版本；用
         `docker run --rm rust:1-slim-bookworm rustc --version` 记录镜像版本。
         想想：如果 CI 用镜像里的版本，本地零警告能不能推出 CI 也零警告？
   提示：第 42 天已经有了答案（1.92 vs 1.99.1）。
   注意：写出你的判断依据，不要只写结论。

7. 设计本项目的 CI（开放题）
   要求：为 `realtime-chat` 写一份 CI 清单：跑哪几条命令、按什么顺序、
         缓存键用什么、要不要矩阵、要不要跑 Docker 构建。
   提示：每加一步，都要能回答"它防的是哪个具体的回归"。
   注意：想清楚"跑 Docker 构建"这一步的**成本**（279 秒）值不值。

8. 亲手制造一次"退出码被吞"
   要求：在 Git Bash 里分别跑
           `bash -c 'false | tee /dev/null >/dev/null; echo $?'`
           `bash -c 'set -o pipefail; false | tee /dev/null >/dev/null; echo $?'`
         记录两次的退出码；再解释为什么 `cmd; echo $?` 是同一个坑的另一副面孔。
   提示：管道（`|`）取最右命令的退出码；分号（`;`）取最后一条命令的退出码。
   注意：两次的结论都要写清楚，别只贴"正确的那次"。
*/
