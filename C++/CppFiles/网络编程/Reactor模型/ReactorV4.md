### ReactorV4版本
#Reactor  #重点  #网络编程 
#### 1、原理图

![image-20240710112429241](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240710112429241.png?lastModify=1720702254)
##### 原理分析
	本模型相对于基本的Reactor模型而言，可以将业务逻辑的处理交给线程池来进行处理，这样能够提高Reactor线程的I/O响应，不至于因为一些耗时的业务逻辑而延迟对后面I/O请求的处理，可以进一步提高性能。
	但是在本模型中，Reactor在获得连接后，需要采用TcpConnection对象读取数据，也就是执行Read操作（代码中是receive函数），然后将获取到的数据交给线程池的对象进行处理，其实也即是将数据当做任务交给线程池即可，这个比较简单，之前的线程池版本中就实现了添加任务与获取任务，然后让线程池中的线程处理任务，当线程池处理好业务逻辑的处理(decode、compute、encode)后，线程池中的线程（也就是线程池对象）需要通知Reactor对象，并将处理好之后的数据发送给客户端，这里就存在一个问题，线程池对象如何与Reactor线程之间进行通信？通信之后数据如何传递给Reactor？Reactor如何将数据发送给客户端？

问题一：线程池与Reactor之间通信的方式有哪些？这里可以采用系统调用eventfd的方式（原理如何、怎么使用后面会讲解），学会了就可以解决本版本的问题,可以跳转到eventfd，学习其原理学习以及使用方式。

问题二：数据如何传递给Reactor？这里面会将数据通过相应的函数传递给Reactor，这个与具体的代码设计相关，因为在第三个版本中，线程池中会有TcpConnection的对象，而TcpConnection对象都是在EventLoop中创建的，所以很好传递到Reactor/EventLoop中。

问题三：Reactor如何将数据发送给客户端？很简单，借助TcpConnection对象的send函数发送即可。
#### 2、类图设计
	将用于通信的eventfd系统调用（也就是线程之间通信的代码）封装到ReactorV3版本的EventLoop类中，然后只要线程池中的线程在执行完业务逻辑的处理之后，调用wakeup唤醒Reactor线程（注意：名为唤醒，其实Reactor线程并不会真正的阻塞，而是负责监听用于通信的文件描述符即可，只要就绪，就可以处理发送任务给客户端了，如果真的Reactor在睡眠，那么新的连接上来了，线程不就阻塞住了吗），也就是通知Reactor线程即可，然后执行发送操作，将数据发送给客户端即可。

![image-20240711141201715](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240711141201715.png?lastModify=1720702254)

![image-20240711141507840](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240711141507840.png?lastModify=1720702254)


#### 3、流程图

![image-20240711150155497](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240711150155497.png?lastModify=1720702254)

```c++
数据成员：
	 就是将线程之间进行通信的文件描述符 int _evtfd
	 
	线程池处理完之后要通过Reactor发送给客户端的任务比较多，所以就用容器存起来vector<Functor> _pengdings
	
	多个线程对容器vector的访问是互斥的，需要加锁 mutex _mutex
	
成员函数：
createEventFd函数：调用eventfd系统调用创建用于通信的文件描述符。

handleRead函数：里面封装了read函数，该函数读取eventfd返回的文件描述符。

wakeup函数：里面封装了write函数，该函数向eventfd返回的文件描述符中写数据，也就是唤醒阻塞的线程，从而达到通信的目的。

doPengdingFunctors函数：就是将存放在vector中的任务进行执行，这里的任务比较多，所以需要遍历。

runInLoop函数：vector中的任务是如何传递进来的？本函数就是将线程池处理好之后的要发送给客户端的数据，最终传递到vector中的，主要是做这个事情。

思考题：vector存放的任务，这里的任务到底是什么，难道只是线程池处理好要发送给客户端的数据
吗？
```



--- 
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

####   TestTcpServer.cc
```c++
#include "TcpConnection.h"
#include "TcpServer.h"
#include "ThreadPool.h"
#include <iostream>
#include <unistd.h>

using std::cout;
using std::endl;

ThreadPool *gPool = nullptr;

class MyTask
{
public:
    MyTask(const string &msg, const TcpConnectionPtr &con)
    : _msg(msg)
    , _con(con)
    {

    }
    void process()
    {
        //现在将msg接受过来之后，打包成了任务传递到了线程池中，
        //而线程池中处理的任务就是通过工作线程执行process进行的
        _msg;
        //....
        //....
        //数据_msg处理完成之后，需要将数据发出去
        //需要有连接的对象将线程池处理好之后的数据发送出去
        //能不能直接在此处创建出一个TcpConnection的对象
        //
        //线程池中的线程在处理好业务逻辑之后，可以立马通知
        //Reactor/EventLoop进行收数据，也就是表明线程池中
        //的子线程需要通知Reactor收数据
        //问题1：线程池中的线程如何通知Reactor呢？
        //解析1：涉及到线程之间的通信问题。如果学会了eventfd，在
        //进程或者线程之间通信的方式，就可以让线程池中的线程与Reactor
        //通信
        //
        //问题2：线程池中的线程如何将处理好之后的数据发给Reactor呢?
        //这个函数的作用就是通过连接_con将线程池处理好之后的数据发送
        //到EventLoop，让EventLoop再将处理好之后的数据发给客户端
        /* _con->send(_msg); */
        //要让TcpConnection将处理好之后的msg发给Reactor/EventLoop
        //那么TcpConnection的对象肯定需要指向Reactor/EventLoop
        //的存在,就需要将EventLoop类型的指针或者引用作为
        //TcpConnection的数据成员
        _con->sendInLoop(_msg);
    }
private:
    string _msg;
    TcpConnectionPtr _con;
};

//连接建立
void onNewConnection(const TcpConnectionPtr &con)
{
    cout << con->toString() << " has connected!!!" << endl;
}

//消息到达
void onMessage(const TcpConnectionPtr &con )
{
    string msg = con->receive();
    cout << ">>>>recv from client : " << msg << endl;
    //接收到数据之后，可以进行业务逻辑的处理
    //处理完成之后，将其发送给客户端
    //如果在此处处理业务逻辑，并且业务逻辑的处理非常耗时，所以本
    //模型（版本）就存在缺陷，所以需要将业务逻辑的处理交给线程池
    //将接受过来的数据msg打包成任务,交给线程池
    MyTask task(msg, con);
    gPool->addTask(std::bind(&MyTask::process, task));

    /* con->send(msg); */
}

//连接断开
void onClose(const TcpConnectionPtr &con)
{
    cout << con->toString() << " has closed!!!" << endl;
}

void test()
{
    ThreadPool pool(4, 10);
    pool.start();
    gPool = &pool;

    TcpServer server("127.0.0.1", 8888);
    server.setAllCallback(std::move(onNewConnection)
                          , std::move(onMessage)
                          , std::move(onClose));
    server.start();
}

int main(int argc, char **argv)
{
    test();
    return 0;
}



```

