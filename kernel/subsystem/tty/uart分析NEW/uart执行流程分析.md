# uart执行流程分析

## 行规程

以下文字引用自参考资料**解密TTY**：

大多数用户都会在输入时犯错，所以退格键会很有用。这当然可以由应用程序本身来实现，但是根据UNIX设计“哲学”，应用程序应尽可能保持简单。为了方便起见，操作系统提供了一个编辑缓冲区和一些基本的编辑命令（退格，清除单个单词，清除行，重新打印），这些命令在行规范（line discipline）内默认启用。高级应用程序可以通过将行规范设置为原始模式（raw mode）而不是默认的成熟或准则模式（cooked and canonical）来禁用这些功能。大多数交互程序（编辑器，邮件客户端，shell，及所有依赖curses或readline的程序）均以原始模式运行，并自行处理所有的行编辑命令。行规范还包含字符回显和回车换行（译者注：\r\n 和 \n）间自动转换的选项。如果你喜欢，可以把它看作是一个原始的内核级sed(1)。

另外，内核提供了几种不同的行规范。一次只能将其中一个连接到给定的串行设备。行规范的默认规则称为N_TTY（drivers/char/n_tty.c，如果你想继续探索的话）。其他的规则被用于其他目的，例如管理数据包交换（ppp，IrDA，串行鼠标），但这不在本文的讨论范围之内。

## 驱动框架

![image-20230608233004862](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608233004862.png)

## 网友分析

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



### tty_write

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



## stap脚本追踪



使用stap脚本追踪/drivers/tty目录下所有函数的调用，脚本如下：

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



