# write/read封装

```c
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
/*设置文件描述符为阻塞或非阻塞，1非阻塞0阻塞*/
int set_fd_blockstatus(int fd,int flag)
{
    if( 1 != flag && 0 != flag)
    {
        flag = 1;
    }
    ioctl(fd, FIONBIO, &flag);
    return 0;
}
/*超时的connect函数，超时时间由结构体struct timval提供，成功返回0，失败返回-1*/
int connect_with_timeout(int fd,struct sockaddr addr,struct timeval tm_val)
{
    if( fd < 0 )
    {
        return -1;
    }
    struct sockaddr server_addr;
    struct timeval tv;
    int ret;
    fd_set set;
    int err;
    int len = sizeof(int);
    memcpy(&server_addr, &addr, sizeof(server_addr));
    memcpy(&tv, &tm_val, sizeof(tv));

    set_fd_blockstatus(fd, 1);

    ret = connect(fd,&server_addr, sizeof(server_addr));
    if(-1 == ret)
    {
        if(EINPROGRESS == errno)
        {
            FD_ZERO(&set);
            FD_SET(fd, &set);
            if(select(fd + 1, NULL, &set, NULL, &tv) > 0)
            {
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t *)&len);
                if(0 == err)
                    ret = 0;
                else
                    ret = -1;
            }
            else
            {
                ret = -1;
            }
        }
    }
    set_fd_blockstatus(fd, 0);

    return ret;

}
/*
int set_fd_blockstatus(int fd,int flag)
{
    int flags;
    if( 1 != flag && 0 != flag)
    {
        flag = 1;
    }
    if(flags = fcntl(fd, F_GETFL, 0 ) < 0)
    {
        return -1;
    }
    if(1 == flag)
    {
        flags |= O_NONBLOCK;
        if(fcntl(fd, F_SETFL, flags) < 0 )
        {
            return -1;
        }
    }
    else
    {
        flags &= ~O_NONBLOCK;
        if(fcntl(fd, F_SETFL, flags) < 0 )
        {
            return -1;
        }
    }
    return 0;
}
*/
int readn(int connfd, void *vptr, int n)
{
    int nleft;
    int nread;
    char *ptr;
    struct timeval  select_timeout;
    fd_set rset;

    select_timeout.tv_sec = 10;
    select_timeout.tv_usec = 1;

    ptr = vptr;
    nleft = n;
    while (nleft > 0)
    {
        FD_ZERO(&rset);
        FD_SET(connfd, &rset);
        if (select(connfd+1, &rset, NULL, NULL, &select_timeout) <= 0) //select中的超时时间会逐渐减小,如把select_timeout放在while里,代表每次读都可以超时10s,放外面代表最多超时10s
        {
            if(errno == EINTR)//对select监听的读时，select返回-1可能由于信号中断导致
            {
                errno = 0;//如select被中断，select再次返回<=0时，errno变量不会被改变，这里手动改变
                continue;
            }
            return -1;
        }
        if ((nread = recv(connfd, ptr, nleft, 0)) < 0)//缓冲区有数据，能读多少都多少，不会阻塞，阻塞在select处
        {
            if (errno == EINTR)
            {
                nread = 0;
            }
            else
            {
                return -1;
            }
        }
        else if (nread == 0)
        {
            break;
        }
        nleft -= nread;
        ptr += nread;
    }
    return(n - nleft);
}

int readnPeek(int connfd, void *vptr, int n)
{
    int nleft;
    int nread;
    char *ptr;
    struct timeval  select_timeout;
    fd_set rset;

    select_timeout.tv_sec = 10;
    select_timeout.tv_usec = 1;

    ptr = vptr;
    nleft = n;
    while (nleft > 0)
    {
        FD_ZERO(&rset);
        FD_SET(connfd, &rset);
        if (select(connfd+1, &rset, NULL, NULL, &select_timeout) <= 0) //select中的超时时间会逐渐减小,如把select_timeout放在while里,代表每次读都可以超时10s,放外面代表最多超时10s
        {
            if(errno == EINTR)//对select监听的读时，select返回-1可能由于信号中断导致
            {
                errno = 0;//如select被中断，select再次返回<=0时，errno变量不会被改变，这里手动改变
                continue;
            }
            return -1;
        }
        if ((nread = recv(connfd, ptr, nleft, MSG_PEEK)) < 0)//缓冲区有数据，能读多少都多少，不会阻塞，阻塞在select处
        {
            if (errno == EINTR || errno == EAGAIN))
            {
                nread = 0;
            }
            else
            {
                return -1;
            }
        }
        else if (nread == 0)
        {
            break;
        }
        nleft -= nread;
        ptr += nread;
    }
    return(n - nleft);
}

int writen(int connfd, void *vptr, size_t n)
{
    int nleft, nwritten;
    char *ptr;

    ptr = vptr;
    nleft = n;

    while (nleft > 0)
    {
        if ((nwritten = send(connfd, ptr, nleft, MSG_NOSIGNAL)) == -1)
        {
            if (errno == EINTR)
            {
                nwritten = 0;
            }
            else
            {
                return -1;
            }
        }
        nleft -= nwritten;
        ptr += nwritten;
    }

    return(n);
}


/*tcp四次挥手过程，调用方调用close会立即返回，并由内核TCP模块负责将残留数据发送出去，应用层无法得知数据发送是否成功。*/
/*SO_LINGER选项：
struct linger 
{
	int l_onoff;    // linger active
	int l_linger;   //how many seconds to linger for 
};
l_onoff = 1,l_linger = 0。链接立即终止，丢弃残留缓冲区数据，发送RST给对方，read/recv等返回WSAECONNRESET错误
l_onoff = 0。缺省
l_onoff = 1,l_linger = n sencond。socket阻塞时，close阻塞最多n秒，n秒内如缓冲区数据全发送出去，close返回0，否则返回-1，errno =EWOULDBLOCK。
								  非阻塞socket立即返回，通过errno判断数据是否发送完*/
int closeLinger(int sockfd,unsigned int nsec)
{
	int retVal = 0;
	struct linger tcpLinger;
	tcpLinger.l_onoff = 1;
	tcpLinger.l_linger = nsec;
	retVal = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (char *)&tcpLinger, sizeof(struct linger));
	if(retVal < 0)
	{
		printf("setsockopt error:%s\n",strerror(errno));
		return -1;
	}
	return close(sockfd);
}

int createTcpSocket(struct sockaddr_in* addr)
{
	if(!addr)
		return -1;
	int retVal = 0;
	int sockfd = -1;
	int addreuse = 1;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		printf("socket error:%s\n",strerror(errno));
		return -1;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&addreuse, sizeof(int)) == ERROR)
	{
		printf("setsockopt error:%s\n",strerror(errno));
		close(sockfd);
		return -1;
	}	
	retVal = bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
	if(retVal < 0)
	{
		printf("bind error:%s\n",strerror(errno));
		close(sockfd);
		return -1;
	}
	retVal = listen(sockfd,20);
	if(retVal < 0)
	{
		printf("listen error:%s\n",strerror(errno));
		close(sockfd);
		return -1;
	}
	
}

/*get buffer's bytes which can be read*/
int getBytesFromBuffer(int fd)
{
	if(fd < 0)
		return -1;
	unsigned int bytes;
	if(ioctl(fd, FIONREAD, &bytes) == 0)
	{
		return bytes;
	}
	return -1;
}

```