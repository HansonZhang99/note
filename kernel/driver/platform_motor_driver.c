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
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/atomic.h>
//#include <asm/uaccess.h>
#include <linux/semaphore.h>

static struct semaphore my_device_sem;

struct motor_gpio
{
	struct gpio_desc *desc;
	int gpio;
	int flag;
};

static struct motor_gpio *motor_desc;
static struct class *motor_class;
static struct task_struct *motor_control_thread;
static atomic_t motor_running = ATOMIC_INIT(0);
static atomic_t flag = ATOMIC_INIT(0);
static atomic_t stop = ATOMIC_INIT(0);

static DEFINE_MUTEX(running_mutex);

static int major;
static unsigned long ms_sleep = 10;
static const struct of_device_id motor_table[] = {
    { .compatible = "motor-control" },
    { },
};

typedef enum 
{
	MOTOR_CONTROL_FORWARD = (1 << 10) + 1,//(ioctl的cmd乖乖取个大点的值吧，从0开始调了我1小时)
	MOTOR_CONTROL_RESERVE,
	MOTOR_CONTROL_ON,
	MOTOR_CONTROL_OFF,
	MOTOR_CONTROL_SPEED,
}motor_control_cmd;


static void start_running(void)
{
	atomic_set(&motor_running, 1);
}

static void stop_running(void)
{
	atomic_set(&motor_running, 0);
}

static int check_running(void)
{
	return atomic_read(&motor_running) == 1;
}

static void set_flag(int flg)
{
	atomic_set(&flag, flg);
}

static int get_flag(void)
{
	return atomic_read(&flag);
}

static void set_stop(int stp)
{
	atomic_set(&stop, stp);
}

static int need_stop(void)
{
	return atomic_read(&stop) == 1;
}


static void motor_contorl_forward(void)
{
	gpiod_set_value(motor_desc[0].desc, 1);
	gpiod_set_value(motor_desc[1].desc, 0);
	gpiod_set_value(motor_desc[2].desc, 0);
	gpiod_set_value(motor_desc[3].desc, 0);
	mdelay(ms_sleep);
	gpiod_set_value(motor_desc[0].desc, 0);
	gpiod_set_value(motor_desc[1].desc, 1);
	gpiod_set_value(motor_desc[2].desc, 0);
	gpiod_set_value(motor_desc[3].desc, 0);
	mdelay(ms_sleep);
	gpiod_set_value(motor_desc[0].desc, 0);
	gpiod_set_value(motor_desc[1].desc, 0);
	gpiod_set_value(motor_desc[2].desc, 1);
	gpiod_set_value(motor_desc[3].desc, 0);
	mdelay(ms_sleep);
	gpiod_set_value(motor_desc[0].desc, 0);
	gpiod_set_value(motor_desc[1].desc, 0);
	gpiod_set_value(motor_desc[2].desc, 0);
	gpiod_set_value(motor_desc[3].desc, 1);
	mdelay(ms_sleep);
}

static void motor_contorl_reserve(void)
{
	gpiod_set_value(motor_desc[0].desc, 0);
	gpiod_set_value(motor_desc[1].desc, 0);
	gpiod_set_value(motor_desc[2].desc, 0);
	gpiod_set_value(motor_desc[3].desc, 1);
	mdelay(ms_sleep);
	gpiod_set_value(motor_desc[0].desc, 0);
	gpiod_set_value(motor_desc[1].desc, 0);
	gpiod_set_value(motor_desc[2].desc, 1);
	gpiod_set_value(motor_desc[3].desc, 0);
	mdelay(ms_sleep);
	gpiod_set_value(motor_desc[0].desc, 0);
	gpiod_set_value(motor_desc[1].desc, 1);
	gpiod_set_value(motor_desc[2].desc, 0);
	gpiod_set_value(motor_desc[3].desc, 0);
	mdelay(ms_sleep);
	gpiod_set_value(motor_desc[0].desc, 1);
	gpiod_set_value(motor_desc[1].desc, 0);
	gpiod_set_value(motor_desc[2].desc, 0);
	gpiod_set_value(motor_desc[3].desc, 0);
	mdelay(ms_sleep);
}

static void reset_motor_control(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int count;
	int i;
	count = of_gpio_count(node);
	for(i = 0; i < count; i++)
	{
		gpiod_set_value(motor_desc[i].desc, 0);
	}
	return ;
}


static int motor_contorl_thread_func(void *data)
{
	struct platform_device *pdev = (struct platform_device *)data;
	while(!need_stop())
	{
		if(check_running())
		{
			if(get_flag() == 0)
				motor_contorl_forward();
			else 
				motor_contorl_reserve();
		}
		else 
		{
			reset_motor_control(pdev);
			mdelay(200);
		}
		
	}
	printk("%s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static long motor_control_ioctl(struct file *file, unsigned int cmd, unsigned long args)
{
	int ret = 0;
	unsigned long speed = 5;
	switch (cmd)
	{
		case MOTOR_CONTROL_SPEED:
			ret = copy_from_user(&speed, (unsigned long *)args, sizeof(unsigned long));
			//speed = *((unsigned long *)args);//段错误
			if(speed > 20)
				speed = 20;
			if(speed < 1)
				speed = 1;
			ms_sleep = speed * 4;
			break;
		case MOTOR_CONTROL_FORWARD:
			set_flag(0);
			break;
		case MOTOR_CONTROL_RESERVE:
			set_flag(1);
			break;
		case MOTOR_CONTROL_ON:
			start_running();
			break;
		case MOTOR_CONTROL_OFF:
			stop_running();
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

static int motor_control_open(struct inode *inode, struct file *file)
{
	if(down_interruptible(&my_device_sem)) 
	{
       return -EBUSY;
    }
    
    return 0;
}

static int motor_control_release(struct inode *inode, struct file *file)
{
	up(&my_device_sem);
    return 0;
}

static struct file_operations motor_control_fops = {
	.owner	 = THIS_MODULE,
	.open    = &motor_control_open, 
	.unlocked_ioctl    = &motor_control_ioctl,
	.release    = &motor_control_release,
};
	


static int motor_control_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	int count;
	int i;
	enum of_gpio_flags flag;
	
	printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	
	count = of_gpio_count(node);
	if(!count)
	{
		printk("!count, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	}
		
	motor_desc = kzalloc(sizeof(struct motor_gpio) * count, GFP_KERNEL);
	if(!motor_desc)
	{
		printk("kzalloc error, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	}	

	for(i = 0; i < count; i++)
	{
		motor_desc[i].gpio = of_get_gpio_flags(node, i, &flag);
		motor_desc[i].desc = gpio_to_desc(motor_desc[i].gpio);
		motor_desc[i].flag = flag & OF_GPIO_ACTIVE_LOW;
		gpiod_direction_output(motor_desc[i].desc, 0);
	}	
	reset_motor_control(pdev);
	
	major = register_chrdev(0, "motor_control", &motor_control_fops);

	motor_class = class_create(THIS_MODULE, "motor_control_class");
	if (IS_ERR(motor_class)) {
		printk("%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
		unregister_chrdev(major, "motor_control");
		return PTR_ERR(motor_class);
	}

	device_create(motor_class, NULL, MKDEV(major, 0), NULL, "motor_control");

	motor_control_thread = kthread_run(motor_contorl_thread_func, (void *)pdev, "motor_contorl_thread");

	//start_running();
	return  0;
}

static int motor_control_remove(struct platform_device *pdev)
{
	set_stop(1);
	mdelay(250);
	reset_motor_control(pdev);
	device_destroy(motor_class, MKDEV(major, 0));
	class_destroy(motor_class);
	unregister_chrdev(major, "motor_control");

	kfree(motor_desc);
	return 0;
}

static struct platform_driver motor_control_driver =
{
	.probe      = motor_control_probe,
    .remove     = motor_control_remove,
    .driver     = {
        .name   = "motor-control",
        .of_match_table = motor_table,
    },
};

static int __init motor_control_init(void)
{
	int ret = -1;
	printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	sema_init(&my_device_sem, 1);  
	ret = platform_driver_register(&motor_control_driver);
	if(ret)
	{
		printk("platform_driver_register error, file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	return 0;
}

static void __exit motor_control_exit(void)
{
	printk("file:%s, function:%s, line:%d\n", __FILE__, __FUNCTION__, __LINE__);
	platform_driver_unregister(&motor_control_driver);
}

module_init(motor_control_init);
module_exit(motor_control_exit);

MODULE_LICENSE("GPL");

