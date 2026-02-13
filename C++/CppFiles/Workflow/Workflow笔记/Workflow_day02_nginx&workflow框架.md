## URL
![[Pasted image 20240724152706.png]]
### URL 是什么

**URL**（Uniform Resource Locator），即统一资源定位符，是一种标准化的地址，用于在互联网上唯一标识资源。URL常被称为"网页地址"或"链接"。一个完整的URL通常包含以下几个部分：

1. **协议（scheme）**：指定如何访问资源的方式，如`http`、`https`、`ftp`等。例如，`http://`表示使用HTTP协议。
2. **域名（domain）或IP地址**：标识资源所在的服务器。例如，`www.example.com`或`192.0.2.1`。
3. **路径（path）**：指定资源在服务器上的位置。例如，`/path/to/resource`。
4. **查询参数（query）**：提供额外的参数信息，以`?`开始，如`?key=value`。
5. **片段标识符（fragment）**：以`#`开头，指定资源内的某个位置或部分。

一个典型的URL示例为：

```C++
https://www.example.com/path/to/page?query=parameter#section
```


### 为什么用 URL

URL在互联网上的使用非常广泛，其主要原因有以下几点：

1. **资源定位**：URL提供了一种标准化的方式来标识和定位互联网上的资源，使用户能够轻松地找到和访问所需的信息。
    
2. **跨平台访问**：无论用户使用什么操作系统或设备，URL都能在各种浏览器和应用程序中统一使用，确保资源的可访问性。
    
3. **连接和导航**：URL用于创建超链接，允许用户在网页之间快速导航。这是万维网（WWW）结构的基础，使得信息可以互联互通。
    
4. **数据传递**：在Web应用中，URL不仅用于定位资源，还用于传递参数和数据。例如，在搜索引擎查询或表单提交时，URL中的查询参数可以携带用户输入的信息。
    
5. **安全性**：通过使用`https`协议，URL可以确保数据在传输过程中经过加密，保护用户的隐私和安全。
    
URL是互联网应用的基础组成部分之一，支持网络浏览、数据交换和在线服务的实现。

## 请求和响应的格式
![[Pasted image 20240724152746.png]]

### HTTP 请求格式

HTTP请求由以下几个部分组成：

1. **请求行（Request Line）**：包含请求方法、请求目标（URI），以及HTTP版本。
    
    - 请求方法：常见的有`GET`、`POST`、`PUT`、`DELETE`等，表示客户端希望在资源上执行的操作。
    - 请求目标：资源的路径或完整URL。
    - HTTP版本：如`HTTP/1.1`或`HTTP/2.0`。
2. **请求头（Headers）**：包含多个键值对，提供请求的额外信息，如客户端的身份、请求内容类型等。
    
    - 常见头部如`Host`（主机），`User-Agent`（用户代理），`Accept`（可接受的媒体类型），`Content-Type`（内容类型）。
3. **空行**：分隔请求头和请求体的空行。
    
4. **请求体（Body）**：可选部分，通常用于`POST`或`PUT`请求，包含需要发送到服务器的数据。
    

**示例请求：**

```VBNET
GET /index.html HTTP/1.1
Host: www.example.com
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
Accept: text/html,application/xhtml+xml
```
### HTTP 响应格式

HTTP响应也由几个部分组成：

1. **状态行（Status Line）**：包含HTTP版本、状态码和状态描述。
    
    - 状态码：三位数字代码表示请求的处理结果，如`200`（成功）、`404`（未找到）、`500`（服务器错误）。
    - 状态描述：对状态码的简短说明。
2. **响应头（Headers）**：多个键值对，提供响应的额外信息，如内容类型、服务器信息等。
    
    - 常见头部如`Content-Type`（内容类型），`Content-Length`（内容长度），`Set-Cookie`（设置Cookie）。
3. **空行**：分隔响应头和响应体的空行。
    
4. **响应体（Body）**：包含实际的资源数据，如HTML文档、图像、JSON数据等。
    

**示例响应：**

```PHP
HTTP/1.1 200 OK
Content-Type: text/html; charset=UTF-8
Content-Length: 305

<!DOCTYPE html>
<html>
<head>
    <title>Example Page</title>
</head>
<body>
    <h1>Welcome to Example.com!</h1>
</body>
</html>
```
请求和响应的结构化格式确保客户端和服务器之间的通信能够清晰、准确地进行，从而实现有效的信息交换

## 常见的HTTP客户端
![[Pasted image 20240724153029.png|400]]
### postman
![[Pasted image 20240724153100.png]]
### 客户端可以在哪些位置携带
![[Pasted image 20240724153127.png|400]]
### 目标 让HTTP接口用起来像函数调用
![[Pasted image 20240724153156.png|450]]
### 增和改有什么区别？
![[Pasted image 20240724153234.png|450]]
## 使用HTTP服务端的三种方案
![[Pasted image 20240724153327.png]]
### nginx vs apache httpd
![[Pasted image 20240724153351.png]]
Nginx和Apache HTTP Server（通常简称为Apache）是两种流行的Web服务器软件。它们都被广泛用于托管网站和Web应用，但在架构、性能、资源使用等方面有显著差异。

#### 1. 架构

- **Nginx**：
    
    - **事件驱动架构**：Nginx采用异步、事件驱动的架构，这使得它能够高效处理大量并发连接，通常用于反向代理、负载均衡、以及静态内容的分发。
    - **多进程/多线程**：Nginx通过多个工作进程处理请求，每个工作进程可以处理多个连接，这减少了资源开销。
- **Apache**：
    
    - **进程驱动架构**：Apache主要使用基于进程的模型，每个请求通常由单独的进程或线程处理。这种架构的灵活性高，但在处理大量并发连接时，可能会导致较高的资源消耗。
    - **多种MPM（多处理模块）**：Apache支持多种MPM，如Prefork、Worker和Event，允许管理员选择不同的并发处理模型，以优化性能。

#### 2. 性能

- **Nginx**：
    
    - 优于Apache的性能尤其明显在处理静态内容和高并发情况下。由于其事件驱动架构，Nginx能够高效地处理成千上万的并发连接，而不会显著增加资源消耗。
- **Apache**：
    
    - 在处理动态内容时，特别是与特定的应用集成（如PHP-FPM、mod_perl）时，Apache的性能可以非常优秀。然而，在高并发场景下，Apache的资源使用效率通常不如Nginx。

#### 3. 资源使用

- **Nginx**：
    
    - Nginx以轻量和低资源使用著称。即使在高流量环境下，它的内存和CPU占用率仍保持较低。
- **Apache**：
    
    - Apache的资源消耗较大，尤其是在高并发的情况下，每个连接的进程或线程模型可能导致高内存和CPU使用。

#### 4. 配置与扩展性

- **Nginx**：
    
    - 配置文件采用简洁的配置语法，虽然相对简单，但灵活性可能不如Apache。
    - Nginx模块在编译时静态绑定，无法像Apache那样动态加载。尽管如此，Nginx有丰富的第三方模块支持。
- **Apache**：
    
    - 配置语法详细且灵活，支持复杂的配置需求。其模块可以动态加载和卸载，便于扩展功能。
    - 拥有广泛的模块库，涵盖了认证、安全性、日志等多方面的功能。

#### 5. 用例与应用场景

- **Nginx**：
    
    - 常用于反向代理、负载均衡、静态内容服务和API网关。其高效的连接处理能力使得它在需要处理大量并发请求的场景下表现出色。
- **Apache**：
    
    - 适用于需要丰富模块支持和复杂配置的环境，如虚拟主机、多用户环境和需要特定功能扩展的Web服务。Apache也是传统上在Linux服务器上的标准Web服务器。

#### 总结

Nginx和Apache各有优势和应用场景的偏好。Nginx以其高效的资源使用和并发处理能力在静态内容和反向代理服务中占有优势，而Apache以其灵活的模块支持和强大的功能适用于需要复杂配置和功能扩展的场景。选择哪种服务器软件应根据具体的应用需求和服务器环境来决定。

### 事件驱动模型的坏处
![[Pasted image 20240724153610.png|400]]
事件驱动模型（Event-Driven Model）是一种处理并发的编程模式，常用于Web服务器和图形用户界面应用中。虽然它有许多优点，如高效的资源使用和良好的并发处理能力，但也有一些缺点和挑战：

#### 1. **编程复杂性**

事件驱动模型的编程模式通常涉及回调函数、事件循环和状态管理，这可能使代码复杂且难以维护。开发者需要处理多种事件来源和异步操作，这对设计和调试提出了更高的要求。

#### 2. **调试困难**

由于事件驱动模型中的操作通常是异步的，调试和追踪问题变得更加困难。传统的堆栈跟踪工具可能不足以捕捉异步调用链中的问题。此外，问题可能发生在事件处理的某个时刻，这使得定位和解决错误更加复杂。

#### 3. **可预测性**

事件驱动模型的执行顺序依赖于事件的到达时间和处理时间，这可能导致系统行为的不确定性。尤其是在处理复杂的并发场景时，预测系统的行为可能变得困难。

#### 4. **回调地狱**

在使用回调函数时，嵌套的回调容易导致所谓的“回调地狱”（Callback Hell），即代码结构深层嵌套，难以阅读和维护。虽然现代语言和框架提供了诸如Promise、async/await等机制来缓解这一问题，但在一些环境中，这仍然是一个挑战。

#### 5. **性能瓶颈**

尽管事件驱动模型在处理大量小任务时表现出色，但它对长时间运行的任务不太适用。如果某个事件处理程序执行时间过长，可能会阻塞事件循环，导致系统响应变慢。这种情况在实时系统中尤为严重，需要小心管理事件处理的复杂度和时间。

#### 6. **资源管理**

事件驱动模型通常需要手动管理资源（如文件描述符、网络连接等），特别是在高并发情况下。错误的资源管理可能导致资源泄漏或资源枯竭，影响系统的稳定性和性能。

尽管事件驱动模型存在上述缺点，它仍然是构建高效并发系统的常用方法。开发者需要平衡其优缺点，根据具体的应用场景和需求来决定是否使用这种模型。

### 适用场景
![[Pasted image 20240724153853.png|450]]
### Nginx配置
![[Pasted image 20240724153939.png]]

#### 查看系统已经存在的nginx进程
![[Pasted image 20240724154005.png]]
#### 启动一个nginx进程
![[Pasted image 20240724154027.png]]
```BASH
	nginx -c /etc/nginx/nginx.conf         //启动
	nginx -s quit -c /etc/nginx/nginx.conf //重启
```
![[Pasted image 20240724154250.png]]

nginx.conf
```bash
worker_processes  1;
user root;
events {
    worker_connections  1024;
}
http {
    include       mime.types;
    default_type  application/octet-stream;
    sendfile        on;
    keepalive_timeout  65;
    server {
        listen       80;
        server_name  localhost;
        location / {
            #root   html;
            root /home/liao/reference;
            index  index.html index.htm;
        }
        error_page   500 502 503 504  /50x.html;
        location = /50x.html {
            root   html;
        }
    }
}
```

mime.types
```

types {
    text/html                             html htm shtml;
    text/css                              css;
    text/xml                              xml;
    image/gif                             gif;
    image/jpeg                            jpeg jpg;
    application/javascript                js;
    application/atom+xml                  atom;
    application/rss+xml                   rss;

    text/mathml                           mml;
    text/plain                            txt;
    text/vnd.sun.j2me.app-descriptor      jad;
    text/vnd.wap.wml                      wml;
    text/x-component                      htc;

    image/png                             png;
    image/tiff                            tif tiff;
    image/vnd.wap.wbmp                    wbmp;
    image/x-icon                          ico;
    image/x-jng                           jng;
    image/x-ms-bmp                        bmp;
    image/svg+xml                         svg svgz;
    image/webp                            webp;

    application/font-woff                 woff;
    application/java-archive              jar war ear;
    application/json                      json;
    application/mac-binhex40              hqx;
    application/msword                    doc;
    application/pdf                       pdf;
    application/postscript                ps eps ai;
    application/rtf                       rtf;
    application/vnd.apple.mpegurl         m3u8;
    application/vnd.ms-excel              xls;
    application/vnd.ms-fontobject         eot;
    application/vnd.ms-powerpoint         ppt;
    application/vnd.wap.wmlc              wmlc;
    application/vnd.google-earth.kml+xml  kml;
    application/vnd.google-earth.kmz      kmz;
    application/x-7z-compressed           7z;
    application/x-cocoa                   cco;
    application/x-java-archive-diff       jardiff;
    application/x-java-jnlp-file          jnlp;
    application/x-makeself                run;
    application/x-perl                    pl pm;
    application/x-pilot                   prc pdb;
    application/x-rar-compressed          rar;
    application/x-redhat-package-manager  rpm;
    application/x-sea                     sea;
    application/x-shockwave-flash         swf;
    application/x-stuffit                 sit;
    application/x-tcl                     tcl tk;
    application/x-x509-ca-cert            der pem crt;
    application/x-xpinstall               xpi;
    application/xhtml+xml                 xhtml;
    application/xspf+xml                  xspf;
    application/zip                       zip;

    application/octet-stream              bin exe dll;
    application/octet-stream              deb;
    application/octet-stream              dmg;
    application/octet-stream              iso img;
    application/octet-stream              msi msp msm;

    application/vnd.openxmlformats-officedocument.wordprocessingml.document    docx;
    application/vnd.openxmlformats-officedocument.spreadsheetml.sheet          xlsx;
    application/vnd.openxmlformats-officedocument.presentationml.presentation  pptx;

    audio/midi                            mid midi kar;
    audio/mpeg                            mp3;
    audio/ogg                             ogg;
    audio/x-m4a                           m4a;
    audio/x-realaudio                     ra;

    video/3gpp                            3gpp 3gp;
    video/mp2t                            ts;
    video/mp4                             mp4;
    video/mpeg                            mpeg mpg;
    video/quicktime                       mov;
    video/webm                            webm;
    video/x-flv                           flv;
    video/x-m4v                           m4v;
    video/x-mng                           mng;
    video/x-ms-asf                        asx asf;
    video/x-ms-wmv                        wmv;
    video/x-msvideo                       avi;
}
```

#### 重启nginx服务端
![[Pasted image 20240724154542.png]]
```
nginx -s quit -c /etc/nginx/nginx.conf
```

#### root指令的意义部署一个静态资源服务器
![[Pasted image 20240724154646.png]]

#### 部署一个产品文档服务器
![[Pasted image 20240724154820.png]]

改root后路径就行

### 网盘系统
![[Pasted image 20240724154911.png]]

### 反向代理和负载均衡
![[Pasted image 20240724154941.png]]
#### 反向代理（Reverse Proxy）

**是什么：** 反向代理是一种服务器，它位于客户端和服务器之间，代理客户端的请求并将其转发给后端服务器。反向代理服务器接收来自客户端的请求，然后将这些请求转发到后端的多个服务器（即原服务器）。从客户端的角度看，它与普通的目标服务器没有区别，因为它们只需要与反向代理服务器进行交互。

**为什么用：**

1. **安全性**：反向代理可以隐藏后端服务器的实际IP地址，防止直接的恶意攻击。它可以充当防火墙的一部分，保护内部网络的安全。
2. **SSL终止**：反向代理服务器可以处理SSL加密和解密任务，这样后端服务器就无需处理这些开销较大的操作，从而提高效率。
3. **缓存**：反向代理可以缓存静态内容，减少后端服务器的负载，提高响应速度。
4. **内容压缩**：可以在数据传输前对内容进行压缩，减少带宽使用。
5. **负载均衡**：反向代理常与负载均衡功能结合使用，将请求分配给多个后端服务器。

#### 负载均衡（Load Balancing）

**是什么：** 负载均衡是一种技术，通过将网络流量或计算请求分发到多个服务器上，以便最有效地利用资源，提高系统的可靠性和性能。负载均衡器是实现这一技术的核心组件。

**为什么用：**

1. **提高可用性**：通过多台服务器来提供服务，即使其中一台服务器发生故障，其他服务器仍能继续提供服务，确保系统的高可用性。
2. **提升性能**：负载均衡可以分配流量到多台服务器，避免单点瓶颈，使系统能够处理更多的并发请求，提高整体响应速度。
3. **扩展性**：系统可以根据需求增加或减少服务器数量，从而轻松扩展。负载均衡器动态地调整流量分配，确保新加入的服务器能够立即开始工作。
4. **健康检查**：负载均衡器可以对后端服务器进行健康检查，确保请求只被分配到健康的服务器，避免将流量引导到故障节点。
5. **流量管理**：负载均衡器可以根据地理位置、服务器负载等策略进行流量分配，实现更细粒度的流量管理。

#### 总结

**反向代理**和**负载均衡**是现代网络架构中常见的技术，用于提升系统的安全性、性能、可靠性和可扩展性。反向代理通过隐藏后端服务器、处理SSL、缓存和压缩内容来优化用户体验和安全性。负载均衡则通过分配流量来防止单点故障，提升系统性能，确保资源的最佳利用。这两者常常结合使用，构建高效且稳健的网络服务。

### 常见的负载均衡策略
![[Pasted image 20240724155408.png]]
负载均衡策略是指负载均衡器如何决定将传入的流量分配到后端服务器的方式。选择合适的负载均衡策略可以优化资源使用，提高系统性能和可靠性。以下是一些常见的负载均衡策略：

#### 1. **轮询法（Round Robin）**

**描述**：轮询法是最简单的负载均衡策略，负载均衡器按照顺序将请求依次分配给每个服务器。这种方法假设所有服务器的能力相同，且每个请求的处理时间大致相同。

**优点**：

- 简单易实现。
- 对于请求量较低且服务器性能相似的场景，效果较好。

**缺点**：

- 无法考虑服务器的实际负载和处理能力，可能导致性能不均衡。

#### 2. **加权轮询法（Weighted Round Robin）**

**描述**：加权轮询法是轮询法的改进版本。每台服务器被分配一个权重（Weight），权重越高的服务器会处理更多的请求。这样可以根据服务器的处理能力分配负载。

**优点**：

- 考虑了服务器的处理能力，能够更均匀地分配负载。

**缺点**：

- 仍然忽略了服务器当前的负载情况。

#### 3. **最小连接数法（Least Connections）**

**描述**：最小连接数法将新请求分配给当前活动连接数最少的服务器。这种方法适用于会话时间较长的情况，有助于平衡长时间连接的负载。

**优点**：

- 动态调整，适合会话长度不一的场景。
- 能够在负载波动时更均匀地分配请求。

**缺点**：

- 需要实时监控每台服务器的连接数。

#### 4. **加权最小连接数法（Weighted Least Connections）**

**描述**：结合了加权轮询法和最小连接数法的优点。服务器根据其权重和当前连接数进行负载分配，权重较大的服务器即使连接数较多，也可能会接收到更多请求。

**优点**：

- 更加精确地平衡负载，考虑了服务器的处理能力和当前负载。

**缺点**：

- 实现和配置较为复杂。

#### 5. **IP哈希法（IP Hash）**

**描述**：IP哈希法使用请求的来源IP地址（或者客户端和服务器的组合IP）进行哈希运算，将结果映射到特定的服务器。这样同一IP地址的请求总是被分配到同一台服务器。

**优点**：

- 对于需要保持会话状态的应用（如在线购物车），能够保证同一用户的请求被分配到同一服务器。

**缺点**：

- 如果有大量请求来自相同IP地址，可能导致某些服务器过载。

#### 6. **URL哈希法（URL Hash）**

**描述**：与IP哈希法类似，URL哈希法根据请求的URL进行哈希，将相同URL的请求分配到同一台服务器。常用于内容分发网络（CDN）或缓存服务器。

**优点**：

- 能够实现内容的缓存，减少后端服务器的负载。

**缺点**：

- 需要考虑URL分布的均匀性。

#### 7. **源地址哈希法（Source Address Hashing）**

**描述**：基于请求源IP地址的哈希算法，将请求分配给后端服务器。这种策略适合需要会话粘性的场景，即来自同一客户端的请求应始终分配给同一台服务器。

**优点**：

- 能保持会话状态的连续性。

**缺点**：

- 可能导致不均衡的负载分布。

这些策略可以单独使用，也可以结合使用，以实现更精细的负载均衡。选择合适的策略取决于具体的应用需求、服务器性能和网络条件。

## 框架
![[Pasted image 20240724155449.png]]

### 1. 事件驱动

**是什么：** 事件驱动是一种编程模式，系统的行为由事件的发生来触发。事件可以是用户的输入、消息的到达、时间的流逝等。事件驱动模型主要包括事件循环（Event Loop）和事件处理器（Event Handler），事件循环等待和分派事件，而事件处理器则是执行响应逻辑的代码。

**为什么用：**

1. **高效处理并发**：事件驱动模型在处理大量并发连接时非常高效，因为它避免了为每个连接分配独立的线程或进程，节省了资源。
2. **响应速度快**：由于事件驱动模型的非阻塞特性，可以快速响应事件，这对于实时应用程序非常重要。
3. **资源节省**：减少了线程或进程的切换开销，降低了资源使用率，特别适用于I/O密集型应用。

### 2. 异步回调

**是什么：** 异步回调是一种处理异步操作的方法。在进行异步操作时（如I/O、网络请求），程序不会阻塞等待操作完成，而是继续执行其他任务。完成时，操作系统或框架会触发一个回调函数（Callback），这个函数包含了操作完成后的处理逻辑。

**为什么用：**

1. **非阻塞执行**：异步回调允许程序在等待I/O操作完成时继续执行其他任务，提高了系统的并发能力。
2. **资源优化**：避免了线程的长时间阻塞，使得系统资源能够更高效地使用。
3. **用户体验改善**：在GUI应用中，异步回调可以保持界面的响应性，不会因长时间的操作导致界面冻结。

### 3. 协程

**是什么：** 协程（Coroutine）是一种比线程更加轻量级的并发单元。协程可以在执行过程中主动让出控制权，而后继续从上次让出的位置恢复执行。协程通常由程序自身而不是操作系统调度管理，因此切换的开销非常小。

**为什么用：**

1. **轻量级并发**：协程的创建和切换成本比线程低得多，适合大量并发任务的场景。
2. **更简单的异步编程**：协程可以使异步编程更直观，使用同步的编程风格编写异步代码，避免了回调地狱。
3. **资源节省**：由于协程不需要操作系统线程的调度，减少了内存和处理器的资源开销。

### 总结

**事件驱动**、**异步回调**和**协程**都是处理并发和异步操作的编程模型和技术。事件驱动适合高并发场景，异步回调提供了一种非阻塞的异步操作处理方式，而协程则提供了一种轻量级的并发处理手段，结合了同步编程的简单性和异步编程的高效性。它们在不同场景下发挥各自的优势，用于提升系统的性能、资源利用率和用户体验。
### workflow
![[Pasted image 20240724155717.png]]

#### 使用workflow的一般流程
![[Pasted image 20240724155739.png|450]]

#### 安装 workflow
![[Pasted image 20240724155822.png]]

#### 验证库有没有安装成功
![[Pasted image 20240724155959.png]]
#### WaitGroup等待n件事情发生
![[Pasted image 20240724160025.png]]
![[Pasted image 20240724160039.png|300]]
#### 客户端任务
![[Pasted image 20240724160431.png|400]]
##### 是什么

Workflow是一种基于C++的异步事件驱动框架，用于构建高性能的网络应用程序。它提供了一种便捷的方式来处理网络I/O操作，如HTTP、RPC等。**客户端任务**是Workflow框架中用于发起和管理网络请求的关键组件。它抽象了网络通信中的复杂性，让开发者可以专注于业务逻辑。

##### 为什么用

1. **简化网络编程**：客户端任务封装了复杂的网络操作细节，如连接管理、数据传输、错误处理等，使得开发者能够专注于应用逻辑，而不必深入了解底层的网络编程。
    
2. **高效的资源使用**：基于事件驱动和异步模型，客户端任务可以高效地处理并发请求，减少资源消耗，提升应用性能。
    
3. **便捷的异步操作**：通过简单的接口和回调机制，轻松实现异步操作，避免传统异步编程中复杂的状态管理问题。
    
4. **可扩展性和灵活性**：支持各种协议（如HTTP、RPC）和传输层技术（如TLS），可以根据应用需求灵活配置和扩展。
    

##### 怎么用

使用Workflow框架中的客户端任务通常包括以下几个步骤：

1. **创建客户端任务**：选择合适的任务类型，如`WFHttpTask`（HTTP任务），`WFTcpTask`（TCP任务）等，根据需要初始化任务对象。
    
2. **配置任务参数**：设置任务的目标地址、请求方法、请求头、请求体等参数。
    
3. **设置回调函数**：定义任务完成后的回调函数，用于处理响应或错误。回调函数接收任务对象作为参数，可以访问响应数据和状态信息。
    
4. **启动任务**：将任务提交到调度器中，由框架负责执行和管理任务。
    

**示例代码**（以HTTP请求为例）：
```c++
#include "workflow/WFHttpClient.h"

void http_callback(WFHttpTask *task)
{
    const void *body;
    size_t body_len;
    int state = task->get_state();
    int error = task->get_error();
    protocol::HttpResponse *resp = task->get_resp();

    if (state == WFT_STATE_SUCCESS)
    {
        resp->get_parsed_body(&body, &body_len);
        // 处理响应数据
        std::cout << "Response: " << std::string((const char *)body, body_len) << std::endl;
    }
    else
    {
        std::cout << "Request failed, state: " << state << ", error: " << error << std::endl;
    }
}

int main()
{
    WFHttpTask *task = WFTaskFactory::create_http_task("http://example.com", 0, 0, http_callback);
    task->start(); // 启动任务
    return 0;
}
```
##### 总结

**Workflow框架中的客户端任务**通过封装网络通信的复杂性，提供了简洁的异步接口，方便开发者构建高性能的网络应用。通过合理配置任务和回调函数，开发者可以轻松管理网络请求和响应，提高开发效率和代码质量。

##### HTTP客户端任务
![[Pasted image 20240724161052.png]]

###### 是什么

在 Workflow 框架中，**HTTP 客户端任务**（`WFHttpTask`）是一种用于发起和处理 HTTP 请求的任务。它抽象了 HTTP 协议的请求和响应过程，简化了网络通信中的复杂性，让开发者可以专注于业务逻辑而无需深入理解底层的网络操作。HTTP 客户端任务是非阻塞的，基于异步回调机制进行操作。

###### 为什么用

1. **简化 HTTP 通信**：HTTP 客户端任务封装了创建连接、发送请求、接收响应、处理错误等操作，提供简单易用的接口，极大地简化了 HTTP 通信的编程过程。
    
2. **异步非阻塞**：基于事件驱动和异步回调的设计，HTTP 客户端任务可以在等待响应时继续处理其他任务，提升并发性能和资源利用率。
    
3. **高性能**：Workflow 框架的高效调度机制和优化的网络 I/O 处理，确保了 HTTP 客户端任务的高性能，适合构建高并发、高吞吐量的网络应用。
    
4. **灵活性和扩展性**：支持多种 HTTP 功能，如设置请求头、发送表单数据、处理多部分请求等，并可以方便地扩展和定制。
    

###### 怎么用

使用 Workflow 框架中的 HTTP 客户端任务通常包括以下几个步骤：

1. **创建 HTTP 客户端任务**：通过`WFTaskFactory`工厂类创建一个 HTTP 客户端任务对象，指定请求的 URL 和回调函数。
    
2. **配置请求**：设置请求方法（如GET、POST）、请求头、请求体等。
    
3. **设置回调函数**：定义一个回调函数，用于处理 HTTP 请求完成后的响应数据或错误信息。回调函数接收任务对象作为参数，可以从中获取状态码、响应内容等信息。
    
4. **启动任务**：将任务提交到框架的调度器中，任务会自动执行并在完成时调用回调函数。

示例代码：
```c++
#include "workflow/WFHttpClient.h"

void http_callback(WFHttpTask *task)
{
    const void *body;
    size_t body_len;
    int state = task->get_state();
    int error = task->get_error();
    protocol::HttpResponse *resp = task->get_resp();

    if (state == WFT_STATE_SUCCESS)
    {
        resp->get_parsed_body(&body, &body_len);
        // 处理响应数据
        std::cout << "Response: " << std::string((const char *)body, body_len) << std::endl;
    }
    else
    {
        std::cout << "Request failed, state: " << state << ", error: " << error << std::endl;
    }
}

int main()
{
    // 创建一个HTTP GET请求任务
    WFHttpTask *task = WFTaskFactory::create_http_task("http://example.com", 0, 0, http_callback);

    // 设置HTTP请求方法和头部信息
    task->get_req()->set_method("GET");
    task->get_req()->add_header_pair("Accept", "application/json");

    // 启动任务
    task->start();
    return 0;
}
```
###### 总结
Workflow 框架中的 HTTP 客户端任务提供了一个简洁而强大的方式来进行 HTTP 通信。它通过封装复杂的网络操作，提供异步非阻塞的接口，简化了 HTTP 请求的发起和响应处理流程。该任务适合构建高性能网络应用，特别是在高并发环境下。

##### 启动任务 将任务交给框架，由框架进行调度
![[Pasted image 20240724161521.png]]


##### 请求的类
![[Pasted image 20240724161554.png]]

##### 第一个任务
![[Pasted image 20240724161617.png]]

##### 交给框架以后
![[Pasted image 20240724164106.png]]

##### 光标
![[Pasted image 20240724164132.png]]

##### 获取报文体的地址
![[Pasted image 20240724164212.png]]

##### 代码解析
![[Pasted image 20240724164245.png]]
![[Pasted image 20240724164254.png]]
![[Pasted image 20240724164302.png]]

##### workflow框架中 报文头和报文体内存不在一起
![[Pasted image 20240724164326.png]]
