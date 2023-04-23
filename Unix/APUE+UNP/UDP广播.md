# UDP广播

单播：两个主机端到端的通信
广播：整个局域网所有主机的通信，占用带宽比较多。
多播：介于单薄和广播之间，对某个局域网中一组特定的主机进行通信，可以节省带宽

多播IP地址：
IPV4中属于D类IP地址，范围从224.0.0.0到239.255.255.255，并将其再划分为3类
局部链接多播地址范围在 224.0.0.0~224.0.0.255，这是为路由协议和其它用途保留的地址，路由器并不转发属于此范围的IP包；
预留多播地址为 224.0.1.0~238.255.255.255，可用于全球范围（如Internet）或网络协议；
管理权限多播地址为 239.0.0.0~239.255.255.255，可供组织内部使用，类似于私有 IP 地址，不能用于 Internet，可限制多播范围。



多播接收客户端
```c
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>

int main()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);//UDP套接字
    if(sockfd < 0)
    {
        printf("create a new socket failure:%s\n",strerror(errno));
        return 0;
    }
    int rv;
    struct sockaddr_in client_addr;
    memset(&client_addr, 0 ,sizeof(client_addr));

    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(8888);
    rv =bind(sockfd, (struct sockaddr *)&client_addr,sizeof(client_addr));//绑定本地地址和端口
    if(rv < 0)
    {
        printf("bind error:%s\n",strerror(errno));
        close(sockfd);
        return 0;
    }
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.88");//多播地址
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);//将要添加到多播组的 IP
    struct timeval  select_timeout;
    fd_set rset;

    rv = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,&mreq, sizeof(mreq));// 加入多播组
    if (rv < 0)
    {
        perror("setsockopt():IP_ADD_MEMBERSHIP");
        return -4;
    }
    int times = 0;
    int addr_len = 0;
    char buff[256] = {0};
    int n = 0;

    while(1)
    {
        select_timeout.tv_sec  = 1;//设置select超时时间为1s
        select_timeout.tv_usec = 0;
        FD_ZERO(&rset);
        FD_SET(sockfd, &rset);
        addr_len = sizeof(client_addr);
        memset(buff, 0, sizeof(buff));
        if(select(sockfd+1, &rset, NULL,NULL,&select_timeout)>0)//使用select形成超时阻塞,避免轮询和错误引起的永久阻塞
        {
            sleep(5);
            n = recvfrom(sockfd, buff, sizeof(buff), 0,(struct sockaddr*)&client_addr, &addr_len);//接收来自绑定IP和指定端口的数据
            if( n== -1)
            {
                perror("recvfrom()");
            }
            printf("Recv %dst message from server:%s\n", times, buff);
        }
        else
        {
            printf("no message\n");
        }
    }
    setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP,&mreq, sizeof(mreq));//退出多播组

    close(sockfd);

    return 0;
}
```

多播服务器端
```c
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
int main()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        printf("create a new socket failure:%s\n",strerror(errno));
        return 0;
    }
    int rv;
    char buf[]="boardcast data\n";

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_port = htons(8888);
    server_addr.sin_addr.s_addr = inet_addr("224.0.0.88");
    server_addr.sin_family = AF_INET;
    while(1)
    {
        rv = sendto(sockfd,buf,strlen(buf),0,(struct sockaddr*)&server_addr,sizeof(server_addr));

        if(rv < 0)
        {
            printf("sendto error:%s\n",strerror(errno));
            close(sockfd);
            return 0;
        }
        sleep(10);
    }

    return 0;
}
```