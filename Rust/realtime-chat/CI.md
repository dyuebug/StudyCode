# realtime-chat 的 CI 质量门禁

> 生成日期：2026-11-07（第 43 天）
> 配套文件：仓库根 `.github/workflows/rust-realtime-chat.yml`

---

## 0. 一句话

**CI = 固定工具链 + 缓存 + 检查链 + 退出码。**
四者缺一，就会退化成"跑个脚本看看"。

---

## 1. 为什么 workflow 不在这个目录里

GitHub **只读仓库根目录**的 `.github/workflows/`。

如果把它放在 `Rust/realtime-chat/.github/workflows/ci.yml`：

- 不报错
- 不提示
- **永远不运行**

这是"静默失败"的一种：文件在、格式对、就是没生效。
所以本项目的 workflow 放在仓库根，用 `paths` 过滤只对子项目触发：

```yaml
on:
  push:
    paths:
      - 'Rust/realtime-chat/**'
```

同仓库的 Linux / Python 学习目录不受影响。

---

## 2. 检查链与本地实测

| # | 命令 | 防什么 | 本地实测结果 |
|---|---|---|---|
| 1 | `cargo fmt --all -- --check` | 格式漂移 | **退出码 0**（无输出） |
| 2 | `cargo clippy --all-targets -- -D warnings` | 惯用法错误 | 首次 **退出码 101**，改后 **0** |
| 3 | `cargo test` | 行为回归 | **3 passed + 7 passed，1.01 s** |
| 4 | 数量断言（见 §5） | "0 个用例"被当成通过 | 能拦住（见 §4） |

> ⚠️ 第 1–4 条是**本地手工跑通的，不是 CI 跑通的**。
> 本机是 Windows，无法触发 GitHub Actions。
> action 的版本号、runner 镜像、缓存命中率都**未实测**。

**本地 YAML 校验的一个小坑**：用 PyYAML 解析这份 workflow 时，
顶层键会显示成 `['name', True, 'concurrency', 'env', 'jobs']` ——
`on:` 变成了布尔 `True`。这是 YAML 1.1 把 `on`/`off`/`yes`/`no` 当布尔的规则，
不是 workflow 写错了；GitHub 用的是 YAML 1.2 语义，`on` 仍是普通键。
见到这个现象不要去"修" `on:`。

### 第 2 条的真实报错（第 43 天首次跑出来）

三个位置，同一个 lint：

```
error: useless conversion to the same type         --> examples/flood.rs:72:49
error: useless conversion to the same type         --> tests/ws.rs:310:28
error: useless conversion to the same type         --> tests/ws.rs:365:33
```

原因都一样：值已经是 `String`（`format!` 的产物）或 `Vec<u8>`
（`vec![1, 2, 3]` 的类型），再写 `.into()` 就是**同类型转换**。
三处删掉 `.into()` 并加注释说明原因后：

```
Finished `dev` profile [unoptimized + debuginfo] target(s) in 0.32s
clippy 退出码=0
```

**这条经历本身是第 42 天结论的延续**：
当时的"本地零警告"是**构建缓存**造成的——`examples/` 和 `tests/`
从来没有被重新编译过，所以它们的警告从未出现过。
`cargo build` 看不见的东西，`clippy --all-targets` 看得见。

---

## 3. 本地复现整套门禁

在 `Rust/realtime-chat/` 下依次执行（与 workflow 完全一致的顺序）：

```bash
set -e                          # 失败即停，与 CI 的短路语义一致

cargo fmt --all -- --check
cargo clippy --all-targets -- -D warnings
cargo test
```

`set -e` 不可省。没有它，你看到的退出码是**最后一条命令**的，
而 Shell 里最后一条命令经常是 `echo`（永远返回 0）——
第 42 天就是这样把一次失败的 Docker 构建汇报成"成功"的。

---

## 4. 实测：0 个用例也算通过

在一个**空的 cargo 项目**里，把测试文件命名成 `tests/ws.rs.bak`
（`cargo` 只收集 `tests/*.rs`），实测输出：

```
running 0 tests

test result: ok. 0 passed; 0 failed; 0 ignored; 0 measured; 0 filtered out
cargo test 退出码=0
```

于是"只信退出码"的判据会把它放行：

```bash
grep -q "test result: ok" test.log && echo "判为通过"
# → 判为通过     ← 这就是漏洞
```

### 断言不能写成"每一行都要有通过"

本项目正常输出里**天然有两段 `0 passed`**（实测）：

```
running 3 tests   test result: ok. 3 passed    ← src/room.rs 单测
running 0 tests   test result: ok. 0 passed    ← src/main.rs 的 bin 单测（main 里没有 #[test]）
running 7 tests   test result: ok. 7 passed    ← tests/ws.rs 集成测试
running 0 tests   test result: ok. 0 passed    ← doctest（没有文档示例）
```

所以"每一行都必须 ≥1 个通过"会把**正常情况**判成失败。
正确的写法是**汇总**：

```bash
total=$(grep -Eo 'test result: ok\. [0-9]+ passed' /tmp/test.log \
        | grep -Eo '[0-9]+' \
        | awk '{s += $1} END {print s + 0}')
echo "本次通过用例总数：$total"      # 实测 → 10
[ "$total" -ge 10 ] || exit 1
```

`10 = 3 + 0 + 7 + 0`。加测试时要往上改这个阈值（只增不减）。

> 局限说清楚：这条断言证明的是"**至少**跑了 10 个用例"，
> 不是"该跑的都跑了"。单独一条用例被摘掉、另加两条新的，总数仍是 10。
> 更强的做法是按测试二进制分别断言，成本更高——
> 对当前规模，总数量断言已经能拦住"整个 tests/ 没被收集"这类真实事故。

---

## 4.1 实测：`cargo test | tee` 也会吃掉退出码

CI 里要保存测试日志，很自然会写成：

```bash
cargo test 2>&1 | tee /tmp/test.log
```

但管道的退出码是**最右侧命令**的，也就是 `tee` 的——它永远成功。
于是测试失败也变成绿灯。本机实测（第 43 天）：

```bash
bash -c 'false | tee /dev/null >/dev/null; echo "退出码=$?"'
# → 退出码=0        ← 失败被吃掉

bash -c 'set -o pipefail; false | tee /dev/null >/dev/null; echo "退出码=$?"'
# → 退出码=1        ← 正确
```

这是"退出码被吃掉"的**第二副面孔**。
第一副是 `cmd; echo $?`（第 42 天，`;` 让你读到的是 `echo` 的退出码）。
两副面孔的共同点：**退出码来自你没想到的那条命令**。

所以 workflow 的测试步骤写的是：

```bash
set -o pipefail
cargo test 2>&1 | tee /tmp/test.log
```

---

## 5. 缓存键

`actions/cache` 的键：

```yaml
key: cargo-${{ runner.os }}-${{ steps.toolchain.outputs.cachekey }}-${{ hashFiles('Rust/realtime-chat/Cargo.lock') }}
```

三个输入，缺一不可：

| 输入 | 漏掉会怎样 |
|---|---|
| `Cargo.lock` 内容 | 换了依赖版本仍命中旧缓存 → 编译产物与源码不符 |
| 工具链版本 | 升级 rustc 后仍命中旧产物 → 缓存毒化 |
| 操作系统 | 跨系统的 `target/` 不可互换 |

`30_ci_cd.exe` 里有这套逻辑的最小实现，实测：

```
缓存键 = 3caa44347372a825（来源：../realtime-chat/Cargo.lock（42582 字节））
缓存键 = 3caa44347372a825        ← 再跑一次，不变
末尾加一个空格 → 87f9ea202bd3807f  ← 变了一个字节，键就变
```

用的是 FNV-1a（64 位）。**它不是密码学哈希**，只做"内容指纹"；
真实缓存键用 SHA-256（`hashFiles` 内部就是）。

---

## 6. 明确没做的事

| 没做 | 原因 |
|---|---|
| 矩阵构建（多平台 × 多工具链） | 单人学习项目、无多平台发布需求。按 YAGNI 不加 |
| 在 CI 里跑 `docker build` | 首次构建实测 **279.6 秒**。每次推送都付这个成本不值，放在发布流程里更合适 |
| `cargo build --release` | `cargo test` 已覆盖行为；release 差异（第 42 天讲过 `overflow-checks`）属于发布前检查 |
| 缓存 `~/.rustup/toolchains` | `dtolnay/rust-toolchain` 自身已缓存；再叠一层收益小、失效逻辑复杂 |
| 覆盖率上报 | 需要额外服务或 artifact 解析，与当前学习目标无关 |

---

## 7. 下一步（如果要真正启用）

1. 把仓库推到一个有 Actions 额度的远端
2. 进 Actions 页签，手动 `Run workflow`（本文件已开 `workflow_dispatch`）
3. 逐条对照 §2 的本地结论：命令、退出码、耗时
4. 只有这一步做完，"CI 通过"才算事实，而不是推断
