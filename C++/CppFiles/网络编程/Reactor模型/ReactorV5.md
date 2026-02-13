### ReactorV5版本
#Reactor  #重点  #网络编程 
本版本只是在上一个版本的基础上，进一步的做封装，将线程池与TcpServer对象进一步封装。
```C++
ThreadPool _pool;
TcpServer _server;

EchoServer(size_t threadNum, size_t queSize, const string &ip, unsignedshort port);
~EchoServer();

//让服务器启动与停止
void start();
void stop();

//三个回调函数
void onNewConnection(const TcpConnectionPtr &con);
void onMessage(const TcpConnectionPtr &con);
void onClose(const TcpConnectionPtr &con);
```
#### 1、类图

![image-20240711153211999](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240711153211999.png?lastModify=1720702254)

#### 2、重要函数

![image-20240711153606323](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240711153606323.png?lastModify=1720702254)

#### 3、进线程通信方式eventfd

##### 作用
从Linux 2.6.27版本开始，新增了不少系统调用，其中包括eventfd，它的主要是用于进程或者线程间通信(如通知/等待机制的实现)

##### 函数接口
```C++
#include <sys/eventfd.h>

int eventfd(unsigned int initval, int flags);

initval：初始化计数器值，该值保存在内核。
flags:如果是2.6.26或之前版本的内核，flags 必须设置为0。
flags支持以下标志位：
EFD_NONBLOCK 类似于使用O_NONBLOCK标志设置文件描述符。
EFD_CLOEXEC 类似open以O_CLOEXEC标志打开， O_CLOEXEC 应该表示执行exec()时，之前通
过open()打开的文件描述符会自动关闭。
返回值：函数返回一个文件描述符，与打开的其他文件一样，可以进行读写操作。
```

##### eventfd支持的操作

eventfd系统调用返回的是文件描述符，该文件描述符与以前学习的文件描述符一样，可以读、写、监听。

read函数：如果计数器A的值不为0时，读取成功，获得到该值；如果A的值为0，非阻塞模式时，会直接返回失败，并把error置为EINVAL；如果为阻塞模式，一直会阻塞到A为非0为止。

write函数：将缓冲区写入的8字节整形值加到内核计数器上，即会增加8字节的整数在计数器A上，如果其值达到0xfffffffffffffffe时，就会阻塞（在阻塞模式下），直到A的值被read。

select/poll/epoll：支持被io多路复用监听。当内核计数器的值发生变化时，就会触发事件。

通过对eventfd函数返回的文件描述符进行通信。一个进程或者线程A执行read操作，如果内核计数器的值为0，并且是阻塞模式，那么A就会阻塞；另外一个进程或者线程B执行write操作，就会向内核计数器写，那么阻塞的A发现内核计数器的值不为0，就会被触发，那么两个进程或者线程A与B就达到通信的目的了。
#### 4、Reactor各版本的使用场景

1、执行read与send操作的线程，将其称为IO线程，主要就是与网络打交道。

2、将执行decode、compute、encode的线程，称为计算线程。

3、基础的Reactor适用于计算操作比较简单，而主要的事件花费在IO操作。称为IO密集型。

4、如果处理业务逻辑比较耗时的时候，就可以再加上线程池，让线程池去处理业务逻辑，并发处理业务逻辑。称为计算密集型。

### 代码实现
#### NonCopyable类
```c++
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
```c++
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
//TcpConnection.h
#ifndef __TCPCONNECTION_H__
#define __TCPCONNECTION_H__

#include "Socket.h"
#include "SocketIO.h"
#include "InetAddress.h"

#include <memory>
#include <functional>

using std::shared_ptr;
using std::function;

class EventLoop;//前向声明

class TcpConnection
: public std::enable_shared_from_this<TcpConnection>
{
    using TcpConnectionPtr = shared_ptr<TcpConnection>;
    using TcpConnectionCallback = function<void(const TcpConnectionPtr &)>;
public:
    explicit TcpConnection(int fd, EventLoop *loop);
    ~TcpConnection();
    void send(const string &msg);
    void sendInLoop(const string &msg);
    string receive();
    bool isClosed() const;

    //为了方便调试的函数
    string toString();

private:
    //获取本端地址与对端地址
    InetAddress getLocalAddr();
    InetAddress getPeerAddr();

public:
    //三个回调的注册
    void setNewConnectionCallback(const TcpConnectionCallback &cb);
    void setMessageCalback(const TcpConnectionCallback &cb);
    void setCloseCallback(const TcpConnectionCallback &cb);

    //三个回调的执行
    void handleNewConnectionCallback();
    void handleMessageCallback();
    void handleCloseCallback();


private:
    EventLoop *_loop;//让TcpConnection知道EventLoop存在
    SocketIO _sockIO;

    //为了调试而加入的三个数据成员
    Socket _sock;
    InetAddress _localAddr;
    InetAddress _peerAddr;

    //添加三个数据成员，对应的就是三个事件
    TcpConnectionCallback _onNewConnectionCb;//连接建立
    TcpConnectionCallback _onMessageCb;//消息到达（文件描述符可读）
    TcpConnectionCallback _onCloseCb;//连接建立
};

#endif

//TcpConnection.cc
#include "TcpConnection.h"
#include "EventLoop.h"
#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using std::ostringstream;

TcpConnection::TcpConnection(int fd, EventLoop *loop)
: _loop(loop)
, _sockIO(fd)
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

//需要做什么？
//此处的参数msg就是线程池处理好之后的可以发送出去的数据
//需要将其传递个EventLoop
//因为EventLoop本身并没有发送数据的能力，即使将线程池处理好之后的
//msg发给EventLoop，EventLoop也不能将msg发给客户端
//所有需要将发送的数据msg，以及发送数据能力的send函数，以及执行
//send函数的连接TcpConnection的对象都传递给（发送给）EventLoop，
//这样才能将数据发给客户端
void TcpConnection::sendInLoop(const string &msg)
{
    if(_loop)
    {
        /* function<void()> f = bind(&TcpConnection::send, this, msg); */
        //void runInLoop(function<void()> &&)
        _loop->runInLoop(bind(&TcpConnection::send, this, msg));
    }
}
string TcpConnection::receive()
{
    char buff[65535] = {0};
    _sockIO.readLine(buff, sizeof(buff));

    return string(buff);
}

bool TcpConnection::isClosed() const
{
    char buff[10]= {0};
    int ret = ::recv(_sock.fd(), buff, sizeof(buff), MSG_PEEK);

    return (0 == ret);
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

//三个回调的注册
void TcpConnection::setNewConnectionCallback(const TcpConnectionCallback &cb)
{
    _onNewConnectionCb = cb;
}

void TcpConnection::setMessageCalback(const TcpConnectionCallback &cb)
{
    _onMessageCb = cb;
}

void TcpConnection::setCloseCallback(const TcpConnectionCallback &cb)
{
    _onCloseCb = cb;
}

//三个回调的执行
void TcpConnection::handleNewConnectionCallback()
{
    if(_onNewConnectionCb)
    {
        /* shared_ptr<TcpConnection> ttt(this); */
        /* _onNewConnectionCb(shared_ptr<TcpConnection>(this)); */
        _onNewConnectionCb(shared_from_this());
    }
    else
    {
        cout << "_onNewConnectionCb == nullptr" << endl;
    }
}

void TcpConnection::handleMessageCallback()
{
    if(_onMessageCb)
    {
        _onMessageCb(shared_from_this());
    }
    else
    {
        cout << "_onMessageCb == nullptr" << endl;
    }
}

void TcpConnection::handleCloseCallback()
{
    if(_onCloseCb)
    {
        _onCloseCb(shared_from_this());
    }
    else
    {
        cout << "_onCloseCb == nullptr" << endl;
    }
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

#### EventLoop类
```c++
//EventLoop.h
#ifndef __EVENTLOOP_H__
#define __EVENTLOOP_H__

#include <sys/epoll.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>

using std::vector;
using std::map;
using std::shared_ptr;
using std::function;
using std::mutex;
using std::lock_guard;


class Acceptor;//前向声明
class TcpConnection;

using TcpConnectionPtr = shared_ptr<TcpConnection>;
using TcpConnectionCallback = function<void(const TcpConnectionPtr &)>;
using Functor = function<void()>;

class EventLoop
{
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

    //三个回调的注册
    void setNewConnectionCallback(TcpConnectionCallback &&cb);
    void setMessageCalback(TcpConnectionCallback &&cb);
    void setCloseCallback(TcpConnectionCallback &&cb);

public:
    //创建用于通信的文件描述符
    int createEventFd();

    //封装read的函数
    void handleRead();

    //封装write的函数
    void wakeup();

    //执行所有的任务
    void doPengdingFunctors();

    void runInLoop(Functor &&cb);

private:
    int _epfd;//是epoll_create创建出来的文件描述符
    vector<struct epoll_event> _evtList;//存放满足条件的文件描述符的数据结构
    bool _isLooping;//标识循环运行与否
    Acceptor &_acceptor;//连接器Acceptor的引用
    map<int, TcpConnectionPtr> _conns;//存放文件描述符与TcpConnection的键值对

    //添加三个数据成员，对应的就是三个事件
    TcpConnectionCallback _onNewConnectionCb;//连接建立
    TcpConnectionCallback _onMessageCb;//消息到达（文件描述符可读）
    TcpConnectionCallback _onCloseCb;//连接建立

    int _evtfd;//用于通信的文件描述符
    vector<Functor> _pendings;//待执行的任务（存放任务的容器）
    mutex _mutex;//互斥访问vector

};

#endif

//EventLoop.cc
#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include <unistd.h>
#include <sys/eventfd.h>
#include <iostream>

using std::cout;
using std::endl;

EventLoop::EventLoop(Acceptor &acceptor)
: _epfd(createEpollFd())
, _evtList(1024)
, _isLooping(false)
, _acceptor(acceptor)
, _evtfd(createEventFd())
, _mutex()
{
    //获取listenfd（也就是socket返回的文件描述符）
    //并且将listenfd放在红黑树上进行监听
    int listenfd = _acceptor.fd();
    addEpollReadFd(listenfd);

    //需要监听用于通信的文件描述符_evtfd
    addEpollReadFd(_evtfd);
}

EventLoop::~EventLoop()
{
    close(_epfd);
    close(_evtfd);
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
            else if(fd == _evtfd)//用于通信的文件描述符就绪了
            {
                handleRead();
                //然后执行所有的任务
                doPengdingFunctors();
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
    TcpConnectionPtr con(new TcpConnection(connfd, this));

    //当连接TcpConnection对象创建出来之后，需要将三个回调
    //从EventLoop中传递给TcpConnection的对象
    con->setNewConnectionCallback(_onNewConnectionCb);//连接建立
    con->setMessageCalback(_onMessageCb);//消息达到
    con->setCloseCallback(_onCloseCb);//连接断开

    //将connfd与TcpConnection的键值对存放在map中
    /* _conns.insert(std::make_pair(connfd, con)); */
    _conns[connfd] = con;

    //连接建立的事件的触发时机已经到达了，可以执行
    con->handleNewConnectionCallback();//连接建立的事件的执行
}

//处理老的连接上的数据的收发
void EventLoop::handleMessage(int fd)
{
    auto it = _conns.find(fd);
    if(it != _conns.end())
    {
        //拿到这个连接之后，如何判断连接是不是断开的呢
        //如果连接断开就可以执行连接断开的事件
        //否则就执行消息到达的事件
        bool flag = it->second->isClosed();
        if(flag)
        {
            //连接断开
            it->second->handleCloseCallback();//连接断开的事件的触发时机
            //将fd从红黑树上删除
            delEpollReadFd(fd);
            //还需要将连接从map中删除
            _conns.erase(it);
        }
        else
        {
            //连接没有断开，可以正常收发数据
            it->second->handleMessageCallback();//消息到达
        }
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

void EventLoop::setNewConnectionCallback(TcpConnectionCallback &&cb)
{
    _onNewConnectionCb = std::move(cb);
}

void EventLoop::setMessageCalback(TcpConnectionCallback &&cb)
{
    _onMessageCb = std::move(cb);
}

void EventLoop::setCloseCallback(TcpConnectionCallback &&cb)
{
    _onCloseCb = std::move(cb);
}

//创建用于通信的文件描述符
int EventLoop::createEventFd()
{
    int fd = eventfd(10, 0);
    if(fd < 0)
    {
        perror("createEventFd");
        return -1;
    }

    return fd;
}

//封装read的函数
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t ret = read(_evtfd, &one, sizeof(uint64_t));
    if(ret != sizeof(uint64_t))
    {
        perror("handleRead");
        return;
    }
}

//封装write的函数
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t ret = write(_evtfd, &one, sizeof(uint64_t));
    if(ret != sizeof(uint64_t))
    {
        perror("wakeup");
        return;
    }
}

//执行所有的任务(先写在这里，然后再来看看有没有优化的可能性)
void EventLoop::doPengdingFunctors()
{
    vector<Functor> tmp;
    {
        //尽量的让锁的粒度要小一些（范围）
        lock_guard<mutex> lg(_mutex);
        tmp.swap(_pendings);
    }

    for(auto &cb : tmp)
    {
        //“任务”就是要发送的数据msg，以及发送数据的能力的send
        //以及发送的连接TcpConnection对象
        cb();//这就是执行所有存放在vector中的"任务"
    }
}

void EventLoop::runInLoop(Functor &&cb)
{
    //这里写什么东西?
    {
        lock_guard<mutex> lg(_mutex);
        _pendings.push_back(std::move(cb));
    }

    wakeup();//通知/唤醒Reactor去将数据发给客户端
}

```

#### EchoServer类
```c++
//EchoServer.h
#ifndef __ECHOSERVER_H__
#define __ECHOSERVER_H__

#include "ThreadPool.h"
#include "TcpServer.h"

class MyTask
{
public:
    MyTask(const string &msg, const TcpConnectionPtr &con);
    void process();
private:
    string _msg;
    TcpConnectionPtr _con;
};

class EchoServer
{
public:
    EchoServer(size_t threadNum, size_t queSize
               , const string &ip
               , unsigned short port);
    ~EchoServer();

    //服务器的启动与停止
    void start();
    void stop();

    //三个回调
    void onNewConnection(const TcpConnectionPtr &con);
    void onMessage(const TcpConnectionPtr &con);
    void onClose(const TcpConnectionPtr &con);

private:
    ThreadPool _pool;
    TcpServer _server;

};

#endif

//EchoServer.cc
#include "EchoServer.h"
#include "TcpConnection.h"
#include <iostream>

using std::cout;
using std::endl;

MyTask::MyTask(const string &msg, const TcpConnectionPtr &con)
: _msg(msg)
, _con(con)
{

}

void MyTask::process()
{
    //业务逻辑的处理
    _con->sendInLoop(_msg);
}

EchoServer::EchoServer(size_t threadNum, size_t queSize
                       , const string &ip
                       , unsigned short port)
: _pool(threadNum, queSize)
, _server(ip, port)
{
    /* start(); */
}

EchoServer::~EchoServer()
{

}

//服务器的启动与停止
void EchoServer::start()
{
    _pool.start();

    using namespace  std::placeholders;
    //将所有的回调都写到这里来
    _server.setAllCallback(std::bind(&EchoServer::onNewConnection, this, _1)
                           , std::bind(&EchoServer::onMessage, this, _1)
                           , std::bind(&EchoServer::onClose, this, _1));

    _server.start();
}

void EchoServer::stop()
{
    _pool.stop();
    _server.stop();
}

//三个回调
void EchoServer::onNewConnection(const TcpConnectionPtr &con)
{
    cout << con->toString() << " has connected!!!" << endl;
}

void EchoServer::onMessage(const TcpConnectionPtr &con)
{
    string msg = con->receive();
    cout << ">>>>recv from client : " << msg << endl;

    MyTask task(msg, con);
    _pool.addTask(std::bind(&MyTask::process, task));
}

void EchoServer::onClose(const TcpConnectionPtr &con)
{
    cout << con->toString() << " has closed!!!" << endl;
}

```

#### TaskQueue类
```c++
// TaskQueue.h
#ifndef __TASKQUEUE_H__
#define __TASKQUEUE_H__

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

using std::queue;
using std::mutex;
using std::unique_lock;
using std::condition_variable;
using std::function;


using ElemType = function<void()>;

class TaskQueue
{
public:
    TaskQueue(size_t queSize);
    ~TaskQueue();

    //生产任务与消费任务
    void push(ElemType &&taskcb);
    ElemType pop();

    //判断任务队列是不是空或者满
    bool full() const;
    bool empty() const;

    //唤醒所有等待在_notEmpty条件变量上的线程
    void wakeup();

private:
    size_t _queSize;//任务队列的大小
    queue<ElemType> _que;//存放数据的数据结构
    mutex _mutex;//C++11中的互斥锁
    condition_variable _notFull;//非满条件变量
    condition_variable _notEmpty;//非空条件变量
    bool _flag;//标志位

};

#endif

// TaskQueue.cc
#include "TaskQueue.h"

TaskQueue::TaskQueue(size_t queSize)
: _queSize(queSize)
, _que()
, _mutex()
, _notFull()
, _notEmpty()
, _flag(true)
{

}

TaskQueue::~TaskQueue()
{

}

//生产任务与消费任务
void TaskQueue::push(ElemType &&taskcb)
{
    //1、先上锁
    unique_lock<mutex> ul(_mutex);
    //2、判满
    while(full())
    {
        //上半部：先上锁、解锁、睡眠
        //下半部：被唤醒、抢到锁、函数返回
        //2.1、如果是满的，睡眠
        _notFull.wait(ul);

    }
    //3、生产数据
    _que.push(std::move(taskcb));
    //4、唤醒消费者
    _notEmpty.notify_one();
}

ElemType TaskQueue::pop()
{
    //1、先上锁
    unique_lock<mutex> ul(_mutex);
    //2、判空
    while(empty() &&_flag)
    {
        //上半部：先上锁、解锁、睡眠
        //下半部：被唤醒、抢到锁、函数返回
        //2.1、如果是满的，睡眠
        _notEmpty.wait(ul);

    }
    if(_flag)
    {
        //3、消费数据
        ElemType tmp = _que.front();
        _que.pop();
        //4、唤醒生产者
        _notFull.notify_one();
        //解锁

        return tmp;
    }
    else
    {
        return nullptr;
    }
}

//判断任务队列是不是空或者满
bool TaskQueue::full() const
{
    return _queSize == _que.size();
}

bool TaskQueue::empty() const
{
    return 0 == _que.size();
}

void TaskQueue::wakeup()
{
    _flag = false;
    _notEmpty.notify_all();
}

```

#### TcpServer类
```c++
//TcpServer.h
#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "Acceptor.h"
#include "EventLoop.h"

class TcpServer
{
public:
    TcpServer(const string &ip, unsigned short port);
    ~TcpServer();

    //服务器启动与停止
    void start();
    void stop();

    //将三个回调一次都注册进来
    void setAllCallback(TcpConnectionCallback &&cb1
                       , TcpConnectionCallback &&cb2
                       , TcpConnectionCallback &&cb3);

private:
    Acceptor _acceptor;
    EventLoop _loop;

};

#endif


//TcpServer.cc
#include "TcpServer.h"

TcpServer::TcpServer(const string &ip, unsigned short port)
: _acceptor(ip, port)
, _loop(_acceptor)
{

}

TcpServer::~TcpServer()
{

}

//服务器启动与停止
void TcpServer::start()
{
    _acceptor.ready();
    _loop.loop();
}

void TcpServer::stop()
{
    _loop.unloop();
}

//将三个回调一次都注册进来
void TcpServer::setAllCallback(TcpConnectionCallback &&cb1
                              , TcpConnectionCallback &&cb2
                              , TcpConnectionCallback &&cb3)
{
    _loop.setNewConnectionCallback(std::move(cb1));
    _loop.setMessageCalback(std::move(cb2));
    _loop.setCloseCallback(std::move(cb3));
}

```

#### ThreadPool类
```c++
//ThreadPool.h
#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "TaskQueue.h"
#include <thread>
#include <vector>
#include <functional>

using std::thread;
using std::vector;
using std::function;

using Task = function<void()>;

class ThreadPool
{
public:
    ThreadPool(size_t threadNum, size_t queSize);
    ~ThreadPool();

    //线程的启动与停止
    void start();
    void stop();

    //添加任务与获取任务
    void addTask(Task &&taskcb);
private:
    Task getTask();

    //线程池交给工作线程thread执行的任务
    void doTask();


private:
    size_t _threadNum;//子线程的数目
    vector<thread> _threads;//存放thread的数据结构
    size_t _queSize;//任务队列的大小
    TaskQueue _taskQue;//任务队列
    bool _isExit;//标识线程池是否退出的标志位

};

#endif

//ThreadPool.cc
#include "ThreadPool.h"
#include <iostream>

using std::cout;
using std::endl;

ThreadPool::ThreadPool(size_t threadNum, size_t queSize)
: _threadNum(threadNum)
, _queSize(queSize)
, _taskQue(_queSize)
, _isExit(false)
{

}

ThreadPool::~ThreadPool()
{

}

//线程的启动与停止
void ThreadPool::start()
{
    //创建线程，并且存放在容器中
    for(size_t idx = 0; idx != _threadNum; ++idx)
    {
        _threads.push_back(thread(&ThreadPool::doTask, this));
    }
}

void ThreadPool::stop()
{
    //只要任务没有执行完，就不能让主线程继续向下执行
    while(!_taskQue.empty())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    //线程池要退出，那么标志位可以设置为true
    _isExit = true;
    //唤醒所有等待在_notEmpty上的线程
    _taskQue.wakeup();

    for(auto &th : _threads)
    {
        th.join();
    }
}

//添加任务与获取任务
void ThreadPool::addTask(Task &&taskcb)
{
    if(taskcb)
    {
        _taskQue.push(std::move(taskcb));
    }
}

Task ThreadPool::getTask()
{
    return _taskQue.pop();
}

//线程池交给工作线程thread执行的任务
void ThreadPool::doTask()
{
    while(!_isExit)
    {
        //线程池中的线程需要先获取任务，然后执行任务
        Task taskcb = getTask();
        if(taskcb)
        {
            /* ptask->process();//会体现出多态 */
            taskcb();//回调
        }
        else
        {
            cout << "taskcb == nullptr" << endl;
        }
    }
}

```

####   TestEchoServer.cc
```c++
#include "EchoServer.h"
#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char **argv)
{
    EchoServer server(4, 10, "127.0.0.1", 8888);
    server.start();
    return 0;
}


```
