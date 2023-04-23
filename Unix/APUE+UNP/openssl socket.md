# openssl socket

## server

```cpp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


void EXIT_IF_TRUE(int  x)
{
    if (x) {
        exit(2);
    }
}
// 服务端代码
int Test_openssl_server_v2()
{
    SSL_CTX     *ctx;
    SSL         *ssl;
    X509        *client_cert;


    char szBuffer[1024];
    int nLen;

    struct sockaddr_in addr;
    int len;
    int nListenFd, nAcceptFd;


    // 初始化
    SSL_library_init();//ssl库初始化
    OpenSSL_add_all_algorithms();//加载所有 加密 和 散列 函数
    SSL_load_error_strings();//加载 SSL 错误消息
    ERR_load_BIO_strings();//加载 BIO 抽象库的错误信息，如果不使用openssl库自带socket封装，可不用


    // 我们使用SSL V3,V2
    if((ctx = SSL_CTX_new(SSLv23_method())) == NULL)//申请ssl回话环境
    {
        printf("SSL_CTX_new error\n");
        return -1;
    }


    // 要求校验对方证书
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);//SSL_VERIFY_PEER标志要求对方校验证书，第三个参数为一个回调函数


    // 加载CA的证书
    if((!SSL_CTX_load_verify_locations(ctx, "ca.crt", NULL)))//加载CA证书
    {
        printf("SSL_CTX_load_verify_locations error\n");
        return -1;
    }


    // 加载自己的证书
    if(SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0)
    {
        printf("SSL_CTX_use_certificate_file error\n");
        return -1;
    }


    // 加载自己的私钥  
    if(SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0)
    {
        printf("SSL_CTX_use_PrivateKey_file error\n");
        return -1;
    }


    // 判定私钥是否正确  
    EXIT_IF_TRUE(!SSL_CTX_check_private_key(ctx));


    // 建立socket连接
    nListenFd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = PF_INET;
    my_addr.sin_port = htons(8888);
    my_addr.sin_addr.s_addr = inet_addr("192.168.0.16");//  INADDR_ANY;
    if (bind(nListenFd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        int a = 2;
        int b = a;
    }


    //
    listen(nListenFd, 10);


    memset(&addr, 0, sizeof(addr));
    len = sizeof(addr);
    nAcceptFd = accept(nListenFd, (struct sockaddr *)&addr, (int *)&len);
    printf("Accept a connect from [%s:%d]\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));


    // 将连接付给SSL  
    EXIT_IF_TRUE((ssl = SSL_new(ctx)) == NULL);
    SSL_set_fd(ssl, nAcceptFd);//关联ssl和套接字
    int n1 = SSL_accept(ssl);//建立ssl连接
    if (n1 == -1) {
        const char* p1 = SSL_state_string_long(ssl);
        int a = 2;
        int b = a;
        return -1;
    }
    // 进行操作  
    strcat(szBuffer, " this is from server");
    SSL_write(ssl, szBuffer, strlen(szBuffer));

    printf("write \n");
    memset(szBuffer, 0, sizeof(szBuffer));
    nLen = SSL_read(ssl, szBuffer, sizeof(szBuffer));
    printf("Get Len %d %s ok\n", nLen, szBuffer);

    // 释放资源  
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    close(nAcceptFd);


    return 0;
}


int main()
{
    Test_openssl_server_v2();
    return 0;
}
```

## client

```cpp
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


#define MAXBUF 1024


void EXIT_IF_TRUE(int  x)
{
    if (x) {
        exit(2);
    }
}
int Test_openssl_client_v2()
{


    SSL_CTX     *ctx;
    SSL         *ssl;


    int nFd;
    int nLen;
    char szBuffer[1024];


    // 初始化
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();


    // 我们使用SSL V3,V2
    EXIT_IF_TRUE((ctx = SSL_CTX_new(SSLv23_method())) == NULL);


    // 要求校验对方证书
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);


    // 加载CA的证书
    EXIT_IF_TRUE(!SSL_CTX_load_verify_locations(ctx, "ca.crt", NULL));


    // 加载自己的证书
    EXIT_IF_TRUE(SSL_CTX_use_certificate_file(ctx, "client.crt", SSL_FILETYPE_PEM) <= 0);


    // 加载自己的私钥
    EXIT_IF_TRUE(SSL_CTX_use_PrivateKey_file(ctx, "client.key", SSL_FILETYPE_PEM) <= 0);


    // 判定私钥是否正确
    EXIT_IF_TRUE(!SSL_CTX_check_private_key(ctx));


    //SSL_CTX_set_default_passwd_cb_userdata(ctx, "123456");
    // 创建连接
    nFd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(8888);
    dest.sin_addr.s_addr = inet_addr("192.168.0.16");
    if (connect(nFd, (struct sockaddr *) &dest, sizeof(dest)) != 0) {
        perror("Connect ");
        exit(errno);
    }


    // 将连接付给SSL
    EXIT_IF_TRUE((ssl = SSL_new(ctx)) == NULL);
    SSL_set_fd(ssl, nFd);
    int n1 = SSL_connect(ssl);
    if (n1 == -1) {
        int n2 = SSL_get_error(ssl, n1);

        const char* p1 = SSL_state_string(ssl);
    }


    // 进行操作
    sprintf(szBuffer, "this is from client %d", getpid());
    int nWriten = SSL_write(ssl, szBuffer, strlen(szBuffer));


    // 释放资源
    memset(szBuffer, 0, sizeof(szBuffer));
    nLen = SSL_read(ssl, szBuffer, sizeof(szBuffer));
    printf( "Get Len %d %s ok\n", nLen, szBuffer);


    SSL_free(ssl);//释放资源
    SSL_CTX_free(ctx);//释放资源
    close(nFd);


    return 0;
}
int main()
{
    Test_openssl_client_v2();
    return 0;
}
```

## openssl socket 证书认证

### 对称加密

加密是用一个密码加密文件,然后解密也用同样的密码

### 非对称加密

有些加密,加密用的一个密码,而解密用另外一组密码,这个叫非对称加密,意思就是加密解密的密码不一样， 是的没错,公钥和私钥都可以用来加密数据,相反用另一个解开,公钥加密数据,然后私钥解密的情况被称为加密解密,私钥加密数据,公钥解密一般被称为签名和验证签名.
因为公钥加密的数据只有它相对应的私钥可以解开,所以你可以把公钥给人和人,让他加密他想要传送给你的数据,这个数据只有到了有私钥的你这里,才可以解开成有用的数据,其他人就是得到了,也看懂内容.同理,如果你用你的私钥对数据进行签名,那这个数据就只有配对的公钥可以解开,有这个私钥的只有你,所以如果配对的公钥解开了数据,就说明这数据是你发的,相反,则不是.这个被称为签名. 实际应用中,一般都是和对方交换公钥,然后你要发给对方的数据,用他的公钥加密,他得到后用他的私钥解密,他要发给你的数据,用你的公钥加密,你得到后用你的私钥解密,这样最大程度保证了安全性.



非对称加密的算法有很多,比较著名的有RSA/DSA ,不同的是RSA可以用于加/解密,也可以用于签名验签,DSA则只能用于签名.至于SHA则是一种和md5相同的算法,它不是用于加密解密或者签名的,它被称为摘要算法.就是通过一种算法,依据数据内容生成一种固定长度的摘要,这串摘要值与原数据存在对应关系,就是原数据会生成这个摘要,但是,这个摘要是不能还原成原数据的,嗯....,正常情况下是这样的,这个算法起的作用就是,如果你把原数据修改一点点,那么生成的摘要都会不同,传输过程中把原数据给你再给你一个摘要,你把得到的原数据同样做一次摘要算法,与给你的摘要相比较就可以知道这个数据有没有在传输过程中被修改了.
实际应用过程中,因为需要加密的数据可能会很大,进行加密费时费力,所以一般都会把原数据先进行摘要,然后对这个摘要值进行加密,将原数据的明文和加密后的摘要值一起传给你.这样你解开加密后的摘要值,再和你得到的数据进行的摘要值对应一下就可以知道数据有没有被修改了,而且,因为私钥只有你有,只有你能解密摘要值,所以别人就算把原数据做了修改,然后生成一个假的摘要给你也是不行的,你这边用密钥也根本解不开.


一般的公钥不会用明文传输给别人的,正常情况下都会生成一个文件,这个文件就是公钥文件,然后这个文件可以交给其他人用于加密,但是传输过程中如果有人恶意破坏,将你的公钥换成了他的公钥,然后得到公钥的一方加密数据,不是他就可以用他自己的密钥解密看到数据了吗,为了解决这个问题,需要一个公证方来做这个事,任何人都可以找它来确认公钥是谁发的.这就是CA,CA确认公钥的原理也很简单,它将它自己的公钥发布给所有人,然后一个想要发布自己公钥的人可以将自己的公钥和一些身份信息发给CA,CA用自己的密钥进行加密,这里也可以称为签名.然后这个包含了你的公钥和你的信息的文件就可以称为证书文件了.这样一来所有得到一些公钥文件的人,通过CA的公钥解密了文件,如果正常解密那么机密后里面的信息一定是真的,因为加密方只可能是CA,其他人没它的密钥啊.这样你解开公钥文件,看看里面的信息就知道这个是不是那个你需要用来加密的公钥了.

### 证书生成

#### CA证书的生成
生成RSA私钥：
openssl genrsa -out ca.key 1024
创建证书请求：
openssl req -new -x509 -days 36500 -key ca.key -out ca.crt
req是证书请求的子命令，-x509表示输出证书 -days365 为有效期

#### 创建server证书
生成RSA私钥：
openssl genrsa -out server.key 1024
创建证书请求：
openssl req -new -key server.key -out server.csr
使用根CA证书对"请求签发证书"进行签发，生成x509格式证书
openssl x509 -req -days 3650 -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt

#### 创建client证书
生成RSA私钥：
openssl genrsa -out client.key 1024
创建证书请求：
openssl req -new -key client.key -out client.csr

#### 用根CA证书对"请求签发证书"进行签发，生成x509格式证书
openssl x509 -req -days 3650 -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt
-in arg 待处理密钥文件 -CAcreateserial 如果序列号文件(serial number file)没有指定，则自动创建它
参考： https://blog.csdn.net/gengxiaoming7/article/details/78505107



### SSL/TLS

![image-20230424005258078](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005258078.png)

SSL：（Secure Socket Layer，安全套接字层），位于可靠的面向连接的网络层协议和应用层协议之间的一种协议层。SSL通过互相认证、使用数字签名确保完整性、使用加密确保私密性，以实现客户端和服务器之间的安全通讯。该协议由两层组成：SSL记录协议和SSL握手协议。

TLS：(Transport Layer Security，传输层安全协议)，用于两个应用程序之间提供保密性和数据完整性。该协议由两层组成：TLS记录协议和TLS握手协议。

SSL协议位于TCP/IP协议与各种应用层协议之间，为数据通讯提供安全支持。SSL协议可分为两层： SSL记录协议（SSL Record Protocol）：它建立在可靠的传输协议（如TCP）之上，为高层协议提供数据封装、压缩、加密等基本功能的支持。 SSL握手协议（SSL Handshake Protocol）：它建立在SSL记录协议之上，用于在实际的数据传输开始前，通讯双方进行身份认证、协商加密算法、交换加密密钥等。

安全传输层协议（TLS）用于在两个通信应用程序之间提供保密性和数据完整性。该协议由两层组成： TLS 记录协议（TLS Record）和 TLS 握手协议（TLS Handshake）。较低的层为 TLS 记录协议，位于某个可靠的传输协议（例如 TCP）上面。



### wireshark抓包测试

![image-20230424005225005](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005225005.png)

函数connnect返回后，client向server发起请求建立TLS连接：

#### 客户端发起请求（Client Hello)
由于客户端(如浏览器)对一些加解密算法的支持程度不一样，但是在TLS协议传输过程中必须使用同一套加解密算法才能保证数据能够正常的加解密。在TLS握手阶段，客户端首先要告知服务端，自己支持哪些加密算法，所以客户端需要将本地支持的加密套件(Cipher Suite)的列表传送给服务端。除此之外，客户端还要产生一个随机数，这个随机数一方面需要在客户端保存，另一方面需要传送给服务端，客户端的随机数需要跟服务端产生的随机数结合起来产生后面要讲到的 Master Secret
综上，在这一步，客户端主要向服务器提供以下信息：

1. 支持的协议版本，比如TLS 1.0版
2. 一个客户端生成的随机数，稍后用于生成”对话密钥”
3. 支持的加密方法，比如RSA公钥加密
4. 支持的压缩方法

#### 服务器响应（Server Hello)

![image-20230424005232874](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005232874.png)

从Server Hello到Server Done，有些服务端的实现是每条单独发送，有服务端实现是合并到一起发送。Sever Hello和Server Done都是只有头没有内容的数据。
服务端在接收到客户端的Client Hello之后，服务端需要将自己的证书发送给客户端。这个证书是对于服务端的一种认证。例如，客户端收到了一个来自于称自己是www.alipay.com的数据，但是如何证明对方是合法的alipay支付宝呢？这就是证书的作用，支付宝的证书可以证明它是alipay，而不是财付通。证书是需要申请，并由专门的数字证书认证机构(CA)通过非常严格的审核之后颁发的电子证书。颁发证书的同时会产生一个私钥和公钥。私钥由服务端自己保存，不可泄漏。公钥则是附带在证书的信息中，可以公开的。证书本身也附带一个证书电子签名，这个签名用来验证证书的完整性和真实性，可以防止证书被串改。另外，证书还有个有效期。
在服务端向客户端发送的证书中没有提供足够的信息（证书公钥）的时候，还可以向客户端发送一个 Server Key Exchange，
此外，对于非常重要的保密数据，服务端还需要对客户端进行验证，以保证数据传送给了安全的合法的客户端。服务端可以向客户端发出 Cerficate Request 消息，要求客户端发送证书对客户端的合法性进行验证。比如，金融机构往往只允许认证客户连入自己的网络，就会向正式客户提供USB密钥，里面就包含了一张客户端证书。
跟客户端一样，服务端也需要产生一个随机数发送给客户端。客户端和服务端都需要使用这两个随机数来产生Master Secret。
最后服务端会发送一个Server Hello Done消息给客户端，表示Server Hello消息结束了。
综上，在这一步，服务器的回应包含以下内容：

1. 确认使用的加密通信协议版本，比如TLS 1.0版本。如果浏览器与服务器支持的版本不一致，服务器关闭加密通信
2. 一个服务器生成的随机数，稍后用于生成”对话密钥”
3. 确认使用的加密方法，比如RSA公钥加密
4. 服务器证书

#### 客户端回应

![image-20230424005311459](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005311459.png)

##### Client Key Exchange
如果服务端需要对客户端进行验证，在客户端收到服务端的 Server Hello 消息之后，首先需要向服务端发送客户端的证书，让服务端来验证客户端的合法性。
接着，客户端需要对服务端的证书进行检查，如果证书不是可信机构颁布、或者证书中的域名与实际域名不一致、或者证书已经过期，就会向访问者显示一个警告，由其选择是否还要继续通信。如果证书没有问题，客户端就会从服务器证书中取出服务器的公钥。然后，向服务器发送下面三项信息：

1. 一个随机数。该随机数用服务器公钥加密，防止被窃听
2. 编码改变通知，表示随后的信息都将用双方商定的加密方法和密钥发送
3. 客户端握手结束通知，表示客户端的握手阶段已经结束。这一项同时也是前面发送的所有内容的hash值，用来供服务器校验

上面第一项的随机数，是整个握手阶段出现的第三个随机数，它是客户端使用一些加密算法(例如：RSA, Diffie-Hellman)产生一个48个字节的Key，这个Key叫 PreMaster Secret，很多材料上也被称作 PreMaster Key。

##### ChangeCipherSpec

ChangeCipherSpec是一个独立的协议，体现在数据包中就是一个字节的数据，用于告知服务端，客户端已经切换到之前协商好的加密套件（Cipher Suite）的状态，准备使用之前协商好的加密套件加密数据并传输了。
在ChangecipherSpec传输完毕之后，客户端会使用之前协商好的加密套件和Session Secret加密一段 Finish 的数据传送给服务端，此数据是为了在正式传输应用数据之前对刚刚握手建立起来的加解密通道进行验证。

#### 服务器的最后回应（Server Finish）

![image-20230424005326325](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005326325.png)

服务端在接收到客户端传过来的 PreMaster 加密数据之后，使用私钥对这段加密数据进行解密，并对数据进行验证，也会使用跟客户端同样的方式生成 Session Secret，一切准备好之后，会给客户端发送一个 ChangeCipherSpec，告知客户端已经切换到协商过的加密套件状态，准备使用加密套件和 Session Secret加密数据了。之后，服务端也会使用 Session Secret 加密一段 Finish 消息发送给客户端，以验证之前通过握手建立起来的加解密通道是否成功。
根据之前的握手信息，如果客户端和服务端都能对Finish信息进行正常加解密且消息正确的被验证，则说明握手通道已经建立成功，接下来，双方可以使用上面产生的Session Secret对数据进行加密传输了。

#### 加密数据传输

![image-20230424005338345](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005338345.png)

在网络数据传输的过程中，经常采用RSA和AES混合加密的方式：
服务端生成RSA密钥对，将其中的RSA公钥传递给客户端，然后用RSA密钥对AES密钥加密，传递给客户端，客户端用RSA公钥解密后，得到AES密钥，最后使用AES密钥跟服务端互通数据。

### openssl + socket伪代码

client

```cpp
SSL_CTX     *ctx;
SSL         *ssl;
SSL_library_init();
OpenSSL_add_all_algorithms();
SSL_load_error_strings();
ctx = SSL_CTX_new(SSLv23_method());
SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);// 要求校验对方证书
SSL_CTX_load_verify_locations(ctx, "ca.crt", NULL);// 加载CA的证书
SSL_CTX_use_certificate_file(ctx, "client.crt", SSL_FILETYPE_PEM);// 加载自己的证书
SSL_CTX_use_PrivateKey_file(ctx, "client.key", SSL_FILETYPE_PEM);// 加载自己的私钥
SSL_CTX_check_private_key(ctx);// 判定私钥是否正确
socket();
connect();
ssl = SSL_new(ctx);// 将连接付给SSL
SSL_set_fd(ssl, nFd);
SSL_connect(ssl);//建立ssl连接
read();
write();
SSL_free(ssl);
SSL_CTX_free(ctx);
close(fd);
```

server

```cpp
SSL_CTX     *ctx;
SSL         *ssl;
SSL_library_init();
OpenSSL_add_all_algorithms();
SSL_load_error_strings();
ctx = SSL_CTX_new(SSLv23_method());
SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);// 要求校验对方证书
SSL_CTX_load_verify_locations(ctx, "ca.crt", NULL);// 加载CA的证书
SSL_CTX_use_certificate_file(ctx, "client.crt", SSL_FILETYPE_PEM);// 加载自己的证书
SSL_CTX_use_PrivateKey_file(ctx, "client.key", SSL_FILETYPE_PEM);// 加载自己的私钥
SSL_CTX_check_private_key(ctx);// 判定私钥是否正确
socket();
bind();
listen();
accept();
ssl = SSL_new(ctx);
SSL_set_fd(ssl, nAcceptFd);
SSL_accept(ssl);//建立ssl连接
read();
write();
SSL_free(ssl);
SSL_CTX_free(ctx);
close(fd);
```

ref:

https://cloud.tencent.com/developer/article/1115445

https://blog.csdn.net/xs574924427/article/details/17240793

http://blog.sina.com.cn/s/blog_4c451e0e010143v3.html