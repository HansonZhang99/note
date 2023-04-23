## Linux内核通知机制

在看`i2c`子系统时，`i2c-dev.c`中调用`i2c_dev_init`函数注册`class`，并在此初始化函数中遍历`i2c`总线，为所有适配器调用`i2cdev_attach_adapter`函数，此函数会创建设备节点并绑定`fops`，这是设备启动时，内核中未模块化直接嵌入内核的`i2c`设备的注册，也仅仅只有这个函数可以为设备注册设备节点并绑定`fops`。查看上下文，除了设备初始化时被调用，还有一个地方就是`i2cdev_notifier`这个结构体的函数指针会初始化此函数。

```cpp
static int __init i2c_dev_init(void)
{
	int res;

	printk(KERN_INFO "i2c /dev entries driver\n");

	res = register_chrdev_region(MKDEV(I2C_MAJOR, 0), I2C_MINORS, "i2c");
	if (res)
		goto out;

	i2c_dev_class = class_create(THIS_MODULE, "i2c-dev");
	if (IS_ERR(i2c_dev_class)) {
		res = PTR_ERR(i2c_dev_class);
		goto out_unreg_chrdev;
	}
	i2c_dev_class->dev_groups = i2c_groups;

	/* Keep track of adapters which will be added or removed later */
	res = bus_register_notifier(&i2c_bus_type, &i2cdev_notifier);//
	if (res)
		goto out_unreg_class;

	/* Bind to already existing adapters right away */
	i2c_for_each_dev(NULL, i2cdev_attach_adapter);

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

`i2cdev_notifier`在`i2c_dev_init`中调用的`bus_register_notifier`中被传入，跟随被传入的还有`i2c`总线

```cpp
static struct notifier_block i2cdev_notifier = {
	.notifier_call = i2cdev_notifier_call,
};

static int i2cdev_notifier_call(struct notifier_block *nb, unsigned long action,
			 void *data)
{
	struct device *dev = data;

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE:
		return i2cdev_attach_adapter(dev, NULL);
	case BUS_NOTIFY_DEL_DEVICE:
		return i2cdev_detach_adapter(dev, NULL);
	}

	return 0;
}
```



```cpp
int bus_register_notifier(struct bus_type *bus, struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&bus->p->bus_notifier, nb);
}
```

`struct blocking_notifier_head bus_notifier;`是`i2c_bus`的`subsys_private`域的一个成员。

```cpp
int blocking_notifier_chain_register(struct blocking_notifier_head *nh,
		struct notifier_block *n)
{
	int ret;

	/*
	 * This code gets used during boot-up, when task switching is
	 * not yet working and interrupts must remain disabled.  At
	 * such times we must not call down_write().
	 */
	if (unlikely(system_state == SYSTEM_BOOTING))
		return notifier_chain_register(&nh->head, n);

	down_write(&nh->rwsem);
	ret = notifier_chain_register(&nh->head, n);
	up_write(&nh->rwsem);
	return ret;
}
```

会调用`notifier_chain_register`传入的是`&bus->p->bus_notifier.head`

```cpp
struct blocking_notifier_head {
	struct rw_semaphore rwsem;
	struct notifier_block __rcu *head;
};
```



```cpp
static int notifier_chain_register(struct notifier_block **nl,
		struct notifier_block *n)
{
	while ((*nl) != NULL) {
		if (n->priority > (*nl)->priority)
			break;
		nl = &((*nl)->next);
	}
	n->next = *nl;
	rcu_assign_pointer(*nl, n);
	return 0;
}
```

实际应该把`i2cdev_notifier`结构体加入到`bus->p->bus_notifier.head`链表。也就是回调函数`i2cdev_notifier_call`（里面会调用`i2cdev_attach_adapter`)被注册到了`i2c_bus`的通知链表中，猜测此函数会在有新的设备注册到`i2c_bus`上时，会触发总线通知链上的的回调函数。

在内核搜索`BUS_NOTIFY_ADD_DEVICE`，也就是`i2cdev_attach_adapter`能被调用的条件，可以找到统一设备模型的`device_add`函数中，存在此宏的传递：

```cpp
int device_add(struct device *dev)
{
	...

	/* Notify clients of device addition.  This call must come
	 * after dpm_sysfs_add() and before kobject_uevent().
	 */
	if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_ADD_DEVICE, dev);

	kobject_uevent(&dev->kobj, KOBJ_ADD);
	bus_probe_device(dev);
	if (parent)
		klist_add_tail(&dev->p->knode_parent,
			       &parent->p->klist_children);

	...
```

如果当前设备挂载在某条总线上，会调用`blocking_notifier_call_chain`函数，并传入`&dev->bus->p->bus_notifier,BUS_NOTIFY_ADD_DEVICE, dev`其中`&dev->bus->p->bus_notifier`就是总线的通知链

```cpp
int blocking_notifier_call_chain(struct blocking_notifier_head *nh,
		unsigned long val, void *v)
{
	return __blocking_notifier_call_chain(nh, val, v, -1, NULL);
}
```



```cpp
int __blocking_notifier_call_chain(struct blocking_notifier_head *nh,
				   unsigned long val, void *v,
				   int nr_to_call, int *nr_calls)
{
	int ret = NOTIFY_DONE;

	/*
	 * We check the head outside the lock, but if this access is
	 * racy then it does not matter what the result of the test
	 * is, we re-check the list after having taken the lock anyway:
	 */
	if (rcu_access_pointer(nh->head)) {
		down_read(&nh->rwsem);
		ret = notifier_call_chain(&nh->head, val, v, nr_to_call,
					nr_calls);
		up_read(&nh->rwsem);
	}
	return ret;
}
```



```cpp
static int notifier_call_chain(struct notifier_block **nl,
			       unsigned long val, void *v,
			       int nr_to_call, int *nr_calls)
{
	int ret = NOTIFY_DONE;
	struct notifier_block *nb, *next_nb;

	nb = rcu_dereference_raw(*nl);

	while (nb && nr_to_call) {
		next_nb = rcu_dereference_raw(nb->next);

#ifdef CONFIG_DEBUG_NOTIFIERS
		if (unlikely(!func_ptr_is_kernel_text(nb->notifier_call))) {
			WARN(1, "Invalid notifier called!");
			nb = next_nb;
			continue;
		}
#endif
		ret = nb->notifier_call(nb, val, v);

		if (nr_calls)
			(*nr_calls)++;

		if ((ret & NOTIFY_STOP_MASK) == NOTIFY_STOP_MASK)
			break;
		nb = next_nb;
		nr_to_call--;
	}
	return ret;
}
```



```cpp
nb->notifier_call(nb, val, v);
```

最终会调用通知链的所有函数，就会回到`i2cdev_attach_adapter`的调用，注销同理。





### 三种通知链

#### 原子通知链

```cpp
struct atomic_notifier_head {
    spinlock_t lock;
    struct notifier_block *head;
};
```

原子通知链采用的是自旋锁，通知链元素的回调函数（当事件发生时要执行的函数）只能在中断上下文中运行，而且不允许阻塞。

#### 可阻塞通知链

```cpp
struct blocking_notifier_head {
    struct rw_semaphore rwsem;
    struct notifier_block *head;
};

struct srcu_notifier_head {
    struct mutex mutex;
    struct srcu_struct srcu;
    struct notifier_block *head;
};
```

一种用信号量实现回调函数的加锁，另一种是采用互斥锁和叫做“可睡眠的读拷贝更新机制”`(Sleepable Read-Copy UpdateSleepable Read-Copy Update)`，运行在进程空间的上下文环境里。

#### 原始通知链

```cpp
struct raw_notifier_head {
    struct notifier_block *head;
};
```

没有任何安保措施，对链表的加锁和保护全部由调用者自己实现

#### 初始化方法

```cpp
ATOMIC_NOTIFIER_HEAD(name)     //定义并初始化一个名为name的原子通知链
BLOCKING_NOTIFIER_HEAD(name)   //定义并初始化一个名为name的阻塞通知链
RAW_NOTIFIER_HEAD(name)        //定义并初始化一个名为name的原始通知链
    
static struct atomic_notifier_head dock_notifier_list;
ATOMIC_INIT_NOTIFIER_HEAD(&dock_notifier_list);
```

#### 注册注销和触发

内核提供最基本的通知链的常用接口如下

```cpp
static int notifier_chain_register(struct notifier_block **nl,  struct notifier_block *n);
static int notifier_chain_unregister(struct notifier_block **nl, struct notifier_block *n);
static int __kprobes notifier_call_chain(struct notifier_block **nl, unsigned long val, void *v, int nr_to_call, int *nr_calls);   
```

封装接口

```cpp
//原子通知链
int atomic_notifier_chain_register(struct atomic_notifier_head *nh,  struct notifier_block *nb);
int atomic_notifier_chain_unregister(struct atomic_notifier_head *nh, struct notifier_block *nb);
int atomic_notifier_call_chain(struct atomic_notifier_head *nh, unsigned long val, void *v);

//可阻塞通知链
int blocking_notifier_chain_register(struct blocking_notifier_head *nh, struct notifier_block *nb);
int blocking_notifier_chain_cond_register(struct blocking_notifier_head *nh, struct notifier_block *nb);
int srcu_notifier_chain_register(struct srcu_notifier_head *nh, struct notifier_block *nb);

int blocking_notifier_call_chain(struct blocking_notifier_head *nh,unsigned long val, void *v);
int srcu_notifier_call_chain(struct srcu_notifier_head *nh, unsigned long val, void *v);

int blocking_notifier_chain_unregister(struct blocking_notifier_head *nh, struct notifier_block *nb);
int srcu_notifier_chain_unregister(struct srcu_notifier_head *nh, struct notifier_block *nb);

//原始通知链
int raw_notifier_chain_register(struct raw_notifier_head *nh, struct notifier_block *nb);
int raw_notifier_chain_unregister(struct raw_notifier_head *nh,struct notifier_block *nb);
int raw_notifier_call_chain(struct raw_notifier_head *nh, unsigned long val, void *v);
```



### 通知链的使用

1.定义并初始化一条通知链:

```cpp
static struct blocking_notifier_head dock_notifier_list;
ATOMIC_INIT_NOTIFIER_HEAD(&dock_notifier_list);
```

2.定义通知链链表成员，并初始化其对应的回调函数：

```cpp
static struct notifier_block i2cdev_notifier =
{
        .notifier_call = i2cdev_notifier_call,
        .priority = 2,//优先级，事件触发时，通知链会遍历链表，调用成员的回调函数，排列顺序优先级从小到大
};
//回调函数原型：
//typedef	int (*notifier_fn_t)(struct notifier_block *nb,unsigned long action, void *data);
```

3.注册链表成员到通知链中：

```cpp
blocking_notifier_chain_register(&dock_notifier_list, &i2cdev_notifier);
```

4.触发通知链：

```cpp
blocking_notifier_call_chain(&dock_notifier_list,BUS_NOTIFY_ADD_DEVICE, data);
```

当通知被触发后，会遍历通知链链表，依次按照优先级从小到大顺序调用通知链成员的回调函数，传给回调函数的参数是链表成员本身，`action`和`void *data`，通知链成员回调函数可根据传入的数据判定是否继续执行回调函数。

回调函数返回值：

```cpp
#define NOTIFY_DONE		0x0000		/* Don't care *///此回调不关心此事件，直接返回
#define NOTIFY_OK		0x0001		/* Suits me *///此回调执行事件并返回成功
#define NOTIFY_STOP_MASK	0x8000		/* Don't call further */
#define NOTIFY_BAD		(NOTIFY_STOP_MASK|0x0002)//此回调执行失败返回异常，此时链表结束执行直接返回
						/* Bad/Veto action */
/*
 * Clean way to return from the notifier and stop further calls.
 */
#define NOTIFY_STOP		(NOTIFY_OK|NOTIFY_STOP_MASK)//人为的停止链表的继续执行，此时链表结束执行直接返回
```