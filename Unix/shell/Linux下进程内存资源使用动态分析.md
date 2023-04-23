## Linux下进程内存资源使用动态分析
### 相关命令和文件
`top`动态查看操作系统所有进程的资源消耗和汇总（总的资源消耗其实就是读取`/proc/meminfo`，而进程的资源消耗读取的是`/proc/pid/statm`）
`/proc/meminfo` 操作系统自带反应总体内存消耗最全的文件
`free (-m)`读取`/proc/meminfo`
`/proc/pid/statm`反应`pid`进程的资源消耗情况

#### /proc/meminfo
`cat /proc/meminfo (ubuntu)`
`MemTotal:        1006976 kB`物理内存总量
`MemFree:          527636 kB`空闲物理内存总量
`MemAvailable:     489576 kB`可用的物理内存总量（相对MemFree这个对“可用”来说更准确）

##### 关于高速缓冲区
高速缓冲区是为了优化硬件与硬件之间数据处理频率差异较大而存在的。
分为CPU**高速缓存**和**页高速缓存**

###### CPU高速缓存(cache)
由于CPU主频与内存主频相差太大，所以产生了cpu高速缓存。
cache译名高速缓冲存储器，一般是SRAM（静态的RAM），存在于CPU和主存SDRAM之间。其作用是为了更好的利用局部性原理，减少CPU访问主存的次数。简单地说，CPU正在访问的指令和数据，其可能会被以后多次访问到，或者是该指令和数据附近的内存区域，也可能会被多次访问。因此，第一次访问这一块区域时，将其复制到cache中，以后访问该区域的指令或者数据时，就不用再从主存中取出。
cache对于程序员是不可见的，它完全是由硬件控制的。

CPU要读数据首先是在cache中读，如果cache命中，也叫cache hit，CPU就可以极快的得到该地址处的值。如果cache miss 也就是没有命中，它就会通过总线在内存中去读，并把连续的一块单元加载到cache中，下次好使用。
CPU写回也就是当CPU写入cache中的时候，数据不会马上从cache中写到内存里面，而是等待时机成熟后写入（比如 发生cache miss，其他内存要占用该cache line的时候将该单元写回到内存中，或者一定周期后写入到内存中 ，或者其它地核需要读取该内存的时候）。

cache大多是SRAM（静态RAM），而内存大多是DRAM（动态随即存储）或者DDR(双倍动态随机存储)。
cache容量一般非常小，因为价格贵，所以cache小是有道理的。一级cache一般就几KB，cache 的单位又分成cache line ，它是从内存单元加载到cache中的最小单元，一般为几个字大小，32字节或者64字节偏多。（因为时间局部性和空间局部性所以加载一次是以一个cache单元为最小单位）

多级的cache：
一级cache 有指令cache和数据cache之分，这使整个系统更加高效，因为1Lcache 容量小，所以有了多级cache ，比如二级cache ，他容量大，但是速度就要比1Lcache 慢些，但比内存快多了。三级cache就更慢一些了。
![image-20230424012753734](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424012753734.png)

###### 页高速缓存 （cache，也叫page cache）
页高速缓存用于优化内存和磁盘之间访问速率相差较大的情况，页高速缓存cache在主存里
页高速缓存(cache)是Linux内核实现磁盘缓存。磁盘高速缓存是一种软件机制，**它允许系统把通常存放在磁盘上的一些数据保留在 RAM 中，以便对那些数据的进一步访问不用再访问磁盘而能尽快得到满足。**它主要用来减少对磁盘I/O操作。是通过把磁盘中的数据缓存到 物理内存中，把对磁盘的访问变为对物理内存的访问。

当内核从磁盘中读取数据时，内核试图先从高速缓冲中读。如果数据已经在该高速缓冲中，则内核可以不必从磁盘上读。如果数据不在该高速缓冲中，则内核从磁盘上读数据，并将其缓冲起来。

写操作与之类似，要往磁盘上写的数据也被暂存于高速缓冲中，以便如果内核随后又试图读它时，它能在高速缓冲中。内核也通过判定是否数据必须真的需要存储到磁盘上，或数据是否是将要很快被重写的暂时性数据，来减少磁盘写操作的频率。


`Buffers:            3804 kB`用来给文件做缓冲大小 
`Cached:            46032 kB`被高速缓冲存储器（cache memory）用的内存的大小（等于 diskcache minus SwapCache ）. 
`SwapCached:          148 kB`被高速缓冲存储器（cache memory）用的交换空间的大小 已经被交换出来的内存，但仍然被存放在swapfile中。用来在需要的时候很快的被替换而不需要再次打开I/O端口。
`Active:           208216 kB`在活跃使用中的缓冲或高速缓冲存储器页面文件的大小，除非非常必要否则不会被移作他用. 
`Inactive:         136796 kB`在不经常使用中的缓冲或高速缓冲存储器页面文件的大小，可能被用于其他途径. 
`Active(anon):     186388 kB`
...
`SwapTotal:       1046524 kB`swap分区内存总量
`SwapFree:        1044692 kB`swap分区内存使用总量
`Dirty:                 0 kB`未被写回磁盘的数据总量
`Writeback:             0 kB`正在被写回到磁盘的内存大小

ref:
https://blog.csdn.net/whbing1471/article/details/105468139/
https://blog.csdn.net/weixin_40535588/article/details/119998102

#### TOP
top : 实时反应操作系统内存资源的使用情况
-d 刷新时间s  -p pid只查看pid进程的内存使用情况

```cpp
zh@zh-virtual-machine:/proc$ top
top - 16:46:35 up  2:26,  4 users,  load average: 0.00, 0.00, 0.00
/*依次当前时间，系统运行时间，当前登录用户数，系统负载，即任务队列的平均长度。 三个数值分别为 1分钟、5分钟、15分钟前到现在的平均值。*/
Tasks: 268 total,   1 running, 266 sleeping,   0 stopped,   1 zombie
/*进程总数 当前运行进程数 睡眠进程数 停止运行进程数 僵尸进程数*/
%Cpu(s):  0.0 us,  0.2 sy,  0.0 ni, 99.7 id,  0.1 wa,  0.0 hi,  0.0 si,  0.0 st
/*用户空间占用CPU百分比 内核空间占用CPU百分比 用户进程空间内改变过优先级的进程占用CPU百分比 空闲CPU百分比 等待输入输出的CPU时间百分比 硬中断（Hardware IRQ）占用CPU的百分比 软中断（Software Interrupts）占用CPU的百分比*/
KiB Mem:   1006976 total,   482200 used,   524776 free,     4448 buffers
/*物理内存总量 使用的物理内存总量 空闲内存总量 用作内核缓存的内存量*/
KiB Swap:  1046524 total,     1832 used,  1044692 free.    47832 cached Mem
/*交换区总量 使用的交换区总量 空闲交换区总量 缓冲的交换区总量。（缓冲的交换区总量，这里解释一下，所谓缓冲的交换区总量，即内存中的内容被换出到交换区，而后又被换入到内存，但使用过的交换区尚未被覆盖，该数值即为这些内容已存在于内存中的交换区的大小。相应的内存再次被换出时可不必再对交换区写入）*/
  PID USER      PR  NI    VIRT    RES    SHR S  %CPU %MEM     TIME+ COMMAND
  /*进程PID 进程所有者用户名 优先级 NICE值（负优先级高）虚拟内存使用量 物理内存使用量 共享内存使用量 进程状态（D=不可中断的睡眠状态 R=运行 S=睡眠 T=跟踪/停止 Z=僵尸进程） 进程占用CPU百分比 进程使用物理内存百分比 进程使用CPU时间总计0.01s 执行的命令*/
    1 root      20   0   33916   2912   2152 S   0.0  0.3   0:01.77 init
    2 root      20   0       0      0      0 S   0.0  0.0   0:00.02 kthreadd
    3 root      20   0       0      0      0 S   0.0  0.0   0:00.09 ksoftirqd/0
    4 root      20   0       0      0      0 S   0.0  0.0   0:00.00 kworker/0:0
    5 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 kworker/0:0H
    7 root      20   0       0      0      0 S   0.0  0.0   0:04.48 rcu_sched
    8 root      20   0       0      0      0 S   0.0  0.0   0:00.00 rcu_bh
    9 root      rt   0       0      0      0 S   0.0  0.0   0:00.00 migration/0
   10 root      rt   0       0      0      0 S   0.0  0.0   0:00.03 watchdog/0
   11 root      rt   0       0      0      0 S   0.0  0.0   0:00.02 watchdog/1
   12 root      rt   0       0      0      0 S   0.0  0.0   0:00.00 migration/1
   13 root      20   0       0      0      0 S   0.0  0.0   0:00.00 ksoftirqd/1
   14 root      20   0       0      0      0 S   0.0  0.0   0:00.00 kworker/1:0
   15 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 kworker/1:0H
   16 root      rt   0       0      0      0 S   0.0  0.0   0:00.02 watchdog/2
   17 root      rt   0       0      0      0 S   0.0  0.0   0:00.00 migration/2
   18 root      20   0       0      0      0 S   0.0  0.0   0:00.01 ksoftirqd/2
   19 root      20   0       0      0      0 S   0.0  0.0   0:00.20 kworker/2:0
   20 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 kworker/2:0H
   21 root      rt   0       0      0      0 S   0.0  0.0   0:00.02 watchdog/3
   22 root      rt   0       0      0      0 S   0.0  0.0   0:00.01 migration/3
   23 root      20   0       0      0      0 S   0.0  0.0   0:00.00 ksoftirqd/3
   25 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 kworker/3:0H
   26 root      20   0       0      0      0 S   0.0  0.0   0:00.01 kdevtmpfs
   27 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 netns
   28 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 perf
   29 root      20   0       0      0      0 S   0.0  0.0   0:00.00 khungtaskd
   30 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 writeback
   31 root      25   5       0      0      0 S   0.0  0.0   0:00.00 ksmd
   32 root      39  19       0      0      0 S   0.0  0.0   0:00.00 khugepaged
   33 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 crypto
   34 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 kintegrityd
   35 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 bioset
   36 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 kblockd
   37 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 ata_sff
   38 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 md
   39 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 devfreq_wq
   41 root      20   0       0      0      0 S   0.0  0.0   0:02.88 kworker/0:1
   43 root      20   0       0      0      0 S   0.0  0.0   0:00.44 kswapd0
   44 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 vmstat
   45 root      20   0       0      0      0 S   0.0  0.0   0:00.00 fsnotify_mark
   46 root      20   0       0      0      0 S   0.0  0.0   0:00.00 ecryptfs-kthrea
   62 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 kthrotld
   63 root      20   0       0      0      0 S   0.0  0.0   0:00.24 kworker/1:1
   65 root      20   0       0      0      0 S   0.0  0.0   0:00.48 kworker/3:1
   66 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 acpi_thermal_pm
   67 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 bioset
   68 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 bioset
   69 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 bioset
   70 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 bioset
   71 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 bioset
   72 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 bioset
   73 root       0 -20       0      0      0 S   0.0  0.0   0:00.00 bioset

```
##### TOP中 buffers 与 cached 的异同
用户空间在向内核发起内存申请时,会根据当前请求页面是否在内存缓存当中进行相应的策略调整.内核在内存当中会开辟一块缓冲区,用于提升读取页面的效率,否则就要进行相应的磁盘I/O读写. cached和buffers正是缓冲区中对应的两个不同区域,从字面上理解,感觉相差不大,以下说明下cached和buffers的意义.
实际cached和buffers来时free命令
```
zh@zh-virtual-machine:/proc$ free
             total       used       free     shared    buffers     cached
Mem:       1006976     488292     518684       4360       7004      51292
-/+ buffers/cache:     429996     576980
Swap:      1046524       1832    1044692

```
buffers：主要用于I/O写，将应用程序的多次零碎写事件，集中到一个缓冲区(即buffers)，然后再一次性写入磁盘中，这样就提高了写磁盘的效率。
cached：主要用于提升读取相关页面的效率，内核会将经常访问的页面放到cached中，这样就提高了页面的命中率，cached越大，命中率就越高，就减少了I/O读请求。



## 页

物理页是内存管理的基本单位，尽管处理器的最下可寻址单位是字或者字节。但是由于内存管理单元MMU通常以页为单位进行处理，所以虚拟内存角度来看，页就是最小单位。体系架构不同，支持页的代销也不尽相同，有些体系支持几种不同大小的页，大多数32位体系支持4KB的页，64位一般会支持8KB的页。

内核用struct page表示系统中的每个物理页：
flag存放页的状态，包括是不是脏页，是不是被锁定在内存中等，每一位代表一种状态，可同时表示32种状态。
_count存放页的引用计数，也就是页被引用了多少次，当计数变为-1时，说明当前内存中没有引用这一页，于是在新分配时就可以使用它了，内核调用page_count范围0表示页空闲，大于0表示在被使用。
一个页可以作为页缓存使用，也可作为私有数据，或作为进程中的页表映射。
virtual域是页的虚拟地址，通常就是页映射在虚拟地址空间的地址。有些内存并不永久映射在内核空间上，此时这个域为NULL，需要时，必须动态映射这些页。

```cpp
struct page {
		unsigned long			flags;
		atomic_t				_count;
		atomic_t				_mapcount;
		unsigned long			private;
		struct address_space	*mapping;
		pgoff_t					index;
		struct list_head		lru;
		void					*virtual;
}
```

## 区

由于硬件限制，内核不能对所有页一视同仁，有些页位于特定物理地址上，不能将其用于一些特定的任务。由于存在这样的限制，所以内核把页划分成不同的区。内核使用区来对具有相似特性的页进行分组，Linux内核必须如下两种由于硬件存在的缺陷引起的内存问题：

1.一些硬件只能用某些特定的内存地址来执行DMA。

2.一些体系结构的内存的物理地址寻址范围比虚拟寻址范围大的多，这样，就有一些内存不能永久映射到内核空间上。

因为存在如上两种制约条件，Linux将内存划分为4个区域。

1.ZONE_DMA 这个区包含的页只能用于执行DMA操作。

2.ZONE_DMA32和ZONE_DMA类似，该区包含的页面可用来执行DMA操作，而和ZONE_DMA不同在于，这些页面只能被32位设备访问，在某些体系结构中，这个区比ZONE_DMA还要大。

3.ZONE_NORMAL 包含的页都是正常映射的页。

4.ZONE_HIGHEM 高端内存，其页不能永久映射到内核地址空间。