### ReactorV1版本
#Reactor  #重点  #网络编程 
#### 1、类的设计

Socket类：将所有与套接字相关的操作全部封装到一个类中。包括：套接字的创建、套接字的关闭、套接字的获取。

InetAddress类：将所有与地址相关的操作封装到一个类中。包括：通过ip与port创建地址对象、通过该对象获取ip、获取端口号。

Acceptor类：连接器类，将服务器的所有基本操作全部封装到该类中。包括：地址复用、端口复用、bind、listen、accept。

TcpConnection类：Tcp连接类。如果上面的Acceptor类创建的对象调用accept函数，并且有正确范围结果，那么就表明三次握手建立成功，就将这条连接称为Tcp连接，也就是TcpConnection类创建的对象。就可以通过该连接进行收发数据。需要封装发送数据的函数send，接收数据的函数receive函数。

SocketIO类：该类的作用就是真正进行数据收发的类，也就是封装的read/recv、send/write系统调用。

#### 2、类图设计

![image-20240708164944485](file://D:/ProgramData/Material/C++Video/03_CppBoost_%E5%93%A9%E5%93%A9/CppBoostDay09/1_%E7%AC%94%E8%AE%B0/C++57%E6%9C%9F%E6%8F%90%E9%AB%98%E9%83%A8%E5%88%86.assets/image-20240708164944485.png?lastModify=1720702254)3、几个新函数
```C++
#include <sys/socket.h>  
//通过sockfd文件描述符获取本端地址信息，并且存放在addr中  
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);  
​  
//通过sockfd文件描述符获取对端地址信息，并且存放在addr中（对端地址）  
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);  
​  
//flags标志位如果为MSG_PEEK，只会将数据从内核态拷贝到用户态，而不会从内核缓冲区中移走  
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

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