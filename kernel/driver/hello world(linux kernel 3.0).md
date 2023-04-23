# hello world(linux kernel 3.0)

## demo

```cpp
#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>


static __init int hello_init(void)
{
    printk(KERN_ALERT "Hello\n");
    return 0;
}


static __exit int hello_exit(void)
{
    printk(KERN_ALERT "GoodBye\n");
    return 0;
}


module_init(hello_init);
module_exit(hello_exit);


MODULE_AUTHOR("zhanghang");
MODULE_DESCRIPTION("Linux");
MODULE_LICENSE("Dual BSD/GPL");
```

这个模块定义了两个函数, 一个在模块加载到内核时（命令行调用`insmod`)被调用(`hello_init`)以及一个在模块被去除时(命令行调用`rmmod`)被调用(`hello_exit` ). `moudle_init` 和 `module_exit` 这几行使用了特别的内核宏来指出这两个函数的角色. 另一个特别的宏 (`MODULE_LICENSE`) 是用来告知内核, 该模块带有一个自由的许可证; 没有这样的说明, 在模块加载时内核会警告



## printk

`printk` 函数在 Linux 内核中定义并且对模块可用，它与标准 C 库函数 `printf` 的行为相似。内核需要它自己的打印函数, 因为它靠自己运行, 没有 C 库的帮助. 模块能够调用 `printk` 是因为在 `insmod`加载了它之后, 模块被链接到内核并且可存取内核的公用符号. 字串 `KERN_ALERT` 是消息的优先级。`printk`支持分级别打印调试，这些级别定义在`linux-3.0/include/linux/printk.h`文件中：



```cpp
#define KERN_EMERG "<0>" /* system is unusable */
#define KERN_ALERT "<1>" /* action must be taken immediately */
#define KERN_CRIT "<2>" /* critical conditions */
#define KERN_ERR "<3>" /* error conditions */
#define KERN_WARNING "<4>" /* warning conditions */
#define KERN_NOTICE "<5>" /* normal but significant condition */
#define KERN_INFO "<6>" /* informational */
#define KERN_DEBUG "<7>" /* debug-level messages */
/* Use the default kernel loglevel */
#define KERN_DEFAULT "<d>"
```

Linux内核中`printk()`的语句是否打印到串口终端上，与u-boot里的`bootargs`参数中的 `loglelve=7`相关，只有低于`loglevel`级别的信息才会打印到控制终端上，否则不会在控制终端上输出。这时我们只能通过`dmesg`命令查看。 linux下的`dmesg`命令的可以查看`linux`内核所有的打印信息，它们记录在`/var/log/messages`系统日志文件中。linux内核中的打印信息很多，我们可以使用 `dmesg -c`命令清除之前的打印信息。



## 关于宏__init,__exit,module_init(),module_exit()说明



### init/exit

```cpp
//include/linux/init.h：
#define __init __section(.init.text) __cold notrace
#define __exit __section(.exit.text) __exitused __cold notrace


#define __init __attribute__ ((__section__ (".init.text")))
#define __initdata __attribute__ ((__section__ (".init.data")))
#define __exitdata __attribute__ ((__section__(".exit.data")))
#define __exit_call __attribute_used__ __attribute__ ((__section__(".exitcall.exit")))
#ifdef MODULE
#define __exit __attribute__ ((__section__(".exit.text")))
#else
#define __exit __attribute_used__ __attribute__((__section__(".exit.text")))
#endif

#define module_init(x) __initcall(x);
#define __initcall(fn) device_initcall(fn)
#define device_initcall(fn) __define_initcall("6",fn,6)
typedef int (*initcall_t)(void);
#define __define_initcall(level,fn,id) \
static initcall_t __initcall_##fn##id __used \
__attribute__((__section__(".initcall" level ".init"))) = fn
```

`static __init int hello_init(void) `宏会被展开为 

`static __section(.init.text) __cold notrace int hello_init(void)`

`static __exit int hello_exit(void)` 宏会被展开为 

`static __section(.exit.text) __exitused __coldnotrace int hello_exit（void)`

`__init`:     

`__init`将函数放在`".init.text"`这个代码区中，`__initdata`将数据放在`".init.data"`这个数据区中。 标记为初始化的函数,表明该函数供在初始化期间使用。在模块装载之后，模块装载就会将初始化函数扔掉。这样可以将该函数占用的内存释放出来。`__exit`: 

`__exit`宏告知编译器，将函数放在`".exit.text"`这个区域中。`__exitdata`宏则告知编译器将数据放在`".exit.data"`这个区域中。`exit.*`区域仅仅对于模块是有用的：如果编译稳定的话，`exit`函数将永远不会被调用。只有当模块支持无效的时候，`exit.*`区域将被丢弃。这就是为什么定义中会出现`ifdef`。
     这里的 `__section` 为`gcc`的链接选项，他表示把该函数链接到`Linux`内核映像文件的相应段中，这样`hello_init`将会被链接进`.init.text`段中，而`hello_exit`将会被链接进`.exit.text`段中。被链接进这两个段中的函数代码在调用完成之后，内核将会自动释放他们所占用的内存资源。因为这些函数只需要初始化或退出一次，所以`hello_init()`和`hello_exit()`函数最好在前面加上`__init` 和`__exit`。



### module_init(),module_exit()

`module_init`除了初始化加载之外，还有后期释放内存的作用。`linux kernel`中有很大一部分代码是设备驱动代码，这些驱动代码都有初始化和反初始化函数，这些代码一般都只执行一次，为了有更有效的利用内存，这些代码所占用的内存可以释放出来。`linux`就是这样做的，对只需要初始化运行一次的函数都加上`__init`属性，`__init `宏告诉编译器如果这个模块被编译到内核则把这个函数放到（.init.text）段，`module_exit`的参数卸载时同`__init`类似，如果驱动被编译进内核，则`__exit`宏会忽略清理函数，因为编译进内核的模块不需要做清理工作，显然`__init`和`__exit`对动态加载的模块是无效的，只支持完全编译进内核。在kernel初始化后期，释放所有这些函数代码所占的内存空间。连接器把带`__init`属性的函数放在同一个`section`里，在用完以后，把整个`section`释放掉。当函数初始化完成后这个区域可以被清除掉以节约系统内存。`Kenrel`启动时看到的消息`“Freeing unused kernel memory: xxxk freed”`同它有关。

参考链接： 

http://emb.hqyj.com/Column/Column517.htm

http://emb.hqyj.com/Column/Column870.htm



## makefile

```Makefile
LINUX_SRC=${shell pwd}/../linux/linux-3.0/
CROSS_COMPILE=armgcc
INST_PATH=/tftp
PWD:=${shell pwd}
EXTRA_CFLAGS +=-DMODULE
obj-m +=test.o


modules:
    @make -C ${LINUX_SRC} M=${PWD} modules
    @make clear


uninstall:
    rm -f ${INST_PATH}/*.ko


install:uninstall
    cp -af *.ko ${INST_PATH}
clear:
    @rm -f *.o *.cmd *.mod.c
    @rm -rf *~ core .depend .tmp_versions Module.symvers modules.order -f
    @rm -f .*ko.cmd .*.o.cmd .*.o.d
    @rm -f *.unsigned


clean:
    @rm -f test.ko
```

`LINUX_SRC` 应该指定开发板所运行的`Linux`内核源码的路径，并且这个`linux`内核源码必须`makemenuconfig`并且`make`过的，因为Linux内核的一个模块可能依赖另外一个模块，如果另外一个没有编译则会出现问题。所以`Linux`内核必须编译过，这样才能确认这种依赖关系。     

交叉编译器必须使用 `CROSS_COMPILE` 变量指定。     

如果编译`Linux`内核需要其它的一些编译选项，那可以使用 `EXTRA_CFLAGS` 参数来指定。     

`obj-m += test.o` 该行告诉`Makefile`要将 `test.c`源码编译生成内核模块文件`test.ko` 。     

`@make -C $(LINUX_SRC) M=$(PWD) modules @ make`是不打印这个命令本身 

`-C`：把工作目录切换到`-C`后面指定的参数目录，`M`是`Linux`内核源码`Makefile`里面的一个变量，作用是回到当前目录继续读取`Makefile`。当使用`make`命令编译内核驱动模块时，将会进入到 `KERNEL_SRC` 指定的`Linux`内核源码中去编译，并在当前目录下生成很多临时文件以及驱动模块文件`test.ko`。     

`clear` 目标将编译`linux`内核过着产生的一些临时文件全部删掉。

## test

开发板内核与编译Hello模块内核应该一致

```shell
~ >: insmod test.ko
Hello
~ >: rmmod test.ko
GoodBye
```

PS：

如果出现

```shell
kernel_hello: Unknown symbol __aeabi_unwind_cpp_pr0 (err 0)
insmod: can't insert 'kernel_hello.ko': unknown symbol in module or invalid parameter
```



这个错误是因为驱动模块与开发板现在正在运行的内核不一致，重新编译升级最新的Linux内核之后重新测试OK。

