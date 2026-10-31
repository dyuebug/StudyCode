// ============================================
// realtime-chat —— 实时聊天应用
// 日期：2026-11-04（第40天）
// ============================================

/*
【和第 34 天的 ws-basics 差在哪】

ws-basics 只演示"协议层 + 广播"；
realtime-chat 要回答的是**把它做成一个应用**要补什么：

    1. 装配   —— app()/configure() 与 task-manager 同款，main 和 tests 共用
    2. 可观测 —— 在线人数能被 HTTP 查到（否则只能"感觉它在")
    3. 失败模式 —— 慢客户端要被踢掉，而不是拖垮广播
    4. 校验   —— 房间名不是随便什么字符串都能当 key

【WebSocket 为什么能和 HTTP 共用一个端口】
握手本身就是一个 HTTP 请求：`GET /ws/lobby` 带
`Upgrade: websocket` + `Connection: Upgrade` + `Sec-WebSocket-Key`。
服务端回 `101 Switching Protocols`，**之后**这条 TCP 连接上跑的就是
WebSocket 帧，不再是 HTTP 报文。所以不是"另开一个服务"，是"协议升级"。
`actix_ws::handle` 一次做完：校验握手头 → 返回 101 响应 + Session + Stream。
*/

pub mod chat;
pub mod room;

use actix_web::body::BoxBody;
use actix_web::dev::{ServiceFactory, ServiceRequest, ServiceResponse};
use actix_web::{web, App, Error, HttpRequest, HttpResponse};
use serde_json::json;

use room::Rooms;

/// 房间名规则：1~32 个字符，只允许字母、数字、`-`、`_`
///
/// 【为什么要限制】
/// 房间名会进 HashMap 的 key、日志、JSON。放任任意字符串进来，
/// 就是一个"用户可以往你的日志里注入换行"的入口。
/// 白名单校验比转义便宜，也更难写错。
fn valid_room(name: &str) -> bool {
    !name.is_empty()
        && name.chars().count() <= 32
        && name
            .chars()
            .all(|c| c.is_ascii_alphanumeric() || c == '-' || c == '_')
}

/// 构建应用
///
/// `rooms` 由调用方传入（而不是在函数内 `Rooms::new()`）：
/// 必须跨 worker 共享同一份注册表，否则 4 个 worker 会各有 4 个"大厅"，
/// 在线人数永远只看到自己那一个。
pub fn app(
    rooms: Rooms,
    cfg: chat::ChatConfig,
) -> App<
    impl ServiceFactory<
            ServiceRequest,
            Config = (),
            Response = ServiceResponse<BoxBody>,
            Error = Error,
            InitError = (),
        > + 'static,
> {
    App::new()
        .app_data(web::Data::new(rooms))
        .app_data(web::Data::new(cfg))
        // 【为什么这里没有 `.wrap(Logger::default())`】
        // 1) 它不打印：Logger 只是往 `log` 门面里写记录，
        //    没有初始化 env_logger 之类的实现，日志会被直接丢掉。
        //    本项目不引入 log 实现，连接生命周期用 chat.rs 里的 println! 记录，
        //    够用且零依赖。
        // 2) 它还会**改变 body 类型**（包一层 `StreamLog`），
        //    而该类型在 actix-web 里是私有的、外部无法命名，
        //    一旦 wrap 上去，`app()` 的返回类型签名就写不出来。
        //    这是 actix-web 类型系统里最常见的"明明没写错却对不上"。
        .configure(configure)
}

pub fn configure(cfg: &mut web::ServiceConfig) {
    cfg.route("/", web::get().to(index))
        .route("/health", web::get().to(health))
        .route("/rooms", web::get().to(list_rooms))
        .route("/rooms/{room}/members", web::get().to(members))
        // {room} 是路径参数：`/ws/lobby` 里 lobby 就是房间名
        .route("/ws/{room}", web::get().to(ws));
}

/// 聊天页（`include_str!` 把 HTML 编进二进制）
///
/// 好处：部署时只有一个可执行文件，不会出现"程序在跑、静态文件没同步"。
/// 代价：改一行 CSS 也要重新编译。教学项目里这个取舍很划算。
async fn index() -> HttpResponse {
    HttpResponse::Ok()
        .content_type("text/html; charset=utf-8")
        .body(include_str!("../static/chat.html"))
}

async fn health() -> HttpResponse {
    HttpResponse::Ok().json(json!({ "status": "ok" }))
}

/// 列出全部房间及在线数
///
/// 【为什么要有这个接口】
/// WebSocket 的状态在服务端内部，HTTP 是唯一"从外面看得见"的窗口。
/// 没有它，"两个客户端真的在同一个房间里"这句话就无法验证，
/// 只能靠"我觉得它连上了"。
async fn list_rooms(rooms: web::Data<Rooms>) -> HttpResponse {
    let mut out: Vec<_> = rooms
        .snapshot()
        .into_iter()
        .map(|(name, online)| json!({ "room": name, "count": online }))
        .collect();
    // 顺序稳定，便于断言与对比
    out.sort_by(|a, b| a["room"].as_str().cmp(&b["room"].as_str()));
    HttpResponse::Ok().json(json!({ "success": true, "data": out }))
}

async fn members(path: web::Path<String>, rooms: web::Data<Rooms>) -> HttpResponse {
    let name = path.into_inner();
    // 不存在的房间返回 0 而不是 404：
    // "这个房间现在没人"和"这个房间不存在"对调用方是同一件事——
    // 房间本来就是空即消失的概念（本项目里房间对象会保留，见 README 局限）
    let count = rooms.get(&name).map(|r| r.online()).unwrap_or(0);
    HttpResponse::Ok().json(json!({ "success": true, "data": { "room": name, "count": count } }))
}

/// WebSocket 握手
///
/// # 流程
/// 1. 校验房间名（非法 → 400，**不进入**握手）
/// 2. `actix_ws::handle` 完成握手（缺 Upgrade 头 → 400）
/// 3. 加入房间 → 拿到成员编号
/// 4. `spawn` 读写循环，立刻返回 101
///
/// 【为什么第 4 步是 spawn 而不是 await】
/// 握手响应必须**马上**返回，客户端才会认为连接建立。
/// 读写循环要活到连接结束（可能几小时），不能挡住这个响应。
/// 这也是为什么循环里要用 `Session` 而不是共享状态——
/// 连接的全部东西都被 move 进了这个任务。
async fn ws(
    req: HttpRequest,
    body: web::Payload,
    path: web::Path<String>,
    rooms: web::Data<Rooms>,
    cfg: web::Data<chat::ChatConfig>,
) -> Result<HttpResponse, Error> {
    let name = path.into_inner();
    if !valid_room(&name) {
        return Ok(HttpResponse::BadRequest().json(json!({
            "success": false,
            "code": "invalid_room",
            "message": "房间名需 1~32 个字符，只允许字母、数字、- 和 _"
        })));
    }

    // 注意顺序：先 handle 再 join。
    // 如果先 join 后 handle，握手失败（比如没有 Upgrade 头）时，
    // 在线数已经被加上去了，而且没有任何人负责减回来。
    let (response, session, msg_stream) = actix_ws::handle(&req, body)?;

    let room = rooms.get_or_create(&name);
    let member = room.join();
    println!(
        "[WS] 成员 {member} 加入 {}，当前 {}",
        room.name,
        room.online()
    );

    actix_web::rt::spawn(chat::run(session, msg_stream, room, member, *cfg.get_ref()));

    Ok(response)
}
