# IO多路复用
#重点  #网络编程  #IO多路复用
## select

```cpp
**接口解析**
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

**int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
struct timeval *timeout);**

**nfds:监控的文件描述符集里最大文件描述符+1，因为此参数会告诉内核检测前多少个文件描述符的状态。readfs/writes/exceptfds:监控有读数据/写数据/异常发生到达文件描述符集合，三个都是传入传出参数。
timeout:定时阻塞监控时间，3中情况：
1、NULL，永远等下去
2、设置timeval，等待固定时间
3、设置timeval里时间均为0，检查描述字后立即返回，轮询。
fd_set：本质是个位图。**

struct timeval
{
long tv_sec; /* seconds */
long tv_usec; /* microseconds */
};

返回值：
成功：所监听的所有的监听集合中，满足条件的总数。失败：返回-1.

**void FD_ZERO(fd_set *set);//将set清空为0
void FD_SET(int fd, fd_set *set);//将fd设置到set集合中
void FD_CLR(int fd, fd_set *set);//将fd从set中清除出去
int FD_ISSET(int fd, fd_set *set);//判断fd是否在集合中**
```

### select优缺点

1、文件描述符上限（1024），同时监听的文件描述符1024个，历史原因，不好修改，除非重新编译Linux内核。

2、当监听的文件描述符个数比较稀疏的时候（比如3， 600， 1023），循环判断比较麻烦，所以需要自定义数据结构：数组3、监听集合与满足监听条件的集合是同一个，需要将原有集合保存。

server代码

```cpp
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#define SERV_PORT 8888
int main()
{
int listenfd, connfd, sockfd;
struct sockaddr_in serv_addr, clie_addr;socklen_t clie_addr_len;
int ret, maxfd, maxi, i, j, nready, nByte;fd_set rset, allset;int client[FD_SETSIZE];
char buf[BUFSIZ], str[BUFSIZ];
listenfd = socket(AF_INET, SOCK_STREAM, 0);
if(-1 == listenfd)
{
perror("socket error");
exit(-1);} int opt = 1;
setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
bzero(&serv_addr, sizeof(serv_addr));serv_addr.sin_family = AF_INET;serv_addr.sin_port = htons(SERV_PORT);serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
ret = bind(listenfd, (struct sockaddr *)&serv_addr,
sizeof(serv_addr));

if(-1 == ret)
{
perror("bind error");
exit(-1);} ret = listen(listenfd, 128);
if(-1 == ret)
{
perror("listen error");
exit(-1);} maxfd = listenfd;
maxi = -1;
for(i = 0; i < FD_SETSIZE; ++i)
{
client[i] = -1;
} FD_ZERO(&allset);FD_SET(listenfd, &allset);
while(1){
rset = allset;
nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
if(nready < 0)
{
perror("select error");
exit(-1);} if(FD_ISSET(listenfd, &rset))
{
clie_addr_len = sizeof(clie_addr);
connfd = accept(listenfd, (struct sockaddr *)&clie_addr,
&clie_addr_len);
if(-1 == connfd)
{
perror("accept error");
exit(-1);} printf("receive from %s from port %d\n",
inet_ntop(AF_INET, &clie_addr.sin_addr, str,
sizeof(str)),
ntohs(clie_addr.sin_port));
for(i = 0; i < FD_SETSIZE; ++i)
{
if(client[i] < 0)
{
client[i] = connfd;
break;}
}

if(i == FD_SETSIZE)
{
fputs("too many clients\n", stderr);
exit(1);} FD_SET(connfd, &allset);
if(connfd > maxfd)
{
maxfd = connfd;
} if(i > maxi)
{
maxi = i;
} if(--nready == 0)
{
continue;}
} for(i = 0; i <= maxi; ++i)
{
if((sockfd = client[i]) < 0)
{
continue;} if(FD_ISSET(sockfd, &rset))
{
if((nByte = read(sockfd, buf, sizeof(buf))) == 0)
{
close(sockfd);FD_CLR(sockfd, &allset);
client[i] = -1;
} else if(nByte > 0)
{
for(j = 0; j < nByte; ++j)
{
buf[j] = toupper(buf[j]);
} write(sockfd, buf, nByte);
write(STDOUT_FILENO, buf, nByte);} if(--nready == 0)
{
break;}
}}
} close(listenfd);

close(connfd);return 0;
}
```