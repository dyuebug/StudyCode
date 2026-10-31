# ============================================
# realtime-chat —— 多阶段构建
# 日期：2026-11-06（第42天）
# ============================================
#
# 【为什么要多阶段（multi-stage）】
# 编译需要 rustc + cargo + 链接器（`rust:1-slim` 约 1.2 GB），
# 运行只需要一个二进制（`debian:bookworm-slim` + 一份 4 MB 的文件）。
# 单阶段 Dockerfile 会把整套编译工具链留进镜像；
# 多阶段只在阶段 1 里编译，阶段 2 从阶段 1 **拷贝产物**，工具链被丢弃。
#
# 【为什么依赖和源码要分两层 COPY】
# Docker 按层做缓存：某层的内容没变，构建时直接复用上一次的结果。
# 依赖（actix-web、tokio、serde…）编译要几分钟，源码改动却很频繁。
#   - 第 1 层：Cargo.toml + Cargo.lock + 占位源码 → 编出依赖
#   - 第 2 层：真源码 → 只重编本 crate
# 改一行代码时第 1 层命中缓存，构建从"几分钟"降到"几十秒"。

# ---------------- 阶段 1：编译 ----------------
FROM rust:1-slim-bookworm AS builder
WORKDIR /app

# 【为什么要有这四个环境变量】
# 第一次构建实测失败在这句话上：
#   `spurious network error: transfer too slow: failed to transfer more than 10 bytes in 30s`
# 容器里的 TCP 能连通，但下载 crates.io 索引的吞吐极低，cargo 默认的
# "30 秒内慢于 10 字节就放弃"直接判定网络不可用。
# 这四个值是"容忍慢网络"的配置，对构建结果没有任何影响：
#   CARGO_NET_RETRY=10            多试几次（默认 3 次）
#   CARGO_HTTP_TIMEOUT=600        单次请求超时放到 10 分钟（默认 30 秒）
#   CARGO_HTTP_LOW_SPEED_LIMIT=1  只要还有 1 字节/秒的进度就不算失败（默认 10）
#   CARGO_HTTP_MULTIPLEXING=false 关掉 HTTP/2 多路复用——某些代理/网络下它是元凶
# 国内环境更彻底的办法是换镜像源（rsproxy 等），这里不写死，避免镜像绑死在某一家。
ENV CARGO_NET_RETRY=10 \
    CARGO_HTTP_TIMEOUT=600 \
    CARGO_HTTP_LOW_SPEED_LIMIT=1 \
    CARGO_HTTP_MULTIPLEXING=false

# 只拷清单。Cargo.toml 同时声明了 lib（src/lib.rs）和 bin（src/main.rs），
# 所以两个占位文件都必须存在，否则 cargo 报"找不到目标"，缓存层直接失效。
COPY Cargo.toml Cargo.lock ./
RUN mkdir -p src \
 && echo '' > src/lib.rs \
 && echo 'fn main() {}' > src/main.rs \
 && cargo build --release --locked

# 真源码。COPY 会更新 mtime，cargo 据此判定"要重编"。
# static/chat.html 被 include_str! 编进二进制，所以也是构建输入。
#
# ⚠️ 但仅仅 COPY 是**不够的**：Docker 会保留构建上下文里文件的原始 mtime，
# 而宿主机上的源码 mtime 很可能**早于**上一步刚生成的占位文件。
# 于是 cargo 认为"源码没变过"，直接复用 stub 的产物——镜像里躺着一个
# `fn main() {}`，容器起来 0 秒退出、日志一行没有。
# 本次实测就这么踩了一次（见 DEPLOY.md §10）。所以必须显式 touch。
COPY src ./src
COPY static ./static
RUN set -e; \
    touch src/main.rs src/lib.rs; \
    cargo build --release --locked; \
    if ! grep -aq "聊天服务" /app/target/release/realtime-chat; then \
      echo "构建产物是 stub 二进制：源码没被重编"; exit 1; \
    fi

# ---------------- 阶段 2：运行 ----------------
FROM debian:bookworm-slim

# 非 root 运行：容器默认以 root 启动，服务被攻破就等于把 root 交出去。
# UID 固定成 10001，便于在宿主机上设置目录权限。
RUN useradd --create-home --uid 10001 app
WORKDIR /app

COPY --from=builder --chown=app:app /app/target/release/realtime-chat /app/realtime-chat

USER app

# ⚠️ 必须在镜像里把默认值改成 0.0.0.0。
# 程序自己的默认值是 127.0.0.1:8090（本机开发友好），
# 但容器里的 127.0.0.1 是容器自己的回环，宿主机永远访问不到。
ENV BIND_ADDR=0.0.0.0:8090
EXPOSE 8090

# 健康检查：容器里没有 curl，用 bash 的 /dev/tcp 直接说 HTTP。
# 只测"端口能连上"是不够的（端口开着重启中的服务也可能连上），
# 这里真的发一个 GET /health 并检查状态行是 200。
# 注意：如果启动时改了 BIND_ADDR 的端口，这条检查也要跟着改。
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD bash -c 'exec 3<>/dev/tcp/127.0.0.1/8090 && printf "GET /health HTTP/1.0\r\n\r\n" >&3 && head -1 <&3 | grep -q " 200 "'

# 前台运行（PID 1）。容器里的主进程必须在前台，写成 `app &` 会被判定为"已退出"。
CMD ["/app/realtime-chat"]
