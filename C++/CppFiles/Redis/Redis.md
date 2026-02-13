# C++57期Redis部分
## 什么是Redis

Redis（Remote Dictionary Server）是一个开源的、基于内存的数据结构存储系统，可以用作数据库、缓存和消息中间件。它支持多种数据结构，如字符串、哈希表、列表、集合、有序集合、位图、HyperLogLogs 和地理空间索引半径查询等。
## 为什么使用 Redis

1. **高性能**：由于 Redis 是基于内存的，它提供了非常高的读写性能，适合需要快速响应的应用场景。
2. **丰富的数据结构**：Redis 支持多种数据结构，可以根据需求灵活选择，满足复杂的数据操作需求。
3. **持久化**：虽然 Redis 是内存数据库，但它支持将数据持久化到磁盘，防止数据丢失。
4. **分布式**：Redis 支持主从复制、哨兵模式和集群模式，可以构建高可用和可扩展的分布式系统。
5. **简单易用**：Redis 提供了简单的命令行接口和丰富的客户端库，易于集成到各种编程语言和应用中。

## 如何使用 Redis

### 1. 安装 Redis

Redis 可以在 Linux、macOS 和 Windows 上安装。以 Ubuntu 为例，安装步骤如下：

```bash
sudo apt update
sudo apt install redis-server
```

安装完成后，可以通过以下命令启动 Redis 服务：

```bash
sudo systemctl start redis-server
sudo systemctl enable redis-server
```

### 2. 连接 Redis

可以使用 Redis 提供的命令行客户端 `redis-cli` 进行连接：
```bash
redis-cli
```

连接后可以通过输入 Redis 命令进行交互：

```bash
SET key "value"
GET key
```

### 3. 基本操作示例

以下是一些常见的 Redis 操作示例：

#### 字符串操作
```bash
SET key "Hello, Redis!"
GET key
```

#### 哈希表操作
```bash
HSET user:1000 name "John Doe"
HSET user:1000 email "john.doe@example.com"
HGET user:1000 name
HGETALL user:1000
```

#### 列表操作

```bash
LPUSH mylist "item1"
LPUSH mylist "item2"
RPUSH mylist "item3"
LRANGE mylist 0 -1
LPOP mylist
RPOP mylist
```
#### 集合操作
```bash
SADD myset "member1"
SADD myset "member2"
SMEMBERS myset
SISMEMBER myset "member1"
```

#### 有序集合操作
```bash
ZADD myzset 1 "one"
ZADD myzset 2 "two"
ZRANGE myzset 0 -1 WITHSCORES
ZREM myzset "one"
```

### 4. 持久化

Redis 提供了两种持久化机制：

1. **RDB（Redis Database File）**：在指定的间隔时间内生成数据集快照并存储到磁盘。
2. **AOF（Append Only File）**：记录每个写操作到日志文件，可以更精细地还原数据。

可以通过编辑 `redis.conf` 配置文件来启用和配置持久化机制。

### 5. 分布式和高可用

Redis 支持多种方式实现分布式和高可用：

1. **主从复制**：可以设置多个从节点，从节点复制主节点的数据，实现读写分离和数据冗余。
2. **哨兵模式**：通过哨兵监控主节点的状态，自动进行主从切换，保证高可用性。
3. **集群模式**：将数据分片存储在多个节点上，实现水平扩展和高可用。

### 示例代码
以下是使用 C++ 连接和操作 Redis 的示例代码。我们将使用 [hiredis](https://github.com/redis/hiredis)，这是一个简单且高效的 C 库，用于与 Redis 服务器通信。

首先，你需要安装 hiredis 库。可以通过以下命令安装：
```bash
sudo apt-get install libhiredis-dev
```

然后，编写以下 C++ 代码来连接和操作 Redis：
```C++
#include <hiredis/hiredis.h>
#include <iostream>

void checkReply(redisReply* reply) {
    if (reply == nullptr) {
        std::cerr << "Reply is null" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "Error: " << reply->str << std::endl;
        freeReplyObject(reply);
        exit(EXIT_FAILURE);
    }
}

int main() {
    // 连接到 Redis 服务器
    redisContext* c = redisConnect("127.0.0.1", 6379);
    if (c == nullptr || c->err) {
        if (c) {
            std::cerr << "Connection error: " << c->errstr << std::endl;
            redisFree(c);
        } else {
            std::cerr << "Connection error: can't allocate redis context" << std::endl;
        }
        exit(EXIT_FAILURE);
    }

    // 字符串操作
    redisReply* reply = (redisReply*)redisCommand(c, "SET foo bar");
    checkReply(reply);
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "GET foo");
    checkReply(reply);
    std::cout << "GET foo: " << reply->str << std::endl;
    freeReplyObject(reply);

    // 哈希表操作
    reply = (redisReply*)redisCommand(c, "HSET user:1000 name \"John Doe\"");
    checkReply(reply);
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "HGET user:1000 name");
    checkReply(reply);
    std::cout << "HGET user:1000 name: " << reply->str << std::endl;
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "HGETALL user:1000");
    checkReply(reply);
    std::cout << "HGETALL user:1000:" << std::endl;
    for (size_t i = 0; i < reply->elements; i += 2) {
        std::cout << "  " << reply->element[i]->str << ": " << reply->element[i + 1]->str << std::endl;
    }
    freeReplyObject(reply);

    // 列表操作
    reply = (redisReply*)redisCommand(c, "LPUSH mylist item1");
    checkReply(reply);
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "RPUSH mylist item2");
    checkReply(reply);
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "LRANGE mylist 0 -1");
    checkReply(reply);
    std::cout << "LRANGE mylist 0 -1:" << std::endl;
    for (size_t i = 0; i < reply->elements; i++) {
        std::cout << "  " << reply->element[i]->str << std::endl;
    }
    freeReplyObject(reply);

    // 集合操作
    reply = (redisReply*)redisCommand(c, "SADD myset member1");
    checkReply(reply);
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "SMEMBERS myset");
    checkReply(reply);
    std::cout << "SMEMBERS myset:" << std::endl;
    for (size_t i = 0; i < reply->elements; i++) {
        std::cout << "  " << reply->element[i]->str << std::endl;
    }
    freeReplyObject(reply);

    // 有序集合操作
    reply = (redisReply*)redisCommand(c, "ZADD myzset 1 one");
    checkReply(reply);
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "ZADD myzset 2 two");
    checkReply(reply);
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "ZRANGE myzset 0 -1 WITHSCORES");
    checkReply(reply);
    std::cout << "ZRANGE myzset 0 -1 WITHSCORES:" << std::endl;
    for (size_t i = 0; i < reply->elements; i += 2) {
        std::cout << "  " << reply->element[i]->str << ": " << reply->element[i + 1]->str << std::endl;
    }
    freeReplyObject(reply);

    // 断开连接
    redisFree(c);

    return 0;
}
```
通过上述步骤和示例代码，可以基本掌握 Redis 的安装、使用和操作方法。Redis 是一个功能强大且灵活的内存数据库，适用于各种高性能和高可用的应用场景。
## Day01

### 一、Redis的基本概念

#### 1、关系型数据库与非关系型数据库

关系型数据库：MySQL就是典型的关系型数据库，使用表结构。

非关系型数据库：redis是典型的非关系型数据库，存放数据的时候，使用键值对的形式。



#### 2、非关系型数据库的分类

基于键值对 key-value类型：**Redis**，memcached
列存储数据库 Column-oriented Graph：HBase
图形数据库 Graphs based：Neo4j
文档型数据库： MongoDB，MongoDB是一个基于分布式文件存储的数据库，主要用来处理大量的文档。



#### 3、redis的基本概念

远程字典服务器。使用C语言进行编写的，开源的，性能比较高。主要用作**数据库、缓存和消息中间件**，是基于内存运行的，并且支持**持久化**。

redis中文官网：http://www.redis.cn/(https://redis.com.cn/)
redis英文官网：https://redis.io/



#### 4、redis的特性

- 支持持久化
- 支持丰富的数据类型，例如：string、list、set、sort set、hash
- 支持数据的备份，主从复制

#### 5、redis的优点

- **快速高效**：Redis数据存储在内存中，并通过磁盘异步持久化，因此读写速度极快。每秒钟可以读<font color="#2DC26B">11w次</font>，每秒钟可以写<font color="#2DC26B">8.1w次</font>
- **丰富的数据结构**：支持丰富的数据结构，如字符串、列表、集合、哈希表、有序集合等，使其在不同场景下具有灵活的应用能力。
- **持久化支持**：支持数据持久化，可以将内存中的数据定期保存到磁盘中，以便重启后恢复数据。
- **原子性操作**：支持原子性操作，能够在多个命令上保证操作的原子性。
- **分布式支持**：通过主从复制和分片技术支持数据的分布式存储和高可用性。
- **丰富的功能**：支持事务、发布/订阅、Lua脚本、LRU过期和事务等高级功能。
#### 6、redis的缺点
- **内存限制**：由于数据存储在内存中，受到内存容量的限制，大规模数据存储需要合理规划内存资源。
- **持久化效率**：虽然支持持久化，但持久化操作可能会影响性能，特别是在大量写入时。
- **单线程模型**：Redis使用单线程模型来处理命令请求，虽然能够通过多路复用等方式提高并发处理能力，但单个命令的处理能力受到单线程的限制。
- **数据复杂性**：虽然支持多种数据结构，但在复杂的数据处理需求下可能需要复杂的应用逻辑和数据结构设计。
#### 7、使用Redis或任何缓存技术的主要原因
- **提升访问速度和性能**：缓存可以将经常访问的数据存储在内存中，从而加快数据的读取速度。相比于从磁盘或数据库中读取数据，内存访问速度更快，可以显著提升系统的响应速度和性能。
- **减轻后端数据库压力**：通过缓存可以减少对后端数据库的频繁访问，降低数据库的负载。这对于高并发和大规模数据应用尤为重要，可以有效避免数据库成为系统瓶颈。
- **提升系统的可伸缩性**：缓存技术能够帮助系统更好地处理大量并发请求，减少服务器的压力，从而提升系统的可伸缩性和扩展性。
- **改善用户体验**：通过缓存可以提供更快速的响应时间和更流畅的用户体验，特别是对于需要快速加载数据的应用，如电子商务网站、社交媒体平台等。
- **降低成本**：优化性能和减少服务器负载可以降低硬件和基础设施成本。使用缓存技术可以延长硬件的使用寿命，减少对高性能硬件的依赖。
- **支持临时数据存储**：缓存不仅可以用于存储常用数据，还可以用于临时存储计算中间结果、会话信息等，有助于优化计算和数据处理流程。
- **提供数据保护和安全**：通过缓存可以实现数据备份和复制，提高数据的可靠性和安全性。此外，缓存技术通常支持数据加密和访问控制，有助于保护敏感信息。

#### 8、Redis 实现高效快速的原因

- **基于内存存储**：Redis 将数据存储在内存中，读写操作都是在内存完成，而内存的访问速度远远快于磁盘。相比于传统的数据库系统，Redis 不需要频繁地访问磁盘，因此能够显著提升数据的读写性能。
    
- **单线程模型**：Redis 使用单线程模型处理客户端请求，避免了多线程或多进程间的锁竞争和上下文切换的开销。这使得Redis在处理请求时能够更加高效，特别适合处理大量短期请求的场景。
    
-  **非阻塞 I/O**：Redis 使用了多路复用技术（multiplexing），如 epoll 等，实现了非阻塞 I/O 操作。这使得 Redis 能够在等待数据准备好时继续处理其他请求，而不必阻塞在 I/O 操作上。
    
-  **优化的数据结构**：Redis 内置了多种高效的数据结构，如字符串、哈希表、列表、集合、有序集合等，这些数据结构在内存中存储和操作效率极高，能够满足不同的应用需求。
    
-  **支持持久化**：Redis 提供了多种持久化选项，可以将内存中的数据定期保存到磁盘中，以防止数据丢失。即使使用持久化功能，Redis 也能够在内存中高效地读写数据，而不受磁盘 I/O 的限制。
    
-  **简单和高效的代码实现**：Redis 的代码基于 C 语言编写，采用了高效的算法和数据结构设计，经过多年的优化和稳定性测试，保证了其在性能上的优势和稳定性。
### 二、Redis基本命令
```redis
#数据库的切换
select  + index   
    
#清空当前数据库   flushdb
#清空所有数据库   flushall
#查看当前数据库的大小  dbsize

#查看当前数据库有哪些key值    keys *、keys k? 

#删除某个key  del  key
    
#设置key的过期时间  expire  key  seconds
#查看key的过期时间  ttl  key   #-1表示永不过期，-2表示已过期
#查看key的类型  type  key
```

### 三、Redis的五大常用数据类型（==重要==）

![image-20240712093443147](file://D:\ProgramData\Material\C++Video\04_Redis_%E5%93%A9%E5%93%A9\Redis%E8%A7%86%E9%A2%91%E4%B8%8E%E7%AC%94%E8%AE%B0\1%E7%AC%94%E8%AE%B0\C++57%E6%9C%9FRedis%E9%83%A8%E5%88%86.assets\image-20240712093443147.png?lastModify=1720785185)

[https://github.com/sewenew/redis-plus-plus](https://github.com/sewenew/redis-plus-plus)

#### 1、string数据类型

可以存放任何数据，包括图片。是二进制安全的。
```redis
#设置key值与获取key
set key  value
get key

#一次设置多个或者获取多个
mset k1 value1  k2 value2 ...
mget k1 k2 ...

#获取子串
getrange key  start end   #start、end是起始与终止的下标   -1代表倒数第一个
#设置子串
setrange key  offset newValue   #将key的value值从offset位置开始，设置为newValue

#设置key的过期时间，以及设置新值
setex key  seconds newValue

#incr或者incrby只能对value值是数值的情况，进行增加操作，如果value是字符串就不能增加
incr key  #每次将key的value值加1
incrby key  a  #每次将key的value值加a
```

#### 2、list数据类型
```reids
#在左右两边进行插入
lpush/rpush

#在左右两边进行删除
lpop/rpop

#遍历
lrange key  start end  #star与end是起始下标与结束下标

#支持下标（STL中的list是不支持下标的）
lindx key index  #获取下标为index的值
lset key index value  #将下表为index的值设置为value
linsert key before/after value newValue  #在list的value的前或者后面插入一个新值
```

#### 3、set数据类型
```redis
#添加元素
sadd key  value1 value2 value3 ...

#遍历
smembers key  #遍历set中所有元素

#可以在集合中取交集、差集、并集
SINTER/SDIFF/SUNION

#随机删除count个元素
spop key  count
```
#### 4、sort set数据类型
```redis
#添加元素
zadd key score1 value1 score2 value2 ...

#遍历
zrange key start stop [withscores]
```

#### 5、hash数据类型
```redis
#设置与获取
hset key field value
hget key field

#设置多个与获取多个
hmset key field1 value1 field2 value2 ....
hmget key field1 field2 ...
```

### 四、redis的配置文件

#### 1、配置文件的路径

/etc/redis下面，有个文件名 6379.conf

#### 2、杀死redis服务器

kill -9 进程id号  
可以在客户端中直接执行shutdowm命令

#### 3、如何启动redis服务器

redis-server + 配置文件的路径  
```redis
sudo redis-server /etc/redis/6379.conf
```


### 五、持久化（==重要==）

#### 1、概念

将内存中的数据定期的备份到磁盘中，下一次重启的时候，就可以将磁盘中的数据恢复到内存中。（防止数据丢失了）

Redis 持久化是指将 Redis 中的数据存储到持久化存储介质（如硬盘）上，以保证数据在服务器重启后不会丢失的机制。

#### 2、分类

RDB：将当前**数据**保存到硬盘（原理是将Reids在内存中的数据库记录定时dump到磁盘上的RDB持久化）

AOF：将每次执行的**写命令**保存到硬盘（原理是将Reids的操作日志以追加的方式写入文件，类似于MySQL的binlog）

#### 3、RDB（Redis DataBase）持久化

##### 3.1、基本概念

RDB是Redis **默认的持久化方案**。==**在指定的时间间隔内，执行指定次数的写操作，则会将内存中的数据写入到磁盘中**==。即在指定目录下生成一个dump.rdb文件。Redis 重启会通过加载dump.rdb文件恢复数据。（/var/lib/redis/6379）

##### 3.2、触发快照的方式

- 执行shutdown关闭服务器，会触发快照，也就是修改dump.rdb文件的修改时间
    
- 执行flushall清空数据库，也会触发快照
    
- 手动执行save或者bgsave可以触发快照，将数据保持到磁盘中
    
- ==在指定的时间间隔内，执行指定次数的写操作==（==最重要==），自动触发快照
    

14:30:40的时候，执行了一次save命令，接着执行两次写操作，等时间达到了14:31:00的时候，文件dump.rdb的修改时间依旧是14:30，但是等时间差达到30s的时候，比如：14:31:10以后再查看dump.rdb会发现该文件的修改时间自动就变为14:31

##### 3.3、优缺点

优点

- 如果对数据的完整性与一致性要求不高，可以使用rdb的持久化方式
    
- 相对于aof而言，数据的恢复速率是比较快的
    

缺点

- 进行数据恢复的时候，数据的完整性与一致性不高（可能丢失60s的数据）
    
- 备份时占用内存，因为Redis 在备份时会独立创建一个fork子进程，将数据写入到一个临时文件（此时内存中的数据是原来的两倍哦），最后再将临时文件替换之前的备份文件
    

#### 4、AOF（Append Only File）持久化

##### 4.1、概念

将所有执行的**写命令**全部记录下来，然后数据恢复的时候，再将写命令重新执行一次。

##### 4.2、重写
```redis
set k1 100  
incr k1  
//100  
//...  101次  
最终k1的值是200  
set k1 200  
​  
set k1 100  
incr k1 300  
//...  
//...  
del k1
```


重写之后就可以让文件的大小减小。

触发重写的方式：可以手动执行BGREWRITEAOF命令。

自动触发重写的方式：Redis会记录上次重写时的AOF大小，默认配置是当AOF文件大小是上次rewrite后大小的一倍且文件大于64M时触发
```redis
auto-aof-rewrite-percentage 100  
auto-aof-rewrite-min-size 64mb

```

##### 4.3、优缺点

优点

- 相对于rdb而言，数据的完整性与一致性高。
    
缺点

- 因为采用的是命令追加的形式，有可能导致文件的体积比较大
    
- 相对于rdb而言，数据的恢复速率是比较慢的
    

#### 5、两种持久化的总结

- 如果想要数据恢复快，可以选择rdb持久化；如果想要精度比较高，可以选择aof
    
- 两种持久化的方式是可以同时存在，起到互补的作用
    
- 如果单独的以aof持久化的方式，也是可以让服务器启动成功的。
    
- 如果只有aof持久化方式，并且aof文件被损坏了，redis服务器是启动不成功的。可以使用redis-check-aof文件进行修复
    

![image-20240712151039503](file://D:\ProgramData\Material\C++Video\04_Redis_%E5%93%A9%E5%93%A9\Redis%E8%A7%86%E9%A2%91%E4%B8%8E%E7%AC%94%E8%AE%B0\1%E7%AC%94%E8%AE%B0\C++57%E6%9C%9FRedis%E9%83%A8%E5%88%86.assets\image-20240712151039503.png?lastModify=1720785185)

#### 6、在Redis中，设置key的过期时间和永久有效

1. **设置key的过期时间**：  
    你可以使用 `EXPIRE` 或 `PEXPIRE` 命令来设置key的过期时间。`EXPIRE` 命令以秒为单位设置过期时间，而 `PEXPIRE` 命令以毫秒为单位设置过期时间。
 
    ```Redis
    EXPIRE key_name seconds
    PEXPIRE key_name milliseconds
    
    #例如，如果你想设置一个key在10秒后过期：
    EXPIRE my_key 10
    ```

    
2. **设置key永久有效**：  
    如果你想让一个key永久有效，可以使用 `PERSIST` 命令来移除key的过期时间。
        PERSIST key_name
    
    例如，如果你想让一个key永久有效：
    ```redis
    PERSIST my_key
    ```
    

### 六、Redis的事务

#### 1、概念

一组命令的集合

#### 2、三个阶段

开始事务、命令入队、执行事务

#### 3、事务的命令
```redis
#开始事务  
multi  
​  
#执行事务  
exec  
​  
#取消事务  
discard
```


![image-20240712155243409](file://D:\ProgramData\Material\C++Video\04_Redis_%E5%93%A9%E5%93%A9\Redis%E8%A7%86%E9%A2%91%E4%B8%8E%E7%AC%94%E8%AE%B0\1%E7%AC%94%E8%AE%B0\C++57%E6%9C%9FRedis%E9%83%A8%E5%88%86.assets\image-20240712155243409.png?lastModify=1720785185)

![image-20240712155748145](file://D:\ProgramData\Material\C++Video\04_Redis_%E5%93%A9%E5%93%A9\Redis%E8%A7%86%E9%A2%91%E4%B8%8E%E7%AC%94%E8%AE%B0\1%E7%AC%94%E8%AE%B0\C++57%E6%9C%9FRedis%E9%83%A8%E5%88%86.assets\image-20240712155748145.png?lastModify=1720785185)

==**总结：redis中的事务是没有原子性的**。==

#### 4、乐观锁与悲观锁

悲观锁：每次拿数据的时候，都认为其他的线程或者进程会修改数据，所以需要进行加锁。例如：linux学习到的pthread_mutex_t的锁、C++11学习的std::mutex.

乐观锁：每次拿数据的时候，都认为其他的线程或者进程不会修改数据，所以就不需要加锁。实现手段：**版本号机制或者CAS机制。**

### 七、主从复制（==比较重要==）

#### 1、概念

主指将一台Redis服务器的数据，**复制**到其他的Redis服务器。前者称为**主节点**(master)，后者称为**从节点**(slave)；数据的复制是**单向**的，只能由主节点到从节点

#### 2、为何要做主从复制

达到负载均衡、防止主机挂掉之后，不能相应请求、读写分离

#### 3、主从复制的配置方法

1、首先进入到/etc/redis下面，将6379.conf拷贝两份，分别取名6380.conf与6381.conf

2、修改6380.conf与6381.conf文件中的以下内容
```redis
port    6380                       6381  
pidfile "/var/run/redis_6380.pid"  "/var/run/redis_6381.pid"  
logfile "/var/log/redis_6380.log"  "/var/log/redis_6381.log"  
dbfilename "dump6380.rdb"          "dump6381.rdb"  
appendfilename "appendonly6380.aof" "appendonly6381.aof"
```


3、将三个配置文件进行启动
```redis
sudo redis-server /etc/redis/6379.conf  
sudo redis-server /etc/redis/6380.conf  
sudo redis-server /etc/redis/6381.conf
```


![image-20240712162604621](file://D:\ProgramData\Material\C++Video\04_Redis_%E5%93%A9%E5%93%A9\Redis%E8%A7%86%E9%A2%91%E4%B8%8E%E7%AC%94%E8%AE%B0\1%E7%AC%94%E8%AE%B0\C++57%E6%9C%9FRedis%E9%83%A8%E5%88%86.assets\image-20240712162604621.png?lastModify=1720785185)

4、分别进入到对应的服务器的客户端下面
```redis
redis-cli -p 6379  
redis-cli -p 6380  
redis-cli -p 6381

```

5、查看每台机器的身份信息，发现默认都是主机(他们的role都是master)
```redis
info replication
```


6、开始配置主从复制的关系，在从机上执行slaveof命令
```redis
#在6380与6381下面执行  
slaveof 127.0.0.1 6379  
```

然后查看主从复制的信息info replication，发现从机上可以看到主机的ip、port以及状态信息；在主机上也可以看到从机的ip、port以及状态信息

- 如果从机6381断开了，然后主机6379就不会记录6381的信息了，即使6381重新启动，6379也不会记录6381的信息，因为6381重新启动的时候，是以主机的身份启动的
    
- 如果主机因为某系愿意挂掉了，那么从机6380与6381会感知到6379的下线，同时会记录主机的状态为down，但是还是会默默的等待主机的上线，当主机6379上线之后，6379依旧会作为主机，然后6380与6381就依旧是6379的从机。
    

如果主机一直挂掉，永不上线，那么基本的主从复制关系就不能执行写操作，因为没有主机。

### 八、哨兵模式（==比较==）

哨兵会执行留言协议（造谣）与投票协议（少数服用多数）

#### 配置步骤

1、在/etc/redis下，创建哨兵的配置文件sentinel.conf
```redis
#哨兵    监视    主机名     主机ip    端口 票数  
sentinel monitor master6379 127.0.0.1 6379 1

```

2、启动哨兵的配置文件
```redis
sudo redis-sentinel  /etc/redis/sentinel.conf
```

3、哨兵会监视主机6379的状态变更，如果6379因为某些原因断开，那么哨兵会将自己的一票投递给6380或者6381，谁得到这一票，谁就是新的主机，所以新的主从复制状态就展示出来了。这样就可以解决传统的主从复制，因为主机断掉而不能执行写操作的弊端。

### 九、面试常见的三个问题(==重要==)

#### 1、缓存雪崩

概念：**缓存中的大量数据在同一时间失效了**，相当于缓存中没有数据，那么客户端的请求会下沉到底层数据数据库上，那么有可能会导致底层数据库的压力比较大，甚至将底层数据库压垮。

解决问题：1、可以让key值不过期 2、将key的过期失效分散

![image-20240712172046839](file://D:\ProgramData\Material\C++Video\04_Redis_%E5%93%A9%E5%93%A9\Redis%E8%A7%86%E9%A2%91%E4%B8%8E%E7%AC%94%E8%AE%B0\1%E7%AC%94%E8%AE%B0\C++57%E6%9C%9FRedis%E9%83%A8%E5%88%86.assets\image-20240712172046839.png?lastModify=1720785185)

#### 2、缓存击穿

概念：**热点数据key在缓存中失效了**，热点数据不在缓存中，那么大量客户端的请求会下沉到底层数据数据库上，那么有可能会导致底层数据库的压力比较大，甚至将底层数据库压垮。

解决方案：1、延迟热点数据的失效时间或者不失效 2、可以将数据库看成共享资源，互斥访问数据库

![image-20240712172757249](file://D:\ProgramData\Material\C++Video\04_Redis_%E5%93%A9%E5%93%A9\Redis%E8%A7%86%E9%A2%91%E4%B8%8E%E7%AC%94%E8%AE%B0\1%E7%AC%94%E8%AE%B0\C++57%E6%9C%9FRedis%E9%83%A8%E5%88%86.assets\image-20240712172757249.png?lastModify=1720785185)

#### 3、缓存穿透

概念：**访问的数据在缓存中与底层数据库中都不存在**，那么还是会对底层数据库的压力比较大，导致有可能将底层数据库压垮。

解决方案：1、在缓存中设置键值对形式`<key, null>`

![image-20240712173441884](file://D:\ProgramData\Material\C++Video\04_Redis_%E5%93%A9%E5%93%A9\Redis%E8%A7%86%E9%A2%91%E4%B8%8E%E7%AC%94%E8%AE%B0\1%E7%AC%94%E8%AE%B0\C++57%E6%9C%9FRedis%E9%83%A8%E5%88%86.assets\image-20240712173441884.png?lastModify=1720785185)

#### 基本概念

**1. Redis 是什么？** Redis 是一个开源的基于内存的高性能键值对存储系统，支持多种数据结构，常用于数据库、缓存和消息中间件。

**2. Redis 的主要特点是什么？**

- 高性能：由于基于内存，读写速度非常快。
- 丰富的数据结构：支持字符串、哈希表、列表、集合、有序集合等。
- 持久化：支持将数据持久化到磁盘。
- 分布式：支持主从复制、哨兵模式和集群模式。
- 简单易用：提供简单的命令行接口和丰富的客户端库。

#### 使用场景

**3. Redis 适用于哪些场景？**

- 缓存：用于加速读取频繁访问的数据，减少数据库压力。
- 会话存储：存储用户会话信息，实现高效的会话管理。
- 排行榜：利用有序集合实现排行榜功能。
- 计数器：用于计数操作，如页面浏览量统计。
- 发布/订阅：用于消息队列，实现实时消息系统。

#### 数据结构

**4. Redis 支持哪些数据结构？**

- 字符串（String）
- 列表（List）
- 集合（Set）
- 有序集合（Sorted Set）
- 哈希表（Hash）
- 位图（Bitmaps）
- HyperLogLog
- 地理空间索引（Geospatial）

**5. Redis 列表（List）和集合（Set）有什么区别？**

- 列表是有序的，元素可以重复，支持按索引访问。
- 集合是无序的，元素不重复，支持集合运算（交集、并集、差集）。

#### 性能优化

**6. 如何优化 Redis 的性能？**

- 使用适当的数据结构：根据使用场景选择合适的数据结构。
- 开启持久化策略：选择适当的持久化策略（RDB 或 AOF）。
- 使用缓存策略：合理设置缓存失效时间，避免缓存雪崩。
- 分片和集群：将数据分片存储在多个节点上，实现水平扩展。
- 减少网络延迟：使用 pipelining 减少网络往返次数。

**7. 什么是 Redis 的管道（Pipelining）？** Pipelining 是一种批量发送命令的技术，可以在一次网络请求中发送多个命令，从而减少网络延迟，提高性能。

#### 持久化

**8. Redis 有哪些持久化机制？**

- RDB（Redis Database File）：在指定的时间间隔生成数据集快照并存储到磁盘。
- AOF（Append Only File）：记录每个写操作到日志文件，可以更精细地还原数据。

**9. RDB 和 AOF 有什么区别？**

- RDB 是基于时间间隔的快照持久化，数据恢复速度快，但在发生故障时可能会丢失最近的数据。
- AOF 是基于日志的持久化，记录每个写操作，数据恢复更加完整，但日志文件较大，恢复速度较慢。

#### 分布式架构

**10. 什么是 Redis 哨兵（Sentinel）？** Redis Sentinel 是一种高可用性解决方案，用于监控 Redis 主从架构的健康状态，自动进行故障转移（failover），并通知客户端新的主节点位置。

**11. Redis 集群（Cluster）是什么？** Redis Cluster 是 Redis 的分布式解决方案，通过数据分片（sharding）将数据存储在多个节点上，实现水平扩展和高可用性。

**12. 如何在 Redis 中实现分布式锁？** 分布式锁可以使用 Redis 的 `SET` 命令和 NX 参数（仅在键不存在时设置键），并设置过期时间（EX 参数），确保锁的自动释放。例如：

```bash
SET mylock myvalue NX EX 10
```

### 其他常见问题

**13. Redis 如何处理缓存穿透？** 可以使用布隆过滤器（Bloom Filter）来拦截无效请求，避免每次请求都查询数据库。

**14. Redis 如何处理缓存雪崩？** 通过设置缓存失效时间为随机值，避免大量缓存同时失效，或者使用互斥锁（mutex）控制缓存的重建。

**15. Redis 如何处理缓存击穿？** 可以使用互斥锁（mutex）或双重检查锁（double-check locking）来控制热点数据的缓存重建。

这些问题涵盖了 Redis 的基本概念、使用场景、数据结构、性能优化、持久化以及分布式架构等方面，能够帮助你在面试中更好地回答与 Redis 相关的问题。