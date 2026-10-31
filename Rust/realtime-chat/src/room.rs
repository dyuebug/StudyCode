// ============================================
// room.rs —— 房间与成员注册表
// 日期：2026-11-04（第40天）
// ============================================

/*
【这一层解决什么问题】

WebSocket 连接是"一对一"的，聊天室需要"一发多收"。
第 34 天用 `tokio::sync::broadcast` 做过一次；这次要把它装进
一个**能测、能查在线人数**的结构里。

【为什么用 broadcast，而不是 `Arc<Mutex<Vec<Session>>>`】
后者是 actix-ws 官方示例的写法，可行，但有两个代价：
  1. 发包时**持锁**，所有连接的发包排在同一把锁后面
  2. 慢客户端没有背压信号——你只能等它，或者靠超时硬断

broadcast 给每个订阅者一个独立的 Receiver，慢的那个自己落后，
收的时候拿到 `RecvError::Lagged(n)`：**明确告诉你"你丢了 n 条"**。
本项目的策略是"落后就断开"——聊天室里，一个看不到历史的客户端
继续连着没有意义（而且它会一直拖累缓冲）。

【`Rooms` 用什么锁】
`std::sync::RwLock`，不是 `tokio::sync::RwLock`。
理由：临界区只有一次 `HashMap` 查找/插入，**没有 await**。
用 tokio 的异步锁反而要多付一次调度成本。
⚠️ 前提是绝不跨 await 持锁——本文件里所有持锁代码都是同步的，
拿到 `Arc<Room>` 就立刻释放。
*/

use std::collections::HashMap;
use std::sync::atomic::{AtomicU64, AtomicUsize, Ordering};
use std::sync::{Arc, RwLock};

use tokio::sync::broadcast;

/// 广播环缓冲容量
///
/// = 单个客户端最多能落后多少条消息。
/// 太小：正常抖动也会被判 Lagged；太大：慢客户端占内存更久。
/// 按"消息速率 × 可容忍延迟"估算，聊天室取 64 足够。
pub const BROADCAST_CAPACITY: usize = 64;

/// 一个房间
///
/// `Clone` 是 `Arc` 克隆，很便宜——但通常我们用 `Arc<Room>` 传递。
pub struct Room {
    /// 房间名（日志与 HTTP 响应里用）
    pub name: String,
    /// 广播发送端。它自己可 Clone，Clone 出的仍是同一个频道
    tx: broadcast::Sender<String>,
    /// 当前在线数
    online: AtomicUsize,
    /// 成员编号分配器（房间内递增）
    next_member: AtomicU64,
}

impl Room {
    pub fn new(name: impl Into<String>) -> Self {
        // 先丢掉初始 Receiver：一个订阅者都没有时 publish 返回 0，
        // 这是正常情况，不是错误
        let (tx, _rx) = broadcast::channel(BROADCAST_CAPACITY);
        Self {
            name: name.into(),
            tx,
            online: AtomicUsize::new(0),
            next_member: AtomicU64::new(1),
        }
    }

    /// 新连接调用一次：返回成员编号，并把在线数 +1
    pub fn join(&self) -> u64 {
        self.online.fetch_add(1, Ordering::SeqCst);
        self.next_member.fetch_add(1, Ordering::SeqCst)
    }

    /// 连接结束时调用：在线数 -1（不会减到负数）
    pub fn leave(&self) {
        // `fetch_sub` 会回绕成一个巨大的数；这里用 CAS 循环防止下溢。
        // 多减一次通常意味着"重复调用 leave"，那本身就是 bug，
        // 所以宁可让它卡在 0，也不要让在线数变成 18446744073709551615。
        let _ = self
            .online
            .fetch_update(Ordering::SeqCst, Ordering::SeqCst, |n| n.checked_sub(1));
    }

    pub fn online(&self) -> usize {
        self.online.load(Ordering::SeqCst)
    }

    /// 订阅这个房间的广播
    pub fn subscribe(&self) -> broadcast::Receiver<String> {
        self.tx.subscribe()
    }

    /// 广播一条消息，返回当前订阅者数量
    pub fn publish(&self, msg: impl Into<String>) -> usize {
        // 没有订阅者时 send 返回 Err —— 不是失败，是"没人在听"
        self.tx.send(msg.into()).unwrap_or(0)
    }
}

/// 全部房间的注册表
///
/// 用 `Arc<RwLock<..>>` 而不是 `Arc<Mutex<..>>`：
/// 读（查成员数）远多于写（建房间），RwLock 让读之间不互斥。
#[derive(Clone, Default)]
pub struct Rooms {
    inner: Arc<RwLock<HashMap<String, Arc<Room>>>>,
}

impl Rooms {
    pub fn new() -> Self {
        Self::default()
    }

    /// 取房间，不存在就建
    ///
    /// 【为什么要"读锁先试一次"】
    /// 常见路径是"房间已存在"，先拿读锁能在高并发下避免所有请求
    /// 排在同一把写锁上。只有真的没有时才升级为写锁。
    /// 这两次加锁之间没有 await，所以不存在"释放后状态又变了"的窗口问题
    /// ——就算真的被别人先建了，下面也会覆盖成同一个名字的新 Room；
    /// 为避免这种竞态，写锁内**先查再插**。
    pub fn get_or_create(&self, name: &str) -> Arc<Room> {
        if let Some(room) = self.inner.read().unwrap().get(name) {
            return Arc::clone(room);
        }

        let mut map = self.inner.write().unwrap();
        // 双重检查：从读锁释放到拿到写锁之间，别人可能已经建好了
        if let Some(room) = map.get(name) {
            return Arc::clone(room);
        }
        let room = Arc::new(Room::new(name));
        map.insert(name.to_string(), Arc::clone(&room));
        room
    }

    /// 只查不建：HTTP 查在线人数用这个，不能因为查询就把房间创建出来
    pub fn get(&self, name: &str) -> Option<Arc<Room>> {
        self.inner.read().unwrap().get(name).map(Arc::clone)
    }

    pub fn room_count(&self) -> usize {
        self.inner.read().unwrap().len()
    }

    /// 所有房间的 (名字, 在线数)
    ///
    /// 返回的是**快照**而不是引用：调用方拿到的数据在返回那一刻就固定了。
    /// 如果返回 `&HashMap`，调用方就被迫在持锁期间处理它——
    /// 而 HTTP handler 里后面要 await 序列化，跨 await 持锁是死锁的经典来源。
    pub fn snapshot(&self) -> Vec<(String, usize)> {
        self.inner
            .read()
            .unwrap()
            .iter()
            .map(|(name, room)| (name.clone(), room.online()))
            .collect()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn join_and_leave_change_online() {
        let room = Room::new("t");
        assert_eq!(room.online(), 0);
        let a = room.join();
        let b = room.join();
        assert_ne!(a, b, "成员编号必须唯一");
        assert_eq!(room.online(), 2);
        room.leave();
        assert_eq!(room.online(), 1);
        room.leave();
        assert_eq!(room.online(), 0);
        room.leave();
        assert_eq!(room.online(), 0, "多减一次不应下溢");
    }

    #[test]
    fn get_or_create_is_shared() {
        let rooms = Rooms::new();
        let a = rooms.get_or_create("lobby");
        let b = rooms.get_or_create("lobby");
        assert!(Arc::ptr_eq(&a, &b), "同名房间必须返回同一个实例");
        assert_eq!(rooms.room_count(), 1);
        assert!(rooms.get("nope").is_none(), "get 不应创建房间");
    }

    #[tokio::test]
    async fn publish_reaches_subscriber() {
        let room = Room::new("t");
        let mut rx = room.subscribe();
        assert_eq!(room.publish("hi"), 1);
        assert_eq!(rx.recv().await.unwrap(), "hi");
    }
}
