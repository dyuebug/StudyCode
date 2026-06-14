// ============================================
// 简单 HTTP 服务器 - 阶段3：多线程并发
// 功能：使用线程池并发处理多个请求
// ============================================

use http_server::ThreadPool;
use std::fs;
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;
use std::time::Duration;

fn main() {
    // 监听本地 7878 端口
    let listener = TcpListener::bind("127.0.0.1:7878").unwrap();

    // 创建线程池（4个工作线程）
    let pool = ThreadPool::new(4);

    println!("🚀 HTTP 服务器启动成功！");
    println!("📍 监听地址: http://127.0.0.1:7878");
    println!("🔧 线程池大小: 4 个工作线程");
    println!("💡 在浏览器中访问或按 Ctrl+C 停止");
    println!("\n可用路由：");
    println!("  • /         - 首页");
    println!("  • /about    - 关于页面");
    println!("  • /sleep    - 慢请求（5秒延迟）- 测试并发");
    println!("  • 其他      - 404 页面\n");

    // 监听传入的连接
    for stream in listener.incoming() {
        let stream = stream.unwrap();

        // 使用线程池处理连接
        // 每个连接在独立线程中处理，不会阻塞其他连接
        pool.execute(|| {
            handle_connection(stream);
        });
    }
}

fn handle_connection(mut stream: TcpStream) {
    // 1. 读取 HTTP 请求
    let mut buffer = [0; 1024];
    stream.read(&mut buffer).unwrap();

    // 2. 将字节转换为字符串
    let request = String::from_utf8_lossy(&buffer[..]);

    // 3. 解析请求的第一行（请求行）
    let request_line = request.lines().next().unwrap_or("");
    println!("📥 收到请求: {}", request_line);

    // 4. 路由匹配
    let (status_line, filename) = if request_line.starts_with("GET / HTTP/1.1") {
        // 根路径 / -> 返回首页
        ("HTTP/1.1 200 OK", "index.html")
    } else if request_line.starts_with("GET /about HTTP/1.1") {
        // /about 路径 -> 返回关于页面
        ("HTTP/1.1 200 OK", "about.html")
    } else if request_line.starts_with("GET /sleep HTTP/1.1") {
        // /sleep 路径 -> 模拟慢请求（用于测试并发）
        println!("💤 模拟慢请求，等待 5 秒...");
        thread::sleep(Duration::from_secs(5));
        ("HTTP/1.1 200 OK", "index.html")
    } else {
        // 其他路径 -> 返回 404
        ("HTTP/1.1 404 NOT FOUND", "404.html")
    };

    // 5. 读取 HTML 文件
    let contents = fs::read_to_string(filename).unwrap_or_else(|_| {
        String::from("<h1>Error: File not found</h1>")
    });

    // 6. 构造完整的 HTTP 响应
    let length = contents.len();
    let response = format!(
        "{}\r\nContent-Length: {}\r\nContent-Type: text/html; charset=utf-8\r\n\r\n{}",
        status_line, length, contents
    );

    // 7. 发送响应
    stream.write_all(response.as_bytes()).unwrap();
    stream.flush().unwrap();

    println!("📤 响应已发送: {}\n", status_line);
}
