# syscall-glibc_to_kernel

Linux Kernel:4.9.37

glibc:2.24

platform:arm32

## 系统调用

系统调用(syscall)是Linux内核提供的一套底层API供用户层调用实现内核的访问。用户层通过系统调用陷入内核，系统调用本质上是一种软中断，通过软中断指令陷入内核执行对应的中断处理程序并返回完成一次系统调用。linux内核中断初始化过程中会初始化中断向量表，其中0x80号中断向量用于系统调用。

![image-20230424005355905](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005355905.png)

### glibc->kernel

#### 1.系统调用(函数)->glibc->swi软中断

用户层通过glibc库函数间接调用系统调用（实际使用库函数几乎都是经过glibc调用到内核的，man 2手册的系统调用如read/write/socket实际也是经过glibc的）

以read系统调用为例子，用户层调用read系统调用，实际会先调用到glibc

```cpp
/* glibc/sysdeps/unix/sysv/linux/read.c  */
#include <unistd.h>
#include <sysdep-cancel.h>

/* Read NBYTES into BUF from FD.  Return the number read or -1.  */
ssize_t
__libc_read (int fd, void *buf, size_t nbytes)
{
  return SYSCALL_CANCEL (read, fd, buf, nbytes);
}
libc_hidden_def (__libc_read)

libc_hidden_def (__read)
weak_alias (__libc_read, __read)//声明__read的弱别名为__libc_read
libc_hidden_def (read)
weak_alias (__libc_read, read)//声明read的弱别名为__libc_read
```

用户层调用`read`（man 2 read）或者调用`__read`（测试可行）都会调用到`__libc_read`

```cpp
#define SYSCALL_CANCEL(...) \
  ({									     \
    long int sc_ret;							     \
    if (SINGLE_THREAD_P) 						     \
      sc_ret = INLINE_SYSCALL_CALL (__VA_ARGS__); 			     \
    else								     \
      {									     \
	int sc_cancel_oldtype = LIBC_CANCEL_ASYNC ();			     \
	sc_ret = INLINE_SYSCALL_CALL (__VA_ARGS__);			     \
        LIBC_CANCEL_RESET (sc_cancel_oldtype);				     \
      }									     \
    sc_ret;								     \
  })
//实际调用INLINE_SYSCALL_CALL


#define INLINE_SYSCALL_CALL(...) \
  __INLINE_SYSCALL_DISP (__INLINE_SYSCALL, __VA_ARGS__)


#define __INLINE_SYSCALL_DISP(b,...) \
  __SYSCALL_CONCAT (b,__INLINE_SYSCALL_NARGS(__VA_ARGS__))(__VA_ARGS__)
//__SYSCALL_CONCAT 最后返回__INLINE_SYSCALL3(__VA_ARGS__)

#define __SYSCALL_CONCAT_X(a,b)     a##b
#define __SYSCALL_CONCAT(a,b)       __SYSCALL_CONCAT_X (a, b)

#define __INLINE_SYSCALL_NARGS(...) \
  __INLINE_SYSCALL_NARGS_X (__VA_ARGS__,7,6,5,4,3,2,1,0,)

#define __INLINE_SYSCALL_NARGS_X(a,b,c,d,e,f,g,h,n,...) n
```

`INLINE_SYSCALL_CALL` 最后返回`__INLINE_SYSCALL3(__VA_ARGS__)`

```cpp
#define __INLINE_SYSCALL3(name, a1, a2, a3) \
  INLINE_SYSCALL (name, 3, a1, a2, a3)
//INLINE_SYSCALL (read, 3, fd, buf, nbytes)
```

```cpp
#define INLINE_SYSCALL(name, nr, args...)				\
  ({ unsigned int _sys_result = INTERNAL_SYSCALL (name, , nr, args);	\
     if (__builtin_expect (INTERNAL_SYSCALL_ERROR_P (_sys_result, ), 0))	\
       {								\
	 __set_errno (INTERNAL_SYSCALL_ERRNO (_sys_result, ));		\
	 _sys_result = (unsigned int) -1;				\
       }								\
     (int) _sys_result; })

#define INTERNAL_SYSCALL(name, err, nr, args...)		\
	INTERNAL_SYSCALL_RAW(SYS_ify(name), err, nr, args)


#if defined(__thumb__)//一般走下面
/* We can not expose the use of r7 to the compiler.  GCC (as
   of 4.5) uses r7 as the hard frame pointer for Thumb - although
   for Thumb-2 it isn't obviously a better choice than r11.
   And GCC does not support asms that conflict with the frame
   pointer.

   This would be easier if syscall numbers never exceeded 255,
   but they do.  For the moment the LOAD_ARGS_7 is sacrificed.
   We can't use push/pop inside the asm because that breaks
   unwinding (i.e. thread cancellation) for this frame.  We can't
   locally save and restore r7, because we do not know if this
   function uses r7 or if it is our caller's r7; if it is our caller's,
   then unwinding will fail higher up the stack.  So we move the
   syscall out of line and provide its own unwind information.  */
# undef INTERNAL_SYSCALL_RAW
# define INTERNAL_SYSCALL_RAW(name, err, nr, args...)		\
  ({								\
      register int _a1 asm ("a1");				\
      int _nametmp = name;					\
      LOAD_ARGS_##nr (args)					\
      register int _name asm ("ip") = _nametmp;			\
      asm volatile ("bl      __libc_do_syscall"			\
                    : "=r" (_a1)				\
                    : "r" (_name) ASM_ARGS_##nr			\
                    : "memory", "lr");				\
      _a1; })
#else /* ARM */
# undef INTERNAL_SYSCALL_RAW
# define INTERNAL_SYSCALL_RAW(name, err, nr, args...)		\
  ({								\
       register int _a1 asm ("r0"), _nr asm ("r7");		\
       LOAD_ARGS_##nr (args)					\
       _nr = name;						\
       asm volatile ("swi	0x0	@ syscall " #name	\
		     : "=r" (_a1)				\
		     : "r" (_nr) ASM_ARGS_##nr			\
		     : "memory");				\
       _a1; })
#endif
//INTERNAL_SYSCALL调用INTERNAL_SYSCALL_RAW时，通过SYS_ify(name)宏将name处理成__NR_read，这个宏就是系统调用号（整型）
#define SYS_ify(syscall_name)	(__NR_##syscall_name)
```

`SYS_ify(name)`宏将`name`处理成`__NR_name`,arm32中`__NR_name`定义在`/arch/arm/include/asm/unistd.h->/arch/arm/include/uapi/asm/unistd.h`中，如下是`linux4.9.37`版本内核的定义

```cpp

#ifndef _UAPI__ASM_ARM_UNISTD_H
#define _UAPI__ASM_ARM_UNISTD_H

#define __NR_OABI_SYSCALL_BASE	0x900000

#if defined(__thumb__) || defined(__ARM_EABI__)
#define __NR_SYSCALL_BASE	0//一般都走这里
#else
#define __NR_SYSCALL_BASE	__NR_OABI_SYSCALL_BASE
#endif

/*
 * This file contains the system call numbers.
 */

#define __NR_restart_syscall		(__NR_SYSCALL_BASE+  0)
#define __NR_exit			(__NR_SYSCALL_BASE+  1)
#define __NR_fork			(__NR_SYSCALL_BASE+  2)
#define __NR_read			(__NR_SYSCALL_BASE+  3)//__NR_read值为3
#define __NR_write			(__NR_SYSCALL_BASE+  4)
#define __NR_open			(__NR_SYSCALL_BASE+  5)
#define __NR_close			(__NR_SYSCALL_BASE+  6)
					/* 7 was sys_waitpid */
#define __NR_creat			(__NR_SYSCALL_BASE+  8)
#define __NR_link			(__NR_SYSCALL_BASE+  9)
#define __NR_unlink			(__NR_SYSCALL_BASE+ 10)
#define __NR_execve			(__NR_SYSCALL_BASE+ 11)
#define __NR_chdir			(__NR_SYSCALL_BASE+ 12)
#define __NR_time			(__NR_SYSCALL_BASE+ 13)
#define __NR_mknod			(__NR_SYSCALL_BASE+ 14)
#define __NR_chmod			(__NR_SYSCALL_BASE+ 15)
#define __NR_lchown			(__NR_SYSCALL_BASE+ 16)
...
#endif
```



glibc最后通过swi 0x0软中断指令调用内核系统调用，将系统调用号放入r7寄存器，r0-r6保存参数。当执行这条指令时，CPU会跳转至中断向量表的软中断指令处，执行该处保存的调用函数，而在函数中会根据swi后面的24位立即数(在我们的例子中是0x0)执行不同操作。在这时候CPU已经处于保护模式，陷入内核中。

##### 2.用户访问内核的方法

##### 直接调用系统调用

```cpp
#include <sys/stat.h>

int main()
{
    int rv;
    rv = chmod("./txt", 0777);
  	return rv;
}
```

##### 使用汇编swi软中断指令

```cpp
#include <asm/unistd.h>//这个头文件定义了__NR_chmod,此应该头文件来自交叉编译器制作时使用的内核版本，注意不是<unistd.h>,<unistd.h>来自glibc。

/*宏定义来自内核，实际就是glibc处理系统调用陷入内核那一段*/
#define LOAD_ARGS_0()
#define ASM_ARGS_0
#define LOAD_ARGS_1(a1)             \
  int _a1tmp = (int) (a1);          \
  LOAD_ARGS_0 ()                \
  _a1 = _a1tmp;
#define ASM_ARGS_1  ASM_ARGS_0, "r" (_a1)
#define LOAD_ARGS_2(a1, a2)         \
  int _a2tmp = (int) (a2);          \
  LOAD_ARGS_1 (a1)              \
  register int _a2 asm ("a2") = _a2tmp;
#define ASM_ARGS_2  ASM_ARGS_1, "r" (_a2)

# define INTERNAL_SYSCALL_RAWS(name, err, nr, args...)      \
    ({                              \
     register int _a1 asm ("r0"), _nr asm ("r7");       \
     LOAD_ARGS_##nr (args)                  \
     _nr = name;                        \
     asm volatile ("swi 0x0 @ syscall " #name   \
             : "=r" (_a1)               \
             : "r" (_nr) ASM_ARGS_##nr          \
             : "memory");               \
             _a1; })

int main()
{
    INTERNAL_SYSCALL_RAWS(__NR_chmod,0,2,"./txt",0777);
}
```

编译反汇编后，此代码中swi 0x0会变成svc 0x0。(作为 ARM 汇编语言开发成果的一部分，SWI 指令已重命名为 SVC)

##### 3.使用syscall系统调用

syscall是glibc实现的一个函数，其原型和实现如下

```cpp
//glibc/posix/unistd.h
extern long int syscall (long int __sysno, ...);
```



```assembly
/*glibc/sysdeps/unix/sysv/linux/arm/syscall.S*/

#include <sysdep.h>

/* In the EABI syscall interface, we don't need a special syscall to
   implement syscall().  It won't work reliably with 64-bit arguments
   (but that is true on many modern platforms).  */

ENTRY (syscall)
	mov	ip, sp
	push	{r4, r5, r6, r7}
	cfi_adjust_cfa_offset (16)
	cfi_rel_offset (r4, 0)
	cfi_rel_offset (r5, 4)
	cfi_rel_offset (r6, 8)
	cfi_rel_offset (r7, 12)
	mov	r7, r0
	mov	r0, r1
	mov	r1, r2
	mov	r2, r3
	ldmfd	ip, {r3, r4, r5, r6}
	swi	0x0 @本质上也是通过swi 0x0
	pop	{r4, r5, r6, r7}
	cfi_adjust_cfa_offset (-16)
	cfi_restore (r4)
	cfi_restore (r5)
	cfi_restore (r6)
	cfi_restore (r7)
	cmn	r0, #4096
	it	cc
	RETINSTR(cc, lr)
	b	PLTJMP(syscall_error)
PSEUDO_END (syscall)
```

实例

```cpp
#include <unistd.h> //syscall func
#include <sys/syscall.h>   /* For SYS_xxx definitions */
//<sys/syscall.h>包含了 <asm/unistd.h>定义了__NR_xxx和<bits/syscall.h>定义了SYS_xxx
int main()
{
    syscall(SYS_chmod, "./txt", 0777);
    //SYS_chmod会被转为__NR_chmod进而转为系统调用号
}
```

#### swi软中断处理

```assembly
ENTRY(vector_swi)
#ifdef CONFIG_CPU_V7M
	v7m_exception_entry
#else
	sub	sp, sp, #PT_REGS_SIZE   //陷入内核以后，sp已经切换成svc特权模式下的栈了，先把该栈顶向下移动S_FRAME_SIZE个单位，为保留操作做准备
	stmia	sp, {r0 - r12}			@ Calling r0 - r12  //把r0 ~ r12寄存器放入栈中，ia是increase after的意思，r0放在栈顶的位置，不更新s
 ARM(	add	r8, sp, #S_PC		)  //移动r8到sp+#S_PC的位置
 ARM(	stmdb	r8, {sp, lr}^		)	@ Calling sp, lr  //指令中的“^”符号表示访问user mode的寄存器，db”是decrement before，该指令把用户态的sp,和lr放到栈中偏移#S_PC+4的位置
 THUMB(	mov	r8, sp			)
 THUMB(	store_user_sp_lr r8, r10, S_SP	)	@ calling sp, lr
	mrs	r8, spsr			@ called from non-FIQ mode, so ok.  //读取用户态的cpsr	
	str	lr, [sp, #S_PC]			@ Save calling PC //把返回地址放入栈中偏移#S_PC的位置
	str	r8, [sp, #S_PSR]		@ Save CPSR //保存	用户态的cpsr
	str	r0, [sp, #S_OLD_R0]		@ Save OLD_R0  //保存旧的r0，后面会用到该寄存器
#endif
	zero_fp
	alignment_trap r10, ip, __cr_alignment
	enable_irq @陷入异常以后cpu 默认禁止中断，这边开启中断
	ct_user_exit
	get_thread_info tsk @tsk是r9 寄存器的别名,把thread_info放入r9寄存器

	/*
	 * Get the system call number.
	 */

#if defined(CONFIG_OABI_COMPAT)

	/*
	 * If we have CONFIG_OABI_COMPAT then we need to look at the swi
	 * value to determine if it is an EABI or an old ABI call.
	 */
#ifdef CONFIG_ARM_THUMB
	tst	r8, #PSR_T_BIT
	movne	r10, #0				@ no thumb OABI emulation
 USER(	ldreq	r10, [lr, #-4]		)	@ get SWI instruction
#else
 USER(	ldr	r10, [lr, #-4]		)	@ get SWI instruction //当定义了老的OABI传参模式，则从返回地址-4，得到原来那条svc 陷入指令
#endif
 ARM_BE8(rev	r10, r10)			@ little endian instruction

#elif defined(CONFIG_AEABI)

	/*
	 * Pure EABI user space always put syscall number into scno (r7).//如果定义了AEABI，则系统调用号在r7中，这边什么都不做
	 */
#elif defined(CONFIG_ARM_THUMB)
	/* Legacy ABI only, possibly thumb mode. */
	tst	r8, #PSR_T_BIT			@ this is SPSR from save_user_regs
	addne	scno, r7, #__NR_SYSCALL_BASE	@ put OS number in
 USER(	ldreq	scno, [lr, #-4]		)

#else
	/* Legacy ABI only. */
 USER(	ldr	scno, [lr, #-4]		)	@ get SWI instruction
#endif

	uaccess_disable tbl

	adr	tbl, sys_call_table		@ load syscall table pointer  //tbl是r8寄存器的别名,把系统调用表放入r8寄存器中

#if defined(CONFIG_OABI_COMPAT)
	/*
	 * If the swi argument is zero, this is an EABI call and we do nothing.
	 *
	 * If this is an old ABI call, get the syscall number into scno and
	 * get the old ABI syscall table address.
	 */
	bics	r10, r10, #0xff000000
	eorne	scno, r10, #__NR_OABI_SYSCALL_BASE //OABI 方式从svc指令中取出系统调用号，放入scno中
	ldrne	tbl, =sys_oabi_call_table  @OABI用的系统调用表更新为sys_oabi_call_table，其实起始地址和sys_call_table是一样的
#elif !defined(CONFIG_AEABI)
	bic	scno, scno, #0xff000000		@ mask off SWI op-code
	eor	scno, scno, #__NR_SYSCALL_BASE	@ check OS number
#endif

local_restart:
	ldr	r10, [tsk, #TI_FLAGS]		@ check for syscall tracing #//取出thread_info中的TI_FLAGS放入r10中，该位跟是否需要调度有关
	stmdb	sp!, {r4, r5}			@ push fifth and sixth args #把r4,r5压入栈，!符号表示同时更新sp的值，这个时候的sp的值是刚陷入内核的时候的sp-#S_FRAME_SIZE-8 这两个寄存器中保存着系统调用第五个和第六个参数(如果存在)

	tst	r10, #_TIF_SYSCALL_WORK		@ are we tracing syscalls? #跟踪系统调用中？
	bne	__sys_trace		#跳转到跟踪处理

	cmp	scno, #NR_syscalls		@ check upper syscall limit  //检查系统调用号是否超过范围
	badr	lr, ret_fast_syscall		@ return address //设置返回地址
	ldrcc	pc, [tbl, scno, lsl #2]		@ call sys_* routine #利用系统调用表加系统调用号的偏移，跳转到对应的系统调用函数进行处理

	add	r1, sp, #S_OFF
2:	cmp	scno, #(__ARM_NR_BASE - __NR_SYSCALL_BASE)
	eor	r0, scno, #__NR_SYSCALL_BASE	@ put OS number back
	bcs	arm_syscall 
	mov	why, #0				@ no longer a real syscall
	b	sys_ni_syscall			@ not private func

#if defined(CONFIG_OABI_COMPAT) || !defined(CONFIG_AEABI)
	/*
	 * We failed to handle a fault trying to access the page
	 * containing the swi instruction, but we're not really in a
	 * position to return -EFAULT. Instead, return back to the
	 * instruction and re-enter the user fault handling path trying
	 * to page it in. This will likely result in sending SEGV to the
	 * current task.
	 */
9001:
	sub	lr, lr, #4
	str	lr, [sp, #S_PC]
	b	ret_fast_syscall
#endif
ENDPROC(vector_swi)
```

内核定义了如下寄存器别名：

```assembly
scno    .req    r7        @ syscall number
tbl    .req    r8        @ syscall table pointer
why    .req    r8        @ Linux syscall (!= 0)
tsk    .req    r9        @ current thread_info
```

get_thread_info

```assembly
/*
 * Get current thread_info.
 */
	.macro	get_thread_info, rd
 ARM(	mov	\rd, sp, lsr #THREAD_SIZE_ORDER + PAGE_SHIFT	)
 THUMB(	mov	\rd, sp			)
 THUMB(	lsr	\rd, \rd, #THREAD_SIZE_ORDER + PAGE_SHIFT	)
	mov	\rd, \rd, lsl #THREAD_SIZE_ORDER + PAGE_SHIFT
	.endm
```

linux thread_info内核栈是在同一块内存中，一般设置为8K，所以把内核栈8K字节对齐以后，就能获取到thread_info的地址，并把该地址放入tsk寄存器中



##### adr   tbl, sys_call_table

```assembly
//arch/arm/kernel/entry-common.S

.equ NR_syscalls,0
#define CALL(x) .equ NR_syscalls,NR_syscalls+1 #计算calls.S中 最大系统调用数  并把该值赋值给NR_syscalls
#include "calls.S"

/*
 * Ensure that the system call table is equal to __NR_syscalls,
 * which is the value the rest of the system sees
 */
.ifne NR_syscalls - __NR_syscalls
.error "__NR_syscalls is not equal to the size of the syscall table"
.endif

#undef CALL
#define CALL(x) .long x #重定义CALL

.type	sys_call_table, #object
ENTRY(sys_call_table)
#include "calls.S" #初始化系统调用表
```



calls.S

```assembly
 //linux/arch/arm/kernel/calls.S
 *
 *  Copyright (C) 1995-2005 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This file is included thrice in entry-common.S
 */
/* 0 */		CALL(sys_restart_syscall)
		CALL(sys_exit)
		CALL(sys_fork)
		CALL(sys_read)
		CALL(sys_write)
/* 5 */		CALL(sys_open)
		CALL(sys_close)
		CALL(sys_ni_syscall)		/* was sys_waitpid */
		CALL(sys_creat)
		CALL(sys_link)
/* 10 */	CALL(sys_unlink)
		CALL(sys_execve)
		CALL(sys_chdir)
		CALL(OBSOLETE(sys_time))	/* used by libc4 */
		CALL(sys_mknod)
/* 15 */	CALL(sys_chmod)
/* 10 */	CALL(sys_unlink)

```

CALL在此处保存了所有系统调用的首地址，内核中系统调用都是以sys_开头的函数。

##### ldrcc   pc, [tbl, scno, lsl #2]  

利用系统调用表加系统调用号的偏移，跳转到对应的系统调用函数进行处理



#### 内核的系统调用定义

以open为例，内核没有直接定义sys_open。

声明如下

```cpp
asmlinkage long sys_open(const char __user *filename,
				int flags, umode_t mode);
```

定义fs/open.c

```cpp
SYSCALL_DEFINE3(open, const char __user *, filename, int, flags, umode_t, mode)
{
	if (force_o_largefile())
		flags |= O_LARGEFILE;

	return do_sys_open(AT_FDCWD, filename, flags, mode);
}
```

**SYSCALL_DEFINE3**是一个宏

```cpp
//syscalls.h (include\linux)


#define SYSCALL_DEFINE3(name, ...) SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)


#define SYSCALL_DEFINEx(x, sname, ...)				\
	SYSCALL_METADATA(sname, x, __VA_ARGS__)			\
	__SYSCALL_DEFINEx(x, sname, __VA_ARGS__)

#define __PROTECT(...) asmlinkage_protect(__VA_ARGS__)
#define __SYSCALL_DEFINEx(x, name, ...)					\
	asmlinkage long sys##name(__MAP(x,__SC_DECL,__VA_ARGS__))	\
		__attribute__((alias(__stringify(SyS##name))));		\
	static inline long SYSC##name(__MAP(x,__SC_DECL,__VA_ARGS__));	\
	asmlinkage long SyS##name(__MAP(x,__SC_LONG,__VA_ARGS__));	\
	asmlinkage long SyS##name(__MAP(x,__SC_LONG,__VA_ARGS__))	\
	{								\
		long ret = SYSC##name(__MAP(x,__SC_CAST,__VA_ARGS__));	\
		__MAP(x,__SC_TEST,__VA_ARGS__);				\
		__PROTECT(x, ret,__MAP(x,__SC_ARGS,__VA_ARGS__));	\
		return ret;						\
	}								\
	static inline long SYSC##name(__MAP(x,__SC_DECL,__VA_ARGS__))
```

最终展开结果如下：

```cpp
asmlinkage long sys_open(const char __user *filename, int flags, int mode)
{
	if (force_o_largefile())
		flags |= O_LARGEFILE;
 
	return do_sys_open(AT_FDCWD, filename, flags, mode);
}
```

随后do_sys_open会继续执行，直到调用file->fops->open,可能来自文件系统(ext2,ext4(fs->ext4)...)的普通文件，也可能来自设备驱动的设备文件

#### 系统调用分类

系统调⽤按照功能逻辑⼤致可分为 `进程控制`  `⽂件系统控制`  `系统控制`   `存管管理`   `⽹络管理`   `socket控制`   `⽤户管理`   `进程间通信`



##### 进程控制类系统调用

```shell
fork，clone，execve，exit，_exit，getdtablesize，getpgid，setpgid，getpgrp，setpgrp，getpid，getppid，getpriority，setpriority，modify_ldt，nanosleep，nice，pause，personality，prctl，ptrace，sched_get_priority_max，sched_get_priority_min，sched_getparam，sched_getscheduler，sched_rr_get_interval，sched_setparam，sched_setscheduler，sched_yield，vfork，wait，wait3，waitpid，wait4，capget，capset，getsid，setsid
```

##### 文件操作类系统调用

```shell
fcntl，open，creat，close，read，write，readv，writev，pread，pwrite，lseek，_llseek，dup，dup2，flock，polltruncate，ftruncate，umask，fsync
```

#### 文件系统操作类系统调用

```shell
access，chdir，fchdir，chmod，fchmod，chown，fchown，lchown，chroot，stat，lstat，fstat，statfs，fstatfs，readdir，getdents，mkdir，mknod，rmdir，rename，link，symlink，unlink，readlink，mount，umount，ustat，utime，utimes，quotactl
```

#### 系统控制类系统调用

```shell
ioctl，_sysctl，acct，getrlimit，setrlimit，getrusage，uselib，ioperm，iopl，outb，reboot，swapon，swapoff，bdflush，sysfs，sysinfo，adjtimex，alarm，getitimer，setitimer，gettimeofday，settimeofday，stime，time，times，uname，vhangup，nfsservctl，vm86，create_module，delete_module，init_modulequery_module，get_kernel_syms
```

##### 内存管理类系统调用

```shell
brk，sbrk，mlock，munlock，mlockall，munlockall，mmap，munmap，mremap，msync，mprotect，getpagesize，sync，cacheflush
```

##### 网络管理类系统调用

```shell
getdomainname，setdomainname，gethostid，sethostid，gethostname，sethostname，socketcall，socket，bind，connect，accept，send，sendto，sendmsg，recv，recvfrom，recvmsg，listen，select，shutdown，getsockname，getpeername，getsockopt，setsockopt，sendfile，socketpair
```

##### 用户管理类系统调用

```shell
getuid，setuid，getgid，setgid，getegid，setegid，geteuid，seteuid，setregid，setreuid，getresgid，setresgid，getresuid，setresuid，setfsgid，setfsuid，getgroups，setgroups
```

##### 进程间通信类系统调用

```cpp
sigaction，sigprocmask，sigpending，sigsuspend，signal，kill，sigblock，siggetmask，sigsetmask，sigmask，sigpause，sigvec，ssetmask，msgctl，msgget，msgsnd，msgrcv，pipe ，semctl，semget，semop，shmctl，shmget，shmat，shmdt
```



ref:

https://www.sohu.com/a/560153388_121124360

https://www.cnblogs.com/tolimit/p/4262851.html

https://blog.csdn.net/oqqYuJi12345678/article/details/100746436?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522166683977516800184123590%2522%252C%2522scm%2522%253A%252220140713.130102334.pc%255Fblog.%2522%257D&request_id=166683977516800184123590&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~blog~first_rank_ecpm_v1~rank_v31_ecpm-1-100746436-null-null.nonecase&utm_term=%E7%B3%BB%E7%BB%9F%E8%B0%83%E7%94%A8&spm=1018.2226.3001.4450

https://blog.csdn.net/oqqYuJi12345678/article/details/100713243?ops_request_misc=%257B%2522request%255Fid%2522%253A%2522166683977516800184123590%2522%252C%2522scm%2522%253A%252220140713.130102334.pc%255Fblog.%2522%257D&request_id=166683977516800184123590&biz_id=0&utm_medium=distribute.pc_search_result.none-task-blog-2~blog~first_rank_ecpm_v1~rank_v31_ecpm-2-100713243-null-null.nonecase&utm_term=%E7%B3%BB%E7%BB%9F%E8%B0%83%E7%94%A8&spm=1018.2226.3001.4450



### x86-64

只看了进入glibc部分，没有看内核处理

```cpp
/sysdep.h/unix/sysv/linux/x86_64
# define INLINE_SYSCALL(name, nr, args...) \
  ({									      \
    unsigned long int resultvar = INTERNAL_SYSCALL (name, , nr, args);	      \
    if (__glibc_unlikely (INTERNAL_SYSCALL_ERROR_P (resultvar, )))	      \
      {									      \
	__set_errno (INTERNAL_SYSCALL_ERRNO (resultvar, ));		      \
	resultvar = (unsigned long int) -1;				      \
      }									      \
    (long int) resultvar; })


#define INTERNAL_SYSCALL(name, err, nr, args...)			\
	internal_syscall##nr (SYS_ify (name), err, args)


#define internal_syscall3(number, err, arg1, arg2, arg3)		\
({									\
    unsigned long int resultvar;					\
    TYPEFY (arg3, __arg3) = ARGIFY (arg3);			 	\
    TYPEFY (arg2, __arg2) = ARGIFY (arg2);			 	\
    TYPEFY (arg1, __arg1) = ARGIFY (arg1);			 	\
    register TYPEFY (arg3, _a3) asm ("rdx") = __arg3;			\
    register TYPEFY (arg2, _a2) asm ("rsi") = __arg2;			\
    register TYPEFY (arg1, _a1) asm ("rdi") = __arg1;			\
    asm volatile (							\
    "syscall\n\t"							\
    : "=a" (resultvar)							\
    : "0" (number), "r" (_a1), "r" (_a2), "r" (_a3)			\
    : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);			\
    (long int) resultvar;						\
})

```

x86-64直接使用syscall汇编指令，进入内核继续指令

x86(32位)则使用int80指令

ref:https://cloud.tencent.com/developer/article/1492374



demo:

```cpp
#include <unistd.h>
#include <asm/unistd.h>//包含x86-64下的__NR_write定义
//以下宏直接copy的glibc
#define TYPEFY(X, name) __typeof__ ((X) - (X)) name
#define ARGIFY(X) ((__typeof__ ((X) - (X))) (X))
# define REGISTERS_CLOBBERED_BY_SYSCALL "cc", "r11", "cx"

#define internal_syscall3(number, err, arg1, arg2, arg3)        \
({                                  \
    unsigned long int resultvar;                    \
    TYPEFY (arg3, __arg3) = ARGIFY (arg3);              \
    TYPEFY (arg2, __arg2) = ARGIFY (arg2);              \
    TYPEFY (arg1, __arg1) = ARGIFY (arg1);              \
    register TYPEFY (arg3, _a3) asm ("rdx") = __arg3;           \
    register TYPEFY (arg2, _a2) asm ("rsi") = __arg2;           \
    register TYPEFY (arg1, _a1) asm ("rdi") = __arg1;           \
    asm volatile (                          \
    "syscall\n\t"                           \
    : "=a" (resultvar)                          \
    : "0" (number), "r" (_a1), "r" (_a2), "r" (_a3)         \
    : "memory", REGISTERS_CLOBBERED_BY_SYSCALL);            \
    (long int) resultvar;                       \
})
int main()
{
    int i = 0;
    {
        internal_syscall3(__NR_write,0,1,"hello",5);
    }
    return 0;
}
```