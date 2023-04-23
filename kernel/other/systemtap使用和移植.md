# systemtap使用

当在Linux环境下打开一个普通文件，向里面写入内容，再关闭文件，这一整个过程肯定会涉及到系统调用`open` `write`和`close`，而当调用系统调用时，肯定会陷入内核，执行内核中的一段代码。在初学Linux时，就有人讲过，Linux内核提供虚拟文件系统（VFS）支持用户对文件的操作，而后来也通过学习Linux内核驱动，了解到了字符设备的`file_operation`结构体，字符设备驱动需要开发者实现这个结构体中一系列的函数如`open` `read` `write` `ioctl`，供用户层驱动设备实现一些功能。此文只对特定的内核函数调用进行追踪。



## 主机上使用systemtap

### 安装systemtap

#### 使用apt install直接安装

```shell
zhhhhhh@ubuntu:~$ sudo apt-get install systemtap systemtap-runtime
[sudo] password for zhhhhhh:
Reading package lists... Done
Building dependency tree
Reading state information... Done
systemtap is already the newest version (3.1-3ubuntu0.1).
systemtap-runtime is already the newest version (3.1-3ubuntu0.1).
systemtap-runtime set to manually installed.
The following packages were automatically installed and are no longer required:
  linux-hwe-5.4-headers-5.4.0-100 linux-hwe-5.4-headers-5.4.0-117 linux-hwe-5.4-headers-5.4.0-120 linux-hwe-5.4-headers-5.4.0-122
Use 'sudo apt autoremove' to remove them.
0 upgraded, 0 newly installed, 0 to remove and 145 not upgraded.
```

安装好后可以执行`stap -V`查看工具的信息

```shell
zhhhhhh@ubuntu:~$ stap -V
Systemtap translator/driver (version 3.1/0.170, Debian version 3.1-3ubuntu0.1 (bionic-proposed))
Copyright (C) 2005-2017 Red Hat, Inc. and others
This is free software; see the source for copying conditions.
tested kernel versions: 2.6.18 ... 4.10-rc8
enabled features: AVAHI LIBSQLITE3 NLS NSS
```

这里有个`tested kernel versions: 2.6.18 ... 4.10-rc8`猜测这个版本的systemtap只在这个范围的内核版本测试过，如果使用的内核版本高于此版本，并且在使用过程中出现问题，可以尝试手动下载systemtap源码包自己编译安装，安装前先卸载原来的版本。

#### 下载systemtap源码编译安装

可以从这个下载，也可以走官方：https://src.fedoraproject.org/repo/pkgs/systemtap/  ,这里下载需要进入目录内部，最后会有压缩包。

如下载4.7版本，systemtap有些工具需要在root下运行，建议先切换root

```shell
zhhhhhh@ubuntu:~/bsp/other$ wget https://src.fedoraproject.org/repo/pkgs/systemtap/systemtap-4.7.tar.gz/sha512/7d7c213dc4f7c5430f81763668da21403fbc351d1701b1096eb1ad233e3f0325e35f01dfd0a33e75f277b26fdde88c46d42dd32e32e4d4f27a45d53e2dd0f831/systemtap-4.7.tar.gz

zhhhhhh@ubuntu:~/bsp/other$ tar -xzf systemtap-4.7.tar.gz
zhhhhhh@ubuntu:~/bsp/other$ su
Password:
root@ubuntu:/home/zhhhhhh/bsp/other# cd systemtap-4.7
root@ubuntu:/home/zhhhhhh/bsp/other/systemtap-4.7# ./configure --prefix=`pwd`/_install
root@ubuntu:/home/zhhhhhh/bsp/other/systemtap-4.7# make -j8 && make install
root@ubuntu:/home/zhhhhhh/bsp/other/systemtap-4.7# cd _install/
root@ubuntu:/home/zhhhhhh/bsp/other/systemtap-4.7/_install# ls
bin  etc  include  lib  libexec  sbin  share
root@ubuntu:/home/zhhhhhh/bsp/other/systemtap-4.7/_install# cd bin/
root@ubuntu:/home/zhhhhhh/bsp/other/systemtap-4.7/_install/bin# ./stap -V
Systemtap translator/driver (version 4.7/0.170, non-git sources)
Copyright (C) 2005-2022 Red Hat, Inc. and others
This is free software; see the source for copying conditions.
tested kernel versions: 2.6.32 ... 5.18-rc3
enabled features: BPF PYTHON2 PYTHON3 LIBSQLITE3 LIBXML2 NLS MONITOR_LIBS
```

### 下载具有调试符号的内核

使用systemtap调试内核，内核必须开启调试，常规的ubuntu发行版本内核都是不带debug symbol的，因此需要在系统中安装对应版本的具有调试信息的内核 或者 自己重新编译内核并安装，这里选择第一种方式

这边使用的主机系统是 **Ubuntu18.04**，版本：

```shell
zhhhhhh@ubuntu:~$ uname -a
Linux ubuntu 5.4.0-146-generic #163~18.04.1-Ubuntu SMP Mon Mar 20 15:02:59 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux
```

安装具有调试信息的内核：

可以执行以下脚本直接安装

```shell
#!/bin/sh

apt-key adv --keyserver keyserver.ubuntu.com --recv-keys C8CAB6595FDFF622
#ubuntu版本低于等于16.04时，recv-keys为ECDCAD72428D7C01
codename=$(lsb_release -c | awk '{print $2}')
tee /etc/apt/sources.list.d/ddebs.list << EOF
deb http://ddebs.ubuntu.com/ ${codename} main restricted universe multiverse
deb http://ddebs.ubuntu.com/ ${codename}-updates main restricted universe multiverse
#deb http://ddebs.ubuntu.com/ ${codename}-security main restricted universe multiverse
deb http://ddebs.ubuntu.com/ ${codename}-proposed main restricted universe multiverse
EOF

apt update
apt install -y linux-image-$(uname -r)-dbgsym
```

调试版本的内核镜像会比较大，下载比较慢，也可以自己在http://ddebs.ubuntu.com/pool/main/l/linux/下载对应版本的.ddeb包，然后执行`dpkg -i *.ddeb`安装：

比如我的版本就要下载这个包：

![image-20230424000644530](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424000644530.png)

下载完成后执行安装

```shell
sudo dpkg -i linux-image-unsigned-5.4.0-146-generic-dbgsym_5.4.0-146.163_amd64.ddeb
```

安装完成执行如下命令验证是否安装成功

```shell
root@ubuntu:/home/zhhhhhh# uname -a
Linux ubuntu 5.4.0-146-generic #163~18.04.1-Ubuntu SMP Mon Mar 20 15:02:59 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux
root@ubuntu:/home/zhhhhhh# dpkg -l | grep "5.4.0-146"
ii  linux-image-unsigned-5.4.0-146-generic-dbgsym 5.4.0-146.163~18.04.1                            amd64        Linux kernel debug image for version 5.4.0 on 64 bit x86 SMP
```

这里显示安装成功。至此，systemtap工具已经可以使用。

### systemtap 使用

用法我也只懂皮毛，详细用法使用`stap --help`查看，也可以`man stap`，更多用法自行百度或者看官方文档：https://sourceware.org/systemtap

#### 测试stap命令

可以先用一条简单的命令测试stap工具是否正常运行，:

```shell
root@ubuntu:/home/zhhhhhh/bsp/other# ./systemtap-4.7/_install/bin/stap -e 'probe begin{printf("Hello, World\n"); exit();}'
Hello, World
```

#### stap命令探测系统调用在内核源码中的位置

用户调用的系统调用在内核中命名一般符合下面规则：

用户层调用`read`函数，会导致glibc触发软中断陷入内核，调用对应的向量函数进行处理。

通常应用调用`read`内核中对应执行的函数为`sys_read`

当然也并不是所有系统都符合，可能存在前缀或后缀，可以通过`systemtap`中的`stap`工具和通配符完成查找，探测系统调用所在的文件，如以下命令查看`open`系统调用在对应版本内核源码中的所处的文件位置：

```shell
root@ubuntu:/home/zhhhhhh/bsp/other# ./systemtap-4.7/_install/bin/stap -l 'kernel.function("*sys_open*")'
```

输出如下

```shell
kernel.function("__bpf_trace_do_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/include/trace/events/fs.h:10")
kernel.function("__do_compat_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1134")
kernel.function("__do_compat_sys_open_by_handle_at@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/fhandle.c:274")
kernel.function("__do_compat_sys_openat@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1143")
kernel.function("__do_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1112")
kernel.function("__do_sys_open_by_handle_at@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/fhandle.c:256")
kernel.function("__do_sys_open_tree@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/namespace.c:2389")
kernel.function("__do_sys_openat@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1120")
kernel.function("__ia32_compat_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1134")
kernel.function("__ia32_compat_sys_open_by_handle_at@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/fhandle.c:274")
kernel.function("__ia32_compat_sys_openat@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1143")
kernel.function("__ia32_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1112")
kernel.function("__ia32_sys_open_by_handle_at@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/fhandle.c:256")
kernel.function("__ia32_sys_open_tree@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/namespace.c:2389")
kernel.function("__ia32_sys_openat@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1120")
kernel.function("__se_compat_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1134")
kernel.function("__se_compat_sys_open_by_handle_at@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/fhandle.c:274")
kernel.function("__se_compat_sys_openat@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1143")
kernel.function("__se_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1112")
kernel.function("__se_sys_open_by_handle_at@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/fhandle.c:256")
kernel.function("__se_sys_open_tree@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/namespace.c:2389")
kernel.function("__se_sys_openat@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1120")
kernel.function("__x32_compat_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1134")
kernel.function("__x32_compat_sys_open_by_handle_at@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/fhandle.c:274")
kernel.function("__x32_compat_sys_openat@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1143")
kernel.function("__x64_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1112")
kernel.function("__x64_sys_open_by_handle_at@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/fhandle.c:256")
kernel.function("__x64_sys_open_tree@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/namespace.c:2389")
kernel.function("__x64_sys_openat@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1120")
kernel.function("do_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/open.c:1083")
kernel.function("ksys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/include/linux/syscalls.h:1380")
kernel.function("perf_trace_do_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/include/trace/events/fs.h:10")
kernel.function("proc_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/fs/proc/proc_sysctl.c:639")
kernel.function("trace_do_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/include/trace/events/fs.h:10")
kernel.function("trace_event_get_offsets_do_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/include/trace/events/fs.h:10")
kernel.function("trace_event_raw_event_do_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/include/trace/events/fs.h:10")
kernel.function("trace_raw_output_do_sys_open@/build/linux-hwe-5.4-djKyw8/linux-hwe-5.4-5.4.0/include/trace/events/fs.h:10")
```

输出很多，证明有很多地方出现了`*sys_open*`，但是可以判断出`open`系统调用位于`*fs/open.c`中

对于`lseek` `read` `write` `close`同理

#### 使用stap脚本追踪systemtap

在前面知道`open`系统调用所处的文件之后，可以将探测范围缩小到文件：`*fs/open.c`中，通过以下脚本探测`open`的调用在文件中执行流程：

`test.stp`

```cpp
probe begin {
        printf("begin\n")/*开始检测，脚本启动需要一定时间，最迟也需要等begin打印出来，脚本才会开始监视内核*/
}
/*这里表示要监视内核中'*'任意路径下fs/open.c中所有函数*/
probe kernel.function("*@fs/open.c").call {
        if (target() == pid()) {
                printf("%s -> %s\n", thread_indent(4), ppfunc())/*当pid为命令行指定pid时，打印进程名和函数名*/
        }
}
/*同上，不同在于这里是函数返回时*/
probe kernel.function("*@fs/open.c").return {
        if (target() == pid()) {
                printf("%s -> %s\n", thread_indent(-4), ppfunc())
        }
}

probe kernel.function("*@fs/read_write.c").call {
        if (target() == pid()) {
                printf("%s -> %s\n", thread_indent(4), ppfunc())
        }
}
probe kernel.function("*@fs/read_write.c").return {
        if (target() == pid()) {
                printf("%s <- %s\n", thread_indent(-4), ppfunc())
        }
}
```

用户程序`test.c`

```cpp
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
int main()
{
        getchar();
        int fd = open("./txt", O_CREAT|O_RDWR, 0666);
        if(fd< 0)
        {
                printf("open error:%s\n",strerror(errno));
                return 0;
        }
        lseek(fd, SEEK_SET, 0);
        write(fd, "hello wrold", strlen("hello world"));
        close(fd);
        return 0;
}
```

双终端下其中一个终端执行用户层程序`test.c`，在`getchar`等待输出时，在另一终端通过`ps`命令找到`test.c`进程号，执行:

`stap -x pid test.stp`

`stap`执行时间可能会有点长，毕竟是需要探测内核，等待打印出`begin`后，再等待一会，切换到另一终端输入回车使程序继续运行，此时`stap`端开始输出内核指定文件的函数调用流程：

```shell
root@ubuntu:/home/zhhhhhh/bsp/other# ./systemtap-4.7/_install/bin/stap -x 12570 test.stp
begin
     0 a.out(12570):    -> __x64_sys_openat
    10 a.out(12570):        -> do_sys_open
   114 a.out(12570):            -> vfs_open
   120 a.out(12570):                -> do_dentry_open
   127 a.out(12570):                    -> generic_file_open
   130 a.out(12570):                    -> generic_file_open
   133 a.out(12570):                -> do_dentry_open
   136 a.out(12570):            -> vfs_open
   140 a.out(12570):        -> do_sys_open
   143 a.out(12570):    -> __x64_sys_openat
     0 a.out(12570):    -> __x64_sys_lseek
     4 a.out(12570):        -> ksys_lseek
     7 a.out(12570):            -> generic_file_llseek_size
    10 a.out(12570):            <- generic_file_llseek_size
    13 a.out(12570):        <- ksys_lseek
    15 a.out(12570):    <- __x64_sys_lseek
     0 a.out(12570):    -> __x64_sys_write
     2 a.out(12570):        -> ksys_write
     5 a.out(12570):            -> vfs_write
     7 a.out(12570):                -> rw_verify_area
    10 a.out(12570):                <- rw_verify_area
    12 a.out(12570):                -> __vfs_write
    15 a.out(12570):                    -> new_sync_write
    45 a.out(12570):                    <- new_sync_write
    46 a.out(12570):                <- __vfs_write
    48 a.out(12570):            <- vfs_write
    49 a.out(12570):        <- ksys_write
    50 a.out(12570):    <- __x64_sys_write
     0 a.out(12570):    -> __x64_sys_close
     2 a.out(12570):        -> filp_close
     5 a.out(12570):        -> filp_close
     6 a.out(12570):    -> __x64_sys_close
     0 a.out(12570):    -> filp_close
     2 a.out(12570):    -> filp_close
     0 a.out(12570):    -> filp_close
     2 a.out(12570):    -> filp_close
     0 a.out(12570):    -> filp_close
     1 a.out(12570):    -> filp_close
```

其中第一列为执行耗时，单位us，第二列为进程，第三列为内核函数调用链。`test.c`中调用了`open` `lseek` `write`和`close`，此处打印了`stp`脚本中指定的文件中被调用的函数的函数名。这些函数在系统所使用的内核版本中都可以找到实现。

可以看到当用户层调用`open`时，内核的`fd/open.c`中会依次执行`__x64_sys_openat`->`do_sys_open`->`vfs_open`...`generic_file_open`

#### xxxxxxxxxx #define NOTIFY_DONE     0x0000      /* Don't care *///此回调不关心此事件，直接返回#define NOTIFY_OK       0x0001      /* Suits me *///此回调执行事件并返回成功#define NOTIFY_STOP_MASK    0x8000      /* Don't call further */#define NOTIFY_BAD      (NOTIFY_STOP_MASK|0x0002)//此回调执行失败返回异常，此时链表结束执行直接返回                        /* Bad/Veto action *//* * Clean way to return from the notifier and stop further calls. */#define NOTIFY_STOP     (NOTIFY_OK|NOTIFY_STOP_MASK)//人为的停止链表的继续执行，此时链表结束执行直接返回cpp

`test.stp`

```cpp
probe begin
{
        printf("probe begin\n");
}

probe syscall.*
{
        if(target() == pid())
                printf("%s <- %s  %d %s   %s\n", thread_indent(1), execname(), pid(), pp(),ppfunc());
}
probe syscall.*.return
{
        if(target() == pid())
                printf("%s -> %s  %d %s   %s\n", thread_indent(-1), execname(), pid(), pp(),ppfunc());
}
probe end
{
        printf("probe end\n");
}
```

还是以test.c为例，以下是输出：

```shell
root@ubuntu:/home/zhhhhhh/bsp/other# ./systemtap-4.7/_install/bin/stap -x 7099 test2.stp
probe begin
     0 a.out(7099): <- a.out  7099 kprobe.function("__x64_sys_openat")?   __x64_sys_openat
    23 a.out(7099): -> a.out  7099 kprobe.function("__x64_sys_openat").return?   __x64_sys_openat
     0 a.out(7099): <- a.out  7099 kprobe.function("__x64_sys_lseek")?   __x64_sys_lseek
     2 a.out(7099): -> a.out  7099 kprobe.function("__x64_sys_lseek").return?   __x64_sys_lseek
     0 a.out(7099): <- a.out  7099 kprobe.function("__x64_sys_write")   __x64_sys_write
    21 a.out(7099): -> a.out  7099 kprobe.function("__x64_sys_write").return   __x64_sys_write
     0 a.out(7099): <- a.out  7099 kprobe.function("__x64_sys_close")?   __x64_sys_close
     3 a.out(7099): -> a.out  7099 kprobe.function("__x64_sys_close").return?   __x64_sys_close
     0 a.out(7099): <- a.out  7099 kprobe.function("__x64_sys_exit_group")   __x64_sys_exit_group
```



## 交叉编译ARM平台使用

### 开启内核debug选项

将systemtap交叉编译移植到arm平台使用，需要先开启arm linux内核的一些调试选项，重新编译移植Linux内核后才能使用，需要开启的选项如下，可以查看内核根目录`.config`文件中这些选项是否是yes，否则需要在`make menuconfig`中配置并重新编译内核：
```shell
  CONFIG_DEBUG_INFO
  CONFIG_KPROBES
  CONFIG_DEBUG_FS
  CONFIG_RELAY
```

### ARM运行systemtap的问题

即使将内核设置成debug模式，在arm平台上依然不能直接使用stap命令。

直接将交叉编译移植的stap工具运行在arm上会产生如下报错：

```shell
[root@imx6ull:~/test]# ./stap test.stp
Checking "/lib/modules/4.9.88/build/.config" failed with error: No such file or directory
[root@imx6ull:~/test]# ./stap -l 'kernel.function("*sys_open*")'
Checking "/lib/modules/4.9.88/build/.config" failed with error: No such file or directory
```

意思是无法在指定目录找到一些信息，我尝试查看PC主机对应位置的文件，发现应该是Linux内核源码中的部分目录文件，但是我查看arm使用的Linux内核源码的对应目录，这些目录都非常大，对于移植很不方便，PC应该是做过一些提取。盲目对比PC目录进行不一定可以生效，暂时没有想到其他方法移植stap命令。

stap命令实际是将stap脚本转换为C语言，然后将C语言代码编译成模块并加载到内核中。虽然无法移植stap到arm上，但是stap工具可以将stap脚本先编译为模块文件，再使用systemtap中另外的工具staprun,stapio等工具，执行这些模块，即可达到stap直接运行脚本的效果，示例如下：

test.stp脚本：

```shell
probe begin
{
        printf("Hello, World\n");
        exit();
}
```

shell执行：

```shell
root@ubuntu:/home/zhhhhhh/bsp/other# ./systemtap-4.7/_install/bin/stap test.stp
Hello, World
root@ubuntu:/home/zhhhhhh/bsp/other# ./systemtap-4.7/_install/bin/stap test.stp -m test.ko
Truncating module name to 'test'
Hello, World
root@ubuntu:/home/zhhhhhh/bsp/other# ./systemtap-4.7/_install/bin/staprun test.ko
Hello, World
```

将test.stp脚本先编译成test.ko模块再使用staprun执行模块，可以达到直接使用stap执行test.stp的效果。

这里使用systemtap-4.7版本来进行移植，思路是这样：

1.在PC机上将这个库分别以arm交叉编译器和PC编译器编译两次，得到可以运行在PC机器上和运行在arm上的systemtap工具，为了防止这两个版本工具在功能上的差异，库的版本和configure的参数都保持一致。

2.在PC上编写脚本，使用pc版本的工具中的stap将脚本转换为.ko文件

3.将文件和arm版本的工具包移植到arm上，使用arm工具包中的staprun执行.ko模块。

### 编译PC版本的systemtap

```shell
root@ubuntu:/home/zhhhhhh/bsp/other# cd systemtap-4.7
root@ubuntu:/home/zhhhhhh/bsp/other# ./configure --prefix=`pwd`/../x86_systemtap_install  LIBS=-lz --disable-docs --disable-refdocs --disable-grapher --without-rpm --disable-option-checking --disable-nls --enable-FEATURE=no --disable-ssp --without-nss
root@ubuntu:/home/zhhhhhh/bsp/other# make -j8 && make install
```

如果提示有库找不到或者命令执行失败，可以百度错误，然后使用`apt-get install`安装库或者命令工具包

### 编译arm版本systemtap

arm版本的systemtap编译会麻烦一点，pc缺少库直接install就行，arm需要交叉编译缺少的库

以下是我编译过程中遇到需要重新编译的库，这些库都可以在网上下载。

##### elfutils编译

elfutils库依赖zlib库，先编译zlib:

```shell
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap$ tar -xf zlib-1.2.12.tar
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap$ cd zlib-1.2.12/
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap$ ./configure --prefix=`pwd`/../arm_zlib_install
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap$ vim Makefile #修改Makefile中所有gcc的位置为交叉编译的gcc
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap$ make -j8 && make install
```

编译elfutils:

```shell
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap$ tar -jxf elfutils-0.186.tar.bz2
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap$ cd elfutils-0.186/
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap$ ./configure --prefix=`pwd`/../arm_elf_install --host=arm-linux --with-zlib=yes LDFLAGS=-L../arm_zlib_install/lib CPPFLAGS=-I../arm_zlib_install/include CFLAGS=-lz --disable-libdebuginfod --disable-debuginfodtall/include CFLAGS=-lz
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap$ make -j8 && make install
```

#### systemtap编译

```shell
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/systemtap-4.7$ tar -xzf systemtap-4.7.tar.gz
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/systemtap-4.7$ cd systemtap-4.7/
./configure --prefix=`pwd`/../arm_systemtap_install --host=arm-linux --with-elfutils=`pwd`/../arm_elf_install/lib  LIBS=-lz --disable-docs --disable-refdocs --disable-grapher --without-rpm --disable-option-checking --disable-nls --enable-FEATURE=no --disable-ssp --without-nss  LDFLAGS="-L`pwd`/../arm_zlib_install/lib -L`pwd`/../arm_elf_install/lib" CFLAGS="-I `pwd`/../arm_elf_install/include" CPPFLAGS=-I`pwd`/../arm_elf_install/include #configure中不加--disable-translator选项时是会编译出stap工具的，由于arm上不需要stap，这里也可以加上--disable-translator禁止生成
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/systemtap-4.7$ make -j8 && make install
```

在arm_systemtap_install目录下生成可执行文件

```shell
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/arm_systemtap_install$ file bin/staprun
bin/staprun: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 4.9.0, with debug_info, not stripped
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/arm_systemtap_install$ file libexec/systemtap/stapio
libexec/systemtap/stapio: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 4.9.0, not stripped
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/arm_systemtap_install$ readelf -edSW bin/stap | grep "NEEDED"
 0x00000001 (NEEDED)                     Shared library: [libdw.so.1]
 0x00000001 (NEEDED)                     Shared library: [libelf.so.1]
 0x00000001 (NEEDED)                     Shared library: [libsqlite3.so.0]
 0x00000001 (NEEDED)                     Shared library: [libpthread.so.0]
 0x00000001 (NEEDED)                     Shared library: [libreadline.so.8]
 0x00000001 (NEEDED)                     Shared library: [libz.so.1]
 0x00000001 (NEEDED)                     Shared library: [libstdc++.so.6]
 0x00000001 (NEEDED)                     Shared library: [libm.so.6]
 0x00000001 (NEEDED)                     Shared library: [libgcc_s.so.1]
 0x00000001 (NEEDED)                     Shared library: [libc.so.6]
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/arm_systemtap_install$ readelf -edSW libexec/systemtap/stapio | grep "NEEDED"
 0x00000001 (NEEDED)                     Shared library: [libpthread.so.0]
 0x00000001 (NEEDED)                     Shared library: [libjson-c.so.4]
 0x00000001 (NEEDED)                     Shared library: [libpanel.so.6]
 0x00000001 (NEEDED)                     Shared library: [libncurses.so.6]
 0x00000001 (NEEDED)                     Shared library: [libz.so.1]
 0x00000001 (NEEDED)                     Shared library: [libc.so.6]
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/arm_systemtap_install$ du -sh ./
56M     ./
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/arm_systemtap_install$ mv bin/stap ../ #移除stap
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/arm_systemtap_install$ du -sh ./
16M     ./

```

整个成果物除去stap工具只有16M,查看staprun和stapio工具依赖的动态库，这些动态库或者是自己编译的，或者是交叉编译器带的，如果移植到arm上报错找不到库，可以依次从交叉编译工具链中移动过去。

以下是静态编译方式，推荐使用`动态编译`。

可以使用静态编译的方式编译，静态编译如果工具链没有静态库，可能需要自己编译这些静态库：`libpanel.a libncurses.a`这两个都包含在ncurses库中，`libjson-c.a`，`libreadline.a`其中可能还会依赖`libsqlite3.a`。

我这里提前编译好了这些库，以下是静态编译方法：

```shell
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/systemtap-4.7$ ./configure --prefix=`pwd`/../arm_systemtap_install --host=arm-linux --with-elfutils=`pwd`/../elfutils-0.186 CXXFLAGS=-static CFLAGS=-static LIBS=-lz --disable-docs --disable-refdocs --disable-grapher --without-rpm --disable-option-checking --disable-nls --enable-FEATURE=no --disable-ssp --without-nss --disable-translator CPPFLAGS=-I`pwd`/../arm_elf_install/include LDFLAGS="-L`pwd`/../arm_zlib_install/lib -L`pwd`/../arm_elf_install/lib -L`pwd`/../arm_ncurses_install/lib -L`pwd`/../arm_cjson_install/lib" CFLAGS="-I `pwd`/../arm_elf_install/include" #这里和之前相比加了--disable-translator选项，这个选项其实就是不编译出stap这个工具，实际交叉编译环境并不需要这个工具，只需要staprun和staio，而且这个工具动态编译都有40M，静态会更大。除此之外还增加了一下环境变量指明库和头文件路径，增加静态编译选项
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/systemtap-4.7$ make -j8 && make install
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/arm_systemtap_install$ file libexec/systemtap/stapio
libexec/systemtap/stapio: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), dynamically linked, interpreter /lib/ld-linux-armhf.so.3, for GNU/Linux 4.9.0, not stripped
zhhhhhh@ubuntu:~/bsp/IMX6ULL/utils/systemtap/arm_systemtap_install$ file bin/staprun
bin/staprun: ELF 32-bit LSB executable, ARM, EABI5 version 1 (GNU/Linux), statically linked, for GNU/Linux 4.9.0, with debug_info, not stripped
```

实际静态编译还是存在一些工具是动态链接的，configure直接指定环境变量的方式可能覆盖不全面，可能需要进一步修改源码中的Makefile指定静态编译。



### 使用

现在拥有pc平台和arm平台的systemtap工具集，分别位于arm_systemtap_install、x86_systemtap_install。

以上面`使用stap脚本追踪systemtap`这一节中的test.stp和test.c为例。

使用pc平台stap工具将test.stp编译成test.ko模块，并移植到arm平台上

```shell
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/test# ./../x86_systemtap_install/bin/stap -gv -r /home/zhhhhhh/bsp/IMX6ULL/100ask_imx6ull-sdk/Linux-4.9.88/ -a arm test.stp  -m test.ko -B CROSS_COMPILE=arm-linux-
Truncating module name to 'test'
WARNING: kernel release/architecture mismatch with host forces last-pass 4.
Pass 1: parsed user script and 467 library scripts using 118936virt/89232res/5568shr/83908data kb, in 190usr/30sys/218real ms.
Pass 2: analyzed script: 181 probes, 11 functions, 1 embed, 2 globals using 221580virt/187580res/6396shr/186552data kb, in 600usr/20sys/622real ms.
Pass 3: translated to C into "/tmp/stapO7vV7u/test_src.c" using 221580virt/187772res/6588shr/186552data kb, in 10usr/0sys/14real ms.
test.ko
Pass 4: compiled C into "test.ko" in 13270usr/2730sys/2932real ms.
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/test# ls
test.c  test.ko  test.stp
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/test# arm-linux-gcc test.c -o test
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/test# cp ./test.ko test ../../../nfs/
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/test# cp ../arm_systemtap_install/bin/staprun  ../../../nfs/
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/test# cp ../arm_elf_install/lib/libelf
libelf-0.186.so  libelf.a         libelf.so        libelf.so.1
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/test# cp ../arm_elf_install/lib/libelf.so.1  ../../../nfs/
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/test# rm -rf ../../../nfs/arm_systemtap_install/
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/test# cp -rf ../arm_systemtap_install/ ../../../nfs/
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/test# cd ../arm_systemtap_install/
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/arm_systemtap_install# du -sh
16M     .
```

在arm上开启两个ssh终端，一个运行staprun，一个执行test

ssh1:

```shell
[root@imx6ull:~]# cd test/
[root@imx6ull:~/test]# ls
libelf.so.1   nfs_mount.sh  staprun       test          test.ko
[root@imx6ull:~/test]# ./test
```

ssh2

```shell
[root@imx6ull:~/test]# cp /mnt/test.ko ./
[root@imx6ull:~/test]# cp /mnt/test ./
[root@imx6ull:~/test]# cp /mnt/staprun ./
[root@imx6ull:~/test]# ./staprun test.ko
./staprun: error while loading shared libraries: libelf.so.1: cannot open shared object file: No such file or directory
[root@imx6ull:~/test]# cp /mnt/libelf.so.1 ./
[root@imx6ull:~/test]# export LD_LIBRARY_PATH=`pwd`
[root@imx6ull:~/test]# ./staprun test.ko
/home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/systemtap-4.7/../arm_systemtap_install/libexec/systemtap/stapio: No such file or directory  #这里可以看到staprun对stapio有来自pc上的绝对路径记忆，不妨创建同样的路径，直接将16M的成果物都拷贝过来
[root@imx6ull:~/test]# mkdir -p /home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/systemtap-4.7/../arm_systemtap_install
[root@imx6ull:~/test]# cp -rf /mnt/arm_systemtap_install/* /home/zhhhhhh/bsp/IMX6ULL/utils/systemtap/systemtap-4.7/../arm_systemtap_install
[root@imx6ull:~/test]# ps aux | grep "test"
20665 root     ./test
20741 root     grep test
[root@imx6ull:~/test]# ./staprun -x 20665 test.ko
ERROR: Couldn't insert module 'test.ko': File exists
ERROR: Rerun with staprun option '-R' to rename this module.
[root@imx6ull:~/test]# rmmod test.ko
[root@imx6ull:~/test]# ./staprun -x 20665 test.ko\
> ^C
[root@imx6ull:~/test]# ./staprun -x 20665 test.ko
begin #以下即Linux4.9.88,arm32平台上一个文件open,lseek,write,close在对应文件中的执行流程
     0 test(20665):    -> SyS_openat
     0 test(20665):        -> do_sys_open
     0 test(20665):            -> vfs_open
     0 test(20665):                -> do_dentry_open
     0 test(20665):                    -> generic_file_open
     0 test(20665):                    -> generic_file_open
     0 test(20665):                -> do_dentry_open
     0 test(20665):            -> vfs_open
     0 test(20665):            -> open_check_o_direct
     0 test(20665):            -> open_check_o_direct
     0 test(20665):        -> do_sys_open
     0 test(20665):    -> SyS_openat
     0 test(20665):    -> SyS_llseek
     0 test(20665):        -> generic_file_llseek_size
     0 test(20665):        <- generic_file_llseek_size
     0 test(20665):    <- SyS_llseek
     0 test(20665):    -> SyS_write
     0 test(20665):        -> vfs_write
     0 test(20665):            -> rw_verify_area
     0 test(20665):            <- rw_verify_area
     0 test(20665):            -> __vfs_write
     0 test(20665):            <- __vfs_write
     0 test(20665):        <- vfs_write
     0 test(20665):    <- SyS_write
     0 test(20665):    -> SyS_close
     0 test(20665):        -> filp_close
     0 test(20665):        -> filp_close
     0 test(20665):    -> SyS_close
     0 test(20665):    -> filp_close
     0 test(20665):    -> filp_close
     0 test(20665):    -> filp_close
     0 test(20665):    -> filp_close
     0 test(20665):    -> filp_close
     0 test(20665):    -> filp_close
^C[root@imx6ull:~/test]#

```



参考博客：

https://mlog.club/article/2127629

https://www.cnblogs.com/hazir/p/systemtap_introduction.html

https://snappyjack.github.io/articles/2019-12/SystemTap%E5%AE%89%E8%A3%85

https://blog.csdn.net/qq_23662505/article/details/120309680?spm=1001.2014.3001.5502

官方示例：

https://sourceware.org/systemtap/examples/

推荐学习地址：

https://segmentfault.com/a/1190000010774974
