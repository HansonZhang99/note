# 定时器(linux-3.0)

内核定时器用于在未来某一时刻，执行一个特定的函数，它和中断在内核驱动中被广泛时间，其实现在linux/timer.h和kernel/timer.c中。

被调度的函数肯定是异步执行的，类似于一种“软件中断”，而且是处于非进程的上下文中的，所以调度函数必须遵守如下规则:

1. 没有 current 指针、不允许访问用户空间。因为没有进程上下文，相关代码和被中断的进程没有任何联系。
2. 不能执行休眠（或可能引起休眠的函数）和调度。
3. 任何被访问的数据结构都应该针对并发访问进行保护，以防止竞争条件。

## 定义

```cpp
struct timer_list {
    /*
     * All fields that change during normal runtime grouped to the
     * same cacheline
     */
    struct list_head entry;/*将定时器添加到内核定时器链表中*/
    unsigned long expires;/* 表示期望定时器执行的 jiffies 值，到达该 jiffies 值时，将调用 function 函数*/
    struct tvec_base *base;

    void (*function)(unsigned long);/*定时器超时将会执行此函数*/
    unsigned long data;/*函数传入的参数*/

    int slack;

#ifdef CONFIG_TIMER_STATS
    int start_pid;
    void *start_site;
    char start_comm[16];
#endif
#ifdef CONFIG_LOCKDEP
    struct lockdep_map lockdep_map;
#endif
};
```

## 初始化

使用函数初始化

```cpp
struct timer_list * timer;
init_timer(&timer )只初始化定时器结构体
timer.expires=jiffies+(2*HZ);设置超时时间为2秒
timer.function=&function；超时函数function
timer.data=(unsigned long)0;函数参数
```



宏初始化

```cpp
DEFINE_TIMER(timer_name, function_name,expires_value, data);
```

该宏会定义一个名叫timer_name 内核定时器，并初始化其function, expires,和 data 字段。



一键函数初始化

```cpp
setup_timer(&mytimer, (*function)(unsignedlong), unsigned long data);
mytimer.expires = jiffies + 5*HZ;超时时间5秒
```

可在kernel/timer.c中找到函数定义，linux/timer.h找到宏定义和函数声明     注意定时器函数传入参数只能是一个unsigend long类型值（32位），当然地址也是32位的，所以你懂的。。。

## 添加定时器到内核

在添加定时器到内核前，定时器所有初始化参数都可以修改，添加定时器到内核后，就不能再修改。     

添加定时器到内核，就是添加定时器到全局定时器链表。

```cpp
add_timer(struct timer_list *timer)；
```

定时器添加到内核后，就会到超时时间后执行定时器超时函数。当函数执行完后，函数就不会再执行。

## 重新注册

定时器函数只在超时后执行一次，此后不会再执行，但是可以通过重新给定时器结构 expires域赋值，来使定时器在新的超时时间后再次调用定时器超时函数。

```cpp
mod_timer(struct timer_list *timer, unsigned long expires)；
```

## 注销定时器

也就是将定时器从内核定时器链表移除：

```cpp
del_timer(struct timer_list *timer)；
```

## demo

操作定时器的一般流程

```cpp
static struct timer_list timer;//定义定时器
static void func(unsigned long data)
{
    //do something
}
static void tiemr_init(void)
{
    init_timer(&timer);//初始化定时器
    timer.expires=jiffies+2*HZ//超时时间为加入到链表后两秒
    timer.function=&func;//定时器函数
    timer.data=((unsigend long)0)//定时器函数传如参数
    add_timer(&timer);//注册定时器
}

void main()
{
    timer_init();//调用函数后将在两秒后调用func函数，传如参数0
    //do something??? 大于两秒
    mod_timer(&timer,jiffies+(4*HZ));//此函数被调用时，将在未来4秒后再次调用func函数
    //do something??? 大于4秒
    del_timer(&timer);//注销定时器
    return ;
}
```

## jiffies

jiffies在内核中是一个全局变量，它用来统计系统启动以来系统中产生的总节拍数，这个变量定义在include/linux/jiffies.h中，定义形式如下。

```cpp
unsigned long volatile jiffies;
```

### 时钟

时钟应用于处理器的定时信号，它使得处理器在时钟中运行，依靠信号时钟， 处理器便知道什么时候能够执行它的下一个功能。在Linux系统中，时钟分为硬件时钟（又叫实时时钟）和软件时钟（又叫系统时钟）。在对内核编程中，我们经常用到的是系统时钟，系统时钟的主要任务有如下三点：

1. 保证系统时钟的正确性。
2. 防止进程超额使用CPU。
3. 记录CPU和资源消耗统计时间。

系统时钟的初始值在系统启动时，通过读取硬件时钟获得，然后由Linux内核来维护。在系统运行中，系统时钟的更新是根据系统启动后的时钟滴答数来更新的。

实时时钟的主要作用是提供计时和产生精确的时钟中断。实时时钟是用来持久存放系统时间的设备，即便系统关闭后，它也可以靠主板上的微型电池提供的电力保持系统的计时。

### 节拍率

节拍率其实就是系统定时器产生中断的频率，所谓频率即指中断每秒钟产生多少次，即Hz（赫兹）。**不同的体系结构的系统而言，节拍率不一定相同。**

节拍率（Hz）的值可以在文件include/asm-x86/param.h中看到，定义如下:

```cpp
#define  Hz 1000
```



### 节拍

节拍就是指系统中连续两次时钟中断的间隔时间，该值等于节拍率分之一，即1/Hz。因为系统再启动时已经设置了Hz，所以系统的节拍也可以确定。内核正是利用节拍来计算系统时钟和系统运行时间的。



### jiffies变量

jiffies用来统计系统启动以来系统中产生的总节拍数。该变量在系统启动时被初始化为0，接下来没进行一次时钟中断，jiffies自动加1。因此，知道了总的节拍数，然后再除以Hz，即可知系统的运行时间（jiffies/Hz）。

对于jiffies+Hz的含义，jiffies表示当前的系统时钟中断数，Hz表示一秒后的时钟中断的增加量，假设time=jiffies+Hz，正如上面所说 ，内核正是利用节拍数来计算系统时钟和系统运行时间的，则通过jiffies+Hz即可间接表示一秒钟。



如果系统中某个程序运行一段时间后，需要比较该运行时间是否超过一秒，即可通过比较time和程序运行后的jiffies值来判断是否超过一秒。当然此时，我们需要考虑jiffies变量的回绕问题，不可直接用if（time > jiffies）来比较，linux系统提供了4个宏定义来解决用户空间利用jiffies变量进行时间比较时可能产生的回绕现象，如下所示：

```cpp
#define time_after(unknown,known) ((long)(known)-(long)(unknown)<0)
#define time_before(unknown,known) ((long)(unknown)-(long)(known)<0)
#define time_after_eq(unknown,known) ((long)(unknown)-(long)(unknown)<=0)
#define time_before_eq(unknown,known) ((long)(unknown)-(long)(known)<=0)
```

ref:

https://blog.csdn.net/u012160436/article/details/45530621
https://blog.csdn.net/allen6268198/article/details/7270194
