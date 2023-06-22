# interrupt

## 1. 概念

![image-20230610190604551](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610190604551.png)

### 1.1 异常有哪些

CPU在运行的过程中，也会被各种“异常”打断。这些“异常”有：

- 指令未定义
- 指令、数据访问有问题
- SWI(软中断)
- 快中断
- 中断

### 1.2 中断也是一种异常

中断也属于一种“异常”，导致中断发生的情况有很多，比如：

- 按键
- 定时器
- ADC转换完成
- UART发送完数据、收到数据
- 等等

这些众多的“中断源”，汇集到“中断控制器”，由“中断控制器”选择优先级最高的中断并通知CPU。

Linux 系统中有`硬件中断，也有软件中断`。

对硬件中断的处理有 2 个原则：`不能嵌套，越快越好`。

### 1.3 中断处理流程

arm对异常(中断)处理过程：

1. 初始化

- 设置中断源，让它可以产生中断
- 设置中断控制器(可以屏蔽某个中断，优先级)
- 设置CPU总开关(使能中断)

2. 执行其他程序：正常程序
3. 产生中断：比如按下按键--->中断控制器--->CPU
4. CPU 每执行完一条指令都会检查有无中断/异常产生
5. CPU发现有中断/异常产生，开始处理。



对于不同的异常，跳去不同的地址执行程序。这地址上，只是一条跳转指令，跳去执行某个`函数`(地址)，这个就是异常向量。`3.4.5都是硬件做的。`

这些`函数`做什么事情？

软件做的:

1. 保存现场(各种寄存器)
2. 处理异常(中断):分辨中断源，再调用不同的处理函数
3. 恢复现场

### 1.4 异常向量表

u-boot或是Linux内核，都有类似如下的代码：

```assembly
_start: b	reset
	ldr	pc, _undefined_instruction
	ldr	pc, _software_interrupt
	ldr	pc, _prefetch_abort
	ldr	pc, _data_abort
	ldr	pc, _not_used
	ldr	pc, _irq //发生中断时，CPU跳到这个地址执行该指令 **假设地址为0x18**
	ldr	pc, _fiq

```

这就是异常向量表，每一条指令对应一种异常。

发生`复位`时，CPU就去 执行第1条指令：b reset。

发生`中断`时，CPU就去执行“ldr pc, _irq”这条指令。

这些指令`存放的位置是固定的`，比如对于ARM9芯片中断向量的地址是0x18。

`当发生中断时，CPU就强制跳去执行0x18处的代码`。

在向量表里，一般都是放置一条跳转指令，发生该异常时，CPU就会执行向量表中的跳转指令，去调用更复杂的函数。



当然，向量表的位置并不总是从0地址开始，很多芯片可以设置某个`vector base`寄存器，指定向量表在其他位置，比如设置vector base为0x80000000，指定为DDR的某个地址。但是表中的`各个异常向量的偏移地址，是固定的`：复位向量偏移地址是0，中断是0x18。



## 2. 现场保护

ARM芯片属于精简指令集计算机 (RISC Reduced Instruction Set Computing)，它所用的指令比较简单，有如下特点

- 对内存只有读、写指令
- 对于数据的运算是在 CPU内部实现
- 使用 RISC指令的 CPU复杂度小一点，易于设计



### 2.1 单个程序的运行

![image-20230610190321344](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610190321344.png)

### 2.2 函数切换/中断/进程调度 的现场保存

![image-20230610193140247](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610193140247.png)

这里的程序A->B的切换可以理解为进程A和B的切换，函数A->B的切换，以及A被B中断的切换

#### 2.2.1 函数调用

a) 在函数A 里调用函数B，实际就是中断函数A 的执行。

b) 那么需要把函数 A调用 B之前瞬间的 CPU寄存器的值，保存到栈里；

c) 再去执行函数 B

d) 函数 B返回之后，就从栈中恢复函数 A对应的 CPU寄存器值，继续执行。

#### 2.2.2 中断处理

a) 进程 A正在执行，这时候发生了中断。

b) CPU强制跳到中断异常向量地址去执行，

c) 这时就需要保存进程 A被中断瞬间的 CPU寄存器值，

d) 可以保存在进程 A的内核态栈，也可以保存在进程 A的内核结构体中。

e) 中断处理完毕，要继续运行进程 A之前，恢复这些值。

#### 2.2.3 进程切换

a) 在所谓的多任务操作系统中，我们以为多个程序是同时运行的。

b) 如果我们能感知微秒、纳秒级的事件，可以发现操作系统时让这些程序依次执行一小段时间，进程 A的时间用完了，就切换到进程 B。

c) 怎么切换？

d) 切换过程是发生在内核态里的，跟中断的处理类似。

e) 进程 A的被切换瞬间的 CPU寄存器值保存在某个地方；

f) 恢复进程 B之前保存的 CPU寄存器值，这样就可以运行进程 B了。



所以，在中断处理的过程中，伴存着进程的保存现场、恢复现场。进程的调度也是使用栈来保存、恢复现场：

![image-20230610193948944](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610193948944.png)

#### 2.3 调度的最小单位--线程

在Linux中：资源分配的单位是进程，调度的单位是线程。
也就是说，在一个进程里，可能有多个线程，这些线程共用打开的文件句柄、全局变量等等。
而这些线程，之间是互相独立的，“同时运行”，也就是说：每一个线程，都有自己的栈。如下图示：

![image-20230610194154301](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610194154301.png)

## 3. 硬件中断和软件中断

### 3.1 硬件中断

对于按键中断等硬件产生的中断，称之为“硬件中断”(hard irq)。每个硬件中断都有对应的处理函数，比如按键中断、网卡中断的处理函数肯定不一样。

![image-20230610195347791](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610195347791.png)

### 3.2 软件中断

当发生A 中断时，对应的irq_function_A 函数被调用。硬件导致该函数被调用。
相对的，还可以人为地制造中断：软件中断(soft irq)，如下图所示：

![image-20230610195531893](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610195531893.png)

#### 3.2.1 软中断的种类

![image-20230610195840785](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610195840785.png)

触发：

raise_softirq ，简单地理解就是设置 softirq_veq[nr] 的标记位：

![image-20230610195814446](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610195814446.png)

设置：

![image-20230610195922659](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610195922659.png)

## 4. 中断上半部和下半部

中断处理原则：

1. 不能嵌套

- 中断 A 正在处理的过程中，假设又发生了中断 B ，那么在栈里要保存 A 的现场，然后 处理 B 。
- 在处理 B 的过程中又发生了中断 C ，那么在栈里要保存 B 的现场，然后处理C 。
- 如果中断嵌套突然暴发，那么栈将越来越大，栈终将耗尽。
- 所以，为了防止这种情况发生，也是为了简单化中断的处理，在Linux 系统上中断无法嵌套：即当前中断 A 没处理完之前，不会响应另一个中断 B 即使它的优先级更高 。

2. 越快越好

- 在单芯片系统中，假设中断处理很慢，那应用程序在这段时间内就无法执行：系统显得很迟顿。
- 在SMP新系统中，中断处理时会禁止当前cpu的中断，本cpu无法处理其他中断。
- 在中断的处理过程中，该CPU 是不能进行进程调度的，所以中断的处理要越快越好，尽早让其他中断能被处理──进程调度靠定时器中断来实现。

### 4.1 中断上半部和中断下半部

当一个中断要耗费很多时间来处理时，它的坏处是：在这段时间内，其他中断无法被处理。换句话说，在这段时间内，系统是关中断的。如果某个中断就是要做那么多事，我们能不能把它拆分成两部分：紧急的、不紧急的？在handler 函数里只做紧急的事，然后就重新开中断，让系统得以正常运行；那些不紧急的事，以后再处理，处理时是开中断的。

![image-20230610203725726](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610203725726.png)

![image-20230610204059774](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610204059774.png)

![image-20230610204403863](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610204403863.png)





1. 中断的处理可以分为上半部，下半部
2. 中断上半部，用来处理紧急的事，它是在关中断的状态下执行的
3. 中断下半部，用来处理耗时的、不那么紧急的事，它是在开中断的状态下执行的
4. 中断下半部执行时，有可能会被多次打断，有可能会再次发生同一个中断
5. 中断上半部执行完后，触发中断下半部的处理
6. 中断上半部、下半部的执行过程中，不能休眠：中断休眠的话，以后谁来调度进程啊？

#### 4.1.1 中断下半部的处理

##### tasklet

tasklet 是使用软件中断来实现。

![image-20230610203943080](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610203943080.png)



##### 工作队列

如果中断要做的事情实在太耗时，那就不能用软件中断来做，而应该用内核线程来做：在中断上半部唤醒内核线程。内核线程和 APP 都一样竞争执行，APP 有机会执行，系统不会卡顿。这个内核线程是系统帮我们创建的，一般是kworker线程，内核中有很多这样的线程：

![image-20230610210644295](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610210644295.png)

kworker线程要去“工作队列” work queue) 上取出一个一个“工作” work)来执行它里面的函数：

1. 内核使用`DECLARE_WORK`创建一个work，其中包含要执行的函数
2. `schedule_work`会把work 提供给系统默认的 work queue:system_wq ，它是一个队列。schedule_work 函数不仅仅是把 work 放入队列，还会把kworker 线程唤醒。此线程抢到时间运行时，它就会从队列中取出 work ，执行里面的函数。
3. 在中断上半部调用 schedule_work 函数，触发 work 的处理
4. 既然是在线程中运行，那对应的函数可以休眠。



工作对列实际就是使用线程去异步处理中断下半部，甚至可以自己去创建一个内核线程去执行中断下半部。

除此外，还有很多变种函数用来使用工作队列

##### threaded irq

![image-20230610211945881](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230610211945881.png)

你可以只提供thread_fn ，系统会为这个函数创建一个内核线程。发生中断时，内核线程就会执行这个函数。

以前用work 来线程化地处理中断，一个 worker 线程只能由一个 CPU 执行，多个中断的 work 都由同一个 worker 线程来处理，在单 CPU 系统中也只能忍着了。但是在 SMP 系统中，明明有那么多 CPU 空着，你偏偏让多个中断挤在这个CPU 上？
新技术threaded irq ，为每一个中断都创建一个内核线程；多个中断的内核线程可以分配到多个 CPU 上执行，这提高了效率。

## 5. 中断硬件框架

### 5.1 中断体系

#### 中断源

中断源多种多样，比如GPIO、定时器、UART、DMA等等。
它们都有自己的寄存器，可以进行相关设置：使能中断、中断状态、中断类型等等。

#### 中断控制器

各种中断源发出的中断信号，汇聚到中断控制器。
可以在中断控制器中设置各个中断的优先级。
中断控制器会向CPU发出中断信号，CPU可以读取中断控制器的寄存器，判断当前处理的是哪个中断。
中断控制器有多种实现，比如：

* STM32F103中被称为NVIC：Nested vectored interrupt controller(嵌套向量中断控制器)
* ARM9中一般是芯片厂家自己实现的，没有统一标准
* Cortex A7中使用GIC(Generic Interrupt Controller)

#### CPU

CPU每执行完一条指令，都会判断一下是否有中断发生了。
CPU也有自己的寄存器，可以设置它来使能/禁止中断，这是中断处理的总开关。

![image-20230614230304439](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230614230304439.png)

### 5.2 imx6ull cortex a7 GPIO中断

IMX6ULL的GPIO中断在硬件上的框架，跟STM32MP157是类似的。
IMX6ULL中没有EXTI控制器，对GPIO的中断配置、控制，都在GPIO模块内部实现：

![image-20230614230531443](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230614230531443.png)

#### 5.2.1配置gpio中断

每组GPIO中都有对应的GPIOx_ICR1、GPIOx_ICR2寄存器(interrupt configuration register )。
每个引脚都可以配置为中断引脚，并配置它的触发方式：

![image-20230614230405093](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230614230405093.png)

#### 5.2.2 使能gpio中断

![image-20230614230442434](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230614230442434.png)

#### 5.2.3 判断中断状态、清中断

![image-20230614230506231](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230614230506231.png)

#### 5.2.4 GIC

见6



#### 5.2.5 CPU

CPU的CPSR寄存器中有一位：I位，用来使能/禁止中断。

![image-20230614230646159](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230614230646159.png)

可以使用以下汇编指令修改I位：

CPSIE I  ; 清除I位，使能中断
CPSID I  ; 设置I位，禁止中断

## 6. GIC

ARM体系结构定义了通用中断控制器（GIC），该控制器包括一组用于管理单核或多核系统中的中断的硬件资源。GIC提供了内存映射寄存器，可用于管理中断源和行为，以及（在多核系统中）用于将中断路由到各个CPU核。它使软件能够屏蔽，启用和禁用来自各个中断源的中断，以（在硬件中）对各个中断源进行优先级排序和生成软件触发中断。它还提供对TrustZone安全性扩展的支持。GIC接受系统级别中断的产生，并可以发信号通知给它所连接的每个内核，从而有可能导致IRQ或FIQ异常发生。

### gic构成

从软件角度来看，GIC具有两个主要功能模块，简单画图如下：

![image-20230614234706948](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230614234706948.png)

#### 分发器(Distributor)
​	系统中的所有中断源都连接到该单元。可以通过仲裁单元的寄存器来控制各个中断源的属性，例如优先级、状态、安全性、路由信息和使能状态。
分发器把中断输出到“CPU接口单元”，后者决定将哪个中断转发给CPU核。

#### CPU接口单元（CPU Interface）

​	CPU核通过控制器的CPU接口单元接收中断。CPU接口单元寄存器用于屏蔽，识别和控制转发到CPU核的中断的状态。系统中的每个CPU核心都有一个单独的CPU接口。
​	中断在软件中由一个称为中断ID的数字标识。中断ID唯一对应于一个中断源。软件可以使用中断ID来识别中断源并调用相应的处理程序来处理中断。呈现给软件的中断ID由系统设计确定，一般在SOC的数据手册有记录。

### 中断的类型

#### SGI

软件触发中断（SGI，Software Generated Interrupt）

这是由软件通过写入专用仲裁单元的寄存器即软件触发中断寄存器（ICDSGIR）显式生成的。它最常用于CPU核间通信。SGI既可以发给所有的核，也可以发送给系统中选定的一组核心。中断号0-15保留用于SGI的中断号。用于通信的确切中断号由软件决定。 

#### PPI

这是由单个CPU核私有的外设生成的。PPI的中断号为16-31。它们标识CPU核私有的中断源，并且独立于另一个内核上的相同中断源，比如，每个核的计时器。

#### SPI

这是由外设生成的，中断控制器可以将其路由到多个核。中断号为32-1020。SPI用于从整个系统可访问的各种外围设备发出中断信号。

中断可以是边沿触发的（在中断控制器检测到相关输入的上升沿时认为中断触发，并且一直保持到清除为止）或电平触发（仅在中断控制器的相关输入为高时触发）。

### 中断的状态

#### 非活动状态（Inactive）

这意味着该中断未触发。

#### 挂起（Pending）

这意味着中断源已被触发，但正在等待CPU核处理。待处理的中断要通过转发到CPU接口单元，然后再由CPU接口单元转发到内核。

#### 活动（Active）

描述了一个已被内核接收并正在处理的中断。

#### 活动和挂起（Active and pending）

描述了一种情况，其中CPU核正在为中断服务，而GIC又收到来自同一源的中断。



​	中断的优先级和可接收中断的核都在分发器(distributor)中配置。外设发给分发器的中断将标记为pending状态（或Active and Pending状态，如触发时果状态是active）。distributor确定可以传递给CPU核的优先级最高的pending中断，并将其转发给内核的CPU interface。通过CPU interface，该中断又向CPU核发出信号，此时CPU核将触发FIQ或IRQ异常。

​	作为响应，CPU核执行异常处理程序。异常处理程序必须从CPU interface寄存器查询中断ID，并开始为中断源提供服务。完成后，处理程序必须写入CPU interface寄存器以报告处理结束。然后CPU interface准备转发distributor发给它的下一个中断。

​	在处理中断时，中断的状态开始为pending，active，结束时变成inactive。中断状态保存在distributor寄存器中。

![image-20230614235644500](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230614235644500.png)

### gic使能

#### 配置

GIC作为内存映射的外围设备，被软件访问。所有内核都可以访问公共的distributor单元，但是CPU interface是备份的，也就是说，`每个CPU核都使用相同的地址来访问其专用CPU接口。一个CPU核不可能访问另一个CPU核的CPU接口。`

Distributor拥有许多寄存器，可以通过它们配置各个中断的属性。这些可配置属性是：

*  `中断优先级`：Distributor使用它来确定接下来将哪个中断转发到CPU接口。
*  中断配置：这确定中断是对`电平触发还是边沿触发`。
*  中断目标：这确定了`可以将中断发给哪些CPU核`。
*  `中断启用或禁用状态`：只有Distributor中启用的那些中断变为挂起状态时，才有资格转发。
*  中断安全性：确定将中断分配给Secure还是Normal world软件。
*  中断状态。

Distributor还提供`优先级屏蔽`，可防止低于某个优先级的中断发送给CPU核。每个CPU核上的CPU interface，专注于控制和处理发送给该CPU核的中断。

#### 初始化

`Distributor和CPU interface在复位时均被禁用。复位后，必须初始化GIC，才能将中断传递给CPU核。`

1. 在Distributor中，软件必须配置优先级、目标核、安全性并启用单个中断；随后必须通过其控制寄存器使能。
2. 对于每个CPU interface，软件必须对优先级和抢占设置进行编程。每个CPU接口模块本身必须通过其控制寄存器使能。
3. 在CPU核可以处理中断之前，软件会通过在向量表中设置有效的中断向量并清除CPSR中的中断屏蔽位来让CPU核可以接收中断。

可以通过禁用Distributor单元来禁用系统中的整个中断机制；可以通过禁用单个CPU的CPU接口模块或者在CPSR中设置屏蔽位来禁止向单个CPU核的中断传递。也可以在Distributor中禁用（或启用）单个中断。

为了使某个中断可以触发CPU核，必须将各个中断，Distributor和CPU interface全部使能，并将CPSR中断屏蔽位清零，如下图：

![image-20230615000239075](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230615000239075.png)

#### GIC中断处理与状态变化

当CPU核接收到中断时，它会跳转到中断向量表执行。(硬件决定的)

1. 顶层中断处理程序读取CPU接口模块的`Interrupt Acknowledge Register`，以`获取中断ID`。除了返回中断ID之外，读取操作还会使该中断在Distributor中标记为`active`状态。一旦知道了中断ID（标识中断源），顶层处理程序现在就可以分派特定于设备的处理程序来处理中断。
2. 当特定于设备的处理程序完成执行时，顶级处理程序将相同的中断ID写入CPU interface模块中的`End of Interrupt register`中断结束寄存器，指示中断处理结束。除了把当前中断移除active状态之外，这将使最终中断状态变为`inactive或pending`（如果状态为active and pending），这将使CPU interface能够将更多待处理pending的中断转发给CPU核。这样就结束了单个中断的处理。
3. 同一CPU核上可能有多个中断等待服务，但是CPU interface一次只能发出一个中断信号。顶层中断处理程序重复上述顺序，直到读取特殊的中断ID值1023，表明该内核不再有任何待处理的中断。这个特殊的中断ID被称为伪中断ID（spurious interrupt ID）。伪中断ID是保留值，不能分配给系统中的任何设备。



#### gic初始化code

```cpp
GIC_Type * get_gic_base(void)
{
	GIC_Type *dst;

	__asm volatile ("mrc p15, 4, %0, c15, c0, 0" : "=r" (dst)); //mcr指令获取gic的基地址

	return dst;
}

void gic_init(void)
{
	u32 i, irq_num;

	GIC_Type *gic = get_gic_base();

	/* the maximum number of interrupt IDs that the GIC supports */
	/* 读出GIC支持的最大的中断号 */
	/* 注意: 中断个数 = irq_num * 32 */
	irq_num = (gic->D_TYPER & 0x1F) + 1;

	/* Disable all PPI, SGI and SPI */
	/* 禁止所有的PPI、SIG、SPI中断 */
	for (i = 0; i < irq_num; i++)
	  gic->D_ICENABLER[i] = 0xFFFFFFFFUL;

	/* all set to group0 */
	/* 这些中断, 都发给group0 */
	for (i = 0; i < irq_num; i++)
	  gic->D_IGROUPR[i] = 0x0UL;

	/* all spi interrupt target for cpu interface 0 */
	/* 所有的SPI中断都发给cpu interface 0 */
	for (i = 32; i < (irq_num << 5); i++)
	  gic->D_ITARGETSR[i] = 0x01UL;

	/* all spi is level sensitive: 0-level, 1-edge */
	/* it seems level and edge all can work */
	/* 设置GIC内部的中断触发类型 */
	for (i = 2; i < irq_num << 1; i++)
	  gic->D_ICFGR[i] = 0x01010101UL;

	/* The priority mask level for the CPU interface. If the priority of an 
	 * interrupt is higher than the value indicated by this field, 
	 * the interface signals the interrupt to the processor.
	 */
	/* 把所有中断的优先级都设为最高 */
	gic->C_PMR = (0xFFUL << (8 - 5)) & 0xFFUL;
	
	/* No subpriority, all priority level allows preemption */
	/* 没有"次级优先级" */
	gic->C_BPR = 7 - 5;
	
	/* Enables the forwarding of pending interrupts from the Distributor to the CPU interfaces.
	 * Enable group0 distribution
	 */
	/* 使能:   Distributor可以给CPU interfac分发中断 */
	gic->D_CTLR = 1UL;
	
	/* Enables the signaling of interrupts by the CPU interface to the connected processor
	 * Enable group0 signaling 
	 */
	/* 使能:	 CPU interface可以给processor分发中断 */
	gic->C_CTLR = 1UL;
}

void gic_enable_irq(uint32_t nr)//使能nr号硬件中断路径Distributor->CPU interfaces
{
	GIC_Type *gic = get_gic_base();

	/* The GICD_ISENABLERs provide a Set-enable bit for each interrupt supported by the GIC.
	 * Writing 1 to a Set-enable bit enables forwarding of the corresponding interrupt from the
	 * Distributor to the CPU interfaces. Reading a bit identifies whether the interrupt is enabled.
	 */
	gic->D_ISENABLER[nr >> 5] = (uint32_t)(1UL << (nr & 0x1FUL));

}

void gic_disable_irq(uint32_t nr)//禁止nr号硬件中断路径Distributor->CPU interfaces
{
	GIC_Type *gic = get_gic_base();

	/* The GICD_ICENABLERs provide a Clear-enable bit for each interrupt supported by the
	 * GIC. Writing 1 to a Clear-enable bit disables forwarding of the corresponding interrupt from
     * the Distributor to the CPU interfaces. Reading a bit identifies whether the interrupt is enabled. 
	 */
	gic->D_ICENABLER[nr >> 5] = (uint32_t)(1UL << (nr & 0x1FUL));
}


int get_gic_irq(void)//获取触发的中断的硬件中断号
{
	int nr;

	GIC_Type *gic = get_gic_base();
	/* The processor reads GICC_IAR to obtain the interrupt ID of the
	 * signaled interrupt. This read acts as an acknowledge for the interrupt
	 */
	nr = gic->C_IAR;

	return nr;
}

int clear_gic_irq(int nr)//清除触发的中断
{

	GIC_Type *gic = get_gic_base();

	/* write GICC_EOIR inform the CPU interface that it has completed 
	 * the processing of the specified interrupt 
	 */
	gic->C_EOIR = nr;
```

## 7. CPU对中断的处理流程

### 中断栈

arm处理器有多钟处理模式，如usr mode（用户模式），supervisor mode（svc mode，内核态代码大部分处于此状态），IRQ mode(发生中断会进入此mode)。实际发生中断时，`栈的切换顺序是usr mode->irq mode->svc mode`。user mode栈就是应用层自动变量使用的栈，`svc mode使用的栈`是Linux内核创建进程时会为进程分配2个页(配置相关)的空间作为内核栈，`底部是struct thread_info结构`， `顶部就是该进程使用的内核栈`。当进程切换的时候，整个硬件和软件的上下文都会进行切换，这里就包括了svc mode的sp寄存器的值被切换到调度算法选定的新的进程的内核栈上来。

除了irq mode，linux kernel在处理abt mode（当发生data abort exception或者prefetch abort exception的时候进入的模式）和und mode（处理器遇到一个未定义的指令的时候进入的异常模式）的时候也是采用了相同的策略。也就是经过一个简短的abt或者und mode之后，stack切换到svc mode的栈上，这个栈就是发生异常那个时间点current thread的内核栈。anyway，`在irq mode和svc mode之间总是需要一个stack保存数据，这就是中断模式的stack`，系统初始化的时候，cpu_init函数中会进行中断模式stack的设定：

```cpp
struct stack {
	u32 irq[3];
	u32 abt[3];
	u32 und[3];
	u32 fiq[3];
} ____cacheline_aligned;

static struct stack stacks[NR_CPUS];

/*start_kernel
        ---------->setup_arch
               ------------>setup_processor
                      --------------->cpu_init*/
    
/*
 * cpu_init - initialise one CPU.
 *
 * cpu_init sets up the per-CPU stacks.
 */
void notrace cpu_init(void)
{
#ifndef CONFIG_CPU_V7M
	unsigned int cpu = smp_processor_id(); //获取CPU ID
	struct stack *stk = &stacks[cpu]; //获取该CPU对于的irq abt...的stack指针

	if (cpu >= NR_CPUS) {
		pr_crit("CPU%u: bad primary CPU number\n", cpu);
		BUG();
	}

	/*
	 * This only works on resume and secondary cores. For booting on the
	 * boot cpu, smp_prepare_boot_cpu is called after percpu area setup.
	 */
	set_my_cpu_offset(per_cpu_offset(cpu));

	cpu_proc_init();

	/*
	 * Define the placement constraint for the inline asm directive below.
	 * In Thumb-2, msr with an immediate value is not allowed.
	 */
#ifdef CONFIG_THUMB2_KERNEL
#define PLC	"r"  //Thumb-2下，msr指令不允许使用立即数，只能使用寄存器。
#else
#define PLC	"I"
#endif

	/*
	 * setup stacks for re-entrant exception handlers
	 */
	__asm__ (
	"msr	cpsr_c, %1\n\t" //让CPU进入IRQ mode
	"add	r14, %0, %2\n\t" //r14寄存器保存stk->irq
	"mov	sp, r14\n\t"  //设定IRQ mode的stack为stk->irq
	"msr	cpsr_c, %3\n\t"
	"add	r14, %0, %4\n\t"
	"mov	sp, r14\n\t"  //设定abt mode的stack为stk->abt
	"msr	cpsr_c, %5\n\t"
	"add	r14, %0, %6\n\t"
	"mov	sp, r14\n\t" //设定und mode的stack为stk->und 
	"msr	cpsr_c, %7\n\t"
	"add	r14, %0, %8\n\t"
	"mov	sp, r14\n\t" //设定fiq mode的stack为stk->fiq 
	"msr	cpsr_c, %9" //回到SVC mode
	    :  //output部分是空的
	    : "r" (stk), //对应上面代码中的%0
	      PLC (PSR_F_BIT | PSR_I_BIT | IRQ_MODE), //对应上面代码中的%1
	      "I" (offsetof(struct stack, irq[0])), //对应上面代码中的%2
	      PLC (PSR_F_BIT | PSR_I_BIT | ABT_MODE),
	      "I" (offsetof(struct stack, abt[0])),
	      PLC (PSR_F_BIT | PSR_I_BIT | UND_MODE),
	      "I" (offsetof(struct stack, und[0])),
	      PLC (PSR_F_BIT | PSR_I_BIT | FIQ_MODE),
	      "I" (offsetof(struct stack, fiq[0])),
	      PLC (PSR_F_BIT | PSR_I_BIT | SVC_MODE)
	    : "r14"); //上面是input操作数列表，r14是要clobbered register列表
#endif
}
```



### 中断向量表

![image-20230615010449253](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230615010449253.png)

对于ARM处理器而言，当发生异常的时候，处理器会暂停当前指令的执行，保存现场，转而去执行对应的异常向量处的指令，当处理完该异常的时候，恢复现场，回到原来的那点去继续执行程序。系统所有的异常向量（共计8个）组成了异常向量表。向量表（vector table）的代码如下，该代码在arch/arm/kernel/entry-armv.S中 ：

```assembly
.section .vectors, "ax", %progbits
.L__vectors_start:
	W(b)	vector_rst
	W(b)	vector_und
	W(ldr)	pc, .L__vectors_start + 0x1000
	W(b)	vector_pabt
	W(b)	vector_dabt
	W(b)	vector_addrexcptn
	W(b)	vector_irq
	W(b)	vector_fiq
```

### 向量表的搬移

向量表.vector段链接位置如下：

arch/arm/kernel/vmlinux.lds.S

```assembly
	__init_begin = .;

	/*
	 * The vectors and stubs are relocatable code, and the
	 * only thing that matters is their relative offsets
	 */
	__vectors_start = .;
	.vectors 0xffff0000 : AT(__vectors_start) {
		*(.vectors)
	}
	. = __vectors_start + SIZEOF(.vectors);
	__vectors_end = .;

	__stubs_start = .;
	.stubs ADDR(.vectors) + 0x1000 : AT(__stubs_start) {
		*(.stubs)
	}
	. = __stubs_start + SIZEOF(.stubs);
	__stubs_end = .;
```

（1）异常向量表位于0x0的地址。这种设置叫做Normal vectors或者Low vectors。

（2）异常向量表位于0xffff0000的地址。这种设置叫做high vectors



`具体是low vectors还是high vectors是由ARM的CP15协处理器c1寄存器中V位(bit[13])控制的`。`对于启用MMU的ARM Linux而言，系统使用了high vectors`。为什么不用low vector呢？对于linux而言，0～3G的空间是用户空间，如果使用low vector，那么异常向量表在0地址，那么则是用户空间的位置，因此linux选用high vector。当然，使用Low vector也可以，这样Low vector所在的空间则属于kernel space了（也就是说，3G～4G的空间加上Low vector所占的空间属于kernel space），不过这时候要注意一点，因为所有的进程共享kernel space，而用户空间的程序经常会发生空指针访问，这时候，内存保护机制应该可以捕获这种错误（大部分的MMU都可以做到，例如：禁止userspace访问kernel space的地址空间），防止vector table被访问到。对于内核中由于程序错误导致的空指针访问，内存保护机制也需要控制vector table被修改，因此vector table所在的空间被设置成read only的。

`在内核启动过程中的一段汇编中，会将cp15处理器的c1寄存器第十三位设置成1，即使用高地址的异常向量表。`



开启MMU之后，具体异常向量表放在那个物理地址已经不重要了，重要的是把它映射到0xffff0000的虚拟地址就OK了。

```cpp
/*start_kernel
        ---------->setup_arch
              ------------>paging_init
                    -------------->devicemaps_init*/

/*
 * Set up the device mappings.  Since we clear out the page tables for all
 * mappings above VMALLOC_START, except early fixmap, we might remove debug
 * device mappings.  This means earlycon can be used to debug this function
 * Any other function or debugging method which may touch any device _will_
 * crash the kernel.
 */
static void __init devicemaps_init(const struct machine_desc *mdesc)
{
	struct map_desc map;
	unsigned long addr;
	void *vectors;

	/*
	 * Allocate the vector page early.
	 */
	vectors = early_alloc(PAGE_SIZE * 2);//分配2个页存放异常向量表

	early_trap_init(vectors);

	...

	/*
	 * Create a mapping for the machine vectors at the high-vectors
	 * location (0xffff0000).  If we aren't using high-vectors, also
	 * create a mapping at the low-vectors virtual address.
	 */
	map.pfn = __phys_to_pfn(virt_to_phys(vectors));//映射第一个页，虚拟地址为0xffff0000
	map.virtual = 0xffff0000;
	map.length = PAGE_SIZE;
#ifdef CONFIG_KUSER_HELPERS
	map.type = MT_HIGH_VECTORS;
#else
	map.type = MT_LOW_VECTORS;
#endif
	create_mapping(&map);

	if (!vectors_high()) {//如果SCTLR.V的值设定为low vectors，那么还要映射0地址开始的memory 
		map.virtual = 0;
		map.length = PAGE_SIZE * 2;
		map.type = MT_LOW_VECTORS;
		create_mapping(&map);
	}

	/* Now create a kernel read-only mapping */
	map.pfn += 1;//映射第二个页
	map.virtual = 0xffff0000 + PAGE_SIZE;
	map.length = PAGE_SIZE;
	map.type = MT_LOW_VECTORS;
	create_mapping(&map);

	...
}
```

devicemaps_init函数分配两个虚拟页存放异常向量表，最后由create_mapping函数将向量表映射到0xffff0000地址上

```cpp
void __init early_trap_init(void *vectors_base)
{
#ifndef CONFIG_CPU_V7M
	unsigned long vectors = (unsigned long)vectors_base;
	extern char __stubs_start[], __stubs_end[];
	extern char __vectors_start[], __vectors_end[];
	unsigned i;

	vectors_page = vectors_base;

	/*
	 * Poison the vectors page with an undefined instruction.  This
	 * instruction is chosen to be undefined for both ARM and Thumb
	 * ISAs.  The Thumb version is an undefined instruction with a
	 * branch back to the undefined instruction.
	 */
    /*
将整个vector table那个page frame填充成未定义的指令。起始vector table加上kuser helper函数并不能完全的充满这个page，有些缝隙。如果不这么处理，当极端情况下（程序错误或者HW的issue），CPU可能从这些缝隙中取指执行，从而导致不可知的后果。如果将这些缝隙填充未定义指令，那么CPU可以捕获这种异常。 */
	for (i = 0; i < PAGE_SIZE / sizeof(u32); i++)
		((u32 *)vectors_base)[i] = 0xe7fddef1;

	/*
	 * Copy the vectors, stubs and kuser helpers (in entry-armv.S)
	 * into the vector page, mapped at 0xffff0000, and ensure these
	 * are visible to the instruction stream.
	 */
    //拷贝vector table，拷贝stub function
	memcpy((void *)vectors, __vectors_start, __vectors_end - __vectors_start);
	memcpy((void *)vectors + 0x1000, __stubs_start, __stubs_end - __stubs_start);

	kuser_init(vectors_base); //copy kuser helper function

	flush_icache_range(vectors, vectors + PAGE_SIZE * 2);
#else /* ifndef CONFIG_CPU_V7M */
	/*
	 * on V7-M there is no need to copy the vector table to a dedicated
	 * memory area. The address is configurable and so a table in the kernel
	 * image can be used.
	 */
#endif
}
```

early_trap_init将原来放在.vectors段的向量表拷贝到分配的2个页中

### cpu硬件的处理

当一切准备好之后，一旦打开处理器的全局中断就可以处理来自外设的各种中断事件了。

当外设（SOC内部或者外部都可以）检测到了中断事件，就会通过interrupt requestion line上的电平或者边沿（上升沿或者下降沿或者both）通知到该外设连接到的那个中断控制器，而中断控制器就会在多个处理器中选择一个，并把该中断通过IRQ（或者FIQ，本文不讨论FIQ的情况）分发给process。ARM处理器感知到了中断事件后，会进行下面一系列的动作：

#### 修改CPSR

修改CPSR（Current Program Status Register）寄存器中的M[4:0]。M[4:0]表示了ARM处理器当前处于的模式（ processor modes）。ARM定义的mode包括：

| 处理器模式 | 缩写 | 对应的M[4:0]编码 | Privilege level |
| ---------- | ---- | ---------------- | --------------- |
| User       | usr  | 10000            | PL0             |
| FIQ        | fiq  | 10001            | PL1             |
| IRQ        | irq  | 10010            | PL1             |
| Supervisor | svc  | 10011            | PL1             |
| Monitor    | mon  | 10110            | PL1             |
| Abort      | abt  | 10111            | PL1             |
| Hyp        | hyp  | 11010            | PL2             |
| Undefined  | und  | 11011            | PL1             |
| System     | sys  | 11111            | PL1             |

​	一旦设定了CPSR.M，ARM处理器就会将processor mode切换到IRQ mode。

#### 保存中断发生时的CPSR和PC

ARM处理器支持9种processor mode，每种mode看到的ARM core register（R0～R15，共计16个）都是不同的。每种mode都是从一个包括所有的Banked ARM core register中选取。全部Banked ARM core register包括：

| Usr     | System | Hyp      | Supervisor | abort    | undefined | Monitor  | IRQ      | FIQ      |
| ------- | ------ | -------- | ---------- | -------- | --------- | -------- | -------- | -------- |
| R0_usr  |        |          |            |          |           |          |          |          |
| R1_usr  |        |          |            |          |           |          |          |          |
| R2_usr  |        |          |            |          |           |          |          |          |
| R3_usr  |        |          |            |          |           |          |          |          |
| R4_usr  |        |          |            |          |           |          |          |          |
| R5_usr  |        |          |            |          |           |          |          |          |
| R6_usr  |        |          |            |          |           |          |          |          |
| R7_usr  |        |          |            |          |           |          |          |          |
| R8_usr  |        |          |            |          |           |          |          | R8_fiq   |
| R9_usr  |        |          |            |          |           |          |          | R9_fiq   |
| R10_usr |        |          |            |          |           |          |          | R10_fiq  |
| R11_usr |        |          |            |          |           |          |          | R11_fiq  |
| R12_usr |        |          |            |          |           |          |          | R12_fiq  |
| SP_usr  |        | SP_hyp   | SP_svc     | SP_abt   | SP_und    | SP_mon   | SP_irq   | SP_fiq   |
| LR_usr  |        |          | LR_svc     | LR_abt   | LR_und    | LR_mon   | LR_irq   | LR_fiq   |
| PC      |        |          |            |          |           |          |          |          |
| CPSR    |        |          |            |          |           |          |          |          |
|         |        | SPSR_hyp | SPSR_svc   | SPSR_abt | SPSR_und  | SPSR_mon | SPSR_irq | SPSR_fiq |
|         |        | ELR_hyp  |            |          |           |          |          |          |

在IRQ mode下，CPU看到的R0～R12寄存器、PC以及CPSR是和usr mode（userspace）或者svc mode（kernel space）是一样的。不同的是IRQ mode下，有自己的R13(SP，stack pointer）、R14（LR，link register）和SPSR（Saved Program Status Register)

##### cpsr->spsr

CPSR是共用的，虽然中断可能发生在usr mode（用户空间），也可能是svc mode（内核空间），不过这些信息都是体现在CPSR寄存器中。硬件会将发生中断那一刻的CPSR保存在SPSR寄存器中（由于不同的mode下有不同的SPSR寄存器，因此更准确的说应该是SPSR-irq，也就是IRQ mode中的SPSR寄存器）。

##### pc

PC也是共用的，由于后续PC会被修改为irq exception vector，因此有必要保存PC值。当然，与其说保存PC值，不如说是保存返回执行的地址。对于IRQ而言，我们期望返回地址是发生中断那一点执行指令的下一条指令。具体的返回地址保存在lr寄存器中（注意：这个lr寄存器是IRQ mode的lr寄存器，可以表示为lr_irq）：



（1）对于thumb state，lr_irq ＝ PC

（2）对于ARM state，lr_irq ＝ PC － 4

为何要减去4？我的理解是这样的（不一定对）。由于ARM采用流水线结构，当CPU正在执行某一条指令的时候，其实取指的动作早就执行了，这时候PC值＝正在执行的指令地址 ＋ 8，如下所示：

－－－－>  发生中断的指令

              发生中断的指令＋4

－PC－－>  发生中断的指令＋8

           发生中断的指令＋12

一旦发生了中断，当前正在执行的指令当然要执行完毕，但是已经完成取指、译码的指令则终止执行。当发生中断的指令执行完毕之后，原来指向（发生中断的指令＋8）的PC会继续增加4，因此发生中断后，ARM core的硬件着手处理该中断的时候，硬件现场如下图所示：

－－－－> 发生中断的指令

          发生中断的指令＋4 <-------中断返回的指令是这条指令
    
         发生中断的指令＋8

－PC－－> 发生中断的指令＋12

这时候的PC值其实是比发生中断时候的指令超前12。减去4之后，lr_irq中保存了（发生中断的指令＋8）的地址。为什么HW不帮忙直接减去8呢？这样，后续软件不就不用再减去4了。这里我们不能孤立的看待问题，实际上ARM的异常处理的硬件逻辑不仅仅处理IRQ的exception，还要处理各种exception，很遗憾，不同的exception期望的返回地址不统一，因此，硬件只是帮忙减去4，剩下的交给软件去调整。

#### cpu禁止中断

mask IRQ exception。也就是设定CPSR.I = 1

#### 设置pc为异常向量表中断对应的入口地址

设定PC值为IRQ exception vector。基本上，ARM处理器的硬件就只能帮你帮到这里了，一旦设定PC值，ARM处理器就会跳转到IRQ的exception vector地址了，后续的动作都是软件行为了。

### 软件处理

一旦涉及代码的拷贝，我们就需要关心其编译连接时地址（link-time address)和运行时地址（run-time address）。在kernel完成链接后，`__vectors_start`有了其link-time address，如果link-time address和run-time address一致，那么这段代码运行时毫无压力。但是，目前对于vector table而言，其被copy到其他的地址上（对于High vector，这是地址就是0xffff00000），也就是说，link-time address和run-time address不一样了，如果仍然想要这些代码可以正确运行，那么需要这些代码是位置无关的代码。对于vector table而言，必须要位置无关。B这个branch instruction本身就是位置无关的，它可以跳转到一个当前位置的offset。不过并非所有的vector都是使用了branch instruction，对于软中断，其vector地址上指令是`W(ldr)  pc, __vectors_start + 0x1000 `，这条指令被编译器编译成`ldr   pc, [pc, #4080]`，这种情况下，该指令也是位置无关的，但是有个限制，offset必须在4K的范围内，这也是为何存在stub section的原因了。

#### 跳转到vector_irq

中断发生时，会跳转到`W(b)	vector_irq`

```assembly
/*
 * Interrupt dispatcher
 */
	vector_stub	irq, IRQ_MODE, 4

	.long	__irq_usr			@  0  (USR_26 / USR_32)
	.long	__irq_invalid			@  1  (FIQ_26 / FIQ_32)
	.long	__irq_invalid			@  2  (IRQ_26 / IRQ_32)
	.long	__irq_svc			@  3  (SVC_26 / SVC_32)
	.long	__irq_invalid			@  4
	.long	__irq_invalid			@  5
	.long	__irq_invalid			@  6
	.long	__irq_invalid			@  7
	.long	__irq_invalid			@  8
	.long	__irq_invalid			@  9
	.long	__irq_invalid			@  a
	.long	__irq_invalid			@  b
	.long	__irq_invalid			@  c
	.long	__irq_invalid			@  d
	.long	__irq_invalid			@  e
	.long	__irq_invalid			@  f
```

#### vector_irq的处理

```assembly
.macro	vector_stub, name, mode, correction=0
	.align	5

vector_\name:
	.if \correction
	sub	lr, lr, #\correction  @---------------------1
	.endif

	@
	@ Save r0, lr_<exception> (parent PC) and spsr_<exception>
	@ (parent CPSR)
	@
	stmia	sp, {r0, lr}		@ save r0, lr @-----------2
	mrs	lr, spsr 
	str	lr, [sp, #8]		@ save spsr  保存spsr到irq模式下的栈中

	@
	@ Prepare for SVC32 mode.  IRQs remain disabled.
	@
	mrs	r0, cpsr  @---------------------3
	eor	r0, r0, #(\mode ^ SVC_MODE | PSR_ISETSTATE)  @设置成SVC模式，但未切换
	msr	spsr_cxsf, r0  @保存到spsr_irq中 cpsr->r0,r0->svc, r0->spsr

	@
	@ the branch table must immediately follow this code
	@
	and	lr, lr, #0x0f @lr存储着上一个处理器模式的cpsr值，lr = lr & 0x0f取出用于判断发生中断前是用户态还是核心态的信息，该值用于下面跳转表的索引。
 THUMB(	adr	r0, 1f			)
 THUMB(	ldr	lr, [r0, lr, lsl #2]	)
	mov	r0, sp @将irq模式下的sp保存到r0，作为参数传递给即将调用的__irq_usr或__irq_svc
 ARM(	ldr	lr, [pc, lr, lsl #2]	) @根据mode，给lr赋值，__irq_usr或者__irq_svc 
	movs	pc, lr			@ branch to handler in SVC mode
ENDPROC(vector_\name)
```

1. 我们期望在栈上保存发生中断时候的硬件现场（HW context），这里就包括ARM的core register。当发生IRQ中断的时候，PC的值已经更新为中断后面那一条指令再偏移两条指令，因为中断完成以后要继续执行中断后面那条指令，而arm硬件会自动把PC-4赋值给l2，所以我们得再减4才能得到正确的返回地址，所以在`vector_stub    irq, IRQ_MODE, 4`中传入的是4。
2. 当前是IRQ mode，SP_irq在初始化的时候已经设定（12个字节）。`在irq mode的stack上，依次保存了发生中断那一点的r0值、PC值以及CPSR值`（具体操作是通过spsr进行的，其实硬件已经帮我们保存了CPSR到SPSR中了）。`为何要保存r0值？因为随后的代码要使用r0寄存器，因此我们要把r0放到栈上，只有这样才能完完全全恢复硬件现场。`
3. 可怜的IRQ mode稍纵即逝，这段代码就是准备`将ARM推送到SVC mode`。如何准备？其实就是`修改SPSR的值`，SPSR不是CPSR，不会引起processor mode的切换（毕竟这一步只是准备而已）。



上面如果发生中断的时候，在用户模式，则lr赋值为0，svc模式，值为3，所以用户模式，偏移为0，svc模式，偏移为3，再看PC，PC的值其实是当前的指令+8，所以lr的值在用户态发生中断则是当前指令+8，如果是svc模式，则是+20 ( 8 + 4 * 3)。

偏移+8，刚好是`__irq_usr`的地址，去执行`__irq_usr`的处理，而+20刚好是`__irq_svc`的处理，去执行`__irq_svc`。



#### 中断前处于用户态的中断处理usr_entry

```assembly
__irq_usr:
	usr_entry  @保存中断上下文
	kuser_cmpxchg_check
	irq_handler   @调用中断处理程序
	get_thread_info tsk  @获取当前进程的进程描述符中的成员变量thread_info的地址，并将该地址保存到寄存器tsk（r9）（在entry-header.S中定义）
	mov	why, #0 @r8=0
	b	ret_to_user_from_irq  @中断处理完成，恢复中断上下文并返回中断产生的位置
 UNWIND(.fnend		)
ENDPROC(__irq_usr)
```

保存发生中断时候的现场。所谓保存现场其实就是把发生中断那一刻的硬件上下文（各个寄存器）保存在了SVC mode的stack上。

```assembly
.macro	usr_entry, trace=1, uaccess=1
 UNWIND(.fnstart	)
 UNWIND(.cantunwind	)	@ don't unwind the user space
	sub	sp, sp, #PT_REGS_SIZE     @-------------1
 ARM(	stmib	sp, {r1 - r12}	) @-------------2
 THUMB(	stmia	sp, {r0 - r12}	)

 ATRAP(	mrc	p15, 0, r7, c1, c0, 0)
 ATRAP(	ldr	r8, .LCcralign)

	ldmia	r0, {r3 - r5} @-----------------3
	add	r0, sp, #S_PC		@ here for interlock avoidance @------4
	mov	r6, #-1			@  ""  ""     ""        "" @-----5

	str	r3, [sp]		@ save the "real" r0 copied  @------6
					@ from the exception stack

 ATRAP(	ldr	r8, [r8, #0])

	@
	@ We are now ready to fill in the remaining blanks on the stack:
	@
	@  r4 - lr_<exception>, already fixed up for correct return/restart
	@  r5 - spsr_<exception>
	@  r6 - orig_r0 (see pt_regs definition in ptrace.h)
	@
	@ Also, separately save sp_usr and lr_usr
	@
	stmia	r0, {r4 - r6} @--------------7
 ARM(	stmdb	r0, {sp, lr}^			) @----------8
 THUMB(	store_user_sp_lr r0, r1, S_SP - S_PC	)

	.if \uaccess
	uaccess_disable ip
	.endif

	@ Enable the alignment trap while in kernel mode
 ATRAP(	teq	r8, r7)
 ATRAP( mcrne	p15, 0, r8, c1, c0, 0)

	@
	@ Clear FP to mark the first stack frame
	@
	zero_fp

	.if	\trace
#ifdef CONFIG_TRACE_IRQFLAGS
	bl	trace_hardirqs_off
#endif
	ct_user_exit save = 0
	.endif
	.endm
```

1. `sub	sp, sp, #PT_REGS_SIZE`

   移动预留空间，这里arm已经处于svc mode了，一旦进入SVC mode，ARM处理器看到的寄存器已经发生变化，这里的sp已经变成了sp_svc了。因此，后续的压栈操作都是压入了发生中断那一刻的进程的（或者内核线程）内核栈（svc mode栈）。具体保存了S_FRAME_SIZE个寄存器值。包括r0-r15 cpsr。

2. `stmib	sp, {r1 - r12}`

   压栈首先`压入了r1～r12`，原始的`r0被保存的irq mode的stack`上了。

   stmib中的ib表示increment before，因此，在压入R1的时候，stack pointer会先增加4，重要是预留r0的位置。stmib  sp, {r1 - r12}指令中的sp没有“！”的修饰符，表示压栈完成后并不会真正更新stack pointer，因此sp保持原来的值。

3. `ldmia	r0, {r3 - r5}`

   这里r0指向了irq stack，因此，r3是中断时候的r0值，r4是中断现场的PC值，r5是中断现场的CPSR值。

4. `add	r0, sp, #S_PC`

   把r0赋值为S_PC的值

5. `mov	r6, #-1`

   r6为orig_r0的值

6. `str	r3, [sp]`

   保存中断那一刻的r0到sp

7. `stmia r0, {r4 - r6}`

   在内核栈上保存剩余的寄存器的值，根据代码，依次是PC，CPSR和orig r0。实际上这段操作就是从irq stack就中断现场搬移到内核栈上。

8. `stmdb	r0, {sp, lr}^`

   内核栈上还有两个寄存器没有保持，分别是发生中断时候sp和lr这两个寄存器。这时候，r0指向了保存PC寄存器那个地址（add  r0, sp, #S_PC），stmdb  r0, {sp, lr}^中的“db”是decrement before，因此，将sp和lr压入stack中的剩余的两个位置。需要注意的是，我们保存的是发生中断那一刻（对于本节，这是当时user mode的sp和lr），指令中的“^”符号表示访问user mode的寄存器。



usr_entry宏填充pt_regs结构体的过程如图所示，先将r1～r12保存到ARM_r1～ARM_ip（绿色部分），然后将产生中断时的r0寄存器内容保存到ARM_r0（蓝色部分），接下来将产生中断时的下一条指令地址lr_irq、spsr_irq和r4保存到ARM_pc、ARM_cpsr和ARM_ORIG_r0（红色部分），最后将用户模式下的sp和lr保存到ARM_sp 和ARM_lr 中。

![image-20230615013350140](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230615013350140.png)

上面的这段代码主要是在填充结构体pt_regs ，在include/asm/ptrace.h中定义：

```cpp
struct pt_regs {
    long uregs[18];
};


#define ARM_ORIG_r0    uregs[17]
#define ARM_cpsr    uregs[16]
#define ARM_pc        uregs[15]//add	r0, sp, #S_PC时r0指向此处
#define ARM_lr        uregs[14]
#define ARM_sp        uregs[13] 
#define ARM_ip        uregs[12]
#define ARM_fp        uregs[11]
#define ARM_r10        uregs[10]
#define ARM_r9        uregs[9]
#define ARM_r8        uregs[8]
#define ARM_r7        uregs[7]
#define ARM_r6        uregs[6]
#define ARM_r5        uregs[5]
#define ARM_r4        uregs[4]
#define ARM_r3        uregs[3]
#define ARM_r2        uregs[2]
#define ARM_r1        uregs[1]
#define ARM_r0        uregs[0]
```

#### 中断前处于内核态的中断处理svc_entry

```assembly
__irq_svc:
	svc_entry @保存中断上下文
	irq_handler @调用中断处理程序

#ifdef CONFIG_PREEMPT
	ldr	r8, [tsk, #TI_PREEMPT]		@ get preempt count @ 获取preempt计数器值
	ldr	r0, [tsk, #TI_FLAGS]		@ get flags  @ 获取flags
	teq	r8, #0				@ if preempt count != 0   @ 判断preempt是否等于0
	movne	r0, #0				@ force flags to 0  @ 如果preempt不等于0，r0=0
	tst	r0, #_TIF_NEED_RESCHED @将r0与#_TIF_NEED_RESCHED做“与操作”
	blne	svc_preempt @如果不等于0，说明发生内核抢占，需要重新调度。
#endif

	svc_exit r5, irq = 1			@ return from exception
UNWIND(.fnend		)
ENDPROC(__irq_svc)
```



```cpp
.macro	svc_entry, stack_hole=0, trace=1, uaccess=1
 UNWIND(.fnstart		)
 UNWIND(.save {r0 - pc}		)
	sub	sp, sp, #(SVC_REGS_SIZE + \stack_hole - 4) @sp指向struct pt_regs中r1的位置 
#ifdef CONFIG_THUMB2_KERNEL
 SPFIX(	str	r0, [sp]	)	@ temporarily saved
 SPFIX(	mov	r0, sp		)
 SPFIX(	tst	r0, #4		)	@ test original stack alignment
 SPFIX(	ldr	r0, [sp]	)	@ restored
#else
 SPFIX(	tst	sp, #4		)
#endif
 SPFIX(	subeq	sp, sp, #4	)
	stmia	sp, {r1 - r12} @寄存器入栈
 
	ldmia	r0, {r3 - r5} @irq stack中的r0,pc,cpsr->r3-r5
	add	r7, sp, #S_SP - 4	@ here for interlock avoidance @r7指向struct pt_regs中r12的位置 
	mov	r6, #-1			@  ""  ""      ""       "" @orig r0设为-1 
	add	r2, sp, #(SVC_REGS_SIZE + \stack_hole - 4) @r2是发现中断那一刻stack的现场
 SPFIX(	addeq	r2, r2, #4	)
	str	r3, [sp, #-4]!		@ save the "real" r0 copied @保存r0，注意有一个！，sp会加上4，这时候sp就指向栈顶的r0位置了
					@ from the exception stack

	mov	r3, lr @保存svc mode的lr到r3 

	@
	@ We are now ready to fill in the remaining blanks on the stack:
	@
	@  r2 - sp_svc
	@  r3 - lr_svc
	@  r4 - lr_<exception>, already fixed up for correct return/restart
	@  r5 - spsr_<exception>
	@  r6 - orig_r0 (see pt_regs definition in ptrace.h)
	@
	stmia	r7, {r2 - r6} @压栈，在栈上形成形成struct pt_regs 

	get_thread_info tsk
	ldr	r0, [tsk, #TI_ADDR_LIMIT]
	mov	r1, #TASK_SIZE
	str	r1, [tsk, #TI_ADDR_LIMIT]
	str	r0, [sp, #SVC_ADDR_LIMIT]

	uaccess_save r0
	.if \uaccess
	uaccess_disable r0
	.endif

	.if \trace
#ifdef CONFIG_TRACE_IRQFLAGS
	bl	trace_hardirqs_off
#endif
	.endif
	.endm
```

svc_entry宏填充pt_regs结构体的过程如图所示，先将r1～r12保存到ARM_r1～ARM_ip（绿色部分），然后将产生中断时的r0寄存器内容保存到ARM_r0（蓝色部分），由于是在svc模式下产生的中断，所以最后将sp_svc、lr_svc、lr_irq、spsr_irq和r4保存到ARM_sp、ARM_lr、ARM_pc、ARM_cpsr和ARM_ORIG_r0（红色部分）。

![image-20230615013651543](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230615013651543.png)

上述的中断上下文保存过程共涉及了3种栈指针，分别是：用户空间栈指针sp_usr，内核空间栈指针sp_svc和irq模式下的栈栈指针sp_irq。sp_usr指向在setup_arg_pages函数中创建的用户空间栈。sp_svc指向在alloc_thread_info函数中创建的内核空间栈。sp_irq在cpu_init函数中被赋值，指向全局变量stacks.irq[0]。

#### 执行中断处理程序irq_handler

```cpp
.macro	irq_handler
#ifdef CONFIG_MULTI_IRQ_HANDLER
	ldr	r1, =handle_arch_irq @设置中断处理函数地址
	mov	r0, sp @设定传递给machine定义的handle_arch_irq的参数 
	badr	lr, 9997f @设定返回地址 
	ldr	pc, [r1] @执行中断处理函数
#else
	arch_irq_handler_default
#endif
9997:
	.endm
```

irq_handler的处理有两种配置。

一种是配置了CONFIG_MULTI_IRQ_HANDLER。这种情况下，linux kernel允许run time设定irq handler。如果我们需要一个linux kernel image支持多个平台，这是就需要配置这个选项。

另外一种是传统的linux的做法，irq_handler实际上就是arch_irq_handler_default。



对于情况一，machine相关代码需要设定handle_arch_irq函数指针，这里的汇编指令只需要调用这个machine代码提供的irq handler即可（当然，要准备好参数传递和返回地址设定）

情况二要稍微复杂一些（而且，看起来kernel中使用的越来越少），代码如下：

```assembly
	.macro	arch_irq_handler_default
	get_irqnr_preamble r6, lr
1:	get_irqnr_and_base r0, r2, r6, lr @获取中断号，存到r0中
	movne	r1, sp @如果中断号不等于0，将r1=sp，即pt_regs结构体首地址
	@
	@ routine called with r0 = irq number, r1 = struct pt_regs *
	@
	badrne	lr, 1b @返回地址设定为符号1，也就是说要不断的解析irq状态寄存器的内容，得到IRQ number，直到所有的irq number处理完毕 
	bne	asm_do_IRQ @进入中断处理

#ifdef CONFIG_SMP
	/*
	 * XXX
	 *
	 * this macro assumes that irqstat (r2) and base (r6) are
	 * preserved from get_irqnr_and_base above
	 */
	ALT_SMP(test_for_ipi r0, r2, r6, lr)
	ALT_UP_B(9997f)
	movne	r1, sp
	badrne	lr, 1b
	bne	do_IPI
#endif
9997:
	.endm
```

get_irqnr_and_base用于判断当前发生的中断号（与CPU紧密相关）

如果获取的中断号不等于0，则将中断号存入r0寄存器作为第一个参数，pt_regs结构体地址存入r1寄存器作为第二个参数，跳转到c语言函数asm_do_IRQ做进一步处理。



这里的代码已经是和machine相关的代码了，我们这里只是简短描述一下。所谓machine相关也就是说和系统中的中断控制器相关了。get_irqnr_preamble是为中断处理做准备，有些平台根本不需要这个步骤，直接定义为空即可。get_irqnr_and_base 有四个参数，分别是：r0保存了本次解析的irq number，r2是irq状态寄存器的值，r6是irq controller的base address，lr是scratch register。

对于ARM平台而言，我们推荐使用第一种方法，因为从逻辑上讲，中断处理就是需要根据当前的硬件中断系统的状态，转换成一个IRQ number，然后调用该IRQ number的处理函数即可。通过get_irqnr_and_base这样的宏定义来获取IRQ是旧的ARM SOC系统使用的方法，它是假设SOC上有一个中断控制器，硬件状态和IRQ number之间的关系非常简单。但是实际上，ARM平台上的硬件中断系统已经是越来越复杂了，需要`引入interrupt controller级联，irq domain等等概念`，因此，`使用第一种方法优点更多`。

#### 用户态产生的中断退出

```assembly
ENTRY(ret_to_user_from_irq)
	ldr	r1, [tsk, #TI_FLAGS]   @获取thread_info->flags
	tst	r1, #_TIF_WORK_MASK  @判断是否有待处理的work
	bne	slow_work_pending  @如果有，则进入work_pending进一步处理，主要是完成用户进程抢占相关处理。
no_work_pending: @no_work_pending: @如果没有work待处理，则准备恢复中断现场，返回用户空间。
	asm_trace_hardirqs_on save = 0

	/* perform architecture specific actions before user return */
	arch_ret_to_user r1, lr @调用体系结构相关的代码
	ct_user_enter save = 0

	restore_user_regs fast = 0, offset = 0
ENDPROC(ret_to_user_from_irq)
```

`tst	r1, #_TIF_WORK_MASK`

thread_info中的flags成员中有一些low level的标识，如果这些标识设定了就需要进行一些特别的处理，这里检测的flag主要包括：

`\#define _TIF_WORK_MASK  (_TIF_NEED_RESCHED | _TIF_SIGPENDING | _TIF_NOTIFY_RESUME)`

这三个flag分别表示是否需要调度、是否有信号处理、返回用户空间之前是否需要调用callback函数。只要有一个flag被设定了，程序就进入work_pending这个分支。

restore_user_regs 恢复中断现场寄存器的宏，就是将发生中断时保存在内核空间堆栈上的寄存器还原:

```assembly
.macro	restore_user_regs, fast = 0, offset = 0
	uaccess_enable r1, isb=0
#ifndef CONFIG_THUMB2_KERNEL
	@ ARM mode restore
	mov	r2, sp @r2 = sp
	ldr	r1, [r2, #\offset + S_PSR]	@ get calling cpsr @ 从内核栈中获取发生中断时的cpsr值
	ldr	lr, [r2, #\offset + S_PC]!	@ get pc @ 从内核栈中获取发生中断时的下一条指令地址即pc
	msr	spsr_cxsf, r1			@ save in spsr_svc @ 将r1保存到spsr_svc
...
	.if	\fast
	ldmdb	r2, {r1 - lr}^			@ get calling r1 - lr @r2即sp,将保存在内核栈上的数据保存到用户态的r0～r14寄存器 
	.else
	ldmdb	r2, {r0 - lr}^			@ get calling r0 - lr @r2即sp,将保存在内核栈上的数据保存到用户态的r0～r14寄存器 
	.endif
	mov	r0, r0				@ ARMv5T and earlier require a nop
						@ after ldm {}^
	add	sp, sp, #\offset + PT_REGS_SIZE @现场已经恢复，移动svc mode的sp到原来的位置 
	movs	pc, lr				@ return & move spsr_svc into cpsr @将发生中断时的下一条指令地址存入pc，从而返回中断点继续执行，并且将发生中断时的cpsr内容恢复到cpsr寄存器中（开启中断）。
...
	.endm
```

#### 内核态产生的中断退出

```assembly
.macro	svc_exit, rpsr, irq = 0
...
	bl	trace_hardirqs_on
...

	ldr	r1, [sp, #SVC_ADDR_LIMIT]
	uaccess_restore
	str	r1, [tsk, #TI_ADDR_LIMIT]

#ifndef CONFIG_THUMB2_KERNEL
	@ ARM mode SVC restore
	msr	spsr_cxsf, \rpsr @ 将\rpsr保存到spsr_svc,\rpsr为r5即cpsr保存的位置
...
	ldmia	sp, {r0 - pc}^			@ load r0 - pc, cpsr @恢复r0-pc 还包括了将spsr copy到cpsr中。 
...
	
	.endm
```



## 内核中断子系统框架

### 分配和初始化irq_desc

start_kernel->early_irq_init

#### 使用Radix tree的中断描述符初始化

对于开启CONFIG_SPARSE_IRQ的情况下

```cpp
int __init early_irq_init(void)
{
	int i, initcnt, node = first_online_node;
	struct irq_desc *desc;

	init_irq_default_affinity();

	/* Let arch update nr_irqs and return the nr of preallocated irqs */
	initcnt = arch_probe_nr_irqs();//体系结构相关的代码来决定预先分配的中断描述符的个数,即便是配置了CONFIG_SPARSE_IRQ选项，在中断描述符初始化的时候，也有机会预先分配一定数量的IRQ
	printk(KERN_INFO "NR_IRQS:%d nr_irqs:%d %d\n", NR_IRQS, nr_irqs, initcnt);

	if (WARN_ON(nr_irqs > IRQ_BITMAP_BITS))
		nr_irqs = IRQ_BITMAP_BITS;

	if (WARN_ON(initcnt > IRQ_BITMAP_BITS))
		initcnt = IRQ_BITMAP_BITS;

	if (initcnt > nr_irqs)//initcnt是需要在初始化的时候预分配的IRQ的个数
		nr_irqs = initcnt;//nr_irqs是当前系统中IRQ number的最大值

	for (i = 0; i < initcnt; i++) {
		desc = alloc_desc(i, node, 0, NULL, NULL);//使用Radix tree的中断描述符初始化
		set_bit(i, allocated_irqs);
		irq_insert_desc(i, desc);//插入radix tree
	}
	return arch_early_irq_init();
}

```

![image-20230618021401038](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230618021401038.png)

#### 静态全局数组的中断描述符初始化

对于没有开启CONFIG_SPARSE_IRQ的情况下

```cpp
struct irq_desc irq_desc[NR_IRQS] __cacheline_aligned_in_smp = {
	[0 ... NR_IRQS-1] = {
		.handle_irq	= handle_bad_irq,
		.depth		= 1,
		.lock		= __RAW_SPIN_LOCK_UNLOCKED(irq_desc->lock),
	}
};
int __init early_irq_init(void)
{
	int count, i, node = first_online_node;
	struct irq_desc *desc;

	init_irq_default_affinity();

	printk(KERN_INFO "NR_IRQS:%d\n", NR_IRQS);

	desc = irq_desc;//静态数组
	count = ARRAY_SIZE(irq_desc);

	for (i = 0; i < count; i++) {
		desc[i].kstat_irqs = alloc_percpu(unsigned int);
		alloc_masks(&desc[i], GFP_KERNEL, node);
		raw_spin_lock_init(&desc[i].lock);
		lockdep_set_class(&desc[i].lock, &irq_desc_lock_class);
		desc_set_defaults(i, &desc[i], node, NULL, NULL);//初始化irq_desc数组
	}
	return arch_early_irq_init();
}
```

![image-20230618021044672](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230618021044672.png)

结构体struct irq_desc:

```cpp
struct irq_desc {
	struct irq_common_data	irq_common_data;
	struct irq_data		irq_data;
	unsigned int __percpu	*kstat_irqs;
	irq_flow_handler_t	handle_irq;
#ifdef CONFIG_IRQ_PREFLOW_FASTEOI
	irq_preflow_handler_t	preflow_handler;
#endif
	struct irqaction	*action;	/* IRQ action list */
	unsigned int		status_use_accessors;
	unsigned int		core_internal_state__do_not_mess_with_it;
	unsigned int		depth;		/* nested irq disables */
	unsigned int		wake_depth;	/* nested wake enables */
	unsigned int		irq_count;	/* For detecting broken IRQs */
	unsigned long		last_unhandled;	/* Aging timer for unhandled count */
	unsigned int		irqs_unhandled;
	atomic_t		threads_handled;
	int			threads_handled_last;
	raw_spinlock_t		lock;
	struct cpumask		*percpu_enabled;
	const struct cpumask	*percpu_affinity;
#ifdef CONFIG_SMP
	const struct cpumask	*affinity_hint;
	struct irq_affinity_notify *affinity_notify;
#ifdef CONFIG_GENERIC_PENDING_IRQ
	cpumask_var_t		pending_mask;
#endif
#endif
	unsigned long		threads_oneshot;
	atomic_t		threads_active;
	wait_queue_head_t       wait_for_threads;
#ifdef CONFIG_PM_SLEEP
	unsigned int		nr_actions;
	unsigned int		no_suspend_depth;
	unsigned int		cond_suspend_depth;
	unsigned int		force_resume_depth;
#endif
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry	*dir;
#endif
#ifdef CONFIG_SPARSE_IRQ
	struct rcu_head		rcu;
	struct kobject		kobj;
#endif
	int			parent_irq;
	struct module		*owner;
	const char		*name;
} ____cacheline_internodealigned_in_smp;
```

### 中断子系统核心初始化

start_kernel->init_IRQ

```cpp
void __init init_IRQ(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_OF) && !machine_desc->init_irq)
		irqchip_init();
	else
		machine_desc->init_irq();//调用板级初始化函数

	if (IS_ENABLED(CONFIG_OF) && IS_ENABLED(CONFIG_CACHE_L2X0) &&
	    (machine_desc->l2c_aux_mask || machine_desc->l2c_aux_val)) {
		if (!outer_cache.write_sec)
			outer_cache.write_sec = machine_desc->l2c_write_sec;
		ret = l2x0_of_init(machine_desc->l2c_aux_val,
				   machine_desc->l2c_aux_mask);
		if (ret && ret != -ENODEV)
			pr_err("L2C: failed to init: %d\n", ret);
	}

	uniphier_cache_init();
}
```

对于imx6ull,这个函数定义在mach-imx6ul.c中：

```cpp
.init_irq	= imx6ul_init_irq,
```



```cpp
static void __init imx6ul_init_irq(void)
{
	imx_gpc_check_dt();//对dt中fsl,imx6q-gpc节点的地址进行虚拟映射，保存在gpc_base中
	imx_init_revision_from_anatop();//dt中fsl,xxx-anatop相关节点的处理
	imx_src_init();//对dt中fsl,imx51-src节点的地址进行虚拟映射，保存在src_base中，并做一些初始化
	irqchip_init();//中断控制器初始化
	imx6_pm_ccm_init("fsl,imx6ul-ccm");//对dt中fsl,imx6ul-ccm节点的地址进行虚拟映射，保存在ccm_base中，并做一些初始化
}
```

##### irqchip_init

irqchip_init展开如下：

```cpp
void __init irqchip_init(void)
{
	of_irq_init(__irqchip_of_table);
	acpi_probe_device_table(irqchip);
}
```

`__irqchip_of_table`是一个全局变量，被定义在arch/arm/kernel/vmlinux.lds的`__irqchip_of_table`段中:

![image-20230614020930079](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230614020930079.png)

内核中使用IRQCHIP_DECLARE宏向此段放入一个struct of_device_id的结构

```cpp
#define IRQCHIP_DECLARE(name, compat, fn) OF_DECLARE_2(irqchip, name, compat, fn)// include/linux/irqchip.h

#if defined(CONFIG_OF) && !defined(MODULE)//3
#define _OF_DECLARE(table, name, compat, fn, fn_type)			\
	static const struct of_device_id __of_table_##name		\
		__used __section(__##table##_of_table)			\
		 = { .compatible = compat,				\
		     .data = (fn == (fn_type)NULL) ? fn : fn  }
#else
#define _OF_DECLARE(table, name, compat, fn, fn_type)			\
	static const struct of_device_id __of_table_##name		\
		__attribute__((unused))					\
		 = { .compatible = compat,				\
		     .data = (fn == (fn_type)NULL) ? fn : fn }
#endif

#define OF_DECLARE_1(table, name, compat, fn) \
		_OF_DECLARE(table, name, compat, fn, of_init_fn_1)
#define OF_DECLARE_1_RET(table, name, compat, fn) \
		_OF_DECLARE(table, name, compat, fn, of_init_fn_1_ret)
#define OF_DECLARE_2(table, name, compat, fn) \
		_OF_DECLARE(table, name, compat, fn, of_init_fn_2)//2
```

内核中使用此宏静态定义了一系列的struct of_device_id结构，这些结构会放入`__irqchip_of_table`段中

```cpp
IRQCHIP_DECLARE(gic_400, "arm,gic-400", gic_of_init);
IRQCHIP_DECLARE(arm11mp_gic, "arm,arm11mp-gic", gic_of_init);
IRQCHIP_DECLARE(arm1176jzf_dc_gic, "arm,arm1176jzf-devchip-gic", gic_of_init);
IRQCHIP_DECLARE(cortex_a15_gic, "arm,cortex-a15-gic", gic_of_init);
IRQCHIP_DECLARE(cortex_a9_gic, "arm,cortex-a9-gic", gic_of_init);
IRQCHIP_DECLARE(cortex_a7_gic, "arm,cortex-a7-gic", gic_of_init);
IRQCHIP_DECLARE(msm_8660_qgic, "qcom,msm-8660-qgic", gic_of_init);
IRQCHIP_DECLARE(msm_qgic2, "qcom,msm-qgic2", gic_of_init);
IRQCHIP_DECLARE(pl390, "arm,pl390", gic_of_init);

```

如`IRQCHIP_DECLARE(cortex_a7_gic, "arm,cortex-a7-gic", gic_of_init);`展开会是
```cpp
static const struct of_device_id __of_table_cortex_a7_gic
{
		.compatible = "arm,cortex-a7-gic",
		.data = gic_of_init,
}
```



##### of_irq_init

```cpp
void __init of_irq_init(const struct of_device_id *matches)
{
	const struct of_device_id *match;
	struct device_node *np, *parent = NULL;
	struct of_intc_desc *desc, *temp_desc;
	struct list_head intc_desc_list, intc_parent_list;

	INIT_LIST_HEAD(&intc_desc_list);
	INIT_LIST_HEAD(&intc_parent_list);

	for_each_matching_node_and_match(np, matches, &match) {//遍历__irqchip_of_table
		if (!of_find_property(np, "interrupt-controller", NULL) ||
				!of_device_is_available(np))//只对节点中有interrupt-controller的项进行处理
			continue;

		/*
		 * Here, we allocate and populate an of_intc_desc with the node
		 * pointer, interrupt-parent device_node etc.
		 */
		desc = kzalloc(sizeof(*desc), GFP_KERNEL);//分配一个of_intc_desc
		if (WARN_ON(!desc)) {
			of_node_put(np);
			goto err;
		}

		desc->irq_init_cb = match->data;//初始化irq_init_cb
		desc->dev = of_node_get(np);//保存设备节点
		desc->interrupt_parent = of_irq_find_parent(np);//此设备的父设备，由设备树体现
		if (desc->interrupt_parent == np)
			desc->interrupt_parent = NULL;//顶级节点没父设备
		list_add_tail(&desc->list, &intc_desc_list);//加入链表
	}

	/*
	 * The root irq controller is the one without an interrupt-parent.
	 * That one goes first, followed by the controllers that reference it,
	 * followed by the ones that reference the 2nd level controllers, etc.
	 */
	while (!list_empty(&intc_desc_list)) {//遍历链表
		/*
		 * Process all controllers with the current 'parent'.
		 * First pass will be looking for NULL as the parent.
		 * The assumption is that NULL parent means a root controller.
		 */
		list_for_each_entry_safe(desc, temp_desc, &intc_desc_list, list) {
			int ret;
			if (desc->interrupt_parent != parent)
				continue;
			list_del(&desc->list);
			of_node_set_flag(desc->dev, OF_POPULATED);

			ret = desc->irq_init_cb(desc->dev,
						desc->interrupt_parent);//对链表中的of_intc_desc成员调用irq_init_cb函数

			/*
			 * This one is now set up; add it to the parent list so
			 * its children can get processed in a subsequent pass.
			 */
			list_add_tail(&desc->list, &intc_parent_list);
		}
	}
		//...
}
```

1. for_each_matching_node_and_match函数会从设备树根节点遍历设备树，找到compatible与__irqchip_of_table数组的.compatible相同且有interrupt-controllers属性的节点
2. 为节点分配一个of_intc_desc结构
3. 设置desc->irq_init_cb，对于arm,cortex-a7-gic来说就是gic_of_init
4. 设置desc->dev为设备树节点
5. 将分配的of_intc_desc加入intc_desc_list
6. 遍历intc_desc_list，调用满足条件的desc->irq_init_cb函数，即gic_of_init
7. 将of_intc_desc加入intc_parent_list

##### gic_of_init

```cpp
int __init
gic_of_init(struct device_node *node, struct device_node *parent)
{
	struct gic_chip_data *gic;
	int irq, ret;

	if (WARN_ON(!node))
		return -ENODEV;

	if (WARN_ON(gic_cnt >= CONFIG_ARM_GIC_MAX_NR))
		return -EINVAL;

	gic = &gic_data[gic_cnt];

	ret = gic_of_setup(gic, node);
	if (ret)
		return ret;

	/*
	 * Disable split EOI/Deactivate if either HYP is not available
	 * or the CPU interface is too small.
	 */
	if (gic_cnt == 0 && !gic_check_eoimode(node, &gic->raw_cpu_base))
		static_key_slow_dec(&supports_deactivate);

	ret = __gic_init_bases(gic, -1, &node->fwnode);
	if (ret) {
		gic_teardown(gic);
		return ret;
	}

	if (!gic_cnt) {
		gic_init_physaddr(node);
		gic_of_setup_kvm_info(node);
	}

	if (parent) {
		irq = irq_of_parse_and_map(node, 0);//对次级中断控制器，还需要为其创建其与主控制器之间的中断
		gic_cascade_irq(gic_cnt, irq);
	}

	if (IS_ENABLED(CONFIG_ARM_GIC_V2M))
		gicv2m_init(&node->fwnode, gic_data[gic_cnt].domain);

	gic_cnt++;
	return 0;
}
```

##### gic_of_setup

```cpp
static int gic_of_setup(struct gic_chip_data *gic, struct device_node *node)
{
	if (!gic || !node)
		return -EINVAL;

	gic->raw_dist_base = of_iomap(node, 0);
	if (WARN(!gic->raw_dist_base, "unable to map gic dist registers\n"))
		goto error;

	gic->raw_cpu_base = of_iomap(node, 1);
	if (WARN(!gic->raw_cpu_base, "unable to map gic cpu registers\n"))
		goto error;

	if (of_property_read_u32(node, "cpu-offset", &gic->percpu_offset))
		gic->percpu_offset = 0;

	return 0;

error:
	gic_teardown(gic);

	return -ENOMEM;
}
```

gic_of_setup函数从设备树获取gic的distributor和cpu interface寄存器基地址，并进行虚拟映射，保存在gic_chip_data中

##### __gic_init_bases

```cpp

static int __init __gic_init_bases(struct gic_chip_data *gic,
				   int irq_start,
				   struct fwnode_handle *handle)
{
	char *name;
	int i, ret;

	if (WARN_ON(!gic || gic->domain))
		return -EINVAL;

	if (gic == &gic_data[0]) {
		/*
		 * Initialize the CPU interface map to all CPUs.
		 * It will be refined as each CPU probes its ID.
		 * This is only necessary for the primary GIC.
		 */
		for (i = 0; i < NR_GIC_CPU_IF; i++)
			gic_cpu_map[i] = 0xff;
#ifdef CONFIG_SMP
		set_smp_cross_call(gic_raise_softirq);
#endif
		cpuhp_setup_state_nocalls(CPUHP_AP_IRQ_GIC_STARTING,
					  "AP_IRQ_GIC_STARTING",
					  gic_starting_cpu, NULL);
		set_handle_irq(gic_handle_irq);//handle_arch_irq = gic_handle_irq;也就是汇编部分最终调用的中断处理函数
		if (static_key_true(&supports_deactivate))
			pr_info("GIC: Using split EOI/Deactivate mode\n");
	}

	if (static_key_true(&supports_deactivate) && gic == &gic_data[0]) {
		name = kasprintf(GFP_KERNEL, "GICv2");
		gic_init_chip(gic, NULL, name, true);//初始化gic->irq_chip
	} else {
		name = kasprintf(GFP_KERNEL, "GIC-%d", (int)(gic-&gic_data[0]));
		gic_init_chip(gic, NULL, name, false);
	}

	ret = gic_init_bases(gic, irq_start, handle);//初始化gic->domain，初始化distributor初始化cpu interface
	if (ret)
		kfree(name);

	return ret;
}

```

##### gic_init_chip

```cpp
static void gic_init_chip(struct gic_chip_data *gic, struct device *dev,
			  const char *name, bool use_eoimode1)
{
	/* Initialize irq_chip */
	gic->chip = gic_chip;//设置gic->chip
	gic->chip.name = name;
	gic->chip.parent_device = dev;

	if (use_eoimode1) {
		gic->chip.irq_mask = gic_eoimode1_mask_irq;
		gic->chip.irq_eoi = gic_eoimode1_eoi_irq;
		gic->chip.irq_set_vcpu_affinity = gic_irq_set_vcpu_affinity;
	}

#ifdef CONFIG_SMP
	if (gic == &gic_data[0])
		gic->chip.irq_set_affinity = gic_set_affinity;
#endif
}

static struct irq_chip gic_chip = {
	.irq_mask		= gic_mask_irq,
	.irq_unmask		= gic_unmask_irq,
	.irq_eoi		= gic_eoi_irq,
	.irq_set_type		= gic_set_type,
	.irq_get_irqchip_state	= gic_irq_get_irqchip_state,
	.irq_set_irqchip_state	= gic_irq_set_irqchip_state,
	.flags			= IRQCHIP_SET_TYPE_MASKED |
				  IRQCHIP_SKIP_SET_WAKE |
				  IRQCHIP_MASK_ON_SUSPEND,
};
```

##### gic_init_bases

```cpp
static int gic_init_bases(struct gic_chip_data *gic, int irq_start,
			  struct fwnode_handle *handle)
{
	irq_hw_number_t hwirq_base;
	int gic_irqs, irq_base, ret;

	//...
	/*
	 * Find out how many interrupts are supported.
	 * The GIC only supports up to 1020 interrupt sources.
	 */
	//获取gic支持的最大中断数
	gic_irqs = readl_relaxed(gic_data_dist_base(gic) + GIC_DIST_CTR) & 0x1f;
	gic_irqs = (gic_irqs + 1) * 32;//这里要*32
	if (gic_irqs > 1020)
		gic_irqs = 1020;//最多支持1020个硬件中断
	gic->gic_irqs = gic_irqs;//保存最大支持中断数
	//初始化gic->domain
	if (handle) {		/* DT/ACPI */
		gic->domain = irq_domain_create_linear(handle, gic_irqs,
						       &gic_irq_domain_hierarchy_ops,
						       gic);
	} else {		/* Legacy support */
		//...
	}

	if (WARN_ON(!gic->domain)) {
		ret = -ENODEV;
		goto error;
	}
	//初始化gic的distributor
	gic_dist_init(gic);
	//初始化gic的cpu interface
	ret = gic_cpu_init(gic);
	if (ret)
		goto error;

	ret = gic_pm_init(gic);
	if (ret)
		goto error;

	return 0;

error:
	if (IS_ENABLED(CONFIG_GIC_NON_BANKED) && gic->percpu_offset) {
		free_percpu(gic->dist_base.percpu_base);
		free_percpu(gic->cpu_base.percpu_base);
	}

	return ret;
}
```

##### irq_domain_create_linear

```cpp
static inline struct irq_domain *irq_domain_create_linear(struct fwnode_handle *fwnode,
					 unsigned int size,
					 const struct irq_domain_ops *ops,
					 void *host_data)
{
	return __irq_domain_add(fwnode, size, size, 0, ops, host_data);
}


```

```cpp
struct irq_domain *__irq_domain_add(struct fwnode_handle *fwnode, int size,
				    irq_hw_number_t hwirq_max, int direct_max,
				    const struct irq_domain_ops *ops,
				    void *host_data)
{
	//获取设备树节点，对于imx6ull只有一个gic，这个节点就是gpc
	struct device_node *of_node = to_of_node(fwnode);
	struct irq_domain *domain;
	//分配domain，剩余空间为domain->linear_revmap线性映射表，大小4*size
	domain = kzalloc_node(sizeof(*domain) + (sizeof(unsigned int) * size),
			      GFP_KERNEL, of_node_to_nid(of_node));
	if (WARN_ON(!domain))
		return NULL;

	of_node_get(of_node);

	/* Fill structure */
	INIT_RADIX_TREE(&domain->revmap_tree, GFP_KERNEL);//初始化radix tree
	domain->ops = ops;//domain操作集，很重要
	domain->host_data = host_data;//gic_chip_data
	domain->fwnode = fwnode;//设备树节点
	domain->hwirq_max = hwirq_max;//最大的硬件中断号
	domain->revmap_size = size;//直接映射数组的大小
	domain->revmap_direct_max_irq = direct_max;
	irq_domain_check_hierarchy(domain);//domain->flags |= IRQ_DOMAIN_FLAG_HIERARCHY;

	mutex_lock(&irq_domain_mutex);
	list_add(&domain->link, &irq_domain_list);//将domain加入全局链表
	mutex_unlock(&irq_domain_mutex);

	pr_debug("Added domain %s\n", domain->name);
	return domain;
}
```

domain操作集:

```cpp
static const struct irq_domain_ops gic_irq_domain_hierarchy_ops = {
	.translate = gic_irq_domain_translate,
	.alloc = gic_irq_domain_alloc,
	.free = irq_domain_free_irqs_top,
};
```

##### gic_chip_data

```cpp
struct gic_chip_data {
	struct irq_chip chip;//保存的irq_chip，里面有屏蔽中断，清除中断
	union gic_base dist_base;
	union gic_base cpu_base;
	void __iomem *raw_dist_base;
	void __iomem *raw_cpu_base;
	u32 percpu_offset;
#if defined(CONFIG_CPU_PM) || defined(CONFIG_ARM_GIC_PM)
	u32 saved_spi_enable[DIV_ROUND_UP(1020, 32)];
	u32 saved_spi_active[DIV_ROUND_UP(1020, 32)];
	u32 saved_spi_conf[DIV_ROUND_UP(1020, 16)];
	u32 saved_spi_target[DIV_ROUND_UP(1020, 4)];
	u32 __percpu *saved_ppi_enable;
	u32 __percpu *saved_ppi_active;
	u32 __percpu *saved_ppi_conf;
#endif
	struct irq_domain *domain;
	unsigned int gic_irqs;
#ifdef CONFIG_GIC_NON_BANKED
	void __iomem *(*get_base)(union gic_base *);
#endif
};
```

##### irq_domain

```cpp
struct irq_domain {
	struct list_head link;
	const char *name;
	const struct irq_domain_ops *ops;//
	void *host_data;//
	unsigned int flags;

	/* Optional data */
	struct fwnode_handle *fwnode;//
	enum irq_domain_bus_token bus_token;
	struct irq_domain_chip_generic *gc;
#ifdef	CONFIG_IRQ_DOMAIN_HIERARCHY
	struct irq_domain *parent;
#endif

	/* reverse map data. The linear map gets appended to the irq_domain */
	irq_hw_number_t hwirq_max;//
	unsigned int revmap_direct_max_irq;
	unsigned int revmap_size;
	struct radix_tree_root revmap_tree;//
	unsigned int linear_revmap[];
};
```

##### imx_gpc_init

对imx6ull，除了gic，还会使用IRQCHIP_DECLARE宏定义gpc:

```cpp
IRQCHIP_DECLARE(imx_gpc, "fsl,imx6q-gpc", imx_gpc_init);
```



```cpp

static int __init imx_gpc_init(struct device_node *node,
			       struct device_node *parent)
{
	struct irq_domain *parent_domain, *domain;
	int i;
	u32 val;
	u32 cpu_pupscr_sw2iso, cpu_pupscr_sw;
	u32 cpu_pdnscr_iso2sw, cpu_pdnscr_iso;

	if (!parent) {
		pr_err("%s: no parent, giving up\n", node->full_name);
		return -ENODEV;
	}

	parent_domain = irq_find_host(parent);
	if (!parent_domain) {
		pr_err("%s: unable to obtain parent domain\n", node->full_name);
		return -ENXIO;
	}

	gpc_base = of_iomap(node, 0);
	if (WARN_ON(!gpc_base))
	        return -ENOMEM;

	domain = irq_domain_add_hierarchy(parent_domain, 0, GPC_MAX_IRQS,
					  node, &imx_gpc_domain_ops,
					  NULL);
	if (!domain) {
		iounmap(gpc_base);
		return -ENOMEM;
	}
	//非中断部分
	/* Initially mask all interrupts */
	//...

	/* Read supported wakeup source in M/F domain */
	//...

	/* clear the L2_PGE bit on i.MX6SLL */
	//...
	/*
	 * Clear the OF_POPULATED flag set in of_irq_init so that
	 * later the GPC power domain driver will not be skipped.
	 */
	//...

	/*
	 * If there are CPU isolation timing settings in dts,
	 * update them according to dts, otherwise, keep them
	 * with default value in registers.
	 */
	//...

	/* Read CPU isolation setting for GPC */
	//...
	/* Return if no property found in dtb */
	//...
	/* Update CPU PUPSCR timing if it is defined in dts */
	//...
	/* Update CPU PDNSCR timing if it is defined in dts */
	//...

	return 0;
}

static inline struct irq_domain *irq_domain_add_hierarchy(struct irq_domain *parent,
					    unsigned int flags,
					    unsigned int size,
					    struct device_node *node,
					    const struct irq_domain_ops *ops,
					    void *host_data)
{
	return irq_domain_create_hierarchy(parent, flags, size,
					   of_node_to_fwnode(node),
					   ops, host_data);
}

struct irq_domain *irq_domain_create_hierarchy(struct irq_domain *parent,
					    unsigned int flags,
					    unsigned int size,
					    struct fwnode_handle *fwnode,
					    const struct irq_domain_ops *ops,
					    void *host_data)
{
	struct irq_domain *domain;

	if (size)
		domain = irq_domain_create_linear(fwnode, size, ops, host_data);//
	else
		domain = irq_domain_create_tree(fwnode, ops, host_data);
	if (domain) {
		domain->parent = parent;//
		domain->flags |= flags;
	}

	return domain;
}
```

这个函数：

根据dt找到parent domain也就是gic domain

完成gpc_base的映射，根据dt将gpc寄存器基地址映射到gpc_base

注册一个domain，指定domain的parent是gic的domain，并提供domain操作集:

```cpp
static const struct irq_domain_ops imx_gpc_domain_ops = {
	.translate	= imx_gpc_domain_translate,
	.alloc		= imx_gpc_domain_alloc,
	.free		= irq_domain_free_irqs_common,
};
```



#### 小结

irq_chip_init函数会调用of_irq_init

of_irq_init函数参数是`__irqchip_of_table`，`__irqchip_of_table`是一个struct of_device_id数组，这个数组的成员由IRQCHIP_DECLARE宏添加。imx6ull中有两个模块使用这个宏定义：

```cpp
IRQCHIP_DECLARE(cortex_a7_gic, "arm,cortex-a7-gic", gic_of_init);
IRQCHIP_DECLARE(imx_gpc, "fsl,imx6q-gpc", imx_gpc_init);
```

其对应的设备树节点如下：

```shell
gpc: gpc@020dc000 {
                                compatible = "fsl,imx6ul-gpc", "fsl,imx6q-gpc";
                                reg = <0x020dc000 0x4000>;
                                interrupt-controller;
                                #interrupt-cells = <3>;
                                interrupts = <GIC_SPI 89 IRQ_TYPE_LEVEL_HIGH>;
                                interrupt-parent = <&intc>;
                                fsl,mf-mix-wakeup-irq = <0xfc00000 0x7d00 0x0 0x1400640>;
                        };
                        
intc: interrupt-controller@00a01000 {
                compatible = "arm,cortex-a7-gic";
                #interrupt-cells = <3>;
                interrupt-controller;
                reg = <0x00a01000 0x1000>,
                      <0x00a02000 0x100>;
        };
        
{
interrupt-parent = <&gpc>;
gpio1: gpio@0209c000 {
                                compatible = "fsl,imx6ul-gpio", "fsl,imx35-gpio";
                                reg = <0x0209c000 0x4000>;
                                interrupts = <GIC_SPI 66 IRQ_TYPE_LEVEL_HIGH>,
                                             <GIC_SPI 67 IRQ_TYPE_LEVEL_HIGH>;
                                gpio-controller;
                                #gpio-cells = <2>;
                                interrupt-controller;
                                #interrupt-cells = <2>;
                        };
};
```

对中断来说，gpc使用一个中断连接到gic,gpio控制器使用2个中断连接到gpc。gic是gpc的父中断控制器，而gpc是gpio控制的父中断控制器。

of_irq_init函数遍历`__irqchip_of_table`这个数组，对其中所有的项与设备树进行匹配，如果compatible和of_device_id->compatible相同，并且设备节点有interrupt-controller项，则调用其match->data函数：

1. 对gic，这个函数是gic_of_init（gic_of_init是处理器相关函数cortex a7)
2. 对gpc，这个函数是imx_gpc_init(imx_gpc_init是板级函数 imx6ull)
3. 对gpio控制器，并没有使用IRQCHIP_DECLARE去处理gpio节点，gpio控制器也是一个中断控制器，它用来处理所有gpio的中断，gpio控制器的中断初始化不在这里，而是在gpio的板级文件中



对gic，gic_of_init函数初始化一个gic_chip_data结构，去掉电源管理相关部分:

```cpp
struct gic_chip_data {
	struct irq_chip chip;
	union gic_base dist_base;
	union gic_base cpu_base;
	void __iomem *raw_dist_base;
	void __iomem *raw_cpu_base;
	u32 percpu_offset;
	//pm...
	struct irq_domain *domain;
	unsigned int gic_irqs;
#ifdef CONFIG_GIC_NON_BANKED
	void __iomem *(*get_base)(union gic_base *);
#endif
};
```

raw_dist_base被映射为gic的distributor的虚拟地址基地址

cpu_base被映射为gic的cpu interface的虚拟地址基地址

gic_irqs表示gic支持的最大硬件中断数

domain用来建立硬件中断和虚拟中断的映射，并保存映射关系

chip用来屏蔽和使能以及清楚一个中断

gic_of_init最后还会调用gic_dist_init和gic_cpu_init对gic的distributor和cpu insterface进行初始化

对于次级中断控制器（如果有），还会建立次级中断控制器和主中断控制器之间的hwirq和virq的映射(gic_of_init:irq_of_parse_and_map)

![image-20230618021806885](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230618021806885.png)

![image-20230618022145962](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230618022145962.png)

对gpc:

初始化gpc的domain，指定domain的parent为gic的domain

![image-20230618022247066](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230618022247066.png)

### imx6ull gpio中断控制器处理流程

根据设备树，imx6ull的gpio控制器的interrupt-parent是gpc，而gpc的interrupt-parent是gic。gpc主注册一个中断到gic，gpio控制器注册两个中断到gic:

![image-20230618020242198](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230618020242198.png)

