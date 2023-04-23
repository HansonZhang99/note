# input subsystem(基于linux 4.9内核)

## 总览，input分层

![image-20230424013041226](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013041226.png)

## 核心层

input向内核注册input类，建立/sys/bus/input/devices和/sys/bus/input/handlers文件，并申请主设备号为13的设备为input设备，相当于input的初始化，从内核获取一些基本的资源

`/drivers/input/input.c`

```cpp
static int __init input_init(void)
{
	int err;

	err = class_register(&input_class);//注册input类
	if (err) {
		pr_err("unable to register input_dev class\n");
		return err;
	}
/*
struct class input_class = {
	.name		= "input",
	.devnode	= input_devnode,
};
*/
	err = input_proc_init();//创建/sys/bus/input/devices和/sys/bus/input/handlers
	if (err)
		goto fail1;
/*
static int __init input_proc_init(void)
{
	...
	proc_bus_input_dir = proc_mkdir("bus/input", NULL);
	entry = proc_create("devices", 0, proc_bus_input_dir,
			    &input_devices_fileops);
	entry = proc_create("handlers", 0, proc_bus_input_dir,
			    &input_handlers_fileops);
	...
}*/
	err = register_chrdev_region(MKDEV(INPUT_MAJOR, 0),
				     INPUT_MAX_CHAR_DEVICES, "input");//静态注册设备号，主设备号13次设备号0-1024,设备名input
	if (err) {
		pr_err("unable to register char major %d", INPUT_MAJOR);
		goto fail2;
	}

	return 0;

 fail2:	input_proc_exit();
 fail1:	class_unregister(&input_class);
	return err;
}
```

## 事件处理层

以evdev为例，相关的还有mousedev等事件处理

`evdev handler`的初始化`drivers/input/evdev.c`

```cpp
static int __init evdev_init(void)
{
	return input_register_handler(&evdev_handler);
}

static void __exit evdev_exit(void)
{
	input_unregister_handler(&evdev_handler);
}

module_init(evdev_init);
module_exit(evdev_exit);
```

`input_register_handler`向内核注册一个事件处理器，事件处理器原型为`struct input_handler`

#### evdev_handler

```cpp
static struct input_handler evdev_handler = {
	.event		= evdev_event,
	.events		= evdev_events,
	.connect	= evdev_connect,
	.disconnect	= evdev_disconnect,
	.legacy_minors	= true,
	.minor		= EVDEV_MINOR_BASE,
	.name		= "evdev",
	.id_table	= evdev_ids,
};
//事件处理器需要实现事件处理，连接和断开连接方法，并实现id_tables用于列举此事件处理器支持的设备，这里表示支持所有设备
static const struct input_device_id evdev_ids[] = {
	{ .driver_info = 1 },	/* Matches all devices */
	{ },			/* Terminating zero entry */
};
struct input_handler {

	void *private;

	void (*event)(struct input_handle *handle, unsigned int type, unsigned int code, int value);
	void (*events)(struct input_handle *handle,
		       const struct input_value *vals, unsigned int count);
	bool (*filter)(struct input_handle *handle, unsigned int type, unsigned int code, int value);
	bool (*match)(struct input_handler *handler, struct input_dev *dev);
	int (*connect)(struct input_handler *handler, struct input_dev *dev, const struct input_device_id *id);
	void (*disconnect)(struct input_handle *handle);
	void (*start)(struct input_handle *handle);

	bool legacy_minors;
	int minor;
	const char *name;

	const struct input_device_id *id_table;

	struct list_head	h_list;//链表头
	struct list_head	node;//链表节点
};
```



#### input_register_handler

将`handler`加入到内核全局链表`input_handler_list`,并遍历`input_device_list`链表，通过`handler`的`id_tables`与设备链表上的设备进行匹配，匹配成功就会调用`handler`的`connect`函数。

```cpp
int input_register_handler(struct input_handler *handler)
{
	struct input_dev *dev;
	int error;

	error = mutex_lock_interruptible(&input_mutex);//互斥锁
	if (error)
		return error;

	INIT_LIST_HEAD(&handler->h_list);//初始化handler的h_list链表头

	list_add_tail(&handler->node, &input_handler_list);//将handler加入input_handler_list全局链表
	list_for_each_entry(dev, &input_dev_list, node)//遍历input_dev_list链表，将次链表上的设备与handler进行match,调用input_attach_handler函数
		input_attach_handler(dev, handler);//match函数

	input_wakeup_procfs_readers();

	mutex_unlock(&input_mutex);//解锁
	return 0;
}
```

#### input_attach_handler

```cpp
static int input_attach_handler(struct input_dev *dev, struct input_handler *handler)
{
	const struct input_device_id *id;
	int error;

	id = input_match_device(handler, dev);//此处是handler和dev的匹配规则，成功匹配才会继续向下执行
	if (!id)
		return -ENODEV;

	error = handler->connect(handler, dev, id);//匹配成功则调用handler->connect函数
	if (error && error != -ENODEV)
		pr_err("failed to attach handler %s to device %s, error: %d\n",
		       handler->name, kobject_name(&dev->dev.kobj), error);

	return error;
}
```

#### input_match_device匹配成功返回handler->id_table中的id

```cpp
static const struct input_device_id *input_match_device(struct input_handler *handler,
							struct input_dev *dev)
{
	const struct input_device_id *id;

	for (id = handler->id_table; id->flags || id->driver_info; id++) {
//遍历handler->id_table,对有效的id进行匹配,从input_register_handler(&evdev_handler);中的evdev_handler可以看出此处不执行任何if判断，直接在最后返回id
		if (id->flags & INPUT_DEVICE_ID_MATCH_BUS)
			if (id->bustype != dev->id.bustype)
				continue;

		if (id->flags & INPUT_DEVICE_ID_MATCH_VENDOR)
			if (id->vendor != dev->id.vendor)
				continue;

		if (id->flags & INPUT_DEVICE_ID_MATCH_PRODUCT)
			if (id->product != dev->id.product)
				continue;

		if (id->flags & INPUT_DEVICE_ID_MATCH_VERSION)
			if (id->version != dev->id.version)
				continue;

		if (!bitmap_subset(id->evbit, dev->evbit, EV_MAX))
			continue;

		if (!bitmap_subset(id->keybit, dev->keybit, KEY_MAX))
			continue;

		if (!bitmap_subset(id->relbit, dev->relbit, REL_MAX))
			continue;

		if (!bitmap_subset(id->absbit, dev->absbit, ABS_MAX))
			continue;

		if (!bitmap_subset(id->mscbit, dev->mscbit, MSC_MAX))
			continue;

		if (!bitmap_subset(id->ledbit, dev->ledbit, LED_MAX))
			continue;

		if (!bitmap_subset(id->sndbit, dev->sndbit, SND_MAX))
			continue;

		if (!bitmap_subset(id->ffbit, dev->ffbit, FF_MAX))
			continue;

		if (!bitmap_subset(id->swbit, dev->swbit, SW_MAX))
			continue;

		if (!handler->match || handler->match(handler, dev))
			return id;//此处返回id
	}

	return NULL;
}
```



#### 匹配成功则调用handler->connect函数

#### evdev_connect

当`handler`与`device`匹配成功时，会触发`evdev_connect`函数，此函数构造一个`struct evdev`结构体描述一个已经匹配`handler`的`device`。

这里会调用通用的字符设备注册流程：

1.为设备分配主次设备号`MKDEV`

2.设置设备所属类`input_class`

3.初始化抽象设备`device_initialize`

4.字符设备初始化`cdev_init`

5.字符设备注册`cdev_add`

6.通用设备注册`device_add`

```cpp
static int evdev_connect(struct input_handler *handler, struct input_dev *dev,
			 const struct input_device_id *id)
{
	struct evdev *evdev;
	int minor;
	int dev_no;
	int error;

	minor = input_get_new_minor(EVDEV_MINOR_BASE, EVDEV_MINORS, true);//动态从之前注册的设备号集合中获取次设备号
	if (minor < 0) {
		error = minor;
		pr_err("failed to reserve new minor: %d\n", error);
		return error;
	}

	evdev = kzalloc(sizeof(struct evdev), GFP_KERNEL);
	if (!evdev) {
		error = -ENOMEM;
		goto err_free_minor;
	}

	INIT_LIST_HEAD(&evdev->client_list);//初始化client链表
	spin_lock_init(&evdev->client_lock);//初始化client链表对应的锁
	mutex_init(&evdev->mutex);//初始化锁
	init_waitqueue_head(&evdev->wait);//初始化等待队列
	evdev->exist = true;

	dev_no = minor;
	/* Normalize device number if it falls into legacy range */
	if (dev_no < EVDEV_MINOR_BASE + EVDEV_MINORS)
		dev_no -= EVDEV_MINOR_BASE;
	dev_set_name(&evdev->dev, "event%d", dev_no);//设置此设备为event[0-1024]

	evdev->handle.dev = input_get_device(dev);//匹配的input_dev
	evdev->handle.name = dev_name(&evdev->dev);//name
	evdev->handle.handler = handler;//匹配的handler
	evdev->handle.private = evdev;

	evdev->dev.devt = MKDEV(INPUT_MAJOR, minor);//初始化设备号，主+次
	evdev->dev.class = &input_class;//设置设备所属的类,第一步初始化的input_class
	evdev->dev.parent = &dev->dev;//设置设备的父设备为input_dev->dev
	evdev->dev.release = evdev_free;//注销回调函数
	device_initialize(&evdev->dev);//初始化device

	error = input_register_handle(&evdev->handle);
	if (error)
		goto err_free_evdev;

	cdev_init(&evdev->cdev, &evdev_fops);//绑定字符设备操作函数fops
	evdev->cdev.kobj.parent = &evdev->dev.kobj;//设置kobj的parent
	error = cdev_add(&evdev->cdev, evdev->dev.devt, 1);//将cdev结构体添加到系统中
	if (error)
		goto err_unregister_handle;

	error = device_add(&evdev->dev);//注册设备,注册完后会在/dev下产生设备节点
	if (error)
		goto err_cleanup_evdev;

	return 0;

 err_cleanup_evdev:
	evdev_cleanup(evdev);
 err_unregister_handle:
	input_unregister_handle(&evdev->handle);
 err_free_evdev:
	put_device(&evdev->dev);
 err_free_minor:
	input_free_minor(minor);
	return error;
}
```

#### input_register_handle

`evdev->handle`同时保存了input_dev和input_handler以及evdev的信息，input_register_handle将handle放入input_dev和input_handler的链表中

```cpp
int input_register_handle(struct input_handle *handle)
{
	struct input_handler *handler = handle->handler;
	struct input_dev *dev = handle->dev;
	int error;

	/*
	 * We take dev->mutex here to prevent race with
	 * input_release_device().
	 */
	error = mutex_lock_interruptible(&dev->mutex);//锁
	if (error)
		return error;

	/*
	 * Filters go to the head of the list, normal handlers
	 * to the tail.
	 */
	if (handler->filter)
		list_add_rcu(&handle->d_node, &dev->h_list);
	else
		list_add_tail_rcu(&handle->d_node, &dev->h_list);//将handle加入到dev的链表中

	mutex_unlock(&dev->mutex);

	/*
	 * Since we are supposed to be called from ->connect()
	 * which is mutually exclusive with ->disconnect()
	 * we can't be racing with input_unregister_handle()
	 * and so separate lock is not needed here.
	 */
	list_add_tail_rcu(&handle->h_node, &handler->h_list);//将handle加入到handler的链表中

	if (handler->start)//NULL
		handler->start(handle);

	return 0;
}
```

### input device初始化

linux内核只提供框架，具体设备实现需要自己实现，但是内核中也集成了一些常用CPU或者开发板的外设实例。

实例最外层依然是platfrom框架，在platfrom的probe函数中，进行input子系统设备的注册，设备最终会匹配input子系统的handler，最后注册字符设备到内核。

`drivers/input/keyboard/gpio_keys.c`

```cpp
static struct platform_driver gpio_keys_device_driver = {
	.probe		= gpio_keys_probe,
	.remove		= gpio_keys_remove,
	.driver		= {
		.name	= "gpio-keys",//linux kernel: grep可以找到内核已经实现的设备实例
		.pm	= &gpio_keys_pm_ops,
		.of_match_table = of_match_ptr(gpio_keys_of_match),
	}
};

static int __init gpio_keys_init(void)
{
	return platform_driver_register(&gpio_keys_device_driver);//注册platform总线驱动
}

static void __exit gpio_keys_exit(void)
{
	platform_driver_unregister(&gpio_keys_device_driver);
}
```

`arch/arm/mach-s3c24xx/mach-mini2440.c`

```cpp
static struct platform_device mini2440_button_device = {
	.name		= "gpio-keys",
	.id		= -1,
	.dev		= {
		.platform_data	= &mini2440_button_data,
	}
};

static struct gpio_keys_platform_data mini2440_button_data = {
	.buttons	= mini2440_buttons,
	.nbuttons	= ARRAY_SIZE(mini2440_buttons),
};

static struct gpio_keys_button mini2440_buttons[] = {
	{
		.gpio		= S3C2410_GPG(0),		/* K1 */
		.code		= KEY_F1,
		.desc		= "Button 1",
		.active_low	= 1,
	},
	{
		.gpio		= S3C2410_GPG(3),		/* K2 */
		.code		= KEY_F2,
		.desc		= "Button 2",
		.active_low	= 1,
	},
	{
		.gpio		= S3C2410_GPG(5),		/* K3 */
		.code		= KEY_F3,
		.desc		= "Button 3",
		.active_low	= 1,
	},
	{
		.gpio		= S3C2410_GPG(6),		/* K4 */
		.code		= KEY_POWER,
		.desc		= "Power",
		.active_low	= 1,
	},
	{
		.gpio		= S3C2410_GPG(7),		/* K5 */
		.code		= KEY_F5,
		.desc		= "Button 5",
		.active_low	= 1,
	},
#if 0
	/* this pin is also known as TCLK1 and seems to already
	 * marked as "in use" somehow in the kernel -- possibly wrongly */
	{
		.gpio		= S3C2410_GPG(11),	/* K6 */
		.code		= KEY_F6,
		.desc		= "Button 6",
		.active_low	= 1,
	},
#endif
};
```

#### probe: gpio_keys_probe



probe函数根据硬件信息，创建input_dev结构，并调用input_register_device注册input设备，这里需要实现input_dev的open close回调函数

```cpp
static int gpio_keys_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct gpio_keys_platform_data *pdata = dev_get_platdata(dev);//dev->platform_data:  mini2440_button_data
	struct gpio_keys_drvdata *ddata;
	struct input_dev *input;
	size_t size;
	int i, error;
	int wakeup = 0;

	if (!pdata) {
		pdata = gpio_keys_get_devtree_pdata(dev);//device tree
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
	}

	size = sizeof(struct gpio_keys_drvdata) +
			pdata->nbuttons * sizeof(struct gpio_button_data);
	ddata = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!ddata) {
		dev_err(dev, "failed to allocate state\n");
		return -ENOMEM;
	}
/*
struct gpio_keys_drvdata {
	const struct gpio_keys_platform_data *pdata;
	struct input_dev *input;
	struct mutex disable_lock;
	struct gpio_button_data data[0];//arraty size 0根据malloc的大小自动扩展
};
*/
	input = devm_input_allocate_device(dev);//分配input_dev结构，并设置input->dev.parent = dev;
	if (!input) {
		dev_err(dev, "failed to allocate input device\n");
		return -ENOMEM;
	}

	ddata->pdata = pdata;//mini2440_button_data
	ddata->input = input;
	mutex_init(&ddata->disable_lock);

	platform_set_drvdata(pdev, ddata);//pdev->dev->driver_data = ddata
	input_set_drvdata(input, ddata);//input->dev->driver_data = ddata

	input->name = pdata->name ? : pdev->name;
	input->phys = "gpio-keys/input0";
	input->dev.parent = &pdev->dev;
	input->open = gpio_keys_open;//callback
	input->close = gpio_keys_close;//callback

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	/* Enable auto repeat feature of Linux input subsystem */
	if (pdata->rep)//0
		__set_bit(EV_REP, input->evbit);

	for (i = 0; i < pdata->nbuttons; i++) {
		const struct gpio_keys_button *button = &pdata->buttons[i];
		struct gpio_button_data *bdata = &ddata->data[i];

		error = gpio_keys_setup_key(pdev, input, bdata, button);
		if (error)
			return error;

		if (button->wakeup)
			wakeup = 1;
	}

	error = sysfs_create_group(&pdev->dev.kobj, &gpio_keys_attr_group);//sysfs
	if (error) {
		dev_err(dev, "Unable to export keys/switches, error: %d\n",
			error);
		return error;
	}

	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device, error: %d\n",
			error);
		goto err_remove_group;
	}

	device_init_wakeup(&pdev->dev, wakeup);

	return 0;

err_remove_group:
	sysfs_remove_group(&pdev->dev.kobj, &gpio_keys_attr_group);
	return error;
}
```

#### gpio_keys_setup_key



调用`devm_gpio_request_one`申请gpio

调用`gpiod_to_irq`通过gpio获取中断号

调用`INIT_DELAYED_WORK`初始化工作队列

调用`devm_request_any_context_irq`申请中断

````cpp
static int gpio_keys_setup_key(struct platform_device *pdev,
				struct input_dev *input,
				struct gpio_button_data *bdata,
				const struct gpio_keys_button *button)
{
	const char *desc = button->desc ? button->desc : "gpio_keys";
	struct device *dev = &pdev->dev;
	irq_handler_t isr;
	unsigned long irqflags;
	int irq;
	int error;

	bdata->input = input;//
	bdata->button = button;//
	spin_lock_init(&bdata->lock);

	/*
	 * Legacy GPIO number, so request the GPIO here and
	 * convert it to descriptor.
	 */
	if (gpio_is_valid(button->gpio)) {
		unsigned flags = GPIOF_IN;

		if (button->active_low)
			flags |= GPIOF_ACTIVE_LOW;

		error = devm_gpio_request_one(&pdev->dev, button->gpio, flags,
					      desc);//申请GPIO,默认状态为低电平
		if (error < 0) {
			dev_err(dev, "Failed to request GPIO %d, error %d\n",
				button->gpio, error);
			return error;
		}

		bdata->gpiod = gpio_to_desc(button->gpio);
		if (!bdata->gpiod)
			return -EINVAL;

		if (button->debounce_interval) {//mini2440 初始化为0了
			error = gpiod_set_debounce(bdata->gpiod,
					button->debounce_interval * 1000);//防抖
			/* use timer if gpiolib doesn't provide debounce */
			if (error < 0)
				bdata->software_debounce =
						button->debounce_interval;
		}

		if (button->irq) {//0
			bdata->irq = button->irq;
		} else {
			irq = gpiod_to_irq(bdata->gpiod);//获取gpio对应的中断号
			if (irq < 0) {
				error = irq;
				dev_err(dev,
					"Unable to get irq number for GPIO %d, error %d\n",
					button->gpio, error);
				return error;
			}
			bdata->irq = irq;//
		}

		INIT_DELAYED_WORK(&bdata->work, gpio_keys_gpio_work_func);//初始化工作队列，注册工作队列回调函数,中断底半部

		isr = gpio_keys_gpio_isr;
		irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;//上升沿和下降沿触发

	} else {
		if (!button->irq) {
			dev_err(dev, "No IRQ specified\n");
			return -EINVAL;
		}
		bdata->irq = button->irq;

		if (button->type && button->type != EV_KEY) {
			dev_err(dev, "Only EV_KEY allowed for IRQ buttons.\n");
			return -EINVAL;
		}

		bdata->release_delay = button->debounce_interval;
		setup_timer(&bdata->release_timer,
			    gpio_keys_irq_timer, (unsigned long)bdata);//初始化定时器，设置定时器回调函数

		isr = gpio_keys_irq_isr;
		irqflags = 0;
	}

	input_set_capability(input, button->type ?: EV_KEY, button->code);

	/*
	 * Install custom action to cancel release timer and
	 * workqueue item.
	 */
	error = devm_add_action(&pdev->dev, gpio_keys_quiesce_key, bdata);
	if (error) {
		dev_err(&pdev->dev,
			"failed to register quiesce action, error: %d\n",
			error);
		return error;
	}
	/*devm_add_action:
	devres->data = data;
	devres->action = action;

	devres_add(&pdev->dev, devres); 新增devres并加入到pdev->dev.devres_head链表
	*/
	/*
	 * If platform has specified that the button can be disabled,
	 * we don't want it to share the interrupt line.
	 */
	if (!button->can_disable)
		irqflags |= IRQF_SHARED;

	error = devm_request_any_context_irq(&pdev->dev, bdata->irq,
					     isr, irqflags, desc, bdata);//申请中断
	if (error < 0) {
		dev_err(dev, "Unable to claim irq %d; error %d\n",
			bdata->irq, error);
		return error;
	}

	return 0;
}
````

#### 

#### devm_request_any_context_irq申请中断

```cpp
/*
dev:申请中断的设备
irq:申请的中断号
irq_handler_t:中断处理函数
irqflags:中断标志位
devname:设备名
dev_id:中断处理函数的参数
*/
int devm_request_any_context_irq(struct device *dev, unsigned int irq,
			      irq_handler_t handler, unsigned long irqflags,
			      const char *devname, void *dev_id)
{
	struct irq_devres *dr;
	int rc;

	dr = devres_alloc(devm_irq_release, sizeof(struct irq_devres),//释放中断的回调函数，此函数可以在设备和驱动分离时，自动释放
			  GFP_KERNEL);
	if (!dr)
		return -ENOMEM;

	rc = request_any_context_irq(irq, handler, irqflags, devname, dev_id);
	if (rc < 0) {
		devres_free(dr);
		return rc;
	}

	dr->irq = irq;
	dr->dev_id = dev_id;
	devres_add(dev, dr);//新增devres并加入到pdev->dev.devres_head链表，包含中断号和data

	return rc;
}
```

#### request_any_context_irq

```cpp
int request_any_context_irq(unsigned int irq, irq_handler_t handler,
			    unsigned long flags, const char *name, void *dev_id)
{
	struct irq_desc *desc;
	int ret;

	if (irq == IRQ_NOTCONNECTED)
		return -ENOTCONN;

	desc = irq_to_desc(irq);//中断子系统相关
	if (!desc)
		return -EINVAL;

	if (irq_settings_is_nested_thread(desc)) {
		ret = request_threaded_irq(irq, NULL, handler,
					   flags, name, dev_id);//申请中断,
		return !ret ? IRQC_IS_NESTED : ret;
	}

	ret = request_irq(irq, handler, flags, name, dev_id);//申请中断
	return !ret ? IRQC_IS_HARDIRQ : ret;
}
```

#### 工作队列回调函数，用于中断底半部gpio_keys_gpio_work_func

这里主要就是对中断产生后的数据处理，这里面可能存在睡眠和调度，所以只能放到中断下半部，最后会通过唤醒等待队列`wake_up_interruptible`和信号驱动`kill_fasync`的方式报告给用于层。

```cpp
static void gpio_keys_gpio_work_func(struct work_struct *work)
{
	struct gpio_button_data *bdata =
		container_of(work, struct gpio_button_data, work.work);

	gpio_keys_gpio_report_event(bdata);//gpio_button_data

	if (bdata->button->wakeup)
		pm_relax(bdata->input->dev.parent);
}


static void gpio_keys_gpio_report_event(struct gpio_button_data *bdata)
{
	const struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned int type = button->type ?: EV_KEY;
	int state;

	state = gpiod_get_value_cansleep(bdata->gpiod);//获取GPIO值
	if (state < 0) {
		dev_err(input->dev.parent,
			"failed to get gpio state: %d\n", state);
		return;
	}

	if (type == EV_ABS) {
		if (state)
			input_event(input, type, button->code, button->value);
	} else {
		input_event(input, type, button->code, state);
	}
	input_sync(input);
}

void input_event(struct input_dev *dev,
		 unsigned int type, unsigned int code, int value)
{
	unsigned long flags;

	if (is_event_supported(type, dev->evbit, EV_MAX)) {

		spin_lock_irqsave(&dev->event_lock, flags);
        /*
        dev: input_dev
        type: event type ：EV_KEY
        code: button code
        value: button state
        */
		input_handle_event(dev, type, code, value);
		spin_unlock_irqrestore(&dev->event_lock, flags);
	}
}

static void input_handle_event(struct input_dev *dev,
			       unsigned int type, unsigned int code, int value)
{
	int disposition = input_get_disposition(dev, type, code, &value);

	if (disposition != INPUT_IGNORE_EVENT && type != EV_SYN)
		add_input_randomness(type, code, value);

	if ((disposition & INPUT_PASS_TO_DEVICE) && dev->event)
		dev->event(dev, type, code, value);

	if (!dev->vals)
		return;

	if (disposition & INPUT_PASS_TO_HANDLERS) {//yes
		struct input_value *v;

		if (disposition & INPUT_SLOT) {//no
			v = &dev->vals[dev->num_vals++];
			v->type = EV_ABS;
			v->code = ABS_MT_SLOT;
			v->value = dev->mt->slot;
		}

		v = &dev->vals[dev->num_vals++];
		v->type = type;
		v->code = code;
		v->value = value;
	}
//有冲洗标志或者数据量达到buffer尾就发送数据
	if (disposition & INPUT_FLUSH) {//冲刷，立即发送
		if (dev->num_vals >= 2)
			input_pass_values(dev, dev->vals, dev->num_vals);
		dev->num_vals = 0;
	} else if (dev->num_vals >= dev->max_vals - 2) {//数据堆积到某个程度在发送...
		dev->vals[dev->num_vals++] = input_value_sync;
		input_pass_values(dev, dev->vals, dev->num_vals);
		dev->num_vals = 0;
	}

}


static int input_get_disposition(struct input_dev *dev,
			  unsigned int type, unsigned int code, int *pval)
{
	int disposition = INPUT_IGNORE_EVENT;
	int value = *pval;

	switch (type) {

	...

	case EV_KEY:
		if (is_event_supported(code, dev->keybit, KEY_MAX)) {

			/* auto-repeat bypasses state updates */
			if (value == 2) {
				disposition = INPUT_PASS_TO_HANDLERS;
				break;
			}

			if (!!test_bit(code, dev->key) != !!value) {

				__change_bit(code, dev->key);
				disposition = INPUT_PASS_TO_HANDLERS;
			}
		}
		break;

	*pval = value;
	return disposition;
}
    
static void input_pass_values(struct input_dev *dev,
			      struct input_value *vals, unsigned int count)
{
	struct input_handle *handle;
	struct input_value *v;

	if (!count)
		return;

	rcu_read_lock();

	handle = rcu_dereference(dev->grab);
	if (handle) {
		count = input_to_handler(handle, vals, count);
	} else {
		list_for_each_entry_rcu(handle, &dev->h_list, d_node)
			if (handle->open) {
				count = input_to_handler(handle, vals, count);//遍历dev的handle链表，找到被打开的handler，执行并退出
				if (!count)
					break;
			}
	}

	rcu_read_unlock();

	/* trigger auto repeat for key events */
	if (test_bit(EV_REP, dev->evbit) && test_bit(EV_KEY, dev->evbit)) {
		for (v = vals; v != vals + count; v++) {
			if (v->type == EV_KEY && v->value != 2) {
				if (v->value)
					input_start_autorepeat(dev, v->code);
				else
					input_stop_autorepeat(dev);
			}
		}
	}
}
    
static unsigned int input_to_handler(struct input_handle *handle,
			struct input_value *vals, unsigned int count)
{
	struct input_handler *handler = handle->handler;
	struct input_value *end = vals;
	struct input_value *v;

	if (handler->filter) {//null
		for (v = vals; v != vals + count; v++) {
			if (handler->filter(handle, v->type, v->code, v->value))
				continue;
			if (end != v)
				*end = *v;
			end++;
		}
		count = end - vals;
	}

	if (!count)
		return 0;

	if (handler->events)
		handler->events(handle, vals, count);//yes
	else if (handler->event)
		for (v = vals; v != vals + count; v++)
			handler->event(handle, v->type, v->code, v->value);

	return count;
}
    

```

#### evdev_events

```cpp
static void evdev_events(struct input_handle *handle,
			 const struct input_value *vals, unsigned int count)
{
	struct evdev *evdev = handle->private;
	struct evdev_client *client;
	ktime_t ev_time[EV_CLK_MAX];

	ev_time[EV_CLK_MONO] = ktime_get();
	ev_time[EV_CLK_REAL] = ktime_mono_to_real(ev_time[EV_CLK_MONO]);
	ev_time[EV_CLK_BOOT] = ktime_mono_to_any(ev_time[EV_CLK_MONO],
						 TK_OFFS_BOOT);

	rcu_read_lock();

	client = rcu_dereference(evdev->grab);

	if (client)
		evdev_pass_values(client, vals, count, ev_time);
	else//yes
		list_for_each_entry_rcu(client, &evdev->client_list, node)
			evdev_pass_values(client, vals, count, ev_time);

	rcu_read_unlock();
}

static void evdev_pass_values(struct evdev_client *client,
			const struct input_value *vals, unsigned int count,
			ktime_t *ev_time)
{
	struct evdev *evdev = client->evdev;
	const struct input_value *v;
	struct input_event event;
	bool wakeup = false;

	if (client->revoked)
		return;

	event.time = ktime_to_timeval(ev_time[client->clk_type]);

	/* Interrupts are disabled, just acquire the lock. */
	spin_lock(&client->buffer_lock);

	for (v = vals; v != vals + count; v++) {
		if (__evdev_is_filtered(client, v->type, v->code))
			continue;

		if (v->type == EV_SYN && v->code == SYN_REPORT) {
			/* drop empty SYN_REPORT */
			if (client->packet_head == client->head)
				continue;

			wakeup = true;
		}

		event.type = v->type;
		event.code = v->code;
		event.value = v->value;
		__pass_event(client, &event);
	}

	spin_unlock(&client->buffer_lock);

	if (wakeup)
		wake_up_interruptible(&evdev->wait);//唤醒等待队列，被唤醒的地方会发送数据
}

static void __pass_event(struct evdev_client *client,
			 const struct input_event *event)
{
	client->buffer[client->head++] = *event;
	client->head &= client->bufsize - 1;

	if (unlikely(client->head == client->tail)) {
		/*
		 * This effectively "drops" all unconsumed events, leaving
		 * EV_SYN/SYN_DROPPED plus the newest event in the queue.
		 */
		client->tail = (client->head - 2) & (client->bufsize - 1);

		client->buffer[client->tail].time = event->time;
		client->buffer[client->tail].type = EV_SYN;
		client->buffer[client->tail].code = SYN_DROPPED;
		client->buffer[client->tail].value = 0;

		client->packet_head = client->tail;
	}

	if (event->type == EV_SYN && event->code == SYN_REPORT) {
		client->packet_head = client->head;
		kill_fasync(&client->fasync, SIGIO, POLL_IN);//信号驱动，需要结合fops中的.async方法，kill_fasync会触发.async方法，触发应用程序的SIGIO信号处理函数
	}
}
```

#### 中断处理函数gpio_keys_gpio_isr

这里在一定延时后唤醒工作队列，这个延时用作按键防抖

```cpp
static irqreturn_t gpio_keys_gpio_isr(int irq, void *dev_id)
{
	struct gpio_button_data *bdata = dev_id;

	BUG_ON(irq != bdata->irq);

	if (bdata->button->wakeup)
		pm_stay_awake(bdata->input->dev.parent);

	mod_delayed_work(system_wq,
			 &bdata->work,
			 msecs_to_jiffies(bdata->software_debounce));//调度工作队列

	return IRQ_HANDLED;
}
```

#### gpio_keys_irq_isr



这是另一种处理方式，直接在中断处理函数中（上半部）进行数据处理。随后唤醒定时器执行防抖后的操作。

```cpp
static irqreturn_t gpio_keys_irq_isr(int irq, void *dev_id)
{
	struct gpio_button_data *bdata = dev_id;
	const struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned long flags;

	BUG_ON(irq != bdata->irq);

	spin_lock_irqsave(&bdata->lock, flags);

	if (!bdata->key_pressed) {//按键未被按下，初始值0
		if (bdata->button->wakeup)
			pm_wakeup_event(bdata->input->dev.parent, 0);

		input_event(input, EV_KEY, button->code, 1);//发送EV_KEY事件
		input_sync(input);//刷新缓冲区，送出数据

		if (!bdata->release_delay) {//没有定时器延时
			input_event(input, EV_KEY, button->code, 0);//发送EV_KEY事件
			input_sync(input);//刷新缓冲区，送出数据
			goto out;
		}

		bdata->key_pressed = true;//按键被按下
	}

	if (bdata->release_delay)//定时器延时回调
		mod_timer(&bdata->release_timer,
			jiffies + msecs_to_jiffies(bdata->release_delay));//延时后调用定时器处理函数
out:
	spin_unlock_irqrestore(&bdata->lock, flags);
	return IRQ_HANDLED;
}
```



#### 定时器回调函数gpio_keys_irq_timer

```cpp
static void gpio_keys_irq_timer(unsigned long _data)
{
	struct gpio_button_data *bdata = (struct gpio_button_data *)_data;
	struct input_dev *input = bdata->input;
	unsigned long flags;

	spin_lock_irqsave(&bdata->lock, flags);
	if (bdata->key_pressed) {//按键被按下
		input_event(input, EV_KEY, bdata->button->code, 0);//发送EV_KEY事件
		input_sync(input);//刷新缓冲区送出数据
		bdata->key_pressed = false;//恢复
	}
	spin_unlock_irqrestore(&bdata->lock, flags);
}
```

#### input_register_device

```cpp
int input_register_device(struct input_dev *dev)
{
	struct input_devres *devres = NULL;
	struct input_handler *handler;
	unsigned int packet_size;
	const char *path;
	int error;

	if (dev->devres_managed) {
		devres = devres_alloc(devm_input_device_unregister,
				      sizeof(struct input_devres), GFP_KERNEL);
		if (!devres)
			return -ENOMEM;

		devres->input = dev;
	}

	/* Every input device generates EV_SYN/SYN_REPORT events. */
	__set_bit(EV_SYN, dev->evbit);//设置同步位

	/* KEY_RESERVED is not supposed to be transmitted to userspace. */
	__clear_bit(KEY_RESERVED, dev->keybit);

	/* Make sure that bitmasks not mentioned in dev->evbit are clean. */
	input_cleanse_bitmasks(dev);

	packet_size = input_estimate_events_per_packet(dev);//获取需要分配的缓冲区的大小
	if (dev->hint_events_per_packet < packet_size)
		dev->hint_events_per_packet = packet_size;

	dev->max_vals = dev->hint_events_per_packet + 2;//设置缓冲区大小
	dev->vals = kcalloc(dev->max_vals, sizeof(*dev->vals), GFP_KERNEL);//分配缓冲区
	if (!dev->vals) {
		error = -ENOMEM;
		goto err_devres_free;
	}

	/*
	 * If delay and period are pre-set by the driver, then autorepeating
	 * is handled by the driver itself and we don't do it in input.c.
	 */
	if (!dev->rep[REP_DELAY] && !dev->rep[REP_PERIOD])
		input_enable_softrepeat(dev, 250, 33);//自动重复设置

	if (!dev->getkeycode)
		dev->getkeycode = input_default_getkeycode;//对于键盘

	if (!dev->setkeycode)
		dev->setkeycode = input_default_setkeycode;//对键盘

	error = device_add(&dev->dev);//
	if (error)
		goto err_free_vals;

	path = kobject_get_path(&dev->dev.kobj, GFP_KERNEL);
	pr_info("%s as %s\n",
		dev->name ? dev->name : "Unspecified device",
		path ? path : "N/A");
	kfree(path);

	error = mutex_lock_interruptible(&input_mutex);
	if (error)
		goto err_device_del;

	list_add_tail(&dev->node, &input_dev_list);

	list_for_each_entry(handler, &input_handler_list, node)
		input_attach_handler(dev, handler);

	input_wakeup_procfs_readers();

	mutex_unlock(&input_mutex);

	if (dev->devres_managed) {
		dev_dbg(dev->dev.parent, "%s: registering %s with devres.\n",
			__func__, dev_name(&dev->dev));
		devres_add(dev->dev.parent, devres);
	}
	return 0;

err_device_del:
	device_del(&dev->dev);
err_free_vals:
	kfree(dev->vals);
	dev->vals = NULL;
err_devres_free:
	devres_free(devres);
	return error;
}
```

#### input_enable_softrepeat

```cpp
void input_enable_softrepeat(struct input_dev *dev, int delay, int period)
{
	dev->timer.data = (unsigned long) dev;
	dev->timer.function = input_repeat_key;//定时器处理函数
	dev->rep[REP_DELAY] = delay;//延时
	dev->rep[REP_PERIOD] = period;//周期
}

static void input_repeat_key(unsigned long data)
{
	struct input_dev *dev = (void *) data;
	unsigned long flags;

	spin_lock_irqsave(&dev->event_lock, flags);

	if (test_bit(dev->repeat_key, dev->key) &&
	    is_event_supported(dev->repeat_key, dev->keybit, KEY_MAX)) {
		struct input_value vals[] =  {
			{ EV_KEY, dev->repeat_key, 2 },
			input_value_sync
		};

		input_pass_values(dev, vals, ARRAY_SIZE(vals));//上报

		if (dev->rep[REP_PERIOD])
			mod_timer(&dev->timer, jiffies +
					msecs_to_jiffies(dev->rep[REP_PERIOD]));//重新激活定时器，也就是只要满足条件，会一直上传事件
	}

	spin_unlock_irqrestore(&dev->event_lock, flags);
}
/*
gpio_keys_gpio_report_event
	->input_event
		->input_handle_event
			->input_pass_values
				->input_start_autorepeat/input_stop_autorepeat
*/

static void input_start_autorepeat(struct input_dev *dev, int code)
{
	if (test_bit(EV_REP, dev->evbit) &&
	    dev->rep[REP_PERIOD] && dev->rep[REP_DELAY] &&
	    dev->timer.data) {
		dev->repeat_key = code;
		mod_timer(&dev->timer,
			  jiffies + msecs_to_jiffies(dev->rep[REP_DELAY]));//延时后激活定时器，执行上面的input_repeat_key,一旦在此处激活定时器，input_repeat_key里面会一直给定时器时间，定时器回调函数会一直执行,直到不满足input_repeat_key里的条件或删除定时器
	}
}

static void input_stop_autorepeat(struct input_dev *dev)
{
	del_timer(&dev->timer);//删除定时器，可被mod_timer再次激活
}
```

### 关于之前的等待队列和信号驱动

#### open

设备被打开时会调用cdev中注册的fops的open函数：

```cpp
static int evdev_open(struct inode *inode, struct file *file)
{
	struct evdev *evdev = container_of(inode->i_cdev, struct evdev, cdev);//cdev注册时是被包含在evdev结构中的，container_of可以找到evdev
	unsigned int bufsize = evdev_compute_buffer_size(evdev->handle.dev);
	unsigned int size = sizeof(struct evdev_client) +
					bufsize * sizeof(struct input_event);
	struct evdev_client *client;
	int error;

	client = kzalloc(size, GFP_KERNEL | __GFP_NOWARN);
	if (!client)
		client = vzalloc(size);
	if (!client)
		return -ENOMEM;

	client->bufsize = bufsize;
	spin_lock_init(&client->buffer_lock);
	client->evdev = evdev;
	evdev_attach_client(evdev, client);//将client加入evdev的client链表上

	error = evdev_open_device(evdev);
	if (error)
		goto err_free_client;

	file->private_data = client;//
	nonseekable_open(inode, file);//禁用了fseek和lseek相关的定位方法？

	return 0;

 err_free_client:
	evdev_detach_client(evdev, client);
	kvfree(client);
	return error;
}



static int evdev_open_device(struct evdev *evdev)
{
	int retval;

	retval = mutex_lock_interruptible(&evdev->mutex);
	if (retval)
		return retval;

	if (!evdev->exist)
		retval = -ENODEV;
	else if (!evdev->open++) {//新注册的evdev设备只能被打开一次
		retval = input_open_device(&evdev->handle);
		if (retval)
			evdev->open--;
	}

	mutex_unlock(&evdev->mutex);
	return retval;
}

int input_open_device(struct input_handle *handle)
{
	struct input_dev *dev = handle->dev;
	int retval;

	retval = mutex_lock_interruptible(&dev->mutex);
	if (retval)
		return retval;

	if (dev->going_away) {
		retval = -ENODEV;
		goto out;
	}

	handle->open++;

	if (!dev->users++ && dev->open)
		retval = dev->open(dev);//这里调用input_dev的open

	if (retval) {
		dev->users--;
		if (!--handle->open) {
			/*
			 * Make sure we are not delivering any more events
			 * through this handle
			 */
			synchronize_rcu();
		}
	}

 out:
	mutex_unlock(&dev->mutex);
	return retval;
}
```

#### gpio_keys_open

```cpp
static int gpio_keys_open(struct input_dev *input)
{
	struct gpio_keys_drvdata *ddata = input_get_drvdata(input);
	const struct gpio_keys_platform_data *pdata = ddata->pdata;
	int error;

	if (pdata->enable) {
		error = pdata->enable(input->dev.parent);
		if (error)
			return error;
	}

	/* Report current state of buttons that are connected to GPIOs */
	gpio_keys_report_state(ddata);

	return 0;
}
```

open时会报告一次按键的初始状态

#### read

```cpp
static ssize_t evdev_read(struct file *file, char __user *buffer,
			  size_t count, loff_t *ppos)
{
	struct evdev_client *client = file->private_data;
	struct evdev *evdev = client->evdev;
	struct input_event event;
	size_t read = 0;
	int error;

	if (count != 0 && count < input_event_size())
		return -EINVAL;

	for (;;) {
		if (!evdev->exist || client->revoked)
			return -ENODEV;

		if (client->packet_head == client->tail &&
		    (file->f_flags & O_NONBLOCK))
			return -EAGAIN;

		/*
		 * count == 0 is special - no IO is done but we check
		 * for error conditions (see above).
		 */
		if (count == 0)
			break;

		while (read + input_event_size() <= count &&
		       evdev_fetch_next_event(client, &event)) {

			if (input_event_to_user(buffer + read, &event))//上报数据到用户
				return -EFAULT;

			read += input_event_size();
		}

		if (read)
			break;

		if (!(file->f_flags & O_NONBLOCK)) {
			error = wait_event_interruptible(evdev->wait,
					client->packet_head != client->tail ||
					!evdev->exist || client->revoked);//等待数据
			if (error)
				return error;
		}
	}

	return read;
}
```



#### async

```cpp
static int evdev_fasync(int fd, struct file *file, int on)
{
	struct evdev_client *client = file->private_data;

	return fasync_helper(fd, file, on, &client->fasync);
}
```

fasync是为了使驱动的读写和应用程序的读写分开，使得应用程序可以在驱动读写的时候去做别的事。

应用程序通过fcntl给自己的SIGIO信号安装自己的响应函数，

驱动通过kill_fasync(&async, SIGIO, POLL_IN); 发SIGIO信号给应用程序，应用程序就调用自己安装的响应函数去处理。

fasync_helper作用就是初始化fasync，包括分配内存和设置属性，最后在驱动的release里把fasync_helper初始化的东西free掉。
https://blog.51cto.com/u_11866419/4765038