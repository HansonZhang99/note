# 设备模型(kobject/kset/ktype)

内核版本：linux-4.9.88

ref:http://www.wowotech.net/device_model/421.html

## kobject/kset/ktype

### kobject

Kobject是基本数据类型，每个Kobject都会在"/sys/“文件系统中以目录的形式出现。

结构体原型：

```cpp
//include/linux/kobject.h
struct kobject {
	const char		*name;//该Kobject的名称，同时也是sysfs中的目录名称。由于Kobject添加到Kernel时，需要根据名字注册到sysfs中，之后就不能再直接修改该字段。如果需要修改Kobject的名字，需要调用kobject_rename接口，该接口会主动处理sysfs的相关事宜。
	struct list_head	entry;//用于将Kobject加入到Kset中的list_head。
	struct kobject		*parent;//指向parent kobject，以此形成层次结构（在sysfs就表现为目录结构）。
	struct kset		*kset;//该kobject属于的Kset。可以为NULL。如果存在，且没有指定parent，则会把Kset作为parent（Kset是一个特殊的Kobject）。
	struct kobj_type	*ktype;//该Kobject属于的kobj_type。每个Kobject必须有一个ktype，或者Kernel会提示错误。
	struct kernfs_node	*sd; /* sysfs directory entry */
	struct kref		kref;//"struct kref”类型（在include/linux/kref.h中定义）的变量，为一个可用于原子操作的引用计数。
#ifdef CONFIG_DEBUG_KOBJECT_RELEASE
	struct delayed_work	release;
#endif
	unsigned int state_initialized:1;//指示该Kobject是否已经初始化，以在Kobject的Init，Put，Add等操作时进行异常校验。
	unsigned int state_in_sysfs:1;//指示该Kobject是否已在sysfs中呈现，以便在自动注销时从sysfs中移除。
    //记录是否已经向用户空间发送ADD uevent，如果有，且没有发送remove uevent，则在自动注销时，补发REMOVE uevent，以便让用户空间正确处理。
	unsigned int state_add_uevent_sent:1;
	unsigned int state_remove_uevent_sent:1;
    //如果该字段为1，则表示忽略所有上报的uevent事件。
	unsigned int uevent_suppress:1;
};
```

### kset

```cpp
//include/linux/kobject.h
struct kset {
	struct list_head list;//用于保存该kset下所有的kobject的链表。
	spinlock_t list_lock;
	struct kobject kobj;//该kset自己的kobject（kset是一个特殊的kobject，也会在sysfs中以目录的形式体现）
	const struct kset_uevent_ops *uevent_ops;//该kset的uevent操作函数集。当任何Kobject需要上报uevent时，都要调用它所从属的kset的uevent_ops，添加环境变量，或者过滤event（kset可以决定哪些event可以上报）。因此，如果一个kobject不属于任何kset时，是不允许发送uevent的。
};
```

### ktype

```cpp
struct kobj_type 
{ 
    void (*release)(struct kobject *kobj);//通过该回调函数，可以将包含该种类型kobject的数据结构的内存空间释放掉。
	const struct sysfs_ops *sysfs_ops;//该种类型的Kobject的sysfs文件系统接口。
	struct attribute **default_attrs;//该种类型的Kobject的atrribute列表（所谓attribute，就是sysfs文件系统中的一个文件）。将会在Kobject添加到内核时，一并注册到sysfs中。
	const struct kobj_ns_type_operations *(*child_ns_type)(struct kobject *kobj); 
    const void *(*namespace)(struct kobject *kobj); 
};
```



### 相关接口

#### kobject_init

```cpp
/*传入已经分配的kobject和ktype,初始化kobject的引用计数为1,置初始化状态*/
void kobject_init(struct kobject *kobj, struct kobj_type *ktype)
{
    char *err_str;


    if (!kobj) {
        err_str = "invalid kobject pointer!";
        goto error;
    }
    if (!ktype) {
        err_str = "must have a ktype to be initialized properly!\n";
        goto error;
    }
    if (kobj->state_initialized) {//一个kobject结构体只能被初始化一次，初始化后state_initialized置位
        /* do not error out as sometimes we can recover */
        printk(KERN_ERR "kobject (%p): tried to init an initialized "
               "object, something is seriously wrong.\n", kobj);
        dump_stack();
    }


    kobject_init_internal(kobj);
    kobj->ktype = ktype;//属性赋值
    return;


error:
    printk(KERN_ERR "kobject (%p): %s\n", kobj, err_str);
    dump_stack();
}
EXPORT_SYMBOL(kobject_init);



static void kobject_init_internal(struct kobject *kobj)
{
    if (!kobj)
        return;
    kref_init(&kobj->kref);//初始化引用计数为1
    INIT_LIST_HEAD(&kobj->entry);//初始化链表头，next和prev都指向自己
    kobj->state_in_sysfs = 0;
    kobj->state_add_uevent_sent = 0;
    kobj->state_remove_uevent_sent = 0;
    kobj->state_initialized = 1;//置1
}
```

#### kobject_add

添加一个kobject到内核中，此操作会在sysfs下根据parent创建一个目录，在目录下创建一系列属性文件，这些属性文件来自ktype的default_attrs。

```cpp
int kobject_add(struct kobject *kobj, struct kobject *parent,
        const char *fmt, ...)
{
    va_list args;
    int retval;


    if (!kobj)
        return -EINVAL;


    if (!kobj->state_initialized) {
        printk(KERN_ERR "kobject '%s' (%p): tried to add an "
               "uninitialized object, something is seriously wrong.\n",
               kobject_name(kobj), kobj);
        dump_stack();
        return -EINVAL;
    }
    va_start(args, fmt);
    retval = kobject_add_varg(kobj, parent, fmt, args);
    va_end(args);


    return retval;
}
EXPORT_SYMBOL(kobject_add);

static __printf(3, 0) int kobject_add_varg(struct kobject *kobj,
                       struct kobject *parent,
                       const char *fmt, va_list vargs)
{
    int retval;


    retval = kobject_set_name_vargs(kobj, fmt, vargs);//通过格式化字符串设置此kobject的名字，此名字将作为sys树下的某个目录名
    if (retval) {
        printk(KERN_ERR "kobject: can not set name properly!\n");
        return retval;
    }
    kobj->parent = parent;//parent赋值
    return kobject_add_internal(kobj);
}

static int kobject_add_internal(struct kobject *kobj)
{
    int error = 0;
    struct kobject *parent;


    if (!kobj)
        return -ENOENT;


    if (!kobj->name || !kobj->name[0]) {//name为空直接返回错误
        WARN(1, "kobject: (%p): attempted to be registered with empty "
             "name!\n", kobj);
        return -EINVAL;
    }


    parent = kobject_get(kobj->parent);//如果kobj->parent非空，增加parent的引用计数，并返回parent


    /* join kset if set, use it as parent if we do not already have one */
    //检查kobj->kset是否被设置,如果被设置，且父节点为空，则使用kobj->kset->kobj作为父节点
    //将kobj加入到kobj->kset链表中
    //kobj父节点设置(优先使用kobj->parent其次kobj->kset->kobj)
    if (kobj->kset) {
        if (!parent)
            parent = kobject_get(&kobj->kset->kobj);
        kobj_kset_join(kobj);//将kobj加入kset的链表
        kobj->parent = parent;
    }


    pr_debug("kobject: '%s' (%p): %s: parent: '%s', set: '%s'\n",
         kobject_name(kobj), kobj, __func__,
         parent ? kobject_name(parent) : "<NULL>",
         kobj->kset ? kobject_name(&kobj->kset->kobj) : "<NULL>");


    error = create_dir(kobj);
    if (error) {
        kobj_kset_leave(kobj);
        kobject_put(parent);
        kobj->parent = NULL;


        /* be noisy on error issues */
        if (error == -EEXIST)
            WARN(1, "%s failed for %s with "
                 "-EEXIST, don't try to register things with "
                 "the same name in the same directory.\n",
                 __func__, kobject_name(kobj));
        else
            WARN(1, "%s failed for %s (error: %d parent: %s)\n",
                 __func__, kobject_name(kobj), error,
                 parent ? kobject_name(parent) : "'none'");
    } else
        kobj->state_in_sysfs = 1;


    return error;
}

static void kobj_kset_join(struct kobject *kobj)
{
	if (!kobj->kset)
		return;

	kset_get(kobj->kset);
	spin_lock(&kobj->kset->list_lock);
	list_add_tail(&kobj->entry, &kobj->kset->list);//将kobj加入其kset链表
	spin_unlock(&kobj->kset->list_lock);
}


static int create_dir(struct kobject *kobj)
{
    const struct kobj_ns_type_operations *ops;
    int error;


    error = sysfs_create_dir_ns(kobj, kobject_namespace(kobj));//创建名为kobj->name的目录，目录会创建在kobj->parent下
    if (error)
        return error;


    error = populate_dir(kobj);
    if (error) {
        sysfs_remove_dir(kobj);
        return error;
    }


    /*
     * @kobj->sd may be deleted by an ancestor going away.  Hold an
     * extra reference so that it stays until @kobj is gone.
     */
    sysfs_get(kobj->sd);


    /*
     * If @kobj has ns_ops, its children need to be filtered based on
     * their namespace tags.  Enable namespace support on @kobj->sd.
     */
    ops = kobj_child_ns_ops(kobj);
    if (ops) {
        BUG_ON(ops->type <= KOBJ_NS_TYPE_NONE);
        BUG_ON(ops->type >= KOBJ_NS_TYPES);
        BUG_ON(!kobj_ns_type_registered(ops->type));


        sysfs_enable_ns(kobj->sd);
    }


    return 0;
}


int sysfs_create_dir_ns(struct kobject *kobj, const void *ns)
{
    //实际用来组织sysfs目录树的成员应该是kernfs_node，每个kobj应该都有自己的node,并能通过node找到kobj
    struct kernfs_node *parent, *kn;


    BUG_ON(!kobj);


    if (kobj->parent)
        parent = kobj->parent->sd;//sd成员保存的是当前kobj对应的kernfs_node，这里获取parent的kernfs_node,目的是在这个node下进行创建
    else
        parent = sysfs_root_kn;//没有parent就是sysfs的根节点创建


    if (!parent)
        return -ENOENT;

	//实际的创建目录接口
    kn = kernfs_create_dir_ns(parent, kobject_name(kobj),
                  S_IRWXU | S_IRUGO | S_IXUGO, kobj, ns);
    if (IS_ERR(kn)) {
        if (PTR_ERR(kn) == -EEXIST)
            sysfs_warn_dup(parent, kobject_name(kobj));
        return PTR_ERR(kn);
    }


    kobj->sd = kn;//保存当前kobj的kernfs_node
    return 0;
}

struct kernfs_node *kernfs_create_dir_ns(struct kernfs_node *parent,
					 const char *name, umode_t mode,
					 void *priv, const void *ns)
{
	struct kernfs_node *kn;
	int rc;

	/* allocate */
	kn = kernfs_new_node(parent, name, mode | S_IFDIR, KERNFS_DIR);//创建一个新node，需要传入parent,name,mode
	if (!kn)
		return ERR_PTR(-ENOMEM);

	kn->dir.root = parent->dir.root;//设置node的dir成员的root节点为父目录的根节点
	kn->ns = ns;//namespace
	kn->priv = priv;//保存自己的kobj

	/* link in */
	rc = kernfs_add_one(kn);
	if (!rc)
		return kn;

	kernfs_put(kn);
	return ERR_PTR(rc);
}

static int populate_dir(struct kobject *kobj)
{
    struct kobj_type *t = get_ktype(kobj);
    struct attribute *attr;
    int error = 0;
    int i;


    if (t && t->default_attrs) {
        for (i = 0; (attr = t->default_attrs[i]) != NULL; i++) {
            error = sysfs_create_file(kobj, attr);//为ktype在kobject目录下创建文件，文件读写操作通过kobject->ktype->sysfs_ops来完成，文件的操作最后都会回溯到ktype->sysfs_ops的show和store这两个函数中。
            if (error)
                break;
        }
    }
    return error;
}
```

#### kobject_init_and_add

等于上面两函数的结合

```cpp
int kobject_init_and_add(struct kobject *kobj, struct kobj_type *ktype,
             struct kobject *parent, const char *fmt, ...)
{
    va_list args;
    int retval;


    kobject_init(kobj, ktype);


    va_start(args, fmt);
    retval = kobject_add_varg(kobj, parent, fmt, args);
    va_end(args);


    return retval;
}
EXPORT_SYMBOL_GPL(kobject_init_and_add);


struct kobject *kobject_create_and_add(const char *name, struct kobject *parent)
{
    struct kobject *kobj;
    int retval;


    kobj = kobject_create();
    if (!kobj)
        return NULL;


    retval = kobject_add(kobj, parent, "%s", name);
    if (retval) {
        printk(KERN_WARNING "%s: kobject_add error: %d\n",
               __func__, retval);
        kobject_put(kobj);
        kobj = NULL;
    }
    return kobj;
}
EXPORT_SYMBOL_GPL(kobject_create_and_add);
```

#### kobject_del

```cpp
void kobject_del(struct kobject *kobj)
{
    struct kernfs_node *sd;


    if (!kobj)
        return;


    sd = kobj->sd;
    sysfs_remove_dir(kobj);
    sysfs_put(sd);


    kobj->state_in_sysfs = 0;
    kobj_kset_leave(kobj);
    kobject_put(kobj->parent);//当引用计数为0时，调用kobject_release函数，此函数会找到ktype并调用release
    kobj->parent = NULL;
}
EXPORT_SYMBOL(kobject_del);
```

### 其它接口

##### kobject_put

减少kobject的引用计数，当引用计数为0时调用kobject_release，可以学习这里的原子操作

```cpp
void kobject_put(struct kobject *kobj)
{
	if (kobj) {
		if (!kobj->state_initialized)
			WARN(1, KERN_WARNING "kobject: '%s' (%p): is not "
			       "initialized, yet kobject_put() is being "
			       "called.\n", kobject_name(kobj), kobj);
		kref_put(&kobj->kref, kobject_release);
	}
}

static inline int kref_put(struct kref *kref, void (*release)(struct kref *kref))
{
	return kref_sub(kref, 1, release);
}

static inline int kref_sub(struct kref *kref, unsigned int count,
	     void (*release)(struct kref *kref))
{
	WARN_ON(release == NULL);

	if (atomic_sub_and_test((int) count, &kref->refcount)) {
		release(kref);//调用release
		return 1;
	}
	return 0;
}
```

##### kobject_release

```cpp
static void kobject_release(struct kref *kref)
{
	struct kobject *kobj = container_of(kref, struct kobject, kref);//通过kref找到kobject
#ifdef CONFIG_DEBUG_KOBJECT_RELEASE
	unsigned long delay = HZ + HZ * (get_random_int() & 0x3);
	pr_info("kobject: '%s' (%p): %s, parent %p (delayed %ld)\n",
		 kobject_name(kobj), kobj, __func__, kobj->parent, delay);
	INIT_DELAYED_WORK(&kobj->release, kobject_delayed_cleanup);

	schedule_delayed_work(&kobj->release, delay);
#else
	kobject_cleanup(kobj);//调用ktype->release
#endif
}

static void kobject_cleanup(struct kobject *kobj)
{
	struct kobj_type *t = get_ktype(kobj);//找到kobject的ktype
	const char *name = kobj->name;

	pr_debug("kobject: '%s' (%p): %s, parent %p\n",
		 kobject_name(kobj), kobj, __func__, kobj->parent);

	//...

	if (t && t->release) {
		pr_debug("kobject: '%s' (%p): calling ktype release\n",
			 kobject_name(kobj), kobj);
		t->release(kobj);//调用ktype->release
	}

	//...
}
```

##### kobject_get

增加kobject的引用计数，在上面的kobject_init中通过kref_init增加引用计数，效果一样

```cpp
struct kobject *kobject_get(struct kobject *kobj)
{
	if (kobj) {
		if (!kobj->state_initialized)
			WARN(1, KERN_WARNING "kobject: '%s' (%p): is not "
			       "initialized, yet kobject_get() is being "
			       "called.\n", kobject_name(kobj), kobj);
		kref_get(&kobj->kref);
	}
	return kobj;
}
```

##### kobject_rename

重命名一个kobject

```cpp
int kobject_rename(struct kobject *kobj, const char *new_name)
{
	int error = 0;
	const char *devpath = NULL;
	const char *dup_name = NULL, *name;
	char *devpath_string = NULL;
	char *envp[2];

	kobj = kobject_get(kobj);
	if (!kobj)
		return -EINVAL;
	if (!kobj->parent)
		return -EINVAL;

	devpath = kobject_get_path(kobj, GFP_KERNEL);
	if (!devpath) {
		error = -ENOMEM;
		goto out;
	}
	devpath_string = kmalloc(strlen(devpath) + 15, GFP_KERNEL);
	if (!devpath_string) {
		error = -ENOMEM;
		goto out;
	}
	sprintf(devpath_string, "DEVPATH_OLD=%s", devpath);
	envp[0] = devpath_string;
	envp[1] = NULL;

	name = dup_name = kstrdup_const(new_name, GFP_KERNEL);
	if (!name) {
		error = -ENOMEM;
		goto out;
	}

	error = sysfs_rename_dir_ns(kobj, new_name, kobject_namespace(kobj));
	if (error)
		goto out;

	/* Install the new kobject name */
	dup_name = kobj->name;
	kobj->name = name;

	/* This function is mostly/only used for network interface.
	 * Some hotplug package track interfaces by their name and
	 * therefore want to know when the name is changed by the user. */
	kobject_uevent_env(kobj, KOBJ_MOVE, envp);

out:
	kfree_const(dup_name);
	kfree(devpath_string);
	kfree(devpath);
	kobject_put(kobj);

	return error;
}
```



### 三者关系

sysfs文件系统中的所有目录对应就是内核中kset/kobject的层次关系。

kset是一个特殊的kobject,不同的是kset提供了一个kset_uevent_ops类型的操作集，此操作集用于kset/kobject上报uevent事件，当一个kobject需要上报uevent时，必须向上获取到所属的kset,并调用kset的uevent操作集函数进行操作。

kobject也是底层数据结构，c语言没有面向对象的语法，但是可以通过结构体包含的方式，让大型数据结构包含kobject来实现面向对象。kobject通过name指定此对象在sysfs中的目录名，通过parent形成sysfs的层级目录结构，通过kset指定kobject的事件上报操作集，通过ktype指定release操作函数，默认属性文件，sysfs文件系统中该目录（kobject）的操作接口，使用kref表示此kobject的引用计数。

ktype提供release函数来释放kobject(甚至也可以释放包含kobject的数据结构)，当kobject的引用计数为0时，将自动调用ktype的release函数释放kobject，因此kobject必须是动态分配。sysfs_ops指定了当前sysfs目录也就是kobject的文件系统操作接口，原型如下：

```cpp
struct sysfs_ops {
	ssize_t	(*show)(struct kobject *, struct attribute *, char *);
	ssize_t	(*store)(struct kobject *, struct attribute *, const char *, size_t);
};
```

实际需要提供的就是show和store函数，对应read/write。

kobj实现对象的生命周期管理（计数为0即清除）；

kset包含一个kobj，相当于也是一个目录；

kset是一个容器，可以包容不同类型的kobj（甚至父子关系的两个kobj都可以属于一个kset）；

注册kobj（如kobj_add()函数）时，优先以kobj->parent作为自身的parent目录；

其次，以kset作为parent目录；若都没有，则是sysfs的顶层目录；

同时，若设置了kset，还会将kobj加入kset.list。举例：

1、无parent、无kset，则将在sysfs的根目录（即/sys/）下创建目录；

2、无parent、有kset，则将在kset下创建目录；并将kobj加入kset.list；

3、有parent、无kset，则将在parent下创建目录；

4、有parent、有kset，则将在parent下创建目录，并将kobj加入kset.list；

注册kset时，如果它的kobj->parent为空，则设置它所属的kset（kset.kobj->kset.kobj）为parent，即优先使用kobj所属的parent；然后再使用kset作为parent。（如platform_bus_type的注册，如某些input设备的注册）

## 内核示例

drives/base/core.c

```cpp
int __init devices_init(void)
{
    devices_kset = kset_create_and_add("devices", &device_uevent_ops, NULL);//在sys顶层目录创建devices目录,绑定device_uevent_ops
    if (!devices_kset)
        return -ENOMEM;
    dev_kobj = kobject_create_and_add("dev", NULL);//sys顶层创建dev目录,这个目录没有uevent_ops所以不允许发送event
    if (!dev_kobj)//同上
        goto dev_kobj_err;
    sysfs_dev_block_kobj = kobject_create_and_add("block", dev_kobj);//在dev目录下创建block目录
    if (!sysfs_dev_block_kobj)
        goto block_kobj_err;
    sysfs_dev_char_kobj = kobject_create_and_add("char", dev_kobj);//在dev目录下创建char目录
    if (!sysfs_dev_char_kobj)
        goto char_kobj_err;

    return 0;


char_kobj_err:
    kobject_put(sysfs_dev_block_kobj);//kobject_get增加kobj引用计数，kobject_put相反减少kobj引用计数,当引用计数为0时，执行kobj的清除操作
block_kobj_err:
    kobject_put(dev_kobj);
dev_kobj_err:
    kset_unregister(devices_kset);
    return -ENOMEM;
}
```

### kset_create_and_add

向内核注册一个kset，最后会调用kobject_uevent

```cpp
struct kset *kset_create_and_add(const char *name,
				 const struct kset_uevent_ops *uevent_ops,
				 struct kobject *parent_kobj)
{
	struct kset *kset;
	int error;

	kset = kset_create(name, uevent_ops, parent_kobj);
	if (!kset)
		return NULL;
	error = kset_register(kset);
	if (error) {
		kfree(kset);
		return NULL;
	}
	return kset;
}

static struct kset *kset_create(const char *name,
				const struct kset_uevent_ops *uevent_ops,
				struct kobject *parent_kobj)
{
	struct kset *kset;
	int retval;

	kset = kzalloc(sizeof(*kset), GFP_KERNEL);
	if (!kset)
		return NULL;
	retval = kobject_set_name(&kset->kobj, "%s", name);
	if (retval) {
		kfree(kset);
		return NULL;
	}
	kset->uevent_ops = uevent_ops;
	kset->kobj.parent = parent_kobj;

	/*
	 * The kobject of this kset will have a type of kset_ktype and belong to
	 * no kset itself.  That way we can properly free it when it is
	 * finished being used.
	 */
	kset->kobj.ktype = &kset_ktype;
	kset->kobj.kset = NULL;

	return kset;
}

int kset_register(struct kset *k)
{
	int err;

	if (!k)
		return -EINVAL;

	kset_init(k);
	err = kobject_add_internal(&k->kobj);
	if (err)
		return err;
	kobject_uevent(&k->kobj, KOBJ_ADD);
	return 0;
}

```

这个kset注册时会绑定一个ktype--> kset_ktype

```cpp
static struct kobj_type kset_ktype = {
	.sysfs_ops	= &kobj_sysfs_ops,
	.release = kset_release,
};

/*通过包含的kobj对象找到kset对象，并释放kset*/
static void kset_release(struct kobject *kobj)
{
	struct kset *kset = container_of(kobj, struct kset, kobj);
	pr_debug("kobject: '%s' (%p): %s\n",
		 kobject_name(kobj), kobj, __func__);
	kfree(kset);
}


const struct sysfs_ops kobj_sysfs_ops = {
	.show	= kobj_attr_show,
	.store	= kobj_attr_store,
};

/*通过attribute找到对应的kobj_attribute,并调用其show函数*/
static ssize_t kobj_attr_show(struct kobject *kobj, struct attribute *attr,
			      char *buf)
{
	struct kobj_attribute *kattr;
	ssize_t ret = -EIO;

	kattr = container_of(attr, struct kobj_attribute, attr);
	if (kattr->show)
		ret = kattr->show(kobj, kattr, buf);
	return ret;
}
/*同上*/
static ssize_t kobj_attr_store(struct kobject *kobj, struct attribute *attr,
			       const char *buf, size_t count)
{
	struct kobj_attribute *kattr;
	ssize_t ret = -EIO;

	kattr = container_of(attr, struct kobj_attribute, attr);
	if (kattr->store)
		ret = kattr->store(kobj, kattr, buf, count);
	return ret;
}
```

### kobject_create_and_add

```cpp
struct kobject *kobject_create_and_add(const char *name, struct kobject *parent)
{
	struct kobject *kobj;
	int retval;

	kobj = kobject_create();
	if (!kobj)
		return NULL;

	retval = kobject_add(kobj, parent, "%s", name);
	if (retval) {
		printk(KERN_WARNING "%s: kobject_add error: %d\n",
		       __func__, retval);
		kobject_put(kobj);
		kobj = NULL;
	}
	return kobj;
}

struct kobject *kobject_create(void)
{
	struct kobject *kobj;

	kobj = kzalloc(sizeof(*kobj), GFP_KERNEL);
	if (!kobj)
		return NULL;

	kobject_init(kobj, &dynamic_kobj_ktype);
	return kobj;
}


static struct kobj_type dynamic_kobj_ktype = {
	.release	= dynamic_kobj_release,
	.sysfs_ops	= &kobj_sysfs_ops,
};

static void dynamic_kobj_release(struct kobject *kobj)
{
	pr_debug("kobject: (%p): %s\n", kobj, __func__);
	kfree(kobj);
}

const struct sysfs_ops kobj_sysfs_ops = {
	.show	= kobj_attr_show,
	.store	= kobj_attr_store,
};
```

上面的`kset_create_and_add`和`kobject_create_and_add`函数从名字可以看出，这是两个通用函数，可以直接调用创建一个kset或者kobject，这两个接口不支持外部传入ktype，而是内部使用了默认的已经存在的ktype：kset_ktype和dynamic_kobj_ktype(这里dynamic的意思应该是此kobject是动态创建的，内核可以静态创建kobject但是不推荐)

#### release

这两个ktype的release函数都是默认释放自己的内存kset和kobject，这里很典型的kset中内嵌了kobject，在释放kset时，release函数通过内嵌的kobject找到了kset，然后释放kset，当kset释放时又会调用kobject的release函数释放掉内部的kobject。当自己去实现一个kobject时，也可以通过container_of的方式释放自己自定义的数据结构

```cpp
/*通过包含的kobj对象找到kset对象，并释放kset*/
static void kset_release(struct kobject *kobj)
{
	struct kset *kset = container_of(kobj, struct kset, kobj);
	pr_debug("kobject: '%s' (%p): %s\n",
		 kobject_name(kobj), kobj, __func__);
	kfree(kset);
}

static void dynamic_kobj_release(struct kobject *kobj)
{
	pr_debug("kobject: (%p): %s\n", kobj, __func__);
	kfree(kobj);
}
```

#### sysfs_ops

这里kset和kobject使用的是相同的sysfs_ops即kobj_sysfs_ops

里面的show和store函数并没有实际的数据操作，而是调用了attribute中kobj_attribute的show。

```cpp
/*通过attribute找到对应的kobj_attribute,并调用其show函数*/
static ssize_t kobj_attr_show(struct kobject *kobj, struct attribute *attr,
			      char *buf)
{
	struct kobj_attribute *kattr;
	ssize_t ret = -EIO;

	kattr = container_of(attr, struct kobj_attribute, attr);
	if (kattr->show)
		ret = kattr->show(kobj, kattr, buf);
	return ret;
}
/*同上*/
static ssize_t kobj_attr_store(struct kobject *kobj, struct attribute *attr,
			       const char *buf, size_t count)
{
	struct kobj_attribute *kattr;
	ssize_t ret = -EIO;

	kattr = container_of(attr, struct kobj_attribute, attr);
	if (kattr->store)
		ret = kattr->store(kobj, kattr, buf, count);
	return ret;
}
```

##### sysfs_create_file

以drivers/base/core.c中的device_add为例，该函数直接调用device_create_file->sysfs_create_file

```cpp
static DEVICE_ATTR_RW(uevent);
int device_add(struct device *dev)
{
    //...
    error = device_create_file(dev, &dev_attr_uevent);
    //...
}

int device_create_file(struct device *dev,
		       const struct device_attribute *attr)
{
	int error = 0;

	if (dev) {
		WARN(((attr->attr.mode & S_IWUGO) && !attr->store),
			"Attribute %s: write permission without 'store'\n",
			attr->attr.name);
		WARN(((attr->attr.mode & S_IRUGO) && !attr->show),
			"Attribute %s: read permission without 'show'\n",
			attr->attr.name);
		error = sysfs_create_file(&dev->kobj, &attr->attr);
	}

	return error;
}
```

内核中像dev_attr_uevent的变量并不会直接定义，一般是通过宏将两个或多个字符串拼接进行定义:

```cpp
//drivers/base/core.c
static DEVICE_ATTR_RW(uevent);

//include/linux/device.h
#define DEVICE_ATTR_RW(_name) \
	struct device_attribute dev_attr_##_name = __ATTR_RW(_name)
//这里宏展开后即struct device_attribute dev_attr_uevent = __ATTR_RW(uevent)

#define __ATTR_RW(_name) __ATTR(_name, (S_IWUSR | S_IRUGO),		\
			 _name##_show, _name##_store)
//即__ATTR(uevent, (S_IWUSR | S_IRUGO), uevent_show, uevent_store)

#define __ATTR(_name, _mode, _show, _store) {				\
	.attr = {.name = __stringify(_name),				\
		 .mode = VERIFY_OCTAL_PERMISSIONS(_mode) },		\
	.show	= _show,						\
	.store	= _store,						\
}
#define __stringify_1(x...)	#x
#define __stringify(x...)	__stringify_1(x)

//最后展开如下
struct device_attribute dev_attr_uevent =
{
    .attr = {.name = "uevent",				\
		.mode = VERIFY_OCTAL_PERMISSIONS(_mode) },		\
    .show = uevent_show,
    .store = uevent_store,
};
```

device_create_file会调用sysfs_create_file，第二个参数为&dev_attr_uevent.attr

```cpp
//include/linux/sysfs.h
static inline int __must_check sysfs_create_file(struct kobject *kobj,
						 const struct attribute *attr)
{
	return sysfs_create_file_ns(kobj, attr, NULL);
}
```

sysfs_create_file会调用sysfs_create_file_ns

```cpp
//fs/sysfs/file.c
int sysfs_create_file_ns(struct kobject *kobj, const struct attribute *attr,
			 const void *ns)
{
	BUG_ON(!kobj || !kobj->sd || !attr);

	return sysfs_add_file_mode_ns(kobj->sd, attr, false, attr->mode, ns);

}
```

sysfs_create_file_ns再调用sysfs_add_file_mode_ns

```cpp
int sysfs_add_file_mode_ns(struct kernfs_node *parent,
			   const struct attribute *attr, bool is_bin,
			   umode_t mode, const void *ns)
{
	struct lock_class_key *key = NULL;
	const struct kernfs_ops *ops;
	struct kernfs_node *kn;
	loff_t size;

	if (!is_bin) {
		struct kobject *kobj = parent->priv;//parent=kobj->sd， 从kobject_add可知sd即kobject对应的kernfs_node,而node中的priv保存的就是kobj本身
		const struct sysfs_ops *sysfs_ops = kobj->ktype->sysfs_ops;//获取kobj的ktype->sysfs_ops

		/* every kobject with an attribute needs a ktype assigned */
		if (WARN(!sysfs_ops, KERN_ERR
			 "missing sysfs attribute operations for kobject: %s\n",
			 kobject_name(kobj)))
			return -EINVAL;
		//通过sysfs_ope的show和store以及attr->mode选择一个ops
		if (sysfs_ops->show && sysfs_ops->store) {
			if (mode & SYSFS_PREALLOC)
				ops = &sysfs_prealloc_kfops_rw;
			else
				ops = &sysfs_file_kfops_rw;
		} else if (sysfs_ops->show) {
			if (mode & SYSFS_PREALLOC)
				ops = &sysfs_prealloc_kfops_ro;
			else
				ops = &sysfs_file_kfops_ro;
		} else if (sysfs_ops->store) {
			if (mode & SYSFS_PREALLOC)
				ops = &sysfs_prealloc_kfops_wo;
			else
				ops = &sysfs_file_kfops_wo;
		} else
			ops = &sysfs_file_kfops_empty;

		size = PAGE_SIZE;
	} else {
		struct bin_attribute *battr = (void *)attr;

		if (battr->mmap)
			ops = &sysfs_bin_kfops_mmap;
		else if (battr->read && battr->write)
			ops = &sysfs_bin_kfops_rw;
		else if (battr->read)
			ops = &sysfs_bin_kfops_ro;
		else if (battr->write)
			ops = &sysfs_bin_kfops_wo;
		else
			ops = &sysfs_file_kfops_empty;

		size = battr->size;
	}

	//...
    //创建文件，参数有文件名，权限，大小，操作集，属性...，这里的kn节点代表的就是文件自己，而kn->parent就是文件所在目录对应的kobject->sd
	kn = __kernfs_create_file(parent, attr->name, mode & 0777, size, ops,
				  (void *)attr, ns, key);
	if (IS_ERR(kn)) {
		if (PTR_ERR(kn) == -EEXIST)
			sysfs_warn_dup(parent, attr->name);
		return PTR_ERR(kn);
	}
	return 0;
}

struct kernfs_node *__kernfs_create_file(struct kernfs_node *parent,
					 const char *name,
					 umode_t mode, loff_t size,
					 const struct kernfs_ops *ops,
					 void *priv, const void *ns,
					 struct lock_class_key *key)
{
	struct kernfs_node *kn;
	unsigned flags;
	int rc;

	flags = KERNFS_FILE;

	kn = kernfs_new_node(parent, name, (mode & S_IALLUGO) | S_IFREG, flags);
	if (!kn)
		return ERR_PTR(-ENOMEM);

	kn->attr.ops = ops;//设置操作集
	kn->attr.size = size;
	kn->ns = ns;
	kn->priv = priv;//私有数据，这里私有数据是&dev_attr_uevent.attr

#ifdef CONFIG_DEBUG_LOCK_ALLOC
	if (key) {
		lockdep_init_map(&kn->dep_map, "s_active", key, 0);
		kn->flags |= KERNFS_LOCKDEP;
	}
#endif

	/*
	 * kn->attr.ops is accesible only while holding active ref.  We
	 * need to know whether some ops are implemented outside active
	 * ref.  Cache their existence in flags.
	 */
	if (ops->seq_show)
		kn->flags |= KERNFS_HAS_SEQ_SHOW;
	if (ops->mmap)
		kn->flags |= KERNFS_HAS_MMAP;

	rc = kernfs_add_one(kn);
	if (rc) {
		kernfs_put(kn);
		return ERR_PTR(rc);
	}
	return kn;
}

```

sysfs_add_file_mode_ns函数会根据属性结构`dev_attr_uevent.attr`中的name和mode选择一个ops，并创建文件，此时sysfs中kobject对应目录中就会多出一个名为uevent的文件。

从`dev_attr_uevent.attr`的mode定义可知，ops = &sysfs_file_kfops_rw：

```cpp
static const struct kernfs_ops sysfs_file_kfops_rw = {
	.seq_show	= sysfs_kf_seq_show,
	.write		= sysfs_kf_write,
};

static int sysfs_kf_seq_show(struct seq_file *sf, void *v)
{
	struct kernfs_open_file *of = sf->private;
	struct kobject *kobj = of->kn->parent->priv;//of->kn就是文件本身,其parent就是文件所在目录对应的kobject->sd,其priv为目录的kobj
	const struct sysfs_ops *ops = sysfs_file_ops(of->kn);//这里直接返回的是kobject->ktype->sysfs_ops
	ssize_t count;
	char *buf;

	/* acquire buffer and ensure that it's >= PAGE_SIZE and clear */
	count = seq_get_buf(sf, &buf);
	if (count < PAGE_SIZE) {
		seq_commit(sf, -1);
		return 0;
	}
	memset(buf, 0, PAGE_SIZE);

	/*
	 * Invoke show().  Control may reach here via seq file lseek even
	 * if @ops->show() isn't implemented.
	 */
	if (ops->show) {
		count = ops->show(kobj, of->kn->priv, buf);//调用sysfs_ops->show
		if (count < 0)
			return count;
	}

	/*
	 * The code works fine with PAGE_SIZE return but it's likely to
	 * indicate truncated result or overflow in normal use cases.
	 */
	if (count >= (ssize_t)PAGE_SIZE) {
		print_symbol("fill_read_buffer: %s returned bad count\n",
			(unsigned long)ops->show);
		/* Try to struggle along */
		count = PAGE_SIZE - 1;
	}
	seq_commit(sf, count);
	return 0;
}

static ssize_t sysfs_kf_write(struct kernfs_open_file *of, char *buf,
			      size_t count, loff_t pos)
{
	const struct sysfs_ops *ops = sysfs_file_ops(of->kn);
	struct kobject *kobj = of->kn->parent->priv;

	if (!count)
		return 0;

	return ops->store(kobj, of->kn->priv, buf, count);
}
```

`count = ops->show(kobj, of->kn->priv, buf);`转到调用`sysfs_ops->show`

虽然device_add中的kobject使用的ktype和这里的kobj_sysfs_ops、dynamic_kobj_ktype不一样，但是套路是一样的：

```cpp

static ssize_t kobj_attr_show(struct kobject *kobj, struct attribute *attr,
			      char *buf)
{
	struct kobj_attribute *kattr;
	ssize_t ret = -EIO;

	kattr = container_of(attr, struct kobj_attribute, attr);
	if (kattr->show)
		ret = kattr->show(kobj, kattr, buf);
	return ret;
}
/*同上*/
static ssize_t kobj_attr_store(struct kobject *kobj, struct attribute *attr,
			       const char *buf, size_t count)
{
	struct kobj_attribute *kattr;
	ssize_t ret = -EIO;

	kattr = container_of(attr, struct kobj_attribute, attr);
	if (kattr->store)
		ret = kattr->store(kobj, kattr, buf, count);
	return ret;
}
```

当sysfs_kf_seq_show被调用时，该文件所在目录的kobject的ktype的sysfs_ops的show被调用，其中第一个参数为该目录对应的kobject,第二个参数在上面传入的是一个私有数据指针，是一个struct attribute类型指针，这里调用了container_of通过attribute指针找到包含其的大数据结构指针kobj_attribute，因为上面分析的属性文件是device的，对应attribute是device_attribute即struct device_attribute dev_attr_uevent。这里是对kobj_attribute的处理，两者并不冲突，最后都会通过attribute转换为对应的大数据结构指针，再调用kobj_attribute/device_attribute中的show/store函数，对这个例子来说，这个函数是uevent_show/uevent_store再进行相应处理。

从这里可以看出来，内核只提供接口，实际的数据结构，操作集sysfs_ops都是可以自定义的。

## 用户访问sysfs文件时内核的执行路径

### glibc->kernel

用户层调用open/read/write系统调用，或者shell下执行cat/echo操作，都会触发glibc的xxx_read/xxx_write调用，最后会通过SWI软件断（对于arm32来说是swi指令触发的软中断）进入内核，这个过程的处理对任何文件系统都是一样的（ext4,sysfs...)

### kernel->open

内核首先会调用open函数打开对应的文件，具体的函数如下，其中根据软中断向量表找到此函数的过程暂时不做说明

```cpp
SYSCALL_DEFINE3(open, const char __user *, filename, int, flags, umode_t, mode)
{
	if (force_o_largefile())
		flags |= O_LARGEFILE;

	return do_sys_open(AT_FDCWD, filename, flags, mode);
}

SYSCALL_DEFINE4(openat, int, dfd, const char __user *, filename, int, flags,
		umode_t, mode) //实际测试走的是这个
{
	if (force_o_largefile())
		flags |= O_LARGEFILE;

	return do_sys_open(dfd, filename, flags, mode);
}
```

随后会经历如下调用：

```shell
-> do_sys_open
	-> vfs_open
		-> do_dentry_open
```

如下是do_dentry_open简化代码：

```cpp
static int do_dentry_open(struct file *f,
			  struct inode *inode,
			  int (*open)(struct inode *, struct file *),
			  const struct cred *cred)
{
	static const struct file_operations empty_fops = {};
	int error;

	f->f_mode = OPEN_FMODE(f->f_flags) | FMODE_LSEEK |
				FMODE_PREAD | FMODE_PWRITE;

	path_get(&f->f_path);
	f->f_inode = inode;
	f->f_mapping = inode->i_mapping;

	//...

	f->f_op = fops_get(inode->i_fop);
	if (unlikely(WARN_ON(!f->f_op))) {
		error = -ENODEV;
		goto cleanup_all;
	}
	//...

	
	if (!open)
		open = f->f_op->open;
	if (open) {
		error = open(inode, f);
		if (error)
			goto cleanup_all;
	}
	//...
	return 0;
	//...
}

```

`f->f_op = fops_get(inode->i_fop);`通过inode获取文件系统的file_operation结构，并给到struct file的f_op。随后执行fop->open函数。

对于每个文件系统如ext3，ext4，ubifs，sysfs等，都有自己文件系统的一个file_opeartion结构体，这个结构体指明了用户在某个文件系统调用了相关接口时，内核相应的执行对应文件系统的函数，如下是ext4，sysfs的file_opeartion结构定义：

```cpp
//fs/ext4/file.c
const struct file_operations ext4_file_operations = {
	.llseek		= ext4_llseek,
	.read_iter	= generic_file_read_iter,
	.write_iter	= ext4_file_write_iter,
	.unlocked_ioctl = ext4_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= ext4_compat_ioctl,
#endif
	.mmap		= ext4_file_mmap,
	.open		= ext4_file_open,
	.release	= ext4_release_file,
	.fsync		= ext4_sync_file,
	.get_unmapped_area = thp_get_unmapped_area,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.fallocate	= ext4_fallocate,
};



//fs/kernfs/file.c
const struct file_operations kernfs_file_fops = {
	.read		= kernfs_fop_read,
	.write		= kernfs_fop_write,
	.llseek		= generic_file_llseek,
	.mmap		= kernfs_fop_mmap,
	.open		= kernfs_fop_open,
	.release	= kernfs_fop_release,
	.poll		= kernfs_fop_poll,
	.fsync		= noop_fsync,
};
```

当文件打开时，文件指针srtuct file的f_ops会指向对应文件系统的file_operations，对于sysfs来说是kernfs_file_fops。在内核中每个文件对应一个inode节点，当文件被打开时，会分配一个struct file类型的结构（被打开多次会分配多个），在用户层这个结构表现为文件描述符fd或者文件指针FILE *。文件打开时，会将文件系统的file_operations赋值给file->f_ops域，并执行其open函数，随后对文件的read/write/close等操作，都会调用f_ops的对应函数。







### kernel->read

同上，内核入口函数为：

```cpp
SYSCALL_DEFINE3(read, unsigned int, fd, char __user *, buf, size_t, count)
{
	struct fd f = fdget_pos(fd);
	ssize_t ret = -EBADF;

	if (f.file) {
		loff_t pos = file_pos_read(f.file);
		ret = vfs_read(f.file, buf, count, &pos);
		if (ret >= 0)
			file_pos_write(f.file, pos);
		fdput_pos(f);
	}
	return ret;
}
```

随后的执行路径：

```shell
-> vfs_read
	__vfs_read
```

如下是`__vfs_read`的代码：

```cpp
ssize_t __vfs_read(struct file *file, char __user *buf, size_t count,
		   loff_t *pos)
{
	if (file->f_op->read)
		return file->f_op->read(file, buf, count, pos);
	else if (file->f_op->read_iter)
		return new_sync_read(file, buf, count, pos);
	else
		return -EINVAL;
}
```

当fop定义了read时，会直接调用`file->f_op->read`，对于sysfs来说，会调用kernfs_fop_read，具体执行路径为：

```shell
-> kernfs_fop_read
    -> kernfs_seq_start
    -> kernfs_seq_show
    	-> sysfs_kf_seq_show
```

即会调用到sysfs_kf_seq_show函数，这个函数随后又会调用`sysfs_ops->show`即kobject->ktype->sysfs_ops->show函数，最后会调用到属性文件自己的show函数上，至此，实现对一个sysfs系统下文件的访问。

以下是使用stap追踪arm32内核版本为linux-4.9.88的open,read,close调用的过程，只关注fs/open.c，fs/kernfs/file.c，sysfs/file.c， fs/read_write.c的函数

```shell
	 0 test_sysfs(16381):    -> SyS_openat
     0 test_sysfs(16381):        -> do_sys_open
     0 test_sysfs(16381):            -> vfs_open
     0 test_sysfs(16381):                -> do_dentry_open
     0 test_sysfs(16381):                    -> kernfs_fop_open
     0 test_sysfs(16381):                    -> kernfs_fop_open
     0 test_sysfs(16381):                -> do_dentry_open
     0 test_sysfs(16381):            -> vfs_open
     0 test_sysfs(16381):            -> open_check_o_direct
     0 test_sysfs(16381):            -> open_check_o_direct
     0 test_sysfs(16381):        -> do_sys_open
     0 test_sysfs(16381):    -> SyS_openat
     0 test_sysfs(16381):    -> SyS_read
     0 test_sysfs(16381):        -> vfs_read
     0 test_sysfs(16381):            -> rw_verify_area
     0 test_sysfs(16381):            <- rw_verify_area
     0 test_sysfs(16381):            -> __vfs_read
     0 test_sysfs(16381):                -> kernfs_fop_read
     0 test_sysfs(16381):                    -> kernfs_seq_start
     0 test_sysfs(16381):                    -> kernfs_seq_start
     0 test_sysfs(16381):                    -> kernfs_seq_show
     0 test_sysfs(16381):                        -> sysfs_kf_seq_show
     0 test_sysfs(16381):                        -> sysfs_kf_seq_show
     0 test_sysfs(16381):                    -> kernfs_seq_show
     0 test_sysfs(16381):                    -> kernfs_seq_next
     0 test_sysfs(16381):                    -> kernfs_seq_next
     0 test_sysfs(16381):                    -> kernfs_seq_stop
     0 test_sysfs(16381):                        -> kernfs_seq_stop_active
     0 test_sysfs(16381):                        -> kernfs_seq_stop_active
     0 test_sysfs(16381):                    -> kernfs_seq_stop
     0 test_sysfs(16381):                -> kernfs_fop_read
     0 test_sysfs(16381):            <- __vfs_read
     0 test_sysfs(16381):        <- vfs_read
     0 test_sysfs(16381):    <- SyS_read
     0 test_sysfs(16381):    -> SyS_close
     0 test_sysfs(16381):        -> filp_close
     0 test_sysfs(16381):        -> filp_close
     0 test_sysfs(16381):    -> SyS_close
     0 test_sysfs(16381):    -> kernfs_fop_release
     0 test_sysfs(16381):        -> kernfs_put_open_node
     0 test_sysfs(16381):        -> kernfs_put_open_node
     0 test_sysfs(16381):    -> kernfs_fop_release
     0 test_sysfs(16381):    -> SyS_write
     0 test_sysfs(16381):        -> vfs_write
     0 test_sysfs(16381):            -> rw_verify_area
     0 test_sysfs(16381):            <- rw_verify_area
     0 test_sysfs(16381):            -> __vfs_write
     0 test_sysfs(16381):            <- __vfs_write
     0 test_sysfs(16381):        <- vfs_write
     0 test_sysfs(16381):    <- SyS_write
     0 test_sysfs(16381):    -> filp_close
     0 test_sysfs(16381):    -> filp_close
     0 test_sysfs(16381):    -> filp_close
     0 test_sysfs(16381):    -> filp_close
     0 test_sysfs(16381):    -> filp_close
     0 test_sysfs(16381):    -> filp_close

```



## test

以下是一个测试kobject的例子，该例子在/sys/下创建example目录，并在目录中创建一个名为exaple_value的属性文件，用户可以向属性文件中写入字符流并查看：

```cpp
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#define min_(a,b) ((a) > (b)) ? (b) : (a)
static struct kobject *example_kobj;
static char example_value[128];

static ssize_t example_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    memcpy(buf, example_value, strlen(example_value));
    return strlen(example_value);
}

static ssize_t example_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    memcpy(example_value, buf, min_(count, sizeof(example_value)));
    return min_(count, sizeof(example_value));
}

static struct kobj_attribute example_attribute = __ATTR(example_value, 0664, example_show, example_store);

static int __init example_init(void)
{
    int ret;

    example_kobj = kobject_create_and_add("example", NULL);
    if (!example_kobj)
        return -ENOMEM;

    ret = sysfs_create_file(example_kobj, &example_attribute.attr);
    if (ret)
        kobject_put(example_kobj);
    printk("create example dir, and create a attribute file in example named example_value\n");
    return ret;
}

static void __exit example_exit(void)
{
    kobject_put(example_kobj);
    printk("remove example dir\n");
}

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("hansonZhang");

```



Makefile:

```Makefile
PWD := $(shell pwd)
CROSS_COMPILE=arm-linux-
KERNELDIR ?= ${PWD}/../../100ask_imx6ull-sdk/Linux-4.9.88/

obj-m := sysfsTestFile.o

default:
        $(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
        $(MAKE) -C $(KERNELDIR) M=$(PWD) clean

```



test:



```shell
[root@imx6ull:~/test]# insmod sysfsTestFile.ko
[root@imx6ull:~/test]# cd /sys/example/
[root@imx6ull:/sys/example]# ls
example_value
[root@imx6ull:/sys/example]# echo "hansonZhang" > example_value
[root@imx6ull:/sys/example]# cat example_value
hansonZhang
[root@imx6ull:/sys/example]# echo "1234567890" > example_value
[root@imx6ull:/sys/example]# cat example_value
1234567890 #代码中使用的是memcpy，没有处理空字符，所以不会覆盖原来的所有数据

[root@imx6ull:/sys/example]# echo "mmm" > example_value
[root@imx6ull:/sys/example]# cat example_value
mmm #代码中使用的是memcpy，没有处理空字符，所以不会覆盖原来的所有数据
567890

[root@imx6ull:/sys/example]# dmesg | tail -2
[11419.574124] remove example dir
[11559.119708] create example dir, and create a attribute file in example named example_value
[root@imx6ull:/sys/example]# cd -
/root/test
[root@imx6ull:~/test]# rmmod sysfsTestFile.ko
[root@imx6ull:~/test]# dmesg | tail -3
[11419.574124] remove example dir
[11559.119708] create example dir, and create a attribute file in example named example_value
[11670.156924] remove example dir
```



