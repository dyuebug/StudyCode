# Qt 网络编程速查表 ⚡

> **阶段9总结** - 快速查找网络编程常用代码模板

---

## 📋 目录

1. [核心类概览](#核心类概览)
2. [HTTP GET 请求模板](#http-get-请求模板)
3. [HTTP POST 请求模板](#http-post-请求模板)
4. [JSON 解析速查](#json-解析速查)
5. [文件下载模板](#文件下载模板)
6. [常见公开 API 列表](#常见公开-api-列表)
7. [常见错误处理](#常见错误处理)
8. [最佳实践](#最佳实践)

---

## 核心类概览

| 类名 | 作用 | 常用方法 |
|------|------|----------|
| **QNetworkAccessManager** | 网络请求管理器 | `get()`, `post()`, `put()`, `deleteResource()` |
| **QNetworkRequest** | 网络请求对象 | `setUrl()`, `setHeader()`, `setRawHeader()` |
| **QNetworkReply** | 网络响应对象 | `readAll()`, `error()`, `errorString()`, `abort()` |
| **QJsonDocument** | JSON 文档 | `fromJson()`, `toJson()`, `object()`, `array()` |
| **QJsonObject** | JSON 对象 | `value()`, `contains()`, `keys()`, `insert()` |
| **QJsonArray** | JSON 数组 | `at()`, `size()`, `append()`, `toVariantList()` |

---

## HTTP GET 请求模板

### 基础 GET 请求

```cpp
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

// 1. 创建管理器（通常作为类成员变量）
QNetworkAccessManager *manager = new QNetworkAccessManager(this);

// 2. 创建请求
QUrl url("https://api.example.com/data");
QNetworkRequest request(url);

// 3. 发送 GET 请求
QNetworkReply *reply = manager->get(request);

// 4. 连接信号槽处理响应
connect(reply, &QNetworkReply::finished, this, [reply]() {
    if (reply->error() == QNetworkReply::NoError) {
        // 成功：读取响应数据
        QByteArray data = reply->readAll();
        qDebug() << "响应：" << data;
    } else {
        // 失败：显示错误信息
        qDebug() << "错误：" << reply->errorString();
    }
    reply->deleteLater();  // 释放内存
});
```

### 带查询参数的 GET 请求

```cpp
// 方法1：手动拼接 URL
QUrl url("https://api.example.com/search?q=keyword&limit=10");

// 方法2：使用 QUrlQuery（推荐）
#include <QUrlQuery>

QUrl url("https://api.example.com/search");
QUrlQuery query;
query.addQueryItem("q", "keyword");
query.addQueryItem("limit", "10");
url.setQuery(query);

QNetworkRequest request(url);
QNetworkReply *reply = manager->get(request);
```

### 带自定义请求头的 GET 请求

```cpp
QNetworkRequest request(url);

// 设置标准请求头
request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
request.setHeader(QNetworkRequest::UserAgentHeader, "MyApp/1.0");

// 设置自定义请求头
request.setRawHeader("Authorization", "Bearer YOUR_TOKEN");
request.setRawHeader("X-Custom-Header", "custom-value");

QNetworkReply *reply = manager->get(request);
```

---

## HTTP POST 请求模板

### 发送 JSON 数据

```cpp
#include <QJsonDocument>
#include <QJsonObject>

// 1. 构造 JSON 数据
QJsonObject json;
json["username"] = "alice";
json["password"] = "secret123";

QJsonDocument doc(json);
QByteArray postData = doc.toJson();

// 2. 创建请求
QNetworkRequest request(QUrl("https://api.example.com/login"));
request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

// 3. 发送 POST 请求
QNetworkReply *reply = manager->post(request, postData);

// 4. 处理响应
connect(reply, &QNetworkReply::finished, this, [reply]() {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(response);
        QJsonObject obj = doc.object();
        qDebug() << "Token:" << obj["token"].toString();
    } else {
        qDebug() << "错误：" << reply->errorString();
    }
    reply->deleteLater();
});
```

### 发送表单数据（application/x-www-form-urlencoded）

```cpp
#include <QUrlQuery>

// 1. 构造表单数据
QUrlQuery params;
params.addQueryItem("username", "alice");
params.addQueryItem("password", "secret123");

QByteArray postData = params.toString(QUrl::FullyEncoded).toUtf8();

// 2. 创建请求
QNetworkRequest request(QUrl("https://api.example.com/login"));
request.setHeader(QNetworkRequest::ContentTypeHeader, 
                  "application/x-www-form-urlencoded");

// 3. 发送 POST 请求
QNetworkReply *reply = manager->post(request, postData);
```

---

## JSON 解析速查

### 解析 JSON 对象

```cpp
// JSON 字符串：{"name": "Alice", "age": 25, "active": true}

QByteArray jsonData = reply->readAll();
QJsonDocument doc = QJsonDocument::fromJson(jsonData);

if (!doc.isNull() && doc.isObject()) {
    QJsonObject obj = doc.object();
    
    QString name = obj["name"].toString();           // "Alice"
    int age = obj["age"].toInt();                    // 25
    bool active = obj["active"].toBool();            // true
    
    // 检查键是否存在
    if (obj.contains("email")) {
        QString email = obj["email"].toString();
    }
}
```

### 解析 JSON 数组

```cpp
// JSON 字符串：[{"id": 1, "name": "Alice"}, {"id": 2, "name": "Bob"}]

QJsonDocument doc = QJsonDocument::fromJson(jsonData);

if (!doc.isNull() && doc.isArray()) {
    QJsonArray arr = doc.array();
    
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject obj = arr[i].toObject();
        int id = obj["id"].toInt();
        QString name = obj["name"].toString();
        qDebug() << "ID:" << id << "Name:" << name;
    }
}
```

### 解析嵌套 JSON

```cpp
// JSON 字符串：
// {
//   "user": {
//     "name": "Alice",
//     "address": {
//       "city": "Beijing",
//       "zip": "100000"
//     }
//   }
// }

QJsonDocument doc = QJsonDocument::fromJson(jsonData);
QJsonObject root = doc.object();

// 访问嵌套对象
QJsonObject user = root["user"].toObject();
QString name = user["name"].toString();

QJsonObject address = user["address"].toObject();
QString city = address["city"].toString();
QString zip = address["zip"].toString();
```

### 解析数组中的对象

```cpp
// JSON 字符串：
// {
//   "current_condition": [
//     {"temp_C": "25", "humidity": "60"}
//   ]
// }

QJsonDocument doc = QJsonDocument::fromJson(jsonData);
QJsonObject root = doc.object();

// 获取数组
QJsonArray conditions = root["current_condition"].toArray();

// 获取数组第一个元素
if (!conditions.isEmpty()) {
    QJsonObject condition = conditions[0].toObject();
    QString temp = condition["temp_C"].toString();
    QString humidity = condition["humidity"].toString();
}
```

---

## 文件下载模板

### 基础文件下载

```cpp
// 1. 发送 GET 请求
QUrl url("https://example.com/file.zip");
QNetworkRequest request(url);
QNetworkReply *reply = manager->get(request);

// 2. 监听下载进度
connect(reply, &QNetworkReply::downloadProgress, 
        this, [](qint64 received, qint64 total) {
    if (total > 0) {
        int percent = (received * 100) / total;
        qDebug() << "下载进度：" << percent << "%";
    }
});

// 3. 下载完成后保存文件
connect(reply, &QNetworkReply::finished, this, [reply]() {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        
        QFile file("downloaded_file.zip");
        if (file.open(QIODevice::WriteOnly)) {
            file.write(data);
            file.close();
            qDebug() << "文件保存成功！";
        }
    } else {
        qDebug() << "下载失败：" << reply->errorString();
    }
    reply->deleteLater();
});
```

### 大文件流式下载（推荐）

```cpp
// 1. 打开文件准备写入
QFile *file = new QFile("large_file.zip");
if (!file->open(QIODevice::WriteOnly)) {
    qDebug() << "无法创建文件！";
    return;
}

// 2. 发送请求
QNetworkReply *reply = manager->get(request);

// 3. 边下载边写入（避免占用大量内存）
connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
    file->write(reply->readAll());
});

// 4. 下载完成
connect(reply, &QNetworkReply::finished, this, [reply, file]() {
    file->close();
    file->deleteLater();
    
    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "下载完成！";
    } else {
        qDebug() << "下载失败：" << reply->errorString();
    }
    reply->deleteLater();
});
```

### 取消下载

```cpp
// 调用 abort() 取消下载
reply->abort();

// abort() 会触发 finished() 信号，error() 返回 OperationCanceledError
connect(reply, &QNetworkReply::finished, this, [reply]() {
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        qDebug() << "下载已取消";
    }
    reply->deleteLater();
});
```

---

## 常见公开 API 列表

| API 名称 | URL | 说明 | 需要 Key |
|----------|-----|------|----------|
| **IP 查询** | `https://api.ipify.org?format=json` | 获取本机公网 IP | ❌ |
| **UUID 生成** | `https://www.uuidtools.com/api/generate/v4` | 生成随机 UUID | ❌ |
| **汇率查询** | `https://api.exchangerate-api.com/v4/latest/USD` | 获取实时汇率 | ❌ |
| **天气查询** | `https://wttr.in/Beijing?format=j1` | 获取城市天气（中文需 URL 编码） | ❌ |
| **随机用户** | `https://randomuser.me/api/` | 生成随机用户信息 | ❌ |
| **狗狗图片** | `https://dog.ceo/api/breeds/image/random` | 随机狗狗图片 | ❌ |
| **笑话** | `https://official-joke-api.appspot.com/random_joke` | 随机笑话（英文） | ❌ |
| **GitHub API** | `https://api.github.com/users/{username}` | 获取 GitHub 用户信息 | ❌ |

### 中文城市名 URL 编码

```cpp
#include <QUrl>

QString city = "北京";
QString encoded = QUrl::toPercentEncoding(city);
// 结果："%E5%8C%97%E4%BA%AC"

QString url = "https://wttr.in/" + encoded + "?format=j1";
```

---

## 常见错误处理

### 错误码速查

```cpp
switch (reply->error()) {
    case QNetworkReply::NoError:
        // 成功
        break;
    case QNetworkReply::ConnectionRefusedError:
        qDebug() << "连接被拒绝";
        break;
    case QNetworkReply::RemoteHostClosedError:
        qDebug() << "远程主机关闭连接";
        break;
    case QNetworkReply::HostNotFoundError:
        qDebug() << "主机未找到";
        break;
    case QNetworkReply::TimeoutError:
        qDebug() << "请求超时";
        break;
    case QNetworkReply::OperationCanceledError:
        qDebug() << "操作已取消";
        break;
    case QNetworkReply::SslHandshakeFailedError:
        qDebug() << "SSL 握手失败";
        break;
    case QNetworkReply::ContentNotFoundError:
        qDebug() << "内容未找到（404）";
        break;
    default:
        qDebug() << "未知错误：" << reply->errorString();
        break;
}
```

### 超时处理

```cpp
// Qt 5.15+ 可以设置超时
request.setTransferTimeout(5000);  // 5秒超时

// 旧版本使用 QTimer 手动超时
QTimer *timer = new QTimer(this);
timer->setSingleShot(true);
connect(timer, &QTimer::timeout, this, [reply]() {
    reply->abort();  // 超时后取消请求
});
timer->start(5000);  // 5秒后触发

connect(reply, &QNetworkReply::finished, timer, &QTimer::stop);
```

---

## 最佳实践

### ✅ 推荐做法

1. **管理器作为成员变量**
   ```cpp
   class MyWidget : public QWidget {
   private:
       QNetworkAccessManager *manager;  // 复用管理器
   public:
       MyWidget() {
           manager = new QNetworkAccessManager(this);
       }
   };
   ```

2. **使用 Lambda 简化代码**
   ```cpp
   connect(reply, &QNetworkReply::finished, this, [reply, this]() {
       // 处理响应
       reply->deleteLater();
   });
   ```

3. **检查 JSON 有效性**
   ```cpp
   QJsonDocument doc = QJsonDocument::fromJson(data);
   if (doc.isNull()) {
       qDebug() << "JSON 解析失败！";
       return;
   }
   ```

4. **大文件使用流式下载**
   ```cpp
   // 使用 readyRead 信号边下载边写入
   connect(reply, &QNetworkReply::readyRead, [reply, file]() {
       file->write(reply->readAll());
   });
   ```

5. **记得释放 reply**
   ```cpp
   reply->deleteLater();  // 在 finished 槽中调用
   ```

### ❌ 常见错误

1. **忘记释放 reply**
   ```cpp
   // ❌ 错误：内存泄漏
   connect(reply, &QNetworkReply::finished, [reply]() {
       // 处理响应
       // 忘记调用 reply->deleteLater()
   });
   ```

2. **重复创建管理器**
   ```cpp
   // ❌ 错误：每次请求都创建新管理器
   void sendRequest() {
       QNetworkAccessManager *manager = new QNetworkAccessManager();
       // ...
   }
   ```

3. **不检查错误**
   ```cpp
   // ❌ 错误：直接使用数据，不检查错误
   connect(reply, &QNetworkReply::finished, [reply]() {
       QByteArray data = reply->readAll();  // 可能是空的
       // ...
   });
   ```

4. **大文件用 readAll()**
   ```cpp
   // ❌ 错误：大文件会占用大量内存
   QByteArray data = reply->readAll();  // 一次性读取所有数据
   ```

5. **不处理 total = -1**
   ```cpp
   // ❌ 错误：服务器未返回 Content-Length 时 total 为 -1
   int percent = (received * 100) / total;  // 除以 -1 导致错误
   
   // ✅ 正确：检查 total
   if (total > 0) {
       int percent = (received * 100) / total;
   } else {
       progressBar->setRange(0, 0);  // 不确定进度条
   }
   ```

---

## 🎯 快速参考

### 发送 GET 请求（3步）
```cpp
QNetworkRequest request(QUrl("https://api.example.com/data"));
QNetworkReply *reply = manager->get(request);
connect(reply, &QNetworkReply::finished, [reply]() { /* 处理 */ });
```

### 发送 POST 请求（4步）
```cpp
QJsonObject json; json["key"] = "value";
QByteArray data = QJsonDocument(json).toJson();
QNetworkRequest request(QUrl("https://api.example.com/post"));
request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
QNetworkReply *reply = manager->post(request, data);
```

### 解析 JSON（2步）
```cpp
QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
QJsonObject obj = doc.object();
QString value = obj["key"].toString();
```

### 下载文件（3步）
```cpp
QNetworkReply *reply = manager->get(request);
connect(reply, &QNetworkReply::readyRead, [reply, file]() { file->write(reply->readAll()); });
connect(reply, &QNetworkReply::finished, [reply, file]() { file->close(); reply->deleteLater(); });
```

---

**阶段9完成！接下来进入阶段10：Qt 多线程编程** 🚀
