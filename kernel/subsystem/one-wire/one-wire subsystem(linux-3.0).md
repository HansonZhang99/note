# one-wire subsystem(linux-3.0)

目前常用的微机与外设之间进行数据传输的串行总线主要有I2C总线、SPI总线和SCI总线。其中I2C总线以同步串行2线方式进行通信（一条时钟线，一条数据线），SPI总线则以同步串行3线方式进行通信（一条时钟线，一条数据输入线，一条数据输出线），而SCI总线是以异步方式进行通信（一条数据输入线，一条数据输出线）的。这些总线至少需要两条或两条以上的信号线。近年来，美国的达拉斯半导体公司（DALLAS SEMICONDUCTOR）推出了一项特有的单总线（1-Wire Bus）技术。该技术与上述总线不同，它采用单根信号线，既可传输时钟，又能传输数据，而且数据传输是双向的，因而这种单总线技术具有线路简单，硬件开销少，成本低廉，便于总线扩展和维护等优点。

​     单总线适用于单主机系统，能够控制一个或多个从机设备。主机可以是微控制器，从机可以是单总线器件，它们之间的数据交换只通过一条信号线。当只有一个从机设备时，系统可按单节点系统操作；当有多个从设备时，系统则按多节点系统操作。图1所示是单总线多节点系统的示意图

![image-20230424013125290](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013125290.png)

**单总线的工作原理：**

​     顾名思义，单总线即只有一根数据线，系统中的数据交换、控制都由这根线完成。设备（主机或从机）通过一个漏极开路或三态端口连至该数据线，以允许设备在不发送数据时能够释放总线，而让其它设备使用总线，其内部等效电路如图2所示。单总线通常要求外接一个约为4.7kΩ的上拉电阻，这样，当总线闲置时，其状态为高电平。主机和从机之间的通信可通过3个步骤完成，分别为初始化1-wire器件、识别1-wire器件和交换数据。由于它们是主从结构，只有主机呼叫从机时，从机才能应答，因此主机访问1-wire器件都必须严格遵循单总线命令序列，即初始化、ROM、命令功能命令。如果出现序列混乱，1-wire器件将不响应主机（搜索ROM命令，报警搜索命令除外）。表1是列为ΔΙΩ命令的说明，而功能命令则根据具体1-wire器件所支持的功能来确定。

![image-20230424013138160](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013138160.png)

**信号方式：**

​     所有的单总线器件都要遵循严格的通信协议，以保证数据的完整性。

​     **1-wire协议定义了复位脉冲、应答脉冲、写0、读0和读1时序等几种信号类型。**所有的单总线命令序列（初始化，ROM命令，功能命令）都是由这些基本的信号类型组成的。在这些信号中，除了应答脉冲外，其它均由主机发出同步信号，并且发送的所有命令和数据都是字节的低位在前。图3是这些信号的时序图。其中，图3(a)是初始化时序，初始化时序包括主机发出的复位脉冲和从机发出的应答脉冲。主机通过拉低单总线至少480μs产生Tx复位脉冲；然后由主机释放总线，并进入Rx接收模式。主机释放总线时，会产生一由低电平跳变为高电平的上升沿，单总线器件检测到该上升沿后，延时15～60μs，接着单总线器件通过拉低总线60～240μsμ来产生应答脉冲。主机接收到从机的以应答脉冲后，说明有单总线器件在线，然后主机就可以开始对从机进行ROM命令和功能命令操作。图3中的（b）、（c）、（d）分别是写1、写0和读时序。在每一个时序中，总线只能传输一位数据。所有的读、写时序至少需要60μs，且每两个独立的时序之间至少需要1μs的恢复时间。图中，读、写时序均始于主机拉低总线。在写时序中，主机将在拉低总线15μs之内释放总线，并向单总线器件写1；若主机拉低总线后能保持至少60μs的低电平，则向单总线器件写0。单总线器件仅在主机发出读时序时才向主机传输数据，所以，当主机向单总线器件发出读数据命令后，必须马上产生读时序，以便单总线器件能传输数据。在主机发出读时序之后，单总线器件才开始在总线上发送0或1。若单总线器件发送1，则总线保持高电平，若发送0，则拉低总线。由于单总线器件发送数据后可保持15μs有效时间，因此，主机在读时序期间必须释放总线，且须在15μs的采样总线状态，以便接收从机发送的数据。

![image-20230424013210655](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013210655.png)

![image-20230424013254739](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013254739.png)



![image-20230424013310807](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013310807.png)

通常把挂在单总线上的器件称之为单总线器件，单总线器件内一般都具有控制、收发、存储等电路。为了区分不同的单总线器件，厂家生产单总线器件时都要刻录一个64位的二进制ROM代码，以标志其ID号。目前，单总线器件主要有数字温度传感器（如DS18B20）、A/D转换器（如 DS2450）、门标、身份识别器（如DS1990A）、单总线控制器（如DS1WM）等。



接下来介绍通过内核自带的w1子系统来使能ds18b20。

在Linux内核的drivers下有一个名为w1的子系统，奇怪的是，这个子系统在网上的资料寥寥无几。这个子系统就是Linux下单总线协议的实现。

可以看到在w1目录下，还有两个目录masters/slaves。

在master目录下，我们看到w1-gpio.c文件，此文件下实现了单总线的I/O操作方法：

我们不仔细去了解这些实现，简单的看一下：

找到__init函数，开始分析：

```cpp
static int __init w1_gpio_init(void)
{
    return platform_driver_probe(&w1_gpio_driver, w1_gpio_probe);
}
```

先看看**platform_driver_probe**函数：

```cpp
int __init_or_module platform_driver_probe(struct platform_driver *drv,
        int (*probe)(struct platform_device *))
{
    int retval, code;

    /* make sure driver won't have bind/unbind attributes */
    drv->driver.suppress_bind_attrs = true;

    /* temporary section violation during probe() */
    drv->probe = probe;
    retval = code = platform_driver_register(drv);

    /*
     * Fixup that section violation, being paranoid about code scanning
     * the list of drivers in order to probe new devices.  Check to see
     * if the probe was successful, and make sure any forced probes of
     * new devices fail.
     */
    spin_lock(&drv->driver.bus->p->klist_drivers.k_lock);
    drv->probe = NULL;
    if (code == 0 && list_empty(&drv->driver.p->klist_devices.k_list))
        retval = -ENODEV;
    drv->driver.probe = platform_drv_probe_fail;
    spin_unlock(&drv->driver.bus->p->klist_drivers.k_lock);

    if (code != retval)
        platform_driver_unregister(drv);
    return retval;
}
```

再看到结构体**w1_gpio_driver**和函数**w1_gpio_probe：**

```cpp
static struct platform_driver w1_gpio_driver = {
    .driver = {
        .name   = "w1-gpio",
        .owner  = THIS_MODULE,
    },
    .remove = __exit_p(w1_gpio_remove),
    .suspend = w1_gpio_suspend,
    .resume = w1_gpio_resume,
};
static int __init w1_gpio_probe(struct platform_device *pdev)
{
    struct w1_bus_master *master;
    struct w1_gpio_platform_data *pdata = pdev->dev.platform_data;
    int err;

    if (!pdata)
        return -ENXIO;

    master = kzalloc(sizeof(struct w1_bus_master), GFP_KERNEL);
    if (!master)
        return -ENOMEM;

    err = gpio_request(pdata->pin, "w1");
    if (err)
        goto free_master;

    master->data = pdata;
    master->read_bit = w1_gpio_read_bit;

    if (pdata->is_open_drain) {
        gpio_direction_output(pdata->pin, 1);
        master->write_bit = w1_gpio_write_bit_val;
    } else {
        gpio_direction_input(pdata->pin);
        master->write_bit = w1_gpio_write_bit_dir;
    }

    err = w1_add_master_device(master);
    if (err)
        goto free_gpio;

    if (pdata->enable_external_pullup)
        pdata->enable_external_pullup(1);

    platform_set_drvdata(pdev, master);

    return 0;

free_gpio:
    gpio_free(pdata->pin);
free_master:
    kfree(master);

    return err;
}
```

这里用到了platform总线，**platform_driver_probe**函数为platform_driver结构体做一些初始化，并将 platform_driver结构体的probe成员设置为**w1_gpio_probe**，并注册platform_driver结构体到内核，按照platform总线的风格，device和driver在链表匹配后，**w1_gpio_probe**函数会被调用，这个函数将注册一个**w1_bus_master**类型的结构体，这个结构体会绑定设备，并封装了I/O操作函数，并将其加入到链表中。



在slaves下的w1_therm.c封装了DS18B20的内部操作方法（读写寄存器），和IO时序无关；我们可以将驱动结构看成是将“w1_therm”挂接到“w1-gpio”总线上，由w1-gpio控制w1_therm工作。

既然是用到了虚拟总线platform,我们只需要在内核使能w1的masters和slaves的某部分，driver就会注册到内核，接下来在内核再添加device端就可以了。

首先看到上面的**w1_gpio_probe**函数的参数，即我们要构造的结构体，主要部分：

```cpp
struct w1_gpio_platform_data {
    unsigned int pin;
    unsigned int is_open_drain:1;
    void (*enable_external_pullup)(int enable);
};  
```

在linux-3.0/arch/arm/mach-s3c2440/mach-smdk2440.c文件中添加注册信息：

```cpp
#include<linux/w1-gpio.h>
#include<linux/gpio.h>//使用到的头文件
static void w1_enable_external_pullup(int enable)
{
    if(enable)
    {
        s3c_gpio_setpull(S3C2410_GPG(0),S3C_GPIO_PULL_UP);
    }
    else
    {
        s3c_gpio_setpull(S3C2410_GPG(0),S3C_GPIO_PULL_NONE);
    }
}
static struct w1_gpio_platform_data ds18b20_data=
{
    .pin=S3C2410_GPG(0),//ds18b20管脚
    .is_open_drain=0,
    .enable_external_pullup=w1_enable_external_pullup,
};
static struct platform_device ds18b20_device_w1_gpio=
{
    .name="w1-gpio",
    .id=-1,
    .dev=
    {
        .platform_data=&ds18b20_data,
    },
};
static struct platform_device *smdk2440_devices[] __initdata = {
    &s3c_device_ohci,
    &s3c_device_lcd,
    &s3c_device_wdt,
    &s3c_device_i2c0,
    &s3c_device_iis,
    &s3c_device_dm9000,//add for dm9000
    &s3c_device_led,
    &ds18b20_device_w1_gpio,//注册device
};
```

内核选中：

```shell
Device Drivers ---> 
<*> Dallas's 1-wire support --->
--- Dallas's 1-wire support
1-wire Bus Masters  ---> 
<*> GPIO 1-wire busmaster
1-wire Slaves  --->
<*> Thermal family implementation
```

重新编译内核，移植后，在/sys/devices/w1 bus master/28-031604ce05ff下可以看到w1_slave文件，cat w1_slave文件，就可以看到温度

```shell
/sys/devices/w1 bus master/28-031604ce05ff >: cat w1_slave
fe 01 4b 46 7f ff 0c 10 6b : crc=6b YES
fe 01 4b 46 7f ff 0c 10 6b t=31875
```

可以看到，温度为31.875度。

如果在内核中配置并使能了ds18b20的管脚，在别的地方再次使用到这个管脚可能会出错。。。