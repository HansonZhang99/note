# led子系统驱动内核示例(linux kernel 3.0)

`leds-gpio.c`是led子系统的一个实现文件

```cpp
static int __init gpio_led_init(void)
{
    return platform_driver_register(&gpio_led_driver);
}
```

采用虚拟总线注册，设备和驱动匹配后会调用函数探测函数probe

```cpp
static struct platform_driver gpio_led_driver = {
    .probe      = gpio_led_probe,
    .remove     = __devexit_p(gpio_led_remove),
    .driver     = {
        .name   = "leds-gpio",
        .owner  = THIS_MODULE,
        .of_match_table = of_gpio_leds_match,
    },
};

static int __devinit gpio_led_probe(struct platform_device *pdev)
{
    struct gpio_led_platform_data *pdata = pdev->dev.platform_data;
    struct gpio_leds_priv *priv;
    int i, ret = 0;

    if (pdata && pdata->num_leds) {
        priv = kzalloc(sizeof_gpio_leds_priv(pdata->num_leds),
                GFP_KERNEL);//为led所有信息分配内存
        if (!priv)
            return -ENOMEM;

        priv->num_leds = pdata->num_leds;//获取灯的数量
        for (i = 0; i < priv->num_leds; i++) {
            ret = create_gpio_led(&pdata->leds[i],
                          &priv->leds[i],
                          &pdev->dev, pdata->gpio_blink_set);
            if (ret < 0) {
                /* On failure: unwind the led creations */
                for (i = i - 1; i >= 0; i--)
                    delete_gpio_led(&priv->leds[i]);
                kfree(priv);
                return ret;
            }
        }
    } else {
        priv = gpio_leds_create_of(pdev);
        if (!priv)
            return -ENODEV;
    }

    platform_set_drvdata(pdev, priv);//设置priv为pdev的私有成员

    return 0;
}

```



```cpp
static int __devinit create_gpio_led(const struct gpio_led *template,
    struct gpio_led_data *led_dat, struct device *parent,
    int (*blink_set)(unsigned, int, unsigned long *, unsigned long *))
{
    int ret, state;

    led_dat->gpio = -1;

    /* skip leds that aren't available */
    if (!gpio_is_valid(template->gpio)) {
        printk(KERN_INFO "Skipping unavailable LED gpio %d (%s)\n",
                template->gpio, template->name);//判断gpio管脚是否合法

        return 0;
    }

    ret = gpio_request(template->gpio, template->name);//申请gpio
    if (ret < 0)
        return ret;

    led_dat->cdev.name = template->name;//led的名字
    led_dat->cdev.default_trigger = template->default_trigger;//led的默认触发器
    led_dat->gpio = template->gpio;//gpio
    led_dat->can_sleep = gpio_cansleep(template->gpio);//设置为可睡眠
    led_dat->active_low = template->active_low;
    led_dat->blinking = 0;
    if (blink_set) {
        led_dat->platform_gpio_blink_set = blink_set;
        led_dat->cdev.blink_set = gpio_blink_set;
    }//如果blink_set函数存在，就设置为它，并设置led_classdev结构的cdev->cdev.blink_set为gpio_blink_set
    led_dat->cdev.brightness_set = gpio_led_set;//同上
    if (template->default_state == LEDS_GPIO_DEFSTATE_KEEP)
        state = !!gpio_get_value(led_dat->gpio) ^ led_dat->active_low;
    else
        state = (template->default_state == LEDS_GPIO_DEFSTATE_ON);
    led_dat->cdev.brightness = state ? LED_FULL : LED_OFF;
    if (!template->retain_state_suspended)
        led_dat->cdev.flags |= LED_CORE_SUSPENDRESUME;

    ret = gpio_direction_output(led_dat->gpio, led_dat->active_low ^ state);
    if (ret < 0)
        goto err;

    INIT_WORK(&led_dat->work, gpio_led_work);//初始化工作队列和工作队列函数

    ret = led_classdev_register(parent, &led_dat->cdev);//注册led驱动
    if (ret < 0)
        goto err;

    return 0;
err:
    gpio_free(led_dat->gpio);
    return ret;
}

static void gpio_led_work(struct work_struct *work)
{
    struct gpio_led_data    *led_dat =
        container_of(work, struct gpio_led_data, work);


    if (led_dat->blinking) {
        led_dat->platform_gpio_blink_set(led_dat->gpio,
                         led_dat->new_level,
                         NULL, NULL);
        led_dat->blinking = 0;
    } else
        gpio_set_value_cansleep(led_dat->gpio, led_dat->new_level);
}
#define gpio_set_value_cansleep gpio_set_value
```





```cpp
static int gpio_blink_set(struct led_classdev *led_cdev,
    unsigned long *delay_on, unsigned long *delay_off)
{
    struct gpio_led_data *led_dat =
        container_of(led_cdev, struct gpio_led_data, cdev);


    led_dat->blinking = 1;
    return led_dat->platform_gpio_blink_set(led_dat->gpio, GPIO_LED_BLINK,
                        delay_on, delay_off);//调用led_dat->platform_gpio_blink_set = blink_set;
}

static void gpio_led_set(struct led_classdev *led_cdev,
    enum led_brightness value)
{
    struct gpio_led_data *led_dat =
        container_of(led_cdev, struct gpio_led_data, cdev);
    int level;

    if (value == LED_OFF)
        level = 0;
    else
        level = 1;

    if (led_dat->active_low)
        level = !level;
//如果设置active_low为高电平，则反转level
    /* Setting GPIOs with I2C/etc requires a task context, and we don't
     * seem to have a reliable way to know if we're already in one; so
     * let's just assume the worst.
     */
    if (led_dat->can_sleep) {
        led_dat->new_level = level;
        schedule_work(&led_dat->work);//如果设置可睡眠，设置level并加入工作队列到系统工作队列，工作队列函数会延后被执行
    } else {
        if (led_dat->blinking) {
            led_dat->platform_gpio_blink_set(led_dat->gpio, level,
                             NULL, NULL);
            led_dat->blinking = 0;//如果blink为0，则调用blink函数
        } else
            gpio_set_value(led_dat->gpio, level);//否则直接设置灯的亮灭
    }
}
```

往往探测函数会取出平台设备结构体的platform_data成员

```cpp
include/linux/leds.h

struct gpio_led_platform_data {
    int         num_leds;
    const struct gpio_led *leds;
            
#define GPIO_LED_NO_BLINK_LOW   0   /* No blink GPIO state low */
#define GPIO_LED_NO_BLINK_HIGH  1   /* No blink GPIO state high */
#define GPIO_LED_BLINK      2   /* Please, blink */
    int     (*gpio_blink_set)(unsigned gpio, int state,
                    unsigned long *delay_on,
                    unsigned long *delay_off);
};

struct gpio_led {
    const char *name;
    const char *default_trigger;
    unsigned    gpio;
    unsigned    active_low : 1;
    unsigned    retain_state_suspended : 1;
    unsigned    default_state : 2;
    /* default_state should be one of LEDS_GPIO_DEFSTATE_(ON|OFF|KEEP) */
};

#define LEDS_GPIO_DEFSTATE_OFF      0
#define LEDS_GPIO_DEFSTATE_ON       1
#define LEDS_GPIO_DEFSTATE_KEEP     2

struct gpio_leds_priv {
    int num_leds;
    struct gpio_led_data leds[];
};

struct gpio_led_data {
    struct led_classdev cdev;
    unsigned gpio;
    struct work_struct work;
    u8 new_level;
    u8 can_sleep;
    u8 active_low;
    u8 blinking;
    int (*platform_gpio_blink_set)(unsigned gpio, int state,
            unsigned long *delay_on, unsigned long *delay_off);
};
```

## s3c2440使用leds-gpio.c添加led驱动到内核

1.构造结构体`struct gpio_led`

```cpp
#include<linux/leds.h>


static struct gpio_led gpio_leds[]={
    {
        .name="led1",
        .gpio=S3C2410_GPB(5),
        .default_state=LEDS_GPIO_DEFSTATE_OFF,
        .active_low=LEDS_GPIO_DEFSTATE_ON,
    },
    {
        .name="led2",
        .gpio=S3C2410_GPB(6),
        .default_state=LEDS_GPIO_DEFSTATE_OFF,
        .active_low=LEDS_GPIO_DEFSTATE_ON,
    },
    {
        .name="led3",
        .gpio=S3C2410_GPB(8),
        .default_state=LEDS_GPIO_DEFSTATE_OFF,
        .active_low=LEDS_GPIO_DEFSTATE_ON,
    },
    {
        .name="led4",
        .gpio=S3C2410_GPB(10),
        .default_state=LEDS_GPIO_DEFSTATE_OFF,
        .active_low=LEDS_GPIO_DEFSTATE_ON,
    },
};
```

2.构造平台设备

```cpp
static struct gpio_led_platform_data gpio_led_info={
    .leds=gpio_leds,
    .num_leds=ARRAY_SIZE(gpio_leds),
};


static struct platform_device s3c_device_led={
    .name="leds-gpio",
    .id=-1,
    .dev={
        .platform_data=&gpio_led_info,
    },
};
```

3.添加驱动到内核

```cpp
添加上两项到内核文件arch/arm/mach-s3c2440/mach-smdk2440.c
并在：
static struct platform_device *smdk2440_devices[] __initdata = {
    &s3c_device_ohci,
    &s3c_device_lcd,
    &s3c_device_wdt,
    &s3c_device_i2c0,
    &s3c_device_iis,
    &s3c_device_dm9000,//add for dm9000
    &s3c_device_led,//添加
    &ds18b20_device_w1_gpio,
    &s3c2440_gpio_keys,
};
```

4.修改内核menuconfig

```shell
Device Drivers  --->
                -*- GPIO Support  --->
                              [*]   /sys/class/gpio/... (sysfs interface)   
                [*] LED Support  --->
                                <*>   LED Support for GPIO connected LEDs                   
                                  [*]     Platform device bindings for GPIO LEDs
                                        <*>   LED Timer Trigger
                                  [*]   LED IDE Disk Trigger
                                <*>   LED Heartbeat Trigger
                                <*>   LED backlight Trigger
                                <*>   LED GPIO Trigger
                                <*>   LED Default ON Trigger
```

5.编译移植内核

```shell
/sys/class/leds >: ls
led1  led2  led3  led4
/sys/class/leds >: cd led1
/sys/devices/platform/leds-gpio/leds/led1 >: ls
brightness      max_brightness  subsystem       uevent
device          power           trigger
/sys/devices/platform/leds-gpio/leds/led1 >: echo 1 > brightness//灯亮
/sys/devices/platform/leds-gpio/leds/led1 >: echo 0 > brightness//灯灭
/sys/devices/platform/leds-gpio/leds/led1 >: echo timer > trigger//灯闪
/sys/devices/platform/leds-gpio/leds/led1 >: echo heartbeat > trigger//灯闪，类似于心脏的跳动
```

