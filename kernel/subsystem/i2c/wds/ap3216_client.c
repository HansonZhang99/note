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

struct i2c_client *client;

static struct i2c_board_info ap3216_i2c_info[] = {
	{ I2C_BOARD_INFO("ap3216", 0x1e), },
};

const unsigned short addr_list[] = {
		0x1e, I2C_CLIENT_END
};
#if 1
static int __init ap3216_client_init(void)
{
	struct i2c_adapter *adp;
	printk("ap3216_client_init, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	
	adp = i2c_get_adapter(0);
	
	client = i2c_new_device(adp, ap3216_i2c_info);
	
	
	i2c_put_adapter(adp);
	return 0;
}
#else
static int __init ap3216_client_init(void)
{
	struct i2c_adapter *adp;
	printk("ap3216_client_init, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	adp = i2c_get_adapter(0);
	
	client = i2c_new_probed_device(adp, ap3216_i2c_info, addr_list, NULL);
	
	i2c_put_adapter(adp);
	return 0;
}
#endif
module_init(ap3216_client_init);

static void __exit ap3216_client_exit(void)
{
	i2c_unregister_device(client);
}
module_exit(ap3216_client_exit);

MODULE_AUTHOR("hanson zhang");
MODULE_LICENSE("GPL");

