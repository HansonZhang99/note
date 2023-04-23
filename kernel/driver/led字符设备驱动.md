# led字符设备驱动

## Linux内核将设备按照访问特性分为三类：字符设备、块设备、网络设备

![Image](https://gitee.com/zhanghang1999/typora-picture/raw/master/Image.png)

### 字符设备

​    一个字符设备是一种可以当作一个字节流来存取的设备( 如同一个文件 )，对于这些设备他一次I/O只访问一个字节。 一个字符驱动负责实现这种行为，这样的驱动常常至少实现 `open,close, read, 和 write` 系统调用. 文本控制台( `/dev/console` )和串口(`/dev/ttyS0` )是字符设备的例子, 因为它们很好地展现了流的抽象. 字符设备通过文件系统设备结点来存取, 例如`/dev/tty1` 和 `/dev/lp0` 在一个字符设备和一个普通文件之间唯一有关的不同就是, 你经常可以在普通文件中移来移去, 但是大部分字符设备仅仅是数据通道, 你只能顺序存取.然而, 存在看起来象数据区的字符设备, 你可以在里面移来移去. 例如LCD驱动`framebuffer`经常这样, 应用程序可以使用 `mmap` 或者 `lseek` 存取整个要求的图像。Linux内核里，绝大部分的设备都是字符设备，今后我们所写的驱动99%也都是处理字符设备。

![image-20230423235755857](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423235755857.png)

字符设备是指在I/O传输过程中以字符为单位进行传输的设备，例如键盘，打印机等。在UNIX系统中，字符设备以特别文件方式在文件目录树中占据位置并拥有相应的结点。

字符设备可以使用与普通文件相同的文件操作命令对字符设备文件进行操作，例如打开、关闭、读、写等。

在UNIX系统中，字符设备以特别文件方式在文件目录树中占据位置并拥有相应的结点。结点中的文件类型指明该文件是字符设备文件。可以使用与普通文件相同的文件操作命令对字符设备文件进行操作，例如打开、关闭、读、写等。

当一台字符型设备在硬件上与主机相连之后，必须为这台设备创建字符特别文件。操作系统的`mknod`命令被用来建立设备特别文件。例如为一台终端创建名为`/dev/tty03`的命令如下(设主设备号为2，次设备为13，字符型类型标记c)：
`mknod /dev/tty03 c 2 13`
此后，`open, close, read, write`等系统调用适用于设备文件`/dev/tty03`。
设备与驱动程序的通信方式依赖于硬件接口。当设备上的数据传输完成时，硬件通过总线发出中断信号导致系统执行一个中断处理程序。中断处理程序与设备驱动程序协同工作完成数据传输的底层控制。                                                                                                                                                                                                                                                                                                                            

### 块设备
​    我们常见的块设备有磁盘、固态硬盘、光盘、U盘、SD卡、Nandflash、Norflash等。一个块设备一次I/O通常访问一个块的大小数据，这个大小通常是2的幂次方字节( 如磁盘512字节，Nandflash使用2048个字节)。对于块设备的使用，需要分区格式化后建立文件系统，之后在应用程序空间中使用mount命令挂载起来之后使用。

![image-20230424000108107](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424000108107.png)

### 网络设备
​    网络设备包括有线网卡(eth0)、无线网卡(wlan0)、回环设备(lo0)、拨号网络设备(ppp0)等，他们在Linux内核里由网络协议栈实现，在/dev路径下并没有相应的设备节点，通过`ifconfig -a`命令可以查看所有的网络设备。在应用程序编程时，所有对网络设备的访问都是通过socket()来访问的。

![image-20230424000131684](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424000131684.png)

## 字符设备驱动

​    字符设备通过文件系统中的设备名来存取，惯例上它们位于 `/dev` 目录. 字符驱动的特殊文件由使用 `ls -l` 的输出的第一列的"c"标识. 块设备也出现在 `/dev` 中, 但是它们由"b"标识。如果你发出 `ls -l` 命令, 你会看到在设备文件项中有 2 个数(由一个逗号分隔)在最后修改日期前面, 这里通常是文件长度出现的地方. 这些数字是给特殊设备的主次设备编号。其中逗号前面的为主设备号，逗号后面的为次设备号。其中主设备号表示一类设备，而次设备号表示它是这类设备的第几个设备。现代 Linux 内核允许多个驱动共享主编号, 但是你看到的大部分设备仍然按照一个主编号一个驱动的原则来组织。
​    例如下图中的`fb0`设备即为字符设备，`ls -l `的第一个字符'c'说明他是字符(character)设备，他的主设备号为29，次设备号为0。其中`fb0`设备就是我们的LCD显示屏设备，从驱动层面上来讲，如果我们想让LCD显示某些内容，我们只需要open()打开该设备获取文件描述符(fd, filedescription)，然后往里面按照相应格式write()相应数据即可，而如果我们想截屏，只需要read()读该设备的内容就行。当然，对LCD的操作并没有表面上的这么简单。

![image-20230424000139716](C:\Users\86135\AppData\Roaming\Typora\typora-user-images\image-20230424000139716.png)

### 设备号

在内核编程中, 使用`dev_t` 类型来定义设备编号， 对于 2.6.0 内核, `dev_t` 是 32 位的量, 其中12位用作主编号, 20 位用作次编号. 在编码时，我们不应该管哪些位是主设备号，哪些位是次设备号。而是应当利用在 中的一套宏定义来获取一个 `dev_t` 的主、次编号:
`MAJOR(dev_t dev);`     获取主设备号
`MINOR(dev_t dev);`     获取次设备号
相反, 如果我们有主、次编号需要将其转换为一个 `dev_t`, 则使用`MKDEV`宏:
`MKDEV(int major, int minor);`

### 获取主次设备号

在建立一个字符驱动时你的驱动需要做的第一件事是获取一个或多个设备编号来使用，在Linux内核里，一些主设备编号是静态分派给最普通的设备的，这些设备列表在内核源码树的`Documentation/devices.txt`中列出. 因此, 作为一个驱动编写者, 我们有两个选择: 一是简单地捡一个看来没有用的主设备号, 二是让内核以动态方式分配一个主设备号给你. 只要你是你的驱动的唯一用户就可以捡一个编号用; 一旦你的驱动更广泛的被使用了, 一个随机捡来的主编号将导致冲突和麻烦。因此, 对于新驱动, 我们强烈建议使用动态分配来获取你的主设备编号, 而不是随机选取一个当前空闲的编号。

#### 静态设定主次设备号

静态设备指定主次设备号是指我们根据当前系统的主设备号分配情况，自己选择一个主设备号。当然我们自己随机选择的话，会跟Linux内核其他的驱动冲突，这时我们可以先查看当前系统已经使用了哪些主设备号，然后我们选择一个没有使用作为我们新的驱动使用。Linux系统中正在使用的主设备号会保存在 /proc/devices 文件中。

上面列出的是当前运行的Linux内核里所有设备驱动使用的主设备号，此时我们在编写驱动时可以选定一个未用的主设备号，如251来使用。

```cpp
dev_t devno;
int result;
int major=251;
devno = MKDEV(major, 0);
result = register_chrdev_region (devno, 4, "led");
if (result < 0)
{
printk(KERN_ERR "LED driver can't use major %d\n", major);
return -ENODEV;}
```

这里register_chrdev_region()函数的原型为：

`int register_chrdev_region(dev_t first, unsigned int count, char *name);`

 first 是你要分配的起始设备编号. first 的次编号部分通常是从0开始，但不是强制的. count 是你请求的连续设备编号的总数. 注意, 如果 count 太大, 你要求的范围可能溢出到下一个次编号;但是只要你要求的编号范围可用, 一切都仍然会正确工作. 最后, name 是应当连接到这个编号范围的设备的名子; 它会出现在 /proc/devices 和 sysfs 中。如同大部分内核函数, 如果分配成功进行, register_chrdev_region 的返回值是 0. 出错的情况下, 返回一个负的错误码, 你不能存取请求的区域。

当驱动的主、次设备号申请成功后，/proc/devices里将会出现该设备，但是/dev 路径下并不会创建该设备文件。   如果我们需要创建该文件，则需要使用mknod命令创建。

#### 动态设定主次设备号

在上面的示例中，我们是首先查看当前的系统使用了哪些设备驱动，然后找一个未用的主设备号来使用。假设将来我们的Linux内核系统升级需要使能其他的设备驱动，如果某个需要的驱动所用的主设备号刚好和我们的设备驱动冲突，那么我们驱动不得不对这个主设备号进行调整，而如果产品已经部署了，这种召回升级是非常致命的。所以我们在写驱动时不应该静态指定一个主设备号，而是由Linux内核根据当前的主设备号使用情况动态分配一个未用的给我们的驱动使用，这样就永远不会冲突。

```cpp
int result;
int dev_major
dev_t devno;
result = alloc_chrdev_region(&devno, 0, 4, "led");
dev_major = MAJOR(devno);
/* Alloc for device major failure */
if (result < 0)
{
printk(KERN_ERR "led driver allocate device major number failure: %d\n",
result);
return -ENODEV;
}
printk(KERN_ERR "led driver choose device major number: %d\n", dev_major);
```

alloc_chrdev_region函数的原型如下，使用这个函数, dev 是一个只输出的参数, 它在函数成功完成时持有你的分配范围的第一个数. fisetminor 应当是请求的第一个要用的次编号; 它常常是0. count 和 name 参数如同给 request_chrdev_region 的一样。

`int alloc_chrdev_region(dev_t *dev, unsigned int firstminor, unsigned int count, char *name);`

如果使用alloc_chrdev_region()函数来动态分配主设备号，那我们驱动在安装后并不知道这个主设备号是多少，那么只能通过 查看 /proc/devices 文件才能知道它的值，然后再创建设备节点。

#### 释放主次设备号

在Linux内核中，主、次设备号是一种资源，不管你使用何种方式分配的设备编号, 你应当在不再使用们时释放它. 设备编号的释放使用下面函数：

`void unregister_chrdev_region(dev_t first, unsigned int count);`

### 字符驱动重要的数据结构
注册设备编号仅仅是驱动代码必须进行的诸多任务中的第一个，在编写字符驱动的过程中，我们将涉及到 3 个重要的内核数据结构, 称为 file_operations, file 和 inode. 我们需要对这些结构有一些基本了解才能够了解整个Linux内核设备驱动的工作机制。

#### file_operation 结构体

file_operation就是把系统调用和驱动程序关联起来的关键数据结构。这个结构的每一个成员都对应着一个系统调用，相应的系统调用将读取file_operation中相应的函数指针，接着把控制权转交给函数，从而完成了Linux设备驱动程序的工作。 在系统内部，I/O设备的存取操作通过特定的入口点来进行，而这组特定的入口点恰恰是由设备驱动程序提供的。通常这组设备驱动程序接口是由结构file_operations结构体向系统说明的，它定义在include/linux/fs.h中。传统上,一个 file_operation 结构或者其一个指针称为 fops( 或者它的一些变体). 结构中的每个成员必须指向驱动中的函数, 这些函数实现一个特别的操作, 或者对于不支持的操作留置为 NULL. 当指定为 NULL 指针时内核的确切的行为是每个函数不同的。

```cpp
struct file_operations {
struct module *owner;
loff_t (*llseek) (struct file *, loff_t, int);
ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
int (*readdir) (struct file *, void *, filldir_t);
unsigned int (*poll) (struct file *, struct poll_table_struct *);
long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
int (*mmap) (struct file *, struct vm_area_struct *);
int (*open) (struct inode *, struct file *);
int (*flush) (struct file *, fl_owner_t id);
int (*release) (struct inode *, struct file *);
int (*fsync) (struct file *, int datasync);
int (*aio_fsync) (struct kiocb *, int datasync);
int (*fasync) (int, struct file *, int);
int (*lock) (struct file *, int, struct file_lock *);
ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long,
unsigned long, unsigned long);
int (*check_flags)(int);
int (*flock) (struct file *, int, struct file_lock *);
ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
int (*setlease)(struct file *, long, struct file_lock **);
long (*fallocate)(struct file *file, int mode, loff_t offset, loff_t len);
};
```

#### inode结构体
Linux中一切皆文件，当我们在Linux中创建一个文件时，就会在相应的文件系统中创建一个inode与之对应，文件实体和文件的inode是一一对应的，创建好一个inode会存在存储器中。第一次open就会将inode在内存中有一个备份，同一个文件被多次打开并不会产生多个inode，当所有被打开的文件都被close之后，inode在内存中的实例才会被释放。既然如此，当我们使用mknod(或其他方法)创建一个设备文件时，也会在文件系统中创建一个inode，这个inode和其他的inode一样，用来存储关于这个文件的静态信息(不变的信息)，包括这个设备文件对应的设备号，文件的路径以及对应的驱动对象等。
    对于不同的文件类型，inode被填充的成员内容也会有所不同，以创建字符设备为例，我们知道add_chrdev_region其实是把一个驱动对象和一个(一组)设备号联系到一起。而创建设备文件，其实是把设备文件和设备号联系到一起。至此，这三者就被绑定在一起了。这样，内核就有能力创建一个struct inode实例了。

inode一样定义在include/linux/fs.h文件中：

```cpp
struct inode {
/* RCU path lookup touches following: */
umode_t i_mode;
uid_t i_uid;
gid_t i_gid;
const struct inode_operations *i_op;
struct super_block *i_sb;
spinlock_t i_lock; /* i_blocks, i_bytes, maybe i_size */
unsigned int i_flags;
unsigned long i_state;
#ifdef CONFIG_SECURITY
void *i_security;
#endif
struct mutex i_mutex;
unsigned long dirtied_when; /* jiffies of first dirtying */
struct hlist_node i_hash;
struct list_head i_wb_list; /* backing dev IO list */
struct list_head i_lru; /* inode LRU list */
struct list_head i_sb_list;
union {
struct list_head i_dentry;
struct rcu_head i_rcu;
};
unsigned long i_ino;
atomic_t i_count;
unsigned int i_nlink;
dev_t i_rdev;
unsigned int i_blkbits;
u64 i_version;
loff_t i_size;
#ifdef __NEED_I_SIZE_ORDERED
seqcount_t i_size_seqcount;
#endif
struct timespec i_atime;
struct timespec i_mtime;
struct timespec i_ctime;
blkcnt_t i_blocks;
unsigned short i_bytes;
struct rw_semaphore i_alloc_sem;
const struct file_operations *i_fop; /* former ->i_op->default_file_ops */
struct file_lock *i_flock;
struct address_space *i_mapping;
struct address_space i_data;
#ifdef CONFIG_QUOTA
struct dquot *i_dquot[MAXQUOTAS];
#endif
struct list_head i_devices;
union {
struct pipe_inode_info *i_pipe;
struct block_device *i_bdev;
struct cdev *i_cdev;
};
__u32 i_generation;
#ifdef CONFIG_FSNOTIFY
__u32 i_fsnotify_mask; /* all events this inode cares about */
struct hlist_head i_fsnotify_marks;
#endif
#ifdef CONFIG_IMA
atomic_t i_readcount; /* struct files open RO */
#endif
atomic_t i_writecount;
#ifdef CONFIG_FS_POSIX_ACL
struct posix_acl *i_acl;
struct posix_acl *i_default_acl;
#endif
void *i_private; /* fs or device private pointer */
};
```

#### file结构体
Linux内核会为每一个进程维护一个文件描述符表，这个表其实就是struct file的索引。open()的过程实就是根据传入的路径填充好一个file结构并将其赋值到数组中并返回其索引。下面是file的主要内容，它一样是定义在include/linux/fs.h文件中：

```cpp
struct file {
/*
* fu_list becomes invalid after file_free is called and queued via
* fu_rcuhead for RCU freeing
*/
union {
struct list_head fu_list;
struct rcu_head fu_rcuhead;
} f_u;
struct path f_path;
#define f_dentry f_path.dentry
#define f_vfsmnt f_path.mnt
const struct file_operations *f_op;
spinlock_t f_lock; /* f_ep_links, f_flags, no IRQ */
#ifdef CONFIG_SMP
int f_sb_list_cpu;
#endif
atomic_long_t f_count;
unsigned int f_flags;
fmode_t f_mode;
loff_t f_pos;
struct fown_struct f_owner;
const struct cred *f_cred;
struct file_ra_state f_ra;
u64 f_version;
#ifdef CONFIG_SECURITY
void *f_security;
#endif
/* needed for tty driver, and maybe others */
void *private_data;
#ifdef CONFIG_EPOLL
/* Used by fs/eventpoll.c to link all the hooks to this file */
struct list_head f_ep_links;
#endif /* #ifdef CONFIG_EPOLL */
struct address_space *f_mapping;
#ifdef CONFIG_DEBUG_WRITECOUNT
unsigned long f_mnt_write_state;
#endif
};
```

### 字符设备注册

#### 分配cdev结构体

内核在内部使用类型 struct cdev 的结构来代表字符设备. 在内核调用你的设备操作前, 你必须分配一个这样的结构体并注册给Linux内核，在这个结构体里有对于这个设备进行操作的函数，具体定义在file_operation结构体中。该结构体

```cpp
struct cdev {
struct kobject kobj;
struct module *owner;
const struct file_operations *ops;
struct list_head list;
dev_t dev;
unsigned int count;
};
```

在Linux内核编程中，我们可以使用两种方法获取该结构体。
一是直接通过变量定义来获取它：

```cpp
struct cdev led_cdev;
```

另外一种方式是通过cdev_alloc()来动态分配他：

```cpp
if(NULL == (led_cdev=cdev_alloc()) )
{
printk(KERN_ERR "S3C %s driver can't alloc for the cdev.\n", DEV_NAME);
unregister_chrdev_region(devno, dev_count);
return -ENOMEM;
}
```

#### 注册到cdev内核

在分配到cdev结构体后，接下来我们将初始化它，并将对该设备驱动所支持的系统调用函数存放的file_operations结构体添加进来，然后我们通过cdev_add函数将他们注册给Linux内核，这样完成整个Linux字符设备的注册过程。其中cdev_add的函数原型如下，其中dev 是 cdev 结构,num 是这个设备响应的第一个设备号, count 是应当关联到设备的设备号的数目.

```cpp
int cdev_add(struct cdev *dev, dev_t num, unsigned int count);
```

下面是字符设备驱动cdev的分配和注册过程：

```cpp
static struct file_operations led_fops =
{
.owner = THIS_MODULE,
.open = led_open,
.release = led_release,.unlocked_ioctl = led_ioctl,
};
struct cdev *led_cdev;
if(NULL == (led_cdev=cdev_alloc()) )
{
printk(KERN_ERR "S3C %s driver can't alloc for the cdev.\n", DEV_NAME);
unregister_chrdev_region(devno, dev_count);
return -ENOMEM;
}
led_cdev->owner = THIS_MODULE;
cdev_init(led_cdev, &led_fops);
result = cdev_add(led_cdev, devno, dev_count);
if (0 != result)
{
printk(KERN_INFO "S3C %s driver can't reigster cdev: result=%d\n", DEV_NAME, result);
goto ERROR;
}
```

## Linux内核驱动和系统调用的关系

![image-20230424000157588](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424000157588.png)

## led driver demo

```cpp
#include <linux/module.h> /* Every Linux kernel must include this head */
#include <linux/init.h> /* Every Linux kernel module must include this head */
#include <linux/kernel.h> /* printk() */
#include <linux/fs.h> /* struct fops */
#include <linux/errno.h> /* error codes */
#include <linux/cdev.h> /* cdev_alloc() */
#include <asm/io.h> /* ioremap() */
#include <linux/ioport.h> /* request_mem_region() */
#include <asm/ioctl.h> /* Linux kernel space head file for macro _IO() to generate ioctl
command */
#include <linux/printk.h> /* Define log level KERN_DEBUG */


#define DEV_NAME "led"
#define LED_NUM 4
/* Set the LED dev major number */
//#define LED_MAJOR 79


#ifndef LED_MAJOR
#define LED_MAJOR 0
#endif




#define DISABLE 0
#define ENABLE 1


#define GPIO_INPUT 0x00
#define GPIO_OUTPUT 0x01


#define PLATDRV_MAGIC 0x60
#define LED_OFF _IO (PLATDRV_MAGIC, 0x18)
#define LED_ON _IO (PLATDRV_MAGIC, 0x19)


#define S3C_GPB_BASE 0x56000010
#define S3C_GPB_LEN 0x10 /* 0x56000010~0x56000020 */
#define GPBCON_OFFSET 0
#define GPBDAT_OFFSET 4
#define GPBUP_OFFSET 8


static void __iomem *gpbbase = NULL;


#define read_reg32(addr) *(volatile unsigned int *)(addr)
#define write_reg32(addr, val) *(volatile unsigned int *)(addr) = (val)


int led[LED_NUM] = {5,6,8,10}; /* Four LEDs use GPB5,GPB6,GPB8,GPB10 */
int dev_count = ARRAY_SIZE(led);
int dev_major = LED_MAJOR;
int dev_minor = 0;
int debug = DISABLE;
static struct cdev *led_cdev;


/*address mapping and memory setting*/
static int s3c_hw_init(void)
{
    int i;
    unsigned int regval;
    if(!request_mem_region(S3C_GPB_BASE, S3C_GPB_LEN, "s3c2440 led"))
    {
        printk(KERN_ERR "request_mem_region failure!\n");
        return -EBUSY;
    }
    if( !(gpbbase=(unsigned int *)ioremap(S3C_GPB_BASE, S3C_GPB_LEN)) )
    {
        release_mem_region(S3C_GPB_BASE, S3C_GPB_LEN);
        printk(KERN_ERR "release_mem_region failure!\n");
        return -ENOMEM;
    }
    for(i=0; i<dev_count; i++)
    {
        /* Set GPBCON register, set correspond GPIO port as input or output mode */
        regval = read_reg32(gpbbase+GPBCON_OFFSET);
        regval &= ~(0x3<<(2*led[i])); /* Clear the currespond LED GPIO configure register */
        regval |= GPIO_OUTPUT<<(2*led[i]); /* Set the currespond LED GPIO as output mode*/
        write_reg32(gpbbase+GPBCON_OFFSET, regval);


        /* Set GPBUP register, set correspond GPIO port pull up resister as enable or disable */
        regval = read_reg32(gpbbase+GPBUP_OFFSET);
        regval |= (0x1<<led[i]); /* Disable pull up resister */
        write_reg32(gpbbase+GPBUP_OFFSET, regval);
        /* Set GPBDAT register, set correspond GPIO port power level as high level or low
        level */


        regval = read_reg32(gpbbase+GPBDAT_OFFSET);
        regval |= (0x1<<led[i]); /* This port set to high level, then turn LED off */
        write_reg32(gpbbase+GPBDAT_OFFSET, regval);
        }
    return 0;
}
/*server fot ioctl*/
static void turn_led(int which, unsigned int cmd)
{
    unsigned int regval;
    regval = read_reg32(gpbbase+GPBDAT_OFFSET);
    if(LED_ON == cmd)
    {
        regval &= ~(0x1<<led[which]); /* Turn LED On */
    }
    else if(LED_OFF == cmd)
    {
        regval |= (0x1<<led[which]); /* Turn LED off */
    }
    write_reg32(gpbbase+GPBDAT_OFFSET, regval);
}
/*close*/
static void s3c_hw_term(void)
{
    int i;
    unsigned int regval;
    for(i=0; i<dev_count; i++)
    {
        regval = read_reg32(gpbbase+GPBDAT_OFFSET);
        regval |= (0x1<<led[i]); /* Turn LED off */
        write_reg32(gpbbase+GPBDAT_OFFSET, regval);
    }
    release_mem_region(S3C_GPB_BASE, S3C_GPB_LEN);
    iounmap(gpbbase);
}


static int led_open(struct inode *inode, struct file *file)
{
    int minor = iminor(inode);
    file->private_data = (void *)minor;
    printk(KERN_DEBUG "/dev/led%d opened.\n", minor);
    return 0;
}


static int led_release(struct inode *inode, struct file *file)
{
    printk(KERN_DEBUG "/dev/led%d closed.\n", iminor(inode));
    return 0;
}


static void print_help(void)
{
    printk("Follow is the ioctl() commands for %s driver:\n", DEV_NAME);
    printk("Turn LED on command : %u\n", LED_ON);
    printk("Turn LED off command : %u\n", LED_OFF);
    return;
}


static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int which = (int)file->private_data;
    switch (cmd)
    {
        case LED_ON:
            turn_led(which, LED_ON);
            break;
        case LED_OFF:
            turn_led(which, LED_OFF);
            break;
        default:
            printk(KERN_ERR "%s driver don't support ioctl command=%d\n", DEV_NAME, cmd);
            print_help();
            break;
    }
    return 0;
}


static struct file_operations led_fops =
{
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .unlocked_ioctl = led_ioctl,
};
/*__init*/
static int __init s3c_led_init(void)
{    
    int result;
    dev_t devno;
    // 字符设备驱动注册流程第一步： 相应的设备硬件初始化
    if( 0 != s3c_hw_init() )
    {
        printk(KERN_ERR "s3c2440 LED hardware initialize failure.\n");
        return -ENODEV;
    }
    // 字符设备驱动注册流程第二步： 分配主次设备号，这里既支持静态指定，也支持动态申请
    /* Alloc the device for driver */
    if (0 != dev_major) /* Static */
    {
        devno = MKDEV(dev_major, 0);
        result = register_chrdev_region (devno, dev_count, DEV_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&devno, dev_minor, dev_count, DEV_NAME);
        dev_major = MAJOR(devno);
    }
    /* Alloc for device major failure */
    if (result < 0)
    {
        printk(KERN_ERR "S3C %s driver can't use major %d\n", DEV_NAME, dev_major);
        return -ENODEV;
    }
    printk(KERN_DEBUG "S3C %s driver use major %d\n", DEV_NAME, dev_major);
    // 字符设备驱动注册流程第三步：分配cdev结构体，我们这里使用动态申请的方式
    if(NULL == (led_cdev=cdev_alloc()) )
    {
        printk(KERN_ERR "S3C %s driver can't alloc for the cdev.\n", DEV_NAME);
        unregister_chrdev_region(devno, dev_count);
        return -ENOMEM;
    }
    // 字符设备驱动注册流程第四步： 绑定主次设备号、fops到cdev结构体中，并注册给Linux内核
    led_cdev->owner = THIS_MODULE;
    cdev_init(led_cdev, &led_fops);
    result = cdev_add(led_cdev, devno, dev_count);
    if (0 != result)
    {
        printk(KERN_INFO "S3C %s driver can't reigster cdev: result=%d\n", DEV_NAME, result);
        goto ERROR;
    }
    printk(KERN_ERR "S3C %s driver[major=%d] version 1.0.0 installed successfully!\n", DEV_NAME, dev_major);
    return 0;
ERROR:
    printk(KERN_ERR "S3C %s driver installed failure.\n", DEV_NAME);
    cdev_del(led_cdev);
    unregister_chrdev_region(devno, dev_count);
    return result;
}
/*__exit*/
static void __exit s3c_led_exit(void)
{
    dev_t devno = MKDEV(dev_major, dev_minor);
    s3c_hw_term();
    cdev_del(led_cdev);
    unregister_chrdev_region(devno, dev_count);
    printk(KERN_ERR "S3C %s driver version 1.0.0 removed!\n", DEV_NAME);
    return ;
}


/* These two functions defined in <linux/init.h> */
module_init(s3c_led_init);
module_exit(s3c_led_exit);
// Linux内核安装驱动时支持模块参数，在这里debug和dev_major可以在insmod时通过参数传
递传进来，如：insmod s3c_led.ko debug=1 dev_major=218
module_param(debug, int, S_IRUGO);
module_param(dev_major, int, S_IRUGO);
MODULE_AUTHOR("zhanghang");
MODULE_DESCRIPTION("character led driver");
MODULE_LICENSE("GPL");
```

## driver makefile

```makefile
LINUX_SRC=${shell pwd}/../../linux/linux-3.0/
CROSS_COMPILE=armgcc
INST_PATH=/tftp

PWD:=${shell pwd}
EXTRA_CFLAGS +=-DMODULE

#obj-m +=test.o
obj-m +=led.o

modules:
    @make -C ${LINUX_SRC} M=${PWD} modules
    @make clear

uninstall:
    rm -f ${INST_PATH}/*.ko

install:uninstall
    cp -af *.ko ${INST_PATH}
clear:
    @rm -f *.o *.cmd *.mod.c
    @rm -rf *~ core .depend .tmp_versions Module.symvers modules.order -f
    @rm -f .*ko.cmd .*.o.cmd .*.o.d
#   @rm -f *.unsigned

clean:
    @rm -f led.ko
```

## app

```cpp
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/select.h>

#define LED_CNT 4
#define DEVNAME_LEN 10

#define MAGIC 0x60
#define LED_OFF _IO (MAGIC, 0x18)
#define LED_ON _IO (MAGIC, 0x19)

static inline msleep(unsigned long ms)
{   
    struct timeval tv;
    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms%1000)*1000;
    select(0, NULL, NULL, NULL, &tv);
}

int main (int argc, char **argv)
{
    int i;
    int fd[LED_CNT];
    char dev_name[DEVNAME_LEN]={0,0,0,0};
    for(i=0; i<LED_CNT; i++)
    {
        snprintf(dev_name, sizeof(dev_name), "/dev/led%d", i);
        fd[i] = open(dev_name, O_RDWR, 0755);
        if(fd[i] < 0)
            goto err;
    }
    while(1)
    {
        for(i=0; i<LED_CNT; i++)
        {
            ioctl(fd[i], LED_ON);
            msleep(300);
            ioctl(fd[i], LED_OFF);
            msleep(300);
            ioctl(fd[i], LED_ON);
            msleep(300);
            ioctl(fd[i], LED_OFF);
            msleep(300);
        }
    }
    for(i=0; i<LED_CNT; i++)
    {
        close(fd[i]);
    }
    return 0;
err:
    for(i=0; i<LED_CNT; i++)
    {
        if(fd[i] >= 0)
        {
            close(fd[i]);
        }
    }
    return -1;
}
```

## app makefile

```makefile
PWD=$(shell pwd)
INSTPATH=/tftp
CROSS_COMPILE=armgcc

export CC=${CROSS_COMPILE}gcc
CFLAGS+=-I`dirname ${PWD}`

SRCFILES = $(wildcard *.c)
BINARIES=$(SRCFILES:%.c=%)

all: binaries install
binaries: ${BINARIES}
    @echo " Compile over"
    
%: %.c
    $(CC) -o $@ $< $(CFLAGS)

install:
    cp $(BINARIES) ${INSTPATH}
clean:
    @rm -f version.h
    @rm -f *.o $(BINARIES)
    @rm -rf *.gdb *.a *.so *.elf*
distclean: clean
    @rm -f tags cscope*
.PHONY: clean
```

## test

```shell
/driver >: ls
led.ko  test
/driver >: insmod led.ko
/driver >: lsmod
led 1698 0 - Live 0xbf000000
/driver >: cat /proc/devices | grep led
252 led
/driver >: mknod -m 755 /dev/led0 c 252 0
/driver >: mknod -m 755 /dev/led1 c 252 1
/driver >: mknod -m 755 /dev/led2 c 252 2
/driver >: mknod -m 755 /dev/led3 c 252 3
/driver >: find ../dev/ -iname "led*" | xargs ls -l
crwxr-xr-x    1 root     root      252,   0 Jan  1 00:01 ../dev/led0
crwxr-xr-x    1 root     root      252,   1 Jan  1 00:01 ../dev/led1
crwxr-xr-x    1 root     root      252,   2 Jan  1 00:02 ../dev/led2
crwxr-xr-x    1 root     root      252,   3 Jan  1 00:02 ../dev/led3
```

运行应用程序

```shell
/driver >: ./test
```

可以观察到开发板LED灯依次循环闪烁两次...