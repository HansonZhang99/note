# input button demo（match gpio-keys.c)

(mini2440.c中会默认添加platform device对button的支持，通过platform总线和gpio-keys.c中注册的platform driver匹配，匹配成功后进入input子系统流程，这里为了防止竞争或者线程安全，可以先注释掉mini2440.c中的button device)

如下是mini2440 button原理图：

![image-20230424013015565](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013015565.png)

根据s3c2440 datasheet可以找到对应的管脚，在linux内核中可以找到对应的宏定义

```cpp
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/timer.h>  /*timer*/
#include <asm/uaccess.h>  /*jiffies*/
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/input.h>
struct input_dev *dev ;
static irqreturn_t irq_fuction(int irq, void *dev_id)
{    
    struct input_dev *keydev ;
    #if  0
    if(in_interrupt()){
         printk("%s in interrupt handle!\n",__FUNCTION__);
    }
    #endif
    keydev = dev_id ; //上报键值
    input_report_key(keydev, KEY_HOME,  irq);/*在这里我们上报的数据为type=EV_KEY,code=KEY_HOME,value=irq，四个按键的事件，只有irq不同，所以在应用层只能通过irq来判断是那个按键被按下，当然也可以在驱动层，定义一个规则，如将四个按键的中断号分别对应一个可以标识这个按键的宏，将这个宏作为value上报，在应用层同样需要这个规则的声明，并由这个规则判断是哪个按键被按下。在这里，四个KEY的中断号依次为16 18 19 48*/
    input_sync(keydev); //上报一个同步事件    
    printk("irq:%d\n",irq);
    return IRQ_HANDLED ;
}
/*static inline void input_report_key(struct input_dev *dev, unsigned int code, int value)
{
    input_event(dev, EV_KEY, code, !!value);
}
static inline void input_sync(struct input_dev *dev)
{   
    input_event(dev, EV_SYN, SYN_REPORT, 0);
}
*/
static int __init fl2440_Key_irq_test_init(void)
{
    int err = 0 ;
    int i;
    int irq_num[] = {gpio_to_irq(S3C2410_GPG(0)),gpio_to_irq(S3C2410_GPG(3)),gpio_to_irq(S3C2410_GPG(5)),gpio_to_irq(S3C2410_GPG(6))};
    int ret ;
    struct input_id id ;
    dev = input_allocate_device();
    if(IS_ERR_OR_NULL(dev)){
        ret = -ENOMEM ;
        goto ERR_alloc;
    }
    //对input_id的成员进行初始化
    dev->name = "input_button" ;
    dev->phys = "haha" ;
    dev->uniq = "20190607" ;
/*    dev->id.bustype = BUS_HOST ;
    dev->id.vendor = ID_PRODUCT ;
    dev->id.version = ID_VENDOR ;
    set_bit(EV_SYN,dev->evbit);    //设置为同步事件，这个宏可以在input.h中找到
    set_bit(EV_KEY,dev->evbit);    //因为是按键，所以要设置成按键事件
    set_bit(KEY_HOME,dev->keybit);    //设置这个按键表示为KEY_HOME这个键，到时用来上报*/
    ret = input_register_device(dev); //注册input设备
    if(IS_ERR_VALUE(ret))
        goto ERR_input_reg ;
    for(i=0;i<sizeof(irq_num) / sizeof(int);i++)
    {
        err = request_irq(irq_num[i],irq_fuction,IRQF_TRIGGER_FALLING,"input_button",dev);
        if(err != 0)
            goto free_irq_flag ;
    }
    //以下除了return 0 都为出错处理
    return 0 ;
    ERR_input_reg:
    input_unregister_device(dev);    
    free_irq_flag:
    free_irq(irq_num,(void *)"key");
    ERR_alloc:
    return ret ;
}
static void __exit fl2440_Key_irq_test_exit(void)

{    //为了简单，我这里不需要验证exit的功能，只要在开机init成功就可以了，日后再完善
    int irq_num[] = {gpio_to_irq(S3C2410_GPG(0)),gpio_to_irq(S3C2410_GPG(3)),gpio_to_irq(S3C2410_GPG(5)),gpio_to_irq(S3C2410_GPG(6))};
    int i ;
    printk("irq_key exit\n");
    for(i=0;i<sizeof(irq_num) / sizeof(int);i++)
    {
        free_irq(irq_num[i],dev);
    }
    input_unregister_device(dev);
}
module_init(fl2440_Key_irq_test_init);
module_exit(fl2440_Key_irq_test_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZH");
MODULE_DESCRIPTION("S3C2440 KEY Driver");



/*这里没有使用定时器，按键触发中断会直接进入中断处理函数，并将产生的事件上报。

```

Makefile

```Makefile
LINUX_SRC=${shell pwd}/../../linux/linux_kernel//*内核路径，与开发板内核版本要一致*/
CROSS_COMPILE=armgcc/*交叉编译器路径，我用了环境变量简化*/
INST_PATH=/tftp

PWD:=${shell pwd}
EXTRA_CFLAGS +=-DMODULE

#obj-m +=test.o
obj-m +=button.o(对应上面的.c文件名要一致*/
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
    @rm -f button.ko
```



run

```shell
/dev/input >: ls
mice    /*可以看到在我们在编译内核时，是选择了input子系统支持的，下面只有mice被编译到了内核中，我们的evdev.c支持handler层也会被编译进来*/

~ >: insmod button.ko
input: input_button1 as /devices/virtual/input/input1

~ >: ls /dev/input/
event0  mice
~ >: cat /proc/bus/input/handlers
N: Number=0 Name=kbd
N: Number=1 Name=sysrq (filter)
N: Number=2 Name=mousedev Minor=32
N: Number=3 Name=evdev Minor=64

~ >: cat /proc/bus/input/devices
input: input_button as /devices/virtual/input/input0
~ >: cat /proc/bus/input/devices
I: Bus=0000 Vendor=0000 Product=0000 Version=0000
N: Name="input_button"
P: Phys=haha
S: Sysfs=/devices/virtual/input/input0
U: Uniq=20190607
H: Handlers=event0
B: PROP=0
B: EV=1

/*此时我们按下对应按键，查看文件/dev/input/event0
~ >: cat /dev/input/event0
irq:16
irq:16
irq:18
irq:19
irq:18
irq:16
irq:16
irq:16
^C
/*每一次按下都会有irq提示，此事件会被放到evdev的client的buffer中，应用层可通过read读取数据，并做出相应的动作*/
```

