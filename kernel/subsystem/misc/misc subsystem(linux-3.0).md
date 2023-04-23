# misc subsystem(linux-3.0)

## 杂项设备

杂项设备驱动，是对字符设备的一种封装，是一种特殊的字符型设备驱动，也是在Linux嵌入式设备中使用的比较多的一种驱动。之所以很大一部分驱动使用的是杂项设备驱动，主要有以下几个方面的原因

#### 节省主设备号

使用普通字符设备，不管该驱动的主设备号是静态还是动态分配，都会消耗一个主设备号，这太浪费了。而且如果你的这个驱动最终会提交到内核主线版本上的话，需要申请一个专门的主设备号，这也麻烦。如果使用misc驱动的话就好多了。因为内核中已经为misc驱动分配了一个主设备号。当系统中拥有多个misc设备驱动时，那么它们的主设备号相同，而用子设备号来区分它们。

#### 使用简单

有时候驱动开发人员需要开发一个功能较简单的字符设备驱动，导出接口让用户空间程序方便地控制硬件，只需要使用misc子系统提供的接口即可快速地创建一个misc设备驱动。

当使用普通的字符设备驱动时，如果开发人员需要导出操作接口给用户空间的话，需要自己去注册字符驱动，并创建字符设备class以自动在/dev下生成设备节点，相对麻烦一点。而misc驱动则无需考虑这些，基本上只需要把一些基本信息通过struct miscdevice交给misc_register()去处理即可。



本质上misc驱动也是一个字符设备驱动，可能相对特殊一点而已。在drivers/char/misc.c的misc驱动初始化函数misc_init()中实际上使用了MISC_MAJOR（主设备号为10）并调用register_chrdev()去注册了一个字符设备驱动。同时也创建了一个misc_class，使得最后可自动在/dev下自动生成一个主设备号为10的字符设备。总的来讲，如果使用misc驱动可以满足要求的话，那么这可以为开发人员剩下不少麻烦。



## 代码分析

Linux下杂项驱动源码：linux-3.0/drivers/char/misc.c

```cpp
static int __init misc_init(void)
{
    int err;

#ifdef CONFIG_PROC_FS
    proc_create("misc", 0, NULL, &misc_proc_fops);//在proc文件系统下创建misc文件，记录了所有注册的混杂设备
#endif
    misc_class = class_create(THIS_MODULE, "misc");//在sys/class下创建misc类（目录）
    err = PTR_ERR(misc_class);
    if (IS_ERR(misc_class))
        goto fail_remove;

    err = -EIO;
    if (register_chrdev(MISC_MAJOR,"misc",&misc_fops))// 注册设备，其中设备的主设备号为MISC_MAJOR，为10。设备名为misc，misc_fops是操作函数的集合
        goto fail_printk;
    misc_class->devnode = misc_devnode;
    return 0;

fail_printk:
    printk("unable to get major %d for misc devices\n", MISC_MAJOR);
    class_destroy(misc_class);
fail_remove:
    remove_proc_entry("misc", NULL);
    return err;
}
```

init函数主要就是在proc文件系统和sys/class下创建对应节点，并注册设备（主设备号为10）到内核，设备操作函数集合misc_fops在打开misc设备时会被调用。

```cpp
static inline int register_chrdev(unsigned int major, const char *name,
                  const struct file_operations *fops)
{       
    return __register_chrdev(major, 0, 256, name, fops);
}   

int __register_chrdev(unsigned int major, unsigned int baseminor,
              unsigned int count, const char *name,
              const struct file_operations *fops)
{
    struct char_device_struct *cd;
    struct cdev *cdev;
    int err = -ENOMEM;

    cd = __register_chrdev_region(major（10）, baseminor（0）, count（255）, name（"misc");
    if (IS_ERR(cd))
        return PTR_ERR(cd);

    cdev = cdev_alloc();//分配cdev
    if (!cdev)
        goto out2;

    cdev->owner = fops->owner;//THIS_MODULE
    cdev->ops = fops;//misc_fops
    kobject_set_name(&cdev->kobj, "%s", name);

    err = cdev_add(cdev, MKDEV(cd->major, baseminor), count);//注册cdev到内核
    if (err)
        goto out;

    cd->cdev = cdev;

    return major ? 0 : cd->major;
out:
    kobject_put(&cdev->kobj);
out2:
    kfree(__unregister_chrdev_region(cd->major, baseminor, count));
    return err;
}
```

注册了主设备号为10的misc，其下可有255个子设备

```cpp
static const struct file_operations misc_fops = {
    .owner      = THIS_MODULE,
    .open       = misc_open,
    .llseek     = noop_llseek,
};  


static int misc_open(struct inode * inode, struct file * file)
{
    int minor = iminor(inode);//根据inode找到次设备号
    struct miscdevice *c;
    int err = -ENODEV;
    const struct file_operations *old_fops, *new_fops = NULL;

    mutex_lock(&misc_mtx);

    list_for_each_entry(c, &misc_list, list) {
        if (c->minor == minor) {
            new_fops = fops_get(c->fops);
            break;
        }
    }//遍历misc_list链表，找到次设备号为minor的设备，并获取其fops给到new_fops

    if (!new_fops) {
        mutex_unlock(&misc_mtx);
        request_module("char-major-%d-%d", MISC_MAJOR, minor);//如果找不到new_fops，则请求加载此设备号对应的模块到内核
        mutex_lock(&misc_mtx);

        list_for_each_entry(c, &misc_list, list) {//再次遍历链表找到此设备号对应的fops
            if (c->minor == minor) {
                new_fops = fops_get(c->fops);
                break;
            }
        }
        if (!new_fops)
            goto fail;//还找不到就退出
    }

    err = 0;
    old_fops = file->f_op;
    file->f_op = new_fops;
    if (file->f_op->open) {
        file->private_data = c;//open的老套路，将设备信息加入到private_data域，方便其他操作函数去找到相关信息
        err=file->f_op->open(inode,file);//调用函数fops->open域
        if (err) {
            fops_put(file->f_op);
            file->f_op = fops_get(old_fops);
        }
    }
    fops_put(old_fops);
fail:
    mutex_unlock(&misc_mtx);
    return err;
}
```



```cpp
linux-3.0/include/linux/miscdevice.h：
struct miscdevice  {
    int minor;  //次设备号
    const char *name;
    const struct file_operations *fops;
    struct list_head list;//链表成员肯定是必须的
    struct device *parent;
    struct device *this_device;
    const char *nodename;
    mode_t mode;
};
```

混杂设备的注册，Linux提供两个函数分别是注册混杂设备和注销混杂设备：

混杂设备的注销：

```cpp
int misc_deregister(struct miscdevice *misc)；
```

混杂设备的注册：

```cpp
int misc_register(struct miscdevice * misc)//传如要注册的混杂设备，此设备的fops需用户自己去封装
{
    struct miscdevice *c;
    dev_t dev;
    int err = 0;

    INIT_LIST_HEAD(&misc->list);//初始化链表成员

    mutex_lock(&misc_mtx);
    list_for_each_entry(c, &misc_list, list) {
        if (c->minor == misc->minor) {
            mutex_unlock(&misc_mtx);
            return -EBUSY;
        }
    }//遍历全局链表misc_list，看是否此设备号已经被其他设备所使用，如果被使用了，返回

    if (misc->minor == MISC_DYNAMIC_MINOR) {//#define MISC_DYNAMIC_MINOR  255
#define DYNAMIC_MINORS 64

        int i = find_first_zero_bit(misc_minors, DYNAMIC_MINORS);//这里变量misc_minors应该是一个64位变量，每注册一个混杂设备，就根据其索取的子设备号将此变量的对应位设备为1，这里找到misc_minors第一个为0位，并将其返回给i
        if (i >= DYNAMIC_MINORS) {//如果i大于或等于64，就返回
            mutex_unlock(&misc_mtx);
            return -EBUSY;
        }
        misc->minor = DYNAMIC_MINORS - i - 1;//获取次设备号
        set_bit(i, misc_minors);//设置i位为1，这样其他混杂设备注册就不能使用此设备号了
    }

    dev = MKDEV(MISC_MAJOR, misc->minor);//根据主次设备号，得出设备号

    misc->this_device = device_create(misc_class, misc->parent, dev,
                      misc, "%s", misc->name);// 在/dev下创建设备节点，这就是有些驱动程序没有显式调用device_create，却出现了设备节点的原因
    if (IS_ERR(misc->this_device)) {
        int i = DYNAMIC_MINORS - misc->minor - 1;
        if (i < DYNAMIC_MINORS && i >= 0)
            clear_bit(i, misc_minors);
        err = PTR_ERR(misc->this_device);
        goto out;
    }

    /*
     * Add it to the front, so that later devices can "override"
     * earlier defaults
     */
    list_add(&misc->list, &misc_list);//添加设备到misc_list全局链表
out:
    mutex_unlock(&misc_mtx);
    return err;
}
```

接下来，用户注册混杂设备，就只需要实现fops结构体的操作函数，然后直接注册混杂设备就可以了

## 示例

以一个s3c2440 外接蜂鸣器为例：

![image-20230424013349718](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013349718.png)

管脚GPB(0)输入高电平时，蜂鸣器导通。这是驱动实现很简单：

```cpp
/*********************************************************************************
*      Copyright:  (C) 2019 none
*                  All rights reserved.
*
*       Filename:  buzzer_driver.c
*    Description:  buzzer_driver
*                 
*        Version:  1.0.0(2019年07月21日)
*         Author:  zhanghang <1711318490@qq.com>
*      ChangeLog:  1, Release initial version on "2019年07月21日 16时39分07秒"
*                 
********************************************************************************/


#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/miscdevice.h>
#include<linux/gpio.h>
#include<mach/regs-gpio.h>
#define DEV_NAME "s3c2440_buzzer"
#ifndef ON
#define ON 1
#endif
#ifndef OFF
#define OFF 0
#endif


static int buzzer_open(struct inode * node,struct file *file)
{
    s3c2410_gpio_cfgpin(S3C2410_GPB(0), S3C2410_GPIO_OUTPUT);//set GPB(0) to output
    s3c2410_gpio_setpin(S3C2410_GPB(0),OFF);
    return 0;
}


static long buzzer_ioctl(struct file *file, unsigned int cmd, unsigned long  arg)
{
    switch(cmd)
    {  
        case ON:
            s3c2410_gpio_setpin(S3C2410_GPB(0),ON);
            break;
        default:
            s3c2410_gpio_setpin(S3C2410_GPB(0),OFF);
            break;
    }       
    return 0;
}


static int buzzer_release(struct inode *inode, struct file *file)
{
    return 0;
}


static struct file_operations buzzer_fops=
{
    .owner=THIS_MODULE,
    .unlocked_ioctl=buzzer_ioctl,
    .open=buzzer_open,
    .release=buzzer_release,
};


static struct miscdevice misc={
    .minor=MISC_DYNAMIC_MINOR,
    .name=DEV_NAME,
    .fops=&buzzer_fops,
};


static int __init buzzer_init(void)
{
    int res;
    res=misc_register(&misc);
    if(res)
    {
        printk("register misc device failure\n");
        return -1;
    }
    printk("buzzer init!\n");
    return 0;
}


static void __exit buzzer_exit(void)
{
    int res;
    res=misc_deregister(&misc);
    if(res)
    {
        printk("deregister misc device failure\n");
        return ;
    }
    printk("buzzer exit!\n");
}

module_init(buzzer_init);
module_exit(buzzer_exit);
MODULE_AUTHOR("zhanghang<1711318490@qq.com>");
MODULE_DESCRIPTION("FL2440 buzzer misc driver");
MODULE_LICENSE("GPL");
```

Makefile

```Makefile
LINUX_SRC=${shell pwd}/../../linux/linux-3.0/
CROSS_COMPILE=armgcc
INST_PATH=/tftp


PWD:=${shell pwd}
EXTRA_CFLAGS +=-DMODULE


#obj-m +=test.o
obj-m +=buzzer_driver.o
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

app

```cpp
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>


#define ON 1
#define OFF 0
int main(int argc, char *argv[])
{
    int flag = 0;
    int fd = open("/dev/s3c2440_buzzer", O_RDWR);
    if(fd < 0)
        printf("open error:%d\n", errno);
    ioctl(fd,ON,0);
    sleep(2);
    ioctl(fd,OFF,0);
    return 0;
}
```

现象：蜂鸣器响两秒后停止。