# u-boot 启动流程

## u-boot启动第一阶段分析

`u-boot-2010.09\arch\arm\cpu\arm920t`下的`u-boot.lds`文件指定连接方式

```cpp
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	. = 0x00000000;//起始地址

	. = ALIGN(4);//4字节对齐
	.text :
	{
		arch/arm/cpu/arm920t/start.o	(.text)
         board/samsung/mini2440/lowlevel_init.o (.text)
		board/samsung/mini2440/nand_read.o (.text)
		*(.text)
	}

	. = ALIGN(4); //前面的“.” 代表当前值，是计算一个当前的值，是计算上面占用的整个空间，再加一个单元就表示它现在的位置
	.rodata : { *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*))) }/*只读数据段*/

	. = ALIGN(4);
	.data : { *(.data) }/*数据段*/

	. = ALIGN(4);
	.got : { *(.got) }

	. = .;
	__u_boot_cmd_start = .;/*命令链接起始地址*/
	.u_boot_cmd : { *(.u_boot_cmd) }/*命令区*/
	__u_boot_cmd_end = .;/*命令链接结束地址*/

	. = ALIGN(4);
	__bss_start = .;/*未初始化和初始化为0的数据段*/
	.bss (NOLOAD) : { *(.bss) . = ALIGN(4); }
	_end = .;
}
```

#### 异常中断向量初始化

```asm
.globl _start								/*相当于extern _start*/
_start:	b	start_code						/* 复位,程序跳转到_start的话，会执行b start_code即跳转到start_code处*/
/*ldr R1,[R2],将R2的值赋值给R1,*/
	ldr	pc, _undefined_instruction			 /* 未定义指令向量*/
	ldr	pc, _software_interrupt				/* 软件中断向量*/
	ldr	pc, _prefetch_abort					/* 预取指令异常向量*/
	ldr	pc, _data_abort						/* 数据操作异常向量*/
	ldr	pc, _not_used						/* 未使用*/
	ldr	pc, _irq							/* irq中断向量*/
	ldr	pc, _fiq							/* fiq中断向量*/ 
	
/*.word意思是分配4个字节，里面存放undefined_instruction，
即_undefined_instruction = &undefined_instruction，而undefined_instruction是个异常处理handler，所以：
ldr pc, _undefined_instruction
_undefined_instruction:	.word undefined_instruction
.align  5
undefined_instruction:
	get_bad_stack
	bad_save_user_regs
	bl	do_undefined_instruction
	的意思就是跳转到undefined_instruction，执行对应代码
*/
/* 中断向量表入口地址*/
_undefined_instruction:	.word undefined_instruction
_software_interrupt:	.word software_interrupt
_prefetch_abort:	.word prefetch_abort
_data_abort:		.word data_abort
_not_used:		.word not_used
_irq:			.word irq
_fiq:			.word fiq

	.balignl 16,0xdeadbeef
...
/*
 * exception handlers
 */
	.align  5/*2^5对齐*/
undefined_instruction:
	get_bad_stack
	bad_save_user_regs
	bl	do_undefined_instruction

	.align	5
software_interrupt:
	get_bad_stack
	bad_save_user_regs
	bl	do_software_interrupt

	.align	5
prefetch_abort:
	get_bad_stack
	bad_save_user_regs
	bl	do_prefetch_abort

	.align	5
data_abort:
	get_bad_stack
	bad_save_user_regs
	bl	do_data_abort

	.align	5
not_used:
	get_bad_stack
	bad_save_user_regs
	bl	do_not_used

#ifdef CONFIG_USE_IRQ

	.align	5
irq:
	get_irq_stack
	irq_save_user_regs
	bl	do_irq
	irq_restore_user_regs

	.align	5
fiq:
	get_fiq_stack
	/* someone ought to write a more effiction fiq_save_user_regs */
	irq_save_user_regs
	bl	do_fiq
	irq_restore_user_regs

#else

	.align	5
irq:
	get_bad_stack
	bad_save_user_regs
	bl	do_irq

	.align	5
fiq:
	get_bad_stack
	bad_save_user_regs
	bl	do_fiq

#endif

```

ARM的异常向量表

![image-20230424005812014](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005812014.png)

其中复位异常向量指令`b start_code`决定了U-Boot启动后将自动跳转到标号`start_code`处执行。



接下来只是定义一些变量

```asm
/*
 *************************************************************************
 *
 * Startup Code (called from the ARM reset exception vector)
 *
 * do important init only if we don't start from memory!
 * relocate armboot to ram
 * setup stack
 * jump to second stage
 *
 *************************************************************************
 */
/*_TEXT_BASE = TEXT_BASE*/
_TEXT_BASE:
	.word	TEXT_BASE
/*_armboot_start = _start*/
.globl _armboot_start
_armboot_start:
	.word _start

/*
 * These are defined in the board-specific linker script.
 */
 /*_bss_start = &__bss_start*/
.globl _bss_start
_bss_start:
	.word __bss_start
/*_bss_end = &__end*/
.globl _bss_end
_bss_end:
	.word _end

#ifdef CONFIG_USE_IRQ
/* IRQ stack memory (calculated at run-time) */
.globl IRQ_STACK_START
IRQ_STACK_START:
	.word	0x0badc0de

/* IRQ stack memory (calculated at run-time) */
.globl FIQ_STACK_START
FIQ_STACK_START:
	.word 0x0badc0de
#endif

```

进入`start_code`

#### 设置CPU为SVC管理员模式

```asm
/*
 * the actual start code
 */

start_code:
	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0, cpsr 				/*MRS读出CPSR寄存器值到R0*/
	bic	r0, r0, #0x1f 			/*清除4:0位*/
	orr	r0, r0, #0xd3 			/*或上0xd3*/
	msr	cpsr, r0 				/*MSR写入CPSR寄存器*/
/*此时cpsr寄存器值为11010011b，CPSR值为此值时，关闭IRQ,FIQ,设置CPU到SVC管理员模式*/
```



```asm
# if defined(CONFIG_S3C2400)
# define pWTCON 0x15300000 
# define INTMSK 0x14400008
# define CLKDIVN 0x14800014
#else /* s3c2410与s3c2440下面4个寄存器地址相同 */
# define pWTCON 0x53000000 				/* (WitchDog Timer)看门狗定时器寄存器WTCON，设为0X0表示关闭看门狗 */
# define INTMSK 0x4A000008 				/* Interrupt Mask)中断屏蔽寄存器INTMSK，相应位=0:开启中断服务，相应位=1:关闭中断服务 */
# define INTSUBMSK 0x4A00001C 			/* 中断次级屏蔽寄存器，相应位=0:开启中断服务，相应位=1:关闭中断服务 */
# define CLKDIVN 0x4C000014 			/* 时钟分频寄存器 */
# endif
```

对与s3c2440开发板，以上代码完成了`WATCHDOG，INTMSK，INTSUBMSK，CLKDIVN `四个寄存器的地址的设置。

#### 关闭看门狗

```asm
ldr	r0, =pWTCON 				/*R0等于WTCON地址*/
mov	r1, #0x0 					/*R1=0x0*/
str	r1, [r0] 					/*关闭WTCON寄存器,pWTCON=0*/
```

看门狗控制器的最低位为0时，看门狗不输出复位信号 */ 以上代码向看门狗控制寄存器写入0，关闭看门狗。否则在U-Boot启动过程中，CPU 将不断重启。

为什么要关看门狗？

就是防止，不同得两个以上得CPU，进行喂狗的时间间隔问题：说白了，就是你运行的代码如果超出喂狗时间，而你不关狗，就会导致，你代码还没运行完又得去喂狗，就这样反复得重启CPU，那你代码永远也运行不完，所以，得先关看门狗得原因，就是这样。关狗---详细的原因：

关闭看门狗，关闭中断，所谓的喂狗是每隔一段时间给某个寄存器置位而已，在实际中会专门启动一个线程或进程会专门喂狗，当上层软件出现故障时就会停止喂狗，停止喂狗之后，cpu会自动复位，一般都在外部专门有一个看门狗，做一个外部的电路，不在cpu内部使用看门狗，cpu内部的看门狗是复位的cpu

当开发板很复杂时，有好几个cpu时，就不能完全让板子复位，但我们通常都让整个板子复位。看门狗每隔短时间就会喂狗，问题是在两次喂狗之间的时间间隔内，运行的代码的时间是否够用，两次喂狗之间的代码是否在两次喂狗的时间延迟之内，如果在延迟之外的话，代码还没改完就又进行喂狗，代码永远也改不完

#### 屏蔽中断

```asm
/*
* mask all IRQs by setting all bits in the INTMR - default
*/
mov r1, #0xffffffff 		/* 某位被置1则对应的中断被屏蔽，R1=0XFFFF FFFF*/
ldr r0, =INTMSK 			/*R0等于INTMSK地址*/
str r1, [r0] 				/**0x4A000008=0XFFFF FFFF(关闭所有中断)*/
```

`INTMSK`是主中断屏蔽寄存器，每一位对应`SRCPND`（中断源引脚寄存器）中的一位，表明`SRCPND`相应位代表的中断请求是否被CPU所处理。`INTMSK`寄存器是一个32位的寄存器，每位对应一个中断，向其中写入0xffffffff就将`INTMSK`寄存器全部位置一，从而屏蔽对应的中断。

```asm
# if defined(CONFIG_S3C2440)
ldr r1, =0x7fff 			/*R1=0x7FF*/
ldr r0, =INTSUBMSK			/*R0等于INTSUBMSK地址*/
str r1, [r0]				/**0x4A00001C=0x3FF(关闭次级所有中断)*/
# endif
```

`INTSUBMSK`每一位对应`SUBSRCPND`中的一位，表明`SUBSRCPND`相应位代表的中断请求是否被CPU所处理`INTSUBMSK`寄存器是一个32位的寄存器，但是只使用了低15位。向其中写入0x7fff就是将`INTSUBMSK`寄存器全部有效位（低15位）置一，从而屏蔽对应的中断。



屏蔽所有中断，为什么要关中断？

中断处理中ldr pc是将代码的编译地址放在了指针上，而这段时间还没有搬移代码，所以编译地址上面没有这个代码，如果进行跳转就会跳转到空指针上面

#### 设置系统时钟

```asm
# if defined(CONFIG_S3C2440)
#define MPLLCON 0x4C000004
#define UPLLCON 0x4C000008
ldr r0, =CLKDIVN
mov r1, #5 				/*设置CLKDIVN寄存器值为0x5*/
str r1, [r0]
ldr r0, =MPLLCON 		/*设置MPLLCON*/
ldr r1, =0x7F021
str r1, [r0]
ldr r0, =UPLLCON		/*设置UPLLCON*/
ldr r1, =0x38022
str r1, [r0]
# else
/* FCLK:HCLK:PCLK = 1:2:4 */
/* default FCLK is 120 MHz ! */
ldr r0, =CLKDIVN
mov r1, #3
str r1, [r0]
#endif
```

CPU上电几毫秒后，晶振输出稳定，FCLK=Fin（晶振频率），CPU开始执行指令。但实际上，FCLK可以高于Fin，为了提高系统时钟，需要用软件来启用PLL。这就需要设置CLKDIVN，MPLLCON，UPLLCON这3个寄存器。

CLKDIVN寄存器用于设置FCLK，HCLK，PCLK三者间的比例，可以根据表2.2来设置。

![image-20230424010525861](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424010525861.png)

设置CLKDIVN为5，就将HDIVN设置为二进制的10，由于CAMDIVN[9]没有被改变过，取默认值0，因此HCLK = FCLK/4。PDIVN被设置为1，因此PCLK= HCLK/2。因此分频比FCLK:HCLK:PCLK = 1:4:8 。MPLLCON寄存器用于设置FCLK与Fin的倍数。MPLLCON的位[19:12]称为MDIV，位[9:4]称为PDIV，位[1:0]称为SDIV。对于S3C2440，FCLK与Fin的关系如下面公式：

`MPLL(FCLK) = (2×m×Fin)/(p×2^s)`

其中： `m=MDIC+8，p=PDIV+2，s=SDIV，Fin=12M`，根据`s3c2440 datasheet`上的`PLL`推荐表(`FCLK`最高工作在`400MHZ`)

![image-20230424010743265](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424010743265.png)

当mini2440系统主频设置为405MHZ，`USB`时钟频率设置为48MHZ时，系统可以稳定运行，因此设置`MPLLCON`与`UPLLCON`为：

`MPLLCON=(0x7f<<12) | (0x02<<4) | (0x01) = 0x7f021`

`UPLLCON=(0x38<<12) | (0x02<<4) | (0x02) = 0x38022`



设置时钟分频，为什么要设置时钟？

其实可以不设，系统能不能跑起来和频率没有任何关系，频率的设置是要让外围的设备能承受所设置的频率，如果频率过高则会导致cpu操作外围设备失败

说白了：设置频率，就为了CPU能去操作外围设备

#### 关闭MMU和cache

```asm
#ifndef CONFIG_SKIP_LOWLEVEL_INIT
	bl	cpu_init_crit
#endif
```

跳转到`cpu_init_crit`，cpu_init_crit这段代码在U-Boot正常启动时才需要执行，若将U-Boot从RAM中启动则应该注释掉这段代码。

```asm
320 #ifndef CONFIG_SKIP_LOWLEVEL_INIT
321 cpu_init_crit:
322 
323 /* 使数据cache与指令cache无效 */
324 
325 mov r0, #0
326 mcr p15, 0, r0, c7, c7, 0 			/* 关闭ICaches(指令缓存,关闭是为了降低MMU查表带来的开销)和DCaches(数据缓存,DCaches使用的是虚拟地址，开启MMU之前必须关闭)*/
327 mcr p15, 0, r0, c8, c7, 0 			/* 使无效整个数据TLB和指令TLB(TLB就是负责将虚拟内存地址翻译成实际的物理内存地址) */
328
329 
330 /* disable MMU stuff and caches*/
331 
332 mrc p15, 0, r0, c1, c0, 0 			/* 读出控制寄存器到r0中 */
333 bic r0, r0, #0x00002300  			@clear bits 13, 9:8 (--V- --RS) /*bit8:系统不保护,bit9:ROM不保护,bit13:设置正常异常模式0x0~0x1c,即异常模式基地址为0X0*/
334 bic r0, r0, #0x00000087 			@ clear bits 7, 2:0 (B--- -CAM) /*bit0~2:禁止MMU，禁止地址对齐检查,禁止数据Cache.bit7:设为小端模式*/
335 orr r0, r0, #0x00000002 			@ set bit 2 (A) Align /*bit2:开启数据Cache*/
336 orr r0, r0, #0x00001000 			@ set bit 12 (I) I-Cache /*bit12:开启指令Cache*/
337 mcr p15, 0, r0, c1, c0, 0 			/* 保存r0到控制寄存器 */
338

/*
/*
mcr/mrc:
Caches:是一种高速缓存存储器，用于保存CPU频繁使用的数据。在使用Cache技术的处理器上，当一条指令要访问内存的数据时，
首先查询cache缓存中是否有数据以及数据是否过期，如果数据未过期则从cache读出数据。处理器会定期回写cache中的数据到内存。
根据程序的局部性原理，使用cache后可以大大加快处理器访问内存数据的速度。
其中DCaches和ICaches分别用来存放数据和执行这些数据的指令

TLB:就是负责将虚拟内存地址翻译成实际的物理内存地址,TLB中存放了一些页表文件，文件中记录了虚拟地址和物理地址的映射关系。
当应用程序访问一个虚拟地址的时候，会从TLB中查询出对应的物理地址，然后访问物理地址。TLB通常是一个分层结构，
使用与Cache类似的原理。处理器使用一定的算法把最常用的页表放在最先访问的层次。
这里禁用MMU，是方便后面直接使用物理地址来设置控制寄存器
*/
*/
```

代码中的`c0`，`c1`，`c7`，`c8`都是`ARM920T`的协处理器`CP15`的寄存器。其中`c7`是`cache` 控制寄存器，`c8`是`TLB`控制寄存器。325~327行代码将0写入`c7`、`c8`，使`Cache`，`TLB`内容无效。

第332~337行代码关闭了`MMU`。这是通过修改`CP15`的`c1`寄存器来实现的，先看`CP15`的`c1`寄存器的格式（仅列出代码中用到的位）：

![image-20230424011156745](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424011156745.png)

各个位的意义如下：

V : 表示异常向量表所在的位置，0：异常向量在`0x00000000`；1：异常向量在` 0xFFFF0000`

 I : 0 ：关闭`ICaches`；1 ：开启`ICaches`

R、S : 用来与页表中的描述符一起确定内存的访问权限

B : 0 ：CPU为小字节序；1 ： CPU为大字节序

C : 0：关闭`DCaches`；1：开启`DCaches`

A : 0：数据访问时不进行地址对齐检查；1：数据访问时进行地址对齐检查

M : 0：关闭MMU；1：开启MMU

332~337行代码将c1的 M位置零，关闭了MMU。



一，关catch

catch和MMU是通过CP15管理的，刚上电的时候，CPU还不能管理他们

上电的时候MMU必须关闭，指令catch可关闭，可不关闭，但数据catch一定要关闭否则可能导致刚开始的代码里面，去取数据的时候，从catch里面取，而这时候RAM 中数据还没有catch过来，导致数据预取异常

二：关MMU

因为MMU是;把虚拟地址转化为物理地址得作用

而目的是设置控制寄存器，而控制寄存器本来就是实地址（物理地址），再使能MMU，不就是多此一举了吗？

详细分析---

Catch是cpu内部的一个2级缓存，它的作用是将常用的数据和指令放在cpu内部，MMU是用来把虚实地址转换为物理地址用的

我们的目的:是设置控制的寄存器，寄存器都是实地址（物理地址），如果既要开启MMU 又要做虚实地址转换的话，中间还多一步，多此一举了嘛?

先要把实地址转换成虚地址，然后再做设置，但对uboot而言就是起到一个简单的初始化的作用和引导操作系统，如果开启MMU的话，很麻烦，也没必要，所以关闭MMU.

说到catch就必须提到一个关键字Volatile，以后在设置寄存器时会经常遇到，他的本质：是告诉编译器不要对我的代码进行优化，作用是让编写者感觉不倒变量的变化情况（也就是说，让它执行速度加快吧）

优化的过程：是将常用的代码取出来放到catch中，它没有从实际的物理地址去取，它直接从cpu的缓存中去取，但常用的代码就是为了感觉一些常用变量的变化

优化原因：如果正在取数据的时候发生跳变，那么就感觉不到变量的变化了，所以在这种情况下要用Volatile关键字告诉编译器不要做优化，每次从实际的物理地址中去取指令，这就是为什么关闭catch关闭MMU。

但在C语言中是不会关闭catch和MMU的，会打开，如果编写者要感觉外界变化，或变化太快，从catch中取数据会有误差，就加一个关键字Volatile。



#### 初始化RAM控制寄存器

```asm
339 /*
340 * before relocating, we have to setup RAM timing
341 * because memory timing is board-dependend, you will
342 * find a lowlevel_init.S in your board directory.
343 */
344 mov ip, lr 				/*临时保存当前子程序返回地址,因为接下来执行bl会覆盖当前返回地址.*/
345
346 bl lowlevel_init 		/*跳转到lowlevel_init(位于u-boot-1.1.6/board/xxx/lowlevel_init.S)*/
347
348 mov lr, ip 				/*恢复当前返回地址*/
349 mov pc, lr 				/*退出*/
350 #endif /* CONFIG_SKIP_LOWLEVEL_INIT */
```

其中的`lowlevel_init`就完成了内存初始化的工作，由于内存初始化是依赖于开发板的，因此`lowlevel_init`的代码一般放在board下面相应的目录中。对于`mini2440`，`lowlevel_init` 在`board/samsung/mini2440/lowlevel_init.S`中定义如下：

```asm
45 #define BWSCON 0x48000000 		/* 13个存储控制器的开始地址 */
...									/*reigster definition*/
129 _TEXT_BASE:
130 .word TEXT_BASE
131
132 .globl lowlevel_init
133 lowlevel_init:
134 /* memory control configuration */
135 /* make r0 relative the current location so that it */
136 /* reads SMRDATA out of FLASH rather than memory ! */
137 ldr r0, =SMRDATA 		/*将SMRDATA的首地址(0x33F806C8)存到r0中*/
138 ldr r1, _TEXT_BASE		/*r1等于_TEXT_BASE内容，也就是TEXT_BASE(0x3ff80000)*/
139 sub r0, r0, r1 			/* SMRDATA减 _TEXT_BASE就是13个寄存器的偏移地址 */ 
140 ldr r1, =BWSCON 		/* Bus Width Status Controller */
141 add r2, r0, #13*4		/*每个寄存器4字节,r2=r0+13*4=最后一个存储器寄存器+4*/
142 0:
143 ldr r3, [r0], #4 		/*将r0的内容存到r3的内容中，同时r0地址+=4;*/
144 str r3, [r1], #4		/*将r3的内容存到r1所指的地址中,同时r1地址+=4;*/
145 cmp r2, r0				/*判断r2和r0*/
146 bne 0b					/*不等则跳转到第6行继续执行*/
147
148 /* everything is fine now */
149 mov pc, lr				/*返回*/
150
151 .ltorg
152 /* the literal pools origin */
153
SMRDATA:
    .word (0+(B1_BWSCON<<4)+(B2_BWSCON<<8)+(B3_BWSCON<<12)+(B4_BWSCON<<16)+(B5_BWSCON<<20)+(B6_BWSCON<<24)+(B7_BWSCON<<28))
    .word ((B0_Tacs<<13)+(B0_Tcos<<11)+(B0_Tacc<<8)+(B0_Tcoh<<6)+(B0_Tah<<4)+(B0_Tacp<<2)+(B0_PMC))
    .word ((B1_Tacs<<13)+(B1_Tcos<<11)+(B1_Tacc<<8)+(B1_Tcoh<<6)+(B1_Tah<<4)+(B1_Tacp<<2)+(B1_PMC))
    .word ((B2_Tacs<<13)+(B2_Tcos<<11)+(B2_Tacc<<8)+(B2_Tcoh<<6)+(B2_Tah<<4)+(B2_Tacp<<2)+(B2_PMC))
    .word ((B3_Tacs<<13)+(B3_Tcos<<11)+(B3_Tacc<<8)+(B3_Tcoh<<6)+(B3_Tah<<4)+(B3_Tacp<<2)+(B3_PMC))
    .word ((B4_Tacs<<13)+(B4_Tcos<<11)+(B4_Tacc<<8)+(B4_Tcoh<<6)+(B4_Tah<<4)+(B4_Tacp<<2)+(B4_PMC))
    .word ((B5_Tacs<<13)+(B5_Tcos<<11)+(B5_Tacc<<8)+(B5_Tcoh<<6)+(B5_Tah<<4)+(B5_Tacp<<2)+(B5_PMC))
    .word ((B6_MT<<15)+(B6_Trcd<<2)+(B6_SCAN))
    .word ((B7_MT<<15)+(B7_Trcd<<2)+(B7_SCAN))
    .word ((REFEN<<23)+(TREFMD<<22)+(Trp<<20)+(Trc<<18)+(Tchr<<16)+REFCNT)
    .word 0x32
    .word 0x30
    .word 0x30
/*lowlevel_init.o被链接后，SMRDATA的地址会被链接到0x33F806C8的地方，而文本段的起始地址TEXT_BASE是0x33f8000，取得13个寄存器的相对TEXT_BASE的偏移(此时还是运行在起始地址为0x0的地方，代码未被重定向到内存，所以需要取得寄存器相对0的偏移，再将13个控制寄存器的值设置到RAM的控制寄存器*/
```

`lowlevel_init`初始化了13个寄存器来实现RAM时钟的初始化。`lowlevel_init`函数对于U-Boot从`NAND Flash`或`NOR Flash`启动的情况都是有效的。

U-Boot.lds链接脚本有如下代码：

```asm
.text :
{
cpu/arm920t/start.o (.text)
board/samsung/mini2440/lowlevel_init.o (.text)
board/samsung/mini2440/nand_read.o (.text)
… …
}
```

`board/samsung/mini2440/lowlevel_init.o`将被链接到`cpu/arm920t/start.o`后面，因此`board/samsung/mini2440/lowlevel_init.o`也在U-Boot的前4KB的代码中。

U-Boot在NAND Flash启动时，`lowlevel_init.o`将自动被读取到CPU内部4KB的内部RAM中。因此第137~146行的代码将从CPU内部RAM中复制寄存器的值到相应的寄存器中。

对于U-Boot在NOR Flash启动的情况，由于U-Boot连接时确定的地址是U-Boot在内存中的地址，而此时U-Boot还在NOR Flash中，因此还需要在NOR Flash中读取数据到RAM 中。

由于NOR Flash的开始地址是0，而U-Boot的加载到内存的起始地址是TEXT_BASE，SMRDATA标号在NOR Flash的地址就是SMRDATA－TEXT_BASE

综上所述，lowlevel_init的作用就是将SMRDATA开始的13个值复制给开始地址[BWSCON]的13个寄存器，从而完成了存储控制器的设置。

#### 复制U-Boot第二阶段代码到RAM

```asm
/***************** CHECK_CODE_POSITION ******************************************/
	adr	r0, _start		/* r0 <- current position of code   */
	ldr	r1, _TEXT_BASE		/* test if we run from flash or RAM */
	cmp	r0, r1			/* don't reloc during debug         */
	beq	stack_setup
/***************** CHECK_CODE_POSITION ******************************************/
```

判断程序是从NandFlash启动还是NorFlash启动。`_TEXT_BASE`为0x33f80000，`_start`为当前代码运行的地址，如果相同，说明代码已经在SDRAM中，跳转初始化堆栈。

```asm
/***************** CHECK_BOOT_FLASH ******************************************/
/*如果将S3C2440配置成从NANDFLASH启动（将开发板的启动开关拔到nand端，此时OM0管脚拉低）S3C2440的Nand控制器会自动把Nandflash中的前4K代码数据搬到内部SRAM中(地址为0x40000000),同时还把这块SRAM地址映射到了0x00000000地址。 CPU从0x00000000位置开始运行程序。

如果将S3C2440配置成从Norflash启动（将开发的启动开关拔到nor端，此时OM0管脚拉高），0x00000000就是norflash实际的起始地址，norflash中的程序就从这里开始运行，不涉及到数据拷贝和地址映射*/
/*
在启动的时候，用程序将0x40000000～0x40001000中的某些位置清零，如果回读0x00000000～0x00001000中的相应位置后为零，说明是Nand boot，如果是原来的数据（一定要选非零的位置）就是Nor boot。但是最后有一点很重要：如果是Nand boot，必须要复原清零的数据。原因是：在nand boot过后，会核对内部SRAM中的4K程序，和从Nand中拷贝到SDRAM的前4K程序是否一致，如果不一致会进入死循环
*/
	ldr	r1, =( (4<<28)|(3<<4)|(3<<2) )		/* address of Internal SRAM  0x4000003C*/
	mov	r0, #0		/* r0 = 0 */
	str	r0, [r1] /* *r1 = 0*/


	mov	r1, #0x3c		/* address of men  0x0000003C*/
	ldr	r0, [r1]  /*r0 = *r1*/
	cmp	r0, #0 /*比较r0是否为0*/
	bne	relocate /*不为0，证明程序从NorFlash启动,跳转*/

	/* recovery  */
	/*是NandFlash启动*/
	ldr	r0, =(0xdeadbeef) /* r0 = 0xdeadbeef*/
	ldr	r1, =( (4<<28)|(3<<4)|(3<<2) )
	str	r0, [r1] /* *r1 = r0 */
	/*这里0x4000003C和0x0000003C的值应该都是0xdeadbeef,这是在start.S开头设置的：
	.balignl 16,0xdeadbeef
	*/
/***************** CHECK_BOOT_FLASH ******************************************/
```

##### NandFlash启动

NandFlash启动时，cpu会自动将的NandFlash的前4K代码拷贝到s3c2440内部的4K的RAM上，其物理地址为0x40000000

```asm
/***************** NAND_BOOT *************************************************/

#define LENGTH_UBOOT 0x60000
#define NAND_CTL_BASE 0x4E000000

#ifdef CONFIG_S3C2440
/* Offset */
#define oNFCONF 0x00
#define oNFCONT 0x04
#define oNFCMD 0x08
#define oNFSTAT 0x20

	@ reset NAND
	mov	r1, #NAND_CTL_BASE /*nandflash控制寄存器基地址*/
	/*结合CPU datasheet和Nandflash datasheet TACLS=1、TWRPH0=2、TWRPH1=0，8位IO */
    /* *(NFCONF) = (7 << 12) | (7 << 8) | (7 << 4) | (0 << 0)*/
	ldr	r2, =( (7<<12)|(7<<8)|(7<<4)|(0<<0) )
	str	r2, [r1, #oNFCONF] /* 设置NFCONF寄存器 */
	ldr	r2, [r1, #oNFCONF] /**/
	/* 设置NFCONT，初始化ECC编/解码器，禁止NAND Flash片选 */
	ldr	r2, =( (1<<4)|(0<<1)|(1<<0) )	@ Active low CE Control 
	str	r2, [r1, #oNFCONT]
	ldr	r2, [r1, #oNFCONT]
	 /* 设置NFSTAT 110 1：检测 RnB 传输    nCE 输出引脚的状态    0：NAND Flash 存储器忙 */
	ldr	r2, =(0x6)	@ RnB Clear
	str	r2, [r1, #oNFSTAT]
	ldr	r2, [r1, #oNFSTAT]
	/* 复位命令0xff写入命令寄存器，第一次使用NAND Flash前复位 */
	mov	r2, #0xff	@ RESET command
	strb	r2, [r1, #oNFCMD]
	
	mov	r3, #0	@ wait /*r3 = 0*/ 
nand1: 
	add	r3, r3, #0x1 /*r3 = r3 + 1*/
	cmp	r3, #0xa /* r3 == 0xa ?*/
	blt	nand1 /*r3 != 0xa -> goto nand1 ,just wait*/

nand2:
	ldr	r2, [r1, #oNFSTAT]	@ wait ready /*读取状态寄存器*/
	tst	r2, #0x4 /*测试r2的bit2是否为0*/
	beq	nand2 /*为0跳转到nand2继续等待*/
	
	
	ldr	r2, [r1, #oNFCONT] /*读取控制寄存器*/
	orr	r2, r2, #0x2	@ Flash Memory Chip Disable /*r2 = r2 | 0010b*/
	str	r2, [r1, #oNFCONT] /*设置控制寄存器，禁止片选*/
	
	@ get read to call C functions (for nand_read())
	ldr	sp, DW_STACK_START	@ setup stack pointer /*sp =DW_STACK_START 设置栈 */
	mov	fp, #0	@ no previous frame, so fp=0 

	@ copy U-Boot to RAM
	ldr	r0, =TEXT_BASE /*r0 = TEXT_BASE = 0x33f8000*/
	mov	r1, #0x0  /*r1 = 0*/
	mov	r2, #LENGTH_UBOOT /* r2 = LENGTH_UBOOT*/
	bl	nand_read_ll /*跳转执行nand_read_ll C函数，此函数会搬移u-boot到RAM*/
	tst	r0, #0x0 /*测试r0是否为0 */
	beq	ok_nand_read /*为0跳转到ok_nand_read*/

bad_nand_read:
loop2:
	b	loop2	@ infinite loop/*r0不为0，说明u-boot copy 到RAM失败，进入死循环*/
ok_nand_read: /*copy u-boot to RAM success*/
	@ verify
	mov	r0, #0 /*r0 = 0*/
	ldr	r1, =TEXT_BASE /*r1 = TEXT_BASE*/
	mov	r2, #0x400	@ 4 bytes * 1024 = 4K-bytes /*r2 = 4K-bytes*/
go_next:
	ldr	r3, [r0], #4 /*r3 = *r0; r0 += 4*/
	ldr	r4, [r1], #4 /*r4 = *r1; r1+= 4*/
	teq	r3, r4 /*r3 == r4 ? continue : goto notmatch*/
	bne	notmatch
	subs	r2, r2, #4 /* r2 = r2 - 4 */ 
	beq	stack_setup /* r2 == 0 ? stack_up : continue*/
	bne	go_next /*goto next*/

notmatch:
loop3:
	b	loop3	@ infinite loop /*notmatch 将会进入死循环*/
#endif
/***************** NAND_BOOT *************************************************/
```



```cpp
/*buf 目的缓冲区首地址, start_addr copy的源地址, size长度*/
int nand_read_ll(unsigned char *buf, unsigned long start_addr, int size)
{
	int i, j;
	unsigned short nand_id;
	struct boot_nand_t nand;

	/* chip Enable */
	nand_select(); /*使能片选*/
	nand_clear_RnB(); /*清除rnb*/
	
	for (i = 0; i < 10; i++)
		;/*wait*/
	nand_id = nand_read_id(); /*读取nandflash id*/
	if (0) { /* dirty little hack to detect if nand id is misread */
		unsigned short * nid = (unsigned short *)0x31fffff0;
		*nid = nand_id;
	}	
/*根据nandflash型号，设置nandflash参数*/
       if (nand_id == 0xec76 ||		/* Samsung K91208 */
           nand_id == 0xad76 ) {	/*Hynix HY27US08121A*/
		nand.page_size = 512;
		nand.block_size = 16 * 1024;
		nand.bad_block_offset = 5;
	//	nand.size = 0x4000000;
	} else if (nand_id == 0xecf1 ||	/* Samsung K9F1G08U0B */
		   nand_id == 0xecda ||	/* Samsung K9F2G08U0B */
		   nand_id == 0xecd3 )	{ /* Samsung K9K8G08 */
		nand.page_size = 2048;
		nand.block_size = 128 * 1024;
		nand.bad_block_offset = nand.page_size;
	//	nand.size = 0x8000000;
	} else {
		return -1; // hang
	}
    /*start_addr和size在nandflash上必须是块对齐的*/
	if ((start_addr & (nand.block_size-1)) || (size & ((nand.block_size-1))))
		return -1;	/* invalid alignment */

	for (i=start_addr; i < (start_addr + size);) {/*开始拷贝*/
#ifdef CONFIG_S3C2410_NAND_SKIP_BAD
		if (i & (nand.block_size-1)== 0) {/*拷贝时切换到新块，需要判断是否是坏块*/
			if (is_bad_block(&nand, i) ||
			    is_bad_block(&nand, i + nand.page_size)) {//是坏块需要跳过
				/* Bad block */
				i += nand.block_size;
				size += nand.block_size;
				continue;
			}
		}
#endif
		j = nand_read_page_ll(&nand, buf, i);//拷贝i地址的数据到buf，拷贝长度为j
		i += j;
		buf += j;
	}

	/* chip Disable */
	nand_deselect();//禁止nandflash 片选

	return 0;
}

```

##### Norflash启动

`NorFlash`启动时，代码直接在`norflash`里运行，此时程序运行的起始地址`_start=_armboot_start=0x0`

需要将`_start`开始的代码段和数据段拷贝到`TEXT_BASE=0x33f80000`为起始的地址处，根据`u-boot.lds`文件·，可以看出代码段和数据段的长度就是`_bss_start - _start`

```asm
loop3:
	b	loop3	@ infinite loop
/***************** NOR_BOOT *************************************************/
relocate:				/* relocate U-Boot to RAM	    */
      /*********** CHECK_FOR_MAGIC_NUMBER***************/
	ldr	r1, =(0xdeadbeef)
	cmp	r0, r1 /*r0 = *(0x3c)*/
	bne	loop3 /*r0 != r1，start.S中0x3c应该是被写入了0xdeadbeff的，这里norflash数据肯定有问题，直接进入死循环*/
      /*********** CHECK_FOR_MAGIC_NUMBER***************/
	adr	r0, _start		/* r0 <- current position of code   *//*nandflash 0x33f80000 ,norflash : 0*/
	ldr	r1, _TEXT_BASE	/*0x33f80000*/	/* test if we run from flash or RAM */
	ldr	r2, _armboot_start /* 0x33f80000,这里_start = _armboot_start,但是adr和ldr是有区别的，adr取得相对地址，而ldr取得绝对地址也就是链接的地址,ldr取得的值不会变，但是adr 取得值如果当前代码运行在Norflash,值将是0，如果是在SDRAM中，值会是0x33f80000*/
	ldr	r3, _bss_start /*defined in u-boot.lds*/
	sub	r2, r3, r2		/* r2 <- size of armboot            */
	add	r2, r0, r2		/* r2 <- source end address         */

copy_loop:
	ldmia	r0!, {r3-r10}		/* copy from source address [r0]    */
	/*将r0所指地址开始的数据拷贝到r3-r10,每次拷贝r0会自增*/
	stmia	r1!, {r3-r10}		/* copy to   target address [r1]    */
	/*将r3-r10所指地址的数据拷贝到r1所指地址开始的位置，每次拷贝r1自增*/
	cmp	r0, r2			/* until source end addreee [r2]    */
	/*r0 == r2时，证明数据已经拷贝完成，否则继续循环*/
	ble	copy_loop
/***************** NOR_BOOT *************************************************/
```

#### 设置堆栈

```asm
/* Set up the stack						    */
stack_setup:
	ldr	r0, _TEXT_BASE		/* upper 128 KiB: relocated uboot   */
	sub	r0, r0, #CONFIG_SYS_MALLOC_LEN	/* malloc area              */
	sub	r0, r0, #CONFIG_SYS_GBL_DATA_SIZE /* bdinfo                 */
#ifdef CONFIG_USE_IRQ
	sub	r0, r0, #(CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ)
#endif
	sub	sp, r0, #12		/* leave 3 words for abort-stack    */
/*sp = _TEXT_BASE - CONFIG_SYS_MALLOC_LEN - CONFIG_SYS_GBL_DATA_SIZE - (CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ) - 12,sp寄存器指向栈顶，所以堆栈在(CONFIG_STACKSIZE_IRQ+CONFIG_STACKSIZE_FIQ) - 12下面*/
clear_bss:
/*清理bss段，在TEXT_BASE上面，中间还有u-boot的代码段和数据段（前面重定向过来的），bss段放置未初始化和初始化为0的数据,数据段为初始化为非0的数据*/
	ldr	r0, _bss_start		/* find start of bss segment        */
	ldr	r1, _bss_end		/* stop here                        */
	mov	r2, #0x00000000		/* clear                            */

clbss_l:
	str	r2, [r0]		/* clear loop...                    */
	add	r0, r0, #4
	cmp	r0, r1
	ble	clbss_l
	/*跳转到第二阶段代码入口,其中_start_armboot:	.word start_armboot*/
	ldr	pc, _start_armboot
```

## u-boot启动第二阶段分析



### start_armboot

`start_armboot`定义在`lib_arm/board.c`

```cpp
gd = (gd_t*)(_armboot_start - CONFIG_SYS_MALLOC_LEN - sizeof(gd_t));
__asm__ __volatile__("": : :"memory");/*memory强制gcc编译器假设RAM所有内存单元均被汇编指令修改，这样cpu中的registers和cache中已缓存的内存单元中的数据将作废。cpu将不得不在需要的时候重新读取内存中的数据。这就阻止了cpu又将registers，cache中的数据用于去优化指令，而避免去访问内存。*/

	memset ((void*)gd, 0, sizeof (gd_t));
	gd->bd = (bd_t*)((char*)gd - sizeof(bd_t));
	memset (gd->bd, 0, sizeof (bd_t));

	gd->flags |= GD_FLG_RELOC;

	monitor_flash_len = _bss_start - _armboot_start;//u-boot映像长度

```

#### gd_t/bd_t

`gd`是一个`struct gd_t`类型的全局变量，被放在`CONFIG_SYS_GBL_DATA`区域

```cpp
typedef	struct	global_data {
	bd_t		*bd;
	unsigned long	flags;
	unsigned long	baudrate;
	unsigned long	have_console;	/* serial_init() was called */
	unsigned long	env_addr;	/* Address  of Environment struct */
	unsigned long	env_valid;	/* Checksum of Environment valid? */
	unsigned long	fb_base;	/* base address of frame buffer */
#ifdef CONFIG_VFD
	unsigned char	vfd_type;	/* display type */
#endif
#if 0
	unsigned long	cpu_clk;	/* CPU clock in Hz!		*/
	unsigned long	bus_clk;
	phys_size_t	ram_size;	/* RAM size */
	unsigned long	reset_status;	/* reset status register at boot */
#endif
	void		**jt;		/* jump table */
} gd_t;

typedef struct bd_info {
    int			bi_baudrate;	/* serial console baudrate */
    unsigned long	bi_ip_addr;	/* IP Address */
    struct environment_s	       *bi_env;
    ulong	        bi_arch_number;	/* unique id for this board */
    ulong	        bi_boot_params;	/* where this board expects params */
    struct				/* RAM configuration */
    {
	ulong start;
	ulong size;
    }			bi_dram[CONFIG_NR_DRAM_BANKS];
} bd_t;/*U-Boot启动内核时要给内核传递参数，这时就要使用gd_t，bd_t结构体中的信息来设置标记列表。*/
```

U-Boot使用了一个存储在寄存器中的指针gd来记录全局数据区的地址：

```cpp
#define DECLARE_GLOBAL_DATA_PTR     register volatile gd_t *gd asm ("r8")
```

`DECLARE_GLOBAL_DA TA_PTR`定义一个`gd_t`全局数据结构的指针，这个指针存放在指定的寄存器`r8`中。这个声明也避免编译器把`r8`分配给其它的变量。任何想要访问全局数据区的代码，只要代码开头加入`DECLARE_GLOBAL_DATA_PTR`一行代码，然后就可以使用`gd`指针来访问全局数据区了。

#### init_sequence初始化

调用`init_sequence`数组中的函数进行初始化，任何一个函数执行失败都会导致`u-boot`阻塞停止执行

```cpp
for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}
```

init_sequence数组如下：

```cpp
init_fnc_t *init_sequence[] = {
#if defined(CONFIG_ARCH_CPU_INIT)
	arch_cpu_init,		/* basic arch cpu dependent setup */
#endif
	board_init,		/* basic board dependent setup */
#if defined(CONFIG_USE_IRQ)
	interrupt_init,		/* set up exceptions */
#endif
	timer_init,		/* initialize timer */
	env_init,		/* initialize environment */
	init_baudrate,		/* initialze baudrate settings */
	serial_init,		/* serial communications setup */
	console_init_f,		/* stage 1 init of console */
	display_banner,		/* say that we are here */
#if defined(CONFIG_DISPLAY_CPUINFO)
	print_cpuinfo,		/* display cpu info (and speed) */
#endif
#if defined(CONFIG_DISPLAY_BOARDINFO)
	checkboard,		/* display board info */
#endif
#if defined(CONFIG_HARD_I2C) || defined(CONFIG_SOFT_I2C)
	init_func_i2c,
#endif
	dram_init,		/* configure available RAM banks */
#if defined(CONFIG_CMD_PCI) || defined (CONFIG_PCI)
	arm_pci_init,
#endif
	display_dram_config,
	NULL,
};
```

##### board_init

再次对系统时钟初始化，初始化`gpio`，初始化`icache,dcache`，初始化`bd_t`,初始化`u-boot`参数在内存的地址为`0x30000100`，`machid = 1999`

##### timer_init

初始化定时器

##### env_init

读取存储在`nandflash`的环境变量，用于初始化`gd->env_addr`，如果`nandflash`校验异常，则使用默认环境变量

##### init_baudrate

从环境变量获取波特率保存到`gd->bd->bi_baudrate`和`gd->baudrate`

##### serial_init

初始化串口

##### console_init_f

初始化控制台`gd->have_console = 1`

##### display_banner

打印欢迎信息，调试信息，前面初始化了串口和控制台。但是还并没有重定向输出，这个显示的信息是无法显示的。

##### dram_init

初始化可用的SDRAM的起始物理地址和大小

```cpp
gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
```

在include/configs/mini2440.h中PHYS_SDRAM_1和PHYS_SDRAM_1_SIZE 分别被定义为0x30000000和0x04000000（64M）。

`mini2440`使用2片`32MB`的`SDRAM`组成了`64MB`的内存，接在存储控制器的`BANK6`，地址空间是`0x30000000~0x34000000`。在`include/configs/mini2440.h`中`PHYS_SDRAM_1`和`PHYS_SDRAM_1_SIZE `分别被定义为`0x30000000`和`0x04000000（64M）`。

##### display_dram_config

打印显示RAM大小

#### NorFlash初始化

```cpp
display_flash_config (flash_init ());
```

初始化并打印NorFlash的大小

#### NandFlash初始化

初始化NandFlash并打印NandFlash大小

```cpp
puts ("NAND:  ");
nand_init();		/* go init the NAND */
```

#### 初始化环境变量

```cpp
/* initialize environment */
	env_relocate ();
```

读取`NandFlash`上保存的环境变量，如果`NandFlash`上没有或者读取后校验`crc`错误，则使用默认环境变量。

#### 从环境变量中获取IP地址

```cpp
gd->bd->bi_ip_addr = getenv_IPaddr ("ipaddr");
```

#### 初始化一些输入设备

```cpp
stdio_init ();	/* get the devices list going. */
```

主要函数是：

```cpp
i2c_init();
drv_video_init();
drv_system_init();
```

其中`drv_system_init`函数会注册读写串口的回调函数。

#### 初始化jumptable

```cpp
jumptable_init();
```

#### 重定向标准输入输出和错误到串口

```cpp
console_init_r();
```

`console_init_r`指定标准输入标准输出和标准出错为串口，并为重写的`printf fputc putc getc...`函数绑定之前注册的读写串口回调函数。

#### 使能中断

```cpp
enable_interrupts();
```

#### 初始化中断和USB

```cpp
usb_init_slave();
```

#### 进入main_loop

```cpp
for (;;) {
		main_loop ();
	}
```

## 启动内核

### `main_loop`函数分析

```cpp
/*手动注释掉了未定义的宏或者关系不大的宏*/
void main_loop (void)
{
//...

	s = getenv ("bootdelay");//获取bootdelay环境变量,bootdelay是u-boot启动后再启动Linux内核之间的延时，期间输入指定字符可以打断内核启动
	bootdelay = s ? (int)simple_strtol(s, NULL, 10) : CONFIG_BOOTDELAY;

	debug ("### main_loop entered: bootdelay=%d\n\n", bootdelay);
//...
		s = getenv ("bootcmd");//获取bootcmd环境变量
    //bootcmd eg:bootcmd:nand read 0x30008000 0x1000000 0x5000000; bootm 30008000

	debug ("### main_loop: bootcmd=\"%s\"\n", s ? s : "<UNDEFINED>");

	if (bootdelay >= 0 && s && !abortboot (bootdelay)) {
/*abortboot 函数再在bootdelay时间内检测输入，如果有任何输入，则abort，返回1，此时进入u-boot命令行而不执行run_command函数。如果在bootdelay时间内没有输入，会自动执行run_command函数，此函数执行bootcmd环境变量*/
		run_command (s, 0);
	}
//以下是u-boot启动内核被打断后，u-boot命令行下的交互操作
    for (;;) {
		len = readline (CONFIG_SYS_PROMPT);//读取输入，底层还是读取串口输入

		flag = 0;	/* assume no special flags for now */
		if (len > 0)
			strcpy (lastcommand, console_buffer);
		else if (len == 0)
			flag |= CMD_FLAG_REPEAT;

		if (len == -1)
			puts ("<INTERRUPT>\n");//输入不匹配，打印<INTERRUPT>，命令行下的显示效果就是<INTERRPUT>
		else
			rc = run_command (lastcommand, flag);//从lastcommand提取命令并执行,run_command支持一行输入多条命令 ';'分隔

		if (rc <= 0) {
			/* invalid command or not repeatable, forget it */
			lastcommand[0] = 0;
		}
	}//循环读取命令并执行
}
```

内核启动在不被打断的情况下，会执行环境变量中的bootcmd环境变量。bootcmd通常会执行bootm命令

### bootm

`common/cmd_bootm.c`

```cpp
U_BOOT_CMD(
	bootm,	CONFIG_SYS_MAXARGS,	1,	do_bootm,
	"boot application image from memory",
	"[addr [arg ...]]\n    - boot application image stored in memory\n"
	"\tpassing arguments 'arg ...'; when booting a Linux kernel,\n"
	"\t'arg' can be the address of an initrd image\n"
	"\nSub-commands to do part of the bootm sequence.  The sub-commands "
	"must be\n"
	"issued in the order below (it's ok to not issue all sub-commands):\n"
	"\tstart [addr [arg ...]]\n"
	"\tloados  - load OS image\n"
	"\tcmdline - OS specific command line processing/setup\n"
	"\tbdt     - OS specific bd_t processing\n"
	"\tprep    - OS specific prep before relocation or go\n"
	"\tgo      - start OS"
);
```

#### U_BOOT_CMD

`include/command.h`

```cpp
#define U_BOOT_CMD(name,maxargs,rep,cmd,usage,help) \
cmd_tbl_t __u_boot_cmd_##name Struct_Section = {#name, maxargs, rep, cmd, usage}

#define Struct_Section  __attribute__ ((unused,section (".u_boot_cmd")))

/*凡是带有attribute ((unused,section (“.u_boot_cmd”))属性声明的变量都将被存放在”.u_boot_cmd”段中，并且即使该变量没有在代码中显式的使用编译器也不产生警告信息。*/


/*结构体*/
struct cmd_tbl_s {
	char		*name;		/* Command Name			*/
	int		maxargs;	/* maximum number of arguments	*/
	int		repeatable;	/* autorepeat allowed?		*/
					/* Implementation function	*/
	int		(*cmd)(struct cmd_tbl_s *, int, int, char *[]);
	char		*usage;		/* Usage message	(short)	*/
#ifdef	CONFIG_SYS_LONGHELP
	char		*help;		/* Help  message	(long)	*/
#endif
#ifdef CONFIG_AUTO_COMPLETE
    int		(*complete)(int argc, char *argv[], char last_char, int maxv, char *cmdv[]);//命令自动补全
#endif
};
/*bootm命令在.u_boot_cmd段中呈现为一个结构体*/
cmd_tbl_t __u_boot_cmd_bootm
{
    bootm,
    CONFIG_SYS_MAXARGS,
    1,
    do_bootm,
    "short help msg",/*简单的帮助信息*/
    "long help msg",/*详细的帮助信息*/
}
```

`u-boot.lds`

```lds
. = .;
__u_boot_cmd_start = .;
.u_boot_cmd : { *(.u_boot_cmd) }
__u_boot_cmd_end = .;
```

这表明带有`.u_boot_cmd`声明的函数或变量将存储在`u_boot_cmd`段。

这样只要将u-boot所有命令对应的`cmd_tbl_t`变量加上`.u_boot_cmd`声明，编译器就会自动将其放在`u_boot_cmd`段，查找`cmd_tbl_t`变量时只要在 `__u_boot_cmd_start` 与` __u_boot_cmd_end `之间查找就可以了。

```cpp
s = getenv ("bootcmd");
...
run_command (s, 0);
...
```

#### run_command

```cpp
//cmd 对应命令如bootm ,set, saveenv...
/*eg : bootcmd=nand read 30008000 100000 500000; bootm 30008000*/
int run_command (const char *cmd, int flag)
{
	cmd_tbl_t *cmdtp;
	char cmdbuf[CONFIG_SYS_CBSIZE];	/* working copy of cmd		*/
	char *token;			/* start of token in cmdbuf	*/
	char *sep;			/* end of token (separator) in cmdbuf */
	char finaltoken[CONFIG_SYS_CBSIZE];
	char *str = cmdbuf;
	char *argv[CONFIG_SYS_MAXARGS + 1];	/* NULL terminated	*/
	int argc, inquotes;
	int repeatable = 1;
	int rc = 0;

	clear_ctrlc();		/* forget any previous Control C */

	if (!cmd || !*cmd) {
		return -1;	/* empty command */
	}

	if (strlen(cmd) >= CONFIG_SYS_CBSIZE) {
		puts ("## Command too long!\n");
		return -1;
	}

	strcpy (cmdbuf, cmd);

	/* Process separators and check for invalid
	 * repeatable commands
	 */

	while (*str) {

		/*
		 * Find separator, or string end
		 * Allow simple escape of ';' by writing "\;"
		 */
		for (inquotes = 0, sep = str; *sep; sep++) {
			if ((*sep=='\'') &&
			    (*(sep-1) != '\\'))
				inquotes=!inquotes;
/*根据;分割多条命令，一条一条处理*/
			if (!inquotes &&
			    (*sep == ';') &&	/* separator		*/
			    ( sep != str) &&	/* past string start	*/
			    (*(sep-1) != '\\'))	/* and NOT escaped	*/
				break;
		}

		/*
		 * Limit the token to data between separators
		 */
		token = str;
		if (*sep) {
			str = sep + 1;	/* start of command for next pass */
			*sep = '\0';
		}
		else
			str = sep;	/* no more commands for next pass */
/*命令中可能包含其他环境变量，如xxx ${EVN1} $(ENV2)...,这里将其他环境变量解析成具体命令*/
		/* find macros in this token and replace them */
		process_macros (token, finaltoken);
/*将命令解析到argv[]数据，其中argv[0]是命令 argv[1-n]是参数，返回argc是命令+参数总个数*/
		/* Extract arguments */
		if ((argc = parse_line (finaltoken, argv)) == 0) {
			rc = -1;	/* no command at all */
			continue;
		}
/*根据命令名bootm在__u_boot_cmd_start和__u_boot_cmd_end之间查找命令，返回cmd_tbl_t结构指针，找不到返回NULL，并打印Unknown command '%s' - try 'help' */
		/* Look up command in command table */
		if ((cmdtp = find_cmd(argv[0])) == NULL) {
			printf ("Unknown command '%s' - try 'help'\n", argv[0]);
			rc = -1;	/* give up after bad command */
			continue;
		}

		/* found - check max args */
		if (argc > cmdtp->maxargs) {
			cmd_usage(cmdtp);
			rc = -1;
			continue;
		}

		/* OK - call function to do the command */
        /*执行命令*/
		if ((cmdtp->cmd) (cmdtp, flag, argc, argv) != 0) {
			rc = -1;
		}

		repeatable &= cmdtp->repeatable;

		/* Did the user stop this? */
		if (had_ctrlc ())
			return -1;	/* if stopped then not repeatable */
	}

	return rc ? rc : repeatable;
}

cmd_tbl_t *find_cmd (const char *cmd)
{
	int len = &__u_boot_cmd_end - &__u_boot_cmd_start;
	return find_cmd_tbl(cmd, &__u_boot_cmd_start, len);
}

cmd_tbl_t *find_cmd_tbl (const char *cmd, cmd_tbl_t *table, int table_len)
{
	cmd_tbl_t *cmdtp;
	cmd_tbl_t *cmdtp_temp = table;	/*Init value */
	const char *p;
	int len;
	int n_found = 0;

	/*
	 * Some commands allow length modifiers (like "cp.b");
	 * compare command name only until first dot.
	 */
	len = ((p = strchr(cmd, '.')) == NULL) ? strlen (cmd) : (p - cmd);

	for (cmdtp = table;
	     cmdtp != table + table_len;
	     cmdtp++) {
		if (strncmp (cmd, cmdtp->name, len) == 0) {
			if (len == strlen (cmdtp->name))
				return cmdtp;	/* full match */

			cmdtp_temp = cmdtp;	/* abbreviated command ? */
			n_found++;
		}
	}
	if (n_found == 1) {			/* exactly one match */
		return cmdtp_temp;
	}

	return NULL;	/* not found or ambiguous command */
}
```



#### do_bootm

```cpp
int do_bootm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong		iflag;
	ulong		load_end = 0;
	int		ret;
	boot_os_fn	*boot_fn;
	static int relocated = 0;

	/* relocate boot function table */
	if (!relocated) {
		int i;
		for (i = 0; i < ARRAY_SIZE(boot_os); i++)
			if (boot_os[i] != NULL)
				boot_os[i] += gd->reloc_off;/*boot_os保存函数指针，如果还未重定向，则先重定向，即+重定向偏移*/
		relocated = 1;
	}

	/* determine if we have a sub command */
	if (argc > 1) {//bootm 30008000;argv[0] = bootm ;argv[1] = 30008000;argc = 2
		char *endp;

		simple_strtoul(argv[1], &endp, 16);/*strtoul: string to unsigned long, base=16代表argv[1]为十六进制，这里将十六进制字符串转换为长整数并返回,即0x30008000转为长整*/
		/* endp pointing to NULL means that argv[1] was just a
		 * valid number, pass it along to the normal bootm processing
		 *
		 * If endp is ':' or '#' assume a FIT identifier so pass
		 * along for normal processing.
		 *
		 * Right now we assume the first arg should never be '-'
		 */
		if ((*endp != 0) && (*endp != ':') && (*endp != '#'))/*endp指向的是第一个非0-9,A-F,a-f的数即不满足十六进制的数，此处30008000后啥也没有，endp指向为0*/
			return do_bootm_subcommand(cmdtp, flag, argc, argv);/*存在子命令，进入处理子命令*/
	}

	if (bootm_start(cmdtp, flag, argc, argv))
		return 1;

	/*
	 * We have reached the point of no return: we are going to
	 * overwrite all exception vector code, so we cannot easily
	 * recover from any failures any more...
	 */
	iflag = disable_interrupts();//禁用中断

#if defined(CONFIG_CMD_USB)
	/*
	 * turn off USB to prevent the host controller from writing to the
	 * SDRAM while Linux is booting. This could happen (at least for OHCI
	 * controller), because the HCCA (Host Controller Communication Area)
	 * lies within the SDRAM and the host controller writes continously to
	 * this area (as busmaster!). The HccaFrameNumber is for example
	 * updated every 1 ms within the HCCA structure in SDRAM! For more
	 * details see the OpenHCI specification.
	 */
	usb_stop();
#endif

#ifdef CONFIG_AMIGAONEG3SE
	/*
	 * We've possible left the caches enabled during
	 * bios emulation, so turn them off again
	 */
	icache_disable();/*禁用cache*/
	dcache_disable();
#endif

	ret = bootm_load_os(images.os, &load_end, 1);//解压内核到images.os.load,结束地址为load_end

	if (ret < 0) {
		if (ret == BOOTM_ERR_RESET)
			do_reset (cmdtp, flag, argc, argv);
		if (ret == BOOTM_ERR_OVERLAP) {
			if (images.legacy_hdr_valid) {
				if (image_get_type (&images.legacy_hdr_os_copy) == IH_TYPE_MULTI)
					puts ("WARNING: legacy format multi component "
						"image overwritten\n");
			} else {
				puts ("ERROR: new format image overwritten - "
					"must RESET the board to recover\n");
				show_boot_progress (-113);
				do_reset (cmdtp, flag, argc, argv);
			}
		}
		if (ret == BOOTM_ERR_UNIMPLEMENTED) {
			if (iflag)
				enable_interrupts();
			show_boot_progress (-7);
			return 1;
		}
	}

	lmb_reserve(&images.lmb, images.os.load, (load_end - images.os.load));

	if (images.os.type == IH_TYPE_STANDALONE) {
		if (iflag)
			enable_interrupts();
		/* This may return when 'autostart' is 'no' */
		bootm_start_standalone(iflag, argc, argv);
		return 0;
	}

	show_boot_progress (8);


	boot_fn = boot_os[images.os.os];//获取内核启动回调函数，索引是images.os.os，操作系统类型为Linux

	if (boot_fn == NULL) {
		if (iflag)
			enable_interrupts();
		printf ("ERROR: booting os '%s' (%d) is not supported\n",
			genimg_get_os_name(images.os.os), images.os.os);
		show_boot_progress (-8);
		return 1;
	}

	arch_preboot_os();//禁用所有异步中断

	boot_fn(0, argc, argv, &images);//启动内核

	show_boot_progress (-9);

	do_reset (cmdtp, flag, argc, argv);//boot_fn函数是不会返回到这里的，到了这里证明内核启动失败，此函数会reset系统

	return 1;
}
```

#### bootm_start

```cpp
static int bootm_start(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	ulong		mem_start;
	phys_size_t	mem_size;
	void		*os_hdr;
	int		ret;

	memset ((void *)&images, 0, sizeof (images));/*清空image*/
	images.verify = getenv_yesno ("verify");

	lmb_init(&images.lmb);

	mem_start = getenv_bootm_low();
	mem_size = getenv_bootm_size();

	lmb_add(&images.lmb, (phys_addr_t)mem_start, mem_size);

	arch_lmb_reserve(&images.lmb);
	board_lmb_reserve(&images.lmb);

	/* get kernel image header, start address and length */
	os_hdr = boot_get_kernel (cmdtp, flag, argc, argv,
			&images, &images.os.image_start, &images.os.image_len);/*获取内核在SDRAM中的地址和长度*/
	if (images.os.image_len == 0) {
		puts ("ERROR: can't get kernel image!\n");
		return 1;
	}

	/* get image parameters */
	switch (genimg_get_format (os_hdr)) {
	case IMAGE_FORMAT_LEGACY:
		images.os.type = image_get_type (os_hdr);//内核类型
		images.os.comp = image_get_comp (os_hdr);//压缩类型
		images.os.os = image_get_os (os_hdr);//操作系统类型LINUX

		images.os.end = image_get_image_end (os_hdr);
		images.os.load = image_get_load (os_hdr);//内核加载地址
		break;
	default:
		puts ("ERROR: unknown image format type!\n");
		return 1;
	}

	/* find kernel entry point */
	if (images.legacy_hdr_valid) {
		images.ep = image_get_ep (&images.legacy_hdr_os_copy);//获取内核入口地址
	} else {
		puts ("Could not find kernel entry point!\n");
		return 1;
	}

	if (images.os.os == IH_OS_LINUX) {
		/* find ramdisk */
		ret = boot_get_ramdisk (argc, argv, &images, IH_INITRD_ARCH,
				&images.rd_start, &images.rd_end);
		if (ret) {
			puts ("Ramdisk image is corrupt or invalid\n");
			return 1;
		}

#if defined(CONFIG_OF_LIBFDT)
#if defined(CONFIG_PPC) || defined(CONFIG_M68K) || defined(CONFIG_SPARC)
		/* find flattened device tree */
		ret = boot_get_fdt (flag, argc, argv, &images,
				    &images.ft_addr, &images.ft_len);
		if (ret) {
			puts ("Could not find a valid device tree\n");
			return 1;
		}

		set_working_fdt_addr(images.ft_addr);
#endif
#endif
	}

	images.os.start = (ulong)os_hdr;
	images.state = BOOTM_STATE_START;

	return 0;
}
```

#### boot_get_kernel

```cpp
static void *boot_get_kernel (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[],
		bootm_headers_t *images, ulong *os_data, ulong *os_len)
{
	image_header_t	*hdr;
	ulong		img_addr;

	/* find out kernel image address */
	if (argc < 2) {
		img_addr = load_addr;
		debug ("*  kernel: default image load address = 0x%08lx\n",
				load_addr);//bootm没有参数就使用默认地址寻找内核
	} else {
		img_addr = simple_strtoul(argv[1], NULL, 16);//返回30008000 string->hex后返回
		debug ("*  kernel: cmdline image address = 0x%08lx\n", img_addr);//这行启动内核时会打印
	}

	show_boot_progress (1);

	/* copy from dataflash if needed */
	img_addr = genimg_get_image (img_addr);/*检查内核是否在flash中，如何在，就将内核拷贝到ram中，并返回ram中的起始地址*/

	/* check image type, for FIT images get FIT kernel node */
	*os_data = *os_len = 0;
	switch (genimg_get_format ((void *)img_addr)) {/*获取并返回内核的类型*/
	case IMAGE_FORMAT_LEGACY:
		printf ("## Booting kernel from Legacy Image at %08lx ...\n",
				img_addr);
		hdr = image_get_kernel (img_addr, images->verify);//校验内核是否正确，返回内核头
		if (!hdr)
			return NULL;
		show_boot_progress (5);

		/* get os_data and os_len */
		switch (image_get_type (hdr)) {
		case IH_TYPE_KERNEL:
			*os_data = image_get_data (hdr);//内核data地址
			*os_len = image_get_data_size (hdr);//内核大小
			break;
		case IH_TYPE_MULTI:
			image_multi_getimg (hdr, 0, os_data, os_len);
			break;
		case IH_TYPE_STANDALONE:
			if (argc >2) {
				hdr->ih_load = htonl(simple_strtoul(argv[2], NULL, 16));
			}
			*os_data = image_get_data (hdr);
			*os_len = image_get_data_size (hdr);
			break;
		default:
			printf ("Wrong Image Type for %s command\n", cmdtp->name);
			show_boot_progress (-5);
			return NULL;
		}

		/*
		 * copy image header to allow for image overwrites during kernel
		 * decompression.
		 */
		memmove (&images->legacy_hdr_os_copy, hdr, sizeof(image_header_t));//copy内核头

		/* save pointer to image header */
		images->legacy_hdr_os = hdr;//保存指针

		images->legacy_hdr_valid = 1;
		show_boot_progress (6);
		break;
	default:
		printf ("Wrong Image Format for %s command\n", cmdtp->name);
		show_boot_progress (-108);
		return NULL;
	}

	debug ("   kernel data at 0x%08lx, len = 0x%08lx (%ld)\n",
			*os_data, *os_len, *os_len);

	return (void *)img_addr;//返回内核地址
}
```



#### boot_os

```cpp
static boot_os_fn do_bootm_integrity;
#endif

static boot_os_fn *boot_os[] = {
#ifdef CONFIG_BOOTM_LINUX
	[IH_OS_LINUX] = do_bootm_linux, // for Linux
#endif
#ifdef CONFIG_BOOTM_NETBSD
	[IH_OS_NETBSD] = do_bootm_netbsd,
#endif
#ifdef CONFIG_LYNXKDI
	[IH_OS_LYNXOS] = do_bootm_lynxkdi,
#endif
#ifdef CONFIG_BOOTM_RTEMS
	[IH_OS_RTEMS] = do_bootm_rtems,
#endif
#if defined(CONFIG_CMD_ELF)
	[IH_OS_VXWORKS] = do_bootm_vxworks,
	[IH_OS_QNX] = do_bootm_qnxelf,
#endif
#ifdef CONFIG_INTEGRITY
	[IH_OS_INTEGRITY] = do_bootm_integrity,
#endif
};
```

#### do_bootm_linux

```cpp
int do_bootm_linux(int flag, int argc, char *argv[], bootm_headers_t *images)
{
	bd_t	*bd = gd->bd;
	char	*s;
	int	machid = bd->bi_arch_number;
	void	(*theKernel)(int zero, int arch, uint params);

#ifdef CONFIG_CMDLINE_TAG
	char *commandline = getenv ("bootargs");//获取bootargs环境变量
#endif

	if ((flag != 0) && (flag != BOOTM_STATE_OS_GO))
		return 1;

	theKernel = (void (*)(int, int, uint))images->ep;//获取内核启动地址

	s = getenv ("machid");//获取machid环境变量
	if (s) {
		machid = simple_strtoul (s, NULL, 16);//转换为十六进制
		printf ("Using machid 0x%x from environment\n", machid);
	}

	show_boot_progress (15);

	debug ("## Transferring control to Linux (at address %08lx) ...\n",
	       (ulong) theKernel);

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG) || \
    defined (CONFIG_LCD) || \
    defined (CONFIG_VFD)
	setup_start_tag (bd);//设置传给内核的参数，标记参数链表头,参数保存在bd->bi_boot_params
#ifdef CONFIG_SERIAL_TAG
	setup_serial_tag (&params);
#endif
#ifdef CONFIG_REVISION_TAG
	setup_revision_tag (&params);
#endif
#ifdef CONFIG_SETUP_MEMORY_TAGS
	setup_memory_tags (bd);
#endif
#ifdef CONFIG_CMDLINE_TAG
	setup_commandline_tag (bd, commandline);
#endif
#ifdef CONFIG_INITRD_TAG
	if (images->rd_start && images->rd_end)
		setup_initrd_tag (bd, images->rd_start, images->rd_end);
#endif
#if defined (CONFIG_VFD) || defined (CONFIG_LCD)
	setup_videolfb_tag ((gd_t *) gd);
#endif
	setup_end_tag (bd);//标记参数链表尾
#endif

	/* we assume that the kernel is in place */
	printf ("\nStarting kernel ...\n\n");

//#ifdef CONFIG_USB_DEVICE
#if 0
	{
		extern void udc_disconnect (void);
		udc_disconnect ();
	}
#endif

	cleanup_before_linux ();//禁中断，关cache

	theKernel (0, machid, bd->bi_boot_params);//启动内核，传入machid和参数链表
	/* does not return */
/*此函数不会返回*/
	return 1;
}
```