# tcp心跳问题

一般服务器端不会去主动重连客户端。在出处理tcp长连接时，假设c-s双方保持连接但是很长一段时间没有数据往来，此时为了检测连接是否存在，可通过心跳包来实现。

1 客户端每隔一个时间间隔发生一个探测包给服务器
2 客户端发包时启动一个超时定时器
3 服务器端接收到检测包，应该回应一个包
4 如果客户机收到服务器的应答包，则说明服务器正常，删除超时定时器
5 如果客户端的超时定时器超时，依然没有收到应答包，则说明服务器挂了

1、对方接收一切正常：以期望的ACK响应，2小时后，TCP将发出另一个探测分节。
2、对方已崩溃且已重新启动：以RST响应。套接口的待处理错误被置为ECONNRESET，套接 口本身则被关闭。
3、对方无任何响应：源自berkeley的TCP发送另外8个探测分节，相隔75秒一个，试图得到一个响应。在发出第一个探测分节11分钟15秒后若仍无响应就放弃。套接口的待处理错误被置为ETIMEOUT，套接口本身则被关闭。如ICMP错误是“host unreachable(主机不可达)”，说明对方主机并没有崩溃，但是不可达，这种情况下待处理错误被置为 EHOSTUNREACH。

有关SO_KEEPALIVE的三个参数详细解释如下:
（16）tcp_keepalive_intvl，保活探测消息的发送频率。默认值为75s。
发送频率tcp_keepalive_intvl乘以发送次数tcp_keepalive_probes，就得到了从开始探测直到放弃探测确定连接断开的时间，大约为11min。
（17）tcp_keepalive_probes，TCP发送保活探测消息以确定连接是否已断开的次数。默认值为9（次）。
注意：只有设置了SO_KEEPALIVE套接口选项后才会发送保活探测消息。
（18）tcp_keepalive_time，在TCP保活打开的情况下，最后一次数据交换到TCP发送第一个保活探测消息的时间，即允许的持续空闲时间。默认值为7200s（2h）。

```cpp
#include <netinet/tcp.h>  
//参数解释
//fd:网络连接描述符
//start:首次心跳侦测包发送之间的空闲时间  
//interval:两次心跳侦测包之间的间隔时间
//count:探测次数，即将几次探测失败判定为TCP断开
int set_tcp_keepAlive(int fd, int start, int interval, int count)  
{  
    int keepAlive = 1;  
    if (fd < 0 || start < 0 || interval < 0 || count < 0) return -1;  //入口参数检查 ，编程的好习惯。
    //启用心跳机制，如果您想关闭，将keepAlive置零即可  
    if(setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,(void*)&keepAlive,sizeof(keepAlive)) == -1)  
    {  
        perror("setsockopt");  
        return -1;  
    }  
    //启用心跳机制开始到首次心跳侦测包发送之间的空闲时间  
    if(setsockopt(fd,SOL_TCP,TCP_KEEPIDLE,(void *)&start,sizeof(start)) == -1)  
    {  
        perror("setsockopt");  
        return -1;  
    }  
    //两次心跳侦测包之间的间隔时间  
    if(setsockopt(fd,SOL_TCP,TCP_KEEPINTVL,(void *)&interval,sizeof(interval)) == -1)  
    {  
        perror("setsockopt");  
        return -1;  
    }  
    //探测次数，即将几次探测失败判定为TCP断开  
    if(setsockopt(fd,SOL_TCP,TCP_KEEPCNT,(void *)&count,sizeof(count)) == -1)  
    {  
        perror("setsockopt");  
        return -1;  
    }  
    return 0;  
}
```

