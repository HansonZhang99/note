# platform led driver(linux 3.0)

## platform总线

一个现实的linux设备驱动通常需要挂接在一种总线上，对于本身依附于PCI，USB，IIC，SPI等的设备而言，这自然不是问题，但是在嵌入式系统里面，SOC系统中集成的独立的外设控制器，挂接在SOC内存空间的外设等确不依附于此类总线。基于这一背景，linux发明了一种虚拟的总线，称为platform总线，相应的设备称为platform_device,而驱动成为platform_driver.Platform总线是linux2.6内核加的一种虚拟总线。
Linux platform driver机制和传统的device driver 机制(通过driver_register函数进行注册)相比，一个十分明显的优势在于platform机制将设备本身的资源注册进内核，由内核统一管理，在驱动程序中使用这些资源时通过platform device提供的标准接口进行申请并使用。这样提高了驱动和资源管理的独立性，并且拥有较好的可移植性和安全性(这些标准接口是安全的)。

## 平台设备驱动软件设计流程

定义平台设备（Platform_device)
注册平台设备（Platform_device)
定义平台驱动（Platform_driver)
注册平台驱动（Platform_driver)

## **平台设备**

### platform_device

在linux/platform_device.h文件中定义

```cpp
struct platform_device{
const char      *name;           //设备名字
int             id;             //设备ID
struct device   dev;            //设备的device数据结构
u32             num_resources;  //资源的个数
struct resource *resource;      //设备的资源
const struct platform_device_id  *id_entry;//设备ID入口
struct pdev_archdata archdata;   //体系结构相关的数据
};

struct device {
    struct device       *parent;

    struct device_private   *p;

    struct kobject kobj;
    const char      *init_name; /* initial name of the device */
    const struct device_type *type;

    struct mutex        mutex;  /* mutex to synchronize calls to
                     * its driver.
                     */

    struct bus_type *bus;       /* type of bus device is on */
    struct device_driver *driver;   /* which driver has allocated this
                       device */
    void        *platform_data; /* Platform specific data, device
                       core doesn't touch it */
    struct dev_pm_info  power;
    struct dev_power_domain *pwr_domain;

#ifdef CONFIG_NUMA
    int     numa_node;  /* NUMA node this device is close to */
#endif
    u64     *dma_mask;  /* dma mask (if dma'able device) */
    u64     coherent_dma_mask;/* Like dma_mask, but for
                         alloc_coherent mappings as
                         not all hardware supports
                         64 bit addresses for consistent
                         allocations such descriptors. */

    struct device_dma_parameters *dma_parms;

    struct list_head    dma_pools;  /* dma pools (if dma'ble) */

    struct dma_coherent_mem *dma_mem; /* internal for coherent mem
                         override */
    /* arch specific additions */
    struct dev_archdata archdata;

    struct device_node  *of_node; /* associated device tree node */

    dev_t           devt;   /* dev_t, creates the sysfs "dev" */

    spinlock_t      devres_lock;
    struct list_head    devres_head;

    struct klist_node   knode_class;
    struct class        *class;
    const struct attribute_group **groups;  /* optional groups */

    void    (*release)(struct device *dev);
};
```

### 分配platform_device结构

注册一个platform_device之前，必须先定义或者通过platform_device_alloc()函数为设备分配一个platform_device结构。原型如下：

```cpp
struct platform_device *platform_device_alloc(const char *name,int id);
```

### 添加资源 

通过platform_device_alloc()申请得到的platform_device结构，必须添加相关资源和私有数据才能注册。添加资源的函数是platform_device_add_resources：

```cpp
int platform_device_add_resources(struct platform_device *pdev,const struct resource *res unsigned int num);
```

添加私有数据的函数是platform_device_add_data:

```cpp
int platform_device_add_data(struct platform_device *pdev,const void *data,size_t size)
```

### 注册和注销platform_device

申请到platform_device结构后，可以通过platform_device_register()往系统进行注册。原型如下：

```cpp
int platform_device_register(struct platform_device *pdev);
```

上述函数只能注册一个platform_device，如果有多个platform_device,则可以用platform_add_devcies()一次性完成注册，原型如下：

```cpp
int platform_add_devices(struct platform_device *devs,int num);
```

如果已经定义了设备的资源和私有数据，可以用 platform_device_register_resndata()一次 
性完成数据结构申请、资源和私有数据添加以及设备注册：

```cpp
struct platform_device *__init_or_module platform_device_register_resndata(struct device *parent,
const char *name, int id,const struct resource *res, unsigned int num,const void *data, size_t size);
```

platform_device_register_simple()函数是 platform_device_register_resndata()函数的简化版， 可以一步实现分配和注册设备操作， platform_device_register_simple()函数原型如下：

```cpp
static inline struct platform_device *platform_device_register_simple(const char *name, int id,const struct resource *res, unsigned int num);
```

实际上就是： platform_device_register_resndata(NULL, name, id, res, num, NULL, 0)。在\Linux/platform_device.h/文件还提供了更多的 platform_device 相关的操作接口函数，在有必要的时候可以查看并使用。

### 系统添加平台设备的流程

向系统添加一个平台设备，可以通过两种方式完成：
方式 1：定义资源，然后定义 platform_device 结构并初始化；最后注册； 
方式 2：定义资源，然后动态分配一个 platform_device 结构，接着往结构添加资源 信息，最后注册。 

## 平台驱动

### **platform_driver** 

```cpp
struct platform_driver {
int (*probe)(struct platform_device *);     /* probe 方法 */
int (*remove)(struct platform_device *);    /* remove 方法 */
void (*shutdown)(struct platform_device *); /* shutdown 方法 */
int (*suspend)(struct platform_device *, pm_message_t state); /* suspend 方法 */
int (*resume)(struct platform_device *);    /* resume 方法 */
struct device_driver driver;                 /* 设备驱动 */
const struct platform_device_id *id_table;  /* 设备的 ID 表 */
};
```

Platform_driver 有 5 个方法：
probe成员指向驱动的探测代码，在 probe方法中获取设备的资源信息并进行处理， 如进行物理地址到虚拟地址的 remap，或者申请中断等操作，与模块的初始化代码 不同； 
remove 成员指向驱动的移除代码，进行一些资源释放和清理工作，如取消物理地 址与虚拟地址的映射关系，或者释放中断号等，与模块的退出代码不同； 
shutdown 成员指向设备被关闭时的实现代码； 
suspend 成员执行设备挂起时候的处理代码； 
resume 成员执行设备从挂起中恢复的处理代码 

### 注册和注销平台设备

注册和注销 platform_driver 的函数分别是 platform_driver_register()和 platform_driver _unregister()， 函数原型分别如下：

```cpp
int platform_driver_register(struct platform_driver *drv);
void platform_driver_unregister(struct platform_driver *drv);
```

另外， platform_driver_probe()函数也能完成设备注册， 原型如下：

```cpp
int platform_driver_probe(struct platform_driver *driver, int (*probe)(struct platform_device *));
```

注意：在设备驱动模型中已经提到，bus 根据驱动和设备的名称寻找匹配的设备和驱动， 因此注册驱动必须保证 platform_driver 的 driver.name 字段必须和 platform_device 的 name相 同， 否则无法将驱动和设备进行绑定而注册失败。

## demo

### platdev_led.h

```cpp
#ifndef  _PLATDEV_LED_H_
#define  _PLATDEV_LED_H_


#include <linux/platform_device.h>
#include <linux/version.h>


#include <mach/regs-gpio.h>


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <asm/irq.h>
#else
#include <asm-arm/irq.h>
#include <asm/arch/gpio.h>
#include <asm/arch/hardware.h>
#endif

#define ENPULLUP                    1
#define DISPULLUP                   0

#define HIGHLEVEL                   1
#define LOWLEVEL                    0

#define INPUT                       1
#define OUTPUT                      0

#define OFF                         0
#define ON                          1

#define ENABLE                      1
#define DISABLE                     0

/*  LED hardware informtation structure*/
struct s3c_led_info
{
    unsigned char           num;              /* The LED number  */
    unsigned int            gpio;             /* Which GPIO the LED used */  
    unsigned char           active_level;     /* The GPIO pin level(HIGHLEVEL or LOWLEVEL) to turn on or off  */
    unsigned char           status;           /* Current LED status: OFF/ON */
    unsigned char           blink;            /* Blink or not */           
};

/*  The LED platform device private data structure */
struct s3c_led_platform_data
{
    struct s3c_led_info    *leds;
    int                     nleds;
};

#endif   /* ----- #ifndef _PLATDEV_LED_H_  ----- */
```

### led_device.c

```cpp
#include <linux/module.h>   
#include <linux/init.h>    
#include <linux/kernel.h>  

#include "platdev_led.h"

/* LED hardware informtation data*/
static struct s3c_led_info  s3c_leds[] = {
    [0] = {
        .num = 1,
        .gpio = S3C2410_GPB(5),
        .active_level = LOWLEVEL,
        .status = OFF,
        .blink = ENABLE,
    },
    [1] = {
        .num = 2,
        .gpio = S3C2410_GPB(6),
        .active_level = LOWLEVEL,
        .status = OFF,
        .blink = DISABLE,
    },
    [2] = {
        .num = 3,
        .gpio = S3C2410_GPB(8),
        .active_level = LOWLEVEL,
        .status = OFF,
        .blink = DISABLE,
    },
    [3] = {
        .num = 4,
        .gpio = S3C2410_GPB(10),
        .active_level = LOWLEVEL,
        .status = OFF,
        .blink = DISABLE,
    },
};


/*  The LED platform device private data */
static struct s3c_led_platform_data s3c_led_data = {
    .leds = s3c_leds,
    .nleds = ARRAY_SIZE(s3c_leds),
};

static void platform_led_release(struct device * dev)
{
    int i;
    struct s3c_led_platform_data *pdata = dev->platform_data;


    for(i=0; i<pdata->nleds; i++)
    {
        /* Turn all LED off */
        s3c2410_gpio_setpin(pdata->leds[i].gpio, ~pdata->leds[i].active_level);
    }
}

static struct platform_device s3c_led_device = {
    .name    = "s3c_led",
    .id      = 1,
    .dev     =
    {
        .platform_data = &s3c_led_data,
        .release = platform_led_release,
    },
};

static int __init platdev_led_init(void)
{
   int       rv = 0;


   rv = platform_device_register(&s3c_led_device);
   if(rv)
   {
        printk(KERN_ERR "%s:%d: Can't register platform device %d\n", __FUNCTION__,__LINE__, rv);
        return rv;
   }
   printk("Regist S3C LED Platform Device successfully.\n");
   return 0;
}

static void platdev_led_exit(void)
{
    printk("%s():%d remove LED platform device\n", __FUNCTION__,__LINE__);
    platform_device_unregister(&s3c_led_device);
}

module_init(platdev_led_init);
module_exit(platdev_led_exit);

MODULE_AUTHOR("zhanghang");
MODULE_DESCRIPTION("FL2440 LED driver platform device");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s3c_platdev_led");
```

每个led灯有多种属性，使用结构体对其封装是最好的选择，使用s3c_led_platform_data结构体是可选的，其作用也只是为了封装，使其调用更简单。
接下来是结构体platform_device的分配，这里使用的是直接定义结构体，也可以采用动态分配。接下来对其必要成员进行初始化，如上所述，这里的name域必须与driver的name域保持相同，添加platform_data域和release域。
最后将其注册到内核platform_device_register。
与其对应，当rmmod时，调用 platform_device_unregister将其卸载。

### led_driver.c

```cpp
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/version.h>

#include "platdev_led.h"

#define DEV_NAME                  "led"

//#define DEV_MAJOR               251
#ifndef DEV_MAJOR
#define DEV_MAJOR                 0 /*  dynamic major by default */
#endif

#define TIMER_TIMEOUT             40

#define PLATDRV_MAGIC             0x60
#define LED_OFF                   _IO (PLATDRV_MAGIC, 0x18)
#define LED_ON                    _IO (PLATDRV_MAGIC, 0x19)
#define LED_BLINK                 _IO (PLATDRV_MAGIC, 0x20)

static int dev_major = DEV_MAJOR;

struct led_device
{
    struct s3c_led_platform_data    *data;
    struct cdev                     cdev;
    struct class                    *dev_class;
    struct timer_list               blink_timer;
} led_device;

void led_timer_handler(unsigned long data)
{
    int  i;
    struct s3c_led_platform_data *pdata = (struct s3c_led_platform_data *)data;

    for(i=0; i<pdata->nleds; i++)
    {
        if(ON == pdata->leds[i].status)
        {
              s3c2410_gpio_setpin(pdata->leds[i].gpio, pdata->leds[i].active_level);
        }
        else
        {
              s3c2410_gpio_setpin(pdata->leds[i].gpio, ~pdata->leds[i].active_level);
        }


        if(ENABLE == pdata->leds[i].blink )  /* LED should blink */
        {
            /* Switch status between 0 and 1 to turn LED ON or off */
            pdata->leds[i].status = pdata->leds[i].status ^ 0x01;  
        }

        mod_timer(&(led_device.blink_timer), jiffies + TIMER_TIMEOUT);
    }
}

static int led_open(struct inode *inode, struct file *file)
{
    struct led_device *pdev ;
    struct s3c_led_platform_data *pdata;

    pdev = container_of(inode->i_cdev,struct led_device, cdev);
    pdata = pdev->data;

    file->private_data = pdata;

    return 0;
}

static int led_release(struct inode *inode, struct file *file)
{
    return 0;
}

static void print_led_help(void)
{
    printk("Follow is the ioctl() command for LED driver:\n");
    printk("Turn LED on command        : %u\n", LED_ON);
    printk("Turn LED off command       : %u\n", LED_OFF);
    printk("Turn LED blink command     : %u\n", LED_BLINK);
}

/* compatible with kernel version >=2.6.38*/
static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct s3c_led_platform_data *pdata = file->private_data;

    switch (cmd)
    {
        case LED_OFF:
            if(pdata->nleds <= arg)
            {
               printk("LED%ld doesn't exist\n", arg);  
               return -ENOTTY;
            }
            pdata->leds[arg].status = OFF;
            pdata->leds[arg].blink = DISABLE;
            break;

        case LED_ON:
            if(pdata->nleds <= arg)
            {
               printk("LED%ld doesn't exist\n", arg);  
               return -ENOTTY;
            }
            pdata->leds[arg].status = ON;
            pdata->leds[arg].blink = DISABLE;
            break;

        case LED_BLINK:
            if(pdata->nleds <= arg)
            {
               printk("LED%ld doesn't exist\n", arg);  
               return -ENOTTY;
            }
            pdata->leds[arg].blink = ENABLE;
            pdata->leds[arg].status = ON;
            break;

        default:
            printk("%s driver don't support ioctl command=%d\n", DEV_NAME, cmd);
            print_led_help();
            return -EINVAL;
    }
    return 0;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .unlocked_ioctl = led_ioctl, /* compatible with kernel version >=2.6.38*/
};

static int s3c_led_probe(struct platform_device *dev)
{
    struct s3c_led_platform_data *pdata = dev->dev.platform_data;
    int result = 0;
    int i;
    dev_t devno;

    /* Initialize the LED status */
    for(i=0; i<pdata->nleds; i++)
    {
         s3c2410_gpio_cfgpin(pdata->leds[i].gpio, S3C2410_GPIO_OUTPUT);
         if(ON == pdata->leds[i].status)
         {
            s3c2410_gpio_setpin(pdata->leds[i].gpio, pdata->leds[i].active_level);
         }
         else
         {
            s3c2410_gpio_setpin(pdata->leds[i].gpio, ~pdata->leds[i].active_level);
         }
    }

    /*  Alloc the device for driver */
    if (0 != dev_major)
    {
        devno = MKDEV(dev_major, 0);
        result = register_chrdev_region(devno, 1, DEV_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&devno, 0, 1, DEV_NAME);
        dev_major = MAJOR(devno);
    }


    /* Alloc for device major failure */
    if (result < 0)
    {
        printk("%s driver can't get major %d\n", DEV_NAME, dev_major);
        return result;
    }


    /* Initialize button structure and register cdev*/
    memset(&led_device, 0, sizeof(led_device));
    led_device.data = dev->dev.platform_data;
    cdev_init (&(led_device.cdev), &led_fops);
    led_device.cdev.owner  = THIS_MODULE;

    result = cdev_add (&(led_device.cdev), devno , 1);
    if (result)
    {
        printk (KERN_NOTICE "error %d add %s device", result, DEV_NAME);
        goto ERROR;
    }
    
    led_device.dev_class = class_create(THIS_MODULE, DEV_NAME);
    if(IS_ERR(led_device.dev_class))
    {
        printk("%s driver create class failture\n",DEV_NAME);
        result =  -ENOMEM;
        goto ERROR;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)     
    device_create(led_device.dev_class, NULL, devno, NULL, DEV_NAME);
#else
    device_create (led_device.dev_class, NULL, devno, DEV_NAME);
#endif

    /*  Initial the LED blink timer */
    init_timer(&(led_device.blink_timer));
    led_device.blink_timer.function = led_timer_handler;
    led_device.blink_timer.data = (unsigned long)pdata;
    led_device.blink_timer.expires  = jiffies + TIMER_TIMEOUT;
    add_timer(&(led_device.blink_timer));

    printk("S3C %s driver version 1.0.0 initiliazed.\n", DEV_NAME);
    return 0;
               
ERROR:
    printk("S3C %s driver version 1.0.0 install failure.\n", DEV_NAME);
    cdev_del(&(led_device.cdev));

    unregister_chrdev_region(devno, 1);
    return result;

}


static int s3c_led_remove(struct platform_device *dev)
{
    dev_t devno = MKDEV(dev_major, 0);

    del_timer(&(led_device.blink_timer));

    cdev_del(&(led_device.cdev));
    device_destroy(led_device.dev_class, devno);
    class_destroy(led_device.dev_class);
    
    unregister_chrdev_region(devno, 1);
    printk("S3C %s driver removed\n", DEV_NAME);

    return 0;
}

static struct platform_driver s3c_led_driver = {
    .probe      = s3c_led_probe,
    .remove     = s3c_led_remove,
    .driver     = {
        .name       = "s3c_led",
        .owner      = THIS_MODULE,
    },
};

static int __init platdrv_led_init(void)
{
   int       rv = 0;

   rv = platform_driver_register(&s3c_led_driver);
   if(rv)
   {
        printk(KERN_ERR "%s:%d: Can't register platform driver %d\n", __FUNCTION__,__LINE__, rv);
        return rv;
   }
   printk("Regist S3C LED Platform Driver successfully.\n");

   return 0;
}

static void platdrv_led_exit(void)
{
    printk("%s():%d remove LED platform drvier\n", __FUNCTION__,__LINE__);
    platform_driver_unregister(&s3c_led_driver);
}

module_init(platdrv_led_init);
module_exit(platdrv_led_exit);

module_param(dev_major, int, S_IRUGO);

MODULE_AUTHOR("zhanghang");
MODULE_DESCRIPTION("FL2440 LED driver platform driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s3c_platdrv_led");
```

比起平台设备的代码，驱动端代码会更复杂一些，驱动端必须实现open，read,release,ioctl等系统调用供应用程序调用，与字符驱动类似，驱动端也要将fops以及主次设备号绑定到cdev结构体，并将cdev结构体注册到Linux内核。两个特殊的函数probe和remove，probe函数将会在insmod后首先调用（在paltform_register（&s3c_led_driver后），操作系统在driver insmod后，会将其放到驱动链表中，同理，device在insmod后，也会被放在对应的设备链表中，操作系统（Linux内核）通过（struct platform_driver)s3c_led_driver和(struct platform_device)s3c_led_device的name域进行匹配，如果name域相同，则可以匹配成功，故一个驱动程序可能与多个设备匹配，反之亦然。匹配成功后，操作系统将device端的信息分享给driver，函数probe开始执行，进行一系列初始化，并创建设备节点，注册cdev，同时一个特殊的结构体成员led_device.data = dev->dev.platform_data，它将用于系统调用led_open时对file->private_data域初始化，为led_ioctl提供操作对象，结构体led_device的另外两个成员：*dev_class，blink_timer，前者用于创建直接设备节点，避免了我们在insmod后，还要自己去创建(mknod)设备节点。后者用于定时器。remove函数，将会在rmmod时被调用，实现对对象的一系列卸载操作，最后调用led_exit函数卸载platform_driver。