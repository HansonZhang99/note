# UDP socket bind

Linux udp socket 客户端调用bind和connect后，可以直接调用write和read。而不用调用sendto和recvfrom

bind函数可用在udp和tcp的服务器和客户端，其目的是绑定数据接收或发送的端口

connect函数可以用在TCP客户端和UDP客户端，在用在UDP客户端时，可以建立端到端的连接，此时可以直接调用write或read读取对端数据。
UDP服务端需要调用bind和connect，而后可以调用recv/recvfrom,sendto但是不能调用send





## server

```cpp
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>




int main(int argc, char **argv)
{
#if 1
    if(argc < 3 )
    {
        printf("./a.out ip port\n");
        return 0;
    }
#endif
    int sock_fd;
    int rv;
    socklen_t len;
    struct sockaddr_in addr,local_addr;
    len = sizeof(addr);
    char buf[1024];
    memset(&addr, 0, sizeof(addr));
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_fd < 0)
    {
        printf("create new udp socket failure:%s\n",strerror(errno));
        return -1;
    }


    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);


    rv= bind(sock_fd, (struct sockaddr*)&addr,sizeof(addr));
    if(rv < 0 )
    {
        printf("bind failure:%s\n",strerror(errno));
        close(sock_fd);
        return 0;
    }
#if 0
    rv = connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
    if(rv < 0)
    {
        printf("connect fialure:%s\n",strerror(errno));
        close(sock_fd);
        return 0;
    }
#endif
    while(1)
    {
        memset(buf ,0, 1024);
        rv =recvfrom(sock_fd, buf, 1024, 0, (struct sockaddr*)&addr, &len);
    //  rv = send(sock_fd, "hello world\n",strlen("hello world\n"),0);
        if(rv <= 0)
        {
            printf("recvfrom failure:%s\n",strerror(errno));
            close(sock_fd);
            return 0;
        }
        printf("recv %d bytes data :%s , from %s: %d\n",rv,buf,inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    //  sleep(3);
    }
    return 0;
}
```

## client

```cpp
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>




int main(int argc, char **argv)
{
    if(argc < 3 )
    {
        printf("./a.out ip port\n");
        return 0;
    }
    int sock_fd;
    int rv;
    struct sockaddr_in addr,local_addr;
    char buf[1024];
    memset(&addr, 0, sizeof(addr));
    memset(&local_addr, 0, sizeof(local_addr));
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_fd < 0)
    {
        printf("create new udp socket failure:%s\n",strerror(errno));
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr(argv[1]);


    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(8888);
    local_addr.sin_addr.s_addr = inet_addr("10.1.74.39");


    rv= bind(sock_fd, (struct sockaddr*)&local_addr,sizeof(local_addr));
    if(rv < 0 )
    {
        printf("bind failure:%s\n",strerror(errno));
        close(sock_fd);
        return 0;
    }


    rv = connect(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
    if(rv < 0)
    {
        printf("connect fialure:%s\n",strerror(errno));
        close(sock_fd);
        return 0;
    }


    while(1)
    {
        rv =write(sock_fd, "hello world\n", strlen("hello world\n"));
        if(rv != strlen("hello world\n"))
    //  rv = read(sock_fd, buf, 1024);
        if(rv <=0)
        {
            printf("write failure:%s\n",strerror(errno));
            close(sock_fd);
            return 0;
        }
    //  printf("+++++%s\n",buf);
        sleep(3);
    }
    return 0;
}
```





```cpp
#include <sys/types.h>
#include <sys/socket.h>

ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
```

1、这个三个函数都是用来从一个socket接收消息的，不同之处在于recvfrom和recvmsg可以用在已经建立连接的socket，也可以用在没有建立连接的socket，关于建立连接的socket，简单来说就是有没有调用connect，调用了bind的socket也可以。

2、addrlen是一个value-result参数，传入函数之前初始化为src_addr的大小，返回之后存放src_addr实际大小。

3、三个函数的返回值都是成功读取的消息的长度，如果一个消息的长度大于buffer的长度，多余的字节根据socket的类型可能会被丢弃（例如UDP）。

4、如果没有消息可读，这个三个函数会阻塞直到有数据；或者可以设置为非阻塞的，在这种情况下返回-1，同时errno被设置为EAGAIN或者EWOULDBLOCK。

5、如果返回值为0，表示对端已经关闭。