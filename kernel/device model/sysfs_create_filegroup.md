# sysfs_create_file/group

`sysfs_create_file`和`sysfs_create_group`是统一设备模型的东西,它们主要作用是为设备添加一个或一组属性。

原型：

```cpp
int sysfs_create_group(struct kobject *kobj,
		       const struct attribute_group *grp)
{
	return internal_create_group(kobj, 0, grp);
}

static inline int __must_check sysfs_create_file(struct kobject *kobj,
						 const struct attribute *attr)
{
	return sysfs_create_file_ns(kobj, attr, NULL);
}
```



demo:

```cpp
#include <linux/init.h>

#include <linux/module.h>

#include <linux/kobject.h>

#include <linux/sysfs.h>

#include <linux/string.h>

static int hello_value;

static ssize_t hello_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)

{

   return sprintf(buf, "%d\n", hello_value);

}

static ssize_t hello_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)

{

   sscanf(buf, "%du", &hello_value);

   return count;

}

static struct kobj_attribute hello_value_attribute = __ATTR(hello_value, 0666, hello_show, hello_store);//创建一个属性

static struct kobject *hellowold_kobj;

static int __init helloworld_init(void)

{

   int retval;

   helloworld_kobj = kobject_create_and_add("helloworld", kernel_kobj);

   if (!helloworld_kobj)

             return -ENOMEM;

   retval = sysfs_create_file(helloworld_kobj, &hello_value_attribute);

   if (retval)

      kobject_put(helloworld_kobj);

   return retval;

}

static void __exit helloworld_exit(void)

{

   kobject_put(helloworld_kobj);

}

module_init(helloworld_init);

module_exit(helloworld_exit);

MODULE_LICENSE("GPL");
```





