# led subsystem(linux kernel 3.0)

## 核心文件

driver/leds/led-class.c

driver/leds/led-core.c

driver/leds/led-triggers.c

include/linux/leds.h

Makefile：

```Makefile
# LED Core
obj-$(CONFIG_NEW_LEDS)          += led-core.o
obj-$(CONFIG_LEDS_CLASS)        += led-class.o
obj-$(CONFIG_LEDS_TRIGGERS)     += led-triggers.o

# LED Triggers
obj-$(CONFIG_LEDS_TRIGGER_TIMER)    += ledtrig-timer.o
obj-$(CONFIG_LEDS_TRIGGER_IDE_DISK) += ledtrig-ide-disk.o
obj-$(CONFIG_LEDS_TRIGGER_HEARTBEAT)    += ledtrig-heartbeat.o
obj-$(CONFIG_LEDS_TRIGGER_BACKLIGHT)    += ledtrig-backlight.o
obj-$(CONFIG_LEDS_TRIGGER_GPIO)     += ledtrig-gpio.o
obj-$(CONFIG_LEDS_TRIGGER_DEFAULT_ON)   += ledtrig-default-on.o
```

![image-20230424012946677](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424012946677.png)

## led数据结构

```cpp
struct led_classdev {
    const char      *name;/* 新建立的led设备的名字，会在"/sys/class/leds/目录下面显示" */
    int          brightness;/* led的默认亮度值 */
    int          max_brightness;/* led的最大亮度值，如果大于255或小于0，则设置为255 */
    int          flags;/* 此flags表示led的状态 */

    /* flags的低16位反映led的状态 */ 
#define LED_SUSPENDED       (1 << 0)
    /* flags的高16位反映led的控制信息*/
#define LED_CORE_SUSPENDRESUME  (1 << 16)

/* 设置LED亮度值;不能休眠,如果有需要可以使用工作队列实现
* led_cdev:对应的led设备
* brightness:亮度值
*/
    void        (*brightness_set)(struct led_classdev *led_cdev,
                      enum led_brightness brightness);
    /* Get LED brightness level */
    enum led_brightness (*brightness_get)(struct led_classdev *led_cdev);

    /*
     * Activate hardware accelerated blink, delays are in milliseconds
     * and if both are zero then a sensible default should be chosen.
     * The call should adjust the timings in that case and if it can't
     * match the values specified exactly.
     * Deactivate blinking again when the brightness is set to a fixed
     * value via the brightness_set() callback.
     */
    int     (*blink_set)(struct led_classdev *led_cdev,
                     unsigned long *delay_on,
                     unsigned long *delay_off);

    struct device       *dev;
    struct list_head     node;          /* LED Device list */
    const char      *default_trigger;   /* Trigger to use 触发方式,可以设置哪些字符串,可以查看ledtrig-xx.c中的定义触发方式*/

    unsigned long        blink_delay_on, blink_delay_off;
    struct timer_list    blink_timer;/* 闪烁用到的定时器timer */
    int          blink_brightness;/* 闪烁时的LED亮度值 */

#ifdef CONFIG_LEDS_TRIGGERS
    /* Protects the trigger data below */
    struct rw_semaphore  trigger_lock;
/* LED触发方式结构体,写驱动时不用设置,当设置了default_trigger之后,注册到内核中会设置此成员 */
    struct led_trigger  *trigger;
    struct list_head     trig_list;/* 触发器链表 */
    void            *trigger_data;/* 保存触发器的私有数据 */
#endif
};

struct led_trigger {
    /* Trigger Properties */
    const char   *name;
/* 设置触发器的名称,很重要,led设备如果设置了触发器会根据此name进行比较,如果比较成功则设置 */
    void        (*activate)(struct led_classdev *led_cdev);/* 此函数中一般会创建设备节点,使能led */
    void        (*deactivate)(struct led_classdev *led_cdev);/* 此函数中会注销设备节点,禁止led */
                     
    /* LEDs under control by this trigger (for simple triggers) */
    rwlock_t      leddev_list_lock;
    struct list_head  led_cdevs;    /* 用到此触发方式的设备链表 */    
    
    /* Link to next registered trigger */
    struct list_head  next_trig;/* 触发器链表 */
};  
```

## 初始化

```cpp
static int __init leds_init(void)
{
    leds_class = class_create(THIS_MODULE, "leds");//在sys/class下创建leds目录
    if (IS_ERR(leds_class))
        return PTR_ERR(leds_class);
    leds_class->suspend = led_suspend;
    leds_class->resume = led_resume;
    leds_class->dev_attrs = led_class_attrs;// 设置类的device_attribute成员,以后根据此类创建的device都会有这些属性
    return 0;
}
```

在`/sys/class/`目录下面创建一个leds类目录,基于leds这个类的每个设备(device)创建对应的属性文件同时将led-class中的suspend的指针以及resume的指针初始化了，一般来说是当系统休眠的时候系统上层会层层通知各个设备进入睡眠状态，那么负责这个设备的驱动则实际执行睡眠，例如手机的休眠键位，唤醒时调用的是 resume，恢复设备的运行状态，这也是为了省电。

### 属性

```cpp
static struct device_attribute led_class_attrs[] = {
    __ATTR(brightness, 0644, led_brightness_show, led_brightness_store),
    __ATTR(max_brightness, 0444, led_max_brightness_show, NULL),
#ifdef CONFIG_LEDS_TRIGGERS//内核选择此项才会产生此文件
    __ATTR(trigger, 0644, led_trigger_show, led_trigger_store),
#endif
    __ATTR_NULL,
};

#define __ATTR(_name,_mode,_show,_store) { \
    .attr = {.name = __stringify(_name), .mode = _mode },   \
    .show   = _show,                    \
    .store  = _store,                   \
}
```

为每个基于/sys/class/leds的设备都创建三个文件，并为每个文件提供不同的操作方法。

#### led_brightness_show

```cpp
static ssize_t led_brightness_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{   
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    /* no lock needed for this */
    led_update_brightness(led_cdev);
    
    return sprintf(buf, "%u\n", led_cdev->brightness);//返回亮度值到用户空间
}
   
static void led_update_brightness(struct led_classdev *led_cdev)
{   
    if (led_cdev->brightness_get)
        led_cdev->brightness = led_cdev->brightness_get(led_cdev);
}

static int brightness_get(struct backlight_device *bd)
{
    int status, res;
    res = mutex_lock_killable(&brightness_mutex);
    if (res < 0)
        return 0;
    
    res = tpacpi_brightness_get_raw(&status);
    mutex_unlock(&brightness_mutex);
    if (res < 0)
        return 0;
    
    return status & TP_EC_BACKLIGHT_LVLMSK;//返回亮度信息
}   
```

用户执行`cat /sys/class/leds/<device>/brightness`就会调用此函数，获取亮度信息并返回给用户。

#### led_brightness_store

```cpp
static ssize_t led_brightness_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    ssize_t ret = -EINVAL;
    char *after;
    unsigned long state = simple_strtoul(buf, &after, 10);
    size_t count = after - buf;
    if (isspace(*after))
        count++;
    
    if (count == size) {
        ret = count;
        if (state == LED_OFF)
            led_trigger_remove(led_cdev);//亮度值为0，就移除led触发器，led灭
        led_set_brightness(led_cdev, state);//设置led亮度值
    }
    return ret;
}

static inline void led_set_brightness(struct led_classdev *led_cdev,
                    enum led_brightness value)
{       
    if (value > led_cdev->max_brightness)
        value = led_cdev->max_brightness;//如果用户设置的亮度大于最大亮度值，取最大亮度值
    led_cdev->brightness = value;
    if (!(led_cdev->flags & LED_SUSPENDED))
        led_cdev->brightness_set(led_cdev, value);
}

static void brightness_set(struct led_classdev *cdev,
    enum led_brightness value)
{
    struct platform_device *pdev = to_platform_device(cdev->dev->parent);
    const struct mfd_cell *cell = mfd_get_cell(pdev);
    struct asic3 *asic = dev_get_drvdata(pdev->dev.parent);
    u32 timebase;
    unsigned int base;
    timebase = (value == LED_OFF) ? 0 : (LED_EN|0x4);
    base = led_n_base[cell->id];
    asic3_write_register(asic, (base + ASIC3_LED_PeriodTime), 32);
    asic3_write_register(asic, (base + ASIC3_LED_DutyTime), 32);
    asic3_write_register(asic, (base + ASIC3_LED_AutoStopCount), 0);
    asic3_write_register(asic, (base + ASIC3_LED_TimeBase), timebase);//设置亮度值
}
```

用户执行`echo val > /sys/class/leds/<device>/brightness `就回调用此函数，设置led灯亮度

#### led_max_brightness_show

```cpp
static ssize_t led_max_brightness_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    
    return sprintf(buf, "%u\n", led_cdev->max_brightness);
}
```

返回最大亮度值给用户

#### led_trigger_show

```cpp
ssize_t led_trigger_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct led_trigger *trig;
    int len = 0;
    down_read(&triggers_list_lock);
    down_read(&led_cdev->trigger_lock);//获取临界资源
    if (!led_cdev->trigger)
        len += sprintf(buf+len, "[none] ");
    else
        len += sprintf(buf+len, "none ");
    
    list_for_each_entry(trig, &trigger_list, next_trig) {
        if (led_cdev->trigger && !strcmp(led_cdev->trigger->name,
                            trig->name))
            len += sprintf(buf+len, "[%s] ", trig->name);
        else
            len += sprintf(buf+len, "%s ", trig->name);
    }遍历链表，获取设备使用的trigger的name
    up_read(&led_cdev->trigger_lock);
    up_read(&triggers_list_lock);//释放临界资源
    len += sprintf(len+buf, "\n");
    return len;
}
```

#### led_trigger_store调用触发器工作

```cpp
ssize_t led_trigger_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    char trigger_name[TRIG_NAME_MAX];
                          
    struct led_trigger *trig;
    size_t len;

    trigger_name[sizeof(trigger_name) - 1] = '\0';
    strncpy(trigger_name, buf, sizeof(trigger_name) - 1);
    len = strlen(trigger_name);

    if (len && trigger_name[len - 1] == '\n')
        trigger_name[len - 1] = '\0';

    if (!strcmp(trigger_name, "none")) {
        led_trigger_remove(led_cdev);//设置触发器为空，灯灭
        return count;
    }

    down_read(&triggers_list_lock);
    list_for_each_entry(trig, &trigger_list, next_trig) {
        if (!strcmp(trigger_name, trig->name)) {
            down_write(&led_cdev->trigger_lock);
            led_trigger_set(led_cdev, trig);
            up_write(&led_cdev->trigger_lock);


            up_read(&triggers_list_lock);
            return count;
        }
    }//在触发器链表中找到对应触发器并设置
    up_read(&triggers_list_lock);

    return -EINVAL;
}


void led_trigger_set(struct led_classdev *led_cdev, struct led_trigger *trigger)
{
    unsigned long flags;

    /* Remove any existing trigger */
    if (led_cdev->trigger) {
        write_lock_irqsave(&led_cdev->trigger->leddev_list_lock, flags);
        list_del(&led_cdev->trig_list);
        write_unlock_irqrestore(&led_cdev->trigger->leddev_list_lock,
            flags);
        if (led_cdev->trigger->deactivate)
            led_cdev->trigger->deactivate(led_cdev);
        led_cdev->trigger = NULL;
        led_brightness_set(led_cdev, LED_OFF);
    }
    if (trigger) {
        write_lock_irqsave(&trigger->leddev_list_lock, flags);
        list_add_tail(&led_cdev->trig_list, &trigger->led_cdevs);
        write_unlock_irqrestore(&trigger->leddev_list_lock, flags);
        led_cdev->trigger = trigger;//设置为新的触发器
        if (trigger->activate)
            trigger->activate(led_cdev);
    }   
}
```

获取led的当前亮度值:

`cat /sys/class/leds/<device>/brightness`

设置led的亮度值为n

`echo n > /sys/class/leds/<device>/brightness`

获取led的最大亮度值

`cat /sys/class/leds/<device>/max_brightness`

设置led的最大亮度值为n

`echo n > /sys/class/leds/<device>/max_brightness`

获取led的触发器

`cat /sys/class/leds/<device>/trigger`

设置led的触发器为xxx

`echo xxx > /sys/class/leds/<device>/trigger`

## 触发器

### gpio触发的实现

```cpp
static int __init gpio_trig_init(void)
{
    return led_trigger_register(&gpio_led_trigger);
}

int led_trigger_register(struct led_trigger *trigger)
{
    struct led_classdev *led_cdev;
    struct led_trigger *trig;

    rwlock_init(&trigger->leddev_list_lock);
    INIT_LIST_HEAD(&trigger->led_cdevs);//初始化链表

    down_write(&triggers_list_lock);
    /* Make sure the trigger's name isn't already in use */
    list_for_each_entry(trig, &trigger_list, next_trig) {
        if (!strcmp(trig->name, trigger->name)) {
            up_write(&triggers_list_lock);
            return -EEXIST;
        }
    }//遍历链表，查看是否此触发器已经注册，如果注册，返回EEXIST
    /* Add to the list of led triggers */
    list_add_tail(&trigger->next_trig, &trigger_list);//添加触发器到触发器链表
    up_write(&triggers_list_lock);

    /* Register with any LEDs that have this as a default trigger */
    down_read(&leds_list_lock);
    list_for_each_entry(led_cdev, &leds_list, node) {
        down_write(&led_cdev->trigger_lock);
        if (!led_cdev->trigger && led_cdev->default_trigger &&
                !strcmp(led_cdev->default_trigger, trigger->name))
            led_trigger_set(led_cdev, trigger);
        up_write(&led_cdev->trigger_lock);
    }遍历led设备链表，查看如果设备没有触发器，并设置默认触发器为gpio触发，则设置此设备触发方式为gpio触发
    up_read(&leds_list_lock);

    return 0;   
}  
```

#### gpio触发器

```cpp
static struct led_trigger gpio_led_trigger = {
    .name       = "gpio",
    .activate   = gpio_trig_activate,
    .deactivate = gpio_trig_deactivate,
};

struct gpio_trig_data {
    struct led_classdev *led;
    struct work_struct work;


    unsigned desired_brightness;    /* desired brightness when led is on */
    unsigned inverted;      /* true when gpio is inverted */
    unsigned gpio;          /* gpio that triggers the leds */
};
```

##### gpio_trig_activate 进入活跃状态

```cpp
static void gpio_trig_activate(struct led_classdev *led)
{
    struct gpio_trig_data *gpio_data;
    int ret;

    gpio_data = kzalloc(sizeof(*gpio_data), GFP_KERNEL);
    if (!gpio_data)
        return;

    ret = device_create_file(led->dev, &dev_attr_gpio);
    if (ret)
        goto err_gpio;

    ret = device_create_file(led->dev, &dev_attr_inverted);
    if (ret)
        goto err_inverted;

    ret = device_create_file(led->dev, &dev_attr_desired_brightness);
    if (ret)
        goto err_brightness;
//主要是给device创建了三个设备属性，会在/sys/对应目录下生成调试节点
    gpio_data->led = led;
    led->trigger_data = gpio_data;
    INIT_WORK(&gpio_data->work, gpio_trig_work);//初始化工作队列

    return;

err_brightness:
    device_remove_file(led->dev, &dev_attr_inverted);

err_inverted:
    device_remove_file(led->dev, &dev_attr_gpio);

err_gpio:
    kfree(gpio_data);
}

static void gpio_trig_work(struct work_struct *work)
{
    struct gpio_trig_data *gpio_data = container_of(work,
            struct gpio_trig_data, work);
    int tmp;

    if (!gpio_data->gpio)
        return;

    tmp = gpio_get_value(gpio_data->gpio);
    if (gpio_data->inverted)
        tmp = !tmp;

    if (tmp) {
        if (gpio_data->desired_brightness)
            led_set_brightness(gpio_data->led,
                       gpio_data->desired_brightness);//如果设置了期望的的亮度，就设置
        else
            led_set_brightness(gpio_data->led, LED_FULL);//没有设置期望亮度，就设置为最亮
    } else {
        led_set_brightness(gpio_data->led, LED_OFF);//灭灯
    }
}
```

##### gpio_trig_deactivate进入休眠状态

```cpp
static void gpio_trig_deactivate(struct led_classdev *led)
{
    struct gpio_trig_data *gpio_data = led->trigger_data;


    if (gpio_data) {
        device_remove_file(led->dev, &dev_attr_gpio);
        device_remove_file(led->dev, &dev_attr_inverted);
        device_remove_file(led->dev, &dev_attr_desired_brightness);
        flush_work(&gpio_data->work);
        if (gpio_data->gpio != 0)
            free_irq(gpio_to_irq(gpio_data->gpio), led);
        kfree(gpio_data);
    }
}//删除文件，刷掉工作队列，释放中断
```

### 定时器触发器

```cpp
static int __init timer_trig_init(void)
{
    return led_trigger_register(&timer_led_trigger);
}

static struct led_trigger timer_led_trigger = {
    .name     = "timer",
    .activate = timer_trig_activate,
    .deactivate = timer_trig_deactivate,
};

static void timer_trig_activate(struct led_classdev *led_cdev)
{
    int rc;

    led_cdev->trigger_data = NULL;


    rc = device_create_file(led_cdev->dev, &dev_attr_delay_on);
    if (rc)
        return;
    rc = device_create_file(led_cdev->dev, &dev_attr_delay_off);
    if (rc)
        goto err_out_delayon;
//创建文件

    led_blink_set(led_cdev, &led_cdev->blink_delay_on,
              &led_cdev->blink_delay_off);

    led_cdev->trigger_data = (void *)1;

    return;

err_out_delayon:
    device_remove_file(led_cdev->dev, &dev_attr_delay_on);
}

void led_blink_set(struct led_classdev *led_cdev,
           unsigned long *delay_on,
           unsigned long *delay_off)
{   
    if (led_cdev->blink_set &&
        !led_cdev->blink_set(led_cdev, delay_on, delay_off))
        return;//如果用户自己实现了bilnk函数，直接调用并返回
        
    /* blink with 1 Hz as default if nothing specified */
    if (!*delay_on && !*delay_off)
        *delay_on = *delay_off = 500;
//如果delay_on和off域都为0，则设置为500
    led_set_software_blink(led_cdev, *delay_on, *delay_off);
}      

static void led_set_software_blink(struct led_classdev *led_cdev,
                   unsigned long delay_on,
                   unsigned long delay_off)
{
    int current_brightness;

    current_brightness = led_get_brightness(led_cdev);//获取当前亮度
    if (current_brightness)
        led_cdev->blink_brightness = current_brightness;
    if (!led_cdev->blink_brightness)
        led_cdev->blink_brightness = led_cdev->max_brightness;
//设置亮度，如果为0，则设置为最大亮度
    if (led_get_trigger_data(led_cdev) &&
        delay_on == led_cdev->blink_delay_on &&
        delay_off == led_cdev->blink_delay_off)
        return;
//如果led_cdev的trigger_data为1，并且delay_on和off为对应值，则返回
    led_stop_software_blink(led_cdev);//删除定时器，设置delay_on和off域为0

    led_cdev->blink_delay_on = delay_on;
    led_cdev->blink_delay_off = delay_off;//设置delay_on和off

    /* never on - don't blink */
    if (!delay_on)
        return;/on为0，灯灭直接返回

    /* never off - just set to brightness */
    if (!delay_off) {//off为0，灯亮
        led_set_brightness(led_cdev, led_cdev->blink_brightness);
        return;
    }
//如果on和off都不为0，执行定时器函数
    mod_timer(&led_cdev->blink_timer, jiffies + 1);//给定时器时间，执行定时器函数
}
//定时器函数应该在用户注册led设备时指定
static void led_timer_function(unsigned long data)
{
    struct led_classdev *led_cdev = (void *)data;
    unsigned long brightness;
    unsigned long delay;

    if (!led_cdev->blink_delay_on || !led_cdev->blink_delay_off) {
        led_set_brightness(led_cdev, LED_OFF);
        return;
    }

    brightness = led_get_brightness(led_cdev);
    if (!brightness) {//brightness为0则设置为blink_brightness
        /* Time to switch the LED on. */
        brightness = led_cdev->blink_brightness;
        delay = led_cdev->blink_delay_on;
    } else {//不为0设置为0
        /* Store the current brightness value to be able
         * to restore it when the delay_off period is over.
         */
        led_cdev->blink_brightness = brightness;
        brightness = LED_OFF;
        delay = led_cdev->blink_delay_off;
    }//实现led交替闪烁

    led_set_brightness(led_cdev, brightness);//改变led的亮灭状态

    mod_timer(&led_cdev->blink_timer, jiffies + msecs_to_jiffies(delay));//延时后再次执行定时器函数
}

static inline void led_set_brightness(struct led_classdev *led_cdev,
                    enum led_brightness value)
{
    if (value > led_cdev->max_brightness)
        value = led_cdev->max_brightness;
    led_cdev->brightness = value;
    if (!(led_cdev->flags & LED_SUSPENDED))
        led_cdev->brightness_set(led_cdev, value);//设置亮灭
}  
```

## led子系统设备注册函数

```cpp
int led_classdev_register(struct device *parent, struct led_classdev *led_cdev)
{
    led_cdev->dev = device_create(leds_class, parent, 0, led_cdev,
                      "%s", led_cdev->name);//在led子系统下创建相应的设备
    if (IS_ERR(led_cdev->dev))
        return PTR_ERR(led_cdev->dev);

#ifdef CONFIG_LEDS_TRIGGERS
    init_rwsem(&led_cdev->trigger_lock);
#endif
    /* add to the list of leds */
    down_write(&leds_list_lock);
    list_add_tail(&led_cdev->node, &leds_list);//加入设备到全局链表
    up_write(&leds_list_lock);

    if (!led_cdev->max_brightness)
        led_cdev->max_brightness = LED_FULL;//未定义最大亮度，则定义为最大亮度

    led_update_brightness(led_cdev);//更新亮度为设备指定亮度

    init_timer(&led_cdev->blink_timer);
    led_cdev->blink_timer.function = led_timer_function;
    led_cdev->blink_timer.data = (unsigned long)led_cdev;//初始化定时器和定时器函数

#ifdef CONFIG_LEDS_TRIGGERS
    led_trigger_set_default(led_cdev);//设置指定的触发器为默认触发器
#endif

    printk(KERN_DEBUG "Registered led device: %s\n",
            led_cdev->name);

    return 0;
}
```

