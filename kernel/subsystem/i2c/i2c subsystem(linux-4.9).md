# i2c subsystem(linux-4.9)

### /dev设备创建与fops

看驱动习惯先找fops，VFS提供文件节点给用户层，用户层的操作一定会调用fops

`i2c-dev.c`

```cpp
static int __init i2c_dev_init(void)
{
	int res;

	printk(KERN_INFO "i2c /dev entries driver\n");

	res = register_chrdev_region(MKDEV(I2C_MAJOR, 0), I2C_MINORS, "i2c");//注册设备号
	if (res)
		goto out;

	i2c_dev_class = class_create(THIS_MODULE, "i2c-dev");
	if (IS_ERR(i2c_dev_class)) {
		res = PTR_ERR(i2c_dev_class);
		goto out_unreg_chrdev;
	}
	i2c_dev_class->dev_groups = i2c_groups;

	/* Keep track of adapters which will be added or removed later */
	res = bus_register_notifier(&i2c_bus_type, &i2cdev_notifier);//注册通知链回调函数，统一设备模型中，device_register中如果dev绑定了某个bus，会执行bus注册的通知链回调函数。
	if (res)
		goto out_unreg_class;

	/* Bind to already existing adapters right away */
	i2c_for_each_dev(NULL, i2cdev_attach_adapter);//遍历i2c总线的设备链表，为所有设备类型为adapter的设备调用i2cdev_attach_adapter，该函数绑定fops，调用device_create注册设备节点，用户空间呈现为/dev/i2c-%d。通知链中的回调函数就是这个。

	return 0;

out_unreg_class:
	class_destroy(i2c_dev_class);
out_unreg_chrdev:
	unregister_chrdev_region(MKDEV(I2C_MAJOR, 0), I2C_MINORS);
out:
	printk(KERN_ERR "%s: Driver Initialisation failed\n", __FILE__);
	return res;
}
```

注册到通知链的结构体和回调函数

```cpp
static struct notifier_block i2cdev_notifier = {
	.notifier_call = i2cdev_notifier_call,
};
```



```cpp
static int i2cdev_notifier_call(struct notifier_block *nb, unsigned long action,
			 void *data)
{
	struct device *dev = data;

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE://通知有两种分别是ADD和DEL分别代表注册和注销
		return i2cdev_attach_adapter(dev, NULL);
	case BUS_NOTIFY_DEL_DEVICE:
		return i2cdev_detach_adapter(dev, NULL);
	}

	return 0;
}
```

`i2cdev_attach_adapter`函数会在device_register中被调用，粗略来看，创建了设备节点，并绑定了fops，后面的i2c设备的操作一定是通过fops进行的，其他的东西后面看。

```cpp
static int i2cdev_attach_adapter(struct device *dev, void *dummy)
{
	struct i2c_adapter *adap;
	struct i2c_dev *i2c_dev;
	int res;

	if (dev->type != &i2c_adapter_type)
		return 0;
	adap = to_i2c_adapter(dev);

	i2c_dev = get_free_i2c_dev(adap);
	if (IS_ERR(i2c_dev))
		return PTR_ERR(i2c_dev);

	cdev_init(&i2c_dev->cdev, &i2cdev_fops);//bind fops
	i2c_dev->cdev.owner = THIS_MODULE;
	res = cdev_add(&i2c_dev->cdev, MKDEV(I2C_MAJOR, adap->nr), 1);
	if (res)
		goto error_cdev;

	/* register this i2c device with the driver core */
	i2c_dev->dev = device_create(i2c_dev_class, &adap->dev,
				     MKDEV(I2C_MAJOR, adap->nr), NULL,
				     "i2c-%d", adap->nr);//create device node
	if (IS_ERR(i2c_dev->dev)) {
		res = PTR_ERR(i2c_dev->dev);
		goto error;
	}

	pr_debug("i2c-dev: adapter [%s] registered as minor %d\n",
		 adap->name, adap->nr);
	return 0;
error:
	cdev_del(&i2c_dev->cdev);
error_cdev:
	put_i2c_dev(i2c_dev);
	return res;
}
```



### 硬件资源

内核对外设资源初始化的函数调用链：

`start_kernel()->setup_arch()->MACHINE_START`

对s3c24xx系列CPU，其mach文件为arch\arm\mach-s3c23xx\mach-smdk2440.c

```cpp
MACHINE_START(S3C2440, "SMDK2440")
	/* Maintainer: Ben Dooks <ben-linux@fluff.org> */
	.atag_offset	= 0x100,

	.init_irq	= s3c2440_init_irq,//中断初始化
	.map_io		= smdk2440_map_io,//IO初始化
	.init_machine	= smdk2440_machine_init,//外设初始化
	.init_time	= smdk2440_init_time,//时钟初始化
MACHINE_END
```

这里面的函数会在内核启动时被调用，其中init_machine的调用不同于其他：

```cpp
static int __init customize_machine(void)
{
	/*
	 * customizes platform devices, or adds new ones
	 * On DT based machines, we fall back to populating the
	 * machine from the device tree, if no callback is provided,
	 * otherwise we would always need an init_machine callback.
	 */
	if (machine_desc->init_machine)
		machine_desc->init_machine();

	return 0;
}
arch_initcall(customize_machine);
```

关于`arch_initcall:`  `ref:https://www.cnblogs.com/downey-blog/p/10486653.html`

针对`smdk2440_machine_init`函数：

```cpp
static void __init smdk2440_machine_init(void)
{
	s3c24xx_fb_set_platdata(&smdk2440_fb_info);
	s3c_i2c0_set_platdata(NULL);//

	platform_add_devices(smdk2440_devices, ARRAY_SIZE(smdk2440_devices));//
	smdk_machine_init();
}
```

`s3c_i2c0_set_platdata`函数：

```cpp
void __init s3c_i2c0_set_platdata(struct s3c2410_platform_i2c *pd)
{
	struct s3c2410_platform_i2c *npd;

	if (!pd) {
		pd = &default_i2c_data;
		pd->bus_num = 0;
	}

	npd = s3c_set_platdata(pd, sizeof(struct s3c2410_platform_i2c),
			       &s3c_device_i2c0);//将default_i2c_data放入到s3c_device_i2c0的dev.platform_data域

	if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_i2c0_cfg_gpio;
}
```

`platform_add_devices`会依次将`smdk2440_devices`中的`devices`加入`paltform`总线

```cpp
static struct platform_device *smdk2440_devices[] __initdata = {
	&s3c_device_ohci,
	&s3c_device_lcd,
	&s3c_device_wdt,
	&s3c_device_i2c0,//
	&s3c_device_iis,
};
```

其中包含`i2c `设备

```cpp
static struct resource s3c_i2c0_resource[] = {
	[0] = DEFINE_RES_MEM(S3C_PA_IIC, SZ_4K),//内存申请
	[1] = DEFINE_RES_IRQ(IRQ_IIC),//中断线
};

struct platform_device s3c_device_i2c0 = {
	.name		= "s3c2410-i2c",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_i2c0_resource),
	.resource	= s3c_i2c0_resource,//i2c需要的硬件资源
};

struct s3c2410_platform_i2c default_i2c_data __initdata = {
	.flags		= 0,
	.slave_addr	= 0x10,//从机地址
	.frequency	= 100*1000,
	.sda_delay	= 100,//SDA总线延时
};
```

### 驱动

`/drivers/i2c/busses/i2c-s3c2410.c`

```cpp
static int __init i2c_adap_s3c_init(void)
{
	return platform_driver_register(&s3c24xx_i2c_driver);
}
```

`platform_driver_register`会将`platform_driver`注册到`platform`总线上，由统一设备模型，最后会根据这根总线的匹配规则对总线上的`driver`和`device`进行匹配，匹配成功调用`platform_driver`的`probe`函数。由第一步可知，使用`mach-smdk2440`板级文件，会默认注册一个`i2c`设备到platform总线上。

`platform`总线的匹配规则如下：

```cpp
static int platform_match(struct device *dev, struct device_driver *drv)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct platform_driver *pdrv = to_platform_driver(drv);

	/* When driver_override is set, only bind to the matching driver */
	if (pdev->driver_override)
		return !strcmp(pdev->driver_override, drv->name);

	/* Attempt an OF style match first */
	if (of_driver_match_device(dev, drv))
		return 1;

	/* Then try ACPI style match */
	if (acpi_driver_match_device(dev, drv))
		return 1;

	/* Then try to match against the id table */
	if (pdrv->id_table)
		return platform_match_id(pdrv->id_table, pdev) != NULL;

	/* fall-back to driver name match */
	return (strcmp(pdev->name, drv->name) == 0);
}
```

此处注册的`platform_driver`如下：

```cpp
static struct platform_driver s3c24xx_i2c_driver = {
	.probe		= s3c24xx_i2c_probe,
	.remove		= s3c24xx_i2c_remove,
	.id_table	= s3c24xx_driver_ids,
	.driver		= {
		.name	= "s3c-i2c",
		.pm	= S3C24XX_DEV_PM_OPS,
		.of_match_table = of_match_ptr(s3c24xx_i2c_match),
	},
};
```

一般使用`platform`总线，我们优先使用的匹配规则是`"driver->name"和"device->name"`相同，则调用`platform_driver`的`probe`函数，但是此处，对比`platform_device->device->name`和`platform_driver->driver->name`并不一样，此处使用的是platform总线的`if (of_driver_match_device(dev, drv))`匹配规则。而此处，`platform_driver`的`of_match_table`类似为`platform_device`提供了一种限制，`of_match_table`中的`s3c24xx_i2c_match`的`.compatible`限制了`s3c2410`支持的`i2c`总线类型，不过这种限制只是针对了Linux内核自带的`s32410-i2c`的驱动。自己去写的话可以自定义匹配规则。

### 匹配

```cpp
static int s3c24xx_i2c_probe(struct platform_device *pdev)
{
	struct s3c24xx_i2c *i2c;
	struct s3c2410_platform_i2c *pdata = NULL;
	struct resource *res;
	int ret;

	if (!pdev->dev.of_node) {//不是在dts上初始化的设备
		pdata = dev_get_platdata(&pdev->dev);//第一步s3c_i2c0_set_platdata函数会为dev->paltform_data赋值
		if (!pdata) {
			dev_err(&pdev->dev, "no platform data\n");
			return -EINVAL;
		}
	}
	//为s3c24xx i2c设备分配内存
	i2c = devm_kzalloc(&pdev->dev, sizeof(struct s3c24xx_i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;
	//为plateform i2c分配内存
	i2c->pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!i2c->pdata)
		return -ENOMEM;

	i2c->quirks = s3c24xx_get_device_quirks(pdev);
	i2c->sysreg = ERR_PTR(-ENOENT);
	if (pdata)
		memcpy(i2c->pdata, pdata, sizeof(*pdata));//copy
	else
		s3c24xx_i2c_parse_dt(pdev->dev.of_node, i2c);//data from dts

	strlcpy(i2c->adap.name, "s3c2410-i2c", sizeof(i2c->adap.name));//adapter name
	i2c->adap.owner = THIS_MODULE;
	i2c->adap.algo = &s3c24xx_i2c_algorithm;//important
	i2c->adap.retries = 2;
	i2c->adap.class = I2C_CLASS_DEPRECATED;
	i2c->tx_setup = 50;

	init_waitqueue_head(&i2c->wait);//初始化等待队列

	/* find the clock and enable it */
	i2c->dev = &pdev->dev;
	i2c->clk = devm_clk_get(&pdev->dev, "i2c");//i2c clock
	if (IS_ERR(i2c->clk)) {
		dev_err(&pdev->dev, "cannot get clock\n");
		return -ENOENT;
	}

	dev_dbg(&pdev->dev, "clock source %p\n", i2c->clk);

	/* map the registers */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);//get mem resource
	i2c->regs = devm_ioremap_resource(&pdev->dev, res);//request mem source

	if (IS_ERR(i2c->regs))
		return PTR_ERR(i2c->regs);

	dev_dbg(&pdev->dev, "registers %p (%p)\n",
		i2c->regs, res);

	/* setup info block for the i2c core */
	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = &pdev->dev;
	i2c->pctrl = devm_pinctrl_get_select_default(i2c->dev);

	/* inititalise the i2c gpio lines */
	if (i2c->pdata->cfg_gpio)
		i2c->pdata->cfg_gpio(to_platform_device(i2c->dev));//init i2c gpio
	else if (IS_ERR(i2c->pctrl) && s3c24xx_i2c_parse_dt_gpio(i2c))
		return -EINVAL;

	/* initialise the i2c controller */
	ret = clk_prepare_enable(i2c->clk);
	if (ret) {
		dev_err(&pdev->dev, "I2C clock enable failed\n");
		return ret;
	}

	ret = s3c24xx_i2c_init(i2c);//init i2c 
	clk_disable(i2c->clk);
	if (ret != 0) {
		dev_err(&pdev->dev, "I2C controller init failed\n");
		clk_unprepare(i2c->clk);
		return ret;
	}

	/*
	 * find the IRQ for this unit (note, this relies on the init call to
	 * ensure no current IRQs pending
	 */
	if (!(i2c->quirks & QUIRK_POLL)) {
		i2c->irq = ret = platform_get_irq(pdev, 0);
		if (ret <= 0) {
			dev_err(&pdev->dev, "cannot find IRQ\n");
			clk_unprepare(i2c->clk);
			return ret;
		}

		ret = devm_request_irq(&pdev->dev, i2c->irq, s3c24xx_i2c_irq,
				       0, dev_name(&pdev->dev), i2c);//request irq
		if (ret != 0) {
			dev_err(&pdev->dev, "cannot claim IRQ %d\n", i2c->irq);
			clk_unprepare(i2c->clk);
			return ret;
		}
	}

	ret = s3c24xx_i2c_register_cpufreq(i2c);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register cpufreq notifier\n");
		clk_unprepare(i2c->clk);
		return ret;
	}

	/*
	 * Note, previous versions of the driver used i2c_add_adapter()
	 * to add the bus at any number. We now pass the bus number via
	 * the platform data, so if unset it will now default to always
	 * being bus 0.
	 */
	i2c->adap.nr = i2c->pdata->bus_num;//adapter.nr = i2c->pdata->bus_num，platform_device是静态初始化的bus_num=0
	i2c->adap.dev.of_node = pdev->dev.of_node;

	platform_set_drvdata(pdev, i2c);//pdev->dev->driver_data = i2c

	pm_runtime_enable(&pdev->dev);//power management

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		pm_runtime_disable(&pdev->dev);
		s3c24xx_i2c_deregister_cpufreq(i2c);
		clk_unprepare(i2c->clk);
		return ret;
	}

	dev_info(&pdev->dev, "%s: S3C I2C adapter\n", dev_name(&i2c->adap.dev));
	return 0;
}
```

`s3c2410`的`i2c`基于`platform`总线匹配成功后，调用`driver->probe`函数。此函数初始化`i2c`设备，包括等待队列，中断，内存IO资源...，并初始化一个封装的`s3c24xx_i2c`中`adapter`的`algo`。最后调用`i2c_add_numbered_adapter`。

```cpp
int i2c_add_numbered_adapter(struct i2c_adapter *adap)
{
	if (adap->nr == -1) /* -1 means dynamically assign bus id *///动态注册走上面
		return i2c_add_adapter(adap);

	return __i2c_add_numbered_adapter(adap);
}
```

下面就是静态注册了

```cpp
static int __i2c_add_numbered_adapter(struct i2c_adapter *adap)
{
	int id;

	mutex_lock(&core_lock);
	id = idr_alloc(&i2c_adapter_idr, adap, adap->nr, adap->nr + 1, GFP_KERNEL);
	mutex_unlock(&core_lock);
	if (WARN(id < 0, "couldn't get idr"))
		return id == -ENOSPC ? -EBUSY : id;

	return i2c_register_adapter(adap);
}
```

动态注册

```cpp
int i2c_add_adapter(struct i2c_adapter *adapter)
{
	struct device *dev = &adapter->dev;
	int id;

	if (dev->of_node) {
		id = of_alias_get_id(dev->of_node, "i2c");
		if (id >= 0) {
			adapter->nr = id;
			return __i2c_add_numbered_adapter(adapter);
		}
	}

	mutex_lock(&core_lock);
	id = idr_alloc(&i2c_adapter_idr, adapter,
		       __i2c_first_dynamic_bus_num, 0, GFP_KERNEL);
	mutex_unlock(&core_lock);
	if (WARN(id < 0, "couldn't get idr"))
		return id;

	adapter->nr = id;

	return i2c_register_adapter(adapter);
}
```



注册适配器

```cpp
static int i2c_register_adapter(struct i2c_adapter *adap)
{
	int res = -EINVAL;

	/* Can't register until after driver model init */
	if (WARN_ON(!is_registered)) {
		res = -EAGAIN;
		goto out_list;
	}

	/* Sanity checks */
	if (WARN(!adap->name[0], "i2c adapter has no name"))//adapter name 必须存在
		goto out_list;

	if (!adap->algo) {//adapter algo必须存在
		pr_err("adapter '%s': no algo supplied!\n", adap->name);
		goto out_list;
	}

	if (!adap->lock_ops)
		adap->lock_ops = &i2c_adapter_lock_ops;//i2c bus mutex

	rt_mutex_init(&adap->bus_lock);//init bus lock	
	rt_mutex_init(&adap->mux_lock);//init mux lock
	mutex_init(&adap->userspace_clients_lock);
	INIT_LIST_HEAD(&adap->userspace_clients);

	/* Set default timeout to 1 second if not already set */
	if (adap->timeout == 0)
		adap->timeout = HZ;

	dev_set_name(&adap->dev, "i2c-%d", adap->nr);//set adapter name
	adap->dev.bus = &i2c_bus_type;//set adapter dev bus
	adap->dev.type = &i2c_adapter_type;
	res = device_register(&adap->dev);//device_register 最后会通知i2c_bus，调用回调函数ADD，绑定fops并创建设备节点
	if (res) {
		pr_err("adapter '%s': can't register device (%d)\n", adap->name, res);
		goto out_list;
	}

	dev_dbg(&adap->dev, "adapter [%s] registered\n", adap->name);

	pm_runtime_no_callbacks(&adap->dev);
	pm_suspend_ignore_children(&adap->dev, true);
	pm_runtime_enable(&adap->dev);

#ifdef CONFIG_I2C_COMPAT
	res = class_compat_create_link(i2c_adapter_compat_class, &adap->dev,
				       adap->dev.parent);
	if (res)
		dev_warn(&adap->dev,
			 "Failed to create compatibility class link\n");
#endif

	i2c_init_recovery(adap);

	/* create pre-declared device nodes */
	of_i2c_register_devices(adap);//register device from device tree
	i2c_acpi_register_devices(adap);//register device from ...
	i2c_acpi_install_space_handler(adap);//register device ...

	if (adap->nr < __i2c_first_dynamic_bus_num)
		i2c_scan_static_board_info(adap);//register device from board info

	/* Notify drivers */
	mutex_lock(&core_lock);
	bus_for_each_drv(&i2c_bus_type, NULL, adap, __process_new_adapter);
	mutex_unlock(&core_lock);

	return 0;

out_list:
	mutex_lock(&core_lock);
	idr_remove(&i2c_adapter_idr, adap->nr);
	mutex_unlock(&core_lock);
	return res;
}
```

适配器并不是一个真正的设备，设备依附于适配器。以下是`mach-mini2440.c`中静态注册i2c设备的部分，`mach-smdk2440.c`并没有静态初始化`i2c`设备。

`mach-mini2440.c`

```cpp
static void __init mini2440_init(void)
{
    ...
	s3c_i2c0_set_platdata(NULL);

	i2c_register_board_info(0, mini2440_i2c_devs,
				ARRAY_SIZE(mini2440_i2c_devs));//

	platform_add_devices(mini2440_devices, ARRAY_SIZE(mini2440_devices));
	...
}
```

`mini2440_i2c_devs`

```cpp
static struct i2c_board_info mini2440_i2c_devs[] __initdata = {
	{
		I2C_BOARD_INFO("24c08", 0x50),
		.platform_data = &at24c08,
	},
};
static struct at24_platform_data at24c08 = {
	.byte_len	= SZ_8K / 8,
	.page_size	= 16,
};
```

`i2c_register_board_info`

```cpp
int i2c_register_board_info(int busnum, struct i2c_board_info const *info, unsigned len)
{
	int status;

	down_write(&__i2c_board_lock);

	/* dynamic bus numbers will be assigned after the last static one */
    //动态总线号在最后一个静态总线号后分配
	if (busnum >= __i2c_first_dynamic_bus_num)
		__i2c_first_dynamic_bus_num = busnum + 1;//bus_num=0

	for (status = 0; len; len--, info++) {//遍历i2c_board_info数组
		struct i2c_devinfo	*devinfo;

		devinfo = kzalloc(sizeof(*devinfo), GFP_KERNEL);//为数组元素分配内存
		if (!devinfo) {
			pr_debug("i2c-core: can't register boardinfo!\n");
			status = -ENOMEM;
			break;
		}

		devinfo->busnum = busnum;//记录静态总线号
		devinfo->board_info = *info;//绑定设备
		list_add_tail(&devinfo->list, &__i2c_board_list);//加入全局链表
	}

	up_write(&__i2c_board_lock);

	return status;
}
```

回到`i2c_scan_static_board_info`函数

```cpp
static void i2c_scan_static_board_info(struct i2c_adapter *adapter)
{
	struct i2c_devinfo	*devinfo;

	down_read(&__i2c_board_lock);
	list_for_each_entry(devinfo, &__i2c_board_list, list) {//扫描全局链表
		if (devinfo->busnum == adapter->nr//nr在probe中被赋值为0，busnum由上可知是0
				&& !i2c_new_device(adapter,
						&devinfo->board_info))
			dev_err(&adapter->dev,
				"Can't create device at 0x%02x\n",
				devinfo->board_info.addr);
	}
	up_read(&__i2c_board_lock);
}
```

调用`i2c_new_device`函数

```cpp
struct i2c_client *
i2c_new_device(struct i2c_adapter *adap, struct i2c_board_info const *info)
{
	struct i2c_client	*client;
	int			status;

	client = kzalloc(sizeof *client, GFP_KERNEL);//新建i2c_client对象
	if (!client)
		return NULL;

	client->adapter = adap;//i2c_clinet绑定adapter

	client->dev.platform_data = info->platform_data;//device data

	if (info->archdata)
		client->dev.archdata = *info->archdata;//device data

	client->flags = info->flags;//device data
	client->addr = info->addr;//device data
	client->irq = info->irq;//device data

	strlcpy(client->name, info->type, sizeof(client->name));//device data

	status = i2c_check_addr_validity(client->addr, client->flags);
	if (status) {
		dev_err(&adap->dev, "Invalid %d-bit I2C address 0x%02hx\n",
			client->flags & I2C_CLIENT_TEN ? 10 : 7, client->addr);
		goto out_err_silent;
	}

	/* Check for address business */
	status = i2c_check_addr_busy(adap, i2c_encode_flags_to_addr(client));
	if (status)
		goto out_err;

	client->dev.parent = &client->adapter->dev;//设置client的parent为adapter
	client->dev.bus = &i2c_bus_type;//绑定到i2c总线上
	client->dev.type = &i2c_client_type;//类型为client
	client->dev.of_node = info->of_node;
	client->dev.fwnode = info->fwnode;

	i2c_dev_set_name(adap, client);
	status = device_register(&client->dev);//register device
	if (status)
		goto out_err;

	dev_dbg(&adap->dev, "client [%s] registered with bus id %s\n",
		client->name, dev_name(&client->dev));

	return client;

out_err:
	dev_err(&adap->dev,
		"Failed to register i2c client %s at 0x%02x (%d)\n",
		client->name, client->addr, status);
out_err_silent:
	kfree(client);
	return NULL;
}
```



`i2c_register_adapter`中为`adapter`调用了`device_register`，后面又为`client`调用了`device_register`。

前面的`platform_device`和`platform_driver`