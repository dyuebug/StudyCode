// ============================================
// protocol.rs —— WebSocket 协议层（纯 std，无异步、无网络）
// 日期：2026-10-29（第34天）
// ============================================
//
// 【为什么单独一个文件】
// 握手和帧编解码是**纯函数**：输入字节，输出字节。
// 把它们和 actix-web 的异步 IO 分开，才能：
//   1. 用单元测试验证（`cargo test`）
//   2. 一眼看出协议本身长什么样
//
// 真实项目里不需要手写这些——actix-ws 已经实现好了。
// 这里手写一遍，是因为"会用库"和"知道库在做什么"是两件事。

use sha1::{Digest, Sha1};

// ============================================
// 第一部分：握手 —— Sec-WebSocket-Accept 怎么算
// ============================================

/// RFC 6455 §1.3 规定的魔法字符串（固定值，不可改）
///
/// 它不是密钥，不是盐，只是一个"常量"。
/// 加它是为了让 Accept 值不可能是客户端自己瞎猜的——
/// 从而排除"某个 HTTP 缓存误把一个普通请求当成 WS 连接"的情况。
const WS_GUID: &str = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/// 由客户端的 `Sec-WebSocket-Key` 计算服务端的 `Sec-WebSocket-Accept`
///
/// # 算法（三步，缺一不可）
/// 1. `key + GUID` 拼接成字符串
/// 2. 对该字符串做 SHA-1（得到 20 字节）
/// 3. 对 20 字节做 Base64
///
/// # 参数
/// - `client_key`: 请求头 `Sec-WebSocket-Key` 的值，是客户端随机的 16 字节的 Base64
///
/// # 返回
/// 应回填到响应头 `Sec-WebSocket-Accept` 的字符串
///
/// # 示例（RFC 6455 给出的官方测试向量）
/// ```
/// assert_eq!(
///     accept_key("dGhlIHNhbXBsZSBub25jZQ=="),
///     "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
/// );
/// ```
pub fn accept_key(client_key: &str) -> String {
    // 步骤 1：拼接
    // 客户端 key 是可见 ASCII，直接拼不会出错
    let joined = format!("{}{}", client_key, WS_GUID);

    // 步骤 2：SHA-1
    // Digest::new() → update() → finalize()
    // 注意 finalize() 返回 GenericArray，要转成 &[u8] 才能交给 base64
    let mut hasher = Sha1::new();
    hasher.update(joined.as_bytes());
    let digest = hasher.finalize();

    // 步骤 3：Base64 标准字母表（STANDARD，带 '=' 填充）
    // ⚠️ 不能用 URL_SAFE：Accept 头里出现 '-' 或 '_' 是错的
    use base64::Engine as _;
    base64::engine::general_purpose::STANDARD.encode(digest)
}

// ============================================
// 第二部分：帧 —— 字节级结构
// ============================================

/// 帧操作码（opcode，4 bit）
///
/// 只列常用的五个。扩展操作码（0x3–0x7、0xB–0xF）保留给将来，收到应报错。
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Opcode {
    /// 0x0：延续帧（分片消息的后续片段）
    Continuation,
    /// 0x1：文本帧（payload 必须是合法 UTF-8）
    Text,
    /// 0x2：二进制帧（payload 任意字节）
    Binary,
    /// 0x8：关闭帧（payload 前 2 字节是关闭码）
    Close,
    /// 0x9：心跳请求
    Ping,
    /// 0xA：心跳应答
    Pong,
}

impl Opcode {
    /// 转成 4 bit 的数值
    pub fn to_u8(self) -> u8 {
        match self {
            Opcode::Continuation => 0x0,
            Opcode::Text => 0x1,
            Opcode::Binary => 0x2,
            Opcode::Close => 0x8,
            Opcode::Ping => 0x9,
            Opcode::Pong => 0xA,
        }
    }

    /// 从 4 bit 数值还原
    ///
    /// # 返回
    /// - `Some(opcode)`: 已知操作码
    /// - `None`: 保留操作码，按 RFC 必须**关闭连接**
    pub fn from_u8(v: u8) -> Option<Self> {
        match v {
            0x0 => Some(Opcode::Continuation),
            0x1 => Some(Opcode::Text),
            0x2 => Some(Opcode::Binary),
            0x8 => Some(Opcode::Close),
            0x9 => Some(Opcode::Ping),
            0xA => Some(Opcode::Pong),
            _ => None,
        }
    }
}

/// 解码后的一个完整帧
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Frame {
    /// 是否为消息的最后一帧（分片时前面若干帧 fin = false）
    pub fin: bool,
    /// 帧类型
    pub opcode: Opcode,
    /// 去掉掩码后的、原始的载荷字节
    pub payload: Vec<u8>,
}

/// 帧编解码错误
#[derive(Debug, PartialEq, Eq)]
pub enum FrameError {
    /// 缓冲区里的字节还不够一个完整帧，需要继续读
    ///
    /// ⚠️ 这不是"错误"，是 TCP 流式传输的**正常状态**。
    /// 单独立一个变体，是为了让调用方无法把它和真正的协议错误混淆。
    Incomplete,
    /// 保留操作码
    ReservedOpcode(u8),
    /// 声明长度超过 `MAX_PAYLOAD`
    PayloadTooLarge(u64),
    /// 客户端帧没有掩码（RFC 6455 §5.1 要求客户端必须掩码）
    MissingMask,
}

/// 允许的单个帧最大载荷（1 MiB）
///
/// 没有这一条，一个恶意的 `payload_len = 2^63-1` 声明就能让你
/// 尝试分配 8 EiB 内存 —— 这是最经典的 DoS 手法之一。
pub const MAX_PAYLOAD: u64 = 1024 * 1024;

/// 编码一个帧
///
/// # 参数
/// - `fin`: 是否最后一帧。单帧消息传 `true`
/// - `opcode`: 帧类型
/// - `payload`: 载荷字节
/// - `mask_key`: `Some([u8;4])` 表示加掩码（**客户端必须传 Some**），
///              `None` 表示不加掩码（**服务端必须传 None**）
///
/// # 返回
/// 可直接写入 TCP 的完整帧字节
///
/// # 长度编码规则（三种）
/// - `< 126`      → 7 bit 直接表示
/// - `126..=65535` → 标记位 126 + 2 字节大端长度
/// - `> 65535`    → 标记位 127 + 8 字节大端长度
pub fn encode_frame(fin: bool, opcode: Opcode, payload: &[u8], mask_key: Option<[u8; 4]>) -> Vec<u8> {
    let mut out = Vec::with_capacity(payload.len() + 14);

    // 字节 0：FIN(1) + RSV(3) + opcode(4)
    // RSV 三位必须全 0（没用扩展时）。这里用位移拼出来：
    //   fin ? 0x80 : 0x00  最高位
    //   opcode             低 4 位
    let b0 = (if fin { 0x80 } else { 0x00 }) | (opcode.to_u8() & 0x0F);
    out.push(b0);

    // 字节 1：MASK(1) + payload_len(7)
    let mask_bit = if mask_key.is_some() { 0x80 } else { 0x00 };
    let len = payload.len();
    if len < 126 {
        out.push(mask_bit | (len as u8));
    } else if len <= 65_535 {
        // 标记 126，后面跟 u16 大端
        out.push(mask_bit | 126);
        out.extend_from_slice(&(len as u16).to_be_bytes());
    } else {
        // 标记 127，后面跟 u64 大端
        out.push(mask_bit | 127);
        out.extend_from_slice(&(len as u64).to_be_bytes());
    }

    // 掩码键 + 掩码后的载荷
    match mask_key {
        None => out.extend_from_slice(payload),
        Some(key) => {
            // 4 字节掩码键原文放在载荷前面
            out.extend_from_slice(&key);
            // 载荷按字节与 key 循环异或
            // `% 4` 就是"循环使用 4 字节"的意思
            for (i, b) in payload.iter().enumerate() {
                out.push(b ^ key[i % 4]);
            }
        }
    }

    out
}

/// 解码一个帧
///
/// # 参数
/// - `buf`: 到目前为止收到的全部字节（可能只包含半个帧）
/// - `is_client_frame`: `true` 表示按"客户端→服务端"的规则校验（必须带掩码）
///
/// # 返回
/// - `Ok((frame, consumed))`: 解析出一个帧，`consumed` 是它占用的字节数
/// - `Err(FrameError::Incomplete)`: 字节还不够，继续读
/// - `Err(...)`: 协议错误，必须关闭连接
pub fn decode_frame(buf: &[u8], is_client_frame: bool) -> Result<(Frame, usize), FrameError> {
    // 至少要有 2 字节才能读出 opcode 和长度标记
    if buf.len() < 2 {
        return Err(FrameError::Incomplete);
    }

    let b0 = buf[0];
    let b1 = buf[1];

    // FIN：最高位
    let fin = b0 & 0x80 != 0;

    // RSV1–3（0x70）：没用扩展时必须为 0
    // 这里不单独报错，简化为"忽略"——真实实现要拒绝
    let opcode_raw = b0 & 0x0F;
    let opcode = Opcode::from_u8(opcode_raw).ok_or(FrameError::ReservedOpcode(opcode_raw))?;

    let masked = b1 & 0x80 != 0;
    let len_marker = b1 & 0x7F;

    // 根据长度标记决定后续头部长度
    let mut cursor = 2usize;
    let payload_len: u64 = match len_marker {
        126 => {
            // 需要 2 字节大端长度
            if buf.len() < cursor + 2 {
                return Err(FrameError::Incomplete);
            }
            let v = u16::from_be_bytes([buf[cursor], buf[cursor + 1]]) as u64;
            cursor += 2;
            v
        }
        127 => {
            // 需要 8 字节大端长度
            if buf.len() < cursor + 8 {
                return Err(FrameError::Incomplete);
            }
            let mut arr = [0u8; 8];
            arr.copy_from_slice(&buf[cursor..cursor + 8]);
            let v = u64::from_be_bytes(arr);
            cursor += 8;
            v
        }
        n => n as u64,
    };

    // 在分配内存**之前**检查大小
    if payload_len > MAX_PAYLOAD {
        return Err(FrameError::PayloadTooLarge(payload_len));
    }

    // 客户端帧必须带掩码（RFC 6455 §5.1）
    if is_client_frame && !masked {
        return Err(FrameError::MissingMask);
    }

    // 掩码键 4 字节
    let mask_key: Option<[u8; 4]> = if masked {
        if buf.len() < cursor + 4 {
            return Err(FrameError::Incomplete);
        }
        let key = [buf[cursor], buf[cursor + 1], buf[cursor + 2], buf[cursor + 3]];
        cursor += 4;
        Some(key)
    } else {
        None
    };

    // 载荷本身
    let payload_len = payload_len as usize;
    if buf.len() < cursor + payload_len {
        return Err(FrameError::Incomplete);
    }
    let mut payload = buf[cursor..cursor + payload_len].to_vec();
    cursor += payload_len;

    // 解掩码：再异或一次就还原（异或的对称性）
    if let Some(key) = mask_key {
        for (i, b) in payload.iter_mut().enumerate() {
            *b ^= key[i % 4];
        }
    }

    Ok((Frame { fin, opcode, payload }, cursor))
}

// ============================================
// 第三部分：断线重连的退避算法（纯函数，可测）
// ============================================

/// 计算第 `attempt` 次重连的等待秒数（指数退避 + 抖动）
///
/// # 参数
/// - `attempt`: 第几次重连，从 0 开始
/// - `base_ms`: 首次等待的毫秒数（如 500）
/// - `max_ms`: 等待上限（如 30_000）
/// - `jitter_seed`: 用来产生抖动的种子（真实代码用随机数，这里用参数便于测试）
///
/// # 返回
/// 本次应等待的毫秒数
///
/// # 为什么必须有抖动
/// 服务端重启时，**所有客户端会在同一毫秒掉线**。
/// 如果退避时间完全确定，它们会在同一毫秒同时重连——
/// 这就是"惊群"（thundering herd），刚起来的服务端会被自己的客户端打垮。
/// 抖动把重连时间打散，代价只是某些客户端晚几百毫秒。
pub fn backoff_ms(attempt: u32, base_ms: u64, max_ms: u64, jitter_seed: u64) -> u64 {
    // 指数部分：base * 2^attempt
    // 用 saturating 系列防止 attempt 很大时溢出
    let exp = base_ms.saturating_mul(1u64.checked_shl(attempt).unwrap_or(u64::MAX));
    let capped = exp.min(max_ms);

    // 抖动：在 [capped/2, capped] 区间内取值
    // 简单可测的伪随机：种子取模
    let half = capped / 2;
    if half == 0 {
        return capped;
    }
    half + (jitter_seed % half)
}

// ============================================
// 单元测试
// ============================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn rfc6455_accept_vector() {
        // RFC 6455 §1.3 的官方测试向量，必须完全一致
        assert_eq!(
            accept_key("dGhlIHNhbXBsZSBub25jZQ=="),
            "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
        );
    }

    #[test]
    fn encode_decode_roundtrip_short() {
        // 短载荷：7 bit 长度
        let payload = b"hello";
        let raw = encode_frame(true, Opcode::Text, payload, Some([1, 2, 3, 4]));
        // 头部 2 字节 + 掩码键 4 字节 + 载荷 5 字节 = 11
        assert_eq!(raw.len(), 11);

        let (frame, consumed) = decode_frame(&raw, true).unwrap();
        assert_eq!(consumed, raw.len());
        assert!(frame.fin);
        assert_eq!(frame.opcode, Opcode::Text);
        assert_eq!(frame.payload, payload);
    }

    #[test]
    fn encode_decode_roundtrip_medium() {
        // 126..=65535：走 2 字节长度
        let payload = vec![0xABu8; 300];
        let raw = encode_frame(true, Opcode::Binary, &payload, Some([9, 8, 7, 6]));
        // 2 + 2 + 4 + 300
        assert_eq!(raw.len(), 308);
        let (frame, _) = decode_frame(&raw, true).unwrap();
        assert_eq!(frame.payload, payload);
    }

    #[test]
    fn encode_decode_roundtrip_large() {
        // > 65535：走 8 字节长度
        let payload = vec![0x5Au8; 70_000];
        let raw = encode_frame(true, Opcode::Binary, &payload, None);
        // 2 + 8 + 70000
        assert_eq!(raw.len(), 70_010);
        // 服务端发出的帧不带掩码
        let (frame, _) = decode_frame(&raw, false).unwrap();
        assert_eq!(frame.opcode, Opcode::Binary);
        assert_eq!(frame.payload.len(), 70_000);
    }

    #[test]
    fn incomplete_returns_incomplete_not_error() {
        // 只给 1 个字节
        assert_eq!(decode_frame(&[0x81], true), Err(FrameError::Incomplete));
        // 给了头但没有载荷
        let raw = encode_frame(true, Opcode::Text, b"hello", None);
        assert_eq!(decode_frame(&raw[..5], false), Err(FrameError::Incomplete));
    }

    #[test]
    fn client_frame_without_mask_is_rejected() {
        let raw = encode_frame(true, Opcode::Text, b"hi", None);
        assert_eq!(decode_frame(&raw, true), Err(FrameError::MissingMask));
        // 同样的字节，按服务端帧解析就合法
        assert!(decode_frame(&raw, false).is_ok());
    }

    #[test]
    fn reserved_opcode_is_rejected() {
        // 0x3 是保留操作码
        let raw = [0x83u8, 0x00];
        assert_eq!(decode_frame(&raw, true), Err(FrameError::ReservedOpcode(3)));
    }

    #[test]
    fn oversized_payload_is_rejected_before_allocating() {
        // 手工构造一个声称 2 GiB 载荷的头部：0x82 0xFF + u64
        let mut raw = vec![0x82u8, 0xFF];
        raw.extend_from_slice(&(2u64 * 1024 * 1024 * 1024).to_be_bytes());
        // 注意：缓冲区里根本没有任何载荷字节，
        // 但我们是先判大小再等数据，所以立刻返回错误而不是 Incomplete
        assert_eq!(
            decode_frame(&raw, false),
            Err(FrameError::PayloadTooLarge(2 * 1024 * 1024 * 1024))
        );
    }

    #[test]
    fn backoff_is_capped_and_jittered() {
        assert!(backoff_ms(0, 500, 30_000, 0) <= 500);
        assert!(backoff_ms(100, 500, 30_000, 12345) <= 30_000);
        // 抖动区间 [capped/2, capped)
        let v = backoff_ms(4, 500, 30_000, 999);
        assert!(v >= 4_000 && v < 8_000, "got {}", v);
    }
}
