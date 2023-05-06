# 设备模型 class

linux-4.9.88

## 数据结构

```cpp
//include/linux/device.h
struct class {
	const char		*name;//class的名称，会在“/sys/class/”目录下体现。
	struct module		*owner;

	struct class_attribute		*class_attrs;//该class的默认attribute，会在class注册到内核时，自动在“/sys/class/xxx_class”下创建对应的attribute文件。
	const struct attribute_group	**dev_groups;//属性组
	struct kobject			*dev_kobj;//表示该class下的设备在/sys/dev/下的目录，现在一般有char和block两个，如果dev_kobj为NULL，则默认选择char

	int (*dev_uevent)(struct device *dev, struct kobj_uevent_env *env);//，当该class下有设备发生变化时，会调用class的uevent回调函数，
	char *(*devnode)(struct device *dev, umode_t *mode);

	void (*class_release)(struct class *class);//用于release自身的回调函数
	void (*dev_release)(struct device *dev);//用于release class内设备的回调函数。在device_release接口中，会依次检查Device、Device Type以及Device所在的class，是否注册release接口，如果有则调用相应的release接口release设备指针。

	int (*suspend)(struct device *dev, pm_message_t state);
	int (*resume)(struct device *dev);
	int (*shutdown)(struct device *dev);

	const struct kobj_ns_type_operations *ns_type;
	const void *(*namespace)(struct device *dev);

	const struct dev_pm_ops *pm;

	struct subsys_private *p;//类似bus中的private指针
};

```

```cpp
struct class_interface {
	struct list_head	node;
	struct class		*class;

	int (*add_dev)		(struct device *, struct class_interface *);
	void (*remove_dev)	(struct device *, struct class_interface *);
};
```

>http://www.wowotech.net/device_model/class.html
>
>struct class_interface是这样的一个结构：它允许class driver在class下有设备添加或移除的时候，调用预先设置好的回调函数（add_dev和remove_dev）。那调用它们做什么呢？想做什么都行（例如修改设备的名称），由具体的class driver实现。

## 接口

### class_register

```cpp
#define class_register(class)			\
({						\
	static struct lock_class_key __key;	\
	__class_register(class, &__key);	\
})

int __class_register(struct class *cls, struct lock_class_key *key)
{
	struct subsys_private *cp;
	int error;

	pr_debug("device class '%s': registering\n", cls->name);

	cp = kzalloc(sizeof(*cp), GFP_KERNEL);//分配private指针
	if (!cp)
		return -ENOMEM;
	klist_init(&cp->klist_devices, klist_class_dev_get, klist_class_dev_put);
	INIT_LIST_HEAD(&cp->interfaces);//初始化设备链表
	kset_init(&cp->glue_dirs);
	__mutex_init(&cp->mutex, "subsys mutex", key);
	error = kobject_set_name(&cp->subsys.kobj, "%s", cls->name);//设置class目录在sysfs中的名字
	if (error) {
		kfree(cp);
		return error;
	}

	/* set the default /sys/dev directory for devices of this class */
	if (!cls->dev_kobj)
		cls->dev_kobj = sysfs_dev_char_kobj;//设置这个类的设备目录对应的kobj默认为sysfs_dev_char_kobj，sysfs_dev_char_kobj在devices_init中被初始化，代表/sys/dev目录

#if defined(CONFIG_BLOCK)
	/* let the block class directory show up in the root of sysfs */
	if (!sysfs_deprecated || cls != &block_class)
		cp->subsys.kobj.kset = class_kset;//非块设备类的class，设置其kobj->kset为class_kset，class_kset代表/sys/class目录
#else
	cp->subsys.kobj.kset = class_kset;
#endif
	cp->subsys.kobj.ktype = &class_ktype;//ktype
	cp->class = cls;
	cls->p = cp;

	error = kset_register(&cp->subsys);//注册一个kset，呈现在/sys/class下
	if (error) {
		kfree(cp);
		return error;
	}
	error = add_class_attrs(class_get(cls));//在对应的class目录下创建来自class->class_attrs的属性文件
	class_put(cls);
	return error;
}
```

class_register向内核注册一个class，在sysfs下呈现为/sys/class/xxx目录，拥有自己的一些属性文件，class除了p里面有一个kobject，外面也有一个名为dev_kobj的kobject，代表该class下的设备在/sys/dev/下的目录，现在一般有char和block两个，如果dev_kobj为NULL，则默认选择char。



在device_reigster时，会有和class相关的调用

1. 调用device_add_class_symlinks 函数建立dev目录和dev->class目录之前的相互链接
2. 调用device_add_attrs 函数，添加dev->class->dev_groups属性组到dev目录下
3. 在device_add中，如果dev->class存在，则将dev加入class的设备链表，同时遍历class->p->interfaces链表，调用其add_dev函数





（后续可以通过input、i2c等子系统分析class，太抽象了）