

# imx6ull 按键驱动(dts)

## 禁用gpio-keys dts节点

![image-20230528195111562](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230528195111562.png)

## datasheet key引脚连接图

![image-20230528195336700](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230528195336700.png)

## 使用工具配置引脚为gpio功能

![image-20230528224646820](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230528224646820.png)

## 在设备树中添加对应节点

![image-20230528225050102](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230528225050102.png)

![image-20230528225119788](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230528225119788.png)

![image-20230528225139184](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230528225139184.png)



## 驱动代码

```cpp
#include <linux/module.h>
#include <linux/poll.h>

#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/stat.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/tty.h>
#include <linux/kmod.h>
#include <linux/gfp.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/fcntl.h>

struct gpio_key
{
	int gpio;
	struct gpio_desc *gpiod;
	int flag;
	int irq;
};
#define BUF_LEN 128
static int g_keys[BUF_LEN];
static int r, w;



static struct gpio_key *gpio_keys_100ask;

static int major = 0;
static struct class *gpio_key_class;
struct fasync_struct *button_fasync;//信号驱动

static DECLARE_WAIT_QUEUE_HEAD(gpio_key_wait);//等到队列

#define NEXT_POS(x) ((x+1) % BUF_LEN)

static int is_key_buf_empty(void)
{
	return (r == w);
}

static int is_key_buf_full(void)
{
	return (r == NEXT_POS(w));
}

static void put_key(int key)
{
	if (!is_key_buf_full())
	{
		g_keys[w] = key;
		w = NEXT_POS(w);
	}
}

static int get_key(void)
{
	int key = 0;
	if (!is_key_buf_empty())
	{
		key = g_keys[r];
		r = NEXT_POS(r);
	}
	return key;
}

static ssize_t my_gpio_key_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	int key;
	int err;
	if (is_key_buf_empty() && (file->f_flags & O_NONBLOCK))
		return -EAGAIN;
	wait_event_interruptible(gpio_key_wait, !is_key_buf_empty());

	key = get_key();
	err = copy_to_user(buf, &key, 4);
	
	return 4;
}

static unsigned int my_gpio_key_drv_poll(struct file *fp, poll_table * wait)
{
	printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
	poll_wait(fp, &gpio_key_wait, wait);
	return is_key_buf_empty() ? 0 : POLLIN | POLLRDNORM;
}

static int my_gpio_key_drv_fasync(int fd, struct file *file, int on)
{
	if(fasync_helper(fd, file, on, &button_fasync) >= 0)
		return 0;
	else
		return -EIO;
}


static struct file_operations gpio_key_fops = {
	.owner	 = THIS_MODULE,
    //./open函数不是必要的，一般open函数中会进行一些初始化，将一些初始化数据放入file->private_data，以便read.write...使用，但是此处不需要
	.read    = my_gpio_key_drv_read,
	.poll    = my_gpio_key_drv_poll,
	.fasync  = my_gpio_key_drv_fasync,
};

static irqreturn_t gpio_key_isr(int irq, void *dev_id)
{
	struct gpio_key *gpio_key = dev_id;
	int val;
	int key;

	val = gpiod_get_value(gpio_key->gpiod);
	printk("key %d %d\n", gpio_key->gpio, val);
	key = (gpio_key->gpio << 8) | val;
	put_key(key);
	wake_up_interruptible(&gpio_key_wait);//唤醒等待队列

	kill_fasync(&button_fasync, SIGIO, POLL_IN);//信号驱动
	
	return IRQ_HANDLED;
}
static int my_gpio_key_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int count;
	int i;
	enum of_gpio_flags flag;
	int err; 
	
	printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	if(!node)
	{
		printk("!node, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	
	count = of_gpio_count(node);
	if(!count)
	{
		printk("!count, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	}

		
	gpio_keys_100ask = kzalloc(sizeof(struct gpio_key) * count, GFP_KERNEL);
	if(!gpio_keys_100ask)
	{
		printk("kzalloc error, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	}	

	for(i = 0; i < count; i++)
	{
		gpio_keys_100ask[i].gpio = of_get_gpio_flags(node, i, &flag);
		if (gpio_keys_100ask[i].gpio < 0)
		{
			printk("gpio_keys_100ask[i].gpio < 0, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
			return -1;
		}
		gpio_keys_100ask[i].gpiod = gpio_to_desc(gpio_keys_100ask[i].gpio);
		gpio_keys_100ask[i].irq = gpio_to_irq(gpio_keys_100ask[i].gpio);
		gpio_keys_100ask[i].flag = flag & OF_GPIO_ACTIVE_LOW;
	}

	for (i = 0; i < count; i++)
	{
		err = request_irq(gpio_keys_100ask[i].irq, gpio_key_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "100ask_gpio_key", &gpio_keys_100ask[i]);
	}

	major = register_chrdev(0, "100ask_gpio_key", &gpio_key_fops);

	gpio_key_class = class_create(THIS_MODULE, "100ask_gpio_key_class");
	if (IS_ERR(gpio_key_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "100ask_gpio_key");
		return PTR_ERR(gpio_key_class);
	}

	device_create(gpio_key_class, NULL, MKDEV(major, 0), NULL, "100ask_gpio_key");
	return  0;
}

static int my_gpio_key_remove(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int count;
	int i;
	device_destroy(gpio_key_class, MKDEV(major, 0));
	class_destroy(gpio_key_class);
	unregister_chrdev(major, "100ask_gpio_key");

	count = of_gpio_count(node);
	for (i = 0; i < count; i++)
	{
		free_irq(gpio_keys_100ask[i].irq, &gpio_keys_100ask[i]);
	}
	kfree(gpio_keys_100ask);
	return 0;
}

static const struct of_device_id my_keys[] = {
    { .compatible = "my_gpio_key" },
    { },
};


static struct platform_driver gpio_key_driver =
{
	.probe      = my_gpio_key_probe,
    .remove     = my_gpio_key_remove,
    .driver     = {
        .name   = "my_gpio_key",
        .of_match_table = my_keys,
    },
};

static int __init gpioKeyDrv_init(void)
{
	int ret = -1;
	printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	ret = platform_driver_register(&gpio_key_driver);
	if(ret)
	{
		printk("platform_driver_register error, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	return 0;
}

static void __exit gpioKeyDrv_exit(void)
{
	printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	platform_driver_unregister(&gpio_key_driver);
}

module_init(gpioKeyDrv_init);
module_exit(gpioKeyDrv_exit);

MODULE_LICENSE("GPL");


```

## 按键内核demsg信息

```shell
[  120.529002] file:/home/zhhhhhh/bsp/IMX6ULL/kernel_driver/gpioKeyDrv.c, function:gpioKeyDrv_init, line:229
[  120.530009] file:/home/zhhhhhh/bsp/IMX6ULL/kernel_driver/gpioKeyDrv.c, function:my_gpio_key_probe, line:141
[  131.754189] key 129 0
[  131.895461] key 129 1
[  142.300113] key 110 0
[  142.490976] key 110 1
```

