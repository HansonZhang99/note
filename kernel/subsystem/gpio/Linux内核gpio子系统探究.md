# Linux内核gpio子系统探究

## 目的

1. 内核如何解析设备树的gpio节点
2. 内核如何将gpio解析成中断



## GPIO和中断

gpio名为通用输入输出general-purpose input/output。相比i2c,spi这些总线，gpio用的应该是最多的。可以直接去控制一个gpio引脚的电平，代表它也可以模拟i2c,spi的信号协议。

对gpio引脚的控制，现在一般直接操作gpio的控制寄存器就可以。如拉高，拉低，输入，输出，都通过寄存器控制。



imx6ull有多个gpio控制器，每个控制器下都有32个gpio引脚。每个控制器通过两条中断线连接gic，控制器的1-16号引脚共用一条中断线，17-32引脚共用一条中断线。当中断发生时，gpio引脚将中断信号发给gpio控制器，控制器硬件逻辑将决定这个信号发往连接到gic的哪条中断线，最后产生中断，可以读取gpio控制器确定是哪个gpio引脚产生了中断。



设备树中申请一个gpio引脚的方法如下：

![image-20230619165051784](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230619165051784.png)

gpios这个key表示申请一个gpio引脚，这个引脚是gpio5的第5个引脚，GPIO_ACTIVE_HIGH表示“高电平有效”，即当此引脚为高电平时，调用内核相关的接口读取这个引脚的逻辑值为1，否则为0。反之，如果设置成GPIO_ACTIVE_LOW，则表示“低电平有效”，当此引脚为低电平时，调用内核相关的接口读取这个引脚的逻辑值，将会是1。



设备树中可能有一些节点申请gpio时使用的key不是“gpios”而是xxx-gpioxx这种，这样的节点内核不能自己解析，一般是用户希望自己处理这个gpio。

## gpio控制器注册代码分析

gpio应该是硬件强相关的，内核提供的通过gpio控制器接口最终一定会操作到硬件，对imx6ull来说，可以通过dts的gpio节点的compatible属性找到gpio的初始化是在drivers/gpio/gpio-mxc.c文件

imx6ull设备树中某个gpio控制器节点：

![image-20230619170322401](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230619170322401.png)

dts被转换为platform_device时gpio节点并不会被内核解析，匹配成功后会调用probe函数：

```cpp
static int mxc_gpio_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct mxc_gpio_port *port;
	struct resource *iores;
	int irq_base = 0;
	int err;

	mxc_gpio_get_hw(pdev);

	port = devm_kzalloc(&pdev->dev, sizeof(*port), GFP_KERNEL);
	if (!port)
		return -ENOMEM;

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);//1
	port->base = devm_ioremap_resource(&pdev->dev, iores);//2
	if (IS_ERR(port->base))
		return PTR_ERR(port->base);

	port->irq_high = platform_get_irq(pdev, 1);//3
	port->irq = platform_get_irq(pdev, 0);
	if (port->irq < 0)
		return port->irq;

	/* the controller clock is optional */
	port->clk = devm_clk_get(&pdev->dev, NULL);//4
	if (IS_ERR(port->clk))
		port->clk = NULL;

	err = clk_prepare_enable(port->clk);//5
	if (err) {
		dev_err(&pdev->dev, "Unable to enable clock.\n");
		return err;
	}

	pm_runtime_set_active(&pdev->dev);//6
	pm_runtime_enable(&pdev->dev);
	err = pm_runtime_get_sync(&pdev->dev);
	if (err < 0)
		goto out_pm_dis;

	/* disable the interrupt and clear the status */
	writel(0, port->base + GPIO_IMR);//7
	writel(~0, port->base + GPIO_ISR);

	if (mxc_gpio_hwtype == IMX21_GPIO) {
		/*
		 * Setup one handler for all GPIO interrupts. Actually setting
		 * the handler is needed only once, but doing it for every port
		 * is more robust and easier.
		 */
		irq_set_chained_handler(port->irq, mx2_gpio_irq_handler);
	} else {														//8
		/* setup one handler for each entry */
		irq_set_chained_handler_and_data(port->irq,
						 mx3_gpio_irq_handler, port);
		if (port->irq_high > 0)
			/* setup handler for GPIO 16 to 31 */
			irq_set_chained_handler_and_data(port->irq_high,
							 mx3_gpio_irq_handler,
							 port);
	}
	//9
	err = bgpio_init(&port->gc, &pdev->dev, 4,
			 port->base + GPIO_PSR,
			 port->base + GPIO_DR, NULL,
			 port->base + GPIO_GDIR, NULL,
			 BGPIOF_READ_OUTPUT_REG_SET);
	if (err)
		goto out_bgio;

	if (of_property_read_bool(np, "gpio_ranges"))
		port->gpio_ranges = true;
	else
		port->gpio_ranges = false;
	//10
	port->gc.request = mxc_gpio_request;
	port->gc.free = mxc_gpio_free;
	port->gc.parent = &pdev->dev;
	port->gc.to_irq = mxc_gpio_to_irq;
	port->gc.base = (pdev->id < 0) ? of_alias_get_id(np, "gpio") * 32 :
					     pdev->id * 32;
	//11
	err = devm_gpiochip_add_data(&pdev->dev, &port->gc, port);
	if (err)
		goto out_bgio;
	//12
	irq_base = irq_alloc_descs(-1, 0, 32, numa_node_id());
	if (irq_base < 0) {
		err = irq_base;
		goto out_bgio;
	}
	//13
	port->domain = irq_domain_add_legacy(np, 32, irq_base, 0,
					     &irq_domain_simple_ops, NULL);
	if (!port->domain) {
		err = -ENODEV;
		goto out_irqdesc_free;
	}
	//14
	/* gpio-mxc can be a generic irq chip */
	err = mxc_gpio_init_gc(port, irq_base, &pdev->dev);
	if (err < 0)
		goto out_irqdomain_remove;
	//15
	list_add_tail(&port->node, &mxc_gpio_ports);

	platform_set_drvdata(pdev, port);
	pm_runtime_put(&pdev->dev);

	return 0;

out_pm_dis:
	pm_runtime_disable(&pdev->dev);
	clk_disable_unprepare(port->clk);
out_irqdomain_remove:
	irq_domain_remove(port->domain);
out_irqdesc_free:
	irq_free_descs(irq_base, 32);
out_bgio:
	dev_info(&pdev->dev, "%s failed with errno %d\n", __func__, err);
	return err;
}
```

结构体mxc_gpio_port描述一个imx6ull的gpio控制器：

```cpp

struct mxc_gpio_port {
	struct list_head node;//imx6ull有多个gpio控制器，将所有控制器放入同一个链表
	struct clk *clk; //时钟可以没有
	void __iomem *base; //寄存器虚拟地址
	int irq; //imx6ull向gic申请了两个中断，这是中断号小的那个
	int irq_high; //同上
	struct irq_domain *domain; //gpio控制器同时也是中断控制器时的domain
	struct gpio_chip gc; //gpio chip定义了一些gpio控制的函数，硬件强相关
	u32 both_edges;
	int saved_reg[6];
	bool gpio_ranges;
};

```

1. 调用platform_get_resource获取寄存器基地址

2. 调用devm_iormap_resource将物理地址转为虚拟地址

3. 获取gpio控制器中注册的两个中断的中断号

4. 时钟设置，可以没有，imx6ull就没有

5. 时钟使能，无效

6. 电源管理

7. 写寄存器，屏蔽gpio控制器的所有中断，清除中断状态位

8. irq_set_chained_handler_and_data根据virq找到irq_desc，并设置irq_desc的handle_irq为mx3_gpio_irq_handler，port将作为这个函数的参数。两个中断号各自控制16个gpio中断

   ![image-20230619191726387](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230619191726387.png)

9. bgpio_init初始化gpio_chip，并对gpio控制器做初始化（操作寄存器）

10. 进一步初始化gpio_chip，前面的bgpio_init是通用的处理，这里的处理是硬件相关的。.base成员解析自设备树节点对应的alias的编号（pdev->id在dts转为platform device是默认初始化为-1），如gpio1对用alias为0，base就是0，gpio1对应alias为1，base就是32，每个gpio控制器管理32个gpio引脚

11. devm_gpiochip_add_data向gpio子系统注册一个gpio chip

12. 在bitmap中分配连续的32个虚拟中断号，并为这32个virq与irq_desc array/radix tree建立映射关系，并返回首个中断号

13. 注册中断控制器的domain，直接为domain初始化这32个virq和hwirq的映射

    这里的hwirq直接从0-31，在domain中直接保存hwirq(0-31)和32个virq的映射

    初始化每个virq对应的irq_desc的irq_data的hwirq和domain成员

    gpio的domain中只有xlate函数，没有translate和alloc函数

14. mxc_gpio_init_gc分配并初始化一个irq_chip_generic结构体，设置所有的virq的irq_desc->handle_irq为handle_level_irq。并将其放入所有virq的irq_desc->irq_data.chip_data中

15. 多个gpio控制器放入同一个链表



### devm_gpiochip_add_data

devm_gpiochip_add_data向内核注册一个gpio_chip:

```cpp

/**
 * devm_gpiochip_add_data() - Resource manager piochip_add_data()
 * @dev: the device pointer on which irq_chip belongs to.
 * @chip: the chip to register, with chip->base initialized
 * Context: potentially before irqs will work
 *
 * Returns a negative errno if the chip can't be registered, such as
 * because the chip->base is invalid or already associated with a
 * different chip.  Otherwise it returns zero as a success code.
 *
 * The gpio chip automatically be released when the device is unbound.
 */
int devm_gpiochip_add_data(struct device *dev, struct gpio_chip *chip,
			   void *data)
{
	struct gpio_chip **ptr;
	int ret;

	ptr = devres_alloc(devm_gpio_chip_release, sizeof(*ptr),
			     GFP_KERNEL);//devm特性不许手动释放
	if (!ptr)
		return -ENOMEM;

	ret = gpiochip_add_data(chip, data);
	if (ret < 0) {
		devres_free(ptr);
		return ret;
	}

	*ptr = chip;
	devres_add(dev, ptr);

	return 0;
}
```



```cpp
int gpiochip_add_data(struct gpio_chip *chip, void *data)
{
	unsigned long	flags;
	int		status = 0;
	unsigned	i;
	int		base = chip->base;
	struct gpio_device *gdev;

	/*
	 * First: allocate and populate the internal stat container, and
	 * set up the struct device.
	 */
	gdev = kzalloc(sizeof(*gdev), GFP_KERNEL);//1
	if (!gdev)
		return -ENOMEM;
	gdev->dev.bus = &gpio_bus_type;//2
	gdev->chip = chip;
	chip->gpiodev = gdev;
	if (chip->parent) {//3
		gdev->dev.parent = chip->parent;
		gdev->dev.of_node = chip->parent->of_node;
	}

#ifdef CONFIG_OF_GPIO
	/* If the gpiochip has an assigned OF node this takes precedence */
	if (chip->of_node)
		gdev->dev.of_node = chip->of_node;
#endif

	gdev->id = ida_simple_get(&gpio_ida, 0, 0, GFP_KERNEL);
	if (gdev->id < 0) {
		status = gdev->id;
		goto err_free_gdev;
	}
	dev_set_name(&gdev->dev, "gpiochip%d", gdev->id);
	device_initialize(&gdev->dev);//4
	dev_set_drvdata(&gdev->dev, gdev);
	if (chip->parent && chip->parent->driver)
		gdev->owner = chip->parent->driver->owner;
	else if (chip->owner)
		/* TODO: remove chip->owner */
		gdev->owner = chip->owner;
	else
		gdev->owner = THIS_MODULE;

	gdev->descs = kcalloc(chip->ngpio, sizeof(gdev->descs[0]), GFP_KERNEL);//5
	if (!gdev->descs) {
		status = -ENOMEM;
		goto err_free_gdev;
	}

	//...

	gdev->ngpio = chip->ngpio;//6
	gdev->data = data;

	//...
	gdev->base = base;

	status = gpiodev_add_to_list(gdev);//7

    //...

	for (i = 0; i < chip->ngpio; i++) {
		struct gpio_desc *desc = &gdev->descs[i];//8

		desc->gdev = gdev;
		//...
	}

#ifdef CONFIG_PINCTRL
	INIT_LIST_HEAD(&gdev->pin_ranges);
#endif

	status = gpiochip_set_desc_names(chip);
	if (status)
		goto err_remove_from_list;

	status = gpiochip_irqchip_init_valid_mask(chip);
	if (status)
		goto err_remove_from_list;

	status = of_gpiochip_add(chip);//9
	if (status)
		goto err_remove_chip;

	acpi_gpiochip_add(chip);

	/*
	 * By first adding the chardev, and then adding the device,
	 * we get a device node entry in sysfs under
	 * /sys/bus/gpio/devices/gpiochipN/dev that can be used for
	 * coldplug of device nodes and other udev business.
	 * We can do this only if gpiolib has been initialized.
	 * Otherwise, defer until later.
	 */
	if (gpiolib_initialized) {
		status = gpiochip_setup_dev(gdev);//10
		if (status)
			goto err_remove_chip;
	}
	return 0;

	//...
	return status;
}
EXPORT_SYMBOL_GPL(gpiochip_add_data)
```

1. 分配一个gpio_device结构体

2. 初始化这个gpio_device的总线是gpio_bus_type

3. 初始化gpio_device的struct device

4. 同上

5. 为gpio_device分配chip->ngpio个struct gpio_desc

6. 进一步初始化gpio_device

7. 调用gpiodev_add_to_list将gpio_device加入全局的gpio_devices链表，链表根据gpio_device->base和gpio_device->ngpio排序

8. 初始化gpio_device->gpio_desc数组的gdev成员，让它们指向gpio_device

9. 函数of_gpiochip_add指定struct gpio_chip成员的of_xlate指针为of_gpio_simple_xlate函数

10. gpiochip_setup_dev注册gpio控制器到内核:

    ```cpp
    static int gpiochip_setup_dev(struct gpio_device *gdev)
    {
    	int status;
    
    	cdev_init(&gdev->chrdev, &gpio_fileops);
    	gdev->chrdev.owner = THIS_MODULE;
    	gdev->chrdev.kobj.parent = &gdev->dev.kobj;
    	gdev->dev.devt = MKDEV(MAJOR(gpio_devt), gdev->id);
    	status = cdev_add(&gdev->chrdev, gdev->dev.devt, 1);
    	if (status < 0)
    		chip_warn(gdev->chip, "failed to add char device %d:%d\n",
    			  MAJOR(gpio_devt), gdev->id);
    	else
    		chip_dbg(gdev->chip, "added GPIO chardev (%d:%d)\n",
    			 MAJOR(gpio_devt), gdev->id);
    	status = device_add(&gdev->dev);
    	if (status)
    		goto err_remove_chardev;
    
    	status = gpiochip_sysfs_register(gdev);
    	if (status)
    		goto err_remove_device;
    
    	/* From this point, the .release() function cleans up gpio_device */
    	gdev->dev.release = gpiodevice_release;
    	pr_debug("%s: registered GPIOs %d to %d on device: %s (%s)\n",
    		 __func__, gdev->base, gdev->base + gdev->ngpio - 1,
    		 dev_name(&gdev->dev), gdev->chip->label ? : "generic");
    
    	return 0;
    
    err_remove_device:
    	device_del(&gdev->dev);
    err_remove_chardev:
    	cdev_del(&gdev->chrdev);
    	return status;
    }
    ```

    调用cdev_add注册一个gpio字符设备，绑定fops为gpio_fileops，主设备号为gpio_devt，次设备号为gdev->id:

    ![image-20230621203709571](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230621203709571.png)

    调用device_add创建sysfs设备节点：

    ![image-20230621203835623](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230621203835623.png)

    调用gpiochip_sysfs_register函数：

    ```cpp
    int gpiochip_sysfs_register(struct gpio_device *gdev)
    {
    	struct device	*dev;
    	struct device	*parent;
    	struct gpio_chip *chip = gdev->chip;
    
    	//...
    	if (chip->parent)
    		parent = chip->parent;
    	else
    		parent = &gdev->dev;
    
    	/* use chip->base for the ID; it's already known to be unique */
    	dev = device_create_with_groups(&gpio_class, parent,
    					MKDEV(0, 0),
    					chip, gpiochip_groups,
    					"gpiochip%d", chip->base);
    	if (IS_ERR(dev))
    		return PTR_ERR(dev);
    
    	mutex_lock(&sysfs_lock);
    	gdev->mockdev = dev;
    	mutex_unlock(&sysfs_lock);
    
    	return 0;
    }
    ```

    获取gpio_device的parent device，调用device_create变种函数注册这个parent设备，设备名为

    gpiochip[chip->base]，并为其添加属性文件gpiochip_groups:

    ![image-20230621204451949](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230621204451949.png)

### mxc_gpio_init_gc

初始化gpio的中断控制器，gpio使用一个通用中断控制器irq_chip_generic。

```cpp
static int mxc_gpio_init_gc(struct mxc_gpio_port *port, int irq_base,
			    struct device *dev)
{
	struct irq_chip_generic *gc;
	struct irq_chip_type *ct;

	gc = irq_alloc_generic_chip("gpio-mxc", 1, irq_base,
				    port->base, handle_level_irq);//1
	if (!gc)
		return -ENOMEM;
	gc->private = port;//2

	ct = gc->chip_types;//3
	ct->chip.parent_device = dev;//4
	ct->chip.irq_ack = irq_gc_ack_set_bit;
	ct->chip.irq_mask = irq_gc_mask_clr_bit;
	ct->chip.irq_unmask = irq_gc_mask_set_bit;
	ct->chip.irq_set_type = gpio_set_irq_type;
	ct->chip.irq_set_wake = gpio_set_wake_irq;
	ct->chip.irq_request_resources = mxc_gpio_irq_reqres;
	ct->chip.irq_release_resources = mxc_gpio_irq_relres,
	ct->chip.flags = IRQCHIP_MASK_ON_SUSPEND;
	ct->regs.ack = GPIO_ISR;
	ct->regs.mask = GPIO_IMR;

	irq_setup_generic_chip(gc, IRQ_MSK(32), IRQ_GC_INIT_NESTED_LOCK,
			       IRQ_NOREQUEST, 0);//5

	return 0;
}
```

1. 调用irq_alloc_generic_chip分配并初始化一个irq_chip_generic结构

2. 在irq_chip_generic中保存硬件参数port

3. 初始化irq_chip_generic->irq_chip

4. irq_setup_generic_chip初始化中断控制器，为irq_desc指定irq_chip和handle

   ```cpp
   void irq_setup_generic_chip(struct irq_chip_generic *gc, u32 msk,
   			    enum irq_gc_flags flags, unsigned int clr,
   			    unsigned int set)
   {
   	struct irq_chip_type *ct = gc->chip_types;
   	struct irq_chip *chip = &ct->chip;
   	unsigned int i;
   
   	raw_spin_lock(&gc_lock);
   	list_add_tail(&gc->list, &gc_list);//1
   	raw_spin_unlock(&gc_lock);
   
   	//...
   
   	for (i = gc->irq_base; msk; msk >>= 1, i++) {//2
   		//...
   		irq_set_chip_and_handler(i, chip, ct->handler);
   		irq_set_chip_data(i, gc);//3
   		irq_modify_status(i, clr, set);
   	}
   	gc->irq_cnt = i - gc->irq_base;
   }
   ```

   1. 将irq_chip_generic加入全局链表gc_list
   2. 遍历所有virq，设置irq_desc->irq_data.chip为irq_chip_generic->chip_types->chip，设置desc->handle_irq为handle_level_irq(irq_alloc_generic_chip初始化时设置的)。设置desc->irq_data.chip_data为irq_chip_generic



## gpio核心初始化分析

### gpiolib_dev_init

drivers/gpio/gpiolib.c

```cpp
static int __init gpiolib_dev_init(void)
{
	int ret;

	/* Register GPIO sysfs bus */
	ret  = bus_register(&gpio_bus_type);//1
	if (ret < 0) {
		pr_err("gpiolib: could not register GPIO bus type\n");
		return ret;
	}

	ret = alloc_chrdev_region(&gpio_devt, 0, GPIO_DEV_MAX, "gpiochip");//2
	if (ret < 0) {
		pr_err("gpiolib: failed to allocate char dev region\n");
		bus_unregister(&gpio_bus_type);
	} else {
		gpiolib_initialized = true;
		gpiochip_setup_devs();//3
	}
	return ret;
}
core_initcall(gpiolib_dev_init);
```

1. 注册gpio bus，这个bus没有match函数，不会与驱动匹配
2. 动态为gpio控制器分配主设备号，此设备号范围0-255
3. 为提前加入到gpio_devices链表的gpio_chip进行注册(cdev_add->device_add->device_create)

### gpiolib_sysfs_init

```cpp
static int __init gpiolib_sysfs_init(void)
{
	int		status;
	unsigned long	flags;
	struct gpio_device *gdev;

	status = class_register(&gpio_class);//1
	if (status < 0)
		return status;

	//...
	spin_lock_irqsave(&gpio_lock, flags);
	list_for_each_entry(gdev, &gpio_devices, list) {
		if (gdev->mockdev)//2
			continue;

		//...
		spin_unlock_irqrestore(&gpio_lock, flags);
		status = gpiochip_sysfs_register(gdev);//3
		spin_lock_irqsave(&gpio_lock, flags);
	}
	spin_unlock_irqrestore(&gpio_lock, flags);

	return status;
}
postcore_initcall(gpiolib_sysfs_init);

```

1. 向内核注册gpio_class
2. 遍历gpio_devices，如果gdev->mockdev存在证明已经向class注册了设备，就不再注册了(core_initcall在postcore_initcall前面被调用)
3. 对还没有注册到class的设备进行注册

### gpio class/group的属性文件

```cpp
static struct class gpio_class = {
	.name =		"gpio",
	.owner =	THIS_MODULE,

	.class_attrs =	gpio_class_attrs,
};
```

内核会为gpio_class注册gpio_class_attrs里的属性文件：

```cpp
static struct class_attribute gpio_class_attrs[] = {
	__ATTR(export, 0200, NULL, export_store),
	__ATTR(unexport, 0200, NULL, unexport_store),
	__ATTR_NULL,
};
```

这些属性分配是export和unexport，这两个属性文件都在/sys/class/gpio目录下

```cpp
static ssize_t export_store(struct class *class,
				struct class_attribute *attr,
				const char *buf, size_t len)//1
{
	long			gpio;
	struct gpio_desc	*desc;
	int			status;

	status = kstrtol(buf, 0, &gpio);//2 
	if (status < 0)
		goto done;

	desc = gpio_to_desc(gpio);//3 
	//...
	status = gpiod_request(desc, "sysfs");//4
	if (status < 0) {
		if (status == -EPROBE_DEFER)
			status = -ENODEV;
		goto done;
	}
	status = gpiod_export(desc, true);//5
	if (status < 0)
		gpiod_free(desc);
	else
		set_bit(FLAG_SYSFS, &desc->flags);

done:
	if (status)
		pr_debug("%s: status %d\n", __func__, status);
	return status ? : len;
}
```

1. echo "xxx" > export 会将"xxx"填入buf

2. 将buf内容转为整数保存到gpio

3. 将这个整数转为gpio_desc结构，gpio_to_desc遍历gpio_devices链表，找到整数gpio落在[gdev->base,gdev->base+gdev->ngpio)区间的那个gdev（gpio_device），并返回gdev->descs[gpio - gdev->base]这个gpio_desc

4. gpiod_request会调用__gpiod_request，这个函数调用gpio_desc->gdev->chip->request函数:chip->request(chip, gpio_chip_hwgpio(desc));其中gpio_chip_hwgpio(desc)获取desc（gpio_desc）在这个gpio_device中所有gpio_desc中的相对偏移。request会涉及到硬件和pinctrl

5. gpiod_export会调用device_create_with_groups为这个gpio_desc在gpio class下创建一个节点目录，这个目录下会有一些属性文件，这些属性文件来自device_create_with_groups的参数gpio_groups:

   ```cpp
   static const struct attribute_group *gpio_groups[] = {
   	&gpio_group,
   	NULL
   };
   
   static const struct attribute_group gpio_group = {
   	.attrs = gpio_attrs,
   	.is_visible = gpio_is_visible,
   };
   
   static struct attribute *gpio_attrs[] = {
   	&dev_attr_direction.attr,
   	&dev_attr_edge.attr,
   	&dev_attr_value.attr,
   	&dev_attr_active_low.attr,
   	NULL,
   };
   ```

   如这里的dev_attr_value属性和dev_attr_direction属性：

   ```cpp
   static ssize_t value_show(struct device *dev,
   		struct device_attribute *attr, char *buf)//1
   {
   	struct gpiod_data *data = dev_get_drvdata(dev);
   	struct gpio_desc *desc = data->desc;
   	ssize_t			status;
   
   	mutex_lock(&data->mutex);
   
   	status = sprintf(buf, "%d\n", gpiod_get_value_cansleep(desc));//2
   
   	mutex_unlock(&data->mutex);
   
   	return status;
   }
   
   static ssize_t value_store(struct device *dev,
   		struct device_attribute *attr, const char *buf, size_t size)
   {
   	struct gpiod_data *data = dev_get_drvdata(dev);
   	struct gpio_desc *desc = data->desc;
   	ssize_t			status;
   
   	mutex_lock(&data->mutex);
   
   	if (!test_bit(FLAG_IS_OUT, &desc->flags)) {
   		status = -EPERM;
   	} else {
   		long		value;
   
   		status = kstrtol(buf, 0, &value);
   		if (status == 0) {
   			gpiod_set_value_cansleep(desc, value);//3
   			status = size;
   		}
   	}
   
   	mutex_unlock(&data->mutex);
   
   	return status;
   }
   static DEVICE_ATTR_RW(value);
   
   
   
   
   static ssize_t direction_show(struct device *dev,
   		struct device_attribute *attr, char *buf)
   {
   	struct gpiod_data *data = dev_get_drvdata(dev);
   	struct gpio_desc *desc = data->desc;
   	ssize_t			status;
   
   	mutex_lock(&data->mutex);
   
   	gpiod_get_direction(desc);//4
   	status = sprintf(buf, "%s\n",
   			test_bit(FLAG_IS_OUT, &desc->flags)
   				? "out" : "in");
   
   	mutex_unlock(&data->mutex);
   
   	return status;
   }
   
   static ssize_t direction_store(struct device *dev,
   		struct device_attribute *attr, const char *buf, size_t size)
   {
   	struct gpiod_data *data = dev_get_drvdata(dev);
   	struct gpio_desc *desc = data->desc;
   	ssize_t			status;
   
   	mutex_lock(&data->mutex);
   
   	if (sysfs_streq(buf, "high"))
   		status = gpiod_direction_output_raw(desc, 1);//5
   	else if (sysfs_streq(buf, "out") || sysfs_streq(buf, "low"))
   		status = gpiod_direction_output_raw(desc, 0);
   	else if (sysfs_streq(buf, "in"))
   		status = gpiod_direction_input(desc);
   	else
   		status = -EINVAL;
   
   	mutex_unlock(&data->mutex);
   
   	return status ? : size;
   }
   static DEVICE_ATTR_RW(direction);
   ```

   1. show/store分别代表用户使用cat/echo对属性文件的操作，参数中的buf将会被填充或者被解析

   2. value_show函数调用gpiod_get_value_cansleep读取并返回这个gpio的值

   3. value_store调用gpiod_set_value_cansleep设置这个gpio的值

   4. direction_show调用gpiod_get_direction获取这个gpio是输入还是输出

   5. direction_store调用gpiod_direction_output_raw/gpiod_direction_input设置gpio的输入输出

​	这些函数最终都会调用到gpio_desc对应的gpio_device的gpio_chip中的函数，这些函数都会涉及到gpio控制器的寄存器操作，硬件强相关





   

   

   

   

   

结构体gpio_chip描述一个抽象的gpio控制器:

```cpp
struct gpio_chip {
	const char		*label;
	struct gpio_device	*gpiodev;//指向一个gpio_device
	struct device		*parent;//父设备，就是gpio dts节点的platform_device->dev
	struct module		*owner;
	//一系列的操作函数
	int			(*request)(struct gpio_chip *chip,
						unsigned offset);
	void			(*free)(struct gpio_chip *chip,
						unsigned offset);
	int			(*get_direction)(struct gpio_chip *chip,
						unsigned offset);
	int			(*direction_input)(struct gpio_chip *chip,
						unsigned offset);
	int			(*direction_output)(struct gpio_chip *chip,
						unsigned offset, int value);
	int			(*get)(struct gpio_chip *chip,
						unsigned offset);
	void			(*set)(struct gpio_chip *chip,
						unsigned offset, int value);
	void			(*set_multiple)(struct gpio_chip *chip,
						unsigned long *mask,
						unsigned long *bits);
	int			(*set_debounce)(struct gpio_chip *chip,
						unsigned offset,
						unsigned debounce);
	int			(*set_single_ended)(struct gpio_chip *chip,
						unsigned offset,
						enum single_ended_mode mode);

	int			(*to_irq)(struct gpio_chip *chip,
						unsigned offset);

	void			(*dbg_show)(struct seq_file *s,
						struct gpio_chip *chip);
	int			base;//该gpio控制器gpio编号的起始值
	u16			ngpio;//该gpio控制器的gpio数量
	const char		*const *names;
	bool			can_sleep;
	bool			irq_not_threaded;

#if IS_ENABLED(CONFIG_GPIO_GENERIC)
	unsigned long (*read_reg)(void __iomem *reg);
	void (*write_reg)(void __iomem *reg, unsigned long data);
	unsigned long (*pin2mask)(struct gpio_chip *gc, unsigned int pin);
	void __iomem *reg_dat;
	void __iomem *reg_set;
	void __iomem *reg_clr;
	void __iomem *reg_dir;
	int bgpio_bits;
	spinlock_t bgpio_lock;
	unsigned long bgpio_data;
	unsigned long bgpio_dir;
#endif

#ifdef CONFIG_GPIOLIB_IRQCHIP
	/*
	 * With CONFIG_GPIOLIB_IRQCHIP we get an irqchip inside the gpiolib
	 * to handle IRQs for most practical cases.
	 */
	struct irq_chip		*irqchip;
	struct irq_domain	*irqdomain;
	unsigned int		irq_base;
	irq_flow_handler_t	irq_handler;
	unsigned int		irq_default_type;
	int			irq_parent;
	bool			irq_need_valid_mask;
	unsigned long		*irq_valid_mask;
	struct lock_class_key	*lock_key;
#endif

#if defined(CONFIG_OF_GPIO)
	/*
	 * If CONFIG_OF is enabled, then all GPIO controllers described in the
	 * device tree automatically may have an OF translation
	 */
	struct device_node *of_node;
	int of_gpio_n_cells;
	int (*of_xlate)(struct gpio_chip *gc,
			const struct of_phandle_args *gpiospec, u32 *flags);
#endif
};

```

结构体gpio_device:

```cpp
struct gpio_device {
	int			id;
	struct device		dev;
	struct cdev		chrdev;
	struct device		*mockdev;
	struct module		*owner;
	struct gpio_chip	*chip;
	struct gpio_desc	*descs;
	int			base;
	u16			ngpio;
	char			*label;
	void			*data;
	struct list_head        list;

#ifdef CONFIG_PINCTRL
	/*
	 * If CONFIG_PINCTRL is enabled, then gpio controllers can optionally
	 * describe the actual pin range which they serve in an SoC. This
	 * information would be used by pinctrl subsystem to configure
	 * corresponding pins for gpio usage.
	 */
	struct list_head pin_ranges;
#endif
};
```

结构体gpio_desc:

```cpp
struct gpio_desc {
	struct gpio_device	*gdev;
	unsigned long		flags;
/* flag symbols are bit numbers */
#define FLAG_REQUESTED	0
#define FLAG_IS_OUT	1
#define FLAG_EXPORT	2	/* protected by sysfs_lock */
#define FLAG_SYSFS	3	/* exported via /sys/class/gpio/control */
#define FLAG_ACTIVE_LOW	6	/* value has active low */
#define FLAG_OPEN_DRAIN	7	/* Gpio is open drain type */
#define FLAG_OPEN_SOURCE 8	/* Gpio is open source type */
#define FLAG_USED_AS_IRQ 9	/* GPIO is connected to an IRQ */
#define FLAG_IS_HOGGED	11	/* GPIO is hogged */

	/* Connection label */
	const char		*label;
	/* Name of the GPIO */
	const char		*name;
};
```



## gpio相关接口

### 从设备树获取gpio

#### of_get_gpio

```cpp
static inline int of_get_gpio(struct device_node *np, int index)
{
	return of_get_gpio_flags(np, index, NULL);
}
```

np是设备树节点，index是第几个gpio，返回gpio index

#### of_get_gpio_flags

```cpp
static inline int of_get_gpio_flags(struct device_node *np, int index,
		      enum of_gpio_flags *flags)
{
	return of_get_named_gpio_flags(np, "gpios", index, flags);
}
```

同上，会多获取一个flag，也就是cell的第二个项

```cpp

int of_get_named_gpio_flags(struct device_node *np, const char *list_name,
			    int index, enum of_gpio_flags *flags)
{
	struct gpio_desc *desc;

	desc = of_get_named_gpiod_flags(np, list_name, index, flags);

	if (IS_ERR(desc))
		return PTR_ERR(desc);
	else
		return desc_to_gpio(desc);//4
}
EXPORT_SYMBOL(of_get_named_gpio_flags);

struct gpio_desc *of_get_named_gpiod_flags(struct device_node *np,
		     const char *propname, int index, enum of_gpio_flags *flags)
{
	struct of_phandle_args gpiospec;
	struct gpio_chip *chip;
	struct gpio_desc *desc;
	int ret;

	ret = of_parse_phandle_with_args(np, propname, "#gpio-cells", index,
					 &gpiospec);//1
	if (ret) {
		pr_debug("%s: can't parse '%s' property of node '%s[%d]'\n",
			__func__, propname, np->full_name, index);
		return ERR_PTR(ret);
	}

	chip = of_find_gpiochip_by_xlate(&gpiospec);//2
	if (!chip) {
		desc = ERR_PTR(-EPROBE_DEFER);
		goto out;
	}

	desc = of_xlate_and_get_gpiod_flags(chip, &gpiospec, flags);//3
	if (IS_ERR(desc))
		goto out;

	pr_debug("%s: parsed '%s' property of node '%s[%d]' - status (%d)\n",
		 __func__, propname, np->full_name, index,
		 PTR_ERR_OR_ZERO(desc));

out:
	of_node_put(gpiospec.np);

	return desc;
}
```

1. of_parse_phandle_with_args解析这个设备节点，并将解析结果放入gpiospec（of_phandle_args结构体）：

   ```cpp
   struct of_phandle_args {
   	struct device_node *np;
   	int args_count;
   	uint32_t args[MAX_PHANDLE_ARGS];
   };
   ```

   其中np是设备节点，这个节点是所申请的这个gpio来自的gpio控制器节点如：

   ```shell
   gpios = <&gpio1 23 GPIO_ACTIVE_HIGH>;
   ```

   这里np将指向gpio1这个节点，而23,GPIO_ACTIVE_HIGH将会作为参数被填充到args,参数个数为args_count。

2. of_find_gpiochip_by_xlate函数根据of_phandle_args参数，遍历gpio_devices，找到gpio_device->chip，满足`chip->gpiodev->dev.of_node == gpiospec->np && chip->of_xlate(chip, gpiospec, NULL) >= 0;`的那个gpio_device，并返回其gpio_chip

3. of_xlate_and_get_gpiod_flags调用chip->of_xlate函数根据of_phandle_args找到这个gpio在这个gpio_chip中的偏移offset，并返回gpio_device->descs[offset]；

   imx的of_xlate函数使用了内核通用的接口：

   ```cpp
   int of_gpio_simple_xlate(struct gpio_chip *gc,
   			 const struct of_phandle_args *gpiospec, u32 *flags)
   {
   	/*
   	 * We're discouraging gpio_cells < 2, since that way you'll have to
   	 * write your own xlate function (that will have to retrieve the GPIO
   	 * number and the flags from a single gpio cell -- this is possible,
   	 * but not recommended).
   	 */
   	if (gc->of_gpio_n_cells < 2) {
   		WARN_ON(1);
   		return -EINVAL;
   	}
   
   	if (WARN_ON(gpiospec->args_count < gc->of_gpio_n_cells))
   		return -EINVAL;
   
   	if (gpiospec->args[0] >= gc->ngpio)
   		return -EINVAL;
   
   	if (flags)
   		*flags = gpiospec->args[1];
   
   	return gpiospec->args[0];
   }
   ```

   这个函数只是简单的返回gpiospec->arg[0]这个参数，对`gpios = <&gpio1 23 GPIO_ACTIVE_HIGH>;`这个节点来说，这个值就是23

4. 最后调用desc_to_gpio函数将gpio_desc转为绝对偏移offset并返回（`return desc->gdev->base + (desc - &desc->gdev->descs[0]);`)，这个offset就是gpio index，如果有gpio1,gpio2两个控制器，每个控制器有32个gpio，那么`gpios = <&gpio2 23 GPIO_ACTIVE_HIGH>;`对这个节点，它在自己的gpio chip中的偏移是23，而绝对偏移就是32+23 = 55。

### gpio转换

#### gpio_to_desc

```cpp
struct gpio_desc *gpio_to_desc(unsigned gpio)
{
	struct gpio_device *gdev;
	unsigned long flags;

	spin_lock_irqsave(&gpio_lock, flags);

	list_for_each_entry(gdev, &gpio_devices, list) {//1
		if (gdev->base <= gpio &&
		    gdev->base + gdev->ngpio > gpio) {
			spin_unlock_irqrestore(&gpio_lock, flags);
			return &gdev->descs[gpio - gdev->base];
		}
	}

	spin_unlock_irqrestore(&gpio_lock, flags);

	if (!gpio_is_valid(gpio))
		WARN(1, "invalid GPIO %d\n", gpio);

	return NULL;
}
```

1. 遍历gpio_devices，找到满足条件的gpio_device，并返回对应的gpio_desc

### gpio控制

```cpp
int gpiod_direction_output(struct gpio_desc *desc, int value);
int gpiod_direction_input(struct gpio_desc *desc);
void gpiod_set_value(struct gpio_desc *desc, int value);
int gpiod_get_value(const struct gpio_desc *desc);
void gpiod_set_value_cansleep(struct gpio_desc *desc, int value);
int gpiod_get_value_cansleep(const struct gpio_desc *desc);

static inline int gpio_direction_input(unsigned gpio);
static inline int gpio_direction_output(unsigned gpio, int value);
#define gpio_get_value  __gpio_get_value
#define gpio_set_value  __gpio_set_value
#define gpio_cansleep   __gpio_cansleep

```

这些接口最终都会根据gpio_desc找到对应的gpio_device的gpio_chip，调用gpio_chip中实现的硬件相关函数，对gpio控制器进行操作

## 数据结构

![image-20230622173710694](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230622173710694.png)

