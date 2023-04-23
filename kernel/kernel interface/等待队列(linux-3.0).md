# 等待队列(linux-3.0)

## 阻塞和非阻塞

**阻塞调用：**进程没有获得资源时则挂起进程，进程进入休眠状态，此时不占用cpu，调用函数只有在得到结果后才返回，进程唤醒继续执行。

**非阻塞调用：**进程在对设备执行某操作时，即使没有获得所需的资源，也会返回，或在while循环中反复查询返回，被调用的函数不会阻塞当前进程，会立刻返回。

在Linux内核中，驱动程序往往需要这种能力：最明显的体现在系统调用read和write的实现，若资源不能被获取，而用户又希望以阻塞的方式访问设备（常常会这么做），驱动程序在read实现中会将进程睡眠，直到资源可以被获取，并唤醒进程，最后read返回。整个过程仍然进行了正确的设备访问，用户并没有感知到访问过程。若使用非阻塞式访问，即使设备资源不能被获取，read也会立即返回给用户一个标志。

非阻塞相比阻塞是低效率的，非阻塞情况下，用户想获取设备资源，只能不断的查询获取，当资源不能被获取时不断返回，消耗cpu资源。而非阻塞从资源不可被获取使进程睡眠，到唤醒资源可被获取，在睡眠状态是不会消耗cpu资源的。

阻塞会使进程休眠，因此必须在某处唤醒进程，通常唤醒发生在中断的下半部，也就是中断处理程序中，因为硬件资源的获取往往会伴随中断的产生。

对象是否处于阻塞模式和函数是不是阻塞调用有很强的相关性，但并不是一一对应的。阻塞对象上可以有非阻塞的调用方式，我们可以通过一定的API去轮询状 态，在适当的时候调用阻塞函数，就可以避免阻塞。而对于非阻塞对象，调用的函数也可以进入阻塞调用。函数select()就是这样一个例子。



## 等待队列

在Linux内核驱动程序中，可使用等待队列来实现阻塞进程和唤醒，以队列为基础数据结构，与进程调度机制紧密结合，用于实现内核的异步事件通知机制，也可用于同步对系统资源的访问。(信号量在内核中也依赖等待队列来实现)。

​     Linux内核的等待队列是以双循环链表为基础数据结构，与进程调度机制紧密结合，能够用于实现核心的异步事件通知机制。它有两种数据结构：等待队列头 （wait_queue_head_t）和等待队列项（wait_queue_t）。等待队列头和等待队列项中都包含一个list_head类型的域作为”连接件”。它通过一个双链表和把等待task的头，和等待的进程列表链接起来。下面具体介绍。

### 定义

​     /include/linux/wait.h

```cpp
struct __wait_queue_head {
      spinlock_t lock;                    /* 保护等待队列的原子锁 (自旋锁),在对task_list与操作的过程中，使用该锁实现对等待队列的互斥访问*/
      struct list_head task_list;          /* 等待队列,双向循环链表，存放等待的进程 */
};

/*__wait_queue，该结构是对一个等待任务的抽象。每个等待任务都会抽象成一个wait_queue，并且挂载到wait_queue_head上。该结构定义如下：*/


struct __wait_queue {
unsigned int flags;
#define WQ_FLAG_EXCLUSIVE   0x01
void *private;                       /* 通常指向当前任务控制块 */
wait_queue_func_t func;             /* 任务唤醒操作方法，该方法在内核中提供，通常为auto remove_wake_function */
struct list_head task_list;              /* 挂入wait_queue_head的挂载点 */
};

typedef struct __wait_queue_head wait_queue_head_t;
typedef struct __wait_queue wait_queue_t;
```

### 初始化

```cpp
/*定义并初始化一个名为name的等待队列 ,注意此处是定义一个wait_queue_t类型的变量name，并将其private与设置为tsk*/
DECLARE_WAITQUEUE(name,tsk);

wait_queue_head_t my_queue;  
init_waitqueue_head(&my_queue);  //会将自旋锁初始化为未锁，等待队列初始化为空的双向循环链表。

//宏名用于定义并初始化，相当于"快捷方式"
DECLARE_WAIT_QUEUE_HEAD (my_queue);
```

其中flags域指明该等待的进程是互斥进程还是非互斥进程。其中0是非互斥进程，WQ_FLAG_EXCLUSIVE(0×01)是互斥进程。等待队列 (wait_queue_t)和等待对列头(wait_queue_head_t)的区别是等待队列是等待队列头的成员。也就是说等待队列头的task_list域链接的成员就是等待队列类型的(wait_queue_t)。

![image-20230424000414979](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424000414979.png)

### 添加和删除目标元素

```cpp
//设置等待的进程为非互斥进程，并将其添加进等待队列头(q)的队头中。
void add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
    unsigned long flags;
    wait->flags &= ~WQ_FLAG_EXCLUSIVE;
    spin_lock_irqsave(&q->lock, flags);
    __add_wait_queue(q, wait);
    spin_unlock_irqrestore(&q->lock, flags);
}
EXPORT_SYMBOL(add_wait_queue);


//下面函数也和add_wait_queue()函数功能基本一样，只不过它是将等待的进程(wait)设置为互斥进程。
void add_wait_queue_exclusive(wait_queue_head_t *q, wait_queue_t *wait)
{
    unsigned long flags;
    wait->flags |= WQ_FLAG_EXCLUSIVE;
    spin_lock_irqsave(&q->lock, flags);
    __add_wait_queue_tail(q, wait);
    spin_unlock_irqrestore(&q->lock, flags);
}
```

```cpp
//在等待的资源或事件满足时，进程被唤醒，使用该函数被从等待头中删除。
void remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
     unsigned long flags;
     spin_lock_irqsave(&q->lock, flags);
     __remove_wait_queue(q, wait);
     spin_unlock_irqrestore(&q->lock, flags);
}
EXPORT_SYMBOL(remove_wait_queue);
```

### 等待事件

```cpp
#define wait_event(wq, condition)                  
do {                                   
        if (condition)                         
         break;                         
        __wait_event(wq, condition);                   
} while (0)
wait_event(queue,condition);等待以queue为等待队列头等待队列被唤醒，condition必须满足，否则阻塞  
wait_event_interruptible(queue,condition);可被信号打断  
wait_event_timeout(queue,condition,timeout);阻塞等待的超时时间，时间到了，不论condition是否满足，都要返回  
wait_event_interruptible_timeout(queue,condition,timeout)
```

#### wait_event()宏

在等待会列中睡眠直到condition为真。在等待的期间，进程会被置为TASK_UNINTERRUPTIBLE进入睡眠，直到condition变量变为真。每次进程被唤醒的时候都会检查condition的值.

#### wait_event_interruptible()函数

和wait_event()的区别是调用该宏在等待的过程中当前进程会被设置为TASK_INTERRUPTIBLE状态.在每次被唤醒的时候,首先检查 condition是否为真,如果为真则返回,否则检查如果进程是被信号唤醒,会返回-ERESTARTSYS错误码.如果是condition为真,则 返回0.

#### wait_event_timeout()宏

也与wait_event()类似.不过如果所给的睡眠时间为负数则立即返回.如果在睡眠期间被唤醒,且condition为真则返回剩余的睡眠时间,否则继续睡眠直到到达或超过给定的睡眠时间,然后返回0.

#### wait_event_interruptible_timeout()宏

与wait_event_timeout()类似,不过如果在睡眠期间被信号打断则返回ERESTARTSYS错误码.

#### wait_event_interruptible_exclusive()宏

同样和wait_event_interruptible()一样,不过该睡眠的进程是一个互斥进程.

### 唤醒

```cpp
/*
__wake_up - wake up threads blocked on a waitqueue.
@q: the waitqueue
@mode: which threads
@nr_exclusive: how many wake-one or wake-many threads to wake up
@key: is directly passed to the wakeup function
*/
void __wake_up(wait_queue_head_t *q, unsigned int mode,
                int nr_exclusive, void *key)
{
    unsigned long flags;
    spin_lock_irqsave(&q->lock, flags);
    __wake_up_common(q, mode, nr_exclusive, 0, key);
    spin_unlock_irqrestore(&q->lock, flags);
}
//唤醒等待队列.可唤醒处于TASK_INTERRUPTIBLE和TASK_UNINTERUPTIBLE状态的进程,和wait_event/wait_event_timeout成对使用



#define wake_up_interruptible(x)    __wake_up(x, TASK_INTERRUPTIBLE, 1, NULL)
//和wake_up()唯一的区别是它只能唤醒TASK_INTERRUPTIBLE状态的进程.,与wait_event_interruptible
wait_event_interruptible_timeout
wait_event_interruptible_exclusive成对使用



#define wake_up(x)          __wake_up(x, TASK_NORMAL, 1, NULL)
#define wake_up_all(x)          __wake_up(x, TASK_NORMAL, 0, NULL)
#define wake_up_interruptible_nr(x, nr) __wake_up(x, TASK_INTERRUPTIBLE, nr, NULL)
#define wake_up_interruptible_all(x)    __wake_up(x, TASK_INTERRUPTIBLE, 0, NULL)
//这些也基本都和wake_up/wake_up_interruptible一样
```

**wake_up()与wake_event()或者wait_event_timeout成对使用，wake_up_intteruptible()与wait_event_intteruptible()或者wait_event_intteruptible_timeout()成对使用。**

### 在等待队列上睡眠

```cpp
void __sched sleep_on(wait_queue_head_t *q)
{  
  sleep_on_common(q, TASK_UNINTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
}


static long __sched
sleep_on_common(wait_queue_head_t *q, int state, long timeout)
{
        unsigned long flags;
        wait_queue_t wait;
        init_waitqueue_entry(&wait, current);
        __set_current_state(state);
        spin_lock_irqsave(&q->lock, flags);
        __add_wait_queue(q, &wait);
        spin_unlock(&q->lock);
        timeout = schedule_timeout(timeout);
        spin_lock_irq(&q->lock);
        __remove_wait_queue(q, &wait);
        spin_unlock_irqrestore(&q->lock, flags);
        return timeout;
}
```

该函数的作用是定义一个等待队列(wait)，并将当前进程添加到等待队列中(wait)，然后将当前进程的状态置为 TASK_UNINTERRUPTIBLE，并将等待队列(wait)添加到等待队列头(q)中。之后就被挂起直到资源可以获取，才被从等待队列头(q) 中唤醒，从等待队列头中移出。在被挂起等待资源期间，该进程不能被信号唤醒。

```cpp
long __sched sleep_on_timeout(wait_queue_head_t *q, long timeout)
{
       return sleep_on_common(q, TASK_UNINTERRUPTIBLE, timeout);
}
EXPORT_SYMBOL(sleep_on_timeout);
```

与sleep_on()函数的区别在于调用该函数时，如果在指定的时间内(timeout)没有获得等待的资源就会返回。实际上是调用 schedule_timeout()函数实现的。值得注意的是如果所给的睡眠时间(timeout)小于0，则不会睡眠。该函数返回的是真正的睡眠时间。

```cpp
void __sched interruptible_sleep_on(wait_queue_head_t *q)
{
        sleep_on_common(q, TASK_INTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
}
EXPORT_SYMBOL(interruptible_sleep_on);
```

该函数和sleep_on()函数唯一的区别是将当前进程的状态置为TASK_INTERRUPTINLE，这意味在睡眠如果该进程收到信号则会被唤醒。

```cpp
long __sched
interruptible_sleep_on_timeout(wait_queue_head_t *q, long timeout)
{
    return sleep_on_common(q, TASK_INTERRUPTIBLE, timeout);
}
EXPORT_SYMBOL(interruptible_sleep_on_timeout);
```

以上四个函数都是让进程在等待队列上睡眠，不过是小有诧异而已。在实际用的过程中，根据需要选择合适的函数使用就是了。例如在对软驱数据的读写中，如果设 备没有就绪则调用sleep_on()函数睡眠直到数据可读(可写)，在打开串口的时候，如果串口端口处于关闭状态则调用 interruptible_sleep_on()函数尝试等待其打开。在声卡驱动中，读取声音数据时，如果没有数据可读，就会等待足够常的时间直到可读取。

### 其他函数

```cpp
#define init_waitqueue_head(q)              \
    do {                        \
        static struct lock_class_key __key; \
                            \
        __init_waitqueue_head((q), &__key); \
    } while (0)

void __init_waitqueue_head(wait_queue_head_t *q, struct lock_class_ke
y *key)
{
    spin_lock_init(&q->lock);
    lockdep_set_class(&q->lock, key);
    INIT_LIST_HEAD(&q->task_list);
}

#define DECLARE_WAIT_QUEUE_HEAD(name) \
    wait_queue_head_t name = __WAIT_QUEUE_HEAD_INITIALIZER(name)

#define __WAIT_QUEUE_HEAD_INITIALIZER(name) {               \
    .lock       = __SPIN_LOCK_UNLOCKED(name.lock),      \
    .task_list  = { &(name).task_list, &(name).task_list } }

#define DECLARE_WAITQUEUE(name, tsk)                    \
    wait_queue_t name = __WAITQUEUE_INITIALIZER(name, tsk)

#define __WAITQUEUE_INITIALIZER(name, tsk) {                \
    .private    = tsk,                      \
    .func       = default_wake_function,            \
    .task_list  = { NULL, NULL } }


static inline void __add_wait_queue(wait_queue_head_t *head, wait_que
ue_t *new)
{
    list_add(&new->task_list, &head->task_list);
}

static inline void __remove_wait_queue(wait_queue_head_t *head,
                            wait_queue_t *old)
{
    list_del(&old->task_list);
}

static inline void init_waitqueue_entry(wait_queue_t *q, struct task_
struct *p)
{
    q->flags = 0;
    q->private = p;
    q->func = default_wake_function;
}

static inline void init_waitqueue_func_entry(wait_queue_t *q,
                    wait_queue_func_t func)
{
    q->flags = 0;
    q->private = NULL;
    q->func = func;
}

#define __set_current_state(state_value)            \
    do { current->state = (state_value); } while (0)
```

ref： https://www.cnblogs.com/gdk-0078/p/5172941.html