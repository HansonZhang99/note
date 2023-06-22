#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/mod_devicetable.h>
#include <linux/log2.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/nvmem-provider.h>
#include <linux/platform_data/at24.h>
#include <asm/uaccess.h>
static int major = 0;
static struct class *ap3216_class;
struct i2c_client *g_ap3216_client;


static ssize_t ap3216_read(struct file *file, char __user *buf, size_t count,
		loff_t *offset)
{
	int size = 6;
	unsigned short value;
	unsigned char valBuf[6];
	int ret;
	if(count < size)
	{
		return -EINVAL;
	}

	value = i2c_smbus_read_word_data(g_ap3216_client, 0xa);
	valBuf[0] = value & 0xff;
	valBuf[1] = (value >> 8) & 0xff;
	value = i2c_smbus_read_word_data(g_ap3216_client, 0xc);
	valBuf[2] = value & 0xff;
	valBuf[3] = (value >> 8) & 0xff;
	value = i2c_smbus_read_word_data(g_ap3216_client, 0xe);
	valBuf[4] = value & 0xff;
	valBuf[5] = (value >> 8) & 0xff;
	
	ret = copy_to_user(buf, valBuf, size);
	return size;
}

static int ap3216_open(struct inode *inode, struct file *file)
{
	i2c_smbus_write_byte_data(g_ap3216_client, 0, 0x4);//reset
	mdelay(20);
	i2c_smbus_write_byte_data(g_ap3216_client, 0, 0x3);//ALS+PS+IR acitve
	mdelay(350);

	return 0;
}
static struct file_operations ap3216_fops = 
{
	.owner = THIS_MODULE,
	.read = ap3216_read,
	.open = ap3216_open,
};

static const struct of_device_id ap3216_of_match[] = {
	{ .compatible = "atmel,ap3216", },
	{ }
};
	
static const struct i2c_device_id ap3216_ids[] = {
	{ "ap3216", 0 },
	{ /* END OF LIST */ }
};	
	
static int ap3216_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk("ap3216_probe, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	g_ap3216_client = client;
	major = register_chrdev(0, "ap3216_chrdev", &ap3216_fops);
	if(major < 0)
	{
		printk("register_chrdev error, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	ap3216_class = class_create(THIS_MODULE, "ap3216_class");

	device_create(ap3216_class , NULL, MKDEV(major, 0), NULL, "ap3216_device");
	return 0;
}

static int ap3216_remove(struct i2c_client *client)
{
	printk("ap3216_remove, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	device_destroy(ap3216_class, MKDEV(major, 0));

	class_destroy(ap3216_class);
	
	unregister_chrdev(major, "ap3216_chrdev");

	return 0;
}
static struct i2c_driver ap3216_driver = {
	.driver = {
		.name = "ap3216_driver",
		.of_match_table = ap3216_of_match,
	},
	
	.probe = ap3216_probe,
	.remove = ap3216_remove,
	.id_table = ap3216_ids,
};

static int __init ap3216_init(void)
{
	
	printk("ap3216_init, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	return i2c_add_driver(&ap3216_driver);
}
module_init(ap3216_init);

static void __exit ap3216_exit(void)
{
	i2c_del_driver(&ap3216_driver);
}
module_exit(ap3216_exit);

MODULE_AUTHOR("hanson zhang");
MODULE_LICENSE("GPL");


