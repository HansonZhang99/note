# 设备模型 device、device_driver

内核版本：Linux-4.9.88

> device和device driver是Linux驱动开发的基本概念。Linux kernel的思路很简单：驱动开发，就是要开发指定的软件（driver）以驱动指定的设备，所以kernel就为设备和驱动它的driver定义了两个数据结构，分别是device和device_driver。(http://www.wowotech.net/device_model/device_and_driver.html)

 ## 数据结构device/device_driver

/include/linux/device.h

```cpp
struct device {
	struct device		*parent;//该设备的父设备，一般是该设备所从属的bus、controller等设备。

	struct device_private	*p;//一个用于struct device的私有数据结构指针，该指针中会保存子设备链表、用于添加到bus/driver/prent等设备中的链表头等等

	struct kobject kobj;//该数据结构对应的struct kobject。
	const char		*init_name; //该设备的名称,在设备模型中，名称是一个非常重要的变量，任何注册到内核中的设备，都必须有一个合法的名称，可以在初始化时给出，也可以由内核根据“bus name + device ID”的方式创造。
	const struct device_type *type;//struct device_type结构是新版本内核新引入的一个结构，它和struct device关系，非常类似stuct kobj_type和struct kobject之间的关系

	struct mutex		mutex;	/* mutex to synchronize calls to
					 * its driver.
					 */

	struct bus_type	*bus;		//该device属于哪个总线
	struct device_driver *driver;	//该device对应的device driver
	void		*platform_data;	//一般保存硬件的如gpio,端口的数据，如寄存器地址等
	void		*driver_data;	//驱动程序特定信息的私有指针。
	struct dev_links_info	links;
	struct dev_pm_info	power;
	struct dev_pm_domain	*pm_domain;

#ifdef CONFIG_GENERIC_MSI_IRQ_DOMAIN
	struct irq_domain	*msi_domain;
#endif
#ifdef CONFIG_PINCTRL
	struct dev_pin_info	*pins;//pinctrl子系统功能
#endif
#ifdef CONFIG_GENERIC_MSI_IRQ
	struct list_head	msi_list;
#endif

#ifdef CONFIG_NUMA
	int		numa_node;	/* NUMA node this device is close to */
#endif
	u64		*dma_mask;	/* dma mask (if dma'able device) */
	u64		coherent_dma_mask;/* Like dma_mask, but for
					     alloc_coherent mappings as
					     not all hardware supports
					     64 bit addresses for consistent
					     allocations such descriptors. */
	unsigned long	dma_pfn_offset;

	struct device_dma_parameters *dma_parms;

	struct list_head	dma_pools;	/* dma pools (if dma'ble) */

	struct dma_coherent_mem	*dma_mem; /* internal for coherent mem
					     override */
#ifdef CONFIG_DMA_CMA
	struct cma *cma_area;		/* contiguous memory area for dma
					   allocations */
#endif
	/* arch specific additions */
	struct dev_archdata	archdata;

	struct device_node	*of_node; //设备树节点
	struct fwnode_handle	*fwnode; /* firmware device node */
	/*
	是一个32位的整数，它由两个部分（Major和Minor）组成，在需要以设备节点的形式（字符设备和块设备）向用户空间提	   供接口的设备中，当作设备号使用。在这里，该变量主要用于在sys文件系统中，为每个具有设备号的device，创		建/sys/dev/* 下的对应目录，如下：

	1|root@android:/storage/sdcard0 #ls /sys/dev/char/1\:                                                                     
	1:1/  1:11/ 1:13/ 1:14/ 1:2/  1:3/  1:5/  1:7/  1:8/  1:9/ 
	1|root@android:/storage/sdcard0 #ls /sys/dev/char/1:1                                                                    
	1:1/  1:11/ 1:13/ 1:14/
	1|root@android:/storage/sdcard0 # ls /sys/dev/char/1\:1 
	/sys/dev/char/1:1  
	*/
	dev_t			devt;
	u32			id;	/* device instance */

	spinlock_t		devres_lock;
	struct list_head	devres_head;

	struct klist_node	knode_class;
	struct class		*class;//该设备属于哪个class。
	const struct attribute_group **groups;	//该设备的默认attribute集合。将会在设备注册时自动在sysfs中创建对应的文件。

	void	(*release)(struct device *dev);
	struct iommu_group	*iommu_group;
	struct iommu_fwspec	*iommu_fwspec;

	bool			offline_disabled:1;
	bool			offline:1;
}
```

```cpp
struct device_driver {
	const char		*name;//该driver的名称。和device结构一样，该名称非常重要
	struct bus_type		*bus;//该driver所驱动设备的总线设备。为什么driver需要记录总线设备的指针？因为内核要保证在driver运行前，设备所依赖的总线能够正确初始化。

	struct module		*owner;
	const char		*mod_name;	/* used for built-in modules */
	/*
	是不在sysfs中启用bind和unbind attribute，如下：
	root@android:/storage/sdcard0 # ls 	/sys/bus/platform/drivers/switch-gpio/                     bind   uevent unbind
	在kernel中，bind/unbind是从用户空间手动的为driver绑定/解绑定指定的设备的机制。这种机制是在bus.c中完成的
	*/
	bool suppress_bind_attrs;	/* disables bind/unbind via sysfs */
	enum probe_type probe_type;

	const struct of_device_id	*of_match_table;
	const struct acpi_device_id	*acpi_match_table;
	/*
	probe、remove，这两个接口函数用于实现driver逻辑的开始和结束。Driver是一段软件code，因此会有开始和结束两个代码逻辑，就像PC程序，会有一个main函数，main函数的开始就是开始，return的地方就是结束。而内核driver却有其特殊性：在设备模型的结构下，只有driver和device同时存在时，才需要开始执行driver的代码逻辑。这也是probe和remove两个接口名称的由来：检测到了设备和移除了设备（就是为热拔插起的！）。
	*/
	int (*probe) (struct device *dev);
	int (*remove) (struct device *dev);
    //shutdown、suspend、resume、pm，电源管理相关的内容
	void (*shutdown) (struct device *dev);
	int (*suspend) (struct device *dev, pm_message_t state);
	int (*resume) (struct device *dev);
    /*和struct device结构中的同名变量类似，driver也可以定义一些默认attribute，这样在将driver注册到内核中时，内核设备模型部分的代码（driver/base/driver.c）会自动将这些attribute添加到sysfs中。*/
	const struct attribute_group **groups;

	const struct dev_pm_ops *pm;
	
	struct driver_private *p;//driver core的私有数据指针，其它模块不能访问。
};

struct driver_private {
	struct kobject kobj;//保存driver对应的kobj
	struct klist klist_devices;//保存和此驱动绑定的设备链表
	struct klist_node knode_bus;//节点，会加入到bus的驱动链表中
	struct module_kobject *mkobj;//module相关
	struct device_driver *driver;//保存device_driver自己
};
```

## 设备驱动匹配模型

### 匹配流程

> 在设备模型框架下，设备驱动的开发是一件很简单的事情，主要包括2个步骤：
>
> 1. 分配一个struct device类型的变量，填充必要的信息后，把它注册到内核中。
> 2. 分配一个struct device_driver类型的变量，填充必要的信息后，把它注册到内核中。
>
> 这两步完成后，内核会在合适的时机，调用struct device_driver变量中的probe、remove、suspend、resume等回调函数，从而触发或者终结设备驱动的执行。而所有的驱动程序逻辑，都会由这些回调函数实现，此时，驱动开发者眼中便不再有“设备模型”，转而只关心驱动本身的实现。(http://www.wowotech.net/device_model/device_and_driver.html)

补充

>1. 一般情况下，Linux驱动开发很少直接使用device和device_driver，因为内核在它们之上又封装了一层，如soc device、platform device等等，而这些层次提供的接口更为简单、易用
>
>2. 内核提供很多struct device结构的操作接口（具体可以参考include/linux/device.h和drivers/base/core.c的代码），主要包括初始化（device_initialize）、注册到内核（device_register）、分配存储空间+初始化+注册到内核（device_create）等等，可以根据需要使用。
>
>3. device和device_driver必须具备相同的名称，内核才能完成匹配操作，进而调用device_driver中的相应接口。这里的同名，作用范围是同一个bus下的所有device和device_driver。
>
>4. device和device_driver必须挂载在一个bus之下，该bus可以是实际存在的，也可以是虚拟的。
>
>5. driver开发者可以在struct device变量中，保存描述设备特征的信息，如寻址空间、依赖的GPIOs等，因为device指针会在执行probe等接口时传入，这时driver就可以根据这些信息，执行相应的逻辑操作了。
>
>   (http://www.wowotech.net/device_model/device_and_driver.html)

### probe时机

>所谓的"probe”，是指在Linux内核中，如果存在相同名称的device和device_driver（还存在其它方式），内核就会执行device_driver中的probe回调函数，而该函数就是所有driver的入口，可以执行诸如硬件设备初始化、字符设备注册、设备文件操作ops注册等动作（"remove”是它的反操作，发生在device或者device_driver任何一方从内核注销时，其原理类似，就不再单独说明了）。
>
>probe时机有如下几种：
>
>1. 将struct device类型的变量注册到内核中时自动触发（device_register，device_add，device_create_vargs，device_create）
>2. 将struct device_driver类型的变量注册到内核中时自动触发（driver_register）
>3. 手动查找同一bus下的所有device_driver，如果有和指定device同名的driver，执行probe操作（device_attach）
>4. 手动查找同一bus下的所有device，如果有和指定driver同名的device，执行probe操作（driver_attach）
>5. 自行调用driver的probe接口，并在该接口中将该driver绑定到某个device结构中----即设置dev->driver（device_bind_driver）
>
>
>
>probe动作实际是由bus模块（会在下一篇文章讲解）实现的，这不难理解：device和device_driver都是挂载在bus这根线上，因此只有bus最清楚应该为哪些device、哪些driver配对。
>
>每个bus都有一个drivers_autoprobe变量，用于控制是否在device或者driver注册时，自动probe。该变量默认为1（即自动probe），bus模块将它开放到sysfs中了，因而可在用户空间修改，进而控制probe行为。

### attribute

在设备模型kobject层，kobject/kset/ktype组成了sysfs目录及其下文件的基本框架，

内核为kset定义了对应的ktype为kset_ktype

内核为kobject定义了对应的ktype为dynamic_kobj_ktype

ktype中的release帮助释放对应的数据结构，sysfs_ops提供attribute文件的访问框架，其中attribute的访问操作函数的定义由开发者实现。attribute访问路径为vfs->kernfs_ops->sysfs_ops->attr.ops

而device和device_driver内核也为其定义了ktype。

#### device attribute

```cpp
//drivers/base/core.c
static struct kobj_type device_ktype = {
	.release	= device_release,
	.sysfs_ops	= &dev_sysfs_ops,
	.namespace	= device_namespace,
};

static const struct sysfs_ops dev_sysfs_ops = {
	.show	= dev_attr_show,
	.store	= dev_attr_store,
};

static ssize_t dev_attr_show(struct kobject *kobj, struct attribute *attr,
			     char *buf)
{
	struct device_attribute *dev_attr = to_dev_attr(attr);
	struct device *dev = kobj_to_dev(kobj);
	ssize_t ret = -EIO;

	if (dev_attr->show)
		ret = dev_attr->show(dev, dev_attr, buf);
	if (ret >= (ssize_t)PAGE_SIZE) {
		print_symbol("dev_attr_show: %s returned bad count\n",
				(unsigned long)dev_attr->show);
	}
	return ret;
}
```

在device中不仅定义了新的ktype，而且还将struct attribute进行了封装（面向对象的派生）

```cpp
//include/linux/device.h
struct device_attribute {
	struct attribute	attr;
	ssize_t (*show)(struct device *dev, struct device_attribute *attr,
			char *buf);
	ssize_t (*store)(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count);
};
```

而创建一个device_attribute的方式如下：

```cpp
//include/linux/device.h
#define DEVICE_ATTR(_name, _mode, _show, _store) \
	struct device_attribute dev_attr_##_name = __ATTR(_name, _mode, _show, _store)
#define DEVICE_ATTR_RW(_name) \
	struct device_attribute dev_attr_##_name = __ATTR_RW(_name)
#define DEVICE_ATTR_RO(_name) \
	struct device_attribute dev_attr_##_name = __ATTR_RO(_name)
#define DEVICE_ATTR_WO(_name) \
	struct device_attribute dev_attr_##_name = __ATTR_WO(_name)
```

device可以通过device_create_file创建attribute文件，其底层调用的还是sysfs_create_file

#### driver_attribute

driver没有定义kobject,driver使用其匹配的device的kobj的ktype操作属性文件（待定，猜的），但是driver有自己的属性文件结构：

```cpp
//include/linux/device.h
struct driver_attribute {
	struct attribute attr;
	ssize_t (*show)(struct device_driver *driver, char *buf);
	ssize_t (*store)(struct device_driver *driver, const char *buf,
			 size_t count);
};

#define DRIVER_ATTR(_name, _mode, _show, _store) \
	struct driver_attribute driver_attr_##_name = __ATTR(_name, _mode, _show, _store)
#define DRIVER_ATTR_RW(_name) \
	struct driver_attribute driver_attr_##_name = __ATTR_RW(_name)
#define DRIVER_ATTR_RO(_name) \
	struct driver_attribute driver_attr_##_name = __ATTR_RO(_name)
#define DRIVER_ATTR_WO(_name) \
	struct driver_attribute driver_attr_##_name = __ATTR_WO(_name)
```

driver可以通过driver_create_file创建attribute文件，其底层调用的还是sysfs_create_file

#### device_type

在uevent中可以看到内核在发送uevent事件时，会查看device的device_type成员是否有uevent要发送，并添加对应的环境变量一并发送。

```cpp
//include/linux/device.h
struct device_type {
	const char *name;
	const struct attribute_group **groups;
	int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
	char *(*devnode)(struct device *dev, umode_t *mode,
			 kuid_t *uid, kgid_t *gid);
	void (*release)(struct device *dev);

	const struct dev_pm_ops *pm;
};
```

> - name表示该类型的名称，当该类型的设备添加到内核时，内核会发出"DEVTYPE=‘name’”类型的uevent，告知用户空间某个类型的设备available了
> - groups，该类型设备的公共attribute集合。设备注册时，会同时注册这些attribute。这就是面向对象中“继承”的概念
> - uevent，同理，所有相同类型的设备，会有一些共有的uevent需要发送，由该接口实现
> - devnode，devtmpfs有关的内容
> - release，如果device结构没有提供release接口，就要查询它所属的type是否提供。用于释放device变量所占的空间
>
> （http://www.wowotech.net/device_model/device_and_driver.html）

## 相关接口

### device_register->device_add

```cpp
//drivers/base/core.c
int device_register(struct device *dev)
{
	device_initialize(dev);
	return device_add(dev);
}
```

device_register调用device_initialize初始化一个struct device:

```cpp
//drivers/base/core.c
void device_initialize(struct device *dev)
{
	dev->kobj.kset = devices_kset;//初始化device的kobject的kset为devices_kset，devices_kset在devices_init函数中被初始化，在sysfs中表现为/sys/devices目录，并有自己的uevent_ops。
	kobject_init(&dev->kobj, &device_ktype);//初始化kobject，绑定ktype
	INIT_LIST_HEAD(&dev->dma_pools);
	mutex_init(&dev->mutex);
	lockdep_set_novalidate_class(&dev->mutex);
	spin_lock_init(&dev->devres_lock);
	INIT_LIST_HEAD(&dev->devres_head);
	device_pm_init(dev);
	set_dev_node(dev, -1);
#ifdef CONFIG_GENERIC_MSI_IRQ
	INIT_LIST_HEAD(&dev->msi_list);
#endif
	INIT_LIST_HEAD(&dev->links.consumers);
	INIT_LIST_HEAD(&dev->links.suppliers);
	dev->links.status = DL_DEV_NO_DRIVER;
}
```

device_initialize主要初始化struct device的kobject，绑定kset,ktype，并初始化struct device中的一些资源如锁，链表等

```cpp
//drivers/base/core.c
int device_add(struct device *dev)
{
	struct device *parent = NULL;
	struct kobject *kobj;
	struct class_interface *class_intf;
	int error = -EINVAL;
	struct kobject *glue_dir = NULL;

	dev = get_device(dev);//增加dev->kobject的引用计数
	if (!dev)
		goto done;

	if (!dev->p) {
		error = device_private_init(dev);//初始化dev->p
		if (error)
			goto done;
	}

	/*
	 * for statically allocated devices, which should all be converted
	 * some day, we need to initialize the name. We prevent reading back
	 * the name, and force the use of dev_name()
	 */
   
	if (dev->init_name) { //如果init_name被设置，就设置dev->kobj->name为init_name
		dev_set_name(dev, "%s", dev->init_name);
		dev->init_name = NULL;
	}

	/* subsystems can specify simple device enumeration */
    
	if (!dev_name(dev) && dev->bus && dev->bus->dev_name)
		dev_set_name(dev, "%s%u", dev->bus->dev_name, dev->id);//如果init_name和kobj->name都不存在，但是设备总线和总线设备名存在，就用总线设备名和设备id作为kobj->name

	if (!dev_name(dev)) {
		error = -EINVAL;
		goto name_error;
	}

	pr_debug("device: '%s': %s\n", dev_name(dev), __func__);
	
	parent = get_device(dev->parent);//get_device增加device->parent的kobject引用计数
    //根据dev->class返回此设备的父kobj节点，可能为NULL
	kobj = get_device_parent(dev, parent);
	if (kobj)
		dev->kobj.parent = kobj;

	/* use parent numa_node */
	if (parent && (dev_to_node(dev) == NUMA_NO_NODE))
		set_dev_node(dev, dev_to_node(parent));

	/* first, register with generic layer. */
	/* we require the name to be set before, and pass NULL */
    //在父设备下创建目录，名为device->init_name
	error = kobject_add(&dev->kobj, dev->kobj.parent, NULL);
	if (error) {
		glue_dir = get_glue_dir(dev);
		goto Error;
	}

	/* notify platform of device entry */
	if (platform_notify)
		platform_notify(dev);
	//创建device相关的uevent属性文件，可以查看和写入设备的uevent文件：
    //uevent_show可以查看通过kset->uevent_ops构造的环境变量(不会导致发送uevent)
    //uevent_store可以发送uevent事件
	error = device_create_file(dev, &dev_attr_uevent);
	if (error)
		goto attrError;
	/* dev->node存在，则在dev->kobj目录中创建到of_node->kobj的链接,名为of_node
     * 如果dev->class存在，则执行以下创建
     * 在dev->kobj目录中创建到class目录的链接，名为subsystem，具体效果：/sys/devices/virtual/input/mice/subsystem链接到/sys/class/input
     * 如果dev->parent存在，则在dev->kobj目录中创建到dev->parent目录的链接，名为device
     * （块设备类或者sysfs_deprecated为1不执行）创建class目录到dev->kobj的链接，链接名为dev->kobj->name,具体效果：/sys/class/input/mice链接到/sys/devices/virtual/input/mice
     */
	error = device_add_class_symlinks(dev);
	if (error)
		goto SymlinkError;
	error = device_add_attrs(dev);//在dev目录中创建属性组和属性文件
	if (error)
		goto AttrsError;
	error = bus_add_device(dev);//创建dev->bus相关符号属性文件和符号链接，将dev加入bus的device链表
	if (error)
		goto BusError;
	error = dpm_sysfs_add(dev);//创建电源管理相关属性文件
	if (error)
		goto DPMError;
	device_pm_add(dev);//电源管理相关

	if (MAJOR(dev->devt)) {//主设备号存在
		error = device_create_file(dev, &dev_attr_dev);//如果设备有主设备号，则创建dev属性文件
		if (error)
			goto DevAttrError;

		error = device_create_sys_dev_entry(dev);//如果设备有class，则在dev->class目录下创建名为"主设备号:次设备号"的符号链接，指向dev目录。没有class目录则在/sys/dev/char下创建。效果如下：
 /*
[root@imx6ull:/sys/dev/char]# cd ../../class/bdi/
[root@imx6ull:/sys/class/bdi]# ls -la
total 0
drwxr-xr-x    2 root     root             0 Jan  1 08:48 .
drwxr-xr-x   49 root     root             0 Jan  1 08:48 ..
lrwxrwxrwx    1 root     root             0 Jan  1 09:05 0:20 -> ../../devices/virtual/bdi/0:20
...
[root@imx6ull:/sys/dev/char]# ls -la
total 0
drwxr-xr-x    2 root     root             0 Jan  1 08:48 .
drwxr-xr-x    4 root     root             0 Jan  1 08:48 ..
lrwxrwxrwx    1 root     root             0 Jan  1 09:00 108:0 -> ../../devices/virtual/ppp/ppp
lrwxrwxrwx    1 root     root             0 Jan  1 09:00 10:130 -> ../../devices/soc0/soc/2000000.aips-bus/20bc000.wdog/misc/watchdog
lrwxrwxrwx    1 root     root             0 Jan  1 09:00 10:183 -> ../../devices/virtual/misc/hw_random
...
 */
		if (error)
			goto SysEntryError;

		devtmpfs_create_node(dev);//在devtmpfs文件系统下创建文件节点
	}

	/* Notify clients of device addition.  This call must come
	 * after dpm_sysfs_add() and before kobject_uevent().
	 */
	if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_ADD_DEVICE, dev);//通知dev所在的总线上的其他设备，不是所有bus都有这个机制，i2c是拥有这个机制的i2cdev_notifier

	kobject_uevent(&dev->kobj, KOBJ_ADD);//发送uevent
	bus_probe_device(dev);//进行设备驱动探测，为设备绑定驱动
	if (parent)
		klist_add_tail(&dev->p->knode_parent,
			       &parent->p->klist_children);//将dev加入到其父节点的子节点链表上

	if (dev->class) {
		mutex_lock(&dev->class->p->mutex);
		/* tie the class to the device */
		klist_add_tail(&dev->knode_class,
			       &dev->class->p->klist_devices);//将设备加入到dev->class的设备链表

		/* notify any interfaces that the device is here */
		list_for_each_entry(class_intf,
				    &dev->class->p->interfaces, node)
			if (class_intf->add_dev)
				class_intf->add_dev(dev, class_intf);//遍历class->p->interface上所有接口，调用其add_dev函数
		mutex_unlock(&dev->class->p->mutex);
	}
done:
	put_device(dev);//解除一次引用
	return error;
 SysEntryError:
	if (MAJOR(dev->devt))
		device_remove_file(dev, &dev_attr_dev);
 DevAttrError:
	device_pm_remove(dev);
	dpm_sysfs_remove(dev);
 DPMError:
	bus_remove_device(dev);
 BusError:
	device_remove_attrs(dev);
 AttrsError:
	device_remove_class_symlinks(dev);
 SymlinkError:
	device_remove_file(dev, &dev_attr_uevent);
 attrError:
	kobject_uevent(&dev->kobj, KOBJ_REMOVE);
	glue_dir = get_glue_dir(dev);
	kobject_del(&dev->kobj);
 Error:
	cleanup_glue_dir(dev, glue_dir);
	put_device(parent);
name_error:
	kfree(dev->p);
	dev->p = NULL;
	goto done;
}
```

#### device_private_init

```cpp
//struct device_private   include/linux/device.h
struct device_private {
	struct klist klist_children;//包含这个设备所有子节点的链表
	struct klist_node knode_parent;//这个设备在兄弟链表中的节点
	struct klist_node knode_driver;//这个设备在驱动链表中的节点
	struct klist_node knode_bus;//这个设备在总线链表中的节点
	struct list_head deferred_probe;//deferred_probe_list 中的条目，用于重试绑定无法获得设备所需所有资源的驱动程序； 通常是因为它取决于首先探测另一个驱动程序。
	struct device *device;//指向该结构所在的结构设备的指针
};

//device_private_init  //drivers/base/core.c
int device_private_init(struct device *dev)
{
	dev->p = kzalloc(sizeof(*dev->p), GFP_KERNEL);//内存分配
	if (!dev->p)
		return -ENOMEM;
	dev->p->device = dev;//保存p所在的device的地址
	klist_init(&dev->p->klist_children, klist_children_get,
		   klist_children_put);//初始化链表
	INIT_LIST_HEAD(&dev->p->deferred_probe);//初始化链表
	return 0;
}
```



#### get_device_parent

```cpp
//drivers/base/core.c
//获取设备的父设备的kobject
/*
如果设备有class：

1.如果设备是块设备，则根据父设备所属类是否也是块设备，返回parent->kobj或者返回块设备类的block_class.p->subsys.kobj
2.如果父设备有class，且子设备没有dev->class->ns_type,就返回parent->kobj作为父节点
3.父设备为空或者其他情况：
父设备为空：第一个满足其parent为/sys/devices/virtual的dev->class->p->glue_dirs.list kobj节点，返回此节点作为父节点
父设备不为空：第一个满足其parent为parent->kobj的dev->class->p->glue_dirs.list kobj节点，返回此节点作为父节点
如果上述节点不存在，则在/sys/devices/virtual或者parent->kobj创建名为dev->class的kobj,返回此节点作为父节点


如果设备没有class:
如果parent不存在，且dev->bus->dev_root存在，使用dev->bus->dev_root.kobj作为父节点
如果parnet存在，返回parent->kobj作为父节点

否则返回NULL
*/
static struct kobject *get_device_parent(struct device *dev,
					 struct device *parent)
{
    //dev有自己的class
	if (dev->class) {
		struct kobject *kobj = NULL;
		struct kobject *parent_kobj;
		struct kobject *k;

#ifdef CONFIG_BLOCK
        //对于块设备：
        //如果设备的class是block_class：
        //如果父设备存在并且class也是block_class，直接返回父设备kobj
    	//如果父设备不存在或者class不是block_class，直接返回block_class->p->subsys.obj
		/* block disks show up in /sys/block */
		if (sysfs_deprecated && dev->class == &block_class) {
			if (parent && parent->class == &block_class)
				return &parent->kobj;
			return &block_class.p->subsys.kobj;
		}
#endif

		/*
		 * If we have no parent, we live in "virtual".
		 * Class-devices with a non class-device as parent, live
		 * in a "glue" directory to prevent namespace collisions.
		 */
        //父设备不存在，则parent_kobj=/sys/devices/virtual目录
        //父设备存在有class并且dev->class->ns_type不存在直接返回父设备的kobj
        //否则parent_kobj取父设备kobj
		if (parent == NULL)
			parent_kobj = virtual_device_parent(dev);
        
		else if (parent->class && !dev->class->ns_type)
			return &parent->kobj;
		else
			parent_kobj = &parent->kobj;

		mutex_lock(&gdp_mutex);
		//遍历dev->class->p->glue_dirs.list,寻找父kobject为parent_kobj的kobject，如果找到就将这个kobj引用计数加1，返回这个kobject。
		/* find our class-directory at the parent and reference it */
		spin_lock(&dev->class->p->glue_dirs.list_lock);
		list_for_each_entry(k, &dev->class->p->glue_dirs.list, entry)
			if (k->parent == parent_kobj) {
				kobj = kobject_get(k);
				break;
			}
		spin_unlock(&dev->class->p->glue_dirs.list_lock);
		if (kobj) {
			mutex_unlock(&gdp_mutex);
			return kobj;
		}
		//如果在dev->class->p->glue_dirs.list找不到父kobject为parent_kobj的kobject，就在parent_kobj目录下创建一个新的kobj,名字为dev->class->name，并返回这个kobj
		/* or create a new class-directory at the parent device */
		k = class_dir_create_and_add(dev->class, parent_kobj);
		/* do not emit an uevent for this simple "glue" directory */
		mutex_unlock(&gdp_mutex);
		return k;
	}
	//在dev没有class情况下：
    //如果没有parent,且dev有bus，且dev->bus->dev_root存在，就返回dev->bus->dev_root->kobj
	/* subsystems can specify a default root directory for their devices */
	if (!parent && dev->bus && dev->bus->dev_root)
		return &dev->bus->dev_root->kobj;
	//如果dev没有class且parent存在，就返回parent->kobj
	if (parent)
		return &parent->kobj;
	return NULL;
}
```

#### device_add_class_symlinks

```cpp
static int device_add_class_symlinks(struct device *dev)
{
	struct device_node *of_node = dev_of_node(dev);
	int error;

	if (of_node) {
		error = sysfs_create_link(&dev->kobj, &of_node->kobj,"of_node");//在dev->kobj目录中创建一个名为of_node的链接，链接指向of_node->kobj目录
		if (error)
			dev_warn(dev, "Error %d creating of_node link\n",error);
		/* An error here doesn't warrant bringing down the device */
	}

	if (!dev->class)
		return 0;

	error = sysfs_create_link(&dev->kobj,
				  &dev->class->p->subsys.kobj,
				  "subsystem");//在dev->kobj目录创建一个名为subsystem的符号链接，链接指向dev->class->p->subsys.kobj目录
	if (error)
		goto out_devnode;

	if (dev->parent && device_is_not_partition(dev)) {
		error = sysfs_create_link(&dev->kobj, &dev->parent->kobj,
					  "device");//在dev->kobj目录创建一个名为device的符号链接，指向dev->parent->kobj目录
		if (error)
			goto out_subsys;
	}

#ifdef CONFIG_BLOCK
	/* /sys/block has directories and does not need symlinks */
	if (sysfs_deprecated && dev->class == &block_class)
		return 0;
#endif

	/* link in the class directory pointing to the device */
	error = sysfs_create_link(&dev->class->p->subsys.kobj,
				  &dev->kobj, dev_name(dev));
	if (error)
		goto out_device;

	return 0;

out_device:
	sysfs_remove_link(&dev->kobj, "device");

out_subsys:
	sysfs_remove_link(&dev->kobj, "subsystem");
out_devnode:
	sysfs_remove_link(&dev->kobj, "of_node");
	return error;
}
```

##### sysfs_create_link

```cpp
//fs/sysfs/symlink.c
//在kobj目录创建一个名为name的符号链接，指向target目录
int sysfs_create_link(struct kobject *kobj, struct kobject *target,
		      const char *name)
{
	return sysfs_do_create_link(kobj, target, name, 1);
}
```

#### device_add_attrs

```cpp
static int device_add_attrs(struct device *dev)
{
	struct class *class = dev->class;
	const struct device_type *type = dev->type;
	int error;

	if (class) {
		error = device_add_groups(dev, class->dev_groups);//在dev目录中创建class的属性组
		if (error)
			return error;
	}

	if (type) {
		error = device_add_groups(dev, type->groups);//在dev目录中创建type的属性组
		if (error)
			goto err_remove_class_groups;
	}

	error = device_add_groups(dev, dev->groups);//在dev目录中创建dev的属性组
	if (error)
		goto err_remove_type_groups;

	if (device_supports_offline(dev) && !dev->offline_disabled) {
		error = device_create_file(dev, &dev_attr_online);//在dev目录中创建online属性文件
		if (error)
			goto err_remove_dev_groups;
	}

	return 0;

 err_remove_dev_groups:
	device_remove_groups(dev, dev->groups);
 err_remove_type_groups:
	if (type)
		device_remove_groups(dev, type->groups);
 err_remove_class_groups:
	if (class)
		device_remove_groups(dev, class->dev_groups);

	return error;
}
```

##### device_add_groups

```cpp
int device_add_groups(struct device *dev, const struct attribute_group **groups)
{
	return sysfs_create_groups(&dev->kobj, groups);
}

int sysfs_create_groups(struct kobject *kobj,
			const struct attribute_group **groups)
{
	int error = 0;
	int i;

	if (!groups)
		return 0;

	for (i = 0; groups[i]; i++) {
		error = sysfs_create_group(kobj, groups[i]);
		if (error) {
			while (--i >= 0)
				sysfs_remove_group(kobj, groups[i]);
			break;
		}
	}
	return error;
}
```

device_add_groups调用sysfs_create_group，该函数根据attribute_group的项数，创建多个属性组，结构体struct attribute_group如下：

```cpp
struct attribute_group {
	const char		*name;//属性组名称
	umode_t			(*is_visible)(struct kobject *,
					      struct attribute *, int);
	umode_t			(*is_bin_visible)(struct kobject *,
						  struct bin_attribute *, int);
	struct attribute	**attrs;//属性组中的属性文件（每个attr代表一个文件，可能有多个文件）
	struct bin_attribute	**bin_attrs;//同上，二进制文件，字节流
};
```

sys_create_group调用internal_create_group:

```cpp
int sysfs_create_group(struct kobject *kobj,
		       const struct attribute_group *grp)
{
	return internal_create_group(kobj, 0, grp);
}

static int internal_create_group(struct kobject *kobj, int update,
				 const struct attribute_group *grp)
{
	struct kernfs_node *kn;
	int error;

	BUG_ON(!kobj || (!update && !kobj->sd));

	/* Updates may happen before the object has been instantiated */
	if (unlikely(update && !kobj->sd))
		return -EINVAL;
	if (!grp->attrs && !grp->bin_attrs) {
		WARN(1, "sysfs: (bin_)attrs not set by subsystem for group: %s/%s\n",
			kobj->name, grp->name ?: "");
		return -EINVAL;
	}
	if (grp->name) {//如果属性组有名字
		kn = kernfs_create_dir(kobj->sd, grp->name,
				       S_IRWXU | S_IRUGO | S_IXUGO, kobj);//在kobj目录（也就是dev目录）创建一个属性组目录，目录名为grp->name，目录使用kn描述，kn->priv=kobj
		if (IS_ERR(kn)) {
			if (PTR_ERR(kn) == -EEXIST)
				sysfs_warn_dup(kobj->sd, grp->name);
			return PTR_ERR(kn);
		}
	} else//如果属性组没有名字，则会直接在dev目录中创建此属性组内的所有属性文件
		kn = kobj->sd;
	kernfs_get(kn);
	error = create_files(kn, kobj, grp, update);//以kn作为parent,在属性组grp中创建属性文件
	if (error) {
		if (grp->name)
			kernfs_remove(kn);
	}
	kernfs_put(kn);
	return error;
}
```

create_files函数根据在属性组目录下创建组内的所有属性文件：

```cpp
static int create_files(struct kernfs_node *parent, struct kobject *kobj,
			const struct attribute_group *grp, int update)
{
	struct attribute *const *attr;
	struct bin_attribute *const *bin_attr;
	int error = 0, i;

	if (grp->attrs) {
		for (i = 0, attr = grp->attrs; *attr && !error; i++, attr++) {//遍历属性文件
			umode_t mode = (*attr)->mode;

			/*
			 * In update mode, we're changing the permissions or
			 * visibility.  Do this by first removing then
			 * re-adding (if required) the file.
			 */
			if (update)
				kernfs_remove_by_name(parent, (*attr)->name);
			if (grp->is_visible) {
				mode = grp->is_visible(kobj, *attr, i);
				if (!mode)
					continue;
			}

			WARN(mode & ~(SYSFS_PREALLOC | 0664),
			     "Attribute %s: Invalid permissions 0%o\n",
			     (*attr)->name, mode);

			mode &= SYSFS_PREALLOC | 0664;
			error = sysfs_add_file_mode_ns(parent, *attr, false,
						       mode, NULL);//创建属性文件，parent->priv=kobj，在访问属性文件时不用担心因为属性文件在属性组目录中而找不到kobj->ktype->sysfs_ops
			if (unlikely(error))
				break;
		}
		if (error) {
			remove_files(parent, grp);
			goto exit;
		}
	}

	if (grp->bin_attrs) {
		for (i = 0, bin_attr = grp->bin_attrs; *bin_attr; i++, bin_attr++) {
			umode_t mode = (*bin_attr)->attr.mode;

			if (update)
				kernfs_remove_by_name(parent,
						(*bin_attr)->attr.name);
			if (grp->is_bin_visible) {
				mode = grp->is_bin_visible(kobj, *bin_attr, i);
				if (!mode)
					continue;
			}

			WARN(mode & ~(SYSFS_PREALLOC | 0664),
			     "Attribute %s: Invalid permissions 0%o\n",
			     (*bin_attr)->attr.name, mode);

			mode &= SYSFS_PREALLOC | 0664;
			error = sysfs_add_file_mode_ns(parent,
					&(*bin_attr)->attr, true,
					mode, NULL);
			if (error)
				break;
		}
		if (error)
			remove_files(parent, grp);
	}
exit:
	return error;
}
```

#### bus_add_device

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

#### device_create_sys_dev_entry

```cpp
static int device_create_sys_dev_entry(struct device *dev)
{
	struct kobject *kobj = device_to_dev_kobj(dev);
	int error = 0;
	char devt_str[15];

	if (kobj) {
		format_dev_t(devt_str, dev->devt);//主:次
		error = sysfs_create_link(kobj, &dev->kobj, devt_str);
	}

	return error;
}
static struct kobject *device_to_dev_kobj(struct device *dev)
{
	struct kobject *kobj;

	if (dev->class)
		kobj = dev->class->dev_kobj;
	else
		kobj = sysfs_dev_char_kobj;

	return kobj;
}
#define format_dev_t(buffer, dev)					\
	({								\
		sprintf(buffer, "%u:%u", MAJOR(dev), MINOR(dev));	\
		buffer;							\
	})
```

#### devtmpfs_create_node

```cpp
int devtmpfs_create_node(struct device *dev)
{
	const char *tmp = NULL;
	struct req req;

	if (!thread)
		return 0;

	req.mode = 0;
	req.uid = GLOBAL_ROOT_UID;
	req.gid = GLOBAL_ROOT_GID;
	req.name = device_get_devnode(dev, &req.mode, &req.uid, &req.gid, &tmp);
	if (!req.name)
		return -ENOMEM;

	if (req.mode == 0)
		req.mode = 0600;
	if (is_blockdev(dev))
		req.mode |= S_IFBLK;
	else
		req.mode |= S_IFCHR;

	req.dev = dev;

	init_completion(&req.done);//初始化完成量

	spin_lock(&req_lock);//获取自旋锁
    
    //将请求放入请求队列头
	req.next = requests;
	requests = &req;
	spin_unlock(&req_lock);//释放自旋锁

	wake_up_process(thread);//唤醒thread
	wait_for_completion(&req.done);//等待完成

	kfree(tmp);

	return req.err;
}
```

thread和requests是全局变量：

```cpp
//drivers/base/devtmpfs.c
static struct task_struct *thread;

static struct req {
	struct req *next;
	struct completion done;
	int err;
	const char *name;
	umode_t mode;	/* 0 => delete */
	kuid_t uid;
	kgid_t gid;
	struct device *dev;
} *requests;
```

devtmpfs初始化：

```cpp
int __init devtmpfs_init(void)
{
	int err = register_filesystem(&dev_fs_type);
	if (err) {
		printk(KERN_ERR "devtmpfs: unable to register devtmpfs "
		       "type %i\n", err);
		return err;
	}

	thread = kthread_run(devtmpfsd, &err, "kdevtmpfs");//注册一个内核线程，名字为kdeftmpfs，线程工作函数为devtmpfsd
	if (!IS_ERR(thread)) {
		wait_for_completion(&setup_done);//等待线程完成
	} else {
		err = PTR_ERR(thread);
		thread = NULL;
	}

	if (err) {
		printk(KERN_ERR "devtmpfs: unable to create devtmpfs %i\n", err);
		unregister_filesystem(&dev_fs_type);
		return err;
	}

	printk(KERN_INFO "devtmpfs: initialized\n");
	return 0;
}

```

devtmpfsd如下：

```cpp
static int devtmpfsd(void *p)
{
	char options[] = "mode=0755";
	int *err = p;
	*err = sys_unshare(CLONE_NEWNS);
	if (*err)
		goto out;
	*err = sys_mount("devtmpfs", "/", "devtmpfs", MS_SILENT, options);//挂载devtmpfs文件系统，mode 755
	if (*err)
		goto out;
	sys_chdir("/.."); /* will traverse into overmounted root */
	sys_chroot(".");
	complete(&setup_done);//通知完成量完成，setup_done是全局的
	while (1) {//循环处理requests中的请求
		spin_lock(&req_lock);
		while (requests) {
			struct req *req = requests;
			requests = NULL;
			spin_unlock(&req_lock);
			while (req) {
				struct req *next = req->next;
				req->err = handle(req->name, req->mode,
						  req->uid, req->gid, req->dev);
				complete(&req->done);//通知请求完成
				req = next;
			}
			spin_lock(&req_lock);
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		spin_unlock(&req_lock);
		schedule();
	}
	return 0;
out:
	complete(&setup_done);
	return *err;
}
```

handle为实际执行请求的函数：

```cpp
static int handle(const char *name, umode_t mode, kuid_t uid, kgid_t gid,
		  struct device *dev)
{
	if (mode)//执行的请求必须有权限
		return handle_create(name, mode, uid, gid, dev);
	else
		return handle_remove(name, dev);
}



static int handle_create(const char *nodename, umode_t mode, kuid_t uid,
			 kgid_t gid, struct device *dev)
{
	struct dentry *dentry;
	struct path path;
	int err;

	dentry = kern_path_create(AT_FDCWD, nodename, &path, 0);
	if (dentry == ERR_PTR(-ENOENT)) {
		create_path(nodename);
		dentry = kern_path_create(AT_FDCWD, nodename, &path, 0);
	}
	if (IS_ERR(dentry))
		return PTR_ERR(dentry);

	err = vfs_mknod(d_inode(path.dentry), dentry, mode, dev->devt);//这里应该是在devtmpfs(也就是/dev)下创建一个设备节点，需要传入权限和主次设备号
	if (!err) {
		struct iattr newattrs;

		newattrs.ia_mode = mode;
		newattrs.ia_uid = uid;
		newattrs.ia_gid = gid;
		newattrs.ia_valid = ATTR_MODE|ATTR_UID|ATTR_GID;
		inode_lock(d_inode(dentry));
		notify_change(dentry, &newattrs, NULL);
		inode_unlock(d_inode(dentry));

		/* mark as kernel-created inode */
		d_inode(dentry)->i_private = &thread;
	}
	done_path_create(&path, dentry);
	return err;
}
```

#### bus_probe_device

```cpp
void bus_probe_device(struct device *dev)
{
	struct bus_type *bus = dev->bus;
	struct subsys_interface *sif;

	if (!bus)//device必须挂载到bus才能开始匹配
		return;

	if (bus->p->drivers_autoprobe)//如果bus允许自动探测，则进行device和driver的匹配
		device_initial_probe(dev);

	mutex_lock(&bus->p->mutex);
	list_for_each_entry(sif, &bus->p->interfaces, node)
		if (sif->add_dev)
			sif->add_dev(dev, sif);
	mutex_unlock(&bus->p->mutex);
}
```



```cpp
void device_initial_probe(struct device *dev)
{
	__device_attach(dev, true);
}
```

##### __device_attach

```cpp
static int __device_attach(struct device *dev, bool allow_async)
{
	int ret = 0;

	device_lock(dev);
	if (dev->driver) {//设备指定了driver
		if (device_is_bound(dev)) {//查看该设备是否已经和某个驱动绑定，没有绑定才能执行后续操作
			ret = 1;
			goto out_unlock;
		}
		ret = device_bind_driver(dev);//将dev和dev->driver进行绑定
		if (ret == 0)
			ret = 1;
		else {
			dev->driver = NULL;
			ret = 0;
		}
	} else {//设备还没有指定driver
		struct device_attach_data data = {
			.dev = dev,
			.check_async = allow_async,//允许异步探测
			.want_async = false,//不希望异步探测
		};//异步探测相关

		if (dev->parent)
			pm_runtime_get_sync(dev->parent);//电源管理相关

		ret = bus_for_each_drv(dev->bus, NULL, &data,
					__device_attach_driver);//遍历bus->p->klist_drivers链表，尝试和dev匹配
		if (!ret && allow_async && data.have_async) {
			/*
			 * If we could not find appropriate driver
			 * synchronously and we are allowed to do
			 * async probes and there are drivers that
			 * want to probe asynchronously, we'll
			 * try them.
			 */
			dev_dbg(dev, "scheduling asynchronous probe\n");
			get_device(dev);
			async_schedule(__device_attach_async_helper, dev);
		} else {
			pm_request_idle(dev);
		}

		if (dev->parent)
			pm_runtime_put(dev->parent);
	}
out_unlock:
	device_unlock(dev);
	return ret;
}
```

##### device_bind_driver

```cpp
int device_bind_driver(struct device *dev)
{
	int ret;

	ret = driver_sysfs_add(dev);
	if (!ret)
		driver_bound(dev);
	else if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_DRIVER_NOT_BOUND, dev);
	return ret;
}


static int driver_sysfs_add(struct device *dev)
{
	int ret;

	if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_BIND_DRIVER, dev);//通知

	ret = sysfs_create_link(&dev->driver->p->kobj, &dev->kobj,
			  kobject_name(&dev->kobj));//在driver->p->kobj下创建指向dev目录的符号链接，名为dev->kobj
	if (ret == 0) {
		ret = sysfs_create_link(&dev->kobj, &dev->driver->p->kobj,
					"driver");//和上面相反
		if (ret)
			sysfs_remove_link(&dev->driver->p->kobj,
					kobject_name(&dev->kobj));
	}
	return ret;
}



static void driver_bound(struct device *dev)
{
	if (device_is_bound(dev)) {
		printk(KERN_WARNING "%s: device %s already bound\n",
			__func__, kobject_name(&dev->kobj));
		return;
	}

	pr_debug("driver: '%s': %s: bound to device '%s'\n", dev->driver->name,
		 __func__, dev_name(dev));

	klist_add_tail(&dev->p->knode_driver, &dev->driver->p->klist_devices);//将dev->p->knode_driver加入dev->driver->p->klist_devices链表, 一个device只能对应一个dirver,一个dirver可以对应多个devices
	device_links_driver_bound(dev);

	device_pm_check_callbacks(dev);

	/*
	 * Make sure the device is no longer in one of the deferred lists and
	 * kick off retrying all pending devices
	 */
	driver_deferred_probe_del(dev);
	driver_deferred_probe_trigger();

	if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_BOUND_DRIVER, dev);
}
```

#### bus_for_each_drv

```cpp
int bus_for_each_drv(struct bus_type *bus, struct device_driver *start,
		     void *data, int (*fn)(struct device_driver *, void *))
{
	struct klist_iter i;
	struct device_driver *drv;
	int error = 0;

	if (!bus)
		return -EINVAL;

	klist_iter_init_node(&bus->p->klist_drivers, &i,
			     start ? &start->p->knode_bus : NULL);
	while ((drv = next_driver(&i)) && !error)
		error = fn(drv, data);////遍历bus->p->klist_drivers链表，对里面的dirver执行fn函数，传入drv和data(data中包含dev)
	klist_iter_exit(&i);
	return error;
}
```

#### __device_attach_driver

```cpp
static int __device_attach_driver(struct device_driver *drv, void *_data)
{
	struct device_attach_data *data = _data;
	struct device *dev = data->dev;
	bool async_allowed;
	int ret;

	/*
	 * Check if device has already been claimed. This may
	 * happen with driver loading, device discovery/registration,
	 * and deferred probe processing happens all at once with
	 * multiple threads.
	 */
	if (dev->driver)
		return -EBUSY;

	ret = driver_match_device(drv, dev);//调用drv->bus->match函数进行匹配
	if (ret == 0) {
		/* no match */
		return 0;
	} else if (ret == -EPROBE_DEFER) {
		dev_dbg(dev, "Device match requests probe deferral\n");
		driver_deferred_probe_add(dev);
	} else if (ret < 0) {
		dev_dbg(dev, "Bus failed to match device: %d", ret);
		return ret;
	} /* ret > 0 means positive match */

	async_allowed = driver_allows_async_probing(drv);//驱动是否支持异步探测

	if (async_allowed)
		data->have_async = true;

	if (data->check_async && async_allowed != data->want_async)
		return 0;//如果支持异步探测这里直接返回

	return driver_probe_device(drv, dev);
}
```

#### driver_probe_device

```cpp
int driver_probe_device(struct device_driver *drv, struct device *dev)
{
	int ret = 0;

	if (!device_is_registered(dev))//dev必须已经位于sysfs中
		return -ENODEV;

	pr_debug("bus: '%s': %s: matched device %s with driver %s\n",
		 drv->bus->name, __func__, dev_name(dev), drv->name);

	if (dev->parent)
		pm_runtime_get_sync(dev->parent);

	pm_runtime_barrier(dev);
	ret = really_probe(dev, drv);
	pm_request_idle(dev);

	if (dev->parent)
		pm_runtime_put(dev->parent);

	return ret;
}
```

##### really_probe

```cpp
static int really_probe(struct device *dev, struct device_driver *drv)
{
	int ret = -EPROBE_DEFER;
	int local_trigger_count = atomic_read(&deferred_trigger_count);
	bool test_remove = IS_ENABLED(CONFIG_DEBUG_TEST_DRIVER_REMOVE) &&
			   !drv->suppress_bind_attrs;

	if (defer_all_probes) {
		/*
		 * Value of defer_all_probes can be set only by
		 * device_defer_all_probes_enable() which, in turn, will call
		 * wait_for_device_probe() right after that to avoid any races.
		 */
		dev_dbg(dev, "Driver %s force probe deferral\n", drv->name);
		driver_deferred_probe_add(dev);
		return ret;
	}

	ret = device_links_check_suppliers(dev);
	if (ret)
		return ret;

	atomic_inc(&probe_count);
	pr_debug("bus: '%s': %s: probing driver %s with device %s\n",
		 drv->bus->name, __func__, drv->name, dev_name(dev));
	WARN_ON(!list_empty(&dev->devres_head));

re_probe:
	dev->driver = drv;//现将driver指定到dev->driver

	/* If using pinctrl, bind pins now before probing */
	ret = pinctrl_bind_pins(dev);//pinctrl子系统相关
	if (ret)
		goto pinctrl_bind_failed;

	if (driver_sysfs_add(dev)) {//执行了类似上面dev->driver已经存在的驱动绑定流程，创建drv和dev目录之间的链接
		printk(KERN_ERR "%s: driver_sysfs_add(%s) failed\n",
			__func__, dev_name(dev));
		goto probe_failed;
	}

	if (dev->pm_domain && dev->pm_domain->activate) {//电源管理相关
		ret = dev->pm_domain->activate(dev);
		if (ret)
			goto probe_failed;
	}

	/*
	 * Ensure devices are listed in devices_kset in correct order
	 * It's important to move Dev to the end of devices_kset before
	 * calling .probe, because it could be recursive and parent Dev
	 * should always go first
	 */
    //确保设备按照正确的顺序列在devices_kset中。在调用.probe之前将device移动到devices_kset的末尾非常重要，因为它可能是递归的，并且父device应该始终先行。最后注册的的必须在全部的device链表的末尾
	devices_kset_move_last(dev);//将dev节点移动到devices_kset的最后，在kobject_add时kobject会把自己加入到kobj->kset的list链表中，这里的kset就是devices_kset，这里会取出再放到链表最后

	if (dev->bus->probe) {
		ret = dev->bus->probe(dev);//如果bus->probe存在，则执行,优先使用bus->probe
		if (ret)
			goto probe_failed;
	} else if (drv->probe) {
		ret = drv->probe(dev);//否则执行drv的probe
		if (ret)
			goto probe_failed;
	}

	if (test_remove) {
		test_remove = false;

		if (dev->bus->remove)
			dev->bus->remove(dev);
		else if (drv->remove)
			drv->remove(dev);

		devres_release_all(dev);
		driver_sysfs_remove(dev);
		dev->driver = NULL;
		dev_set_drvdata(dev, NULL);
		if (dev->pm_domain && dev->pm_domain->dismiss)
			dev->pm_domain->dismiss(dev);
		pm_runtime_reinit(dev);

		goto re_probe;
	}

	pinctrl_init_done(dev);//pinctrl相关

	if (dev->pm_domain && dev->pm_domain->sync)
		dev->pm_domain->sync(dev);//电源管理相关

	driver_bound(dev);//将dev->knode_driver加入到drv->p->klist_devices
	ret = 1;
	pr_debug("bus: '%s': %s: bound device %s to driver %s\n",
		 drv->bus->name, __func__, dev_name(dev), drv->name);
	goto done;

probe_failed:
	if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_DRIVER_NOT_BOUND, dev);
pinctrl_bind_failed:
	device_links_no_driver(dev);
	devres_release_all(dev);
	driver_sysfs_remove(dev);
	dev->driver = NULL;
	dev_set_drvdata(dev, NULL);
	if (dev->pm_domain && dev->pm_domain->dismiss)
		dev->pm_domain->dismiss(dev);
	pm_runtime_reinit(dev);

	switch (ret) {
	case -EPROBE_DEFER:
		/* Driver requested deferred probing */
		dev_dbg(dev, "Driver %s requests probe deferral\n", drv->name);
		driver_deferred_probe_add(dev);
		/* Did a trigger occur while probing? Need to re-trigger if yes */
		if (local_trigger_count != atomic_read(&deferred_trigger_count))
			driver_deferred_probe_trigger();
		break;
	case -ENODEV:
	case -ENXIO:
		pr_debug("%s: probe of %s rejects match %d\n",
			 drv->name, dev_name(dev), ret);
		break;
	default:
		/* driver matched but the probe failed */
		printk(KERN_WARNING
		       "%s: probe of %s failed with error %d\n",
		       drv->name, dev_name(dev), ret);
	}
	/*
	 * Ignore errors returned by ->probe so that the next driver can try
	 * its luck.
	 */
	ret = 0;
done:
	atomic_dec(&probe_count);
	wake_up(&probe_waitqueue);
	return ret;
}
```

#### __device_attach_async_helper

针对异步probe

```cpp
static void __device_attach_async_helper(void *_dev, async_cookie_t cookie)
{
	struct device *dev = _dev;
	struct device_attach_data data = {
		.dev		= dev,
		.check_async	= true,
		.want_async	= true,
	};

	device_lock(dev);

	if (dev->parent)
		pm_runtime_get_sync(dev->parent);

	bus_for_each_drv(dev->bus, NULL, &data, __device_attach_driver);//执行和同步相同的匹配流程
	dev_dbg(dev, "async probe completed\n");

	pm_request_idle(dev);

	if (dev->parent)
		pm_runtime_put(dev->parent);

	device_unlock(dev);

	put_device(dev);
}
```

### driver_register

```cpp
//drivers/base/driver.c
int driver_register(struct device_driver *drv)
{
	int ret;
	struct device_driver *other;

	BUG_ON(!drv->bus->p);

	if ((drv->bus->probe && drv->probe) ||
	    (drv->bus->remove && drv->remove) ||
	    (drv->bus->shutdown && drv->shutdown))
		printk(KERN_WARNING "Driver '%s' needs updating - please use "
			"bus_type methods\n", drv->name);//一般bus->probe和drv->probe不会同时存在，因为内核在执行时优先使用bus的其次使用drv的，内核给出警告

	other = driver_find(drv->name, drv->bus);//查找bus下是否已经存在名为drv->name的device_driver
	if (other) {//如果存在就退出
		printk(KERN_ERR "Error: Driver '%s' is already registered, "
			"aborting...\n", drv->name);
		return -EBUSY;
	}

	ret = bus_add_driver(drv);
	if (ret)
		return ret;
	ret = driver_add_groups(drv, drv->groups);//在driver目录创建相关属性组文件
	if (ret) {
		bus_remove_driver(drv);
		return ret;
	}
	kobject_uevent(&drv->p->kobj, KOBJ_ADD);//发送uevent

	return ret;
}

struct device_driver *driver_find(const char *name, struct bus_type *bus)
{
	struct kobject *k = kset_find_obj(bus->p->drivers_kset, name);//在bus的driver链表中寻找名为name的kobject
	struct driver_private *priv;

	if (k) {
		/* Drop reference added by kset_find_obj() */
		kobject_put(k);
		priv = to_driver(k);//通过device_driver->p->kobj获取p，而p又保存了device_driver
		return priv->driver;//范围device_driver
	}
	return NULL;
}

struct kobject *kset_find_obj(struct kset *kset, const char *name)
{
	struct kobject *k;
	struct kobject *ret = NULL;

	spin_lock(&kset->list_lock);

	list_for_each_entry(k, &kset->list, entry) {
		if (kobject_name(k) && !strcmp(kobject_name(k), name)) {
			ret = kobject_get_unless_zero(k);
			break;
		}
	}

	spin_unlock(&kset->list_lock);
	return ret;
}
```

#### bus_add_driver

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

#### driver_attach

```cpp
int driver_attach(struct device_driver *drv)
{
	return bus_for_each_dev(drv->bus, NULL, drv, __driver_attach);//遍历drv->bus上的device链表，对所有device执行attach函数，传入device和driver
}
```

##### __driver_attach

```cpp
static int __driver_attach(struct device *dev, void *data)
{
	struct device_driver *drv = data;
	int ret;

	/*
	 * Lock device and try to bind to it. We drop the error
	 * here and always return 0, because we need to keep trying
	 * to bind to devices and some drivers will return an error
	 * simply if it didn't support the device.
	 *
	 * driver_probe_device() will spit a warning if there
	 * is an error.
	 */

	ret = driver_match_device(drv, dev);//对dev和drv调用bus的match函数，成功才会继续执行，否则退出
	if (ret == 0) {
		/* no match */
		return 0;
	} else if (ret == -EPROBE_DEFER) {
		dev_dbg(dev, "Device match requests probe deferral\n");
		driver_deferred_probe_add(dev);
	} else if (ret < 0) {
		dev_dbg(dev, "Bus failed to match device: %d", ret);
		return ret;
	} /* ret > 0 means positive match */

	if (dev->parent)	/* Needed for USB */
		device_lock(dev->parent);
	device_lock(dev);
	if (!dev->driver)
		driver_probe_device(drv, dev);//尝试对drv和dev进行匹配
	device_unlock(dev);
	if (dev->parent)
		device_unlock(dev->parent);

	return 0;
}
```

#### module_add_driver

```cpp
struct module_kobject {
	struct kobject kobj;
	struct module *mod;
	struct kobject *drivers_dir;
	struct module_param_attrs *mp;
	struct completion *kobj_completion;
};
//获取module_kobject，保存到driver，创建符号链接
void module_add_driver(struct module *mod, struct device_driver *drv)
{
	char *driver_name;
	int no_warn;
	struct module_kobject *mk = NULL;

	if (!drv)
		return;

	if (mod)//driver有定义owner，就直接使用owner的mkobj
		mk = &mod->mkobj;
	else if (drv->mod_name) {//否则获取drv的mod_name
		struct kobject *mkobj;

		/* Lookup built-in module entry in /sys/modules */
		mkobj = kset_find_obj(module_kset, drv->mod_name);//尝试在module_kset链表中找到与drv->mod_name同名的kobject，module_kset在sysfs下表现为目录/sys/module
		if (mkobj) {
			mk = container_of(mkobj, struct module_kobject, kobj);//通过kobj获取module_kobject
			/* remember our module structure */
			drv->p->mkobj = mk;//保存找到的module_kobj到drv
			/* kset_find_obj took a reference */
			kobject_put(mkobj);
		}
	}

	if (!mk)
		return;

	/* Don't check return codes; these calls are idempotent */
	no_warn = sysfs_create_link(&drv->p->kobj, &mk->kobj, "module");//在drv->p->kobj目录下创建名为module的符号链接，只想mk->kobj目录
	driver_name = make_driver_name(drv);//创建一个name，格式为drv->bus->name:drv->name
	if (driver_name) {
		module_create_drivers_dir(mk);//在mk下创建drivers目录
		no_warn = sysfs_create_link(mk->drivers_dir, &drv->p->kobj,
					    driver_name);//在刚才创建的drivers目录下创建名为driver_name的符号链接，指向driver目录
		kfree(driver_name);
	}
}


static void module_create_drivers_dir(struct module_kobject *mk)
{
	static DEFINE_MUTEX(drivers_dir_mutex);

	mutex_lock(&drivers_dir_mutex);
	if (mk && !mk->drivers_dir)//在mkobj目录下创建名为drivers的目录，并保存到mkobj->drivers_dir
		mk->drivers_dir = kobject_create_and_add("drivers", &mk->kobj);
	mutex_unlock(&drivers_dir_mutex);
}

```



## 总结

### device_register

向内核注册一个device，即调用device_register，主要执行步骤：

1. 初始化device继承自设备模型的kobject，这样device在sysfs下的表现就是kobj目录：在注册device前，初始化kobject，绑定devices_kset（这样注册到内核的dev->kobject会在/sys/devices下呈现），device_ktype提供device属性文件的访问，device的属性文件继承自attribute

2. 调用device_add，该函数做以下工作：
   1. device->parent的kobject引用计数
   
   2. 在device目录下创建uevent属性文件，对应的show/store可以查看和发送uevent事件
   
      sysfs下表现为：
   
      ```shell
      zhhhhhh@ubuntu:/sys/devices/platform/serial8250$ ls uevent
      uevent
      
      ```
   
      
   
   3. 确定sysfs中dev目录的位置，即确定dev->kobj.parent，这个值可能是block_class.p->subsys.kobj（对块设备）、dev->parent.kobj、/sys/devices/virtual/(dev->class's name)、dev->bus->dev_root.kobj
   
   4. 调用kobject_add向sysfs注册一个kobject，名字优先使用dev->init_name，否则使用dev->bus->name:dev->id
   
      sysfs下表现为:
   
      ```shell
      zhhhhhh@ubuntu:/sys/devices/platform$ ls
       ACPI0003:00   eisa.0  'Fixed MDIO bus.0'   i8042   kgdboc   pcspkr   platform-framebuffer.0   power   reg-dummy   serial8250   uevent
      
      ```
   
      
   
   5. 在dev目录创建到dev->class目录的符号链接，名为subsystem；在dev->class->p->subsys目录下创建到dev目录的符号链接，名为dev目录名（块设备类或者sysfs_deprecated为1不执行）；在dev目录创建到dev->parent目录的符号链接，名为device(不一定会创建)
   
      sysfs下表现为：
   
      ```shell
      zhhhhhh@ubuntu:/sys/devices/virtual/input/mice$ ls -la subsystem
      lrwxrwxrwx 1 root root 0 May  1 03:15 subsystem -> ../../../../class/input
      zhhhhhh@ubuntu:/sys/class/input$ ls -la mice
      lrwxrwxrwx 1 root root 0 May  1 03:15 mice -> ../../devices/virtual/input/mice
      
      ```
   
      
   
   6. 在dev目录创建来自dev->class->dev_groups、dev->type、dev->groups的属性组文件，以及online属性（很少看到这些属性文件）
   
   7. 添加来自bus->dev_attrs的属性文件，来自bus->dev_groups的属性组,在bus->p->devices_kset->kobj目录下创建与dev同名的符号链接，指向dev目录，将dev->p->knode_bus加入bus的p->klist_devices链表（属性文件很少看到）
   
   8. 如果设备有主设备号，则：在dev目录下创建名为"dev"的属性文件，如果设备有class，则在dev->class目录下创建名为"主设备号:次设备号"的符号链接，指向dev目录。没有class目录则在/sys/dev/char下创建；调用devtmpfs_create_node在devtmpfs文件系统下创建文件节点，在linux中devympfs虚拟文件系统通常挂载在/dev下，因此这个函数会在/dev下创建一个设备节点，可以通过驱动的file_operations访问这个节点
   
      sysfs下表现为
   
      ```cpp
      zhhhhhh@ubuntu:/sys/devices/virtual/sound/seq$ ls dev*
      dev
      zhhhhhh@ubuntu:/sys/class/bdi$ ls -la 0:49
      lrwxrwxrwx 1 root root 0 May  1 03:16 0:49 -> ../../devices/virtual/bdi/0:49
      zhhhhhh@ubuntu:/sys/dev/char$ ls -la 10:1
      lrwxrwxrwx 1 root root 0 May  1 03:21 10:1 -> ../../devices/virtual/misc/psaux
      
      
      ```
   
      
   
   9. 通知设备所在总线有设备加入
   
   10. 调用kobject_uevent发送uevent
   
   11. 对已经挂载在总线的设备：如果bus->p->drivers_autoprobe即自动探测使能，则尝试探测总线上的设备:
       1. 如果设备已经指定了驱动，即dev->driver，则尝试和这个驱动匹配：通知总线有设备绑定驱动，在driver目录下创建指向dev的符号链接，名为dev目录名；在dev目录创建指向driver目录的符号链接，名为driver；将dev->p->knode_driver加入dev->driver->p->klist_devices链表, 一个device只能对应一个dirver,一个dirver可以对应多个devices
   
          sysfs下表现为：
   
          ```shell
          zhhhhhh@ubuntu:/sys/devices/platform/serial8250$ ls -la driver
          lrwxrwxrwx 1 root root 0 May  1 03:16 driver -> ../../../bus/platform/drivers/serial8250
          zhhhhhh@ubuntu:/sys/devices/platform/serial8250/driver$ ls -la serial8250
          lrwxrwxrwx 1 root root 0 May  1 10:36 serial8250 -> ../../../../devices/platform/serial8250
          
          ```
   
          
   
       2. 如果设备没有指定驱动，就遍历bus->p->klist_drivers链表上的所有驱动，尝试和设备匹配：首先会调用drv->bus->match函数进行匹配，不成功就会退出；匹配成功后调用really_probe进行匹配，该函数会将dev->driver指定为当前匹配的driver，绑定pinctrl相关内容，执行1中的创建drv和dev目录之间的链接，如果bus->probe存在，则执行bus->probe函数，否则如果drv->probe存在则执行drv->probe函数，一切执行成功，就将dev->p->knode_driver加入dev->driver->p->klist_devices链表
   
   12. 如果dev有parent，将dev加入到其父节点的子节点链表上
   
   13. 如果dev有class，将设备加入到dev->class的设备链表，遍历class->p->interface上所有接口，调用其add_dev函数

sysfs表现：



### driver_register

向内核注册一个driver，即调用driver_register，主要执行步骤：

1. 查找bus下是否已经存在名为drv->name的device_driver，如果有直接退出，防止重复添加

2. 分配并初始化driver的driver_private，包括：
   1. 初始化driv的设备链表

   2. 保存device_driver结构到private

   3. 保存priv自己到device_driver

   4. 初始化private的kobj的kset为总线的p->drivers_kset

   5. 注册此kobject，绑定driver_ktype（这样可以访问driver专属的attribute），由于这个kobject没有设置parent节点，因此会使用kset作为父节点，这会导致驱动在sysfs中的表现为都在bus所对应的kset目录中
      sysfs的表现为：

      ```shell
      zhhhhhh@ubuntu:/sys/bus/platform/drivers$ ls
      acpi-fan    charger-manager     crystal_cove_pwm  ehci-platform   intel_pmc_core  parport_pc        serial8250          tps6586x-gpio           
      ```

      

   6. 将driver添加到bus的driver链表

3. 如果drv所在bus支持自动探测，即drv->bus->p->drivers_autoprobe为1，开始对driver和device进行匹配：
   1. 对dev和drv调用bus的match函数
   2. 成功匹配后，调用really_probe函数（device_register中的2.11.2）

4. 在/sys/module下创建driver相关的目录，并建立此目录到driver目录的链接

5. 在driver目录下创建相关的uevent属性文件
   sysfs下表现为

   ```shell
   zhhhhhh@ubuntu:/sys/bus/platform/drivers/serial8250$ ls uevent
   uevent
   
   ```

   

6. 在driver目录创建bus->drv_groups相关属性组

7. 如果driver支持drv->suppress_bind_attrs属性，则在driver下创建bind和unbind属性文件，这两个文件用来手动绑定和解绑驱动

   sysfs下表现为:

   ```cpp
   zhhhhhh@ubuntu:/sys/bus/platform/drivers/serial8250$ ls *bind*
   bind  unbind
   
   ```

   

8. 在driver目录创建drv->groups相关属性组

9. 发送uevent
