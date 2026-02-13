### ReactorV2版本（==重要难==）
#Reactor  #重点  #网络编程 

#### 1、伪代码
```c++

void loop()  
{  
   _isLooping = true;  
    while(_isLooping)  
    {  
        waitEpollFd();  
    }  
}  
​  
void unloop()  
{  
     _isLooping = false;  
}  
​  
​  
void waitEpollFd()  
{  
    nready = epoll_wait(_epfd, _evtList, _evtList.size(), 3000);  
    if(-1 == nready && errno == EINTR)  
    {  
        continue;  
    }  
    else if(-1 == nready)  
    {  
        perror("");  
        return;  
    }  
    else if(0 == nready)  
    {  
        //打印超时  
    }  
    else  
    {  
        for(size_t idx = 0; idx < nready; ++idx)  
        {  
            int fd = _evtList[idx].data.fd;  
            if(listenfd被就绪了)  
            {  
                //处理新的连接请求  
                handleNewConnection();  
            }  
            else  
            {  
                //处理老的连接上的数据的收发  
                handleMessage(fd);  
            }  
        }  
    }  
}  
​  
//处理新的连接  
 void handleNewConnection()  
 {  
     connfd = _acceptor.accept();  
       
     TcpConnection con(coonfd);//创建出TCP连接  
     //监听connfd  
     addEpollReadFd(connfd);  
     //存放文件描述符与连接的键值对  
     _conns.insert(connfd, con);  
 }  
​  
void  handleMessage(int fd)  
{  
    it = _conns.find(fd);  
    if(it != _conns.end())  
    {  
        //连接找到了  
        string msg = it->second->receive();  
        //处理接受的msg  
        //将其发送给客户端  
        it->second->send(msg);  
    }  
    else  
    {  
        //连接不存在  
    }  
}
```

#### 2、类图设计

![image-20240709142203760](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240709142203760.png?lastModify=1720702254)

#### 3、Tcp网络通信的三个半事件

**连接建立**：包括服务器端被动接受连接（accept）和客户端主动发起连接（connect）。TCP连接一旦建立，客户端和服务端就是平等的，可以各自收发数据。 **连接断开**：包括主动断开（close、shutdown）和被动断开（read()返回0）。 **消息到达**：文件描述符可读。这是最为重要的一个事件，对它的处理方式决定了网络编程的风格（阻塞还是非阻塞，如何处理分包，应用层的缓冲如何设计等等）。 **消息发送完毕**：这算半个。对于低流量的服务，可不必关心这个事件；另外，这里的“发送完毕”是指数据写入操作系统缓冲区（内核缓冲区），将由TCP协议栈负责数据的发送与重传，不代表对方已经接收到数据。

#### 4、新的类图（添加回调）

![image-20240709193427014](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240709193427014.png?lastModify=1720702254)

![image-20240709193635081](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240709193635081.png?lastModify=1720702254)

#### 5、重要代码（==重要==）
##### 5.1、EventLoop中键值对的设计

![image-20240709193044085](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240709193044085.png?lastModify=1720702254)

##### 5.2、EventLoop中的回调的存储与注册

![image-20240709193904417](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240709193904417.png?lastModify=1720702254)

##### 5.3、TcpConnection中的回调存储以及注册与执行

![image-20240709194247824](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240709194247824.png?lastModify=1720702254)

##### 5.4、TcpConnection中的回调执行实现

防止智能指针的误用

![image-20240709194510718](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240709194510718.png?lastModify=1720702254)

##### 5.5、EventLoop中epoll_wait函数的传参

![image-20240709194712701](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240709194712701.png?lastModify=1720702254)

##### 5.6、EventLoop中handleNewConnection函数的流程

![image-20240709195147789](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240709195147789.png?lastModify=1720702254)

##### 5.7、EventLoop中handleMessage函数流程

![image-20240709195333656](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240709195333656.png?lastModify=1720702254)

#### 6、流程图
  
![image-20240710094222337](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240710094222337.png?lastModify=1720702254)


### 代码实现
#### NonCopyable类
```C++
#ifndef __NONCOPYABLE_H__

#define __NONCOPYABLE_H__

//不允许复制或者赋值

class NonCopyable

{

protected:

    NonCopyable()

    {

    }

    ~NonCopyable()

    {

    }

    NonCopyable(const NonCopyable &) = delete;

    NonCopyable &operator=(const NonCopyable &) = delete;

};

#endif
```

#### Socket类
```C++
//Socket.h
#ifndef __SOCKET_H__

#define __SOCKET_H__

#include "NonCopyable.h"

class Socket

: NonCopyable

{

public:

    Socket();

    explicit Socket(int fd);

    ~Socket();

    int fd() const;

    void shutDownWrite();

private:

    int _fd;

};

#endif

//Socket.cc
#include "Socket.h"

#include <sys/types.h>

#include <sys/socket.h>

#include <stdio.h>

#include <unistd.h>

Socket::Socket()

{

    _fd = ::socket(AF_INET, SOCK_STREAM, 0);

    if(_fd < 0)

    {
        perror("socket");

        return;
    }

}

Socket::Socket(int fd)

: _fd(fd)

{

}

Socket::~Socket()

{

    close(_fd);

}

int Socket::fd() const

{

    return _fd;

}

void Socket::shutDownWrite()

{

    //关闭写端

    int ret = shutdown(_fd, SHUT_WR);

    if(ret)

    {

        perror("shutdown");

        return;

    }

}
```

#### InetAddress类
```c++
//InetAddress.h
#ifndef __INETADDRESS_H__

#define __INETADDRESS_H__

#include <arpa/inet.h>

#include <string>

using std::string;

class InetAddress

{

public:

    InetAddress(const string &ip, unsigned short port);

    InetAddress(const struct sockaddr_in &addr);

    ~InetAddress();

    string ip() const;

    unsigned short port() const;

    const struct sockaddr_in *getInetAddrPtr() const;

private:

    struct sockaddr_in _addr;

};

#endif

//InetAddress.cc
#include "InetAddress.h"

#include <strings.h>

InetAddress::InetAddress(const string &ip, unsigned short port)

{

    ::bzero(&_addr, sizeof(struct sockaddr_in));

    _addr.sin_family = AF_INET;

    _addr.sin_port = htons(port);

    _addr.sin_addr.s_addr = inet_addr(ip.c_str());//host->network

}

InetAddress::InetAddress(const struct sockaddr_in &addr)

: _addr(addr)

{

}

InetAddress::~InetAddress()

{

}

string InetAddress::ip() const

{

    //从网络字节序转换为本机字节序

    return string(inet_ntoa(_addr.sin_addr));

}

unsigned short InetAddress::port() const

{

    //从网络字节序转换为本机字节序

    return ntohs(_addr.sin_port);

}

const struct sockaddr_in *InetAddress::getInetAddrPtr() const

{

    return &_addr;

}
```

#### Acceptor类
```c++
//Acceptor.h
#ifndef __ACCEPTOR_H__

#define __ACCEPTOR_H__

#include "Socket.h"

#include "InetAddress.h"

#include <string>

using std::string;

class Acceptor

{

public:

    Acceptor(const string &ip, unsigned short port);

    ~Acceptor();

    void ready();

private:

    void setReuseAddr();

    void setReusePort();

    void bind();

    void listen();

public:

    int accept();

    int fd() const;

private:

    Socket _sock;

    InetAddress _addr;

};

#endif

//Acceptor.cc
#include "Acceptor.h"

#include <stdio.h>

Acceptor::Acceptor(const string &ip, unsigned short port)

: _sock()

, _addr(ip, port)

{

}

Acceptor::~Acceptor()

{

}

void Acceptor::ready()

{

    setReuseAddr();

    setReusePort();

    bind();

    listen();

}

void Acceptor::setReuseAddr()

{

    int on = 1;

    int ret = setsockopt(_sock.fd(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if(ret)

    {

        perror("setsockopt");

        return;

    }

}

void Acceptor::setReusePort()

{

    int on = 1;

    int ret = setsockopt(_sock.fd(), SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));

    if(-1 == ret)

    {

        perror("setsockopt");

        return;

    }

}

void Acceptor::bind()

{

    int ret = ::bind(_sock.fd(), 

                     (struct sockaddr *)_addr.getInetAddrPtr(),

                     sizeof(struct sockaddr));

    if(-1 == ret)

    {

        perror("bind");

        return;

    }

}

void Acceptor::listen()

{

    int ret = ::listen(_sock.fd(), 128);

    if(-1 == ret)

    {

        perror("listen");

        return;

    }

}

int Acceptor::accept()

{

    int connfd = ::accept(_sock.fd(), nullptr, nullptr);

    if(-1 == connfd)

    {

        perror("accept");

        return -1;

    }

    return connfd;

}

int Acceptor::fd() const

{

    return _sock.fd();

}

```

#### TcpConnection类
```c++
//TcpConnection
#ifndef __TCPCONNECTION_H__

#define __TCPCONNECTION_H__

#include "Socket.h"

#include "SocketIO.h"

#include "InetAddress.h"

class TcpConnection

{

public:

    explicit TcpConnection(int fd);

    ~TcpConnection();

    void send(const string &msg);

    string receive();

    //为了方便调试的函数

    string toString();

private:

    //获取本端地址与对端地址

    InetAddress getLocalAddr();

    InetAddress getPeerAddr();

private:

    SocketIO _sockIO;

    //为了调试而加入的三个数据成员

    Socket _sock;

    InetAddress _localAddr;

    InetAddress _peerAddr;

};

#endif

//TcpConnection
#include "TcpConnection.h"

#include <iostream>

#include <sstream>

using std::cout;

using std::endl;

using std::ostringstream;

TcpConnection::TcpConnection(int fd)

: _sockIO(fd)

, _sock(fd)

, _localAddr(getLocalAddr())

, _peerAddr(getPeerAddr())

{

}

TcpConnection::~TcpConnection()

{

}

void TcpConnection::send(const string &msg)

{

    _sockIO.writen(msg.c_str(), msg.size());

}

string TcpConnection::receive()

{

    char buff[65535] = {0};

    _sockIO.readLine(buff, sizeof(buff));

    return string(buff);

}

string TcpConnection::toString()

{

    ostringstream oss;

    oss << _localAddr.ip() << ":"

        << _localAddr.port() << "---->"

        << _peerAddr.ip() << ":"

        << _peerAddr.port();

    return oss.str();

}

//获取本端的网络地址信息

InetAddress TcpConnection::getLocalAddr()

{

    struct sockaddr_in addr;

    socklen_t len = sizeof(struct sockaddr );

    //获取本端地址的函数getsockname

    int ret = getsockname(_sock.fd(), (struct sockaddr *)&addr, &len);

    if(-1 == ret)

    {

        perror("getsockname");

    }

    return InetAddress(addr);

}

//获取对端的网络地址信息

InetAddress TcpConnection::getPeerAddr()

{

    struct sockaddr_in addr;

    socklen_t len = sizeof(struct sockaddr );

    //获取对端地址的函数getpeername

    int ret = getpeername(_sock.fd(), (struct sockaddr *)&addr, &len);

    if(-1 == ret)

    {

        perror("getpeername");

    }

    return InetAddress(addr);

}
```

#### SocketIO类
```c++
//SocketIO.h
#ifndef __SOCKETIO_H__

#define __SOCKETIO_H__

class SocketIO

{

public:

    explicit SocketIO(int fd);

    ~SocketIO();

    int readn(char *buf, int len);

    int readLine(char *buf, int len);

    int writen(const char *buf, int len);

private:

    int _fd;

};

#endif

//SocketIO.cc
#include "SocketIO.h"

#include <stdio.h>

#include <unistd.h>

#include <errno.h>

#include <sys/types.h>

#include <sys/socket.h>

SocketIO::SocketIO(int fd)

: _fd(fd)

{

}

SocketIO::~SocketIO()

{

    close(_fd);

}

//len = 10000    1500/6     1000/1

int SocketIO::readn(char *buf, int len)

{

    int left = len;

    char *pstr = buf;

    int ret = 0;

    while(left > 0)

    {

        ret = read(_fd, pstr, left);

        if(-1 == ret && errno == EINTR)

        {

            continue;

        }

        else if(-1 == ret)

        {

            perror("read error -1");

            return -1;

        }

        else if(0 == ret)

        {

            break;

        }

        else

        {

            pstr += ret;

            left -= ret;

        }

    }

    return len - left;

}

int SocketIO::readLine(char *buf, int len)

{

    int left = len - 1;

    char *pstr = buf;

    int ret = 0, total = 0;

    while(left > 0)

    {

        //MSG_PEEK不会将缓冲区中的数据进行清空,只会进行拷贝操作

        ret = recv(_fd, pstr, left, MSG_PEEK);

        if(-1 == ret && errno == EINTR)

        {

            continue;

        }

        else if(-1 == ret)

        {

            perror("readLine error -1");

            return -1;

        }

        else if(0 == ret)

        {

            break;

        }

        else

        {

            for(int idx = 0; idx < ret; ++idx)

            {

                if(pstr[idx] == '\n')

                {

                    int sz = idx + 1;

                    readn(pstr, sz);

                    pstr += sz;

                    *pstr = '\0';//C风格字符串以'\0'结尾

                    return total + sz;

                }

            }

            readn(pstr, ret);//从内核态拷贝到用户态

            total += ret;

            pstr += ret;

            left -= ret;

        }

    }

    *pstr = '\0';

    return total - left;

}

int SocketIO::writen(const char *buf, int len)

{

    int left = len;

    const char *pstr = buf;

    int ret = 0;

    while(left > 0)

    {

        ret = write(_fd, pstr, left);

        if(-1 == ret && errno == EINTR)

        {

            continue;

        }

        else if(-1 == ret)

        {

            perror("writen error -1");

            return -1;

        }

        else if(0 == ret)

        {

            break;

        }

        else

        {

            pstr += ret;

            left -= ret;

        }

    }

    return len - left;

}
```


#### EventLoop类  ==重要== 
```C++
//EventLoop.h
#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

#include <sys/epoll.h>
#include <vector>
#include <map>
#include <memory>

using std::vector;
using std::map;
using std::shared_ptr;


class Acceptor;//前向声明
class TcpConnection;

class EventLoop
{
    using TcpConnectionPtr = shared_ptr<TcpConnection>;
public:
    EventLoop(Acceptor &acceptor);
    ~EventLoop();

    //循环与非循环
    void loop();
    void unloop();

    //封装epoll_wait的函数
    void waitEpollFd();

    //处理新的连接
    void handleNewConnection();

    //处理老的连接上的数据的收发
    void handleMessage(int fd);

    //封装epoll_create函数
    int createEpollFd();

    //将文件描述符放在红黑树上进行监听
    void addEpollReadFd(int fd);
    
    //将文件描述符从红黑树上取消监听
    void delEpollReadFd(int fd);

private:
    int _epfd;//是epoll_create创建出来的文件描述符
    vector<struct epoll_event> _evtList;//存放满足条件的文件描述符的数据结构
    bool _isLooping;//标识循环运行与否
    Acceptor &_acceptor;//连接器Acceptor的引用
    map<int, TcpConnectionPtr> _conns;//存放文件描述符与TcpConnection的键值对

};

#endif

//EventLoop.cc
#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include <unistd.h>
#include <iostream>

using std::cout;
using std::endl;

EventLoop::EventLoop(Acceptor &acceptor)
: _epfd(createEpollFd())
, _evtList(1024)
, _isLooping(false)
, _acceptor(acceptor)
{
    //获取listenfd（也就是socket返回的文件描述符）
    //并且将listenfd放在红黑树上进行监听
    int listenfd = _acceptor.fd();
    addEpollReadFd(listenfd);
}

EventLoop::~EventLoop()
{
    close(_epfd);
}

//循环与非循环
void EventLoop::loop()
{
    _isLooping = true;
    while(_isLooping)
    {
        waitEpollFd();
    }
}

void EventLoop::unloop()
{
    _isLooping = false;
}

//封装epoll_wait的函数
void EventLoop::waitEpollFd()
{
    int nready = 0;
    do{
        nready = epoll_wait(_epfd, &*_evtList.begin(), _evtList.size(), 3000);
    }while((-1 == nready && errno == EINTR));

    if(-1 == nready)
    {
        perror("-1 == nready");
        return;
    }
    else if(0 == nready)
    {
        cout << ">>epoll_wait timeout!!!" << endl;
    }
    else 
    {
        //考虑一个问题，vector有可能越界的问题，需要手动扩容
        //nready == -1(int--->size_t)
        //for(size_t idx = 99; idx > 0; idx -=2)
        //{
        //
        //}
        if((int)_evtList.size() == nready)
        {
            _evtList.resize(2 * nready);
        }

        for(int idx = 0; idx < nready; ++idx)
        {
            int fd = _evtList[idx].data.fd;
            int listenfd = _acceptor.fd();
            if(fd == listenfd)
            {
                //有新的连接请求上来
                handleNewConnection();
            }
            else
            {
                //处理老的连接
                handleMessage(fd);
            }
        }
    }
}

//处理新的连接
void EventLoop::handleNewConnection()
{
    //调用Acceptor的accept函数，如果该函数的返回值
    //connfd是正常值，就表明三次握手建立成功了
    int connfd = _acceptor.accept();
    if(connfd < 0)
    {
        perror("handleNewConnection");
        return;
    }
    //监听connfd
    addEpollReadFd(connfd);

    //创建出TcpConnection的对象
    /* TcpConnection con(connfd); */
    TcpConnectionPtr con(new TcpConnection(connfd));

    //将connfd与TcpConnection的键值对存放在map中
    /* _conns.insert(std::make_pair(connfd, con)); */
    _conns[connfd] = con;
}

//处理老的连接上的数据的收发
void EventLoop::handleMessage(int fd)
{
    auto it = _conns.find(fd);
    if(it != _conns.end())
    {
        //通过连接接收客户端的数据
        string msg = it->second->receive();
        cout << ">>recv from client : " << msg << endl;
        //可以处理msg
        //处理完msg之后，可以将处理之后的msg
        //发给客户端
        it->second->send(msg);
    }
    else
    {
        cout << "该连接是不存在" << endl;
        return;
    }
}

//封装epoll_create函数
int EventLoop::createEpollFd()
{
    int fd = epoll_create(10);
    if(fd < 0)
    {
        perror("createEpollFd");
        return -1;
    }

    return fd;
}

//将文件描述符放在红黑树上进行监听
void EventLoop::addEpollReadFd(int fd)
{
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLLIN;

    int ret = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &evt);
    if(ret < 0)
    {
        perror("addEpollReadFd");
        return;
    }
}

//将文件描述符从红黑树上取消监听
void EventLoop::delEpollReadFd(int fd)
{
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLLIN;

    int ret = epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, &evt);
    if(ret < 0)
    {
        perror("delEpollReadFd");
        return;
    }

}

```