# platform button driver(linux 3.0)

```cpp
#define DRV_DESC                  "S3C24XX button driver"

/* Driver version*/
#define DRV_MAJOR_VER             1
#define DRV_MINOR_VER             0
#define DRV_REVER_VER             0

#define DEV_NAME                  DEV_BUTTON_NAME

//#define DEV_MAJOR               DEV_BUTTON_MAJOR
#ifndef DEV_MAJOR
#define DEV_MAJOR                 0 /* dynamic major by default */
#endif

#define BUTTON_UP                 0 /* Button status is up */
#define BUTTON_DOWN               1 /* Button status is pushed down */
#define BUTTON_UNCERTAIN          2 /* Button status uncerntain */

#define TIMER_DELAY_DOWN          (HZ/50)   /*Remove button push down dithering timer delay 20ms  */
#define TIMER_DELAY_UP            (HZ/10)   /*Remove button up dithering timer delay 100ms  */

static int debug = DISABLE;
static int dev_major = DEV_MAJOR;
static int dev_minor = 0;

struct s3c_button_info                                            //定义按键结构体信息
{
    unsigned char           num;       /*Button nubmer  */                        //第几个按键
    char *                  name;      /*Button nubmer  */                        //按键名称
    int                     nIRQ;      /*Button IRQ number*/                    //按键中断号
    unsigned int            setting;   /*Button IRQ Pin Setting*/                //对应管脚设置为中断模式
    unsigned int            gpio;      /*Button GPIO port */                    //按键对应管脚
};


/* The button plaotform device private data structure */
struct s3c_button_platform_data                                    //定义一个platform总线的按键结构体
{
    struct s3c_button_info *buttons;
    int                     nbuttons;
};

static int debug = DISABLE;
static int dev_major = DEV_MAJOR;
static int dev_minor = 0;
```

宏定义不做过多分析，记住按键驱动一般不止两种状态，除了按下和弹起，其触发状态还有边沿触发，也就是当电平发生0，1跳变时，才会触发相应的动作，在这里将按键弹起（常态）设置为0，按下设置为1，采用下降沿触发，即电平由1变为0是触发相应动作，即按键弹起时触发。
对于每一个设备（按键），都有其物理信息，在内核编程中，这些信息通常用一个结构体数组来存放，(s3c_button_info)，而结构体   s3c_button_platform_data将会在设备的注册（结构体类型platform_device)时作为参数传给内核(.dev成员只能接受同一个参数，将设备信息和设备个数封装成结构体传入）。

```cpp
static struct s3c_button_info  s3c_buttons[] = {                //定义每个按键的结构体信息
    [0] = {
        .num = 1,
        .name = "KEY1",
        .nIRQ = IRQ_EINT0,
        .gpio = S3C2410_GPF(0),
        .setting = S3C2410_GPF0_EINT0,
    },
    [1] = {
        .num = 2,
        .name = "KEY2",
        .nIRQ = IRQ_EINT2,
        .gpio = S3C2410_GPF(2),
        .setting = S3C2410_GPF2_EINT2,
    },
    [2] = {
        .num = 3,
        .name = "KEY3",
        .nIRQ = IRQ_EINT3,
        .gpio = S3C2410_GPF(3),
        .setting = S3C2410_GPF3_EINT3,
    },
    [3] = {
        .num = 4,
        .name = "KEY4",
        .nIRQ = IRQ_EINT4,
        .gpio = S3C2410_GPF(4),
        .setting = S3C2410_GPF4_EINT4,
    },
};


/* The button platform device private data */
static struct s3c_button_platform_data s3c_button_data = {                    //定义button的结构体信息
    .buttons = s3c_buttons,                                                    //每个button的信息
    .nbuttons = ARRAY_SIZE(s3c_buttons),                                    //button的数量
};
```

这里的.nIRQ，.gpio,.setting成员信息需要查看硬件的相应电路图才能确定，Linux内核提供了很多方便用户操作的函数结构，而这些函数接口的使用，当然也是有相应的规定。按键信息如下，相应管脚的定义在Linux内核中（arch/arm/mach-s3c2440/include/mach，arch/arm/mach-s3c2410/include/mach/regs-gpio.h）。



![Image](C:\Users\86135\AppData\Local\Temp\chrome_drag10268_20311\Image.png)

![Image](C:\Users\86135\AppData\Local\Temp\chrome_drag10268_32699\Image.png)

结构体s3c_button_data如上所述初始化为结构体数组（设备信息数组）和成员数量（结构体数组大小）。

```cpp
struct button_device                                                        //定义一个button_device的结构体
{
    unsigned char                      *status;      /* The buttons Push down or up status */
    struct s3c_button_platform_data    *data;        /* The buttons hardware information data */
    struct timer_list                  *timers;      /* The buttons remove dithering timers */
    wait_queue_head_t                  waitq;           /* Wait queue for poll()  */
    volatile int                       ev_press;     /* Button pressed event */
    struct cdev                        cdev;           
    struct class                       *dev_class;
} button_device;
```

结构体button_device为自定义结构体，其成员包含了设备所要进行操作所必备的数据结构。
status成员代表按键的状态，该成员代表按键的按下和弹起，当应用程序调用open时，即对应内核函数button_open时，status成员获得内存，并初始化为弹起状态。在按键被按下或者弹起时，status会改变。
data成员指向我们定义的按键信息结构体。
timers成员作为一个定时器，其初始化有多种方式，在platformLED驱动中用到了与该处不同的初始化方式。
waitq和ev_press成员通常会同时出现，后面会做解释。
cdev为注册结构体。
dev_class成员用于设备节点的创建。

```cpp
static void platform_button_release(struct device * dev)
{
    return;
}
static struct platform_device s3c_button_device = {                    //设备节点结构体
    .name    = "s3c_button",
    .id      = 1,
    .dev     =
    {
        .platform_data = &s3c_button_data,
        .release = platform_button_release,
    },
};
```

使用platform总线驱动时，platform_device和platform_driver的注册是必不可少的(insmod)
release函数会在platform_device被移除（rmmod)是被调用，通常不做任何事，但是少了它内核会报错。

```cpp
static irqreturn_t s3c_button_intterupt(int irq,void *de_id)        //中断处理程序
{
    int i;
    int found = 0;
    struct s3c_button_platform_data *pdata = button_device.data;


    for(i=0; i<pdata->nbuttons; i++)
    {
        if(irq == pdata->buttons[i].nIRQ)                            //发现某个按键有下降沿
        {
            found = 1;
            break;
        }
    }
    if(!found) /* An ERROR interrupt  */                            //内核报中断没有中断，返回错误，基本不会发生
        return IRQ_NONE;
    /* Only when button is up then we will handle this event */


    if(BUTTON_UP  == button_device.status[i])                        
    {
       button_device.status[i] = BUTTON_UNCERTAIN;
       mod_timer(&(button_device.timers[i]), jiffies+TIMER_DELAY_DOWN);
    }
    return IRQ_HANDLED;
}
static void button_timer_handler(unsigned long data)
{
    struct s3c_button_platform_data *pdata = button_device.data;
    int num =(int)data;
    int status = s3c2410_gpio_getpin( pdata->buttons[num].gpio );


    if(LOWLEVEL == status)
    {
        if(BUTTON_UNCERTAIN == button_device.status[num]) /* Come from interrupt */
        {
            //dbg_print("Key pressed!\n");
            button_device.status[num] = BUTTON_DOWN;

            printk("%s pressed.\n", pdata->buttons[num].name);

            /* Wake up the wait queue for read()/poll() */
            button_device.ev_press = 1;
            wake_up_interruptible(&(button_device.waitq));
        }

        /* Cancel the dithering  */
        mod_timer(&(button_device.timers[num]), jiffies+TIMER_DELAY_UP);
    }
    else
    {
        //dbg_print("Key Released!\n");
        button_device.status[num] = BUTTON_UP;
     //   enable_irq(pdata->buttons[num].nIRQ);
    }
    return ;
}


/*===================== Button device driver part ===========================*/


static int button_open(struct inode *inode, struct file *file)                                    //打开按键
{
    struct button_device *pdev ;
    struct s3c_button_platform_data *pdata;
    int i, result;


    pdev = container_of(inode->i_cdev,struct button_device, cdev);                                //通过i_cdev找到cdev结构体
    pdata = pdev->data;
    file->private_data = pdev;


    /* Malloc for all the buttons remove dithering timer */
    pdev->timers = (struct timer_list *) kmalloc(pdata->nbuttons*sizeof(struct timer_list), GFP_KERNEL);            //为cdev结构体中的timer分配内存
    if(NULL == pdev->timers)
    {
        printk("Alloc %s driver for timers failure.\n", DEV_NAME);
        return -ENOMEM;
    }
    memset(pdev->timers, 0, pdata->nbuttons*sizeof(struct timer_list));                            //初始化timer内存


    /* Malloc for all the buttons status buffer */
    pdev->status = (unsigned char *)kmalloc(pdata->nbuttons*sizeof(unsigned char), GFP_KERNEL);                        //为button状态分配内存空间
    if(NULL == pdev->status)    
    {
        printk("Alloc %s driver for status failure.\n", DEV_NAME);
        result = -ENOMEM;
        goto  ERROR;
    }
    memset(pdev->status, 0, pdata->nbuttons*sizeof(unsigned char));                                //初始化状态内存


    init_waitqueue_head(&(pdev->waitq));                                                        //加入等待队列


    for(i=0; i<pdata->nbuttons; i++)                                                             //初始化button信息
    {
        /* Initialize all the buttons status to UP  */
        pdev->status[i] = BUTTON_UP;                                                             //设置为未按


        /* Initialize all the buttons' remove dithering timer */
        setup_timer(&(pdev->timers[i]), button_timer_handler, i);                                //设置定时器及调用函数


        /* Set all the buttons GPIO to EDGE_FALLING interrupt mode */
        s3c2410_gpio_cfgpin(pdata->buttons[i].gpio, pdata->buttons[i].setting);            //配置成中断模式
        irq_set_irq_type(pdata->buttons[i].nIRQ, IRQ_TYPE_EDGE_FALLING);                //中断采用下降沿触发


        /* Request for button GPIO pin interrupt  */
        result = request_irq(pdata->buttons[i].nIRQ, s3c_button_intterupt, IRQF_DISABLED, DEV_NAME, (void *)i);
//注册给内核，一旦发生pdata->buttons[i].nIRQ中断号的中断之后，调用s3c_button_intterupt中断处理程序
        if( result )
        {
            result = -EBUSY;
            goto ERROR1;
        }
    }


    return 0;


ERROR1:                                                        //出错反向退出
     kfree((unsigned char *)pdev->status);
     while(--i)
     {
         disable_irq(pdata->buttons[i].nIRQ);
         free_irq(pdata->buttons[i].nIRQ, (void *)i);
     }


ERROR:
     kfree(pdev->timers);


     return result;
}
```

这里直接分析button_open函数，应用程序空间调用open后，内核空间会调用此函数，该函数会初始化button_device的某些功能成员，（为什么不在probe中初始化那些成员呢？因为当设备和驱动在内核配对后，probe函数会内执行，此时也许应用程序空间并不会使用此设备（不调用open），但是在内核空间，内存是非常宝贵的，这样会使probe申请的内存没有“被使用”，导致内核空间的浪费。而放在open中是最合适的），button_open函数只知道打开的文件的inode和file结构体，为了初始化button_device的成员，必须获取button_device的地址。此时，宏函数container_of就可以做这个事，根据inode的i_cdev成员获取cdev结构的地址，再通过cdev再结构体button_device中的位置，获取button_device的首地址。
为timer成员（struct time_list *）分配内存，这里将timer全体成员初始化为0，即成员expires被初始化为0，延时为0，函数setup_timer设置延时时间为0，即不断调用函数button_timer_handler,并将i传给该函数。
为成员status分配内存，并初始化为BUTTON_UP状态。
初始化等待队列。
将gpio口配置为中断模式s3c2410_gpio_cfgpin(pdata->buttons[i].gpio, pdata->buttons[i].setting);
将触发模式设置为下降沿触发irq_set_irq_type(pdata->buttons[i].nIRQ, IRQ_TYPE_EDGE_FALLING);  宏 IRQ_TYPE_EDGE_FALLING定义在 include/linux/interrupt.h中。
request_irq(pdata->buttons[i].nIRQ, s3c_button_intterupt, IRQF_DISABLED, DEV_NAME, (void *)i)；将中断注册到内核。当中断发生时，调用函数s3c_button_intterupt，并将传给该函数。

再来分析两个回调函数：
函数s3c_button_intterupt为中断处理程序，当按键被按下时被调用，并将状态传给此函数（irq)，函数遍历每个button的nIRQ域，找到被按下的button，并将其status域设置为下降沿。并做一个延时后（按键去抖，按键被按下的瞬间，管脚电平可能受到影响，延时可以消除影响），唤醒定时器函数，定时器函数开始执行。
函数button_timer_handler为定时器函数，在probe中被设置，在中断发生后，发生一个去抖延时，并执行此函数，该函数会读取gpio管脚状态，并判断status域是否为下降沿（在中断处理程序中status域会被设置为下降沿），条件成立便设置status域为被按下，同时设置标志ev_press为真(此条件会在为唤醒等待队列的必要条件），最后唤醒等待队列（在后面的阻塞型read函数中，等待队列是实现阻塞的方式），read函数返回数据到用户空间。



```cpp
static int button_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)                        //读按键信息函数
{
    struct button_device *pdev = file->private_data;
    struct s3c_button_platform_data *pdata;
    int   i, ret;
    unsigned int status = 0;


    pdata = pdev->data;


    dbg_print("ev_press: %d\n", pdev->ev_press);                                            
    if(!pdev->ev_press)                                                            //按键没有按下
    {
         if(file->f_flags & O_NONBLOCK)                                            //如果是非阻塞模式则返回EAGAIN
         {
             dbg_print("read() without block mode.\n");
             return -EAGAIN;
         }
         else                                                                    //阻塞模式加入等待队列
         {
             /* Read() will be blocked here */
             dbg_print("read() blocked here now.\n");
             wait_event_interruptible(pdev->waitq, pdev->ev_press);                //加入等待队列
         }
    }

    pdev->ev_press = 0;                                                            //将按键设置为未按下

    for(i=0; i<pdata->nbuttons; i++)                                            //读出按键状态并保存在status中
    {
        dbg_print("button[%d] status=%d\n", i, pdev->status[i]);
        status |= (pdev->status[i]<<i);                                         //这里的status[i]是如何保存各个按键状态信息的？
    }


    ret = copy_to_user(buf, (void *)&status, min(sizeof(status), count));        //将此按键信息发送给用户空间


    return ret ? -EFAULT : min(sizeof(status), count);
}
```

接下来是read系统调用的实现，与ioctl不同，read系统调用通常会返回一个数据到用户空间，（用户空间的read(fd,buf,sizeof))，buf就是从内核带回的数据）。read的实现与应用程序空间有很大关系，也是按键驱动中等待队列设置的原因。。在此内核程序中，read函数想要带回用户空间的数据是对哪一个按键的操作，read在此被设置为阻塞的，并且被加入到等待队列（设置条件ev_press为假），当某一个按键被按下，会触发中断服务程序，中断服务程序会设置status域为下降沿，同时做一个延时去抖，触发定时器函数，传给定时器函数按键号，定时器函数会检测该按键gpio管教状态，判断status域是否为下降沿，成立会设置status域为按下（该status域代表哪个按键被按下，该数据会被带会用户空间），ev_press域为真，唤醒等待队列，read系统调用被唤醒，开始向下执行，再次设置ev_press域为假，并将每个按键的status域返回到应用程序空间。

```cpp
static unsigned int button_poll(struct file *file, poll_table * wait)            //监视button函数
{
    struct button_device *pdev = file->private_data;                            
    unsigned int mask = 0;




    poll_wait(file, &(pdev->waitq), wait);                                        //加入等待队列
    if(pdev->ev_press)
    {
        mask |= POLLIN | POLLRDNORM; /* The data aviable */                     //按键按下设置mask值
    }




    return mask;
}
```

多路复用的poll和select的实现

```cpp
static int button_release(struct inode *inode, struct file *file)
{
    int i;
    struct button_device *pdev = file->private_data;
    struct s3c_button_platform_data *pdata;
    pdata = pdev->data;


    for(i=0; i<pdata->nbuttons; i++)
    {
        disable_irq(pdata->buttons[i].nIRQ);                            //关中断
        free_irq(pdata->buttons[i].nIRQ, (void *)i);                    //删中断
        del_timer(&(pdev->timers[i]));                                    //取消timer
    }


    kfree(pdev->timers);                                                //释放timer内存
    kfree((unsigned char *)pdev->status);                                //释放申请的status内存


    return 0;
}




static struct file_operations button_fops = {                             //功能函数
    .owner = THIS_MODULE,
    .open = button_open,
    .read = button_read,
    .poll = button_poll,
    .release = button_release,
};




static int s3c_button_probe(struct platform_device *dev)    
{
    int result = 0;
    dev_t devno;




    /* Alloc the device for driver  */
    if (0 != dev_major)
    {
        devno = MKDEV(dev_major, dev_minor);
        result = register_chrdev_region(devno, 1, DEV_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&devno, dev_minor, 1, DEV_NAME);
        dev_major = MAJOR(devno);
    }


    /* Alloc for device major failure */
    if (result < 0)
    {
        printk("%s driver can't get major %d\n", DEV_NAME, dev_major);
        return result;
    }


    /*  Initialize button_device structure and register cdev*/
     memset(&button_device, 0, sizeof(button_device));
     button_device.data = dev->dev.platform_data;
     cdev_init (&(button_device.cdev), &button_fops);
     button_device.cdev.owner  = THIS_MODULE;


     result = cdev_add (&(button_device.cdev), devno , 1);
     if (result)
     {
         printk (KERN_NOTICE "error %d add %s device", result, DEV_NAME);
         goto ERROR;
     }


     button_device.dev_class = class_create(THIS_MODULE, DEV_NAME);
     if(IS_ERR(button_device.dev_class))
     {
         printk("%s driver create class failture\n",DEV_NAME);
         result =  -ENOMEM;
         goto ERROR;
     }


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)     
     device_create(button_device.dev_class, NULL, devno, NULL, DEV_NAME);
#else
     device_create (button_device.dev_class, NULL, devno, DEV_NAME);
#endif


     printk("S3C %s driver version %d.%d.%d initiliazed.\n", DEV_NAME, DRV_MAJOR_VER, DRV_MINOR_VER, DRV_REVER_VER);


     return 0;


ERROR:
     printk("S3C %s driver version %d.%d.%d install failure.\n", DEV_NAME, DRV_MAJOR_VER, DRV_MINOR_VER, DRV_REVER_VER);
     cdev_del(&(button_device.cdev));
     unregister_chrdev_region(devno, 1);
     return result;
}




static int s3c_button_remove(struct platform_device *dev)
{
    dev_t devno = MKDEV(dev_major, dev_minor);


    cdev_del(&(button_device.cdev));
    device_destroy(button_device.dev_class, devno);
    class_destroy(button_device.dev_class);


    unregister_chrdev_region(devno, 1);
    printk("S3C %s driver removed\n", DEV_NAME);


    return 0;
}




/*===================== Platform Device and driver regist part ===========================*/


static struct platform_driver s3c_button_driver = {
    .probe      = s3c_button_probe,
    .remove     = s3c_button_remove,
    .driver     = {
        .name       = "s3c_button",
        .owner      = THIS_MODULE,
    },
};




static int __init s3c_button_init(void)
{
   int       ret = 0;


   ret = platform_device_register(&s3c_button_device);
   if(ret)
   {
        printk(KERN_ERR "%s: Can't register platform device %d\n", __FUNCTION__, ret);
        goto fail_reg_plat_dev;
   }
   dbg_print("Regist S3C %s Device successfully.\n", DEV_NAME);


   ret = platform_driver_register(&s3c_button_driver);
   if(ret)
   {
        printk(KERN_ERR "%s: Can't register platform driver %d\n", __FUNCTION__, ret);
        goto fail_reg_plat_drv;
   }
   dbg_print("Regist S3C %s Driver successfully.\n", DEV_NAME);


   return 0;


fail_reg_plat_drv:
   platform_driver_unregister(&s3c_button_driver);
fail_reg_plat_dev:
   return ret;
}




static void s3c_button_exit(void)
{
    platform_driver_unregister(&s3c_button_driver);
    dbg_print("S3C %s platform device removed.\n", DEV_NAME);

    platform_device_unregister(&s3c_button_device);
    dbg_print("S3C %s platform driver removed.\n", DEV_NAME);
}


module_init(s3c_button_init);
module_exit(s3c_button_exit);


module_param(debug, int, S_IRUGO);
module_param(dev_major, int, S_IRUGO);
module_param(dev_minor, int, S_IRUGO);


MODULE_AUTHOR(DRV_AUTHOR);
MODULE_DESCRIPTION(DRV_DESC);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:S3C24XX_button");
```

