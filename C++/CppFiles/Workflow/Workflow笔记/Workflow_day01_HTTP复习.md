
## 第二周内容 服务端
![[Pasted image 20240720153918.png|300]]

### HTTP是什么

HTTP（HyperText Transfer Protocol，超文本传输协议）是一个用于在客户端和服务器之间传输超文本（如HTML）的应用层协议。它是万维网（WWW）的基础协议，通过它，Web浏览器能够与Web服务器通信，以请求和传递网页、图片、视频等资源。

### 为什么用HTTP

HTTP之所以被广泛使用，主要是因为以下几个原因：

1. **简洁性和易用性**：
    
    - HTTP协议简单易懂，容易实现。它使用明文格式，报文结构清晰，开发和调试都相对容易。
2. **无状态特性**：
    
    - 每个HTTP请求都是独立的，与之前或之后的请求没有直接关系。这种无状态特性简化了服务器的设计和实现，使得服务器能够更好地扩展和维护。
3. **灵活性和扩展性**：
    
    - HTTP协议允许传输任意类型的数据对象（如文本、图像、视频等），并通过MIME类型来标识数据类型。HTTP/1.1及后续版本（如HTTP/2、HTTP/3）增加了许多扩展功能，进一步提高了性能和效率。
4. **广泛兼容性**：
    
    - HTTP协议被广泛支持，几乎所有的Web浏览器、服务器和开发框架都支持HTTP协议，使得它成为开发Web应用和服务的标准选择。
5. **安全性（HTTPS）**：
    
    - 虽然HTTP本身是无状态且明文传输，但通过结合TLS（传输层安全协议），可以实现HTTPS（安全超文本传输协议），提供数据加密和身份验证功能，保障数据传输的安全性。

### 怎么用HTTP

使用HTTP协议的基本步骤如下：

1. **客户端发送HTTP请求**：
    
    - 客户端（通常是Web浏览器）向服务器发送HTTP请求，请求特定的资源（如网页、图片等）。
    - HTTP请求报文包含请求行、请求头部、空行和请求体（可选）。
2. **服务器处理请求并发送HTTP响应**：
    
    - 服务器接收到请求后，处理请求并生成响应。
    - HTTP响应报文包含状态行、响应头部、空行和响应体（可选）。

#### HTTP请求报文示例

```
GET /index.html HTTP/1.1
Host: www.example.com
User-Agent: Mozilla/5.0
Accept: text/html
```

- **请求行**：`GET /index.html HTTP/1.1`
    - 请求方法：GET
    - 请求URI：/index.html
    - HTTP版本：HTTP/1.1
- **请求头部**：
    - Host: [www.example.com](http://www.example.com)
    - User-Agent: Mozilla/5.0
    - Accept: text/html

#### HTTP响应报文示例

```
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 137

<html>
<head><title>Example</title></head>
<body>
<h1>Hello, World!</h1>
</body>
</html>
```

- **状态行**：`HTTP/1.1 200 OK`
    - HTTP版本：HTTP/1.1
    - 状态码：200
    - 状态短语：OK
- **响应头部**：
    - Content-Type: text/html
    - Content-Length: 137
- **响应体**：
    - HTML内容

### HTTP的常用方法

1. **GET**：请求指定的资源，通常用于获取数据。
2. **POST**：提交数据给服务器，通常用于表单提交或上传文件。
3. **PUT**：更新或创建资源。
4. **DELETE**：删除指定的资源。
5. **HEAD**：与GET类似，但不返回响应体，通常用于获取报文头部信息。
6. **OPTIONS**：返回服务器支持的HTTP方法。
7. **PATCH**：对资源进行部分修改。

### HTTP的常用状态码

#### 1xx 信息响应

1. **100 Continue**：继续请求。
2. **101 Switching Protocols**：协议切换。

#### 2xx 成功

1. **200 OK**：请求成功。
2. **201 Created**：资源已创建。
3. **204 No Content**：请求成功，但无内容返回。

#### 3xx 重定向

1. **301 Moved Permanently**：资源已永久移动。
2. **302 Found**：临时重定向。
3. **304 Not Modified**：资源未修改。

#### 4xx 客户端错误

1. **400 Bad Request**：请求无效。
2. **401 Unauthorized**：需要身份验证。
3. **403 Forbidden**：服务器拒绝请求。
4. **404 Not Found**：资源不存在。

#### 5xx 服务器错误

1. **500 Internal Server Error**：服务器内部错误。
2. **501 Not Implemented**：服务器不支持请求方法。
3. **502 Bad Gateway**：网关错误。
4. **503 Service Unavailable**：服务不可用。
5. **504 Gateway Timeout**：网关超时。

### 总结

HTTP是一个用于传输超文本数据的应用层协议，它的简洁性、无状态特性、灵活性、广泛兼容性和通过HTTPS提供的安全性使得它成为Web应用和服务的基础。通过理解HTTP的工作原理、请求和响应报文的组成、常用方法和状态码，可以更好地开发和调试Web应用。

## HTTP

![[Pasted image 20240720153335.png|400]]
### 无状态性
![[Pasted image 20240720153356.png|400]]
### 假如有一天，负载特别大
![[Pasted image 20240720153423.png|400]]

#### 让状态转移
![[Pasted image 20240720153509.png|400]]

