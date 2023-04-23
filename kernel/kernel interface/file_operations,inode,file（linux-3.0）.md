# file_operations,inode,file（linux-3.0）

## file_operations

该结构是将系统调用 和驱动程序连接起来，这个结构的每一成员都对应着一个系统调用。当用户进程利用系统调用对设备进行读写操作的时候，这些系统调用通过设备节点中的主设备号和次设备号来确定相应的驱动程序，而每一个字符驱动在linux内核中又是由cdev结构体来描述的，其中cdev结构体中含有成员fops结构体，然后就可以读取file_operations结构体中相应的函数指针，接着把控制权交给函数，从而完成linux设备驱动程序的工作。

一般的设备驱动程序通常用到以下5种操作：

```cpp
struct file_operations
{
    ssize_t (*read)(struct file *，char *, size_t, loff_t *);//从设备同步读取数据
    ssize_t (*write)(struct file *,const char *, size_t, loff_t *);
    int (*ioctl) (struct  inode *,  struct file *, unsigned int,  unsigned long);//执行设备IO控制命令
    int (*open) (struct inode *, struct file *);//打开
    int (*release)(struct inode *, struct file *);//关闭
};
```

具体到应用程序中：

fd=open("/dev/hello",O_RDWR)
通过系统调用open（）来打开设备文件，此设备节点对应有一个设备号。打开 /dev/hello时，通过主次设备号找到相应的字符驱动程序。即在cdev链表中找到cdev这个结构体，cdev里面又包含了file_operations结构体，含有对设备的各种操作，打开时即调用里面的.open 函数指针指向的open函数。

## file

表示一个打开的文件（文件的描述符），由内核在open（）时创建，并将（该文件描述符）传递给该文件上进行操作的所有函数，直到最后的close函数，在文件的所有实例都被关闭后，内核会释放这个数据结构。主要供与文件系统对应的设备文件驱动程序使用。

```cpp
struct file
{
    mode_t f_mode;//表示文件是否可读或可写，FMODE_READ或FMODE_WRITE
    dev_ t  f_rdev ;// 用于/dev/tty
    off_t  f_ops;//当前文件位移
    unsigned short f_flags；//文件标志，O_RDONLY,O_NONBLOCK和O_SYNC
    unsigned short f_count；//打开的文件数目
    unsigned short f_reada；
    struct inode *f_inode；//指向inode的结构指针
    struct file_operations *f_op；//文件索引指针
}
```

## inode

在linux中inode结构用于表示文件，file结构表示打开的文件的描述，因为对于单个文件而言可能会有许多个表示打开的文件的描述符，因而就可能会的对应有多个file结构，但是都指向单个inode结构。该结构里面包含了很多信息,但是,驱动开发者只关心里面两个重要的域:

dev_t i_rdev;     
struct cdev *i_cdev;    //含有真正的设备号
struct cdev                //是内核内部表示字符设备的结构.

## 关系

 struct file结构体中包含有struct file_operations结构体，struct file_operations是struct file的一个域；我们在使用系统调用open()打开一个设备节点struct inode时，我们会得到一个文件struct file,同时返回一个文件描述符，该文件描述符是一个整数，我们称之为句柄，通过访问句柄我们能够访问设备文件struct file,描述符是一个有着特殊含义的整数，特定位都有一定的意义或属性。