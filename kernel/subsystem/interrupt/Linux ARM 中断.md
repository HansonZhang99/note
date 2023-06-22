# Linux ARM 中断

## 中断和异常

中断分为同步和异步中断：

同步中断又称为异常，是当指令执行时由CPU控制单元产生。同步是指只有一条指令执行完后CPU才会发出中断，典型的异常有缺页异常和除0异常。

异步中断是指其他硬件设备依照CPU时钟信号随机产生的中断。





## ARM中断(汇编部分)

### ARM寄存器

#### 通用寄存器&&程序计数器

![image-20230423224750848](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423224750848.png)

`R0~R7`：在所有模式下对应的物理寄存器都是相同的，在中断或者异常处理程序中需要对这几个寄存器的数据进行保存；

`R8~R12`：fiq模式下一组物理寄存器，其余模式下一组物理寄存器；

`R13、R14`：用户、系统模式共享一组寄存器，其余每个模式各一组寄存器。

`R13（SP指针）`即栈指针，系统初始化时需对所有模式的SP指针赋值，MCU工作在不同模式下时，栈指针会自动切换；

`R14`：1、调用子程序时用于保存调用返回地址，2、发生异常时用于保存异常返回地址

`R15（程序计数器PC）`：可以用作通用寄存器（未验证，一旦使用后果自负），部分指令在使用R15时有特殊限制（暂不清楚是哪些指令）；

#### 当前程序状态寄存器

![image-20230423224807668](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423224807668.png)

`CPSR（当前程序状态寄存器）`：所有模式下可读写

##### 条件标志位

N：Negative，负标志

Z：Zero，0

C：Carry，进位

V：Overflow，溢出

##### 中断标志位如下

I：1表示禁止IRQ中断响应，0表示允许IRQ中断响应

F：1表示禁止FIQ中断响应，偶表示允许FIQ中断响应

##### ARM/Thumb控制标志位

T：0表示执行32 bits的ARM指令，1表示执行16 bits的Thumb指令

##### 模式控制位M0~M4

![image-20230423224824360](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423224824360.png)

### ARM处理器对异常中断的响应过程

1.保存处理器的当前状态，中断屏蔽位以及各条件标志位。这是通过将当前程序状态寄存器cpsr的内容保存到将要执行的异常中断对应的spsr寄存器中实现的，各异常中断有自己的物理spsr寄存器。

2.设置当前程序状态寄存器cpsr中相应的位。使处理器进入相应的执行模式，并屏蔽中断。

3.将寄存器lr_mode 设置成返回地址。

4.将pc设置成该异常的中断向量入口地址，跳转到相应的异常中断处理程序执行。

#### 响应swi异常中断(系统调用）

用如下伪代码描述：

```shell
R14_srv = address of next instruction after the SWI instruction
SPSP_svc = CPSR
//进入特权模式
CPSR[4:0]=0b10011
//切换到arm状态
CPSR[5]=0
//禁止IRQ异常中断
CPSR[7]=1
if high vectors configured then
	PC=0xFFFF0008
else
	PC=0x00000008
```

#### 响应数据访问中止异常中断

用如下伪代码描述：

```shell
R14_srv = address of the aborted instruction +8
SPSP_abt = CPSR
//进入特权模式
CPSR[4:0]=0b10111
//切换到arm状态
CPSR[5]=0
//禁止IRQ异常中断
CPSR[7]=1
if high vectors configured then
	PC=0xFFFF0010
else
	PC=0x00000010
```

#### 响应IRQ异常中断

用如下伪代码描述：

```shell
R14_irq = address of next instruction to be executed +4
SPSP_abt = CPSR
//进入特权模式
CPSR[4:0]=0b10111
//切换到arm状态
CPSR[5]=0
//禁止IRQ异常中断
CPSR[7]=1
if high vectors configured then
	PC=0xFFFF0010
else
	PC=0x00000010
```

### 从异常中断处理器程序中返回

#### ARM流水线

流水线技术通过多个功能部件并行工作来缩短程序执行时间，提高处理器核的效率和吞吐率，从而成为微处理器设计中最为重要的技术之一。也就是说，通过划分指令执行过程中的不同阶段，并通过并行执行，从而提高指令的执行效率。ARM7处理器采用了三级流水线结构，包括取指（fetch）、译码（decode）、执行（execute）三级。

![image-20230423224835798](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423224835798.png)

如上图所示，在执行add r0, r1, #5指令时，第二条指令正在译码阶段，而第三条指令正在取指阶段。在执行第一条指令时，PC寄存器应指向第三条指令。也即，当处理器为三级流水线结构时，PC寄存器总是指向随后的第三条指令。 

当处理器处于ARM指令集状态时，每条指令4字节，所以PC得值为当前指令地址+8。

此外，在ARM9中，采用了五级流水线结构，是在ARM7的三级流水线结构后面添加了两个新的过程。因此，指令的执行过程和取指过程还是相隔一个译码过程，因而PC还是指向当前指令随后的第三条指令。 



从异常中断处理程序中返回包括下面两个基本操作：

恢复被中断的程序的处理器状态，即将SPSR_mode 寄存器内容复制到当前的状态寄存器CPSR中。

返回到发生异常中断的指令的下一条指令处执行，即将lr_mode 寄存器的内容复制到PC中。



#### SWI 和未定义指令异常中断处理程序的返回

SWI 和未定义指令异常中断是由当前执行的指令自身产生的，当SWI 和未定义指令异常中断产生时，PC的值还没有更新，它指向当前指令后面第二条指令（对于arm指令集来说，它指向当前执行的指令地址加8个字节的位置），这时中断发生，处理器会把PC-4保存到异常模式下的寄存器lr_mode中。这时PC-4 即指向当前指令的下一条指令。所以返回操作可以直接使用如下指令实现：

`MOVS  PC,LR`  s后缀会把SPSR_mode寄存器替换CPSR

#### IRQ 和FIQ异常中断处理程序的返回

通常处理器执行完当前指令以后，查询是否有IRQ和FIQ到来。如果有中断产生，PC值已经更新，它指向当前指令后面第三条指令（ARM指令集是当前指令地址加12个字节的位置），然后处理器会将PC-4 保存到异常模式下的寄存器lr_mode中。这时PC-4即指向当前指令后的第二条指令，因此返回操作可通过下面的指令实现：

`SUBS PC,LR ,#4`

先将LR指令减4，赋值到PC中，同时把SPSR_mode寄存器替换CPSR

#### 数据访问中止异常中断处理程序的返回

当发生数据访问中止异常中断时（比如缺页异常），程序在处理完异常地址以后，需要返回到该有问题的数据访问处，重新访问该数据。因此数据访问中止异常程序应该返回到产生该数据访问中止异常中断的指令处，而不是像前面两种情况下返回到当前指令的下一条指令。

数据范文中止异常中断是由数据访问指令产生的，当数据访问中止异常中断产生时，程序计数器已经更新，它指向当前指令后面的第二条指令（当前指令指的是异常指令的后一条指令），然后处理器会将PC-4保存到异常模式下的lr_mode中，PC-4 即为当前指令后面那条指令。所以要返回那条异常指令重新执行，lr寄存器必须要减去8

`SUBS PC,LR,#8`

## ARM中断处理

### 用户态，内核态，中断态的栈

arm处理器有多钟处理模式，如usr mode（用户模式），supervisor mode（svc mode，内核态代码大部分处于此状态），IRQ mode(发生中断会进入此mode)。实际发生中断时，栈的切换顺序是usr mode->irq mode->svc mode。user mode栈就是应用层自动变量使用的栈，svc mode使用的栈是Linux内核创建进程时会为进程分配2个页(配置相关)的空间作为内核栈，底部是struct thread_info结构， 顶部就是该进程使用的内核栈。当进程切换的时候，整个硬件和软件的上下文都会进行切换，这里就包括了svc mode的sp寄存器的值被切换到调度算法选定的新的进程的内核栈上来。

除了irq mode，linux kernel在处理abt mode（当发生data abort exception或者prefetch abort exception的时候进入的模式）和und mode（处理器遇到一个未定义的指令的时候进入的异常模式）的时候也是采用了相同的策略。也就是经过一个简短的abt或者und mode之后，stack切换到svc mode的栈上，这个栈就是发生异常那个时间点current thread的内核栈。anyway，在irq mode和svc mode之间总是需要一个stack保存数据，这就是中断模式的stack，系统初始化的时候，cpu_init函数中会进行中断模式stack的设定：

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

#### 嵌入式内联汇编

嵌入式内联汇编的语法格式是：asm(code : output operand list : input operand list : clobber list)

在input operand list中，有两种限制符（constraint），"r"或者"I"，"I"表示立即数（Immediate operands），"r"表示用通用寄存器传递参数。clobber list中有一个r14，表示在汇编代码中修改了r14的值，这些信息是编译器需要的内容。






### ARM异常向量表

![image-20230423224853328](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423224853328.png)

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

.vector段链接位置如下：

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

异常向量表可能被安放在两个位置上：

（1）异常向量表位于0x0的地址。这种设置叫做Normal vectors或者Low vectors。

（2）异常向量表位于0xffff0000的地址。这种设置叫做high vectors

具体是low vectors还是high vectors是由ARM的CP15协处理器c1寄存器中V位(bit[13])控制的。对于启用MMU的ARM Linux而言，系统使用了high vectors。为什么不用low vector呢？对于linux而言，0～3G的空间是用户空间，如果使用low vector，那么异常向量表在0地址，那么则是用户空间的位置，因此linux选用high vector。当然，使用Low vector也可以，这样Low vector所在的空间则属于kernel space了（也就是说，3G～4G的空间加上Low vector所占的空间属于kernel space），不过这时候要注意一点，因为所有的进程共享kernel space，而用户空间的程序经常会发生空指针访问，这时候，内存保护机制应该可以捕获这种错误（大部分的MMU都可以做到，例如：禁止userspace访问kernel space的地址空间），防止vector table被访问到。对于内核中由于程序错误导致的空指针访问，内存保护机制也需要控制vector table被修改，因此vector table所在的空间被设置成read only的。

在内核启动过程中的一段汇编中，会将cp15处理器的c1寄存器第十三位设置成1，即使用高地址的异常向量表。



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

### 异常中断硬件处理

当一切准备好之后，一旦打开处理器的全局中断就可以处理来自外设的各种中断事件了。

当外设（SOC内部或者外部都可以）检测到了中断事件，就会通过interrupt requestion line上的电平或者边沿（上升沿或者下降沿或者both）通知到该外设连接到的那个中断控制器，而中断控制器就会在多个处理器中选择一个，并把该中断通过IRQ（或者FIQ，本文不讨论FIQ的情况）分发给process。ARM处理器感知到了中断事件后，会进行下面一系列的动作：

1、修改CPSR（Current Program Status Register）寄存器中的M[4:0]。M[4:0]表示了ARM处理器当前处于的模式（ processor modes）。ARM定义的mode包括：

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

一旦设定了CPSR.M，ARM处理器就会将processor mode切换到IRQ mode。



2、保存发生中断那一点的CPSR值（step 1之前的状态）和PC值

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

在IRQ mode下，CPU看到的R0～R12寄存器、PC以及CPSR是和usr mode（userspace）或者svc mode（kernel space）是一样的。不同的是IRQ mode下，有自己的R13(SP，stack pointer）、R14（LR，link register）和SPSR（Saved Program Status Register）。



CPSR是共用的，虽然中断可能发生在usr mode（用户空间），也可能是svc mode（内核空间），不过这些信息都是体现在CPSR寄存器中。硬件会将发生中断那一刻的CPSR保存在SPSR寄存器中（由于不同的mode下有不同的SPSR寄存器，因此更准确的说应该是SPSR-irq，也就是IRQ mode中的SPSR寄存器）。



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

3、mask IRQ exception。也就是设定CPSR.I = 1

4、设定PC值为IRQ exception vector。基本上，ARM处理器的硬件就只能帮你帮到这里了，一旦设定PC值，ARM处理器就会跳转到IRQ的exception vector地址了，后续的动作都是软件行为了。

### 异常中断软件处理（汇编部分）

一旦涉及代码的拷贝，我们就需要关心其编译连接时地址（link-time address)和运行时地址（run-time address）。在kernel完成链接后，`__vectors_start`有了其link-time address，如果link-time address和run-time address一致，那么这段代码运行时毫无压力。但是，目前对于vector table而言，其被copy到其他的地址上（对于High vector，这是地址就是0xffff00000），也就是说，link-time address和run-time address不一样了，如果仍然想要这些代码可以正确运行，那么需要这些代码是位置无关的代码。对于vector table而言，必须要位置无关。B这个branch instruction本身就是位置无关的，它可以跳转到一个当前位置的offset。不过并非所有的vector都是使用了branch instruction，对于软中断，其vector地址上指令是`W(ldr)  pc, __vectors_start + 0x1000 `，这条指令被编译器编译成`ldr   pc, [pc, #4080]`，这种情况下，该指令也是位置无关的，但是有个限制，offset必须在4K的范围内，这也是为何存在stub section的原因了。

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

1.我们期望在栈上保存发生中断时候的硬件现场（HW context），这里就包括ARM的core register。当发生IRQ中断的时候，PC的值已经更新为中断后面那一条指令再偏移两条指令，因为中断完成以后要继续执行中断后面那条指令，而arm硬件会自动把PC-4赋值给l2，所以我们得再减4才能得到正确的返回地址，所以在`vector_stub    irq, IRQ_MODE, 4`中传入的是4。

2.当前是IRQ mode，SP_irq在初始化的时候已经设定（12个字节）。在irq mode的stack上，依次保存了发生中断那一点的r0值、PC值以及CPSR值（具体操作是通过spsr进行的，其实硬件已经帮我们保存了CPSR到SPSR中了）。为何要保存r0值？因为随后的代码要使用r0寄存器，因此我们要把r0放到栈上，只有这样才能完完全全恢复硬件现场。

3.可怜的IRQ mode稍纵即逝，这段代码就是准备将ARM推送到SVC mode。如何准备？其实就是修改SPSR的值，SPSR不是CPSR，不会引起processor mode的切换（毕竟这一步只是准备而已）。

4.很多异常处理的代码返回的时候都是使用了stack相关的操作，这里没有。`movs  pc, lr `指令除了字面上意思（把lr的值付给pc），还有一个隐含的操作（movs中‘s’的含义）：把SPSR copy到CPSR，从而实现了模式的切换。lr里面如何填写了正确的跳转指令呢，需要注意这条指令：

`ARM(    ldr    lr, [pc, lr, lsl #2]    )`

`ldr    lr, pc+lr<<2`

上面如果发生中断的时候，在用户模式，则lr赋值为0，svc模式，值为3，所以用户模式，偏移为0，svc模式，偏移为3，再看PC，PC的值其实是当前的指令+8，所以lr的值在用户态发生中断则是当前指令+8，如果是svc模式，则是+20。

偏移+8，刚好是`__irq_usr`的地址，去执行`__irq_usr`的处理，而+20刚好是`__irq_svc`的处理，去执行`__irq_svc`。

#### 中断前处于用户态的中断处理

`__irq_usr`

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

1.保存发生中断时候的现场。所谓保存现场其实就是把发生中断那一刻的硬件上下文（各个寄存器）保存在了SVC mode的stack上。

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

1.`sub	sp, sp, #PT_REGS_SIZE`

这里arm已经处于svc mode了，一旦进入SVC mode，ARM处理器看到的寄存器已经发生变化，这里的sp已经变成了sp_svc了。因此，后续的压栈操作都是压入了发生中断那一刻的进程的（或者内核线程）内核栈（svc mode栈）。具体保存了S_FRAME_SIZE个寄存器值。包括r0-r15 cpsr。

2.`stmib	sp, {r1 - r12}`

压栈首先压入了r1～r12，原始的r0被保存的irq mode的stack上了。

stmib中的ib表示increment before，因此，在压入R1的时候，stack pointer会先增加4，重要是预留r0的位置。stmib  sp, {r1 - r12}指令中的sp没有“！”的修饰符，表示压栈完成后并不会真正更新stack pointer，因此sp保持原来的值。

3.`ldmia	r0, {r3 - r5}`

这里r0指向了irq stack，因此，r3是中断时候的r0值，r4是中断现场的PC值，r5是中断现场的CPSR值。

4.`add	r0, sp, #S_PC`

把r0赋值为S_PC的值

5.`mov	r6, #-1`

r6为orig_r0的值

6.`str	r3, [sp]`

保存中断那一刻的r0到sp

7.`stmia r0, {r4 - r6}`

在内核栈上保存剩余的寄存器的值，根据代码，依次是PC，CPSR和orig r0。实际上这段操作就是从irq stack就中断现场搬移到内核栈上。

8.`stmdb	r0, {sp, lr}^`

内核栈上还有两个寄存器没有保持，分别是发生中断时候sp和lr这两个寄存器。这时候，r0指向了保存PC寄存器那个地址（add  r0, sp, #S_PC），stmdb  r0, {sp, lr}^中的“db”是decrement before，因此，将sp和lr压入stack中的剩余的两个位置。需要注意的是，我们保存的是发生中断那一刻（对于本节，这是当时user mode的sp和lr），指令中的“^”符号表示访问user mode的寄存器。





usr_entry宏填充pt_regs结构体的过程如图所示，先将r1～r12保存到ARM_r1～ARM_ip（绿色部分），然后将产生中断时的r0寄存器内容保存到ARM_r0（蓝色部分），接下来将产生中断时的下一条指令地址lr_irq、spsr_irq和r4保存到ARM_pc、ARM_cpsr和ARM_ORIG_r0（红色部分），最后将用户模式下的sp和lr保存到ARM_sp 和ARM_lr 中。

![image-20230423224924202](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423224924202.png)

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

#### 中断前处于内核态的中断处理

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

`svc_entry`

```assembly
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

![image-20230423224936018](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423224936018.png)

上述的中断上下文保存过程共涉及了3种栈指针，分别是：用户空间栈指针sp_usr，内核空间栈指针sp_svc和irq模式下的栈栈指针sp_irq。sp_usr指向在setup_arg_pages函数中创建的用户空间栈。sp_svc指向在alloc_thread_info函数中创建的内核空间栈。sp_irq在cpu_init函数中被赋值，指向全局变量stacks.irq[0]。



#### 执行中断处理程序

```assembly
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

irq_handler的处理有两种配置。一种是配置了CONFIG_MULTI_IRQ_HANDLER。这种情况下，linux kernel允许run time设定irq handler。如果我们需要一个linux kernel image支持多个平台，这是就需要配置这个选项。另外一种是传统的linux的做法，irq_handler实际上就是arch_irq_handler_default。



对于情况一，machine相关代码需要设定handle_arch_irq函数指针，这里的汇编指令只需要调用这个machine代码提供的irq handler即可（当然，要准备好参数传递和返回地址设定）



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

## ARM中断（C）

根据内核不同配置，汇编部分最终会调用handle_arch_irq或者asm_do_IRQ来处理中断

### asm_do_IRQ

```cpp
asmlinkage void __exception_irq_entry
asm_do_IRQ(unsigned int irq, struct pt_regs *regs)
{
	handle_IRQ(irq, regs);
}

void handle_IRQ(unsigned int irq, struct pt_regs *regs)
{
	__handle_domain_irq(NULL, irq, false, regs);
}


int __handle_domain_irq(struct irq_domain *domain, unsigned int hwirq,
			bool lookup, struct pt_regs *regs)
{
    /*set_irq_regs将指向寄存器结构体的指针保存在一个全局的CPU变量中，后续的程序可以通过该变量访问寄存器结构体。所以在进入中断处理前，先将全局CPU变量中保存的旧指针保留下来，等到中断处理结束后再将其恢复。*/
	struct pt_regs *old_regs = set_irq_regs(regs);
	unsigned int irq = hwirq;
	int ret = 0;
	/*如果系统开启动态时钟特性且很长时间没有产生时钟中断，则调用tick_check_idle更新全局变量jiffies*/
	irq_enter();

#ifdef CONFIG_IRQ_DOMAIN
	if (lookup)
		irq = irq_find_mapping(domain, hwirq);
#endif

	/*
	 * Some hardware gives randomly wrong interrupts.  Rather
	 * than crashing, do something sensible.
	 */
	if (unlikely(!irq || irq >= nr_irqs)) {
		ack_bad_irq(irq);
		ret = -EINVAL;
	} else {
		generic_handle_irq(irq);
	}

	irq_exit();
	set_irq_regs(old_regs);
	return ret;
}


void irq_enter(void)
{
	rcu_irq_enter();
	if (is_idle_task(current) && !in_interrupt()) {
		/*
		 * Prevent raise_softirq from needlessly waking up ksoftirqd
		 * here, as softirq will be serviced on return from interrupt.
		 */
		local_bh_disable();
		tick_irq_enter();
		_local_bh_enable();
	}

	__irq_enter();
}


#define __irq_enter()					\
	do {						\
		account_irq_enter_time(current);	\
		preempt_count_add(HARDIRQ_OFFSET);	\
		trace_hardirq_enter();			\
	} while (0)

int generic_handle_irq(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);

	if (!desc)
		return -EINVAL;
	generic_handle_irq_desc(desc);
	return 0;
}

static inline void generic_handle_irq_desc(struct irq_desc *desc)
{
	desc->handle_irq(desc);
}
```

preempt_count_add(HARDIRQ_OFFSET)使表示中断处理程序嵌套层次的计数器加1。计数器保存在当前进程thread_info结构的preempt_count字段中：

![image-20230423224947935](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423224947935.png)



内核将preempt_count分成5部分：bit0~7与PREEMPT相关，bit8~15用作软中断计数器，bit16~25用作硬中断计数器，bit26用作不可屏蔽中断计数器，bit28用作PREEMPT_ACTIVE标志。



### 内核中断初始化

#### 相关结构体

##### irq_desc中断描述符

内核通过irq_desc来描述一个中断，irq_desc结构体在include/linux/irqdesc.h中定义：

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

其中，handle_irq指向电流层中断处理函数，主要用于对不同触发方式（电平、边沿）的处理。handler_data可以指向任何数据，供handle_irq函数使用。每当发生中断时，体系结构相关的代码都会调用handle_irq，该函数负责使用chip中提供的处理器相关的方法，来完成处理中断所必需的一些底层操作。用于不同中断类型的默认函数由内核提供，例如handle_level_irq和handle_edge_irq等。

##### irq_chip中断控制器

中断控制器相关操作被封装到irq_chip，该结构体在include/linux/irq.h中定义：

```cpp
struct irq_chip {
	struct device	*parent_device;
	const char	*name;
	unsigned int	(*irq_startup)(struct irq_data *data);
	void		(*irq_shutdown)(struct irq_data *data);
	void		(*irq_enable)(struct irq_data *data);
	void		(*irq_disable)(struct irq_data *data);

	void		(*irq_ack)(struct irq_data *data);
	void		(*irq_mask)(struct irq_data *data);
	void		(*irq_mask_ack)(struct irq_data *data);
	void		(*irq_unmask)(struct irq_data *data);
	void		(*irq_eoi)(struct irq_data *data);

	int		(*irq_set_affinity)(struct irq_data *data, const struct cpumask *dest, bool force);
	int		(*irq_retrigger)(struct irq_data *data);
	int		(*irq_set_type)(struct irq_data *data, unsigned int flow_type);
	int		(*irq_set_wake)(struct irq_data *data, unsigned int on);

	void		(*irq_bus_lock)(struct irq_data *data);
	void		(*irq_bus_sync_unlock)(struct irq_data *data);

	void		(*irq_cpu_online)(struct irq_data *data);
	void		(*irq_cpu_offline)(struct irq_data *data);

	void		(*irq_suspend)(struct irq_data *data);
	void		(*irq_resume)(struct irq_data *data);
	void		(*irq_pm_shutdown)(struct irq_data *data);

	void		(*irq_calc_mask)(struct irq_data *data);

	void		(*irq_print_chip)(struct irq_data *data, struct seq_file *p);
	int		(*irq_request_resources)(struct irq_data *data);
	void		(*irq_release_resources)(struct irq_data *data);

	void		(*irq_compose_msi_msg)(struct irq_data *data, struct msi_msg *msg);
	void		(*irq_write_msi_msg)(struct irq_data *data, struct msi_msg *msg);

	int		(*irq_get_irqchip_state)(struct irq_data *data, enum irqchip_irq_state which, bool *state);
	int		(*irq_set_irqchip_state)(struct irq_data *data, enum irqchip_irq_state which, bool state);

	int		(*irq_set_vcpu_affinity)(struct irq_data *data, void *vcpu_info);

	void		(*ipi_send_single)(struct irq_data *data, unsigned int cpu);
	void		(*ipi_send_mask)(struct irq_data *data, const struct cpumask *dest);

	unsigned long	flags;
};
```

##### irqaction 中断处理

irq_desc中的action指向一个操作链表，在中断发生时执行。由中断通知设备驱动程序，可以将与之相关的处理函数放置在此处。irqaction结构体在include/linux/interrupt.h中定义：

```cpp
struct irqaction {
	irq_handler_t		handler;
	void			*dev_id;
	void __percpu		*percpu_dev_id;
	struct irqaction	*next;
	irq_handler_t		thread_fn;
	struct task_struct	*thread;
	struct irqaction	*secondary;
	unsigned int		irq;
	unsigned int		flags;
	unsigned long		thread_flags;
	unsigned long		thread_mask;
	const char		*name;
	struct proc_dir_entry	*dir;
} ____cacheline_internodealigned_in_smp;
```

每个处理函数都对应该结构体的一个实例irqaction，该结构体中最重要的成员是handler，这是个函数指针。该函数指针就是通过request_irq来初始化的。当系统产生中断，内核将调用该函数指针所指向的处理函数。name和dev_id唯一地标识一个irqaction实例，name用于标识设备名，dev_id用于指向设备的数据结构实例。

   请注意irq_desc中的handle_irq不同于irqaction中的handler，这两个函数指针分别定义为：

```cpp
typedef irqreturn_t (*irq_handler_t)(int, void *);
typedef	void (*irq_flow_handler_t)(struct irq_desc *desc);
```

内核通过维护一个irq_desc的全局数组来管理所有的中断。该数组在kernel/irq/handle.c中定义：

```cpp
struct irq_desc irq_desc[NR_IRQS] __cacheline_aligned_in_smp = {
	[0 ... NR_IRQS-1] = {
		.handle_irq	= handle_bad_irq,
		.depth		= 1,
		.lock		= __RAW_SPIN_LOCK_UNLOCKED(irq_desc->lock),
	}
};
```

其中NR_IRQS表示支持中断的最大数，一般在arch/arm/mach-XXX/include/mach/irqs.h文件中定义。

![image-20230423225004736](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423225004736.png)

#### 初始化流程

##### 初始化handle_arch_irq 

start_kernel->setup_arch

```cpp
#ifdef CONFIG_MULTI_IRQ_HANDLER
	handle_arch_irq = mdesc->handle_irq;
#endif
```

##### 初始化中断向量表，完成io映射

start_kernel->setup_arch->paging_init->devicemaps_init

setup_arch中都是和板级相关的初始化，通过设备树或者静态定义的machine_desc进行初始化

```cpp
static void __init devicemaps_init(const struct machine_desc *mdesc)
{
	...
	early_trap_init(vectors);//初始化中断向量表，copy向量表到物理地址，并随后进行虚拟地址映射
	if (mdesc->map_io)
		mdesc->map_io();
	...
}
```

mdesc->map_io()实际调用的是板级的回调函数：

```cpp
MACHINE_START(S3C2440, "SMDK2440")
	/* Maintainer: Ben Dooks <ben-linux@fluff.org> */
	.atag_offset	= 0x100,

	.init_irq	= s3c2440_init_irq,
	.map_io		= smdk2440_map_io,
	.init_machine	= smdk2440_machine_init,
	.init_time	= smdk2440_init_time,
MACHINE_END
```

即调用smdk2440_map_io，此函数完成硬件IO资源和固定虚拟地址的映射，只看中断部分：

```cpp
#define IODESC_ENT(x) { (unsigned long)S3C24XX_VA_##x, __phys_to_pfn(S3C24XX_PA_##x), S3C24XX_SZ_##x, MT_DEVICE }

#define S3C24XX_VA_IRQ		S3C_VA_IRQ
#define S3C_VA_IRQ	S3C_ADDR(0x00000000)	/* irq controller(s) */
#define S3C_ADDR_BASE	0xF6000000  //映射到的虚拟基地址
#define S3C_ADDR(x)	(S3C_ADDR_BASE + (x))
#define S3C2410_PA_IRQ		(0x4A000000)  //中断控制器物理基地址
#define S3C24XX_SZ_IRQ		SZ_1M  //映射建立的大小
struct map_desc {
	unsigned long virtual;
	unsigned long pfn;
	unsigned long length;
	unsigned int type;
};

static struct map_desc s3c_iodesc[] __initdata = {
	IODESC_ENT(GPIO),
	IODESC_ENT(IRQ),//中断映射表
	IODESC_ENT(MEMCTRL),
	IODESC_ENT(UART)
};

static void __init smdk2440_map_io(void)
{
	s3c24xx_init_io(smdk2440_iodesc, ARRAY_SIZE(smdk2440_iodesc));
	s3c24xx_init_uarts(smdk2440_uartcfgs, ARRAY_SIZE(smdk2440_uartcfgs));
	samsung_set_timer_source(SAMSUNG_PWM3, SAMSUNG_PWM4);
}

void __init s3c24xx_init_io(struct map_desc *mach_desc, int size)
{
	arm_pm_idle = s3c24xx_default_idle;

	/* initialise the io descriptors we need for initialisation */
	iotable_init(mach_desc, size);
	iotable_init(s3c_iodesc, ARRAY_SIZE(s3c_iodesc));//进行物理地址和虚拟地址的静态映射

	if (cpu_architecture() >= CPU_ARCH_ARMv5) {
		samsung_cpu_id = s3c24xx_read_idcode_v5();
	} else {
		samsung_cpu_id = s3c24xx_read_idcode_v4();
	}

	s3c_init_cpu(samsung_cpu_id, cpu_ids, ARRAY_SIZE(cpu_ids));

	samsung_pwm_set_platdata(&s3c24xx_pwm_variant);
}

void __init iotable_init(struct map_desc *io_desc, int nr)
{
	struct map_desc *md;
	struct vm_struct *vm;
	struct static_vm *svm;

	if (!nr)
		return;

	svm = early_alloc_aligned(sizeof(*svm) * nr, __alignof__(*svm));

	for (md = io_desc; nr; md++, nr--) {
		create_mapping(md);

		vm = &svm->vm;
		vm->addr = (void *)(md->virtual & PAGE_MASK);
		vm->size = PAGE_ALIGN(md->length + (md->virtual & ~PAGE_MASK));
		vm->phys_addr = __pfn_to_phys(md->pfn);
		vm->flags = VM_IOREMAP | VM_ARM_STATIC_MAPPING;
		vm->flags |= VM_ARM_MTYPE(md->type);
		vm->caller = iotable_init;
		add_static_vm_early(svm++);
	}
}
```

最后实际物理地址0x4A000000开始的1M空间被映射到虚拟地址0xF6000000处

##### 初始化中断描述符

start_kernel->early_irq_init

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

	desc = irq_desc;
	count = ARRAY_SIZE(irq_desc);

	for (i = 0; i < count; i++) {
		desc[i].kstat_irqs = alloc_percpu(unsigned int);
		alloc_masks(&desc[i], GFP_KERNEL, node);
		raw_spin_lock_init(&desc[i].lock);
		lockdep_set_class(&desc[i].lock, &irq_desc_lock_class);
		desc_set_defaults(i, &desc[i], node, NULL, NULL);
	}
	return arch_early_irq_init();
}
```

kernel/irq/handle.c文件中，根据内核的配置选项CONFIG_SPARSE_IRQ，可以选择两个不同版本的early_irq_init()中的一个进行编译。CONFIG_SPARSE_IRQ配置项，用于支持稀疏irq号，对于发行版的内核很有用，它允许定义一个高CONFIG_NR_CPUS值，但仍然不希望消耗太多内存的情况。通常情况下，我们并不配置该选项。初始化struct irq_desc irq_desc[NR_IRQS]数组

##### 初始化中断控制器

```cpp
void __init init_IRQ(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_OF) && !machine_desc->init_irq)//设备树走这里
		irqchip_init();
	else
		machine_desc->init_irq();//否则使用MACHINE_START中函数静态初始化

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

machine_desc->init_irq()被定义为s3c2440_init_irq

```cpp
void __init s3c2440_init_irq(void)
{
	pr_info("S3C2440: IRQ Support\n");

#ifdef CONFIG_FIQ
	init_FIQ(FIQ_START);
#endif

	s3c_intc[0] = s3c24xx_init_intc(NULL, &init_s3c2440base[0], NULL,
					0x4a000000);
	if (IS_ERR(s3c_intc[0])) {
		pr_err("irq: could not create main interrupt controller\n");
		return;
	}

	s3c24xx_init_intc(NULL, &init_eint[0], s3c_intc[0], 0x560000a4);
	s3c_intc[1] = s3c24xx_init_intc(NULL, &init_s3c2440subint[0],
					s3c_intc[0], 0x4a000018);
}
```

其中init_s3c2440base[0]和&init_s3c2440subint[0]定义如下:

```cpp
static struct s3c_irq_data init_s3c2440base[32] = {
	{ .type = S3C_IRQTYPE_EINT, }, /* EINT0 */
	{ .type = S3C_IRQTYPE_EINT, }, /* EINT1 */
	{ .type = S3C_IRQTYPE_EINT, }, /* EINT2 */
	{ .type = S3C_IRQTYPE_EINT, }, /* EINT3 */
	{ .type = S3C_IRQTYPE_LEVEL, }, /* EINT4to7 */
	{ .type = S3C_IRQTYPE_LEVEL, }, /* EINT8to23 */
	{ .type = S3C_IRQTYPE_LEVEL, }, /* CAM */
	{ .type = S3C_IRQTYPE_EDGE, }, /* nBATT_FLT */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TICK */
	{ .type = S3C_IRQTYPE_LEVEL, }, /* WDT/AC97 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TIMER0 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TIMER1 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TIMER2 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TIMER3 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TIMER4 */
	{ .type = S3C_IRQTYPE_LEVEL, }, /* UART2 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* LCD */
	{ .type = S3C_IRQTYPE_EDGE, }, /* DMA0 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* DMA1 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* DMA2 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* DMA3 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* SDI */
	{ .type = S3C_IRQTYPE_EDGE, }, /* SPI0 */
	{ .type = S3C_IRQTYPE_LEVEL, }, /* UART1 */
	{ .type = S3C_IRQTYPE_LEVEL, }, /* NFCON */
	{ .type = S3C_IRQTYPE_EDGE, }, /* USBD */
	{ .type = S3C_IRQTYPE_EDGE, }, /* USBH */
	{ .type = S3C_IRQTYPE_EDGE, }, /* IIC */
	{ .type = S3C_IRQTYPE_LEVEL, }, /* UART0 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* SPI1 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* RTC */
	{ .type = S3C_IRQTYPE_LEVEL, }, /* ADCPARENT */
};

static struct s3c_irq_data init_s3c2440subint[32] = {
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 28 }, /* UART0-RX */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 28 }, /* UART0-TX */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 28 }, /* UART0-ERR */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 23 }, /* UART1-RX */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 23 }, /* UART1-TX */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 23 }, /* UART1-ERR */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 15 }, /* UART2-RX */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 15 }, /* UART2-TX */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 15 }, /* UART2-ERR */
	{ .type = S3C_IRQTYPE_EDGE, .parent_irq = 31 }, /* TC */
	{ .type = S3C_IRQTYPE_EDGE, .parent_irq = 31 }, /* ADC */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 6 }, /* CAM_C */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 6 }, /* CAM_P */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 9 }, /* WDT */
	{ .type = S3C_IRQTYPE_LEVEL, .parent_irq = 9 }, /* AC97 */
};
```

s3c2440支持60个外部中断源，提供这些中断源的是内部外设，如 DMA 控制器、UART、IIC 等等。

如下是s3c2440的中断源：

![image-20230423235320815](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423235320815.png)

![image-20230423235333215](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423235333215.png)

当从内部外设和外部中断请求引脚收到多个中断请求时，中断控制器在仲裁步骤后请求 ARM920T 内核的 FIQ 或 IRQ。 仲裁步骤由硬件优先级逻辑决定并且写入结果到帮助用户通告是各种中断源中的哪个中断发生了的中断挂起 寄存器中。

中断源和次级中断源，处理器对其处理的流程也有所不同，一下是2440处理中断请求的流程：

![image-20230423235245097](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230423235245097.png)

当中断请求是次级中断源时，首先是次级源挂起（SUBSRCPND）寄存器对应的中断源被置位，随后到达中断次级屏蔽（INTSUBMSK）寄存器。

INTSUBMSK会根据是否屏蔽对应的中断源，发送或者不发送给源挂起（SRCPND）寄存器。

当SRCPND收到请求后，会根据中断模式（INTMOD）寄存器判断是IRQ还是FIQ。

当为FIQ时，会发送给中断屏蔽（INTMSK）寄存器，

INTMSK会根据是否屏蔽对应的中断源，发送或者不发送给下一级的优先级寄存器（PRIORITY）

PRIORITY会根据中断优先级，将最高优先级的中断源给到中断挂起（INTPND）寄存器

最后INTPND产生中断请求IRQ给CPU。

```cpp
static struct s3c_irq_intc * __init s3c24xx_init_intc(struct device_node *np,
				       struct s3c_irq_data *irq_data,//中断源
				       struct s3c_irq_intc *parent,//是否有父级
				       unsigned long address)//基地址
{
	struct s3c_irq_intc *intc;
	void __iomem *base = (void *)0xf6000000; /* static mapping */  //中断控制器物理基地址映射的静态映射的虚拟地址
	int irq_num;
	int irq_start;
	int ret;

	intc = kzalloc(sizeof(struct s3c_irq_intc), GFP_KERNEL);
	if (!intc)
		return ERR_PTR(-ENOMEM);

	intc->irqs = irq_data;

	if (parent)
		intc->parent = parent;

	/* select the correct data for the controller.
	 * Need to hard code the irq num start and offset
	 * to preserve the static mapping for now
	 */
	switch (address) {
	case 0x4a000000://源挂起寄存器物理基地址
		pr_debug("irq: found main intc\n");
		intc->reg_pending = base;//源挂起寄存器基地址
		intc->reg_mask = base + 0x08;//中断屏蔽寄存器基地址
		intc->reg_intpnd = base + 0x10;//中断挂起寄存器基地址
		irq_num = 32;//中断源个数
		irq_start = S3C2410_IRQ(0);//起始地址
		break;
	case 0x4a000018://次级源挂起寄存器物理基地址
		pr_debug("irq: found subintc\n");
		intc->reg_pending = base + 0x18;//次级源挂起寄存器基地址
		intc->reg_mask = base + 0x1c;//次级源中断屏蔽寄存器基地址
		irq_num = 29;//中断源个数
		irq_start = S3C2410_IRQSUB(0);//起始地址
		break;
	case 0x4a000040:
		pr_debug("irq: found intc2\n");
		intc->reg_pending = base + 0x40;
		intc->reg_mask = base + 0x48;
		intc->reg_intpnd = base + 0x50;
		irq_num = 8;
		irq_start = S3C2416_IRQ(0);
		break;
	case 0x560000a4://外部中断屏蔽寄存器物理基地址
		pr_debug("irq: found eintc\n");
		base = (void *)0xfd000000;//静态映射虚拟地址

		intc->reg_mask = base + 0xa4;//外部中断屏蔽寄存器基地址
		intc->reg_pending = base + 0xa8;//外部中断挂起寄存器基地址
		irq_num = 24;//源个数
		irq_start = S3C2410_IRQ(32);//起始地址
		break;
	default:
		pr_err("irq: unsupported controller address\n");
		ret = -EINVAL;
		goto err;
	}

	/* now that all the data is complete, init the irq-domain */
	s3c24xx_clear_intc(intc);
	intc->domain = irq_domain_add_legacy(np, irq_num, irq_start,
					     0, &s3c24xx_irq_ops,
					     intc);
	if (!intc->domain) {
		pr_err("irq: could not create irq-domain\n");
		ret = -EINVAL;
		goto err;
	}

	set_handle_irq(s3c24xx_handle_irq);

	return intc;

err:
	kfree(intc);
	return ERR_PTR(ret);
}
```

其中s3c24xx_clear_intc用来清除一些寄存器

```cpp
static void s3c24xx_clear_intc(struct s3c_irq_intc *intc)
{
	void __iomem *reg_source;
	unsigned long pend;
	unsigned long last;
	int i;

	/* if intpnd is set, read the next pending irq from there */
	//获取中断挂起寄存器/源挂起寄存器基地址，这里优先获取中断挂起寄存器是因为处理流程中它的状态在源的后面。
	reg_source = intc->reg_intpnd ? intc->reg_intpnd : intc->reg_pending;

	last = 0;
	for (i = 0; i < 4; i++) {
		pend = readl_relaxed(reg_source);//读取寄存器值

		//pend为0，即没有中断请求到达中断挂起寄存器/源挂起寄存器。pend == last即回写后读取的值没有变
		//不理解这个操作在干嘛
		if (pend == 0 || pend == last)
			break;

		writel_relaxed(pend, intc->reg_pending);//回写寄存器
		if (intc->reg_intpnd)
			writel_relaxed(pend, intc->reg_intpnd);//回写寄存器

		pr_info("irq: clearing pending status %08x\n", (int)pend);
		last = pend;
	}
}
```



```cpp
struct irq_domain *irq_domain_add_legacy(struct device_node *of_node,
					 unsigned int size,
					 unsigned int first_irq,
					 irq_hw_number_t first_hwirq,
					 const struct irq_domain_ops *ops,
					 void *host_data)
{
	struct irq_domain *domain;

	domain = __irq_domain_add(of_node_to_fwnode(of_node), first_hwirq + size,
				  first_hwirq + size, 0, ops, host_data);
	if (domain)
		irq_domain_associate_many(domain, first_irq, first_hwirq, size);

	return domain;
}

struct irq_domain *__irq_domain_add(struct fwnode_handle *fwnode, int size,
				    irq_hw_number_t hwirq_max, int direct_max,
				    const struct irq_domain_ops *ops,
				    void *host_data)
{
	struct device_node *of_node = to_of_node(fwnode);
	struct irq_domain *domain;

	domain = kzalloc_node(sizeof(*domain) + (sizeof(unsigned int) * size),
			      GFP_KERNEL, of_node_to_nid(of_node));//分配一个新的domain
	if (WARN_ON(!domain))
		return NULL;

	of_node_get(of_node);
	//初始化domain
	/* Fill structure */
	INIT_RADIX_TREE(&domain->revmap_tree, GFP_KERNEL);
	domain->ops = ops;
	domain->host_data = host_data;
	domain->fwnode = fwnode;
	domain->hwirq_max = hwirq_max;
	domain->revmap_size = size;
	domain->revmap_direct_max_irq = direct_max;
	irq_domain_check_hierarchy(domain);
	//加入链表
	mutex_lock(&irq_domain_mutex);
	list_add(&domain->link, &irq_domain_list);
	mutex_unlock(&irq_domain_mutex);

	pr_debug("Added domain %s\n", domain->name);
	return domain;
}

void irq_domain_associate_many(struct irq_domain *domain, unsigned int irq_base,
			       irq_hw_number_t hwirq_base, int count)
{
	struct device_node *of_node;
	int i;

	of_node = irq_domain_get_of_node(domain);
	pr_debug("%s(%s, irqbase=%i, hwbase=%i, count=%i)\n", __func__,
		of_node_full_name(of_node), irq_base, (int)hwirq_base, count);

	for (i = 0; i < count; i++) {//建立irq和hwirq的映射关系
		irq_domain_associate(domain, irq_base + i, hwirq_base + i);
	}
}


int irq_domain_associate(struct irq_domain *domain, unsigned int virq,
			 irq_hw_number_t hwirq)
{
	//根据虚拟中断号，获取中断描述符的irq_data。如主中断源起始地址为S3C2410_IRQ(0)
	//#define S3C2410_CPUIRQ_OFFSET	 (16)
	//#define S3C2410_IRQ(x) ((x) + S3C2410_CPUIRQ_OFFSET)
	//即virq = 16，此时的hwirq=0;

	struct irq_data *irq_data = irq_get_irq_data(virq);
	int ret;

	if (WARN(hwirq >= domain->hwirq_max,
		 "error: hwirq 0x%x is too large for %s\n", (int)hwirq, domain->name))
		return -EINVAL;
	if (WARN(!irq_data, "error: virq%i is not allocated", virq))
		return -EINVAL;
	if (WARN(irq_data->domain, "error: virq%i is already associated", virq))
		return -EINVAL;

	mutex_lock(&irq_domain_mutex);
	irq_data->hwirq = hwirq;//设置hwirq
	irq_data->domain = domain;//保存domain
	if (domain->ops->map) {//进行virq和hwirq的映射
		ret = domain->ops->map(domain, virq, hwirq);
		if (ret != 0) {
			/*
			 * If map() returns -EPERM, this interrupt is protected
			 * by the firmware or some other service and shall not
			 * be mapped. Don't bother telling the user about it.
			 */
			if (ret != -EPERM) {
				pr_info("%s didn't like hwirq-0x%lx to VIRQ%i mapping (rc=%d)\n",
				       domain->name, hwirq, virq, ret);
			}
			irq_data->domain = NULL;
			irq_data->hwirq = 0;
			mutex_unlock(&irq_domain_mutex);
			return ret;
		}

		/* If not already assigned, give the domain the chip's name */
		if (!domain->name && irq_data->chip)
			domain->name = irq_data->chip->name;
	}

	if (hwirq < domain->revmap_size) {
		domain->linear_revmap[hwirq] = virq;
	} else {
		mutex_lock(&revmap_trees_mutex);
		radix_tree_insert(&domain->revmap_tree, hwirq, irq_data);//组织hwirq和irq_data关系并放入树
		mutex_unlock(&revmap_trees_mutex);
	}
	mutex_unlock(&irq_domain_mutex);

	irq_clear_status_flags(virq, IRQ_NOREQUEST);

	return 0;
}
```

domain->ops->map即s3c24xx_irq_map

```cpp

```