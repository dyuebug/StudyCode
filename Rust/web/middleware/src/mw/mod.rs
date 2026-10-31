//! 中间件实现集合。
//!
//! 每个中间件都拆成**两个类型**，这是 actix-web 的固定套路：
//!
//! | 类型 | 实现的 trait | 职责 |
//! |---|---|---|
//! | `Logging` / `Auth` | `Transform<S, ServiceRequest>` | 配置 + 工厂：把内层服务包一层 |
//! | `LoggingMiddleware<S>` / `AuthMiddleware<S>` | `Service<ServiceRequest>` | 真正处理请求 |
//!
//! 为什么必须拆开：`Transform` 是"工厂"，`new_transform` 在**应用启动时**每个 worker
//! 调用一次；`Service::call` 是"真正跑请求"，每个请求调用一次。
//! 想清楚"这段代码跑几次"是用对中间件的前提——
//! 把每次请求都要做的事写在 `new_transform` 里，就是典型的错位。

pub mod auth;
pub mod logging;
