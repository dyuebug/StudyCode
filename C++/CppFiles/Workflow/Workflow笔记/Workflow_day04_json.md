### JSON 是什么？

JSON（JavaScript Object Notation）是一种轻量级的数据交换格式。它使用易于人类阅读和编写的文本，并且易于机器解析和生成。JSON 是以键值对的形式存储数据，非常适合描述对象的结构。尽管 JSON 的语法来自 JavaScript，但它独立于任何编程语言，广泛用于各种编程环境中。

### 为什么使用 JSON？

1. **轻量级和易读**: JSON 格式简单明了，易于阅读和编写，适合快速传输数据。
    
2. **语言无关性**: JSON 作为一种标准格式，被几乎所有编程语言支持，便于在不同系统之间进行数据交换。
    
3. **广泛应用**: JSON 是现代Web开发中常用的数据格式，用于前后端数据交换、配置文件、存储结构化数据等。
    

### 在 C++ 中如何使用 JSON？

在 C++ 中，通常通过第三方库来处理 JSON 数据。常用的 JSON 库包括：

1. **nlohmann/json**: 一个头文件库，使用方便，功能全面。
2. **RapidJSON**: 一个快速、面向内存的 JSON 解析和生成库。

以下是如何使用 nlohmann/json 库的简单示例：

#### 安装 nlohmann/json

您可以通过以下命令安装 nlohmann/json：

- 使用包管理器，例如 `vcpkg`：
    
```
vcpkg install nlohmann-json
```
    
- 手动下载源代码并将其包含在项目中。

#### 使用示例

以下是如何在 C++ 中使用 nlohmann/json 库来解析和生成 JSON 数据的示例：
```c++
#include <iostream>
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

int main() {
    // 解析 JSON 字符串
    std::string jsonString = R"({"name": "Alice", "age": 25, "isStudent": false})";
    json j = json::parse(jsonString);

    // 访问 JSON 数据
    std::string name = j["name"];
    int age = j["age"];
    bool isStudent = j["isStudent"];

    std::cout << "Name: " << name << "\n";
    std::cout << "Age: " << age << "\n";
    std::cout << "Is Student: " << std::boolalpha << isStudent << "\n";

    // 构造 JSON 对象
    json newJ;
    newJ["name"] = "Bob";
    newJ["age"] = 30;
    newJ["isStudent"] = true;

    // 输出 JSON 字符串
    std::cout << newJ.dump() << std::endl;

    return 0;
}
```
## 标准的报文体格式
![[Pasted image 20240724201100.png|400]]


## json序列化方案
![[Pasted image 20240724201219.png|500]]

JSON 序列化是指将数据结构（如对象或数组）转换为 JSON 字符串的过程。这在数据存储、传输和交换中非常常见。在 C++ 中，序列化 JSON 通常涉及将对象或数据结构转换为 JSON 格式，以便传输到其他系统或存储。

### C++ 中的 JSON 序列化方案

在 C++ 中，常用的 JSON 序列化库有 `nlohmann/json` 和 `RapidJSON`。这些库提供了将 C++ 数据结构转换为 JSON 字符串的方法。以下是如何使用这些库进行 JSON 序列化的示例。

#### 使用 `nlohmann/json` 库

`nlohmann/json` 是一个非常流行的 C++ JSON 库，它使用简洁的语法，易于使用。以下是使用该库进行 JSON 序列化的示例：
```C++
#include <iostream>
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

int main() {
    // 创建一个 JSON 对象
    json j;
    j["name"] = "Alice";
    j["age"] = 25;
    j["isStudent"] = true;

    // 序列化为 JSON 字符串
    std::string serializedString = j.dump(); // 默认缩进为 0
    std::string prettySerializedString = j.dump(4); // 美化输出，缩进 4 空格

    std::cout << "Serialized JSON: " << serializedString << std::endl;
    std::cout << "Pretty Serialized JSON: " << prettySerializedString << std::endl;

    return 0;
}
```
## 树形结构
![[Pasted image 20240724201345.png]]
## nlohmann/json 它是header-only
![[Pasted image 20240724201412.png]]

## 构造object
![[Pasted image 20240724201441.png]]

## 构造array
![[Pasted image 20240724201549.png]]
## 反序列化
![[Pasted image 20240724201617.png]]
JSON 反序列化是指将 JSON 字符串转换为编程语言中的数据结构的过程。在 C++ 中，这通常涉及将 JSON 字符串解析为对象、数组或其他类型的数据结构。以下是如何在 C++ 中使用常见的 JSON 库进行反序列化的示例。
### 反序列化是什么？

反序列化（Deserialization）是将数据从一种特定的格式（如JSON、XML或二进制）转换回程序中数据结构（如对象、数组或字典）的过程。这个过程的逆操作称为序列化（Serialization），即将数据结构转换为特定格式以便存储或传输。

### 为什么使用反序列化？

反序列化在以下场景中非常有用：

1. **数据传输**: 在客户端和服务器之间传递数据时，数据通常以序列化格式传输。反序列化将接收到的格式化数据恢复为可操作的数据结构。
    
2. **数据存储和读取**: 数据通常以序列化格式（如JSON或XML文件）存储在文件系统中。反序列化将这些数据恢复为原始的编程语言对象或数据结构。
    
3. **跨平台和跨语言数据交换**: 序列化格式通常是语言无关的，允许不同编程语言之间的数据交换。反序列化使得接收方能够将数据转换为本地语言的结构

### 使用 `nlohmann/json` 库进行反序列化

`nlohmann/json` 库提供了一种简单直观的方法来解析 JSON 字符串。以下是一个例子，展示了如何将 JSON 字符串反序列化为 C++ 数据结构：
```C++
#include <iostream>
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

int main() {
    // JSON 字符串
    std::string jsonString = R"({"name": "Alice", "age": 25, "isStudent": true})";

    // 反序列化为 JSON 对象
    json j = json::parse(jsonString);

    // 访问数据
    std::string name = j["name"];
    int age = j["age"];
    bool isStudent = j["isStudent"];

    std::cout << "Name: " << name << "\n";
    std::cout << "Age: " << age << "\n";
    std::cout << "Is Student: " << std::boolalpha << isStudent << "\n";

    return 0;
}
```
在这个示例中，`json::parse` 方法用于将 JSON 字符串解析为 JSON 对象 `j`。之后，可以像访问标准容器一样访问 JSON 对象中的数据。
