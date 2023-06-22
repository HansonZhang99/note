# uart

## uart介绍

UART：通用异步收发传输器（Universal Asynchronous Receiver/Transmitter)，简称串口。

* 调试：移植u-boot、内核时，主要使用串口查看打印信息

* 外接各种模块

  

  串口因为结构简单、稳定可靠，广受欢迎。

  通过三根线即可，发送、接收、地线。

  ![image-20230608224712056](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608224712056.png)

### 串口的参数

- 波特率：一般选波特率都会有9600,19200,115200等选项。其实意思就是每秒传输这么多个比特位数(bit)。
- 起始位:先发出一个逻辑”0”的信号，表示传输数据的开始。
- 数据位：可以是5~8位逻辑”0”或”1”。如ASCII码（7位），扩展BCD码（8位）。小端传输。
- 校验位：数据位加上这一位后，使得“1”的位数应为偶数(偶校验)或奇数(奇校验)，以此来校验数据传送的正确性。
- 停止位：它是一个字符数据的结束标志。

### 发送数据示例

怎么发送一字节数据，比如‘A‘?
‘A’的ASCII值是0x41,二进制就是01000001，怎样把这8位数据发送给PC机呢？

* 双方约定好波特率（每一位占据的时间）；

* 规定传输协议

  *  原来是高电平，ARM拉低电平，保持1bit时间；
  *  PC在低电平开始处计时；
  *  ARM根据数据依次驱动TxD的电平，同时PC依次读取RxD引脚电平，获得数据；

![image-20230608224838709](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608224838709.png)

前面图中提及到了逻辑电平，也就是说代表信号1的引脚电平是人为规定的。
如图是TTL/CMOS逻辑电平下，传输‘A’时的波形：

![image-20230608224855475](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608224855475.png)

在xV至5V之间，就认为是逻辑1，在0V至yV之间就为逻辑0。

如图是RS-232逻辑电平下，传输‘A’时的波形：

![image-20230608224906793](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608224906793.png)

在-12V至-3V之间，就认为是逻辑1，在+3V至+12V之间就为逻辑0。

RS-232的电平比TTL/CMOS高，能传输更远的距离，在工业上用得比较多。

市面上大多数ARM芯片都不止一个串口，一般使用串口0来调试，其它串口来外接模块。



### 串口电平转换TTL->232

ARM芯片上得串口都是TTL电平的，通过板子上或者外接的电平转换芯片，转成RS232接口，连接到电脑的RS232串口上，实现两者的数据传输。
![image-20230608225009696](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608225009696.png)
现在的电脑越来越少有RS232串口的接口，当USB是几乎都有的。因此使用USB串口芯片将ARM芯片上的TTL电平转换成USB串口协议，即可通过USB与电脑数据传输。
![image-20230608225015800](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608225015800.png)
上面的两种方式，对ARM芯片的编程操作都是一样的。

### 内部结构

ARM芯片是如何发送/接收数据？
如图所示串口结构图：

![image-20230608233131986](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608233131986.png)

要发送数据时，CPU控制内存要发送的数据通过FIFO传给UART单位，UART里面的移位器，依次将数据发送出去，在发送完成后产生中断提醒CPU传输完成。
接收数据时，获取接收引脚的电平，逐位放进接收移位器，再放入FIFO，写入内存。在接收完成后产生中断提醒CPU传输完成。

## 设备节点差异

参考资料

* [解密TTY](https://www.cnblogs.com/liqiuhao/p/9031803.html)
* [彻底理解Linux的各种终端类型以及概念](https://blog.csdn.net/dog250/article/details/78766716)
* [Linux终端和Line discipline图解](https://blog.csdn.net/dog250/article/details/78818612)
* [What Are Teletypes, and Why Were They Used with Computers?](https://www.howtogeek.com/727213/what-are-teletypes-and-why-were-they-used-with-computers/)

| 设备节点                            | 含义                                         |
| ----------------------------------- | -------------------------------------------- |
| /dev/ttyS0、/dev/ttySAC0            | 串口                                         |
| /dev/tty1、/dev/tty2、/dev/tty3、…… | 虚拟终端设备节点(Ctrl+Alt+F3/4..)            |
| /dev/tty0                           | 前台终端                                     |
| /dev/tty                            | 程序自己的终端，可能是串口、也可能是虚拟终端 |
| /dev/console                        | 控制台，又内核的cmdline参数确定              |



| 术语     | 含义                                                         |
| -------- | ------------------------------------------------------------ |
| TTY      | 来自teletype，最古老的输入输出设备，现在用来表示内核的一套驱动系统 |
| Terminal | 终端，暗含远端之意，也是一个输入输出设备，可能是真实设备，也可能是虚拟设备 |
| Console  | 控制台，含控制之意，也是一种Terminal，权限更大，可以查看内核打印信息 |
| UART     | 串口，它的驱动程序包含在TTY驱动体系之内                      |

### /dev/ttyN(N=1,2,3,...)

![image-20230608231535220](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608231535220.png)

/dev/tty3、/dev/tty4：表示某个程序使用的虚拟终端

```shell
// 在tty3、tty4终端来回切换，执行命令

echo hello > /dev/tty3 #tty4执行后tty3打印hello
echo hi    > /dev/tty4 #tty3执行后tty4打印hi
```

### /dev/tty0

/dev/tty0：表示前台程序的虚拟终端

* 你正在操作的界面，就是前台程序

* 其他后台程序访问/dev/tty0的话，就是访问前台程序的终端，切换前台程序时，/dev/tty0是变化的

```shell
// 1. 在tty3终端执行如下命令
// 2. 然后在tty3、tty4来回切换
//切换到哪个终端，就会在哪个终端上打印
while [ 1 ]; do echo msg_from_tty3 > /dev/tty0; sleep 5; done
```



### /dev/tty

/dev/tty表示本程序的终端，可能是虚拟终端，也可能是真实的中断。

程序A在前台、后台间切换，它自己的/dev/tty都不会变。

```shell
// 1. 在tty3终端执行如下命令
// 2. 然后在tty3、tty4来回切换

while [ 1 ]; do echo msg_from_tty3 > /dev/tty; sleep 5; done
```

### Terminal和Console的差别

Terminal含有远端的意思，中文为：终端。Console翻译为控制台，可以理解为权限更大、能查看更多信息。

比如我们可以在Console上看到内核的打印信息，从这个角度上看：

* Console是某一个Terminal
* Terminal并不都是Console。
* 我们可以从多个Terminal中选择某一个作为Console
* 很多时候，两个概念混用，并无明确的、官方的定义

### /dev/console

选哪个？内核的打印信息从哪个设备上显示出来？
可以通过内核的cmdline来指定，
比如: console=ttyS0 console=tty
我不想去分辨这个设备是串口还是虚拟终端，
有没有办法得到这个设备？
有！通过/dev/console！
console=ttyS0时：/dev/console就是ttyS0
console=tty时：/dev/console就是前台程序的虚拟终端
console=tty0时：/dev/console就是前台程序的虚拟终端
console=ttyN时：/dev/console就是/dev/ttyN
console有多个取值时，使用最后一个取值来判断

```shell
[root@100ask:/sys/bus/nvmem/devices/4-00500]# cat /proc/cmdline
console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw
```

### 小结

![image-20210714151502368](https://gitee.com/zhanghang1999/typora-picture/raw/master/09_tty_console.png)

## 代码流程

### 行规程

以下文字引用自参考资料**解密TTY**：

大多数用户都会在输入时犯错，所以退格键会很有用。这当然可以由应用程序本身来实现，但是根据UNIX设计“哲学”，应用程序应尽可能保持简单。为了方便起见，操作系统提供了一个编辑缓冲区和一些基本的编辑命令（退格，清除单个单词，清除行，重新打印），这些命令在行规范（line discipline）内默认启用。高级应用程序可以通过将行规范设置为原始模式（raw mode）而不是默认的成熟或准则模式（cooked and canonical）来禁用这些功能。大多数交互程序（编辑器，邮件客户端，shell，及所有依赖curses或readline的程序）均以原始模式运行，并自行处理所有的行编辑命令。行规范还包含字符回显和回车换行（译者注：\r\n 和 \n）间自动转换的选项。如果你喜欢，可以把它看作是一个原始的内核级sed(1)。

另外，内核提供了几种不同的行规范。一次只能将其中一个连接到给定的串行设备。行规范的默认规则称为N_TTY（drivers/char/n_tty.c，如果你想继续探索的话）。其他的规则被用于其他目的，例如管理数据包交换（ppp，IrDA，串行鼠标），但这不在本文的讨论范围之内。

### 驱动框架

![image-20230608233004862](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608233004862.png)

### 设备树节点

![image-20230608234234473](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608234234473.png)

![image-20230608234244706](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608234244706.png)



板级设备树文件:

![image-20230608234255372](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608234255372.png)

### 代码分析(来自wowo)

![tty-1](https://gitee.com/zhanghang1999/typora-picture/raw/master/bcddafd8286191e2319615a12b08824620180420100152.gif)

![tty-2](https://gitee.com/zhanghang1999/typora-picture/raw/master/cba9808fc3cf1aee8accc2d6d5530e7a20180420100153.gif)

![tty-3](https://gitee.com/zhanghang1999/typora-picture/raw/master/e530afc715f9fb9d88c67c2c2a86a09b20180420100154.gif)

![tty-4](https://gitee.com/zhanghang1999/typora-picture/raw/master/585ef07160941003219bf60ee0e5433520180420100155.gif)

![tty-5](https://gitee.com/zhanghang1999/typora-picture/raw/master/a9d3c6e0dc7c148e663e70a3ca0db11920180420100156.gif)

![tty-6](https://gitee.com/zhanghang1999/typora-picture/raw/master/6a46b55d88897df9309736b1cbd6079420180420100156.gif)

![tty-7](https://gitee.com/zhanghang1999/typora-picture/raw/master/c816eb62f4349c8b89a19000e4253e9920180420100157.gif)

![tty-8](https://gitee.com/zhanghang1999/typora-picture/raw/master/a3eb1ebe671051653f5f9567f32406cc20180420100158.gif)

![tty-9](https://gitee.com/zhanghang1999/typora-picture/raw/master/edd8d57d15a1e22576c0c17177756ecb20180420100159.gif)

![tty-10](https://gitee.com/zhanghang1999/typora-picture/raw/master/2468da7d6035fdde994297c990820c4220180420100200.gif)

![tty-11](https://gitee.com/zhanghang1999/typora-picture/raw/master/a6a7cf23e1a5baa7603c70eb9630c02320180420100201.gif)

![tty-12](https://gitee.com/zhanghang1999/typora-picture/raw/master/447d363de2320567367a84989d0e184a20180420100201.gif)

![tty-13](https://gitee.com/zhanghang1999/typora-picture/raw/master/7e0579a694ffb30e21d1866dc36af1f620180420100202.gif)

![tty-14](https://gitee.com/zhanghang1999/typora-picture/raw/master/0faf3458db458e97bbeb664881d6516820180420100203.gif)

![tty-15](https://gitee.com/zhanghang1999/typora-picture/raw/master/8fdb13634e11a47254f7afc94f72db5420180420100204.gif)

![tty-16](https://gitee.com/zhanghang1999/typora-picture/raw/master/e5c8a000df2080dadda33fac5095556920180420100205.gif)

![tty-17](https://gitee.com/zhanghang1999/typora-picture/raw/master/59221280526f6824f6c331a0a1ff807420180420100205.gif)

![tty-18](https://gitee.com/zhanghang1999/typora-picture/raw/master/b2865931e21740fac709e03b30dcf79b20180420100206.gif)

![tty-19](https://gitee.com/zhanghang1999/typora-picture/raw/master/e2f752c17aa9cfb69afa0b7832876e0420180420100207.gif)

### 总结

1.系统先调用uart_register_driver注册一个uart_driver结构，主要流程：

- 分配uart_driver->uart_state[nr]空间，nr是uart设备个数由uart_driver指定，uart_driver是一个静态的结构。
- 分配一个tty_driver结构，在此结构中保存来自uart_driver的主次设备号，uart_driver->tty_driver=tty_driver
- tty_driver绑定核心层处理操作集uart_ops
- 初始化tty_driver的num为nr,分配cdev[], tty_struct[],tty_port[],ktermios[]指针数组
- 初始化uart_state[nr]中的tty_port，这个成员中有缓冲区，工作队列flush_to_ldisc和操作集uart_port_ops
- 注册tty_driver，实际只是分配主次设备号区间和将tty_driver加入一个全局的链表

2.调用platform_driver_register注册一个实际的uart设备，主要流程：

- 从设备树获取设备的信息如中断，时钟，寄存器基地址等，还有一个设备编号line
- 中断申请，（rx,tx中断，可以共用一个中断号，也可以分开），硬件地址资源获取，虚拟映射
- 分配一个imx_port其中包含了一个uart_port结构，使用上一步的资源初始化uart_port，并绑定底层操作集imx_fops到uart_port，imx_port中会保存更多的硬件信息
- 将imx_port保存到全局数组，数组下边就是line
- 调用uart_add_one_port将uart_port注册到uart_driver中：
  - 将uart_port保存到uart_driver->uart_state[line]->uart_port中
  - 将uart_state[line]保存到uart_port->state中，实现相互访问
  - uart_port->minor为uart_driver->tty_driver->minor_start+line(minor_start来自uart_driver)
  - 调用tty_port_link_device，将uart_driver->uart_state[liane].port赋给tty_driver->tty_port[line]
  - 调用tty_register_device_attr：
    - 使用主次设备号生成设备号
    - 根据tty_driver->name和line确定设备名，(tty_driver->name来自uart_driver->dev_name)
    - 调用tty_cdev_add，分配一个cdev结构，保存在tty_driver->cdev[line],绑定fops=tty_fops,调用cdev_add注册cdev
    - 分配一个struct device结构，绑定devt,class为tty_class,parent为uart_port->dev,设置device的name，调用device_register注册设备，最终形成/dev/tty*节点，如imx，uart_driver->name为ttymxc，而此设备的line为4，则节点为/dev/ttymxc4

### 设备操作

#### tty_open

- 调用tty_alloc_file分配一个tty_file_private结构，并保存到file->private

- 调用tty_open_by_driver,还函数根据主次设备号获取保存在tty_drivers全局链表中的tty_driver
  - 调用tty_init_dev:
    - 调用alloc_tty_struct分配一个tty_struct名为tty，调用tty_ldisc_init初始化线路规程ld，绑定线路规程操作集ld->ops,tty->ld_ldisc=ld，初始化tty->driver为tty_driver,tty->ops为driver->ops即uart_ops
    - tty->port=tty_driver->ports[idx],idx为前面的line
    - 调用线路规程的open函数n_tty_open
- 调用uart_open:
  - 调用port->ops->acitve
  - 调用uart_startup
    - 调用硬件startup函数如imx_startup这个函数会完成硬件初始化工作，并打开接收中断

#### tty_read

- 调用线路规程的read即n_tty_read函数：
  - 此函数循环等待tty->disc_data->ld_data缓冲区数组可读
  - 判断打开方式是否有O_NONBLOCK直接返回-EAGAIN
  - signal_pending是否被信号唤醒：返回-ERESTARTSYS
  - wait_woken进入睡眠



read被正常唤醒路径：

- 数据进入串口触发中断接收处理函数：
  - 调用tty_insert_flip_char将字符一个一个插入uart_port.state->port.buf
  - 没有数据了就调用tty_flip_buffer_push,这个函数会唤醒uart_port.state->port.buf->work工作队列
- 工作队列被激活，调用flush_to_ldisc,将uart_port.state->port.buf的数据上送到线路规程：
  - 调用receive_buf函数，uart_port.state->port.buf缓冲区是链表组成，每个缓冲区都保存一块数据，缓冲区容量不够会新分配一个链表节点保存数据，这个函数会将一块数据中的普通字符和标志分开存放（实际在链表节点中字符和标志也是分开的，只是在一个缓冲区的不同位置）这个函数只是将两个位置输出了
  - 调用线路规程的receive_buf2函数处理数据：
    - 调用__receive_buf，这个函数会对每个字符分析，并调用put_tty_queue将字符依次放入线路规程缓冲区
    - 数据处理完成后调用kill_fasync和wake_up_interruptible_poll唤醒信号驱动,等待队列



- wait_woken被唤醒后，会调用canon_copy_from_read_buf/copy_from_read_buf将数据拷贝到用户空间

#### tty_write

- 为tty_struct分配一个写缓冲区
- 循环执行：
  - 调用copy_from_user将数据从用户空间拷贝到tty_struct->write_buf中
  - 调用线路规程的write:n_tty_write
    - 循环调用uart_wirte处理，直到数据全部写完，这里当uart_write返回0时，说明缓冲区已满无法写入数据，进入休眠
      - 将数据从tty_struct->write_buf拷贝到uart_state的xmit循环buffer中
      - 调用__uart_start：
        - 调用imx_start_tx,这个函数会开启串口的写中断，这个中断会在串口没有数据时一直产生
        - imx_start_tx会导致中断处理函数被调用，即imx_start_tx：
          - 如果发现缓冲区uart_port.state->xmit数据为空就禁用中断（防止持续产生）
          - 将数据依次写入串口
          - 当数据写完后，调用uart_write_wakeup唤醒写操作，此时缓冲器未满，可以继续写数据。再次禁用写中断。n_tty_write被唤醒继续调用uart_write写数据到缓冲器并使能写中断。

### stap脚本追踪

使用stap脚本追踪/drivers/tty目录下所有函数的调用，脚本如下,将串口rx,tx短接测试：

```shell
global tty_get_device_enabled = 1

probe begin {
        printf("begin\n")/*开始检测，脚本启动需要一定时间，最迟也需要等begin打印出来，脚本才会开始监视内核*/
}

/*这里表示要监视内核中'*'任意路径下fs/open.c中所有函数*/
probe kernel.function("*@serial/*.c").call {
        if (execname() == "a.out") {
                printf("%s -> %s\n", thread_indent(4), ppfunc())/*当进程名为a.out时，打印进程名和函数名*/
        }
}
/*同上，不同在于这里是函数返回时*/
probe kernel.function("*@serial/*.c").return {
        if (execname() == "a.out") {
                printf("%s -> %s\n", thread_indent(-4), ppfunc())
        }
}

/*这里表示要监视内核中'*'任意路径下fs/open.c中所有函数*/
probe kernel.function("*@tty/*.c").call {
        if (tty_get_device_enabled && ppfunc() != "dev_match_devt" && execname() == "a.out") {
                printf("%s -> %s\n", thread_indent(4), ppfunc())/*当进程名为a.out时，打印进程名和函数名*/
        }
}
/*同上，不同在于这里是函数返回时*/
probe kernel.function("*@tty/*.c").return {
        if (tty_get_device_enabled && ppfunc() != "dev_match_devt" && execname() == "a.out") {
                printf("%s -> %s\n", thread_indent(-4), ppfunc())/*当进程名为a.out时，打印进程名和函数名*/
        }
}

```

APP程序如下：

```cpp
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

/* set_opt(fd,115200,8,'N',1) */
int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
        struct termios newtio,oldtio;

        if ( tcgetattr( fd,&oldtio) != 0) {
                perror("SetupSerial 1");
                return -1;
        }

        bzero( &newtio, sizeof( newtio ) );
        newtio.c_cflag |= CLOCAL | CREAD;
        newtio.c_cflag &= ~CSIZE;

        newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
        newtio.c_oflag  &= ~OPOST;   /*Output*/

        switch( nBits )
        {
        case 7:
                newtio.c_cflag |= CS7;
        break;
        case 8:
                newtio.c_cflag |= CS8;
        break;
        }

        switch( nEvent )
        {
        case 'O':
                newtio.c_cflag |= PARENB;
                newtio.c_cflag |= PARODD;
                newtio.c_iflag |= (INPCK | ISTRIP);
        break;
        case 'E':
                newtio.c_iflag |= (INPCK | ISTRIP);
                newtio.c_cflag |= PARENB;
                newtio.c_cflag &= ~PARODD;
        break;
        case 'N':
                newtio.c_cflag &= ~PARENB;
        break;
        }

        switch( nSpeed )
        {
        case 2400:
                cfsetispeed(&newtio, B2400);
                cfsetospeed(&newtio, B2400);
        break;
        case 4800:
                cfsetispeed(&newtio, B4800);
                cfsetospeed(&newtio, B4800);
        break;
        case 9600:
                cfsetispeed(&newtio, B9600);
                cfsetospeed(&newtio, B9600);
        break;
        case 115200:
                cfsetispeed(&newtio, B115200);
                cfsetospeed(&newtio, B115200);
        break;
        default:
                cfsetispeed(&newtio, B9600);
                cfsetospeed(&newtio, B9600);
        break;
        }

        if( nStop == 1 )
                newtio.c_cflag &= ~CSTOPB;
        else if ( nStop == 2 )
                newtio.c_cflag |= CSTOPB;

        newtio.c_cc[VMIN]  = 1;  /* 读数据时的最小字节数: 没读到这些数据我就不返回! */
        newtio.c_cc[VTIME] = 0; /* 等待第1个数据的时间:
                                 * 比如VMIN设为10表示至少读到10个数据才返回,
                                 * 但是没有数据总不能一直等吧? 可以设置VTIME(单位是10秒)
                                 * 假设VTIME=1，表示:
                                 *    10秒内一个数据都没有的话就返回
                                 *    如果10秒内至少读到了1个字节，那就继续等待，完全读到VMIN个数据再返回
                                 */

        tcflush(fd,TCIFLUSH);

        if((tcsetattr(fd,TCSANOW,&newtio))!=0)
        {
                perror("com set error");
                return -1;
        }
        //printf("set done!\n");
        return 0;
}

int open_port(char *com)
{
        int fd;
        //fd = open(com, O_RDWR|O_NOCTTY|O_NDELAY);
        fd = open(com, O_RDWR|O_NOCTTY);
    if (-1 == fd){
                return(-1);
    }

          if(fcntl(fd, F_SETFL, 0)<0) /* 设置串口为阻塞状态*/
          {
                        printf("fcntl failed!\n");
                        return -1;
          }

          return fd;
}


/*
 * ./serial_send_recv <dev>
 */
int main(int argc, char **argv)
{
        int fd;
        int iRet;
        char c;

        /* 1. open */

        /* 2. setup
         * 115200,8N1
         * RAW mode
         * return data immediately
         */

        /* 3. write and read */

        if (argc != 2)
        {
                printf("Usage: \n");
                printf("%s </dev/ttySAC1 or other>\n", argv[0]);
                return -1;
        }
        sleep(10);
        printf("open\n");
        fd = open_port(argv[1]);
        if (fd < 0)
        {
                printf("open %s err!\n", argv[1]);
                return -1;
        }
        //printf("open\n");
        sleep(5);
        iRet = set_opt(fd, 115200, 8, 'N', 1);
        if (iRet)
        {
                printf("set port err!\n");
                return -1;
        }
        sleep(5);
        printf("Enter a char: ");
        while (1)
        {
                scanf("%c", &c);
                sleep(5);
                iRet = write(fd, &c, 1);
                sleep(5);
                iRet = read(fd, &c, 1);
        //      if (iRet == 1)
        //              printf("get: %02x %c\n", c, c);
        //      else
        //              printf("can not get data\n");
        }

        return 0;
}

```

shell交互如下:

```shell
[root@100ask:/mnt]# ./a.out /dev/ttymxc5
open #printf
Enter a char: 0 #Enter a char: 为printf，0\n为输入
ad #ad\n为输入
```

stp脚本输出如下：

```shell
 0 a.out(590):    -> tty_write #这个tty_write对应代码中printf("open\n");  这个打印并不会输出到open的串口，而是输出到由ssh打开的虚拟终端/dev/ptsN，这样的终端同样适用于tty子系统
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> n_tty_write
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_write_room
     0 a.out(590):                -> pty_write_room
     0 a.out(590):                    -> tty_buffer_space_avail
     0 a.out(590):                    -> tty_buffer_space_avail
     0 a.out(590):                -> pty_write_room
     0 a.out(590):            -> tty_write_room
     0 a.out(590):            -> pty_write
     0 a.out(590):                -> tty_insert_flip_string_fixed_flag
     0 a.out(590):                    -> __tty_buffer_request_room
     0 a.out(590):                    -> __tty_buffer_request_room
     0 a.out(590):                -> tty_insert_flip_string_fixed_flag
     0 a.out(590):                -> tty_flip_buffer_push
     0 a.out(590):                -> tty_flip_buffer_push
     0 a.out(590):            -> pty_write
     0 a.out(590):            -> tty_write_room
     0 a.out(590):                -> pty_write_room
     0 a.out(590):                    -> tty_buffer_space_avail
     0 a.out(590):                    -> tty_buffer_space_avail
     0 a.out(590):                -> pty_write_room
     0 a.out(590):            -> tty_write_room
     0 a.out(590):            -> do_output_char
     0 a.out(590):                -> pty_write
     0 a.out(590):                    -> tty_insert_flip_string_fixed_flag
     0 a.out(590):                        -> __tty_buffer_request_room
     0 a.out(590):                        -> __tty_buffer_request_room
     0 a.out(590):                    -> tty_insert_flip_string_fixed_flag
     0 a.out(590):                    -> tty_flip_buffer_push
     0 a.out(590):                    -> tty_flip_buffer_push
     0 a.out(590):                -> pty_write
     0 a.out(590):            -> do_output_char
     0 a.out(590):        -> n_tty_write
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_write
     0 a.out(590):    -> tty_open #这个对应open_port中的open函数
     0 a.out(590):        -> tty_init_dev
     0 a.out(590):            -> alloc_tty_struct
     0 a.out(590):                -> tty_ldisc_init
     0 a.out(590):                    -> get_ldops
     0 a.out(590):                    -> get_ldops
     0 a.out(590):                -> tty_ldisc_init
     0 a.out(590):                -> __init_ldsem
     0 a.out(590):                -> __init_ldsem
     0 a.out(590):                -> tty_line_name
     0 a.out(590):                -> tty_line_name
     0 a.out(590):            -> alloc_tty_struct
     0 a.out(590):            -> tty_lock
     0 a.out(590):            -> tty_lock
     0 a.out(590):            -> tty_standard_install
     0 a.out(590):                -> tty_init_termios
     0 a.out(590):                    -> tty_termios_input_baud_rate
     0 a.out(590):                    -> tty_termios_input_baud_rate
     0 a.out(590):                    -> tty_termios_baud_rate
     0 a.out(590):                    -> tty_termios_baud_rate
     0 a.out(590):                -> tty_init_termios
     0 a.out(590):            -> tty_standard_install
     0 a.out(590):            -> tty_ldisc_lock
     0 a.out(590):                -> ldsem_down_write
     0 a.out(590):                -> ldsem_down_write
     0 a.out(590):            -> tty_ldisc_lock
     0 a.out(590):            -> tty_ldisc_setup
     0 a.out(590):                -> tty_ldisc_open
     0 a.out(590):                    -> n_tty_open
     0 a.out(590):                        -> n_tty_set_termios
     0 a.out(590):                        -> n_tty_set_termios
     0 a.out(590):                        -> tty_unthrottle
     0 a.out(590):                        -> tty_unthrottle
     0 a.out(590):                    -> n_tty_open
     0 a.out(590):                -> tty_ldisc_open
     0 a.out(590):            -> tty_ldisc_setup
     0 a.out(590):            -> tty_ldisc_unlock
     0 a.out(590):                -> ldsem_up_write
     0 a.out(590):                -> ldsem_up_write
     0 a.out(590):            -> tty_ldisc_unlock
     0 a.out(590):        -> tty_init_dev
     0 a.out(590):        -> tty_driver_kref_put
     0 a.out(590):        -> tty_driver_kref_put
     0 a.out(590):        -> tty_add_file
     0 a.out(590):        -> tty_add_file
     0 a.out(590):        -> check_tty_count
     0 a.out(590):        -> check_tty_count
     0 a.out(590):        -> uart_open# tty_open -> uart_open
     0 a.out(590):            -> uart_open
     0 a.out(590):                -> tty_port_open
     0 a.out(590):                    -> tty_port_tty_set
     0 a.out(590):                        -> tty_kref_put
     0 a.out(590):                        -> tty_kref_put
     0 a.out(590):                    -> tty_port_tty_set
     0 a.out(590):                    -> uart_port_activate#这个函数会开启串口传输，分配串口的缓冲区
     0 a.out(590):                        -> uart_port_activate
     0 a.out(590):                            -> imx_startup# 硬件相关imx_startup函数，这个函数会打开接收中断，并不会打开发送中断
     0 a.out(590):                                -> imx_startup
     0 a.out(590):                                    -> imx_setup_ufcr
     0 a.out(590):                                        -> imx_setup_ufcr
     0 a.out(590):                                        -> imx_setup_ufcr
     0 a.out(590):                                    -> imx_setup_ufcr
     0 a.out(590):                                    -> imx_enable_ms
     0 a.out(590):                                        -> imx_enable_ms
     0 a.out(590):                                            -> mctrl_gpio_enable_ms
     0 a.out(590):                                                -> mctrl_gpio_enable_ms
     0 a.out(590):                                                -> mctrl_gpio_enable_ms
     0 a.out(590):                                            -> mctrl_gpio_enable_ms
     0 a.out(590):                                        -> imx_enable_ms
     0 a.out(590):                                    -> imx_enable_ms
     0 a.out(590):                                -> imx_startup
     0 a.out(590):                            -> imx_startup
     0 a.out(590):                            -> uart_change_speed
     0 a.out(590):                                -> uart_change_speed
     0 a.out(590):                                    -> imx_set_termios# 这个函数会对串口进行一系列设置如波特率等
     0 a.out(590):                                        -> imx_set_termios
     0 a.out(590):                                            -> uart_get_baud_rate
     0 a.out(590):                                                -> uart_get_baud_rate
     0 a.out(590):                                                    -> tty_termios_baud_rate
110017 a.out(590):                                                    -> tty_termios_baud_rate
110017 a.out(590):                                                -> uart_get_baud_rate
110017 a.out(590):                                            -> uart_get_baud_rate
110017 a.out(590):                                            -> uart_get_divisor
110017 a.out(590):                                                -> uart_get_divisor
110017 a.out(590):                                                -> uart_get_divisor
110017 a.out(590):                                            -> uart_get_divisor
110017 a.out(590):                                            -> uart_update_timeout
110017 a.out(590):                                                -> uart_update_timeout
110017 a.out(590):                                                -> uart_update_timeout
110017 a.out(590):                                            -> uart_update_timeout
110017 a.out(590):                                            -> tty_termios_encode_baud_rate
110017 a.out(590):                                            -> tty_termios_encode_baud_rate
110017 a.out(590):                                        -> imx_set_termios
110017 a.out(590):                                    -> imx_set_termios
110017 a.out(590):                                -> uart_change_speed
110017 a.out(590):                            -> uart_change_speed
110017 a.out(590):                        -> uart_port_activate
110017 a.out(590):                    -> uart_port_activate
110017 a.out(590):                    -> tty_port_block_til_ready
110017 a.out(590):                        -> uart_dtr_rts
110017 a.out(590):                            -> uart_dtr_rts
110017 a.out(590):                                -> uart_update_mctrl
110017 a.out(590):                                    -> uart_update_mctrl
110017 a.out(590):                                    -> uart_update_mctrl
110017 a.out(590):                                -> uart_update_mctrl
110017 a.out(590):                            -> uart_dtr_rts
110017 a.out(590):                        -> uart_dtr_rts
110017 a.out(590):                        -> tty_hung_up_p
110017 a.out(590):                        -> tty_hung_up_p
110017 a.out(590):                        -> tty_hung_up_p
110017 a.out(590):                        -> tty_hung_up_p
110017 a.out(590):                    -> tty_port_block_til_ready
110017 a.out(590):                -> tty_port_open
110017 a.out(590):            -> uart_open
110017 a.out(590):        -> uart_open
110017 a.out(590):        -> tty_unlock
110017 a.out(590):            -> tty_kref_put
110017 a.out(590):            -> tty_kref_put
110017 a.out(590):        -> tty_unlock
110017 a.out(590):    -> tty_open







     0 a.out(590):    -> tty_ioctl#对应set_opt中的tcgetattr/tcsetattr中的ioctl函数
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> uart_ioctl
     0 a.out(590):            -> uart_ioctl
     0 a.out(590):            -> uart_ioctl
     0 a.out(590):        -> uart_ioctl
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_ioctl
     0 a.out(590):            -> n_tty_ioctl_helper
     0 a.out(590):                -> tty_mode_ioctl
     0 a.out(590):                    -> copy_termios
     0 a.out(590):                    -> copy_termios
     0 a.out(590):                -> tty_mode_ioctl
     0 a.out(590):            -> n_tty_ioctl_helper
     0 a.out(590):        -> n_tty_ioctl
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_ioctl
     0 a.out(590):    -> tty_ioctl
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_buffer_flush
     0 a.out(590):        -> tty_buffer_flush
     0 a.out(590):        -> uart_ioctl
     0 a.out(590):            -> uart_ioctl
     0 a.out(590):            -> uart_ioctl
     0 a.out(590):        -> uart_ioctl
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_ioctl
     0 a.out(590):            -> n_tty_ioctl_helper
     0 a.out(590):                -> tty_check_change
     0 a.out(590):                -> tty_check_change
     0 a.out(590):                -> __tty_perform_flush
     0 a.out(590):                    -> n_tty_flush_buffer
     0 a.out(590):                        -> n_tty_kick_worker
     0 a.out(590):                        -> n_tty_kick_worker
     0 a.out(590):                    -> n_tty_flush_buffer
     0 a.out(590):                    -> tty_unthrottle
     0 a.out(590):                    -> tty_unthrottle
     0 a.out(590):                -> __tty_perform_flush
     0 a.out(590):            -> n_tty_ioctl_helper
     0 a.out(590):        -> n_tty_ioctl
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_ioctl
     0 a.out(590):    -> tty_ioctl
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> uart_ioctl
     0 a.out(590):            -> uart_ioctl
     0 a.out(590):            -> uart_ioctl
     0 a.out(590):        -> uart_ioctl
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_ioctl
     0 a.out(590):            -> n_tty_ioctl_helper
     0 a.out(590):                -> tty_mode_ioctl
     0 a.out(590):                    -> set_termios
     0 a.out(590):                        -> tty_check_change
     0 a.out(590):                        -> tty_check_change
     0 a.out(590):                        -> tty_termios_input_baud_rate
     0 a.out(590):                        -> tty_termios_input_baud_rate
     0 a.out(590):                        -> tty_ldisc_ref
     0 a.out(590):                            -> ldsem_down_read_trylock
     0 a.out(590):                            -> ldsem_down_read_trylock
     0 a.out(590):                        -> tty_ldisc_ref
     0 a.out(590):                        -> tty_ldisc_deref
     0 a.out(590):                            -> ldsem_up_read
     0 a.out(590):                            -> ldsem_up_read
     0 a.out(590):                        -> tty_ldisc_deref
     0 a.out(590):                        -> tty_set_termios
     0 a.out(590):                            -> uart_set_termios
     0 a.out(590):                                -> uart_set_termios
     0 a.out(590):                                -> uart_set_termios
     0 a.out(590):                            -> uart_set_termios
     0 a.out(590):                            -> tty_ldisc_ref
     0 a.out(590):                                -> ldsem_down_read_trylock
     0 a.out(590):                                -> ldsem_down_read_trylock
     0 a.out(590):                            -> tty_ldisc_ref
     0 a.out(590):                            -> n_tty_set_termios
     0 a.out(590):                            -> n_tty_set_termios
     0 a.out(590):                            -> tty_ldisc_deref
     0 a.out(590):                                -> ldsem_up_read
     0 a.out(590):                                -> ldsem_up_read
     0 a.out(590):                            -> tty_ldisc_deref
     0 a.out(590):                        -> tty_set_termios
     0 a.out(590):                    -> set_termios
     0 a.out(590):                -> tty_mode_ioctl
     0 a.out(590):            -> n_tty_ioctl_helper
     0 a.out(590):        -> n_tty_ioctl
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_ioctl







     0 a.out(590):    -> tty_write#对应printf("Enter a char: ");打印到虚拟终端
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> n_tty_write
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_write_room
     0 a.out(590):                -> pty_write_room
     0 a.out(590):                    -> tty_buffer_space_avail
     0 a.out(590):                    -> tty_buffer_space_avail
     0 a.out(590):                -> pty_write_room
     0 a.out(590):            -> tty_write_room
     0 a.out(590):            -> pty_write
     0 a.out(590):                -> tty_insert_flip_string_fixed_flag
     0 a.out(590):                    -> __tty_buffer_request_room
     0 a.out(590):                    -> __tty_buffer_request_room
     0 a.out(590):                -> tty_insert_flip_string_fixed_flag
     0 a.out(590):                -> tty_flip_buffer_push
     0 a.out(590):                -> tty_flip_buffer_push
     0 a.out(590):            -> pty_write
     0 a.out(590):        -> n_tty_write
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_write
     0 a.out(590):    -> tty_read#对应scanf函数,scanf函数会调用read系统调用导致这里的阻塞，这个read是虚拟终端的read而不是实际打开的串口的read
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_read
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> tty_buffer_flush_work
     0 a.out(590):            -> tty_buffer_flush_work
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_hung_up_p



#阻塞中等待输入
#输入0回车后解除阻塞
#当用户输入数据回车后，scanf函数运行，这个函数会在循环中被调用2次，一次是字符'0'导致的，一次是回车导致的，这里的解除阻塞是因为'0'
#这里涉及到接收中断的中断发生，处理，激活工作队列，工作队列解除read的阻塞，以及read的数据处理(睡眠在stap中没有包含对应的函数，中断和工作队列是异步的没有跟踪，数据处理的内联函数编译时被优化了无法跟踪)
43229972 a.out(590):            -> n_tty_kick_worker
43229972 a.out(590):            -> n_tty_kick_worker
43229972 a.out(590):            -> tty_wakeup
43229972 a.out(590):            -> tty_wakeup
43229972 a.out(590):            -> n_tty_kick_worker
43229972 a.out(590):            -> n_tty_kick_worker
43229972 a.out(590):        -> n_tty_read
43229972 a.out(590):        -> tty_ldisc_deref
43229972 a.out(590):            -> ldsem_up_read
43229972 a.out(590):            -> ldsem_up_read
43229972 a.out(590):        -> tty_ldisc_deref
43229972 a.out(590):    -> tty_read





     0 a.out(590):    -> tty_write#将虚拟终端read的数据0写入串口
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> n_tty_write#
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> uart_write#数据写入缓冲区
     0 a.out(590):                -> uart_write
     0 a.out(590):                    -> __uart_start
     0 a.out(590):                        -> __uart_start
     0 a.out(590):                            -> imx_start_tx
     0 a.out(590):                                -> imx_start_tx#使能发送中断,后续中断产生后会将缓冲区数据从串口发出
     0 a.out(590):                                -> imx_start_tx
     0 a.out(590):                            -> imx_start_tx
     0 a.out(590):                        -> __uart_start
     0 a.out(590):                    -> __uart_start
     0 a.out(590):                -> uart_write
     0 a.out(590):            -> uart_write
     0 a.out(590):        -> n_tty_write
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_write






     0 a.out(590):    -> tty_read#这个是串口的read读取0,和虚拟终端read相似，经过阻塞等待，读取中断发生，数据拷贝，工作队列，等待唤醒，
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_read
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> copy_from_read_buf#从缓冲区拷贝数据到用户
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> tty_unthrottle_safe
     0 a.out(590):            -> tty_unthrottle_safe
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):        -> n_tty_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_read





     0 a.out(590):    -> tty_write#这里写的是回车
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> n_tty_write
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> uart_write
     0 a.out(590):                -> uart_write
     0 a.out(590):                    -> __uart_start
     0 a.out(590):                        -> __uart_start
     0 a.out(590):                            -> imx_start_tx
     0 a.out(590):                                -> imx_start_tx
     0 a.out(590):                                -> imx_start_tx
     0 a.out(590):                            -> imx_start_tx
     0 a.out(590):                        -> __uart_start
     0 a.out(590):                    -> __uart_start
     0 a.out(590):                -> uart_write
     0 a.out(590):            -> uart_write
     0 a.out(590):        -> n_tty_write
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_write





     0 a.out(590):    -> tty_read# 读取回车
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_read
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> tty_unthrottle_safe
     0 a.out(590):            -> tty_unthrottle_safe
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):        -> n_tty_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_read
     0 a.out(590):    -> tty_read#虚拟终端等待读取
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_read
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> tty_buffer_flush_work
     0 a.out(590):            -> tty_buffer_flush_work
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_hung_up_p
10559920 a.out(590):            -> n_tty_kick_worker#唤醒后读到ab回车
10559920 a.out(590):            -> n_tty_kick_worker
10559920 a.out(590):            -> tty_wakeup
10559920 a.out(590):            -> tty_wakeup
10559920 a.out(590):            -> n_tty_kick_worker
10559920 a.out(590):            -> n_tty_kick_worker
10559920 a.out(590):        -> n_tty_read
10559920 a.out(590):        -> tty_ldisc_deref
10559920 a.out(590):            -> ldsem_up_read
10559920 a.out(590):            -> ldsem_up_read
10559920 a.out(590):        -> tty_ldisc_deref
10559920 a.out(590):    -> tty_read
     0 a.out(590):    -> tty_write#虚拟终端的a写入串口
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> n_tty_write
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> uart_write
     0 a.out(590):                -> uart_write
     0 a.out(590):                    -> __uart_start
     0 a.out(590):                        -> __uart_start
     0 a.out(590):                            -> imx_start_tx
     0 a.out(590):                                -> imx_start_tx
     0 a.out(590):                                -> imx_start_tx
     0 a.out(590):                            -> imx_start_tx
     0 a.out(590):                        -> __uart_start
     0 a.out(590):                    -> __uart_start
     0 a.out(590):                -> uart_write
     0 a.out(590):            -> uart_write
     0 a.out(590):        -> n_tty_write
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_write





     0 a.out(590):    -> tty_read#读取串口的a
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_read
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> tty_unthrottle_safe
     0 a.out(590):            -> tty_unthrottle_safe
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):        -> n_tty_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_read




     0 a.out(590):    -> tty_write#b写
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> n_tty_write
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> uart_write
     0 a.out(590):                -> uart_write
     0 a.out(590):                    -> __uart_start
     0 a.out(590):                        -> __uart_start
     0 a.out(590):                            -> imx_start_tx
     0 a.out(590):                                -> imx_start_tx
     0 a.out(590):                                -> imx_start_tx
     0 a.out(590):                            -> imx_start_tx
     0 a.out(590):                        -> __uart_start
     0 a.out(590):                    -> __uart_start
     0 a.out(590):                -> uart_write
     0 a.out(590):            -> uart_write
     0 a.out(590):        -> n_tty_write
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_write





     0 a.out(590):    -> tty_read#读b
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_read
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> tty_unthrottle_safe
     0 a.out(590):            -> tty_unthrottle_safe
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):        -> n_tty_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_read






     0 a.out(590):    -> tty_write#回车写
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> tty_write_lock
     0 a.out(590):        -> n_tty_write
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> process_echoes
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> uart_write
     0 a.out(590):                -> uart_write
     0 a.out(590):                    -> __uart_start
     0 a.out(590):                        -> __uart_start
     0 a.out(590):                            -> imx_start_tx
     0 a.out(590):                                -> imx_start_tx
     0 a.out(590):                                -> imx_start_tx
     0 a.out(590):                            -> imx_start_tx
     0 a.out(590):                        -> __uart_start
     0 a.out(590):                    -> __uart_start
     0 a.out(590):                -> uart_write
     0 a.out(590):            -> uart_write
     0 a.out(590):        -> n_tty_write
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_write_unlock
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_write





     0 a.out(590):    -> tty_read#读回车
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_read
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> copy_from_read_buf
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> tty_unthrottle_safe
     0 a.out(590):            -> tty_unthrottle_safe
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):            -> n_tty_kick_worker
     0 a.out(590):        -> n_tty_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):            -> ldsem_up_read
     0 a.out(590):        -> tty_ldisc_deref
     0 a.out(590):    -> tty_read
     0 a.out(590):    -> tty_read#继续等待虚拟终端的写入，shell中等待写入
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_paranoia_check
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):            -> ldsem_down_read
     0 a.out(590):        -> tty_ldisc_ref_wait
     0 a.out(590):        -> n_tty_read
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> __tty_check_change
     0 a.out(590):            -> tty_buffer_flush_work
     0 a.out(590):            -> tty_buffer_flush_work
     0 a.out(590):            -> tty_hung_up_p
     0 a.out(590):            -> tty_hung_up_p

```



## gps模块串口编程测试

### api

![image-20230609012845686](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609012845686.png)

在Linux系统中，操作设备的统一接口就是：open/ioctl/read/write。

对于UART，又在ioctl之上封装了很多函数，主要是用来设置行规程。

所以对于UART，编程的套路就是：

* open
* 设置行规程，比如波特率、数据位、停止位、检验位、RAW模式、一有数据就返回
* read/write

行规程的参数用结构体termios来表示

![image-20230609013025692](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609013025692.png)

| 函数名      | 作用                                      |
| ----------- | ----------------------------------------- |
| tcgetattr   | get terminal attributes，获得终端的属性   |
| tcsetattr   | set terminal attributes，修改终端参数     |
| tcflush     | 清空终端未完成的输入/输出请求及数据       |
| cfsetispeed | sets the input baud rate，设置输入波特率  |
| cfsetospeed | sets the output baud rate，设置输出波特率 |
| cfsetspeed  | 同时设置输入、输出波特率                  |

### app

设备树已经写有板子扩展接口的uart节点：

![image-20230609000351804](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609000351804.png)

使用的引脚为：

![image-20230609000413177](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609000413177.png)

![image-20230609000517836](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609000517836.png)

app:

```cpp
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

/* set_opt(fd,115200,8,'N',1) */
int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
        struct termios newtio,oldtio;

        if ( tcgetattr( fd,&oldtio) != 0) {
                perror("SetupSerial 1");
                return -1;
        }

        bzero( &newtio, sizeof( newtio ) );
        newtio.c_cflag |= CLOCAL | CREAD;
        newtio.c_cflag &= ~CSIZE;

        newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
        newtio.c_oflag  &= ~OPOST;   /*Output*/

        switch( nBits )
        {
        case 7:
                newtio.c_cflag |= CS7;
        break;
        case 8:
                newtio.c_cflag |= CS8;
        break;
        }

        switch( nEvent )
        {
        case 'O':
                newtio.c_cflag |= PARENB;
                newtio.c_cflag |= PARODD;
                newtio.c_iflag |= (INPCK | ISTRIP);
        break;
        case 'E':
                newtio.c_iflag |= (INPCK | ISTRIP);
                newtio.c_cflag |= PARENB;
                newtio.c_cflag &= ~PARODD;
        break;
        case 'N':
                newtio.c_cflag &= ~PARENB;
        break;
        }

        switch( nSpeed )
        {
        case 2400:
                cfsetispeed(&newtio, B2400);
                cfsetospeed(&newtio, B2400);
        break;
        case 4800:
                cfsetispeed(&newtio, B4800);
                cfsetospeed(&newtio, B4800);
        break;
        case 9600:
                cfsetispeed(&newtio, B9600);
                cfsetospeed(&newtio, B9600);
        break;
        case 115200:
                cfsetispeed(&newtio, B115200);
                cfsetospeed(&newtio, B115200);
        break;
        default:
                cfsetispeed(&newtio, B9600);
                cfsetospeed(&newtio, B9600);
        break;
        }

        if( nStop == 1 )
                newtio.c_cflag &= ~CSTOPB;
        else if ( nStop == 2 )
                newtio.c_cflag |= CSTOPB;

        newtio.c_cc[VMIN]  = 1;  /* 读数据时的最小字节数: 没读到这些数据我就不返回! */
        newtio.c_cc[VTIME] = 0; /* 等待第1个数据的时间:
                                 * 比如VMIN设为10表示至少读到10个数据才返回,
                                 * 但是没有数据总不能一直等吧? 可以设置VTIME(单位是10秒)
                                 * 假设VTIME=1，表示:
                                 *    10秒内一个数据都没有的话就返回
                                 *    如果10秒内至少读到了1个字节，那就继续等待，完全读到VMIN个数据再返回
                                 */

        tcflush(fd,TCIFLUSH);

        if((tcsetattr(fd,TCSANOW,&newtio))!=0)
        {
                perror("com set error");
                return -1;
        }
        //printf("set done!\n");
        return 0;
}

int open_port(char *com)
{
        int fd;
        //fd = open(com, O_RDWR|O_NOCTTY|O_NDELAY);
        fd = open(com, O_RDWR|O_NOCTTY);
    if (-1 == fd){
                return(-1);
    }

          if(fcntl(fd, F_SETFL, 0)<0) /* 设置串口为阻塞状态*/
          {
                        printf("fcntl failed!\n");
                        return -1;
          }

          return fd;
}


int read_gps_raw_data(int fd, char *buf)
{
        int i = 0;
        int iRet;
        char c;
        int start = 0;

        while (1)
        {
                iRet = read(fd, &c, 1);
                if (iRet == 1)
                {
                        if (c == '$')
                                start = 1;
                        if (start)
                        {
                                buf[i++] = c;
                        }
                        if (c == '\n' || c == '\r')
                                return 0;
                }
                else
                {
                        return -1;
                }
        }
}

/* eg. $GPGGA,082559.00,4005.22599,N,11632.58234,E,1,04,3.08,14.6,M,-5.6,M,,*76"<CR><LF> */
int parse_gps_raw_data(char *buf, char *time, char *lat, char *ns, char *lng, char *ew)
{
        char tmp[10];

        if (buf[0] != '$')
                return -1;
        else if (strncmp(buf+3, "GGA", 3) != 0)
                return -1;
        else if (strstr(buf, ",,,,,"))
        {
                printf("Place the GPS to open area\n");
                return -1;
        }
        else {
                //printf("raw data: %s\n", buf);
                sscanf(buf, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,]", tmp, time, lat, ns, lng, ew);
                return 0;
        }
}


/*
 * ./serial_send_recv <dev>
 */
int main(int argc, char **argv)
{
        int fd;
        int iRet;
        char c;
        char buf[1000];
        char time[100];
        char Lat[100];
        char ns[100];
        char Lng[100];
        char ew[100];

        float fLat, fLng;

        /* 1. open */

        /* 2. setup
         * 115200,8N1
         * RAW mode
         * return data immediately
         */

        /* 3. write and read */

        if (argc != 2)
        {
                printf("Usage: \n");
                printf("%s </dev/ttySAC1 or other>\n", argv[0]);
                return -1;
        }

        fd = open_port(argv[1]);
        if (fd < 0)
        {
                printf("open %s err!\n", argv[1]);
                return -1;
        }

        iRet = set_opt(fd, 9600, 8, 'N', 1);
        if (iRet)
        {
                printf("set port err!\n");
                return -1;
        }

        while (1)
        {
                /* eg. $GPGGA,082559.00,4005.22599,N,11632.58234,E,1,04,3.08,14.6,M,-5.6,M,,*76"<CR><LF>*/
                /* read line */
                iRet = read_gps_raw_data(fd, buf);

                /* parse line */
                if (iRet == 0)
                {
                        iRet = parse_gps_raw_data(buf, time, Lat, ns, Lng, ew);
                }

                /* printf */
                if (iRet == 0)
                {
                        printf("Time : %s\n", time);
                        printf("ns   : %s\n", ns);
                        printf("ew   : %s\n", ew);
                        printf("Lat  : %s\n", Lat);
                        printf("Lng  : %s\n", Lng);

                        /* 纬度格式: ddmm.mmmm */
                        sscanf(Lat+2, "%f", &fLat);
                        fLat = fLat / 60;
                        fLat += (Lat[0] - '0')*10 + (Lat[1] - '0');

                        /* 经度格式: dddmm.mmmm */
                        sscanf(Lng+3, "%f", &fLng);
                        fLng = fLng / 60;
                        fLng += (Lng[0] - '0')*100 + (Lng[1] - '0')*10 + (Lng[2] - '0');
                        printf("Lng,Lat: %.06f,%.06f\n", fLng, fLat);
                }
        }

        return 0;
}


```

## 虚拟的串口驱动程序编写

### 框架

![image-20230609013248162](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609013248162.png)

### 数据流

![image-20230609013620937](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609013620937.png)

### 实现proc文件

参考`/proc/cmdline`，怎么找到它对应的驱动？在Linux内核源码下执行以下命令搜索：

```shell
grep "cmdline" * -nr | grep proc
```

得到：

```shell
fs/proc/cmdline.c:26:   proc_create("cmdline", 0, NULL, &cmdline_proc_fops);
```

### 中断主动触发

使用如下函数：

```c
int irq_set_irqchip_state(unsigned int irq, enum irqchip_irq_state which,
			  bool val);
```

在中断子系统中，往GIC寄存器GICD_ISPENDRn写入某一位就可以触发中断。内核代码中怎么访问这些寄存器？
在`drivers\irqchip\irq-gic.c`中可以看到irq_chip中的"irq_set_irqchip_state"被用来设置中断状态：

```cpp
static struct irq_chip gic_chip = {
	.irq_mask		= gic_mask_irq,
	.irq_unmask		= gic_unmask_irq,
	.irq_eoi		= gic_eoi_irq,
	.irq_set_type		= gic_set_type,
	.irq_get_irqchip_state	= gic_irq_get_irqchip_state,
	.irq_set_irqchip_state	= gic_irq_set_irqchip_state, /* 2. 继续搜"irq_set_irqchip_state" */
	.flags			= IRQCHIP_SET_TYPE_MASKED |
				  IRQCHIP_SKIP_SET_WAKE |
				  IRQCHIP_MASK_ON_SUSPEND,
};

static int gic_irq_set_irqchip_state(struct irq_data *d,
				     enum irqchip_irq_state which, bool val)
{
	u32 reg;

	switch (which) {
	case IRQCHIP_STATE_PENDING:
		reg = val ? GIC_DIST_PENDING_SET : GIC_DIST_PENDING_CLEAR; /* 1. 找到寄存器 */
		break;

	case IRQCHIP_STATE_ACTIVE:
		reg = val ? GIC_DIST_ACTIVE_SET : GIC_DIST_ACTIVE_CLEAR;
		break;

	case IRQCHIP_STATE_MASKED:
		reg = val ? GIC_DIST_ENABLE_CLEAR : GIC_DIST_ENABLE_SET;
		break;

	default:
		return -EINVAL;
	}

	gic_poke_irq(d, reg);
	return 0;
}
```



可使用如下代码触发某个中断：

```c
irq_set_irqchip_state(irq, IRQCHIP_STATE_PENDING, 1);
```

### 设备树

虚拟的串口:

```shell
zhhhhhh@ubuntu:~/bsp/IMX6ULL/100ask_imx6ull-sdk/Linux-4.9.88/arch/arm/boot/dts$ diff 100ask_imx6ull-14x14.dts ../../../../../orig_dts/100ask_imx6ull-14x14.dts
37,42d36
<       virtual_uart:virtual_uart{
<               compatible = "virtual_uart";
<               interrupt-parent = <&intc>;
<               interrupts = <GIC_SPI 99 IRQ_TYPE_LEVEL_HIGH>;
<
<       };
89a84
>

```

### 程序

```cpp
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/rational.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/proc_fs.h>

#include <asm/irq.h>

static struct uart_port *virt_port;
static const struct of_device_id virtual_uart_table[] = {
    { .compatible = "virtual_uart" },
    { },
};
#define BUF_LEN  1024
#define NEXT_PLACE(i) ((i+1)&0x3FF)

static unsigned char tx_buf[BUF_LEN];
static unsigned int tx_read_idx = 0;
static unsigned int tx_write_idx = 0;

static unsigned char rx_buf[BUF_LEN];
//static unsigned int rx_read_idx = 0;
static unsigned int rx_write_idx = 0;

struct proc_dir_entry *virt_uart_proc;
static int is_txbuf_empty(void)
{
        return tx_read_idx == tx_write_idx;
}

static int is_txbuf_full(void)
{
        return NEXT_PLACE(tx_write_idx) == tx_read_idx;
}

static int txbuf_put(unsigned char val)
{
        if (is_txbuf_full())
                return -1;
        tx_buf[tx_write_idx] = val;
        tx_write_idx = NEXT_PLACE(tx_write_idx);
        return 0;
}

static int txbuf_get(unsigned char *pval)
{
        if (is_txbuf_empty())
                return -1;
        *pval = tx_buf[tx_read_idx];
        tx_read_idx = NEXT_PLACE(tx_read_idx);
        return 0;
}

static int txbuf_count(void)
{
        if (tx_write_idx >= tx_read_idx)
                return tx_write_idx - tx_read_idx;
        else
                return BUF_LEN + tx_write_idx - tx_read_idx;
}


static struct uart_driver virtual_uart_reg = {
        .owner          = THIS_MODULE,
        .driver_name    = "ttyVirDriver",
        .dev_name       = "ttyVirtual",
        .major          = 0,
        .minor          = 0,
        .nr             = 1,
};
static unsigned int virtual_uart_tx_empty(struct uart_port *port)
{
        return 1;
}

/*app write函数会导致此函数最终被调用，此函数在imx中会导致tx中断处理函数被调用，将数据从串口发出，这里直接拷贝数据到txbuf*/
static void virtual_uart_start_tx(struct uart_port *port)
{
        struct circ_buf *xmit = &port->state->xmit;

        while (!uart_circ_empty(xmit) && !uart_tx_stopped(port)){
                /* send xmit->buf[xmit->tail]
                * out the port here */
                txbuf_put(xmit->buf[xmit->tail]);
                xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
                port->icount.tx++;
        }

        if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
                uart_write_wakeup(port);

}


static void virtual_uart_set_termios(struct uart_port *port, struct ktermios *termios,
                   struct ktermios *old)
{
        return ;
}

static int virtual_uart_startup(struct uart_port *port)
{
        return 0;
}
static void virtual_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
        return ;
}

static unsigned int virtual_uart_get_mctrl(struct uart_port *port)
{
        return 0;
}

static void virtual_uart_stop_tx(struct uart_port *port)
{
        return ;
}
static void virtual_uart_stop_rx(struct uart_port *port)
{
        return ;
}

static void virtual_uart_shutdown(struct uart_port *port)
{
        return ;
}

static const char *virtual_uart_type(struct uart_port *port)
{
        return "VIRT_UART_TYPE";
}

static const struct uart_ops virtual_uart_pops = {
        .tx_empty       = virtual_uart_tx_empty,
        .set_mctrl      = virtual_uart_set_mctrl,
        .get_mctrl      = virtual_uart_get_mctrl,
        .stop_tx        = virtual_uart_stop_tx,
        .start_tx       = virtual_uart_start_tx,
        .stop_rx        = virtual_uart_stop_rx,
        //.enable_ms    = imx_enable_ms,
        //.break_ctl    = imx_break_ctl,
        .startup        = virtual_uart_startup,
        .shutdown       = virtual_uart_shutdown,
        //.flush_buffer = imx_flush_buffer,
        .set_termios    = virtual_uart_set_termios,
        .type           = virtual_uart_type,
        //.config_port  = imx_config_port,
        //.verify_port  = imx_verify_port,

};
/*proc文件的读取，从tx_buf读取数据*/
static ssize_t virtual_uart_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
        int cnt;
        unsigned char val;
        int i;
        int ret;
        cnt = txbuf_count();
        cnt = cnt > size ? size : cnt;
        for(i = 0; i < cnt; i++)
        {
                txbuf_get(&val);
                ret = copy_to_user(buf + i, &val, 1);
        }
        return cnt;
}
/*proc文件的写入，会将rx_buf的数据写入到rx_buf，并触发中断，导致中断处理函数被调用，最后会将数据拷贝到可读缓冲区，app read可以读取数据*/
static ssize_t virtual_uart_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
        int ret;
        ret = copy_from_user(rx_buf, buf, size);
        rx_write_idx = size;

        irq_set_irqchip_state(virt_port->irq, IRQCHIP_STATE_PENDING, 1);
        return size;
}


static const struct file_operations virtual_uart_fops = {
        //.open         = cmdline_proc_open,
        .read           = virtual_uart_read,
        .write          = virtual_uart_write,
        //.llseek               = seq_lseek,
        //.release      = single_release,
};

/*中断处理函数将rx_buf的数据送到行规程，数据最后会保存到可读缓冲区*/
static irqreturn_t virt_uart_rxint(int irq, void *dev_id)
{
        struct tty_port *port = &virt_port->state->port;
        unsigned long flags;
        unsigned int i = 0;

        spin_lock_irqsave(&virt_port->lock, flags);

        while (i < rx_write_idx) {
                virt_port->icount.rx++;
                if (tty_insert_flip_char(port, rx_buf[i++], TTY_NORMAL) == 0)
                        virt_port->icount.buf_overrun++;
        }
        rx_write_idx = 0;

        spin_unlock_irqrestore(&virt_port->lock, flags);
        tty_flip_buffer_push(port);
        return IRQ_HANDLED;
}


static int virtual_uart_probe(struct platform_device *pdev)
{
        int rxirq;
        int ret;

        printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
        virt_uart_proc = proc_create("virtual_uart", 0, NULL, &virtual_uart_fops);
        rxirq = platform_get_irq(pdev, 0);//接收中断

        virt_port = devm_kzalloc(&pdev->dev, sizeof(*virt_port), GFP_KERNEL);

        virt_port->dev = &pdev->dev;

        virt_port->type = PORT_IMX,
        virt_port->iotype = UPIO_MEM;
        virt_port->irq = rxirq;
        virt_port->fifosize = 32;
        virt_port->type = PORT_8250;
        virt_port->ops = &virtual_uart_pops;
        //virt_port->rs485_config = imx_rs485_config;
        //virt_port->rs485.flags =
        //      SER_RS485_RTS_ON_SEND | SER_RS485_RX_DURING_TX;
        virt_port->flags = UPF_BOOT_AUTOCONF;

        ret = devm_request_irq(&pdev->dev, rxirq, virt_uart_rxint, 0,
                                       dev_name(&pdev->dev), virt_port);//中断注册
        return uart_add_one_port(&virtual_uart_reg, virt_port);//注册uart_port
}

static int virtual_uart_remove(struct platform_device *pdev)
{
        uart_remove_one_port(&virtual_uart_reg, virt_port);
        proc_remove(virt_uart_proc);
        return 0;
}

static struct platform_driver virtual_uart_driver =
{
        .probe      = virtual_uart_probe,
    .remove     = virtual_uart_remove,
    .driver     = {
        .name   = "virtual_uart",
        .of_match_table = virtual_uart_table,
    },
};



static int __init virtual_uart_init(void)
{
        int ret = -1;
        printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);

        ret = uart_register_driver(&virtual_uart_reg);//注册uart_driver
        if(ret)
        {
                printk("uart_register_driver error, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
                return -1;
        }
        ret = platform_driver_register(&virtual_uart_driver);//注册platform驱动，会与设备树匹配执行probe,probe中调用uart_add_one_port注册tty设备，形成设备节点
        if(ret)
        {
                printk("platform_driver_register error, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
                return -1;
        }
        return 0;
}

static void __exit virtual_uart_exit(void)
{
        printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
        platform_driver_unregister(&virtual_uart_driver);
        uart_unregister_driver(&virtual_uart_reg);

}

module_init(virtual_uart_init);
module_exit(virtual_uart_exit);

MODULE_LICENSE("GPL");



```



```shell
#写数据到串口会最后会导致start_tx将数据copy到txbuf, proc的read就是从txbuf读取数据更新txbuf读指针
[root@100ask:/]# echo "hahahahah" > /dev/ttyVirtual0    
[root@100ask:/]# cat /proc/virtual_uart
hahahahah

#读串口数据是，proc write会将数据写入rxbuf并产生中断将数据copy到行规程，行规程会将数据写入应用的缓冲区，这里如果先写入再读取串口会读不到数据，因为用户没有分配读取缓冲区（可能在行规程的位置就停止上送了，所以串口必须一直开着
[root@100ask:/]# cat /dev/ttyVirtual0 &
[1] 414
[root@100ask:/]# echo "zzzzzz" > /proc/virtual_uart
[root@100ask:/]# zzzzzz

#这里还能读到数据是因为rxbuf->中断->行规程并没有更新rxbuf指针
[root@100ask:/]# cat /proc/virtual_uart
zzzzzz
#proc read会更新指针
[root@100ask:/]# cat /proc/virtual_uart
[root@100ask:/]#

```

## printk执行流程

![image-20230609185935897](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609185935897.png)

### 使用方法

调试内核、驱动的最简单方法，是使用printk函数打印信息。

printk函数与用户空间的printf函数格式完全相同，它所打印的字符串头部可以加入“\001n”样式的字符。

其中n为0～7，表示这条信息的记录级别，n数值越小级别越高。

**注意**：linux 2.x内核里，打印级别是用"<n>"来表示。

在驱动程序中，可以这样使用printk：

```c
printk("This is an example\n");
printk("\0014This is an example\n");
printk("\0014""This is an example\n");
printk(KERN_WARNING"This is an example\n");
```

在上述例子中：

* 第一条语句没有明确表明打印级别，它被处理前内核会在前面添加默认的打印级别："<4>"

* KERN_WARNING是一个宏，它也表示打印级别，以下是内核所有打印级别

```c
#define KERN_SOH	"\001"		/* ASCII Start Of Header */
#define KERN_SOH_ASCII	'\001'

#define KERN_EMERG	KERN_SOH "0"	/* system is unusable */
#define KERN_ALERT	KERN_SOH "1"	/* action must be taken immediately */
#define KERN_CRIT	KERN_SOH "2"	/* critical conditions */
#define KERN_ERR	KERN_SOH "3"	/* error conditions */
#define KERN_WARNING	KERN_SOH "4"	/* warning conditions */
#define KERN_NOTICE	KERN_SOH "5"	/* normal but significant condition */
#define KERN_INFO	KERN_SOH "6"	/* informational */
#define KERN_DEBUG	KERN_SOH "7"	/* debug-level messages */

```

内核的每条打印信息都有自己的级别，当自己的级别在数值上小于某个阈值时，内核才会打印该信息。

### 配置日志级别过滤

在内核代码`include/linux/kernel.h`中，下面几个宏确定了printk函数怎么处理打印级别：

```c
#define console_loglevel (console_printk[0])
#define default_message_loglevel (console_printk[1])
#define minimum_console_loglevel (console_printk[2])
#define default_console_loglevel (console_printk[3])
```

 

举例说明这几个宏的含义：

① 对于printk(“<n>……”)，只有n小于console_loglevel时，这个信息才会被打印。

② 假设default_message_loglevel的值等于4，如果printk的参数开头没有“<n>”样式的字符，则在printk函数中进一步处理前会自动加上“<4>”；

③ minimum_console_logleve是一个预设值，平时不起作用。通过其他工具来设置console_loglevel的值时，这个值不能小于minimum_console_logleve。

④ default_console_loglevel也是一个预设值，平时不起作用。它表示设置console_loglevel时的默认值，通过其他工具来设置console_loglevel的值时，用到这个值。

 

上面代码中，console_printk是一个数组，它在kernel/printk.c中定义：

```c
/* 数组里的宏在include/linux/printk.h中定义
 */
int console_printk[4] = {
	CONSOLE_LOGLEVEL_DEFAULT,	/* console_loglevel */
	MESSAGE_LOGLEVEL_DEFAULT,	/* default_message_loglevel */
	CONSOLE_LOGLEVEL_MIN,		/* minimum_console_loglevel */
	CONSOLE_LOGLEVEL_DEFAULT,	/* default_console_loglevel */
};

/* Linux 4.9.88 include/linux/printk.h */
#define CONSOLE_LOGLEVEL_DEFAULT 7 /* anything MORE serious than KERN_DEBUG */
#define MESSAGE_LOGLEVEL_DEFAULT CONFIG_MESSAGE_LOGLEVEL_DEFAULT
#define CONSOLE_LOGLEVEL_MIN	 1 /* Minimum loglevel we let people use */

/* Linux 5.4 include/linux/printk.h */
#define CONSOLE_LOGLEVEL_DEFAULT CONFIG_CONSOLE_LOGLEVEL_DEFAULT
#define MESSAGE_LOGLEVEL_DEFAULT CONFIG_MESSAGE_LOGLEVEL_DEFAULT
#define CONSOLE_LOGLEVEL_MIN	 1 /* Minimum loglevel we let people use */
```

 ### 修改过滤配置

挂接proc文件系统后，读取/proc/sys/kernel/printk文件可以得知console_loglevel、default_message_loglevel、minimum_console_loglevel和default_console_loglevel这4个值。

比如执行以下命令，它的结果“7 4 1 7”表示这4个值：

![image-20230609190338924](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609190338924.png)

也可以直接修改/proc/sys/kernel/printk文件来改变这4个值，比如：

```shell
# echo "1 4 1 7" > /proc/sys/kernel/printk
```

这使得console_loglevel被改为1，于是所有的printk信息都不会被打印。

### 函数调用过程

```shell

printk
	vprintk_default
		vprintk_emit #上下文判断和数据处理
			log_store #数据放入缓冲区
			console_unlock #数据根据level过滤和发送到call_console_drivers
				call_console_drivers #数据通过串口发送
```

#### vprintk_emit

```cpp
asmlinkage int vprintk_emit(int facility, int level,
			    const char *dict, size_t dictlen,
			    const char *fmt, va_list args)
{
	static bool recursion_bug;
	static char textbuf[LOG_LINE_MAX];
	char *text = textbuf;
	size_t text_len = 0;
	enum log_flags lflags = 0;
	unsigned long flags;
	int this_cpu;
	int printed_len = 0;
	int nmi_message_lost;
	bool in_sched = false;
	/* cpu currently holding logbuf_lock in this function */
	static unsigned int logbuf_cpu = UINT_MAX;

	if (level == LOGLEVEL_SCHED) {
		level = LOGLEVEL_DEFAULT;
		in_sched = true;
	}

	boot_delay_msec(level);
	printk_delay();

	local_irq_save(flags);//禁止中断
	this_cpu = smp_processor_id();

	/*
	 * Ouch, printk recursed into itself!
	 */
    /*
    内核会尽量避免在同一时刻由多个 CPU 进行日志输出。当一个 CPU 正在持有 logbuf_lock（日志缓冲区锁）时，其他 CPU 将会被阻塞，直到持有锁的 CPU 释放该锁。这种机制确保了在任意时刻只		有一个 CPU 在进行日志输出操作。

	因此，要发生 logbuf_cpu == this_cpu 的情况，通常需要同时满足以下条件：

	1. 在多个 CPU 上几乎同时发生了嵌套的 printk 调用。
	2. logbuf_lock 的持有时间非常短，以至于没有其他 CPU 能够获取该锁。
	3. 当前 CPU 已经持有了 logbuf_lock，而又有一个 printk 函数调用发生，导致logbuf_cpu == this_cpu
	在实际情况中，这种并发的嵌套 printk 调用的发生频率较低，并且在大多数情况下，内核的同步机制可以确保不会出现这种情况。然而，由于并发和特殊情况的存在，虽然可能性较小，但仍然有可能发生。
    */
	if (unlikely(logbuf_cpu == this_cpu)) {
		/*
		 * If a crash is occurring during printk() on this CPU,
		 * then try to get the crash message out but make sure
		 * we can't deadlock. Otherwise just return to avoid the
		 * recursion and return - but flag the recursion so that
		 * it can be printed at the next appropriate moment:
		 */
        
        //检查是否存在崩溃（oops）或者当前进程正在进行锁依赖分析（lockdep_recursing）。如果不存在这些情况，那么将设置一个标志（recursion_bug），然后返回，此次printk的打印或丢失
        
        //lockdep_recursing返回current->lockdep_recursion
		if (!oops_in_progress && !lockdep_recursing(current)) {
			recursion_bug = true;
			local_irq_restore(flags);//恢复中断
			return 0;
		}
        //在发生递归调用期间重置锁状态，以防止在发生崩溃或其他紧急情况时出现死锁，并确保下一次的printk()调用能够正常进行。
		zap_locks();
	}

	lockdep_off();
	/* This stops the holder of console_sem just where we want him */
	raw_spin_lock(&logbuf_lock);//自旋锁获取
	logbuf_cpu = this_cpu;

	if (unlikely(recursion_bug)) {//printk递归调用导致返回后下一次printk会打印的错误信息
		static const char recursion_msg[] =
			"BUG: recent printk recursion!";

		recursion_bug = false;
		/* emit KERN_CRIT message */
		printed_len += log_store(0, 2, LOG_PREFIX|LOG_NEWLINE, 0,
					 NULL, 0, recursion_msg,
					 strlen(recursion_msg));
	}

	nmi_message_lost = get_nmi_message_lost();
	if (unlikely(nmi_message_lost)) {
		text_len = scnprintf(textbuf, sizeof(textbuf),
				     "BAD LUCK: lost %d message(s) from NMI context!",
				     nmi_message_lost);
		printed_len += log_store(0, 2, LOG_PREFIX|LOG_NEWLINE, 0,
					 NULL, 0, textbuf, text_len);
	}

	/*
	 * The printf needs to come first; we need the syslog
	 * prefix which might be passed-in as a parameter.
	 */
	text_len = vscnprintf(text, sizeof(textbuf), fmt, args);//解析printk的数据

	/* mark and strip a trailing newline */
	if (text_len && text[text_len-1] == '\n') {//新行
		text_len--;
		lflags |= LOG_NEWLINE;
	}

	/* strip kernel syslog prefix and extract log level or control flags */
	if (facility == 0) {
		int kern_level;

		while ((kern_level = printk_get_level(text)) != 0) {//从数据中，获取printk的日志等级，
			switch (kern_level) {
			case '0' ... '7':
				if (level == LOGLEVEL_DEFAULT)
					level = kern_level - '0';
				/* fallthrough */
			case 'd':	/* KERN_DEFAULT */
				lflags |= LOG_PREFIX;
				break;
			case 'c':	/* KERN_CONT */
				lflags |= LOG_CONT;
			}

			text_len -= 2;
			text += 2;
		}
	}

	if (level == LOGLEVEL_DEFAULT)
		level = default_message_loglevel;

	if (dict)
		lflags |= LOG_PREFIX|LOG_NEWLINE;

	printed_len += log_output(facility, level, lflags, dict, dictlen, text, text_len);//调用log_store将数据放入缓冲区,level保存日志等级,text保存日志信息

	logbuf_cpu = UINT_MAX;
	raw_spin_unlock(&logbuf_lock);//解除自旋锁
	lockdep_on();
	local_irq_restore(flags);//恢复中断

	/* If called from the scheduler, we can not call up(). */
	if (!in_sched) {
		lockdep_off();//增加锁的引用current->lockdep_recursion++;
		/*
		 * Try to acquire and then immediately release the console
		 * semaphore.  The release will print out buffers and wake up
		 * /dev/kmsg and syslog() users.
		 */
		if (console_trylock())
			console_unlock();//
		lockdep_on();//减少锁的引用current->lockdep_recursion--;
	}

	return printed_len;
}
EXPORT_SYMBOL(vprintk_emit);
```

#### log_store

```cpp
static int log_store(int facility, int level,
		     enum log_flags flags, u64 ts_nsec,
		     const char *dict, u16 dict_len,
		     const char *text, u16 text_len)
{
	struct printk_log *msg;
	u32 size, pad_len;
	u16 trunc_msg_len = 0;

	/* number of '\0' padding bytes to next message */
    /*获取日志所需空间*/
	size = msg_used_size(text_len, dict_len, &pad_len);

	if (log_make_free_space(size)) {//如果日志缓冲区空间不够
		/* truncate the message if it is too long for empty buffer */
		size = truncate_msg(&text_len, &trunc_msg_len,
				    &dict_len, &pad_len);//尝试将此次打印的信息截断
		/* survive when the log buffer is too small for trunc_msg */
		if (log_make_free_space(size))//再次尝试放入截断后的信息，如果空间还是不够就返回
			return 0;
	}
	/*循环buffer索引回滚*/
	if (log_next_idx + size + sizeof(struct printk_log) > log_buf_len) {
		/*
		 * This message + an additional empty header does not fit
		 * at the end of the buffer. Add an empty header with len == 0
		 * to signify a wrap around.
		 */
		memset(log_buf + log_next_idx, 0, sizeof(struct printk_log));
		log_next_idx = 0;
	}
	/*数据放入缓冲区*/
	/* fill message */
	msg = (struct printk_log *)(log_buf + log_next_idx);
	memcpy(log_text(msg), text, text_len);
	msg->text_len = text_len;
	if (trunc_msg_len) {
		memcpy(log_text(msg) + text_len, trunc_msg, trunc_msg_len);
		msg->text_len += trunc_msg_len;
	}
	memcpy(log_dict(msg), dict, dict_len);
	msg->dict_len = dict_len;
	msg->facility = facility;
	msg->level = level & 7;
	msg->flags = flags & 0x1f;
	if (ts_nsec > 0)
		msg->ts_nsec = ts_nsec;
	else
		msg->ts_nsec = local_clock();
	memset(log_dict(msg) + dict_len, 0, pad_len);
	msg->len = size;

	/* insert message */
	log_next_idx += msg->len;
	log_next_seq++;

	return msg->text_len;
}

```

#### console_unlock

```cpp

/**
 * console_unlock - unlock the console system
 *
 * Releases the console_lock which the caller holds on the console system
 * and the console driver list.
 *
 * While the console_lock was held, console output may have been buffered
 * by printk().  If this is the case, console_unlock(); emits
 * the output prior to releasing the lock.
 *
 * If there is output waiting, we wake /dev/kmsg and syslog() users.
 *
 * console_unlock(); may be called from any context.
 */
void console_unlock(void)
{
	static char ext_text[CONSOLE_EXT_LOG_MAX];
	static char text[LOG_LINE_MAX + PREFIX_MAX];
	static u64 seen_seq;
	unsigned long flags;
	bool wake_klogd = false;
	bool do_cond_resched, retry;

	if (console_suspended) {
		up_console_sem();
		return;
	}

	/*
	 * Console drivers are called under logbuf_lock, so
	 * @console_may_schedule should be cleared before; however, we may
	 * end up dumping a lot of lines, for example, if called from
	 * console registration path, and should invoke cond_resched()
	 * between lines if allowable.  Not doing so can cause a very long
	 * scheduling stall on a slow console leading to RCU stall and
	 * softlockup warnings which exacerbate the issue with more
	 * messages practically incapacitating the system.
	 */
	do_cond_resched = console_may_schedule;
	console_may_schedule = 0;

again:
	/*
	 * We released the console_sem lock, so we need to recheck if
	 * cpu is online and (if not) is there at least one CON_ANYTIME
	 * console.
	 */
	if (!can_use_console()) {
		console_locked = 0;
		up_console_sem();
		return;
	}

	/* flush buffered message fragment immediately to console */
	console_cont_flush(text, sizeof(text));//静态变量，下面设置

	for (;;) {//循环读取缓冲区数据块，并打印
		struct printk_log *msg;
		size_t ext_len = 0;
		size_t len;
		int level;

		raw_spin_lock_irqsave(&logbuf_lock, flags);//自旋锁+中断禁止
		if (seen_seq != log_next_seq) {
			wake_klogd = true;
			seen_seq = log_next_seq;
		}

		if (console_seq < log_first_seq) {
			len = sprintf(text, "** %u printk messages dropped ** ",
				      (unsigned)(log_first_seq - console_seq));//text，下次console_unlock调用会打印

			/* messages are gone, move to first one */
			console_seq = log_first_seq;
			console_idx = log_first_idx;
			console_prev = 0;
		} else {
			len = 0;
		}
skip:
		if (console_seq == log_next_seq)
			break;//退出，log_next_seq尾指针，console_seq当前指针

		msg = log_from_idx(console_idx);//从缓冲区获取一条消息
		level = msg->level;//消息等级
		if ((msg->flags & LOG_NOCONS) ||
				suppress_message_printing(level)) {//如果消息等级小于console_loglevel默认等级
			/*
			 * Skip record we have buffered and already printed
			 * directly to the console when we received it, and
			 * record that has level above the console loglevel.
			 */
			console_idx = log_next(console_idx);//获取下一个消息
			console_seq++;
			/*
			 * We will get here again when we register a new
			 * CON_PRINTBUFFER console. Clear the flag so we
			 * will properly dump everything later.
			 */
			msg->flags &= ~LOG_NOCONS;
			console_prev = msg->flags;
			goto skip;//上跳
		}

		len += msg_print_text(msg, console_prev, false,
				      text + len, sizeof(text) - len);
		if (nr_ext_console_drivers) {
			ext_len = msg_print_ext_header(ext_text,
						sizeof(ext_text),
						msg, console_seq, console_prev);
			ext_len += msg_print_ext_body(ext_text + ext_len,
						sizeof(ext_text) - ext_len,
						log_dict(msg), msg->dict_len,
						log_text(msg), msg->text_len);
		}
		console_idx = log_next(console_idx);
		console_seq++;
		console_prev = msg->flags;
		raw_spin_unlock(&logbuf_lock);//解除自旋锁

		stop_critical_timings();	/* don't trace print latency */
		call_console_drivers(level, ext_text, ext_len, text, len);//真正打印消息的函数，此函数调用时当前cpu中断被禁止
		start_critical_timings();
		local_irq_restore(flags);//恢复中断

		if (do_cond_resched)
			cond_resched();
	}
	console_locked = 0;

	/* Release the exclusive_console once it is used */
	if (unlikely(exclusive_console))
		exclusive_console = NULL;

	raw_spin_unlock(&logbuf_lock);

	up_console_sem();

	/*
	 * Someone could have filled up the buffer again, so re-check if there's
	 * something to flush. In case we cannot trylock the console_sem again,
	 * there's a new owner and the console_unlock() from them will do the
	 * flush, no worries.
	 */
	raw_spin_lock(&logbuf_lock);
	retry = console_seq != log_next_seq;
	raw_spin_unlock_irqrestore(&logbuf_lock, flags);

	if (retry && console_trylock())
		goto again;

	if (wake_klogd)
		wake_up_klogd();
}

```

#### call_console_drivers

```cpp

/*
 * Call the console drivers, asking them to write out
 * log_buf[start] to log_buf[end - 1].
 * The console_lock must be held.
 */
static void call_console_drivers(int level,
				 const char *ext_text, size_t ext_len,
				 const char *text, size_t len)
{
	struct console *con;

	trace_console_rcuidle(text, len);

	if (!console_drivers)
		return;

	for_each_console(con) {//遍历console_drivers链表，调用write函数发送消息到串口
		if (exclusive_console && con != exclusive_console)
			continue;
		if (!(con->flags & CON_ENABLED))
			continue;
		if (!con->write)
			continue;
		if (!cpu_online(smp_processor_id()) &&
		    !(con->flags & CON_ANYTIME))
			continue;
		if (con->flags & CON_EXTENDED)
			con->write(con, ext_text, ext_len);
		else
			con->write(con, text, len);
	}
}

```



### /proc/kmesg

我们执行`dmesg`命令可以打印以前的内核信息，所以这些信息必定是保存在内核buffer中。

在`kernel\printk\printk.c`中，定义有一个全局buffer：

![image-20210813155849961](https://gitee.com/zhanghang1999/typora-picture/raw/master/45_log_buf.png)



执行`dmesg`命令时，它就是访问虚拟文件`/proc/kmsg`，把log_buf中的信息打印出来。



### printk最后通过哪些设备打印

#### 命令行参数

在内核的启动信息中，有类似这样的命令行参数：

```shell
/* IMX6ULL */
[root@100ask:~]# cat /proc/cmdline
console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw

/* STM32MP157 */
[root@100ask:~]# cat /proc/cmdline
root=PARTUUID=491f6117-415d-4f53-88c9-6e0de54deac6 rootwait rw console=ttySTM0,115200
```

在命令行参数中，"console=ttymxc0"、"console=ttySTM0"就是用来选择printk设备的。

可以指定多个"console="参数，表示从多个设备打印信息。

#### 设备树

```shell
/ {
	chosen {
                bootargs = "console=ttymxc1,115200";
        };
};
```

#### UBOOT根据环境参数修改设备树：IMX6ULL

```shell
/* 进入IMX6ULL的UBOOT */
=> print mmcargs
mmcargs=setenv bootargs console=${console},${baudrate} root=${mmcroot}
=> print console
console=ttymxc0
=> print baudrate
baudrate=115200
```

## console驱动注册

printk最后会遍历console_drivers链表，调用struct console的write函数发送消息到串口

### struct console

```cpp
struct console {
	char	name[16];  // name为"ttyXXX"，在cmdline中用"console=ttyXXX0"来匹配
    
    // 输出函数
	void	(*write)(struct console *, const char *, unsigned);
    
	int	    (*read)(struct console *, char *, unsigned);
    
    // APP访问/dev/console时通过这个函数来确定是哪个(index)设备
    // 举例:
    // a. cmdline中"console=ttymxc1"
    // b. 则注册对应的console驱动时：console->index = 1
    // c. APP访问/dev/console时调用"console->device"来返回这个index
	struct  tty_driver *(*device)(struct console *co, int *index);
    
	void	(*unblank)(void);
    
    // 设置函数, 可设为NULL
	int	    (*setup)(struct console *, char *);
    
    // 匹配函数, 可设为NULL
	int	    (*match)(struct console *, char *name, int idx, char *options); 
    
	short	flags;
    
    // 哪个设备用作console: 
    // a. 可以设置为-1, 表示由cmdline确定
    // b. 也可以直接指定
	short	index;
    
    // 常用: CON_PRINTBUFFER
	int	    cflag;
	void	*data;
	struct	 console *next;
};
```

### console驱动注册

在`kernel\printk\printk.c`中，可以看到如下代码：

```cpp
__setup("console=", console_setup);
```

这是用来处理u-boot通过设备树传给内核的cmdline参数，比如cmdline中有如下代码：

```c
console=ttymxc0,115200  console=ttyVIRT0
```

对于这两个"console=xxx"就会调用console_setup函数两次，构造得到2个数组项：

```cpp
struct console_cmdline
{
	char	name[16];			/* Name of the driver	    */
	int	index;				/* Minor dev. to use	    */
	char	*options;			/* Options for the driver   */
#ifdef CONFIG_A11Y_BRAILLE_CONSOLE
	char	*brl_options;			/* Options for braille driver */
#endif
};

static struct console_cmdline console_cmdline[MAX_CMDLINECONSOLES];
```

![image-20230609192035246](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609192035246.png)

在cmdline中，最后的"console=xxx"就是"selected_console"(被选中的console，对应/dev/console)：

![image-20230609192104780](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609192104780.png)

#### register_console

console分为两类，它们通过console结构体的flags来分辨(flags中含有CON_BOOT)：

* boot consoles：用来打印很早的信息
* real consoles：真正的console

可以注册很多的bootconsoles，但是一旦注册`real consoles`时，所有的bootconsoles都会被注销，并且以后再注册bootconsoles都不会成功。



被注册的console会放在console_drivers链表中，谁放在链表头部？

* 如果只有一个`real consoles`，它自然是放在链表头部
* 如果有多个`real consoles`，"selected_console"(被选中的console)被放在链表头部

放在链表头有什么好处？APP打开"/dev/console"时，就对应它。



```shell
uart_add_one_port
    uart_configure_port
    	register_console(port->cons);

```

![image-20230609204857243](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230609204857243.png)



#### /dev/cosole

```shell
[root@100ask:~]# ls -la /dev/console
crw------- 1 root root 5, 1 Jan  1 00:00 /dev/console	
```

major = 5, minor = 1

```cpp
tty_open
    tty = tty_open_by_driver(device, inode, filp);
		driver = tty_lookup_driver(device, filp, &index);
			case MKDEV(TTYAUX_MAJOR, 1): {
                struct tty_driver *console_driver = console_device(index);
                
                
/* 从console_drivers链表头开始寻找
 * 如果console->device成功，就返回它对应的tty_driver
 * 这就是/dev/console对应的tty_driver
 */ 
struct tty_driver *console_device(int *index)
{
	struct console *c;
	struct tty_driver *driver = NULL;

	console_lock();
	for_each_console(c) {
		if (!c->device)
			continue;
		driver = c->device(c, index);
		if (driver)
			break;
	}
	console_unlock();
	return driver;
}
```

#### uart_console_device

一般struct console的device函数指定为如下：

```cpp
struct tty_driver *uart_console_device(struct console *co, int *index)
{
	struct uart_driver *p = co->data;
	*index = co->index;
	return p->tty_driver;
}
```



## 编写console驱动

在原来的虚拟串口驱动程序中添加如下部分

```cpp
static struct uart_driver virtual_uart_reg;


struct tty_driver *uart_console_device(struct console *co, int *index)
{
	struct uart_driver *p = co->data;
	*index = co->index;
	return p->tty_driver;
}
static void virt_uart_console_write(struct console *co, const char *s, unsigned int count)
{
	int i;
	for (i = 0; i < count; i++)
		if (txbuf_put(s[i]) != 0)//将printk的数据放入txbuf，buf不会循环覆盖
			return;
}

static struct console virt_uart_console = {
	.name		= "ttyVirtual",
	.write		= virt_uart_console_write,
	.device		= uart_console_device,
	.flags		= CON_PRINTBUFFER,
	.index		= -1,
	.data		= &virtual_uart_reg,
};

static struct uart_driver virtual_uart_reg = {
	.owner          = THIS_MODULE,
	.driver_name    = "ttyVirDriver",
	.dev_name       = "ttyVirtual",
	.major          = 0,
	.minor          = 0,
	.nr             = 1,
	.cons           = &virt_uart_console,//
};

...
    
static int virtual_uart_probe(struct platform_device *pdev)
{
	int rxirq;
	int ret;

	printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	virt_uart_proc = proc_create("virtual_uart", 0, NULL, &virtual_uart_fops);
	rxirq = platform_get_irq(pdev, 0);

	virt_port = devm_kzalloc(&pdev->dev, sizeof(*virt_port), GFP_KERNEL);

	virt_port->dev = &pdev->dev;

	virt_port->type = PORT_IMX,
		virt_port->iotype = UPIO_MEM;
	virt_port->irq = rxirq;
	virt_port->fifosize = 32;
	virt_port->type = PORT_8250;
	virt_port->ops = &virtual_uart_pops;
	//virt_port->rs485_config = imx_rs485_config;
	//virt_port->rs485.flags =
	//	SER_RS485_RTS_ON_SEND | SER_RS485_RX_DURING_TX;
	virt_port->iobase = 1; /* 为了让uart_configure_port能执行 */
	virt_port->flags = 0;

	ret = devm_request_irq(&pdev->dev, rxirq, virt_uart_rxint, 0,
			dev_name(&pdev->dev), virt_port);
	return uart_add_one_port(&virtual_uart_reg, virt_port);
}
```

测试:

uboot下设置bootargs:

```shell
setenv mmcargs setenv bootargs console=${console},${baudrate} root=${mmcroot} console=ttyVirtual0
boot
```

启动内核后：

![image-20230610002437028](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610002437028.png)

## early_printk和early_con

console驱动，它属于uart_driver的一部分，内核的打印信息并不是必须等到uart注册后才能打印。

如果想更早地使用printk函数，比如在安装UART驱动之前就使用printk，这时就需要自己去注册console。

更早地、单独地注册console，有两种方法：

* early_printk：自己实现write函数，不涉及设备树，简单明了
* earlycon：通过设备树传入硬件信息，跟内核中驱动程序匹配

earlycon是新的、推荐的方法，在内核已经有驱动的前提下，通过设备树或cmdline指定寄存器地址即可。

### early_printk

源码为：`arch\arm\kernel\early_printk.c`，要使用它，必须实现这几点：

* 配置内核，选择：CONFIG_EARLY_PRINTK
* 内核中实现：printch函数
* cmdline中添加：earlyprintk



early_printk的函数实现：

```cpp
#ifdef CONFIG_EARLY_PRINTK
struct console *early_console;

asmlinkage __visible void early_printk(const char *fmt, ...)
{
	va_list ap;
	char buf[512];
	int n;

	if (!early_console)
		return;

	va_start(ap, fmt);
	n = vscnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	early_console->write(early_console, buf, n);
}
#endif
```



这里需要注册一个early_console，并且需要提供write函数:

```cpp
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/init.h>

extern void printch(int);

static void early_write(const char *s, unsigned n)
{
	while (n-- > 0) {
		if (*s == '\n')
			printch('\r');
		printch(*s);
		s++;
	}
}

static void early_console_write(struct console *con, const char *s, unsigned n)
{
	early_write(s, n);
}
printascii

static struct console early_console_dev = {
	.name =		"earlycon",
	.write =	early_console_write,
	.flags =	CON_PRINTBUFFER | CON_BOOT,
	.index =	-1,
};

static int __init setup_early_printk(char *buf)
{
	early_console = &early_console_dev;
	register_console(&early_console_dev);
	return 0;
}

early_param("earlyprintk", setup_early_printk);
//early_paramearly_param 用于在早期引导阶段传递参数给内核。它提供了一种机制，允许在内核启动过程的早期阶段（例如，在初始化阶段之前）传递命令行参数或配置选项给内核。
//如在cmdline中添加：console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw earlyprintk就可以激活setup_early_printk函数
```

earlyprintk要求实现printch，这个函数实现在`arch/arm/kernel/debug.S`中，同时这个文件还实现了printascii,printhex2...函数

```assembly
ENTRY(printch)
                addruart_current r3, r1, r2
                mov     r1, r0
                mov     r0, #0
                b       1b
ENDPROC(printch)

#printch调用到addruart_current
.macro  addruart_current, rx, tmp1, tmp2
                addruart        \tmp1, \tmp2, \rx
                mrc             p15, 0, \rx, c1, c0
                tst             \rx, #1
                moveq           \rx, \tmp1
                movne           \rx, \tmp2
                .endm
#最后需要实现addruart函数
```

### early_con

内核开启CONFIG_OF_EARLY_FLATTREE

earlycon就是early console的意思，实现的功能跟earlyprintk是一样的，只是更灵活。

我们知道，对于console，最主要的是里面的write函数：它不使用中断，相对简单。

所以很多串口console的write函数，只要确定寄存器的地址就很容易实现了。

假设芯片的串口驱动程序，已经在内核里实现了，我们需要根据板子的配置给它提供寄存器地址。

怎么提供？

* 设备树
* cmdline参数：console=xxx earlycon

![image-20230610011943809](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610011943809.png)

* 如果cmdline中只有"earlycon"，不带更多参数：对应`early_init_dt_scan_chosen_stdout`函数

  * 使用"/chosen"下的"stdout-path"找到节点

  * 或使用"/chosen"下的"linux,stdout-path"找到节点

  * 节点里有"compatible"和"reg"属性

    * 根据"compatible"找到`OF_EARLYCON_DECLARE`，里面有setup函数，它会提供write函数
    * write函数写什么寄存器？在"reg"属性里确定

* 如果cmdline中"earlycon=xxx"，带有更多参数：对应`setup_earlycon`函数

  * earlycon=xxx格式为：

    ```shell
    <name>,io|mmio|mmio32|mmio32be,<addr>,<options>
    <name>,0x<addr>,<options>
    <name>,<options>
    <name>
    ```

  * 根据"name"找到`OF_EARLYCON_DECLARE`，里面有setup函数，它会提供write函数

  * write函数写什么寄存器？在"addr"参数里确定









![image-20230610014310075](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610014310075.png)

#### 对于cmd只设置earlycon

##### 解析设备树,根据compatible从table找到earlycon_id

![image-20230610014422653](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610014422653.png)

##### 设置寄存器基地址和波特率

![image-20230610014659086](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610014659086.png)

这里的port->uartclk = BASE_BAUD * 16；

#define BASE_BAUD ( 1843200 / 16 )

所以port->uartclk = 115200

##### OF_EARLYCON_DECLARE

setup函数设置的write函数将会操作硬件串口将数据发出，setup函数来自`__earlycon_table`全局变量中的成员，与硬件相关注定是和板子相关的，`__earlycon_table`由宏OF_EARLYCON_DECLARE构造

![image-20230610014818118](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610014818118.png)

对于imx,对应的OF_EARLYCON_DECLARE项为：

```shell
OF_EARLYCON_DECLARE(ec_imx6q, "fsl,imx6q-uart", imx_console_early_setup);
OF_EARLYCON_DECLARE(ec_imx21, "fsl,imx21-uart", imx_console_early_setup);
```

这样基于构造了两个struct earlycon_id项，并放入`__earlycon_table`中，当cmdline中使能earlycon时，设备树首先解析chosen中的stdout-path，得到具体硬件节点，根据compatible匹配`__earlycon_table`，然后解析硬件节点，获取寄存器基地址。调用setup函数设置console.write，注册console。

##### setup

![image-20230610015746081](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610015746081.png)

#### 对于cmd设置earlycon和硬件信息

![image-20230610020545377](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610020545377.png)



![image-20230610020640132](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610020640132.png)



### 测试

对于imx6ull，设备树已经写入stdout-path:

![image-20230610020850786](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610020850786.png)

确定内核开启了CONFIG_OF_EARLY_FLATTREE(make menuconfig)

![image-20230610020958549](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610020958549.png)

暂未测试



## rs485

### RS485线路图

RS485使用A、B两条差分线传输数据：

* 要发送数据时
  * 把SP3485的DE(Driver output Enable)引脚设置为高
  * 通过TxD引脚发送`1`给SP3485，SP3485会驱动A、B引脚电压差为`+2~+6V`
  * 通过TxD引脚发送`0`给SP3485，SP3485会驱动A、B引脚电压差为`-6~-2V`
  * SP3485自动把TxD信号转换为AB差分信号
  * 对于软件来说，通过RS485发送数据时，跟一般的串口没区别，只是多了DE引脚的设置
* 要读取数据时
  * 把SP3485的nRE(Receiver Output Enable)引脚设置为低
  * SP3485会根据AB引脚的电压差驱动RO为1或0
  * RO的数据传入UART的RxD引脚
  * 对于软件来说，通过RS485读取数据时，跟一般的串口没区别，只是多了nRE引脚的设置
* nRE和DE使用同一个引脚时，可以简化成这样：
  * 发送：设置DE为高，发送，设置DE为低
  * 接收：无需特殊设置

![image-20230610030431631](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610030431631.png)

在Linux的串口驱动中，它已经支持RS485，可以使用RTS引脚控制RS485芯片的DE引脚，分两种情况

* 有些UART驱动：使用UART的RTS引脚
* 有些UART驱动：使用GPIO作为RTS引脚，可以通过设备树指定这个GPIO

发送之前，自己设置GPIO控制DE引脚。

![image-20230610030619236](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610030619236.png)

imx在发送数据时->start_tx:

![image-20230610030804524](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610030804524.png)

在使用RS485发送数据前，把RTS设置为高电平就可以。

通过`serial_rs485`结构体控制RTS，示例代码如下：

```cpp
#include <linux/serial.h>

/* 用到这2个ioctl: TIOCGRS485, TIOCSRS485 */
#include <sys/ioctl.h>

struct serial_rs485 rs485conf;

/* 打开串口设备 */
int fd = open ("/dev/mydevice", O_RDWR);
if (fd < 0) {
	/* 失败则返回 */
    return -1;
}

/* 读取rs485conf */
if (ioctl (fd, TIOCGRS485, &rs485conf) < 0) {
	/* 处理错误 */
}

/* 使能RS485模式 */
rs485conf.flags |= SER_RS485_ENABLED;

/* 当发送数据时, RTS为1 */
rs485conf.flags |= SER_RS485_RTS_ON_SEND;

/* 或者: 当发送数据时, RTS为0 */
rs485conf.flags &= ~(SER_RS485_RTS_ON_SEND);

/* 当发送完数据后, RTS为1 */
rs485conf.flags |= SER_RS485_RTS_AFTER_SEND;

/* 或者: 当发送完数据后, RTS为0 */
rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);

/* 还可以设置: 
 * 发送数据之前先设置RTS信号, 等待一会再发送数据
 * 等多久? delay_rts_before_send(单位ms)
 */
rs485conf.delay_rts_before_send = ...;

/* 还可以设置: 
 * 发送数据之后, 等待一会再清除RTS信号
 * 等多久? delay_rts_after_send(单位ms)
 */
rs485conf.delay_rts_after_send = ...;

/* 如果想在发送RS485数据的同时也接收数据, 还可以这样设置 */
rs485conf.flags |= SER_RS485_RX_DURING_TX;

if (ioctl (fd, TIOCSRS485, &rs485conf) < 0) {
	/* 处理错误 */
}

/* 使用read()和write()就可以读、写数据了 */

/* 关闭设备 */
if (close (fd) < 0) {
	/* 处理错误 */
}
```



