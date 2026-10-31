# realtime-chat 部署说明

> 本文所有数字均为**实测**（采集命令写在各节）；
> 未验证的内容会明确标注"未验证"。

---

## 0. 实测数据（2026-11-06）

| 项 | 值 | 采集方式 |
|---|---|---|
| 镜像体积 | **123 MB** | `docker images realtime-chat:day42` |
| 其中二进制层 | 7 MB（未 strip）；stub 版本只有 446 KB | `docker history` |
| 运行镜像内是否有 rustc | **没有**（多阶段生效的证明） | `command -v rustc` → 不存在 |
| 首次完整构建 | **279.6 s**（依赖层，受本机网络影响） | `--progress=plain` 的 `#12 DONE` |
| 改源码后重建 | **3.755 s**（依赖层命中缓存，只重编本 crate 1.28 s） | `time docker build` |
| 镜像内二进制 | 437,504 B（427 KB），对比 Windows 本机 206 KB | `ls -l /app/realtime-chat` |
| 容器内 `/health` | `200` `{"status":"ok"}` | `curl -o /dev/null -w '%{http_code}'` |
| 容器内 `/rooms` | `200` `{"data":[],"success":true}` | 同上 |
| 容器内 `GET /` | `200`，5512 字节（`chat.html` 被 `include_str!` 编进二进制） | `curl -w '%{size_download}'` |
| `HEALTHCHECK` | `healthy`（`ExitCode: 0`） | `docker inspect .State.Health` |
| 容器用户 | `uid=10001(app)` | `id` |

那张 3.755 s 的表里藏着本项目最大的一个坑，见 §11。

---

## 1. 一句话

一个单进程的 actix-web 服务：同一端口上跑 HTTP 和 WebSocket，状态全在内存里。
**不需要数据库、不需要外部依赖**——这既是它好部署的原因，也是它不能水平扩展的原因（见 §7）。

---

## 2. 环境变量

全部**可选**，都有默认值。程序只读这四个：

| 变量 | 默认 | 代码位置 | 说明 |
|---|---|---|---|
| `BIND_ADDR` | `127.0.0.1:8090` | `src/main.rs:32` | 监听地址。**容器里必须是 `0.0.0.0:8090`** |
| `WORKERS` | `4` | `src/main.rs:33` | worker 线程数。注册表在 `HttpServer::new` 外创建，worker 数不影响房间共享 |
| `HEARTBEAT_MS` | `30000` | `src/chat.rs`（`ChatConfig::from_env`） | 心跳间隔，毫秒。解析失败或 ≤0 时退回默认值 |
| `SEND_TIMEOUT_MS` | `10000` | `src/chat.rs`（`ChatConfig::from_env`） | 单次写的超时，毫秒。超时判定"对端不读"并踢掉该连接 |

⚠️ 两个时间参数的"解析失败退回默认值"是**有意的**：
它们只在调优时才会改，写错了不该让服务起不来。
但这也意味着**写错了不会有任何报错**——改完请用启动日志确认（§4）。

---

## 3. 部署方式

### 3.1 本机开发

```bash
cargo run
# 打开 http://127.0.0.1:8090/
```

### 3.2 直接跑二进制

```bash
cargo build --release --locked
# 产物：target/release/realtime-chat（Windows 为 realtime-chat.exe）

BIND_ADDR=0.0.0.0:8090 ./target/release/realtime-chat
```

**注意 glibc 版本**：Linux 上这个二进制动态链接 glibc，
在比编译机更老的系统上会报 `version 'GLIBC_2.xx' not found`。
两个出路：在更老的系统上编译，或用 `x86_64-unknown-linux-musl` 目标（本次**未实测**）。

### 3.3 容器

```bash
docker build -t realtime-chat:day42 .

# 前台跑（Ctrl-C 停）
docker run --rm -p 8090:8090 realtime-chat:day42

# 后台跑
docker run -d --name chat -p 8090:8090 \
  -e WORKERS=4 \
  -e HEARTBEAT_MS=30000 \
  realtime-chat:day42

docker logs -f chat
docker stop chat          # 默认给 10 秒优雅停机时间，见 §6
```

`Dockerfile` 是多阶段的：阶段 1 负责编译，阶段 2 只带走一个二进制，工具链不进镜像。
镜像里已经把 `BIND_ADDR` 设成 `0.0.0.0:8090`——这是容器里**最容易错的一处**：
程序自身默认是 `127.0.0.1`，而容器里的 `127.0.0.1` 是容器自己的回环，
宿主机 `curl` 永远连不上，日志却显示"已监听"。

---

## 4. 启动日志：先看这四行

```
🚀 聊天服务 http://0.0.0.0:8090（4 个 worker）
   心跳 30000 ms，写超时 10000 ms
   打开 http://0.0.0.0:8090/ 即可，或在任意 WebSocket 客户端连 ws://0.0.0.0:8090/ws/lobby
   HTTP 观测：GET /rooms   GET /rooms/{room}/members
```

四行分别回答：绑在哪、几个线程、两个时间参数**实际生效的值**、
以及"这个服务是干什么的"。第二行尤其重要——
环境变量写错时它不会报错，只会在这一行露出真相。

---

## 5. 健康检查

```bash
curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:8090/health
# 200
```

`Dockerfile` 里已经挂了 `HEALTHCHECK`，每 30 秒真的发一次 `GET /health` 并检查状态行。

```bash
docker ps --format 'table {{.Names}}\t{{.Status}}'
# chat   Up 2 minutes (healthy)
```

**`/health` 查的是什么**：只查"HTTP 栈还活着"。
它不查房间注册表、不查内存占用——这个服务没有外部依赖，
"HTTP 能应答"就是它能给出的最强保证了。

---

## 6. 优雅停机 —— 本项目最需要注意的一条

WebSocket 是**长连接**，这让"停机"变得比普通 HTTP 服务麻烦：

1. 容器收到 `SIGTERM`，actix-web 开始优雅停机：**停止 accept，等现有连接结束**
2. 但 WebSocket 连接不会自己结束——客户端连上了就一直连着
3. `docker stop` 默认只等 **10 秒**，超时就 `SIGKILL`

结果：连接被硬切断，客户端看到的是"连接异常断开"，而不是一条正常的关闭帧。

处理办法（按推荐顺序）：

1. **给足停机时间**：`docker stop -t 30 chat`
2. **停机前主动通知**：先在 `/` 页面或广播里发一条 `{"type":"bye"}`，
   等几秒再停（本项目的协议里**还没有** `bye`，属于待办；见 §9）
3. **客户端实现自动重连**：任何长连接服务的前提，停机做不到"无缝"时靠它兜底

实测数据（第 41 天集成测试）：actix 的优雅停机默认最多等 **30 秒**；
7 个用例因为连接还开着，曾经每条都白等满 30 秒，整套测试跑了 31.05 秒。
**这正是线上重启会遇到的行为**，测试里踩到等于提前知道了线上会怎么慢。

---

## 7. 日志去哪了

全部走 **stdout / stderr**（12-Factor 第 11 条），不写文件：

```bash
docker logs -f chat                  # 实时
docker logs --tail 100 chat          # 最近 100 行
```

两个**已知的、值得知道的**现状：

1. **没有日志级别、没有时间戳、没有结构化字段**。
   本项目用的是 `println!`，不是 `tracing` / `log` 之类的日志库。
   原因：`actix-web` 的 `Logger` 中间件会把响应体包成一个**外部无法命名的私有类型**，
   与项目的返回类型冲突；而 `Logger` 本身也要初始化一个日志实现才会真的打印。
   （详见 `src/lib.rs` 里的注释。）
   真上生产，第一步就是把 `println!` 换成 `tracing`。
2. **`println!` 在管道被关闭时会 panic**。
   已实测到一次：用 `./app | head -12` 观察输出时，进程在中途 panic
   （`failed printing to stdout: 管道正在被关闭 (os error 232)`）。
   这是竞态，重跑 11 次没再触发。日志收集器重启、`| head`、`| grep -q`
   都可能造成读端提前关闭。换成日志库即可避免（写失败不 panic）。

---

## 8. 质量门禁（部署前必须全绿）

部署的前提是"这个产物是被检查过的"。三条命令，本地实测结论：

| # | 命令 | 本地实测 | 防什么 |
|---|---|---|---|
| 1 | `cargo fmt --all -- --check` | 退出码 **0**（无输出） | 格式漂移 |
| 2 | `cargo clippy --all-targets -- -D warnings` | 首次 **101** → 改后 **0** | 惯用法错误 |
| 3 | `cargo test` | **10 passed，1.01 s** | 行为回归 |

有一条很容易漏：**`cargo test` 在 0 个用例时退出码也是 0**。
所以门禁里要加数量断言（汇总后 ≥10），否则"测试文件没被收集"会静默通过。

```bash
set -e
cargo fmt --all -- --check
cargo clippy --all-targets -- -D warnings
set -o pipefail                  # 不加这个，`| tee` 会吞掉 cargo 的退出码
cargo test 2>&1 | tee /tmp/test.log
total=$(grep -Eo 'test result: ok\. [0-9]+ passed' /tmp/test.log \
        | grep -Eo '[0-9]+' | awk '{s += $1} END {print s + 0}')
[ "$total" -ge 10 ] || { echo "通过用例总数 $total < 10"; exit 1; }
```

> ⚠️ **CI 部分未实测**：workflow 写在仓库根
> `.github/workflows/rust-realtime-chat.yml`（GitHub 不读子目录里的 workflow），
> 但本机无法触发 GitHub Actions。详见 `CI.md`。
> 上面的三条命令是**本地手工跑通**的，不等于"CI 跑通"。

---

## 9. 已知局限（部署视角）

| # | 局限 | 部署上的后果 |
|---|---|---|
| 1 | 状态全在内存，注册表只增不减 | **不能水平扩展**：两个实例互不可见，用户会被分到不同房间；空房间也不回收 |
| 2 | 无鉴权、无昵称 | 能访问端口的人都能进任意房间 |
| 3 | 无历史消息 | 重启即丢失所有对话 |
| 4 | 无 `bye` 关闭通知 | 停机时客户端看到的是"异常断开"而不是正常关闭（见 §6） |
| 5 | `SEND_TIMEOUT_MS` 固定 | 网络极差的移动端可能被误杀 |
| 6 | 无消息大小限制 | 单条消息多大由 WebSocket 协议决定，本项目没设阈值 |
| 7 | 无 `/metrics` | 只能靠 `/rooms` 手工看，没有可接的监控指标 |

结论：**这个服务适合单体部署、少量用户、可随时重启的场景**。
要真正多实例，得把广播换成 Redis pub/sub 一类的外部通道——那是另一次改造。

---

## 10. 排错清单

| 现象 | 先查什么 |
|---|---|
| 宿主机 `curl` 不通，日志却说已监听 | `BIND_ADDR` 是不是还是 `127.0.0.1`（§2） |
| 容器起不来，`docker logs` 一行没有 | 端口被占？`BIND_ADDR` 写的端口和 `-p` 映射的端口不一致？ |
| 两个客户端连同一个房间却看不到对方 | 是不是各连到了不同实例（§9 第 1 条） |
| 有人一直"在线"，但发消息无人回 | 他是"只连不读"，要等 `SEND_TIMEOUT_MS` 才被踢（默认 10 秒） |
| 重启后所有人都掉了 | 预期行为，见 §6；客户端需要自动重连 |
| 消息乱序或丢失 | 慢客户端会收到 `{"type":"lagged","missed":n}` 然后被断开——这是**设计**，不是 bug |

---

## 11. 构建排障：第 42 天踩的三个坑（都实测过）

### 10.1 镜像里的二进制是 stub，容器起来 0 秒退出、日志一行没有

**现象**：`docker run` 之后 `docker ps -a` 显示 `Exited (0)`，`docker logs` **完全空白**。
没有报错、没有堆栈，比崩溃还难查——因为"成功退出"看起来不像故障。

**证据（构建日志）**：

```
#12 DONE 279.6s     ← 依赖编译，正常（依赖确实很多）
#15 DONE 0.4s       ← "真源码"编译，0.4 秒
```

0.4 秒就是答案：**它根本没编**，直接把上一步 `fn main() {}` 的产物留在了镜像里。

**原因**：Docker 的 `COPY` 会**保留构建上下文里文件的原始 mtime**。
宿主机上的 `src/main.rs` mtime（10-29 之前）早于上一步刚生成的占位文件，
于是 cargo 的指纹判定"源码没变过"，跳过编译。

**修法**：`COPY` 之后显式 `touch`，再加一条"产物不是 stub"的断言。

```dockerfile
RUN set -e; \
    touch src/main.rs src/lib.rs; \
    cargo build --release --locked; \
    if ! grep -aq "聊天服务" /app/target/release/realtime-chat; then \
      echo "构建产物是 stub 二进制：源码没被重编"; exit 1; \
    fi
```

**通用教训**：多阶段/分层构建里，"这一步很快就完成"永远值得怀疑。
`0.4s` 编译一个 actix-web 应用是不可能的——**快得不合理就是没干活**。
断言（`grep -a` 查一个只有真二进制才有的字符串）把"静默失败"变成了"构建失败"。

### 10.2 容器能连上 crates.io，但 cargo 判定"网络不可用"

```
warning: spurious network error: transfer too slow:
         failed to transfer more than 10 bytes in 30s (transferred 0 bytes)
error: failed to get `actix-web` as a dependency
```

TCP 层能连通（`/dev/tcp/index.crates.io/443` 成功），但吞吐极低，
触发 cargo 默认"30 秒内慢于 10 字节就放弃"的阈值。

修法：容忍慢网络（不影响构建结果）。

```
CARGO_NET_RETRY=10
CARGO_HTTP_TIMEOUT=600
CARGO_HTTP_LOW_SPEED_LIMIT=1
CARGO_HTTP_MULTIPLEXING=false    # HTTP/2 多路复用在某些代理下是元凶
```

国内环境更彻底的做法是换镜像源（rsproxy 等），本项目没有写死，避免镜像绑死在某一家。

### 10.3 本机"零警告"不等于真的没有警告

构建镜像时新出现一条警告（本机从来没出现过）：

```
warning: use of deprecated method `Atomic::<usize>::fetch_update`:
         renamed to `try_update` for consistency
  --> src/room.rs:84:14
```

两个原因叠在一起：

1. **缓存**：本机 `cargo build` 认为 `room.rs` 没变，不会重新编译，也就不会重新报警告
2. **工具链版本**：镜像里的 `rust:1-slim-bookworm` 是 **rustc 1.99.1**，本机是 **1.92.0**

而"按建议改"这条路**走不通**——实测：

| 工具链 | `try_update` 能否编译 |
|---|---|
| 本机 1.92.0 | ❌ `E0658: use of unstable library feature 'atomic_try_update'`（issue #135894） |
| 镜像 1.99.1 | ✅ 编译通过，运行输出 `Ok(1)` |

结论：**保留 `fetch_update`**，等本机工具链升到 1.99+ 再迁移。
这条警告在稳定版上**无法消除**——编译器给出的替代品在自己这套工具链上还用不了。

### 10.4 一个方法论上的自省

第一次跑 `docker build ... ; echo "退出码=$?"` 时，命令整体返回 0（`echo` 成功），
于是我以为构建成功了。**`;` 之后的命令吃掉了真实退出码。**
真实结果要从日志里看（`#12 ERROR ... exit code: 101`）。

教训：**别在要判断成败的命令后面接 `echo`**；
`cmd; echo $?` 与 `cmd` + `set -e` / `&&` 的语义完全不同。

