// ============================================
// 部署与运维基础 - 超详细注释版
// 日期：2026-11-06（第42天）
// 难度：⭐⭐ 中级
// ============================================

/*
【核心概念】简要说明

是什么？
- 部署 = 把"在你机器上能跑的程序"变成"在别人机器上、没人看着也能跑的程序"。
  它不只是"上传一个文件"，而是三件互相牵连的事：

    【产物】  编出来的到底是什么？多大？依赖什么？
    【配置】  地址、端口、超时从哪里来？换环境要不要重新编译？
    【进程】  它以前台还是后台跑？怎么知道它活着？怎么让它体面地退出？

为什么需要？
- 一个服务 99% 的时间不是在写代码，而是在**运行**。
  运行期出的问题（绑定在 127.0.0.1、容器里找不到配置文件、
  重启时正在处理的请求被硬切断）都不属于"业务逻辑"，但都让服务不可用。
- 这些事在 `cargo run` 阶段全都被掩盖了：本机有配置文件、本机端口没人抢、
  本机 Ctrl-C 就知道结束了。部署把掩盖全部撕开。

核心特点？
- **产物是可测量的**：体积、构建耗时、依赖的动态库列表，都能用命令量出来。
- **配置必须外置**：同一份二进制要能跑在测试、预发、生产三个环境。
- **进程要能被外部观察**：健康检查靠退出码，日志靠标准输出，停机靠信号。

Rust 特色？
- 默认**动态链接 glibc**：产物小，但要求目标机 glibc 版本够新。
  换 `x86_64-unknown-linux-musl` 目标可以编出几乎全静态的二进制，
  代价是 musl 的分配器/解析器性能与 glibc 有差异。
- release 与 debug 的差别不只是"快慢"：`debug_assertions` 会改变
  **程序行为**（整数溢出从"panic"变成"静默回绕"），所以**不能拿 debug 产物上线**。
- 没有运行时、没有 GC、不依赖外部 DLL（MSVC 目标除外），
  所以"一个文件就是一个服务"这件事在 Rust 里格外成立。

【本文件怎么跑】

本文件用 std 手写了一个"可部署服务"的骨架：配置校验、健康检查、
日志分流、优雅停机。它不依赖任何第三方 crate，`rustc` 直接编译。

```bash
# 1) debug 构建（默认）
rustc --edition 2021 29_deployment.rs -o 29_deployment.exe

# 2) release 构建（优化 + 无调试断言 + 去符号）
rustc --edition 2021 -O -C strip=symbols 29_deployment.rs -o 29_deployment_rel.exe

# 3) 看体积差别
ls -l 29_deployment.exe 29_deployment_rel.exe

# 4) 正常运行（模拟一次完整生命周期）
./29_deployment.exe

# 5) 健康检查模式：退出码 0 = 健康，1 = 不健康
./29_deployment.exe --health ; echo "退出码=$?"

# 6) 故意把配置写错，看它怎么拒绝启动
PORT=99999 ./29_deployment.exe
```

⚠️ 本文件是**教学材料**，不是可以直接上线的服务：
它用"打印五步"来模拟优雅停机，真实的信号处理需要 `tokio::signal` 或 `ctrlc` 等 crate。
这一点后面"常见错误"里会专门讲。

【实测结果】（2026-11-06，rustc 1.92.0，Windows x86_64-msvc）

| 构建方式 | exe 体积 | .pdb 体积 | 构建耗时 |
|---|---|---|---|
| 默认（debug） | 247,296 B（241.5 KB） | 1,454,080 B | 0.216 s |
| `-O` | 210,944 B（206.0 KB） | 1,421,312 B | 0.326 s |
| `-O -C strip=symbols` | **210,944 B（与上一行完全相同）** | 1,421,312 B | 0.328 s |
| `-O -C strip=symbols -C panic=abort` | 203,264 B（198.5 KB） | 1,413,120 B | 0.293 s |
| `-O -C strip=symbols -C opt-level=z` | 206,336 B（201.5 KB） | 1,421,312 B | 0.295 s |

两个**反直觉**的实测结论（比表格本身更重要）：

1. **在 Windows/MSVC 上 `strip` 一点体积都没省**（210,944 → 210,944，一个字节没变）。
   MSVC 目标把调试信息放在**外部 `.pdb`** 里，PE 文件里本来就没有符号表可删。
   所以网上"`strip` 能让二进制小一半"的说法是 **ELF（Linux/macOS）世界**的经验，
   在 MSVC 上不成立。（ELF 上的效果本次未实测，不做断言。）
2. **debug 与 release 只差 15%**，没有"一半"那么夸张——原因同上：
   大头在 `.pdb`（1.4 MB），而 debug 与 release 的 `.pdb` 只差 33 KB。
   真正夸张的是 Linux 上把调试信息编进二进制的场景，那是另一回事。

这也说明一件方法论上的事：**别人给的百分比，换一个目标平台就可能完全失效。**
数字必须自己量。
*/

use std::env;
use std::io::{self, Write};
use std::time::{Duration, Instant};

// ============================================
// 第一部分：核心概念详解 ⭐⭐⭐⭐⭐
// ============================================

/*
1.1 debug 与 release 到底差在哪

【是什么】
`rustc` 的优化等级不是"开/关"，是一组互相牵连的开关。常用对照：

| 开关 | debug | release |
|---|---|---|
| `opt-level` | 0 | 3 |
| `debug-assertions` | on | **off** |
| `overflow-checks` | on | **off** |
| `lto` | off | off（`cargo` 默认 `false`，要显式开） |
| `codegen-units` | 256 | 16（`cargo` 默认） |
| `strip` | none | none（**要显式 `-C strip=symbols`**） |

【为什么需要知道】
因为最容易忽略的一行是 `debug-assertions` 和 `overflow-checks`：
它们改的不是"快慢"，是**行为**。

    let x: u8 = 255;
    let y = x + 1;      // debug: panic（'attempt to add with overflow'）
                        // release: 静默变成 0

也就是说，一段"本机测试通过"的代码在 release 下可能算出完全不同的结果。
上线前必须用**将要上线的那个 profile** 跑一遍测试：

    cargo test --release

【Rust 特色】
`-C strip=symbols` 是 rustc 内建的，不需要额外的 `strip` 命令。
**但它在不同目标上的效果完全不同**：ELF（Linux/macOS）上能砍掉一大块；
Windows/MSVC 上几乎为零——因为调试信息根本不在 exe 里，而在旁边的 `.pdb`。
本文件头的实测表格就是证据：加了 `strip` 前后 exe 都是 210,944 字节。
代价（真的生效时）是崩溃时堆栈只有地址没有函数名。

────────────────────────────────────

1.2 产物体积是从哪来的

一个"Hello World"的 release 二进制通常还有 200~300 KB 左右（本文件实测 206 KB），
主要来自：

- **std 的部分**：panic 处理、格式化、内存分配器、回溯支持
- **符号表**：函数名、类型名、行号信息
- **没有用到的代码**：链接器默认会砍掉未引用的函数（`--gc-sections`），
  但 panic/backtrace 相关几乎一定会被保留

想再小，三个杠杆：
1. `-C strip=symbols`（去符号——**只在 ELF 目标上明显**，MSVC 上实测为 0 字节收益）
2. `-C panic=abort`（panic 不展开，省掉展开机制；代价是不能 `catch_unwind`）
3. `opt-level="z"`（优化目标从"快"换成"小"）

本文件头的实测表格把这三个杠杆都量了一遍，包括两个"没效果"的结果。


────────────────────────────────────

1.3 静态链接、动态链接，和"换个机器就跑不起来"

Rust 在 Linux 上默认链接 **glibc**（动态库）。于是经典事故：

    开发机：Ubuntu 24.04（glibc 2.39）
    生产机：CentOS 7（glibc 2.17）
    启动报错：`version 'GLIBC_2.34' not found`

原因：二进制里记录的符号版本，目标机的 glibc 提供不了。
三条出路：

1. **在更老的系统上编译**（"构建环境要老于运行环境"）
2. **用 musl 目标**：`--target x86_64-unknown-linux-musl`，
   静态链接 libc，产物几乎不依赖系统库
3. **容器里编、容器里跑**：把 glibc 一起打包进镜像

Windows 上同理：MSVC 目标依赖 `vcruntime140.dll`，运行时缺失就是
"找不到 VCRUNTIME140.dll"；GNU 目标（`x86_64-pc-windows-gnu`）随 GCC 带一份。

────────────────────────────────────

1.4 配置外置：12-Factor 的第 3 条

「配置存于环境中」（Store config in the environment）。

【为什么不能写死在代码里】
写死一个地址，就意味着"换环境要重新编译"，而重新编译意味着
"你上的不是测试过的那份二进制"。数据和代码分离，是部署可复现的前提。

【三种来源的取舍】

| 来源 | 适合放什么 | 不适合放什么 |
|---|---|---|
| 环境变量 | 部署相关的量：端口、数据库地址、超时、开关 | 长文本、结构化嵌套、需要版本管理的内容 |
| 配置文件 | 复杂的结构化配置（规则表、路由表） | 环境相关的量、密码 |
| 命令行参数 | 一次性的运维动作（`--health`、`--migrate`） | 常驻配置（重启后容易忘） |

【关键原则】
- **有默认值**：缺省也能起，才谈得上"一次编译，到处运行"
- **要校验**：端口 99999 不是"配置错了"就是"代码错了"，
  必须在启动时就报错退出，而不是等到第一个请求进来
- **错误信息要指出是哪个变量**：`PORT=abc` → 必须告诉运维是 `PORT` 有问题

────────────────────────────────────

1.5 日志是事件流，不是文件

12-Factor 第 11 条：把日志当作事件流，写到 **stdout/stderr**。

【为什么】
- 容器是短暂的：往容器内的文件写日志，容器一重建就没了
- 日志轮转是平台的事（journald / docker logs / k8s），不该由应用自己管
- 应用自己写文件，还会带来"文件锁、权限、磁盘满"这一堆额外问题

【stdout 和 stderr 的分工】
- 正常业务输出 → `stdout`
- 错误、诊断、警告 → `stderr`

为什么要分开：运维常常把两者分流（一个进日志平台，一个直接告警），
混在一起就没法区分"信息"和"问题"。

────────────────────────────────────

1.6 进程模型：前台、退出码、优雅停机

【前台运行】
容器要求进程在**前台**（PID 1）跑，后台 `&` 会被立刻判定为"退出了"。

【退出码】
退出码是进程对外的第一语言：
- `0` = 成功
- 非 `0` = 失败（具体数字由你定义）

健康检查（k8s 的 liveness probe、docker 的 `HEALTHCHECK`）本质上就是
"执行一条命令，看退出码"。所以 `--health` 子命令必须**真的检查**，
不能无脑 `return 0`——那就等于没有检查。

【优雅停机】
收到 SIGTERM 后应该做的四件事，顺序不能乱：

    1. 停止接受新连接（不再 listen 的 accept）
    2. 等手上正在处理的请求做完（有上限，不能无限等）
    3. 关闭资源（数据库连接池、文件句柄）
    4. 退出码 0

顺序反了（先关资源再等请求）会把正在处理的请求写坏。
这是第 40 天 realtime-chat 里"收尾顺序：先减在线数、再广播 leave"
的同一个道理，只是尺度从"一个连接"放大到"整个进程"。
*/

/// 服务的配置
///
/// 【为什么是一个结构体，而不是到处 `env::var`】
/// 1. **只解析一次**：解析在最外层做，业务代码拿到的已经是干净的类型
/// 2. **可以校验**：非法值在启动时就暴露，而不是在第一个请求里 panic
/// 3. **可测**：`from_lookup` 接受一个查询函数，测试可以注入假的"环境变量"
/// 4. **可打印**：启动时把配置打一遍（脱敏后），运维一眼能看到跑的是什么
#[derive(Debug, Clone)]
struct Config {
    /// 监听地址。注意：容器里必须是 0.0.0.0，不能是 127.0.0.1
    bind: String,
    /// 端口
    port: u16,
    /// 心跳/超时之类的运行参数
    request_timeout: Duration,
    /// 日志级别（本文件只用来演示"配置驱动开关"）
    log_level: String,
}

impl Config {
    /// 从任意"查询函数"读配置
    ///
    /// 参数 `get` 是个闭包：`name -> Option<String>`。
    /// 用闭包而不是直接 `env::var`，是为了**可测试**——
    /// 测试里可以传 `|n| map.get(n).cloned()`，不碰真实环境变量。
    /// （真实环境变量是全局状态，并行测试时互相污染，这是第 41 天的老坑）
    fn from_lookup(get: impl Fn(&str) -> Option<String>) -> Result<Self, String> {
        let bind = get("BIND_ADDR").unwrap_or_else(|| "127.0.0.1".to_string());

        // 端口：缺省 8090；给了但解析不了，必须报错，不能静默退回默认值
        let port = match get("PORT") {
            None => 8090u16,
            Some(raw) => raw.parse::<u16>().map_err(|_| {
                format!("PORT={raw:?} 不是合法端口（应为 0~65535 的整数）")
            })?,
        };

        let request_timeout = match get("REQUEST_TIMEOUT_MS") {
            None => Duration::from_secs(10),
            Some(raw) => {
                let ms = raw.parse::<u64>().map_err(|_| {
                    format!("REQUEST_TIMEOUT_MS={raw:?} 不是合法整数（单位：毫秒）")
                })?;
                if ms == 0 {
                    return Err(
                        "REQUEST_TIMEOUT_MS 不能为 0：那等于「立刻超时」，不是「无限等」"
                            .to_string(),
                    );
                }
                Duration::from_millis(ms)
            }
        };

        let log_level = get("LOG_LEVEL").unwrap_or_else(|| "info".to_string());

        Ok(Self {
            bind,
            port,
            request_timeout,
            log_level,
        })
    }

    /// 真实环境变量版本
    fn from_env() -> Result<Self, String> {
        Self::from_lookup(|name| env::var(name).ok())
    }

    /// 打印配置（部署后第一眼要看的东西）
    ///
    /// 【为什么值得专门写一个方法】
    /// "服务起不来"最常见的两个原因：配置没传对、传对了但没生效。
    /// 启动日志里有一份完整配置，这两种都能立刻排除。
    /// 注意：**密码、token 之类绝不能被打印**，这里只有非敏感项。
    fn print_summary(&self) {
        println!("配置：");
        println!("  BIND_ADDR          = {}", self.bind);
        println!("  PORT               = {}", self.port);
        println!("  REQUEST_TIMEOUT_MS = {}", self.request_timeout.as_millis());
        println!("  LOG_LEVEL          = {}", self.log_level);
    }
}

// ============================================
// 第二部分：基础操作实践
// ============================================

fn main() {
    let args: Vec<String> = env::args().skip(1).collect();

    match args.first().map(String::as_str) {
        // 运维动作走子命令：一次性的、可脚本化的
        Some("--health") => {
            let ok = health_check();
            std::process::exit(if ok { 0 } else { 1 });
        }
        Some("--version") => {
            println!("deploy-demo {}", build_profile());
        }
        _ => {
            if let Err(e) = run_service() {
                // 启动失败：错误进 stderr，退出码非 0
                // 这是"进程对外的第一语言"——编排系统只看退出码
                eprintln!("启动失败：{e}");
                std::process::exit(1);
            }
        }
    }
}

/// 编译期就能知道自己在哪个 profile
///
/// `cfg!(debug_assertions)` 在 release 下是 `false`。
/// 这也顺带证明了：**profile 不同，程序行为就不同**，
/// 所以"debug 能过"不能推出"release 能过"。
fn build_profile() -> &'static str {
    if cfg!(debug_assertions) {
        "debug（带调试断言与溢出检查）"
    } else {
        "release（无调试断言，溢出静默回绕）"
    }
}

/// 打印当前可执行文件的信息
///
/// 【为什么要从程序内部读自己的体积】
/// 因为这是**唯一不会说谎**的数字：
/// 你 `ls` 看到的可能是旧的产物，而程序读的是"正在运行的这一份"。
fn print_self_info() {
    match env::current_exe() {
        Ok(path) => {
            let size = std::fs::metadata(&path).map(|m| m.len()).unwrap_or(0);
            println!("  可执行文件     = {}", path.display());
            println!(
                "  体积           = {} 字节（{:.1} KB）",
                size,
                size as f64 / 1024.0
            );
            // 顺带看一眼"依赖了哪些动态库"，这是"换个机器跑不起来"的根因所在
            println!("  运行平台       = {}-{}", env::consts::OS, env::consts::ARCH);
        }
        Err(e) => println!("  读不到自身路径：{e}"),
    }
}

/// 健康检查
///
/// 【真实项目里应该检查什么】
/// 不是"文件在不在"，而是"我依赖的服务还能不能用"：
/// 数据库连接池是否有可用连接、必需的配置是否加载、磁盘是否可写。
///
/// 这里的教学版只做一个能被演示的检查：
/// 配置能否解析、端口是否被占（端口占不了说明有另一个实例在跑，
/// 对"我是不是那个该干活的实例"这个问题来说就是坏消息）。
fn health_check() -> bool {
    let cfg = match Config::from_env() {
        Ok(c) => c,
        Err(e) => {
            eprintln!("[health] 配置非法：{e}");
            return false;
        }
    };

    let addr = format!("{}:{}", cfg.bind, cfg.port);
    match std::net::TcpListener::bind(&addr) {
        Ok(_) => {
            // 能绑上说明端口空着；教学版把它当作"健康"
            println!("[health] ok（{addr} 可用）");
            true
        }
        Err(e) => {
            eprintln!("[health] 端口 {addr} 不可用：{e}");
            false
        }
    }
}

/// 服务的完整生命周期：读配置 → 启动 → 运行 → 优雅停机
fn run_service() -> Result<(), String> {
    let started = Instant::now();

    println!("== 1. 编译期信息 ==");
    println!("  profile        = {}", build_profile());
    print_self_info();

    println!("\n== 2. 读取配置 ==");
    let cfg = Config::from_env()?; // 配置不合法在这里就返回 Err，不会带着坏配置跑起来
    cfg.print_summary();

    println!("\n== 3. 启动 ==");
    // ⚠️ 真实服务在这里 `TcpListener::bind` 并开始 accept。
    // 本文件只验证端口可用性，不做真正的服务，避免教学材料变成"半个服务器"。
    let addr = format!("{}:{}", cfg.bind, cfg.port);
    std::net::TcpListener::bind(&addr).map_err(|e| format!("绑定 {addr} 失败：{e}"))?;
    println!("  已绑定 {addr}（演示用，不真正 accept）");
    if cfg.bind == "127.0.0.1" {
        // 这是部署中最常见的错误，值得主动告警
        println!("  ⚠️ 绑定在 127.0.0.1：容器外/宿主机将无法访问，生产请用 0.0.0.0");
    }

    println!("\n== 4. 运行中（日志走 stdout，错误走 stderr）==");
    println!("[INFO] 服务启动完成，耗时 {:?}", started.elapsed());
    // stderr 的演示：这条故意走 stderr，方便对比两条流
    eprintln!("[WARN] 这是一条 stderr 输出（运维通常会把它单独接走）");

    // 演示"处理请求"的耗时受配置控制
    let fake_work = cfg.request_timeout.min(Duration::from_millis(50));
    std::thread::sleep(fake_work);
    println!("[INFO] 处理一个随请求完成，耗时 {:?}", fake_work);

    println!("\n== 5. 收到 SIGTERM，开始优雅停机 ==");
    graceful_shutdown();

    println!("[INFO] 退出，总耗时 {:?}", started.elapsed());
    Ok(())
}

/// 优雅停机的五步（本文件用打印模拟）
///
/// 【为什么顺序重要】
/// 先"停止接受新连接"，再"等现有请求做完"，最后才"关资源"。
/// 如果先关数据库连接池，正在处理的请求会在中途拿到一个已关闭的连接，
/// 表现是"重启期间出现一批 500"——这类事故的根因几乎都是停机顺序。
fn graceful_shutdown() {
    let step = |n: u32, msg: &str| println!("  {n}. {msg}");

    step(1, "停止 accept 新连接（监听套接字不再接受）");
    step(2, "等待在途请求结束（有上限，例如 10 秒；超时则强制结束）");
    step(3, "关闭资源：数据库连接池、文件句柄");
    step(4, "刷新日志缓冲（日志是事件流，别丢最后几条）");
    step(5, "退出码 0");

    // 演示 stdout / stderr 分流时的刷新
    // 【为什么要 flush】stdout 接管道时是块缓冲，`process::exit` 不会自动刷，
    // 不显式刷就可能丢掉最后几行日志——这在"重启前想看一眼收尾日志"时最伤
    let _ = io::stdout().flush();
    let _ = io::stderr().flush();
}

// ============================================
// 第三部分：深入理解（选学）
// ============================================

/*
🔍 深入理解：release 构建为什么慢，以及三个"变小"的取舍

如果你想知道"为什么"，可以阅读这部分。
如果觉得难，可以跳过，不影响后续学习。

【1. 为什么 release 编译慢好几倍】
不是因为"多做了事"，而是因为**优化本身要花时间**：
- 内联与循环优化要在更大的作用域上求解
- 单态化：泛型为每个具体类型各生成一份代码，
  最终代码量可以远超源码量（所以 release 的链接阶段也慢）
- LTO（链接时优化）跨 crate 分析，代价是链接时间指数级上升

这也是为什么 `cargo check` 快、`cargo build` 慢、`cargo build --release` 更慢：
三者做的事完全不在一个量级。

【2. `panic=abort` 的取舍】
默认是 `unwind`：panic 时沿栈展开，逐层析构，所以 `catch_unwind`、
线程池里"一个任务 panic 不影响其他任务"都靠它。
改成 `abort`：直接终止进程。好处是省掉展开机制，产物更小、代码更少；
坏处是**一个 panic 会带走整个进程**，且不能 `catch_unwind`。

对"服务"来说 `abort` 常常反而更安全：一个已经 panic 的进程，
状态可能已经不一致，让它快速死掉、由编排系统重启，比带着坏状态继续服务好。
但这是**策略选择**，不是绝对的对错。

【3. musl 的代价】
musl 让二进制几乎全静态，解决了 glibc 版本问题，但不是免费的：
- 默认分配器在分配密集场景下通常比 glibc 的 ptmalloc 慢
  （所以常见做法是配 `jemalloc` / `mimalloc`）
- DNS 解析实现与 glibc 有差异（如不支持 `nsswitch.conf` 的部分能力）
- 某些依赖系统库的 crate（图形、系统调用封装）在 musl 下要额外配置

【4. 交叉编译为什么需要 `--target`】
`rustc` 的产物依赖目标平台的 **std**（预编译好的标准库）。
`--target aarch64-unknown-linux-gnu` 时，rustup 会去下载对应的 std；
没装就报 "can't find crate for `std`"。
另外还需要一个**链接器**（如 `aarch64-linux-gnu-gcc`）——
交叉编译失败十次有八次死在链接器，而不是编译器。

【5. 为什么"一个文件就是服务"在 Rust 里成立】
没有运行时（不需要 JVM 启动、不需要解释器）、没有动态依赖（musl 目标下），
所以 `scp` 一个二进制到服务器上就能跑。
Go 有同样的性质；Java/Node/Python 则必须把运行时一起打包（这也是容器流行的一部分原因）。
*/

// ============================================
// 第四部分：常见错误和陷阱 ⭐⭐⭐⭐⭐
// ============================================

/*
4.1 容器里绑 127.0.0.1

❌ 错误做法：
```rust
// 代码里写死，或者配置默认值就是它
let bind = "127.0.0.1";
```
容器启动后 `docker logs` 显示"已监听"，但宿主机 `curl` 死活连不上。

原因：`127.0.0.1` 是**容器自己的回环**。容器外看不到它。
这和"在本机跑得好好的"并不矛盾——本机浏览器和程序在同一个网络命名空间里。

✅ 正确做法：
```bash
BIND_ADDR=0.0.0.0 docker run ...
```
本文件在绑定 127.0.0.1 时会主动打一条 ⚠️ 警告，就是为了让人在日志里看见。

记忆技巧：**回环地址是"自己对自己说话"，别人的机器永远听不到。**

────────────────────────────────────

4.2 拿 debug 产物上线

❌ 错误做法：
```bash
cargo build          # 忘了 --release
scp target/debug/app server:/opt/app
```
症状：CPU 高、内存大、偶尔出现"release 下才有的错误"（因为 `debug_assertions` 行为不同）。

原因：debug 不优化，且 `overflow-checks` 是开的——
上线后同一段代码的溢出行为会变化，测试结论直接失效。

✅ 正确做法：
```bash
cargo build --release
cargo test  --release      # 用要上线的 profile 跑测试
```
记忆技巧：**测试和上线必须是同一份构建配置**，否则通过率不代表任何东西。

────────────────────────────────────

4.3 把配置写死

❌ 错误做法：
```rust
const DB_URL: &str = "postgres://10.0.0.5:5432/prod";
```
换环境要改代码、重新编译，最终"线上的二进制"和"测过的二进制"不是同一个。

✅ 正确做法：见本文件的 `Config::from_lookup`——有默认值、有校验、有明确报错。
记忆技巧：**配置能从环境来，就不要进代码。**

────────────────────────────────────

4.4 配置非法但静默退回默认值

❌ 错误做法：
```rust
let port = env::var("PORT").ok().and_then(|s| s.parse().ok()).unwrap_or(8080);
```
运维写了 `PORT=80800`（多打一个 0，超出 u16），服务安安静静地在 8080 起，
流量全打到别处。

原因：`ok()` 和 `and_then(...ok())` 把"没设置"和"设错了"合并成了同一种情况。

✅ 正确做法：
```rust
let port = match env::var("PORT") {
    None => 8080,                                   // 没设置 → 用默认值
    Some(raw) => raw.parse::<u16>()
        .map_err(|_| format!("PORT={raw:?} 不是合法端口"))?,   // 设错了 → 报错退出
};
```
记忆技巧：**"没给"和"给错了"是两件事，不能合并处理。**

────────────────────────────────────

4.5 健康检查无脑返回 0

❌ 错误做法：
```rust
fn health() -> bool { true }
```
编排系统看到"永远健康"，于是永远不重启那个已经连不上数据库的实例。

✅ 正确做法：检查真实依赖（见 `health_check` 的注释），
且**不健康时返回非 0 退出码**。
记忆技巧：**健康检查的价值在于它敢说"不"。**

────────────────────────────────────

4.6 自己往文件里写日志

❌ 错误做法：
```rust
let mut f = File::create("/var/log/app.log")?;   // 容器里容器重建即丢
```
症状：容器重启后日志全无；磁盘被写满后服务挂掉。

✅ 正确做法：写到 stdout/stderr，交给平台收集（见知识点 1.5）。
记忆技巧：**容器是一次性的，容器里的文件也是一次性的。**

────────────────────────────────────

4.7 收到 SIGTERM 立刻退 / 干脆不处理

❌ 错误做法：
```rust
loop { handle_one_request(); }   // 从不检查停止信号
```
症状：每次发布都有一批请求 502/连接被重置；用户恰好在下单时被切断。

✅ 正确做法：收到 SIGTERM → 停止 accept → 等在做完 → 关资源 → 退出（见 `graceful_shutdown`）。
Rust 标准库**不提供**信号处理，需要 crate（`tokio::signal`、`ctrlc`）——
这是本文件没做真实信号处理的诚实说明。
记忆技巧：**发布期间的错误率，往往就是停机顺序写错的量。**

────────────────────────────────────

4.8 用 `println!` 打日志，管道一关进程就 panic

❌ 错误做法：
```rust
for line in huge_log {
    println!("{line}");     // 下游一关管道，这里直接 panic
}
```

实测现象（本文件自己踩到的）：第一次用 `./29_deployment.exe | head -12` 观察输出时，
程序在中途 panic：

```
thread 'main' (77340) panicked at library\std\src\io\stdio.rs:1165:9:
failed printing to stdout: 管道正在被关闭。 (os error 232)
```

原因有两层：
1. `println!` 在**写失败时会 panic**（它不是"尽力而为"的）。`os error 232` 是 Windows 的
   `ERROR_NO_DATA`，对应 POSIX 的 **EPIPE**：读端已经关了。
2. Rust 启动时**默认忽略 SIGPIPE**（C 程序的默认行为是"被 SIGPIPE 杀掉"）。
   于是"对端不见了"不再是一个信号，而是变成了一个普通的 IO 错误——
   而 `println!` 把这个错误转成了 panic。

为什么这条值得记：**日志收集器重启、`| head`、`| grep -q`、k8s 切日志管道**
都可能造成读端提前关闭。一个"日志写得太多太快"的服务，可能被自己的日志搞崩。

✅ 正确做法：
```rust
// 1) 单条打印：明确忽略写失败
let _ = writeln!(io::stdout(), "{line}");

// 2) 正式项目：用日志库（tracing / log + env_logger），
//    它们写失败时不会 panic，而是静默丢弃或记录一次
```

⚠️ 诚实说明：这条我**只捕获到 1 次**（`| head -12`），随后 11 次重跑
（含 10 次 `| head -1`）都没再触发。原因很清楚：这是**竞态**——
取决于读端关闭的那一刻，写端还有没有数据要写。**偶发才最危险：**
它在压测、在线上偶发崩溃，而你在本机怎么试都试不出来。
*/

// ============================================
// 第五部分：函数/宏卡片速查
// ============================================

/*
【env::var(name) -> Result<String, VarError>】
作用：读环境变量。
返回：`Ok(值)` / `Err(NotPresent)` / `Err(NotUnicode)`
注意：它是**全局状态**，并行测试里会互相污染。
      所以好习惯是"用闭包注入"（见 `Config::from_lookup`），而不是到处直接调。

【env::current_exe() -> io::Result<PathBuf>】
作用：拿到"正在运行的这一份"可执行文件的路径。
用途：读自身体积、判断部署目录、打印版本信息。
注意：符号链接 / 容器挂载下路径可能是映射后的路径。

【std::fs::metadata(path) -> io::Result<Metadata>】
作用：文件元信息。
常用：`.len()` 字节数、`.is_file()`、`.modified()`

【cfg!(debug_assertions) -> bool】
作用：编译期判断"是否开启调试断言"。
用途：区分 debug / release 行为，打印版本信息。
注意：它是**表达式**（返回值），`#[cfg(test)]` 是**属性**（作用于代码）。两者不要混。

【std::process::exit(code: i32) -> !】
作用：立即结束进程，返回退出码。
注意：**不会运行任何 Drop**，缓冲区也不会自动刷新。
      所以调用前要 `flush()`；服务里通常用"返回错误"而不是直接 exit。

【eprintln!(...)】
作用：写 stderr。
用途：错误与诊断。与 `println!`（stdout）分流，是 12-Factor 的日志约定。
注意：`println!` / `eprintln!` **写失败会 panic**（见常见错误 4.8）。
      要"尽力而为"地写，用 `let _ = writeln!(io::stdout(), ...);`。

【io::stdout().flush() -> io::Result<()>】
作用：强制刷出缓冲。
用途：退出前、写日志后。管道（非终端）场景下 stdout 通常是块缓冲，
      不 flush 可能丢最后几行。
*/

// ============================================
// 练习题
// ============================================

/*
1. 编译与体积对比
   要求：分别用 debug 和 release 编译本文件，记录两个体积，
         再用 `-C strip=symbols` 和 `-C panic=abort` 各试一次，
         列一张"开关 → 体积"的表格。
   提示：`ls -l` 看字节数；改一个开关必须重新编译一次，不要一次改两个。
   注意：数字要贴进日志，**不要写估计值**。

2. 配置校验
   要求：给 `Config` 再加一个 `WORKERS`，要求 1~64，越界报错。
         然后试 `WORKERS=0`、`WORKERS=65`、`WORKERS=abc`，确认三种输入
         都能得到"指明是哪个变量"的错误信息。
   提示：注意 `u16` 解析成功不代表语义合法（65 能解析，但要拒绝）。

3. 用假环境变量写测试
   要求：为 `Config::from_lookup` 写 3 个单测：全缺省、全部合法、端口非法。
   提示：传入 `|name| map.get(name).cloned()`，别碰真实 `env::var`。
   注意：想一想为什么不能用 `env::set_var`（并行测试 + 全局状态）。

4. 健康检查的真实性
   要求：让 `--health` 同时检查两件事：配置可解析、端口可绑定。
         然后**先启动一个占用该端口的进程**，再跑 `--health`，确认退出码为 1。
   提示：可以用 `python -m http.server <port>` 或再跑一份本程序占端口。
   注意：把命令和退出码都贴出来——"我觉得会返回 1"不算验证。

5. 日志分流
   要求：把程序分别以 `./app > out.log 2> err.log` 和 `./app > all.log 2>&1`
         运行，对比三个文件里各有什么。
   提示：注意观察 stderr 那条 WARN 去了哪里。

6. 停机顺序推演
   要求：假设你把 `graceful_shutdown` 的第 2 步和第 3 步对调，
         用一段话说明会出现什么现象、哪些请求会失败、失败长什么样。
   提示：想想"请求做到一半，数据库连接被关掉"是什么报错。

7. 部署方案设计（开放题）
   要求：为 `realtime-chat` 设计一个部署方案：要不要容器？配置文件还是环境变量？
         健康检查查什么？停机时如何不让正在聊天的用户掉线？
   提示：第 40 天那份代码里，正式停机时需要给客户端发一条什么消息？
   注意：写出**取舍理由**，不要只写结论。
*/
