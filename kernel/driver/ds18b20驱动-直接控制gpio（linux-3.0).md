# ds18b20驱动-直接控制gpio（linux-3.0)

## 时序

**1.初始化时序--复位和存在脉冲**

​     DS18B20的所有通信都由由复位脉冲组成的初始化序列开始。该初始化序列由主机发出，后跟由DS18B20发出的存在脉冲（presence pulse）。当发出应答复位脉冲的存在脉冲后，DS18B20通知主机它在总线上并且准备好操作了。

​     在初始化步骤中，总线上的主机通过拉低单总线至少480μs来产生复位脉冲。然后总线主机释放总线并进入接收模式。当总线释放后，5kΩ的上拉电阻把单总线上的电平拉回高电平。当DS18B20检测到上升沿后等待15到60us，然后以拉低总线60-240us的方式发出存在脉冲。

​     **主机将总线拉低最短480us，之后释放总线。由于5kΩ上拉电阻的作用，总线恢复到高电平。DS18B20检测到上升沿后等待15到60us，发出存在脉冲：拉低总线60-240us。至此，初始化和存在时序完毕。**





## 控制方法

### 读写时隙

主机在写时隙向DS18B20写入数据，并在读时隙从DS18B20读入数据。在单总线上每个时隙只传送一位数据。

**写时间隙：**

​     有两种写时隙：写“0”时间隙和写“1”时间隙。总线主机使用写“1”时间隙向DS18B20写入逻辑1，使用写“0”时间隙向DS18B20写入逻辑0.所有的写时隙必须有最少60us的持续时间，相邻两个写时隙必须要有最少1us的恢复时间。两种写时隙都通过主机拉低总线产生。

​     **为产生写****1时****隙，在拉低总线后主机必须在15μs内释放总线。在总线被释放后，由于5kΩ上拉电阻的作用，总线恢复为高电平。为产生写****0时****隙，在拉低总线后主机必须继续拉低总线以满足时隙持续时间的要求(至少60μs)。**

​     在主机产生写时隙后，DS18B20会在其后的15到60us的一个时间窗口内采样单总线。在采样的时间窗口内，如果总线为高电平，主机会向DS18B20写入1；如果总线为低电平，主机会向DS18B20写入0。

​     所有的写时隙必须至少有60us的持续时间。相邻两个写时隙必须要有最少1us的恢复时间。所有的写时隙（写0和写1）都由拉低总线产生。

​     为产生写1时隙，在拉低总线后主机必须在15us内释放总线（拉低的电平要持续至少1us）。由于上拉电阻的作用，总线电平恢复为高电平，直到完成写时隙。

​     为产生写0时隙，在拉低总线后主机持续拉低总线即可，直到写时隙完成后释放总线（持续时间60-120us）。

​    写时隙产生后，DS18B20会在产生后的15到60us的时间内采样总线，以此来确定写0还是写1。

**读时间隙：**

​     DS18B20只有在主机发出读时隙后才会向主机发送数据。因此，**在发出读暂存器命令 [BEh]或读电源命令[B4h]后，主机必须立即产生读时隙以便DS18B20提供所需数据。另外，主机可在发出温度转换命令T [44h]或Recall命令E 2[B8h]后产生读时隙。**

​     **所有的读时隙必须至少有60us的持续时间。相邻两个读时隙必须要有最少1us的恢复时间。所有的读时隙都由拉低总线，持续至少1us后再释放总线（由于上拉电阻的作用，总线恢复为高电平）产生**。在主机产生读时隙后，DS18B20开始发送0或1到总线上。DS18B20让总线保持高电平的方式发送1，以拉低总线的方式表示发送0.

​     当发送0的时候，DS18B20在读时隙的末期将会释放总线，总线将会被上拉电阻拉回高电平（也是总线空闲的状态）。DS18B20输出的数据在下降沿（下降沿产生读时隙）产生后15us后有效。因此，主机释放总线和采样总线等动作要在15μs内完成。

​     DS18B20只有在主机发出读时隙时才能发送数据到主机。因此，主机必须在BE命令，B4命令后立即产生读时隙以使DS18B20提供相应的数据。另外，在44命令，B8命令后也可以产生读时隙。

​     所有的读时隙必须至少有60us的持续时间。相邻两个读时隙必须要有最少1us的恢复时间。所有的读时隙都由拉低总线，持续至少1us后再释放总线（由于上拉电阻的作用，总线恢复为高电平）产生。DS18B20输出的数据在下降沿产生后15us后有效。因此，释放总线和主机采样总线等动作要在15us内完成。

以上信息来自： https://www.cnblogs.com/wangyuezhuiyi/archive/2012/10/12/2721839.html

ROM指令表

![image-20230423235430846](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423235430846.png)

RAM指令表

![image-20230423235440608](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423235440608.png)

## 初始化

（1） 先将数据线置高电平“1”。

（2） 延时（该时间要求的不是很严格，但是尽可能的短一点）

（3） 数据线拉到低电平“0”。

（4） 延时750微秒（该时间的时间范围可以从480到960微秒）。

（5） 数据线拉到高电平“1”。

（6） 延时等待（如果初始化成功则在15到60微秒时间之内产生一个由DS18B20所返回的低电平“0”。据该状态可以来确定它的存在，但是应注意不能无限的进行等待，不然会使程序进入死循环，所以要进行超时控制）。

（7） 若CPU读到了数据线上的低电平“0”后，还要做延时，其延时的时间从发出的高电平算起（第（5）步的时间算起）最少要480微秒。

（8） 将数据线再次拉高到高电平“1”后结束

## 读

（1）将数据线拉高“1”。

（2）延时2微秒。

（3）将数据线拉低“0”。

（4）延时3微秒。

（5）将数据线拉高“1”。

（6）延时5微秒。

（7）读数据线的状态得到1个状态位，并进行数据处理。

（8）延时60微秒。

## 写

（1） 数据线先置低电平“0”。

（2） 延时确定的时间为15微秒。

（3） 按从低位到高位的顺序发送字节（一次只发送一位）。

（4） 延时时间为45微秒。

（5） 将数据线拉到高电平。

（6） 重复上（1）到（6）的操作直到所有的字节全部发送完为止。

（7） 最后将数据线拉高。

根据DS18B20的通讯协议，主机（单片机）控制DS18B20完成温度转换必须经过三个步骤：每一次读写之前都要对DS18B20进行 复位操作，复位成功后发送一条ROM指令，最后发送RAM指令，这样才能对DS18B20进行预定的操作。复位要求主CPU将数据线下拉500微秒，然后 释放，当DS18B20收到信号后等待16～60微秒左右，后发出60～240微秒的存在低脉冲，主CPU收到此信号表示复位成功。
以上信息来自： https://baike.baidu.com/item/DS18B20?fr=aladdin



## ds18b20 datasheet

原理图

![image-20230423235453018](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423235453018.png)

寄存器

![image-20230423235504618](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423235504618.png)

![image-20230423235527929](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423235527929.png)

**高速暂存存储器**高速暂存存储器由9个字节组成，其分配如表5所示。当温度转换命令发布后，经转换所得的温度值以二字节补码形式存放在 高速暂存存储器的第0和第1个字节。单片机可通过单线接口读到该数据，读取时低位在前，高位在后，数据格式如表1所示。对应的温度计算： 当符号位S=0时，直接将二进制位转换为十进制；当S=1时，先将补码变为原码，再计算十进制值。表 2是对应的一部分温度值。第九个字节是 冗余检验字节。



## driver

```cpp
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <mach/hardware.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#define DRV_AUTHOR                          "zh"
#define DRV_DESC                            "S3C2440 ds18b20 driver"

#define DEV_NAME                            "ds18b20"

#define LOW                                 0
#define HIGH                                1

#ifndef DEV_MAJOR
#define DEV_MAJOR                           0
#endif

int dev_major = DEV_MAJOR;
int dev_minor = 0;

typedef unsigned char BYTE;
static BYTE data[2];

struct ds18b20_device
{
    struct class *class;
    struct cdev  cdev;
};

static struct ds18b20_device dev;

BYTE ds18b20_reset(void)
{
    s3c2410_gpio_cfgpin(S3C2410_GPG(0), S3C2410_GPIO_OUTPUT);/*配置GPG(0)管脚为输出模式，CPU到DS18B20*/
    s3c2410_gpio_setpin(S3C2410_GPG(0), LOW);/*配置GPG(0)管脚低电平，持续480μs*/
    udelay(480);
    s3c2410_gpio_setpin(S3C2410_GPG(0), HIGH);/*配置GPG(0)管脚高电平，持续60μs*/
    udelay(60);

    s3c2410_gpio_cfgpin(S3C2410_GPG(0), S3C2410_GPIO_INPUT);/*配置GPG(0)为输入模式，DS18B20到CPU*/
    if(s3c2410_gpio_getpin(S3C2410_GPG(0)))/*读GPG(0)管脚的状态，此时必须为低电平才算初始化成功*/
    {
        printk("ds18b20 reset failed.\r\n");
        return 1;
    }
    udelay(240);/*延时240μs*/
    return 0;
}

BYTE ds18b20_read_byte(void)
{
    BYTE i = 0;
    BYTE byte = 0;
/* 读“1”时隙：   
    若总线状态保持在低电平状态1微秒到15微秒之间   
    然后跳变到高电平状态且保持在15微秒到60微秒之间   
    就认为从DS18B20读到一个“1”信号   
    理想情况: 1微秒的低电平然后跳变再保持60微秒的高电平   
   读“0”时隙：   
    若总线状态保持在低电平状态15微秒到30微秒之间   
    然后跳变到高电平状态且保持在15微秒到60微秒之间   
    就认为从DS18B20读到一个“0”信号   
    理想情况: 15微秒的低电平然后跳变再保持46微秒的高电平  */

    for(i = 0; i < 8; i++)
    {
        s3c2410_gpio_cfgpin(S3C2410_GPG(0), S3C2410_GPIO_OUTPUT);/*配置GPG(0)管脚为输出模式，CPU到DS18B20*/
        s3c2410_gpio_setpin(S3C2410_GPG(0), LOW);/*设置GPG(0)管脚为低电平状态*/
        udelay(1);/*延时1μs*/
        byte >>= 1;/*byte右移一位*/
        s3c2410_gpio_setpin(S3C2410_GPG(0), HIGH);/*设置GPG(0)管脚为高电平状态*/
        s3c2410_gpio_cfgpin(S3C2410_GPG(0), S3C2410_GPIO_INPUT);/*设置GPG(0)管脚为输入模式,DS18B20到CPU*/
        if(s3c2410_gpio_getpin(S3C2410_GPG(0)))/*读GPG(0)管脚，如果为高电平*/
            byte |= 0x80;/*将最高为置为1*/
        udelay(60);/*延时60μs*/
    }
/*这个循环创造8个读时隙，每次读取1个位到byte，最后byte的最低位为第一次读取数据，最高位为最后其次读取的位*/
    return byte;
}

BYTE ds18b20_write_byte(BYTE byte)
{
/*   写“1”时隙：   
         保持总线在低电平1微秒到15微秒之间   
         然后再保持总线在高电平15微秒到60微秒之间   
         理想状态: 15微秒的低电平然后跳变再保持45微秒的高电平     
     写“0”时隙：   
         保持总线在低电平15微秒到60微秒之间   
         然后再保持总线在高电平1微秒到15微秒之间   
         理想状态: 60微秒的低电平然后跳变再保持1微秒的高电平   */
    BYTE i;

    s3c2410_gpio_cfgpin(S3C2410_GPG(0), S3C2410_GPIO_OUTPUT);/*配置GPG(0)管脚为输出模式，CPU到DS18B20*/
    for(i = 0; i < 8; i++)
    {
        s3c2410_gpio_setpin(S3C2410_GPG(0), LOW);/*设置GPG(0)管脚为低电平*/
        udelay(15);/*延时15μs*/
        if(byte & HIGH)/*判断byte的最低位是否为高电平*/
        {
            /*如果byte的最低位为1，根据写1时隙规则，应将GPG(0)管脚设置为高电平*/
            s3c2410_gpio_setpin(S3C2410_GPG(0), HIGH);/*设置GPG(0)为高电平*/
        }
        else
        {
            /*如果byte的最低位为0，根据写0时隙规则，应将GPG(0)管脚设置为低电平*/
            s3c2410_gpio_setpin(S3C2410_GPG(0), LOW);/*设置GPG(0)为低电平*/
        }
        udelay(45);
        /*延时45μs后，将管脚设置为高电平，释放总线*/

        s3c2410_gpio_setpin(S3C2410_GPG(0), HIGH);
        udelay(1);
        byte >>= 1;
    }
    s3c2410_gpio_setpin(S3C2410_GPG(0), HIGH);/*最后释放总线*/
    return 0;
}

void ds18b20_proc(void)
{
        while(ds18b20_reset());// 每次读写均要先复位
        udelay(120);
        ds18b20_write_byte(0xcc);//发跳过ROM命令
        ds18b20_write_byte(0x44);//发读开始转换命令
        udelay(5);

        while(ds18b20_reset());
        udelay(200);
        ds18b20_write_byte(0xcc);//发跳过ROM命令
        ds18b20_write_byte(0xbe);//读寄存器，共九字节，前两字节为转换值
          
        data[0] = ds18b20_read_byte();//存低字节
        data[1] = ds18b20_read_byte();//存高字节
}

static ssize_t s3c2440_ds18b20_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
        int err;
        ds18b20_proc();

        buf[0] = data[0];
        buf[1] = data[1];

        err = copy_to_user(buff, data, sizeof(data));/*发送数据到用户空间*/

        return err? -EFAULT:len;

        return 1;
}

static struct file_operations ds18b20_fops =
{
    .owner = THIS_MODULE,
    .read = s3c2440_ds18b20_read,
};


static int __init s3c_ds18b20_init(void)
{
    int                     result;
    dev_t                   devno;

    if(0 != ds18b20_reset())
    {
        printk(KERN_ERR "s3c2440 ds18b20 hardware initialize failure.\n");
        return -ENODEV;
    }

    if(0 != dev_major)
    {
        devno = MKDEV(dev_major, 0);
        result = register_chrdev_region(devno, 1, DEV_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&devno, dev_minor, 1, DEV_NAME);
        dev_major = MAJOR(devno);
    }

    if(result < 0)
    {
        printk(KERN_ERR "s3c %s driver cannot use major %d.\n", DEV_NAME, dev_major);
        return -ENODEV;
    }
    printk(KERN_DEBUG "s3c %s driver use major %d.\n", DEV_NAME, dev_major);

    if(NULL == (dev.cdev = cdev_alloc()))
    {
        printk(KERN_ERR "s3c %s driver cannot register cdev: result = %d.\n", DEV_NAME, result);
        unregister_chrdev_region(devno, 1);
        return -ENOMEM;
    }

    dev.cdev -> owner = THIS_MODULE;
    cdev_init(dev.cdev, &ds18b20_fops);

    result = cdev_add(dev.cdev, devno, 1);
    if(0 != result)
    {
        printk(KERN_ERR "s3c %s driver cannot register cdev: result = %d.\n", DEV_NAME, result);
        goto ERROR;
    }
    dev.class  = class_create(THIS_MODULE, DEV_NAME);
    device_create(dev.class, NULL, MKDEV(dev_major, dev_minor), NULL, DEV_NAME);
    printk(KERN_ERR "s3c %s driver[major=%d] installed successfully.\n",DEV_NAME, dev_major);
    
    
    return 0;


ERROR:
    printk(KERN_ERR "s3c %s driver installed failure./", DEV_NAME);
    cdev_del(dev.cdev);
    unregister_chrdev_region(devno, 1);
    return result;
}

static void __exit s3c_ds18b20_exit(void)
{
    dev_t devno = MKDEV(dev_major, dev_minor);

    cdev_del(ds18b20_cdev);
    device_destroy(dev.cdev,devno);
    class_destroy(dev.class);
    unregister_chrdev_region(devno, 1);

    printk(KERN_ERR "s3c %s driver removed!\n", DEV_NAME);
    return;
}

module_init(s3c_ds18b20_init);
module_exit(s3c_ds18b20_exit);

MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_LICENSE("GPL");
```

## app

```shell
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

int main (int argc, char **argv)
{
    int fd;
    double result = 0;
    unsigned char buff[2];
    unsigned short temp = 0;
    int flag = 0;

    if ((fd=open("/dev/ds18b20",O_RDWR | O_NDELAY | O_NOCTTY)) < 0)
    {
        perror("open device ds18b20 failed.\r\n");
        exit(1);
    }
    while(1)
    {
        printf("open device ds18b20 success.\r\n");
        read(fd, buff, sizeof(buff));
        temp=((unsigned short)buff[1])<<8;
        temp|=(unsigned short)buff[0];
        result=0.0625*((double)temp);
        printf("temperature is %4f \r\n", result);
        sleep(2);
    }
    close(fd);

    return 0;
}
```

## 测试效果

```shell
open device ds18b20 success.
temperature is 27.437500
open device ds18b20 success.
temperature is 27.437500
open device ds18b20 success.
temperature is 27.437500
open device ds18b20 success.
temperature is 27.500000
open device ds18b20 success.
temperature is 27.500000
```

## 问题

如果报错

```cpp
WARNING: at drivers/gpio/gpiolib.c:101 gpio_ensure_requested+0x50/0xc4()
autorequest GPIO-192
Modules linked in: test
[<c0039078>] (unwind_backtrace+0x0/0xf4) from [<c0047a44>] (warn_slowpath_common+0x48/0x60)
[<c0047a44>] (warn_slowpath_common+0x48/0x60) from [<c0047af0>] (warn_slowpath_fmt+0x30/0x40)
[<c0047af0>] (warn_slowpath_fmt+0x30/0x40) from [<c01c093c>] (gpio_ensure_requested+0x50/0xc4)
[<c01c093c>] (gpio_ensure_requested+0x50/0xc4) from [<c01c0ae0>] (gpio_direction_output+0x84/0xf0)
[<c01c0ae0>] (gpio_direction_output+0x84/0xf0) from [<c02a782c>] (w1_reset_bus+0x3c/0x84)
[<c02a782c>] (w1_reset_bus+0x3c/0x84) from [<c02a5e14>] (w1_search+0x48/0x168)
[<c02a5e14>] (w1_search+0x48/0x168) from [<c02a6924>] (w1_search_process_cb+0x50/0xe0)
[<c02a6924>] (w1_search_process_cb+0x50/0xe0) from [<c02a6a9c>] (w1_process+0xe8/0xfc)
[<c02a6a9c>] (w1_process+0xe8/0xfc) from [<c00605b4>] (kthread+0x88/0x90)
[<c00605b4>] (kthread+0x88/0x90) from [<c0034b78>] (kernel_thread_exit+0x0/0x8)
---[ end trace 7789c077a1fe3761 ]---
```

这个原因是因为我们之前使用Linux内核自带的单总线子系统来使能过ds18b20，此使能是直接写到了Linux的内核的，也就是内核正在使用此管脚GPG(0)来记录温度到sys/devices/w1 bus master/w1_slave里面。如果我们继续使用此正在使用到管脚可能会警告，报错，甚至系统崩溃。所幸在这里，内核只是给出了警告，并没有很严重。可以取消掉 <> Dallas's 1-wire support  --->内核的相关配置，来消除警告。

## 其他

在这里，有必要关注一下函数：

**s3c2410_gpio_cfgpin(S3C2410_GPG(0), S3C2410_GPIO_OUTPUT)**

**s3c2410_gpio_setpin(S3C2410_GPG(0), LOW)**

**s3c2410_gpio_cfgpin(S3C2410_GPG(0), S3C2410_GPIO_INPUT)**

**s3c2410_gpio_getpin(S3C2410_GPG(0), LOW)**

管脚被设置为输出，也就是cpu到ds1820时，可以发送setpin命令使CPU提供高低电平给ds18b20的管脚，而被设置为输入，cpu可以获取管脚状态。

**void s3c2410_gpio_pullup(unsigned int pin,unsigned int to)**

**设置相应的的GPIO的上拉电阻**。第一个参数：**相应的引脚**，和1里面的用法一致。第二个参数：设置为1或者0，**1表示上拉，0表示不上拉。**





ref:

https://www.cnblogs.com/xiaohexiansheng/p/5778473.html

https://blog.csdn.net/u010944778/article/details/48058433