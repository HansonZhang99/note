#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_data/i2c-imx.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/sched.h>
#include <linux/slab.h>

static struct i2c_adapter *adapter;
static unsigned char eeprom_buffer[512];
static int eeprom_address_index = 0;

static int i2c_eeprom_transfer(struct i2c_adapter *adapter, struct i2c_msg *msg)
{
	int i;
	if(msg->flags & I2C_M_RD)
	{
		for(i = 0; i < msg->len; i++)
		{
			msg->buf[i] = eeprom_buffer[eeprom_address_index++];
			if(eeprom_address_index == 512)
				eeprom_address_index = 0;
		}
	}
	else 
	{
		if(msg->len >= 1)
		{
			eeprom_address_index = msg->buf[0];
			for(i = 1; i < msg->len; i++)
			{
				eeprom_buffer[eeprom_address_index++] = msg->buf[i];
				if(eeprom_address_index == 512)
					eeprom_address_index = 0;
			}
		}
	}
	return 0;
}

static int i2c_virtual_xfer(struct i2c_adapter *adapter,
						struct i2c_msg *msgs, int num)
{
	int i;
	for(i = 0; i < num; i++)
	{
		if(msgs[i].addr == 0x60)
		{
			i2c_eeprom_transfer(adapter, &msgs[i]);
		}
		else 
		{
			i = -EIO;
			break;
		}
	}

	return i;
}

static u32 i2c_virtual_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL
		| I2C_FUNC_SMBUS_READ_BLOCK_DATA;
}

static struct i2c_algorithm i2c_virtual_algo = {
	.master_xfer	= i2c_virtual_xfer,
	.functionality	= i2c_virtual_func,
};




static int i2c_virtual_adapter_probe(struct platform_device *pdev)
{
	adapter = kzalloc(sizeof(struct i2c_adapter), GFP_KERNEL);
	adapter->algo = &i2c_virtual_algo;
	adapter->nr = -1;
	
	strlcpy(adapter->name, "virtual_adapter", sizeof(adapter->name));
	
	memset(eeprom_buffer, 0, 512);
	
	i2c_add_adapter(adapter);
	
	return 0;
}

static int i2c_virtual_adapter_remove(struct platform_device *pdev)
{
	i2c_del_adapter(adapter);
	kfree(adapter);
	return 0;
}

static const struct of_device_id i2c_virtual_adapter_ids[] = {
	{ .compatible = "none,i2c_virtual_adapter"},

	{ /* sentinel */ }
};	

static struct platform_driver i2c_virtual_adapter_driver = {
	.probe = i2c_virtual_adapter_probe,
	.remove = i2c_virtual_adapter_remove,
	.driver = {
		.name = "i2c_virtual_adapter",
		.of_match_table = i2c_virtual_adapter_ids,
	},
};

static int __init i2c_virtual_adapter_init(void)
{
	return platform_driver_register(&i2c_virtual_adapter_driver);
}
module_init(i2c_virtual_adapter_init);

static void __exit i2c_virtual_adapter_exit(void)
{
	platform_driver_unregister(&i2c_virtual_adapter_driver);
}
module_exit(i2c_virtual_adapter_exit);




MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("i2c virtual adapter test");
