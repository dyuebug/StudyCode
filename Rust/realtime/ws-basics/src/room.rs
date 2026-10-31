// ============================================
// room.rs —— 广播房间（tokio::sync::broadcast）
// 日期：2026-10-29（第34天）
// ============================================
//
// 【这一层解决什么问题】
// WebSocket 连接是"一对一"的：一个 Session 对应一个客户端。
// 但聊天室需要"一发多收"。中间缺的就是**广播**。
//
// 【为什么不用 Mutex<Vec<Session>>】
// actix-ws 官方示例用的是 `Arc<Mutex<Vec<Session>>>` + `FuturesUnordered`：
// 它可行，而且没有额外缓冲，但有两个代价：
//   1. 发送期间**持锁**——所有连接的发包排在这一把锁后面
//   2. 慢客户端没有背压信号——只能等它，或者靠超时断开
//
// `tokio::sync::broadcast` 换了个思路：
//   每个连接持有自己的 Receiver，发送方只往 ring buffer 里写一次。
//   慢的那个连接自己落后，收的时候拿到 `RecvError::Lagged(n)` ——
//   **明确告诉你"你丢了 n 条"**，由它自己决定是追赶还是断开。
//
// 代价：每条消息在内存里存满 capacity 份之前不会释放。
// capacity 就是"最多容忍多少条未读"。

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use std::time::{Duration, Instant};

use tokio::sync::broadcast;

/// 广播房间
///
/// `Clone` 很便宜：内部全是 `Arc`。
#[derive(Clone)]
pub struct Room {
    /// 发送端。`broadcast::Sender` 本身可 Clone，且 Clone 出的仍是同一个频道
    tx: broadcast::Sender<String>,
    /// 在线连接数（由连接任务自行加减）
    online: Arc<AtomicUsize>,
    /// 房间名，只用于日志
    name: String,
}

impl Room {
    /// 创建房间
    ///
    /// # 参数
    /// - `name`: 房间名（日志用）
    /// - `capacity`: ring buffer 容量，即"单个最慢客户端最多落后多少条"
    ///
    /// ## capacity 怎么选
    /// - 太小：正常抖动的客户端也会被判定 Lagged，频繁丢消息
    /// - 太大：慢客户端会占住内存，且它追上来时要吐出一大堆过期消息
    /// - 经验值：按"消息速率 × 可容忍延迟秒数"估算。
    ///   每秒 10 条、容忍 5 秒 → 50，取整到 64
    pub fn new(name: impl Into<String>, capacity: usize) -> Self {
        // broadcast::channel 返回 (Sender, Receiver)。
        // 这里**丢掉**初始 Receiver —— 一个订阅者都没有时
        // publish() 会返回 0，这是正常情况，不是错误
        let (tx, _rx) = broadcast::channel(capacity);
        Self {
            tx,
            online: Arc::new(AtomicUsize::new(0)),
            name: name.into(),
        }
    }

    /// 订阅：每个新连接调用一次
    pub fn subscribe(&self) -> broadcast::Receiver<String> {
        self.tx.subscribe()
    }

    /// 广播一条消息
    ///
    /// # 返回
    /// 当前正在等待接收的订阅者数量。
    /// 返回 0 表示"没有人在听"——不是错误，不报。
    pub fn publish(&self, msg: impl Into<String>) -> usize {
        self.tx.send(msg.into()).unwrap_or(0)
    }

    /// 连接建立时调用一次
    pub fn join(&self) -> usize {
        self.online.fetch_add(1, Ordering::Relaxed) + 1
    }

    /// 连接关闭时调用一次
    ///
    /// ⚠️ **必须保证只调用一次**。连接任务里可能有多个退出路径
    /// （收到 Close 帧、读取出错、心跳超时、服务端关闭），
    /// 每条路径都要能走到这里，否则在线数会越跑越偏。
    /// 惯用做法：让"计数守卫"在 Drop 时减一，见 `OnlineGuard`。
    pub fn leave(&self) -> usize {
        self.online.fetch_sub(1, Ordering::Relaxed).saturating_sub(1)
    }

    /// 当前在线数
    pub fn online(&self) -> usize {
        self.online.load(Ordering::Relaxed)
    }

    /// 房间名
    pub fn name(&self) -> &str {
        &self.name
    }
}

/// 在线计数的 RAII 守卫
///
/// 构造时 +1，Drop 时 -1。
/// 这样无论连接任务从哪条路径退出（返回、`?`、panic 展开），
/// 计数都会被正确回收。
///
/// **这是"用类型系统保证清理"的标准手法**——和 `MutexGuard` 一个思路。
pub struct OnlineGuard {
    room: Room,
}

impl OnlineGuard {
    /// 进入房间
    pub fn enter(room: Room) -> Self {
        let n = room.join();
        println!("[{}] 连接进入，在线 {}", room.name(), n);
        Self { room }
    }
}

impl Drop for OnlineGuard {
    fn drop(&mut self) {
        let n = self.room.leave();
        println!("[{}] 连接离开，在线 {}", self.room.name(), n);
    }
}

// --------------------------------------------
// 心跳时间参数（集中定义，便于调）
// --------------------------------------------

/// 多久发一次 ping
pub const PING_INTERVAL: Duration = Duration::from_secs(5);

/// 多久没收到客户端任何数据就判定连接已死
///
/// ⚠️ 必须**大于** PING_INTERVAL 的两倍。
/// 否则网络正常抖动一次（丢一个 ping）就会被误杀。
/// 这里 5s 发一次、10s 判死，刚好容忍丢一次。
///
/// 交付实现里只用这一个阈值（`Alive::elapsed() > IDLE_TIMEOUT`），
/// 不再单独定义"读超时"——两个阈值会造成"到底按哪个算"的歧义。
pub const IDLE_TIMEOUT: Duration = Duration::from_secs(10);

/// 记录"最后一次收到客户端数据"的时刻
#[derive(Debug, Clone)]
pub struct Alive {
    at: Arc<std::sync::Mutex<Instant>>,
}

impl Alive {
    /// 新建一个"刚刚还活着"的标记
    pub fn new() -> Self {
        Self {
            at: Arc::new(std::sync::Mutex::new(Instant::now())),
        }
    }

    /// 收到任何客户端数据时打一次点
    pub fn touch(&self) {
        *self.at.lock().expect("Alive 锁被毒化") = Instant::now();
    }

    /// 距上次打点过了多久
    pub fn elapsed(&self) -> Duration {
        self.at.lock().expect("Alive 锁被毒化").elapsed()
    }
}

impl Default for Alive {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test]
    async fn publish_reaches_all_subscribers() {
        let room = Room::new("t", 16);
        let mut a = room.subscribe();
        let mut b = room.subscribe();

        let n = room.publish("hi");
        assert_eq!(n, 2);
        assert_eq!(a.recv().await.unwrap(), "hi");
        assert_eq!(b.recv().await.unwrap(), "hi");
    }

    #[tokio::test]
    async fn publish_without_subscribers_returns_zero() {
        let room = Room::new("t", 16);
        assert_eq!(room.publish("nobody"), 0);
    }

    #[tokio::test]
    async fn slow_subscriber_gets_lagged() {
        let room = Room::new("t", 2); // 容量只有 2
        let mut slow = room.subscribe();

        // 灌 5 条，慢订阅者一条都没读
        for i in 0..5 {
            room.publish(format!("m{}", i));
        }

        // 读到的不是 "m0"，而是 Lagged(3) —— 前 3 条已被覆盖
        match slow.recv().await {
            Err(broadcast::error::RecvError::Lagged(n)) => assert_eq!(n, 3),
            other => panic!("期望 Lagged，实际 {:?}", other),
        }
        // ⚠️ 注意：Lagged 之后拿到的**不是**最新一条，而是缓冲区里**最旧**的那条。
        // 容量 2、发了 5 条 → 残留 m3、m4，所以先读到 m3。
        // 这点容易记错：Lagged 只告诉你"跳过了几条"，不会自动跳到队尾。
        assert_eq!(slow.recv().await.unwrap(), "m3");
        assert_eq!(slow.recv().await.unwrap(), "m4");
    }

    #[test]
    fn online_guard_is_balanced() {
        let room = Room::new("t", 4);
        assert_eq!(room.online(), 0);
        {
            let _g1 = OnlineGuard::enter(room.clone());
            let _g2 = OnlineGuard::enter(room.clone());
            assert_eq!(room.online(), 2);
        }
        // 两个守卫都已 Drop
        assert_eq!(room.online(), 0);
    }
}
