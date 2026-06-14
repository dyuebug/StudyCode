// ============================================
// reqwest 学习 - HTTP 客户端
// ============================================

use reqwest::blocking::Client;
use serde::{Deserialize, Serialize};

// ============================================
// 示例1：基础 GET 请求
// ============================================

fn example1_basic_get() -> Result<(), Box<dyn std::error::Error>> {
    println!("============================================================");
    println!("示例1：基础 GET 请求");
    println!("============================================================\n");

    // 发送 GET 请求
    let response = reqwest::blocking::get("https://httpbin.org/get")?;

    // 获取状态码
    println!("📊 状态码: {}", response.status());

    // 获取响应头
    println!("\n📋 响应头:");
    for (key, value) in response.headers() {
        println!("  {}: {:?}", key, value);
    }

    // 获取响应体
    let body = response.text()?;
    println!("\n📦 响应体:\n{}\n", body);

    Ok(())
}

// ============================================
// 示例2：带参数的 GET 请求
// ============================================

fn example2_get_with_params() -> Result<(), Box<dyn std::error::Error>> {
    println!("============================================================");
    println!("示例2：带参数的 GET 请求");
    println!("============================================================\n");

    let client = Client::new();

    // 方式1：直接在 URL 中
    let url = "https://httpbin.org/get?name=Alice&age=30";
    let response = client.get(url).send()?;
    println!("📍 URL: {}", url);
    println!("📊 状态码: {}\n", response.status());

    // 方式2：使用 query 方法
    let response = client
        .get("https://httpbin.org/get")
        .query(&[("name", "Bob"), ("age", "25")])
        .send()?;

    let body = response.text()?;
    println!("📦 响应体:\n{}\n", body);

    Ok(())
}

// ============================================
// 示例3：POST 请求和 JSON
// ============================================

#[derive(Serialize, Deserialize, Debug)]
struct User {
    name: String,
    email: String,
    age: u32,
}

fn example3_post_json() -> Result<(), Box<dyn std::error::Error>> {
    println!("============================================================");
    println!("示例3：POST 请求和 JSON");
    println!("============================================================\n");

    let client = Client::new();

    // 创建要发送的数据
    let user = User {
        name: String::from("Alice"),
        email: String::from("alice@example.com"),
        age: 30,
    };

    println!("📤 发送数据: {:?}", user);

    // 发送 POST 请求，自动序列化为 JSON
    let response = client
        .post("https://httpbin.org/post")
        .json(&user)
        .send()?;

    println!("📊 状态码: {}", response.status());

    // 解析 JSON 响应
    let body = response.text()?;
    println!("\n📦 响应体:\n{}\n", body);

    Ok(())
}

// ============================================
// 示例4：自定义请求头
// ============================================

fn example4_custom_headers() -> Result<(), Box<dyn std::error::Error>> {
    println!("============================================================");
    println!("示例4：自定义请求头");
    println!("============================================================\n");

    let client = Client::new();

    let response = client
        .get("https://httpbin.org/headers")
        .header("User-Agent", "MyApp/1.0")
        .header("Accept", "application/json")
        .header("X-Custom-Header", "HelloWorld")
        .send()?;

    println!("📊 状态码: {}", response.status());

    let body = response.text()?;
    println!("\n📦 响应体:\n{}\n", body);

    Ok(())
}

// ============================================
// 示例5：处理 JSON 响应
// ============================================

#[derive(Deserialize, Debug)]
struct IpInfo {
    origin: String,
}

fn example5_parse_json() -> Result<(), Box<dyn std::error::Error>> {
    println!("============================================================");
    println!("示例5：处理 JSON 响应");
    println!("============================================================\n");

    let client = Client::new();

    // 发送请求并直接解析为结构体
    let ip_info: IpInfo = client
        .get("https://httpbin.org/ip")
        .send()?
        .json()?;

    println!("📍 你的 IP 地址: {}\n", ip_info.origin);

    Ok(())
}

// ============================================
// 示例6：错误处理
// ============================================

fn example6_error_handling() {
    println!("============================================================");
    println!("示例6：错误处理");
    println!("============================================================\n");

    let client = Client::new();

    // 测试1：404 错误
    match client.get("https://httpbin.org/status/404").send() {
        Ok(response) => {
            println!("✅ 请求成功");
            println!("📊 状态码: {}", response.status());
            if response.status().is_success() {
                println!("✅ 2xx 成功");
            } else if response.status().is_client_error() {
                println!("⚠️  4xx 客户端错误");
            } else if response.status().is_server_error() {
                println!("❌ 5xx 服务器错误");
            }
        }
        Err(e) => {
            eprintln!("❌ 请求失败: {}", e);
        }
    }

    println!();

    // 测试2：超时
    println!("⏱️  测试超时（如果网络慢可能会真的超时）...");
    let result = client
        .get("https://httpbin.org/delay/10")
        .timeout(std::time::Duration::from_secs(2))
        .send();

    match result {
        Ok(_) => println!("✅ 请求成功"),
        Err(e) => println!("⚠️  请求超时或失败: {}", e),
    }

    println!();
}

fn main() {
    println!("\n🌐 reqwest 学习 - HTTP 客户端\n");

    // 示例1：基础 GET
    if let Err(e) = example1_basic_get() {
        eprintln!("❌ 示例1 错误: {}", e);
    }

    // 示例2：带参数的 GET
    if let Err(e) = example2_get_with_params() {
        eprintln!("❌ 示例2 错误: {}", e);
    }

    // 示例3：POST JSON
    if let Err(e) = example3_post_json() {
        eprintln!("❌ 示例3 错误: {}", e);
    }

    // 示例4：自定义请求头
    if let Err(e) = example4_custom_headers() {
        eprintln!("❌ 示例4 错误: {}", e);
    }

    // 示例5：解析 JSON
    if let Err(e) = example5_parse_json() {
        eprintln!("❌ 示例5 错误: {}", e);
    }

    // 示例6：错误处理
    example6_error_handling();

    println!("============================================================");
    println!("✅ 学习完成！");
    println!("============================================================\n");

    println!("💡 关键要点:");
    println!("1. reqwest::blocking::Client 创建客户端");
    println!("2. get()/post()/put()/delete() 发送请求");
    println!("3. .json(&data) 自动序列化 JSON");
    println!("4. .send()?.json::<T>() 自动反序列化");
    println!("5. .query(&params) 添加查询参数");
    println!("6. .header(key, value) 添加请求头");
    println!("7. .timeout() 设置超时");
    println!("8. status() 获取状态码");
    println!("\n🎯 实际应用：API 调用、数据抓取、微服务通信");
    println!("\n⚠️  注意：这些示例需要网络连接才能运行！");
}
