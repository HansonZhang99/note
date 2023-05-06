# 设备模型Bus和Platform Bus

Linux-4.9.88

>在Linux设备模型中，Bus（总线）是一类特殊的设备，它是连接处理器和其它设备之间的通道（channel）。为了方便设备模型的实现，内核规定，系统中的每个设备都要连接在一个Bus上，这个Bus可以是一个内部Bus、虚拟Bus或者Platform Bus。
>
>内核通过struct bus_type结构，抽象Bus，它是在include/linux/device.h中定义的。
>
>http://www.wowotech.net/device_model/bus.html



## bus相关数据结构

### bus_type

```cpp
//include/linux/device.h
struct bus_type {
	const char		*name;//，该bus的名称，会在sysfs中以目录的形式存在，如platform bus在sysfs中表现为"/sys/bus/platform”。
	const char		*dev_name;//所讲述的struct device结构中的init_name有关。对有些设备而言（例如批量化的USB设备），设计者根本就懒得为它起名字的，而内核也支持这种懒惰，允许将设备的名字留空。这样当设备注册到内核后，设备模型的核心逻辑就会用"bus->dev_name+device ID”的形式，为这样的设备生成一个名称。
	struct device		*dev_root;
	struct device_attribute	*dev_attrs;	/* use dev_groups instead */
    //一些默认的attribute，可以在bus、device或者device_driver添加到内核时，自动为它们添加相应的attribute。
	const struct attribute_group **bus_groups;
	const struct attribute_group **dev_groups;
	const struct attribute_group **drv_groups;

	int (*match)(struct device *dev, struct device_driver *drv);//一个由具体的bus driver实现的回调函数。当任何属于该Bus的device或者device_driver添加到内核时，内核都会调用该接口，如果新加的device或device_driver匹配上了自己的另一半的话，该接口要返回非零值，此时Bus模块的核心逻辑就会执行后续的处理。
	int (*uevent)(struct device *dev, struct kobj_uevent_env *env);//一个由具体的bus driver实现的回调函数。当任何属于该Bus的device，发生添加、移除或者其它动作时，Bus模块的核心逻辑就会调用该接口，以便bus driver能够修改环境变量。
    
    //这两个回调函数，和device_driver中的非常类似，但它们的存在是非常有意义的。可以想象一下，如果需要probe（其实就是初始化）指定的device话，需要保证该device所在的bus是被初始化过、确保能正确工作的。这就要就在执行device_driver的probe前，先执行它的bus的probe。remove的过程相反。并不是所有的bus都需要probe和remove接口的，因为对有些bus来说（例如platform bus），它本身就是一个虚拟的总线，无所谓初始化，直接就能使用，因此这些bus的driver就可以将这两个回调函数留空。
	int (*probe)(struct device *dev);
	int (*remove)(struct device *dev);
	void (*shutdown)(struct device *dev);

	int (*online)(struct device *dev);
	int (*offline)(struct device *dev);

	int (*suspend)(struct device *dev, pm_message_t state);
	int (*resume)(struct device *dev);

	const struct dev_pm_ops *pm;

	const struct iommu_ops *iommu_ops;

	struct subsys_private *p;
	struct lock_class_key lock_key;
};

```

### subsys_private

```cpp
//叫bus_private更好理解，类似device_driver的private
struct subsys_private {
	struct kset subsys;//bus对应的kset
	struct kset *devices_kset;//某个bus下的子kset，表现为/sys/bus/spi/devices
	struct list_head interfaces;
	struct mutex mutex;

	struct kset *drivers_kset;//某个bus下的子kset，表现为/sys/bus/spi/drivers
	struct klist klist_devices;//bus的device链表
	struct klist klist_drivers;//bus的driver链表
	struct blocking_notifier_head bus_notifier;//通知模块
	unsigned int drivers_autoprobe:1;//用于控制该bus下的drivers或者device是否自动probe
	struct bus_type *bus;//保存上层的bus指针

	struct kset glue_dirs;
	struct class *class;
};
```

根据上面的核心数据结构，可以总结出bus模块的功能包括：

- bus的注册和注销
- 本bus下有device或者device_driver注册到内核时的处理
- 本bus下有device或者device_driver从内核注销时的处理
- device_drivers的probe处理
- 管理bus下的所有device和device_driver

## bus相关接口

### bus_register

```cpp
//drivers/base/bus.c
int bus_register(struct bus_type *bus)
{
	int retval;
	struct subsys_private *priv;
	struct lock_class_key *key = &bus->lock_key;

	priv = kzalloc(sizeof(struct subsys_private), GFP_KERNEL);//分配private内存
	if (!priv)
		return -ENOMEM;

	priv->bus = bus;//private保存bus
	bus->p = priv;//bus保存private

	BLOCKING_INIT_NOTIFIER_HEAD(&priv->bus_notifier);//初始化通知链

	retval = kobject_set_name(&priv->subsys.kobj, "%s", bus->name);//设置代表bus的kobj名字
	if (retval)
		goto out;

	priv->subsys.kobj.kset = bus_kset;//kobj相关
	priv->subsys.kobj.ktype = &bus_ktype;//kobj相关
	priv->drivers_autoprobe = 1;//允许自动探测

	retval = kset_register(&priv->subsys);//注册kset，bus将在sysfs中表现为/sys/bus/xxx（bus_kset在buses_init函数中被初始化，bus_kset代表/sys/bus目录
	if (retval)
		goto out;

	retval = bus_create_file(bus, &bus_attr_uevent);//创建bus对应的uevent属性文件
	if (retval)
		goto bus_uevent_fail;

	priv->devices_kset = kset_create_and_add("devices", NULL,
						 &priv->subsys.kobj);//创建bus子目录，在sysfs表现为/sys/bus/xxx/devices
	if (!priv->devices_kset) {
		retval = -ENOMEM;
		goto bus_devices_fail;
	}

	priv->drivers_kset = kset_create_and_add("drivers", NULL,
						 &priv->subsys.kobj);//创建bus子目录，在sysfs表现为/sys/bus/xxx/drivers
	if (!priv->drivers_kset) {
		retval = -ENOMEM;
		goto bus_drivers_fail;
	}

	INIT_LIST_HEAD(&priv->interfaces);
	__mutex_init(&priv->mutex, "subsys mutex", key);
    //初始化设备驱动链表
	klist_init(&priv->klist_devices, klist_devices_get, klist_devices_put);
	klist_init(&priv->klist_drivers, NULL, NULL);

	retval = add_probe_files(bus);//在bus下添加drivers_probe和drivers_autoprobe两个attribute（如/sys/bus/spi/drivers_probe和/sys/bus/spi/drivers_autoprobe），其中drivers_probe允许用户空间程序主动出发指定bus下的device_driver的probe动作，而drivers_autoprobe控制是否在device或device_driver添加到内核时，自动执行probe
	if (retval)
		goto bus_probe_files_fail;

	retval = bus_add_groups(bus, bus->bus_groups);//添加相应的属性组
	if (retval)
		goto bus_groups_fail;

	pr_debug("bus: '%s': registered\n", bus->name);
	return 0;

bus_groups_fail:
	remove_probe_files(bus);
bus_probe_files_fail:
	kset_unregister(bus->p->drivers_kset);
bus_drivers_fail:
	kset_unregister(bus->p->devices_kset);
bus_devices_fail:
	bus_remove_file(bus, &bus_attr_uevent);
bus_uevent_fail:
	kset_unregister(&bus->p->subsys);
out:
	kfree(bus->p);
	bus->p = NULL;
	return retval;
}
```

bus_register向内核注册一条bus，sysfs下表现为/sys/bus/xxx目录，同时还会在xxx下创建/sys/bus/xxx/devices和/sys/bus/xxx/drivers两个目录，xxx有自己的uevent属性文件和属性组，bus数据结构有两条链表分别是设备链表和驱动链表，用来保存设备和驱动。

如下取自上位机目录:

```shell
zhhhhhh@ubuntu:/sys/bus/platform$ ls
devices  drivers  drivers_autoprobe  drivers_probe  uevent
```



#### add_probe_files

```cpp
static int add_probe_files(struct bus_type *bus)
{
	int retval;

	retval = bus_create_file(bus, &bus_attr_drivers_probe);
	if (retval)
		goto out;

	retval = bus_create_file(bus, &bus_attr_drivers_autoprobe);
	if (retval)
		bus_remove_file(bus, &bus_attr_drivers_probe);
out:
	return retval;
}
```

### bus_add_device

```cpp
int bus_add_device(struct device *dev)
{
	struct bus_type *bus = bus_get(dev->bus);
	int error = 0;

	if (bus) {
		pr_debug("bus: '%s': add device %s\n", bus->name, dev_name(dev));
		error = device_add_attrs(bus, dev);//在dev中添加来自bus的attrbute
		if (error)
			goto out_put;
		error = device_add_groups(dev, bus->dev_groups);//在dev中添加来自bus的属性组
		if (error)
			goto out_id;
		error = sysfs_create_link(&bus->p->devices_kset->kobj,
						&dev->kobj, dev_name(dev));//在bus->p->devices_kset->kobj目录下创建与dev同名的符号链接，指向dev->kobj目录
		if (error)
			goto out_groups;
		error = sysfs_create_link(&dev->kobj,
				&dev->bus->p->subsys.kobj, "subsystem");//在dev->kobj目录创建名为subsystem链接，指向dev->bus->p->subsys.kobj(这里有个疑问，之前的device_add_class_symlinks函数中已经在dev下创建了一个名为subsystem的链接，这里又创建一个是否有问题，在同一目录创建同名符号链接测试第二次创建时确实会报错，而且sys下的表述看到的都是指向class的而不是bus)
		if (error)
			goto out_subsys;
		klist_add_tail(&dev->p->knode_bus, &bus->p->klist_devices);//将dev->p->knode_bus加入bus的p->klist_devices链表
	}
	return 0;

out_subsys:
	sysfs_remove_link(&bus->p->devices_kset->kobj, dev_name(dev));
out_groups:
	device_remove_groups(dev, bus->dev_groups);
out_id:
	device_remove_attrs(bus, dev);
out_put:
	bus_put(dev->bus);
	return error;
}

```

### bus_add_driver

```cpp
int bus_add_driver(struct device_driver *drv)
{
	struct bus_type *bus;
	struct driver_private *priv;
	int error = 0;

	bus = bus_get(drv->bus);//获取driver->bus
	if (!bus)
		return -EINVAL;

	pr_debug("bus: '%s': add driver %s\n", bus->name, drv->name);

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);//分配一个driver_private指针
	if (!priv) {
		error = -ENOMEM;
		goto out_put_bus;
	}
	klist_init(&priv->klist_devices, NULL, NULL);//初始化driv的设备链表
	priv->driver = drv;//保存device_driver
	drv->p = priv;//保存priv到device_driver
	priv->kobj.kset = bus->p->drivers_kset;//初始化kobj的kset为总线的p->drivers_kset
	error = kobject_init_and_add(&priv->kobj, &driver_ktype, NULL,
				     "%s", drv->name);//注册driver的kobject
	if (error)
		goto out_unregister;

	klist_add_tail(&priv->knode_bus, &bus->p->klist_drivers);//将driver添加到bus的driver链表
	if (drv->bus->p->drivers_autoprobe) {//如果driver所在的总线支持自动探测，
		if (driver_allows_async_probing(drv)) {//如果driver支持异步探测
			pr_debug("bus: '%s': probing driver %s asynchronously\n",
				drv->bus->name, drv->name);
			async_schedule(driver_attach_async, drv);//执行异步探测，异步探测最后会调用driver_attach
		} else {
			error = driver_attach(drv);
			if (error)
				goto out_unregister;
		}
	}
	module_add_driver(drv->owner, drv);//在/sys/module下创建driver相关的目录，并建立此目录到driver目录的链接

	error = driver_create_file(drv, &driver_attr_uevent);//在drv目录下创建相关的uevent属性文件
	if (error) {
		printk(KERN_ERR "%s: uevent attr (%s) failed\n",
			__func__, drv->name);
	}
	error = driver_add_groups(drv, bus->drv_groups);//在drv目录下创建相关的属性组
	if (error) {
		/* How the hell do we get out of this pickle? Give up */
		printk(KERN_ERR "%s: driver_create_groups(%s) failed\n",
			__func__, drv->name);
	}

	if (!drv->suppress_bind_attrs) {//支持bind属性
		error = add_bind_files(drv);//在drv下创建bind相关属性文件
		if (error) {
			/* Ditto */
			printk(KERN_ERR "%s: add_bind_files(%s) failed\n",
				__func__, drv->name);
		}
	}

	return 0;

out_unregister:
	kobject_put(&priv->kobj);
	/* drv->p is freed in driver_release()  */
	drv->p = NULL;
out_put_bus:
	bus_put(bus);
	return error;
}
```

### system/virtual/platform

>在Linux内核中，有三种比较特殊的bus（或者是子系统），分别是system bus、virtual bus和platform bus。它们并不是一个实际存在的bus（像USB、I2C等），而是为了方便设备模型的抽象，而虚构的。
>
>system bus是旧版内核提出的概念，用于抽象系统设备（如CPU、Timer等等）。而新版内核认为它是个坏点子，因为任何设备都应归属于一个普通的子系统（New subsystems should use plain subsystems, drivers/base/bus.c, line 1264），所以就把它抛弃了（不建议再使用，它的存在只为兼容旧有的实现）。
>
>virtaul bus是一个比较新的bus，主要用来抽象那些虚拟设备，所谓的虚拟设备，是指不是真实的硬件设备，而是用软件模拟出来的设备，例如虚拟机中使用的虚拟的网络设备（有关该bus的描述，可参考该链接处的解释：https://lwn.net/Articles/326540/）。
>
>platform bus就比较普通，它主要抽象集成在CPU（SOC）中的各种设备。这些设备直接和CPU连接，通过总线寻址和中断的方式，和CPU交互信息。
>
>http://www.wowotech.net/device_model/bus.html

## platform bus

>在Linux设备模型的抽象中，存在着一类称作“Platform Device”的设备，内核是这样描述它们的（Documentation/driver-model/platform.txt）：
>
>Platform devices are devices that typically appear as autonomous entities in the system. This includes legacy port-based devices and host bridges to peripheral buses, and most controllers integrated into system-on-chip platforms. What they usually have in common is direct addressing from a CPU bus. Rarely, a platform_device will be connected through a segment of some other kind of bus; but its registers will still be directly addressable.
>
>概括来说，Platform设备包括：基于端口的设备（已不推荐使用，保留下来只为兼容旧设备，legacy）；连接物理总线的桥设备；集成在SOC平台上面的控制器；连接在其它bus上的设备（很少见）。等等。
>
>这些设备有一个基本的特征：可以通过CPU bus直接寻址（例如在嵌入式系统常见的“寄存器”）。因此，由于这个共性，内核在设备模型的基础上（device和device_driver），对这些设备进行了更进一步的封装，抽象出paltform bus、platform device和platform driver，以便驱动开发人员可以方便的开发这类设备的驱动。
>
>可以说，paltform设备对Linux驱动工程师是非常重要的，因为我们编写的大多数设备驱动，都是为了驱动plaftom设备。本文我们就来看看Platform设备在内核中的实现。

![Platform设备软件架构](https://gitee.com/zhanghang1999/typora-picture/raw/master/17339e2f323c0379fb16bdbea9aaf29720140507093251.gif)

### 数据结构

#### platform_device

```cpp
//include/linux/platform_device.h
struct platform_device {
	const char	*name;//设备名
	int		id;//用于标识该设备的ID
	bool		id_auto;//指示在注册设备时，是否自动赋予ID值
	struct device	dev;//继承自device，可以使用device的方法，可以将platform device注册到sysfs中
    /*
    该设备的资源描述，由struct resource（include/linux/ioport.h）结构抽象
    在Linux中，系统资源包括I/O、Memory、Register、IRQ、DMA、Bus等多种类型。这些资源大多具有独占性，不允许多个设备同时使用，因此Linux内核提供了一些API，用于分配、管理这些资源。
当某个设备需要使用某些资源时，只需利用struct resource组织这些资源（如名称、类型、起始、结束地址等），并保存在该设备的resource指针中即可。然后在设备probe时，设备需求会调用资源管理接口，分配、使用这些资源。而内核的资源管理逻辑，可以判断这些资源是否已被使用、是否可被使用等等。
    */
	u32		num_resources;
	struct resource	*resource;

	const struct platform_device_id	*id_entry;
	char *driver_override; /* Driver name to force a match */

	/* MFD cell pointer */
	struct mfd_cell *mfd_cell;

	/* arch specific additions */
	struct pdev_archdata	archdata;
};
```

##### platform_driver

```cpp
struct platform_driver {
	int (*probe)(struct platform_device *);//类似device_driver，在更上层，匹配时优先使用bus的回调，其次使用driver的
	int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
	struct device_driver driver;//继承自driver，可以使用driver的方法，可以将platform driver注册到sysfs中
    
    /*
    of_match_table、acpi_match_table的功能类似：提供其它方式的设备probe。内核会在合适的时机检查device和device_driver的名字，如果匹配，则执行probe。其实除了名称之外，还有一些宽泛的匹配方式，例如这里提到的各种match table*/
	const struct platform_device_id *id_table;
	bool prevent_deferred_probe;
};
```

### 接口

#### platform_bus_init

```cpp
//drivers/base/platform.c
struct device platform_bus = {
	.init_name	= "platform",
};

struct bus_type platform_bus_type = {
	.name		= "platform",
	.dev_groups	= platform_dev_groups,
	.match		= platform_match,
	.uevent		= platform_uevent,
	.pm		= &platform_dev_pm_ops,
};
int __init platform_bus_init(void)
{
	int error;

	early_platform_cleanup();

	error = device_register(&platform_bus);
	if (error)
		return error;
	error =  bus_register(&platform_bus_type);
	if (error)
		device_unregister(&platform_bus);
	of_platform_register_reconfig_notifier();
	return error;
}
```

device_register函数注册一个设备，由于设备只有.init_name字段，此函数只会在sysfs下创建一个/sys/devices/platform的目录，并在目录下创建一个uevent属性文件，示例：

```shell
zhhhhhh@ubuntu:/sys/devices/platform$ ls uevent
uevent

```



bus_register注册一条总线，创建/sys/bus/platform目录，并在目录下创建uevent、drivers_autoprobe、drivers_probe文件，和devices、drivers目录，示例：

```shell
zhhhhhh@ubuntu:/sys/bus/platform$ ls
devices  drivers  drivers_autoprobe  drivers_probe  uevent

```

##### platform_match

设备和驱动匹配成功的必要条件之一就是bus->match执行成功

```cpp
static int platform_match(struct device *dev, struct device_driver *drv)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct platform_driver *pdrv = to_platform_driver(drv);

	/* When driver_override is set, only bind to the matching driver */
	if (pdev->driver_override)//platform_device优先匹配的name
		return !strcmp(pdev->driver_override, drv->name);

	/* Attempt an OF style match first */
	if (of_driver_match_device(dev, drv))//通过of_match_table匹配
		return 1;

	/* Then try ACPI style match */
	if (acpi_driver_match_device(dev, drv))//通过acpi_match_table匹配
		return 1;

	/* Then try to match against the id table */
	if (pdrv->id_table)//通过id_table匹配
		return platform_match_id(pdrv->id_table, pdev) != NULL;

	/* fall-back to driver name match */
	return (strcmp(pdev->name, drv->name) == 0);//直接通过名字匹配
}
```

#### platform_device_register

```cpp
int platform_device_register(struct platform_device *pdev)
{
	device_initialize(&pdev->dev);
	arch_setup_pdev_archdata(pdev);
	return platform_device_add(pdev);
}
```

##### platform_device_add

```cpp
int platform_device_add(struct platform_device *pdev)
{
	int i, ret;

	if (!pdev)
		return -EINVAL;

	if (!pdev->dev.parent)
		pdev->dev.parent = &platform_bus;//如果设备没有指明父设备，则默认父设备为platform_bus。sysfs下表现为/sys/bus/platform/xxx

	pdev->dev.bus = &platform_bus_type;//绑定bus,bus提供match uevent函数

	switch (pdev->id) {//设置platform设备的名字
	default:
		dev_set_name(&pdev->dev, "%s.%d", pdev->name,  pdev->id);
		break;
	case PLATFORM_DEVID_NONE:
		dev_set_name(&pdev->dev, "%s", pdev->name);
		break;
	case PLATFORM_DEVID_AUTO:
		/*
		 * Automatically allocated device ID. We mark it as such so
		 * that we remember it must be freed, and we append a suffix
		 * to avoid namespace collision with explicit IDs.
		 */
		ret = ida_simple_get(&platform_devid_ida, 0, 0, GFP_KERNEL);
		if (ret < 0)
			goto err_out;
		pdev->id = ret;
		pdev->id_auto = true;
		dev_set_name(&pdev->dev, "%s.%d.auto", pdev->name, pdev->id);
		break;
	}

	for (i = 0; i < pdev->num_resources; i++) {
		struct resource *p, *r = &pdev->resource[i];

		if (r->name == NULL)
			r->name = dev_name(&pdev->dev);

		p = r->parent;
		if (!p) {//确定资源类型
			if (resource_type(r) == IORESOURCE_MEM)
				p = &iomem_resource;
			else if (resource_type(r) == IORESOURCE_IO)
				p = &ioport_resource;
		}

		if (p && insert_resource(p, r)) {//对于内存和IO资源，插入资源树
			dev_err(&pdev->dev, "failed to claim resource %d\n", i);
			ret = -EBUSY;
			goto failed;
		}
	}

	pr_debug("Registering platform device '%s'. Parent at %s\n",
		 dev_name(&pdev->dev), dev_name(pdev->dev.parent));

	ret = device_add(&pdev->dev);//注册设备
	if (ret == 0)
		return ret;

 failed:
	if (pdev->id_auto) {
		ida_simple_remove(&platform_devid_ida, pdev->id);
		pdev->id = PLATFORM_DEVID_AUTO;
	}

	while (--i >= 0) {
		struct resource *r = &pdev->resource[i];
		if (r->parent)
			release_resource(r);
	}

 err_out:
	return ret;
}
```

platform_device_register为设备指定名字和Bus，调用device_add将设备注册到内核中

#### platform_driver_register

```cpp
#define platform_driver_register(drv) \
	__platform_driver_register(drv, THIS_MODULE)


int __platform_driver_register(struct platform_driver *drv,
				struct module *owner)
{
	drv->driver.owner = owner;
	drv->driver.bus = &platform_bus_type;//绑定platform bus，bus提供match uevent等函数
	drv->driver.probe = platform_drv_probe;//提供drv的probe函数
	drv->driver.remove = platform_drv_remove;
	drv->driver.shutdown = platform_drv_shutdown;

	return driver_register(&drv->driver);
}
```

#### platform_drv_probe

```cpp
static int platform_drv_probe(struct device *_dev)
{
	struct platform_driver *drv = to_platform_driver(_dev->driver);
	struct platform_device *dev = to_platform_device(_dev);
	int ret;

	ret = of_clk_set_defaults(_dev->of_node, false);
	if (ret < 0)
		return ret;

	ret = dev_pm_domain_attach(_dev, true);
	if (ret != -EPROBE_DEFER) {
		if (drv->probe) {
			ret = drv->probe(dev);
			if (ret)
				dev_pm_domain_detach(_dev, true);
		} else {
			/* don't fail if just dev_pm_domain_attach failed */
			ret = 0;
		}
	}

	if (drv->prevent_deferred_probe && ret == -EPROBE_DEFER) {
		dev_warn(_dev, "probe deferral not supported\n");
		ret = -ENXIO;
	}

	return ret;
}
```

device_add中的really_probe中会优先调用bus->probe其次是drv->probe。platform没有提供bus的probe，因此会调用platform_drv_probe，此函数最终会调用platform_driver->probe函数，最后platform_drv_probe会调用driver_register函数。
