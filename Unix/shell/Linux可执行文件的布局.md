# Linux可执行文件的布局

## /proc/pid/maps

Linux伪文件系统`/proc`可以查看运行中进程的内存布局，通过`cat /proc/pid/maps`

测试程序
`vim test.c`

```cpp
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
char *g_str = "hello";

static int a;
static int b = 0;
static int c = 10;

int d;
int e = 0;
int f = 20;

const int g;
const int h = 0;
const int l = 10;

void *func2(void *args)
{
    char buf[0x10000];
    printf("alloc 0x10000 bytes stack for func2, buf start addr:%p, end addr:%p\n", &buf[0], &buf[0x10000 - 1]);
    getchar();
    char *ptr = malloc(1 * 1024 * 1024);
    printf("malloc 0x1000 bytes heap for func2, buf start addr:%p, end addr:%p\n", &ptr[0], &ptr[0x1000 - 1]);
    getchar();
    printf("fun2 has been over, free heap\n");
    free(ptr);
    return NULL;
}
int main(int argc, char **argv)
{
    static int m;
    static int n = 0;
    static int o = 10;

    const int p;
    const int q = 0;
    const int r = 10;

    int s;
    printf("current process pid:                                            %d\n",getpid());
    printf("argc:                                                           %p, argv:%p\n",&argc, argv[0]);
    printf("global string point address:                                    %p\n",&g_str);
    printf("global string address:                                          %p\n",g_str);
    printf("uninitialized global static integer address:                    %p\n", &a);
    printf("initialized to zero global static integer address:              %p\n",&b);
    printf("initialized to no-zero global static integer address:           %p\n",&c);
    printf("uninitialized global integer address:                           %p\n", &d);
    printf("initialized to zero global integer address:                     %p\n",&e);
    printf("initialized to no-zero global integer address:                  %p\n",&f);
    printf("uninitialized const global integer address:                     %p\n", &g);
    printf("initialized to zero const global integer address:               %p\n",&h);
    printf("initialized to no-zero const global integer address:            %p\n",&l);
    printf("uninitialized local static integer address:                     %p\n", &m);
    printf("initialized to zero local static integer address:               %p\n",&n);
    printf("initialized to no-zero local static integer address:            %p\n",&o);
    printf("uninitialized local const integer address:                      %p\n", &p);
    printf("initialized to zero local const integer address:                %p\n",&q);
    printf("uninitialized no-zero local const integer address:              %p\n", &r);
    printf("local var:                                                      %p\n",&s);
    getchar();
    char *ptr1 = malloc(0x1000);
    printf("malloc 0x1000 bytes for ptr1, start address:                    %p\n",ptr1);
    getchar();
    char *ptr2 = malloc(0x10000);
    printf("malloc 0x10000 bytes for ptr2, start address:                  %p\n",ptr2);
    getchar();
    char *ptr3 = malloc(2 * 1024 * 1024);
    printf("malloc 2M bytes for ptr3, start address:                        %p\n",ptr3);
    getchar();
    printf("free ptr3\n");
    free(ptr3);
    getchar();

    printf("running func\n");
    func();
    getchar();
    printf("running func2\n");
    pthread_t tid;
    pthread_create(&tid, NULL, func2, NULL);
    pthread_join(tid, NULL);
    getchar();
}

```

函数`func`所在文件`fun.c`，编译生成动态库`libfunc.so`

```cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void func()
{
    char buf[0x10000];
    printf("0x10000 bytes stack for func, buf start addr:%p, end addr:%p\n", &buf[0],  &buf[0x10000 - 1]);
    getchar();
    char *p = malloc(1 * 1024 * 1024);
    printf("malloc 1M bytes for func, start address:%p\n",p);
    getchar();
    printf("func has been over, free heap and return\n");
    free(p);
    return ;
}

```

`gcc -fPIC -c fun.c`
`gcc -shared -o libfunc.so fun.o`
链接成可执行文件
`gcc test.c -o test -lpthread -L ./ -lfunc`
配置`LD_LIBRARY_PATH`为`libfunc.c`所在目录，否则运行会提示找不到`libfunc.so`

运行程序
`./test`
此时程序打印`test`进程的`pid`

查看`/proc/pid/maps`文件
得到如下内容：

```
zh@zh-virtual-machine:~/anything_test$ cat /proc/13329/maps
00400000-00402000 r-xp 00000000 08:01 948758                             /home/zh/anything_test/a.out
00601000-00602000 r--p 00001000 08:01 948758                             /home/zh/anything_test/a.out
00602000-00603000 rw-p 00002000 08:01 948758                             /home/zh/anything_test/a.out
7f0defb6f000-7f0defd2d000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f0defd2d000-7f0deff2d000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f0deff2d000-7f0deff31000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f0deff31000-7f0deff33000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f0deff33000-7f0deff38000 rw-p 00000000 00:00 0
7f0deff38000-7f0deff39000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f0deff39000-7f0df0138000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f0df0138000-7f0df0139000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f0df0139000-7f0df013a000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f0df013a000-7f0df0153000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f0df0153000-7f0df0352000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f0df0352000-7f0df0353000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f0df0353000-7f0df0354000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f0df0354000-7f0df0358000 rw-p 00000000 00:00 0
7f0df0358000-7f0df037b000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f0df0562000-7f0df0566000 rw-p 00000000 00:00 0
7f0df0578000-7f0df057a000 rw-p 00000000 00:00 0
7f0df057a000-7f0df057b000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f0df057b000-7f0df057c000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f0df057c000-7f0df057d000 rw-p 00000000 00:00 0
7ffd0ec56000-7ffd0ec77000 rw-p 00000000 00:00 0                          [stack]
7ffd0ed08000-7ffd0ed0b000 r--p 00000000 00:00 0                          [vvar]
7ffd0ed0b000-7ffd0ed0d000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

```

### /proc/pid/maps初步分析

信息全部来之内核中表示进程地址空间的结构体`vm_area_struct`.
一个文件可以映射到进程一块内存区域中，映射文件描述符保存在`vm_area_struct->vm_file`中,这种内存区域叫有名内存区域，相反的为匿名内存区域。vm_area_struct的一些字段对应此文件中的每一列。
每一行有6列，每一列的意思分别如下：
1.`0x....-0x....` `vm_start-vm_end`此段的虚拟地址起始地址-此段虚拟地址结束地址.
2.`rw-p` `vm_flags`此段虚拟地址空间的权限属性，r可读，w可写，x可执行，p/s为互斥关系，p表示私有段，s表示共享段，没有相应的权限则用-表示.
3.`0000000` `vm_pgoff`对有名映射，表示此段虚拟内存起始地址在文件中以页为单位的偏移。对匿名映射为0或者vm_start/PAGE_SIZE.
4.`00:00` `vm_file->f_dentry->inode->i_sb->s_dev`映射文件所属的设备号。对匿名映射因为没有文件在磁盘上，所以没有设备号，为00:00，对有名映射为文件所在设备的设备号。
5.`660570` `vm_file->f_dentry->d_inode->i_ino`映射文件所属的节点号，匿名映射为0。
6.`path/libfunc.so`对有名映射，是映射的文件名，匿名映射，是此段虚拟内存在进程中的角色[stack]为栈，[heap]为堆。[vdso]代表虚拟共享库，系统调用进入内核会用到它
匿名映射由mmap创建，通常用于不在堆上的缓冲区，共享内存，pthread库也使用匿名空间作为线程的栈。
虚拟内存分配以页为单位，页的大小通常为4KB，不足4KB也会用4KB补齐。

**针对其中一行分析**
`7f0df0139000-7f0df013a000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so`
映射到进程的的虚拟地址空间为：`7f0df0139000-7f0df013a000`,8KB的虚拟地址空间
rw-p此段可读可写，不能执行，p表示此段为私有
00001000表示虚拟内存地址在文件中偏移了4KB。
08:01表示映射文件所属设备号，通过df可查看

```
zh@zh-virtual-machine:~/anything_test$ pwd
/home/zh/anything_test
zh@zh-virtual-machine:~/anything_test$ df
Filesystem     1K-blocks    Used Available Use% Mounted on
udev              489984       4    489980   1% /dev
tmpfs             100700    1084     99616   2% /run
/dev/sda1       19478204 5629248  12836476  31% /
none                   4       0         4   0% /sys/fs/cgroup
none                5120       0      5120   0% /run/lock
none              503488     152    503336   1% /run/shm
none              102400      32    102368   1% /run/user
zh@zh-virtual-machine:~/anything_test$ ls -la /dev/sda1
brw-rw---- 1 root disk 8, 1  1æ 18 15:01 /dev/sda1

```

`948764`文件`inode`号

```
zh@zh-virtual-machine:~/anything_test$ ls -li libfunc.so
948764 -rwxrwxr-x 1 zh zh 8268  1æ 19 13:48 libfunc.so
```

最后此动态库被链接到此可执行文件

### 使用strace系统调用跟踪进程，并结合/proc/pid/maps分析

1.`strace ./test`
此时的/proc/pid/map如下

```cpp
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]


```

`strace ./test`输出如下：


```cpp
zh@zh-virtual-machine:~/anything_test$ strace ./a.out
//bash进程fork+exec执行另外一个程序
execve("./a.out", ["./a.out"], [/* 32 vars */]) = 0
brk(0)                                  = 0x24c0000
access("/etc/ld.so.nohwcap", F_OK)      = -1 ENOENT (No such file or directory)
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
//由于编译时设置了-L ./ -lpthread选项为当前目录，运行时指定LD_LIBRARY为当前目录，所以先当前目录查找所需要的动态库libpthread.so
open("/home/zh/anything_test/tls/x86_64/libpthread.so.0", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/home/zh/anything_test/tls/x86_64", 0x7ffee837dd30) = -1 ENOENT (No such file or directory)
open("/home/zh/anything_test/tls/libpthread.so.0", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/home/zh/anything_test/tls", 0x7ffee837dd30) = -1 ENOENT (No such file or directory)
open("/home/zh/anything_test/x86_64/libpthread.so.0", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/home/zh/anything_test/x86_64", 0x7ffee837dd30) = -1 ENOENT (No such file or directory)
open("/home/zh/anything_test/libpthread.so.0", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
stat("/home/zh/anything_test", {st_mode=S_IFDIR|0775, st_size=4096, ...}) = 0
//前面无法找到libpthrad.so,开始在ld.so.cache里找，ld.so.cache中记录了系统中默认加载的所有动态库路径。
open("/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
fstat(3, {st_mode=S_IFREG|0644, st_size=79720, ...}) = 0//ld.so.cache大小为79720bytes=0x13768
//将ld.so.cache整个文件私有映射到虚拟内存的0x7f8d73e1e000-0x7f8d73e31768（4KB对齐后，0x7f8d73e32000）在maps里可以看到截止的0x7f8d73e32000，区间为可读可写私有的
mmap(NULL, 79720, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7f8d73e1e000
close(3)                                = 0
access("/etc/ld.so.nohwcap", F_OK)      = -1 ENOENT (No such file or directory)
//在ld.so.cache里找到了libpthread.so
open("/lib/x86_64-linux-gnu/libpthread.so.0", O_RDONLY|O_CLOEXEC) = 3
//读取动态库前832个字节，这832个字节足以得到所有段在文件的偏移
/*有关这个832:
https://www.jb51.cc/linux/397661.html
elf / dl-load.c：
The ELF header 32-bit files is 52 bytes long and in 64-bit files is 64 bytes long. Each program header entry is again 32 and 56 bytes long respectively. I.e.,even with a file which has 10 program header entries we only have to read 372B/624B respectively. Add to this a bit of margin for program notes and reading 512B and 832B for 32-bit and 64-bit files respecitvely is enough.*/
read(3, "\177ELF\2\1\1\0\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0po\0\0\0\0\0\0"..., 832) = 832
//libpthread.so大小141574 = 0x22906
fstat(3, {st_mode=S_IFREG|0755, st_size=141574, ...}) = 0
//匿名映射4K的空间
mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f8d73e1d000
//将libpthread.so映射到0x7f8d739f2000，总大小为 0x21d530可读可执行,readelf可以看到.text段在这里
mmap(NULL, 2217264, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f8d739f2000
//添加保护区（隔离区）此区域rwx都不具有，2093056 = 0x1ff000 ,0x7f8d73a0b000 + 0x1ff000 =0x7f8d73c0a000
mprotect(0x7f8d73a0b000, 2093056, PROT_NONE) = 0
//映射libpthread.so偏移0x18000的地址处数据到0x7f8d73c0a000大小为8K，权限为rw，通过readelf -hSW libpthread.so可以看到0x18000中包含.data.rel.ro这样的只读段，查看maps被限制成了r--p，0x19000开始的4K中包含.bss和.data段，查看Maps被限制到rw-p
mmap(0x7f8d73c0a000, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x18000) = 0x7f8d73c0a000
//匿名映射13616bytes=0x3530（16K，0x4000），权限rw
mmap(0x7f8d73c0c000, 13616, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7f8d73c0c000
close(3)                                = 0
//同上
open("/home/zh/anything_test/libfunc.so", O_RDONLY|O_CLOEXEC) = 3
//同上
read(3, "\177ELF\2\1\1\0\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0p\7\0\0\0\0\0\0"..., 832) = 832
//动态库大小为8268bytes
fstat(3, {st_mode=S_IFREG|0775, st_size=8268, ...}) = 0
//映射动态库(对齐后12k)，权限rx
mmap(NULL, 2101352, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f8d737f0000
//创建保护，权限---p,因此上面只映射了前4k为rx-p,readelf -hSW libfunc.so可以看到.text段在前4k，因此权限为rx
mprotect(0x7f8d737f1000, 2093056, PROT_NONE) = 0
//映射文件前8192字节(8K)，权限为rw，和前者一样，有地方将前4k的权限修改了。前4k被限制为r--p,后4k被限制成rw。因为前4k包含只读段，.bss和.data在后4k。
mmap(0x7f8d739f0000, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0) = 0x7f8d739f0000
close(3)                                = 0
open("/home/zh/anything_test/libc.so.6", O_RDONLY|O_CLOEXEC) = -1 ENOENT (No such file or directory)
access("/etc/ld.so.nohwcap", F_OK)      = -1 ENOENT (No such file or directory)
open("/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1\1\0\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0P \2\0\0\0\0\0"..., 832) = 832
fstat(3, {st_mode=S_IFREG|0755, st_size=1857312, ...}) = 0
mmap(NULL, 3965632, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_DENYWRITE, 3, 0) = 0x7f8d73427000
mprotect(0x7f8d735e5000, 2097152, PROT_NONE) = 0
mmap(0x7f8d737e5000, 24576, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_DENYWRITE, 3, 0x1be000) = 0x7f8d737e5000
mmap(0x7f8d737eb000, 17088, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS, -1, 0) = 0x7f8d737eb000
close(3)                                = 0
mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f8d73e1c000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f8d73e1a000
arch_prctl(ARCH_SET_FS, 0x7f8d73e1a740) = 0
//这里对上面的rw段做了部分限制，限制到r
mprotect(0x7f8d737e5000, 16384, PROT_READ) = 0
mprotect(0x7f8d739f0000, 4096, PROT_READ) = 0
mprotect(0x7f8d73c0a000, 4096, PROT_READ) = 0
mprotect(0x601000, 4096, PROT_READ)     = 0
mprotect(0x7f8d73e32000, 4096, PROT_READ) = 0
munmap(0x7f8d73e1e000, 79720)           = 0
set_tid_address(0x7f8d73e1aa10)         = 20634
set_robust_list(0x7f8d73e1aa20, 24)     = 0
futex(0x7ffee837e570, FUTEX_WAIT_BITSET_PRIVATE|FUTEX_CLOCK_REALTIME, 1, NULL, 7f8d73e1a740) = -1 EAGAIN (Resource temporarily unavailable)
rt_sigaction(SIGRTMIN, {0x7f8d739f89f0, [], SA_RESTORER|SA_SIGINFO, 0x7f8d73a02330}, NULL, 8) = 0
rt_sigaction(SIGRT_1, {0x7f8d739f8a80, [], SA_RESTORER|SA_RESTART|SA_SIGINFO, 0x7f8d73a02330}, NULL, 8) = 0
rt_sigprocmask(SIG_UNBLOCK, [RTMIN RT_1], NULL, 8) = 0
getrlimit(RLIMIT_STACK, {rlim_cur=8192*1024, rlim_max=RLIM64_INFINITY}) = 0
fstat(1, {st_mode=S_IFCHR|0620, st_rdev=makedev(136, 23), ...}) = 0
mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f8d73e31000
//main函数开始运行了，printf调用了write
write(1, "current process pid:            "..., 70current process pid:                                            20634
) = 70
write(1, "argc:                           "..., 100argc:                                                           0x7ffee837e59c, argv:0x7ffee83807b5
) = 100
write(1, "global string point address:    "..., 73global string point address:                                    0x602088
) = 73
write(1, "global string address:          "..., 73global string address:                                          0x400d88
) = 73
write(1, "uninitialized global static inte"..., 73uninitialized global static integer address:                    0x6020a4
) = 73
write(1, "initialized to zero global stati"..., 73initialized to zero global static integer address:              0x6020a8
) = 73
write(1, "initialized to no-zero global st"..., 73initialized to no-zero global static integer address:           0x602090
) = 73
write(1, "uninitialized global integer add"..., 73uninitialized global integer address:                           0x6020b4
) = 73
write(1, "initialized to zero global integ"..., 73initialized to zero global integer address:                     0x6020a0
) = 73
write(1, "initialized to no-zero global in"..., 73initialized to no-zero global integer address:                  0x602094
) = 73
write(1, "uninitialized const global integ"..., 73uninitialized const global integer address:                     0x6020b8
) = 73
write(1, "initialized to zero const global"..., 73initialized to zero const global integer address:               0x400d90
) = 73
write(1, "initialized to no-zero const glo"..., 73initialized to no-zero const global integer address:            0x400d94
) = 73
write(1, "uninitialized local static integ"..., 73uninitialized local static integer address:                     0x6020ac
) = 73
write(1, "initialized to zero local static"..., 73initialized to zero local static integer address:               0x6020b0
) = 73
write(1, "initialized to no-zero local sta"..., 73initialized to no-zero local static integer address:            0x602098
) = 73
write(1, "uninitialized local const intege"..., 79uninitialized local const integer address:                      0x7ffee837e5a0
) = 79
write(1, "initialized to zero local const "..., 79initialized to zero local const integer address:                0x7ffee837e5a4
) = 79
write(1, "uninitialized no-zero local cons"..., 79uninitialized no-zero local const integer address:              0x7ffee837e5a8
) = 79
write(1, "local var:                      "..., 79local var:                                                      0x7ffee837e5ac
) = 79
fstat(0, {st_mode=S_IFCHR|0620, st_rdev=makedev(136, 23), ...}) = 0
mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f8d73e30000
//调用getchar()阻塞等待数据
read(0,
```

输入回车

```cpp
//此时的maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out
024c0000-024e2000 rw-p 00000000 00:00 0                                  [heap]
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

//strace的跟踪
read(0, 0x7f8d73e30000, 1024)           = ? ERESTARTSYS (To be restarted if SA_RESTART is set)
--- SIGWINCH {si_signo=SIGWINCH, si_code=SI_KERNEL} ---
read(0,
"\n", 1024)                     = 1
//获取heap的起始地址，这个地址在程序开始运行时就被确定了,具体见程序运行最上面
brk(0)                                  = 0x24c0000
//应用调用malloc分配0x1000字节的堆，底层调用brk分配,可见0x24e2000-0x24c0000!=0x1000，可见brk分配会比申请的要大,maps可以看到堆的长度和brk分配长度一致
brk(0x24e2000)                          = 0x24e2000
write(1, "malloc 0x1000 bytes for ptr1, st"..., 74malloc 0x1000 bytes for ptr1, start address:                    0x24c0010//此段heap的起始地址0x24c0010
) = 74
read(0,

```

输入回车

```cpp
//maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
024c0000-024e2000 rw-p 00000000 00:00 0                                  [heap]
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]


//strace
"\n", 1024)                     = 1
write(1, "malloc 0x10000 bytes for ptr2, s"..., 73malloc 0x10000 bytes for ptr2, start address:                  0x24c1020//malloc分配0x10000字节，此heap的起始地址为前一段+0x1010，因为前一次分配的heap比较大，此次不用再分配。maps也可以看出和前一次没有区别
) = 73
read(0,

```

输入回车

```cpp
//maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
024c0000-024e2000 rw-p 00000000 00:00 0                                  [heap]
7f8d73226000-7f8d73427000 rw-p 00000000 00:00 0
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

//strace
"\n", 1024)                     = 1
mmap(NULL, 2101248, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7f8d73226000
write(1, "malloc 2M bytes for ptr3, start "..., 79malloc 2M bytes for ptr3, start address:                        0x7f8d73226010//malloc分配2M 0x200000bytes空间，此处调用了mmap分配0x201000bytes，并且返回的地址紧挨着共享库。
) = 79

```

输入回车

```cpp
//maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
024c0000-024e2000 rw-p 00000000 00:00 0                                  [heap]
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]


//strace
"\n", 1024)                     = 1
write(1, "free ptr3\n", 10free ptr3
)             = 10
munmap(0x7f8d73226000, 2101248)         = 0//释放ptr3指向的内存，maps上也可以看到内存释放

```

回车

```cpp
//maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
024c0000-024e2000 rw-p 00000000 00:00 0                                  [heap]
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

//strace
"\n", 1024)                     = 1
write(1, "running func\n", 13running func
)          = 13
write(1, "0x10000 bytes stack for func, bu"..., 850x10000 bytes stack for func, buf start addr:0x7ffee836e570, end addr:0x7ffee837e56f//调用函数分配栈，maps没有变化，可见栈在程序编译时已经分配好了
) = 85
read(0,

```

回车

```cpp
//maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
024c0000-025f2000 rw-p 00000000 00:00 0                                  [heap]
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

//strace
"\n", 1024)                     = 1
brk(0x25f2000)                          = 0x25f2000
write(1, "malloc 1M bytes for func, start "..., 50malloc 1M bytes for func, start address:0x24d1030
) = 50//函数中调用malloc分配1M内存，此处调用了brk将原来的heap扩大了，maps也可以看到，可见malloc分配内存较小和较大底层分别会调用brk/mmap
read(0,

```

输入回车

```cpp
//maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
024c0000-025f2000 rw-p 00000000 00:00 0                                  [heap]
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]


//strace
"\n", 1024)                     = 1
write(1, "func has been over, free heap an"..., 41func has been over, free heap and return
) = 41//释放heap，查看maps之前brk扩大的内存并没有减少，可见brk和mmap在这里也会有区别
read(0,

```

输入回车

```cpp
//maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
024c0000-025f2000 rw-p 00000000 00:00 0                                  [heap]
7f8d72c26000-7f8d72c27000 ---p 00000000 00:00 0
7f8d72c27000-7f8d73427000 rw-p 00000000 00:00 0
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

//strace
"\n", 1024)                     = 1
write(1, "running func2\n", 14running func2
)         = 14//创建线程
//为线程分配栈空间0x801000bytes，这个值+0x7f8d72c26000=7F8D73427000这个空间大小应该可以通过pthread_attr_t调整，此处使用默认值。
mmap(NULL, 8392704, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0) = 0x7f8d72c26000
//设置4K的栈保护区
mprotect(0x7f8d72c26000, 4096, PROT_NONE) = 0
//分配0x10000bytes栈空间，可见栈是从高地址开始分配，maps多了两块匿名区域前一块是4K的保护区，后一块是与前一块连续的线程栈
clone(alloc 0x10000 bytes stack for func2, buf start addr:0x7f8d73415f00, end addr:0x7f8d73425eff
child_stack=0x7f8d73425fb0, flags=CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|CLONE_THREAD|CLONE_SYSVSEM|CLONE_SETTLS|CLONE_PARENT_SETTID|CLONE_CHILD_CLEARTID, parent_tidptr=0x7f8d734269d0, tls=0x7f8d73426700, child_tidptr=0x7f8d734269d0) = 21488
futex(0x7f8d734269d0, FUTEX_WAIT, 21488, NULL

```

回车

```cpp
//maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
024c0000-025f2000 rw-p 00000000 00:00 0                                  [heap]
7f8d6c000000-7f8d6c121000 rw-p 00000000 00:00 0
7f8d6c121000-7f8d70000000 ---p 00000000 00:00 0
7f8d72c26000-7f8d72c27000 ---p 00000000 00:00 0
7f8d72c27000-7f8d73427000 rw-p 00000000 00:00 0
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]

//strace
//线程调用malloc分配1M 0x1000000（程序中实际分配为1M）bytes，此处没有mmap或brk系统调用，但是maps多了两段，前一段为堆rw，后一段为保护区
malloc 0x1000 bytes heap for func2, buf start addr:0x7f8d6c0008c0, end addr:0x7f8d6c0018bf
```

回车

```cpp
//maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/20634/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out (deleted)
024c0000-025f2000 rw-p 00000000 00:00 0                                  [heap]
7f8d6c000000-7f8d6c121000 rw-p 00000000 00:00 0
7f8d6c121000-7f8d70000000 ---p 00000000 00:00 0
7f8d72c26000-7f8d72c27000 ---p 00000000 00:00 0
7f8d72c27000-7f8d73427000 rw-p 00000000 00:00 0
7f8d73427000-7f8d735e5000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d735e5000-7f8d737e5000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e5000-7f8d737e9000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737e9000-7f8d737eb000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f8d737eb000-7f8d737f0000 rw-p 00000000 00:00 0
7f8d737f0000-7f8d737f1000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d737f1000-7f8d739f0000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f0000-7f8d739f1000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f1000-7f8d739f2000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f8d739f2000-7f8d73a0b000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73a0b000-7f8d73c0a000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0a000-7f8d73c0b000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0b000-7f8d73c0c000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f8d73c0c000-7f8d73c10000 rw-p 00000000 00:00 0
7f8d73c10000-7f8d73c33000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e1a000-7f8d73e1e000 rw-p 00000000 00:00 0
7f8d73e30000-7f8d73e32000 rw-p 00000000 00:00 0
7f8d73e32000-7f8d73e33000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e33000-7f8d73e34000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f8d73e34000-7f8d73e35000 rw-p 00000000 00:00 0
7ffee8360000-7ffee8381000 rw-p 00000000 00:00 0                          [stack]
7ffee8393000-7ffee8396000 r--p 00000000 00:00 0                          [vvar]
7ffee8396000-7ffee8398000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]


//strace

fun2 has been over, free heap
) = 0//释放线程分配的堆，从maps看堆和栈都没有释放，而此处pthread_join已经调用
read(0,

```

至此主函数结束（main函数中依然存在heap没有释放，只用于测试）

接下来看下函数中定义的变量分布,重新运行程序，并查看maps

```cpp
//maps
zh@zh-virtual-machine:~/anything_test$ cat /proc/21659/maps
00400000-00402000 r-xp 00000000 08:01 941751                             /home/zh/anything_test/a.out
00601000-00602000 r--p 00001000 08:01 941751                             /home/zh/anything_test/a.out
00602000-00603000 rw-p 00002000 08:01 941751                             /home/zh/anything_test/a.out
7f305ff04000-7f30600c2000 r-xp 00000000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f30600c2000-7f30602c2000 ---p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f30602c2000-7f30602c6000 r--p 001be000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f30602c6000-7f30602c8000 rw-p 001c2000 08:01 660596                     /lib/x86_64-linux-gnu/libc-2.19.so
7f30602c8000-7f30602cd000 rw-p 00000000 00:00 0
7f30602cd000-7f30602ce000 r-xp 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f30602ce000-7f30604cd000 ---p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f30604cd000-7f30604ce000 r--p 00000000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f30604ce000-7f30604cf000 rw-p 00001000 08:01 948764                     /home/zh/anything_test/libfunc.so
7f30604cf000-7f30604e8000 r-xp 00000000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f30604e8000-7f30606e7000 ---p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f30606e7000-7f30606e8000 r--p 00018000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f30606e8000-7f30606e9000 rw-p 00019000 08:01 660716                     /lib/x86_64-linux-gnu/libpthread-2.19.so
7f30606e9000-7f30606ed000 rw-p 00000000 00:00 0
7f30606ed000-7f3060710000 r-xp 00000000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f30608f7000-7f30608fb000 rw-p 00000000 00:00 0
7f306090d000-7f306090f000 rw-p 00000000 00:00 0
7f306090f000-7f3060910000 r--p 00022000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f3060910000-7f3060911000 rw-p 00023000 08:01 660570                     /lib/x86_64-linux-gnu/ld-2.19.so
7f3060911000-7f3060912000 rw-p 00000000 00:00 0
7ffc84978000-7ffc84999000 rw-p 00000000 00:00 0                          [stack]
7ffc849cf000-7ffc849d2000 r--p 00000000 00:00 0                          [vvar]
7ffc849d2000-7ffc849d4000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]


//a.out
//命令行参数argv地址比栈还要高
argc:
0x7ffc84997cbc, argv:0x7ffc849987bd
//全局变量指向常量字符串的指针在maps的第三段，此段为rw权限，可见此段包含了.data段
global string point address:                                    0x602088
//常量字符串在maps第一段，此段权限为rx,可见此段为，可见.text段和.rodata段位于此区域
global string address:                                          0x400d88
//未初始化/初始化为0/初始化为非0的全局静态变量 在maps第三段，根据地址可见.bss段位于此段，且.data在.bss下
uninitialized global static integer address:                    0x6020a4
initialized to zero global static integer address:              0x6020a8
initialized to no-zero global static integer address:           0x602090
//未初始化/初始化0/初始化非0全局变量，和全局静态变量一致
uninitialized global integer address:                           0x6020b4
initialized to zero global integer address:                     0x6020a0
initialized to no-zero global integer address:                  0x602094
//未初始化全局const变量在.data
uninitialized const global integer address:                     0x6020b8
//初始化为0/非0的全局const变量在.rodata段
initialized to zero const global integer address:               0x400d90
initialized to no-zero const global integer address:            0x400d94
//未初始化/初始化为0/初始化非0局部静态变量同全局变量/全局静态变量
uninitialized local static integer address:                     0x6020ac
initialized to zero local static integer address:               0x6020b0
initialized to no-zero local static integer address:            0x602098
//其他局部变量都在栈
uninitialized local const integer address:                      0x7ffc84997cc0
initialized to zero local const integer address:                0x7ffc84997cc4
uninitialized no-zero local const integer address:              0x7ffc84997cc8
local var:                                                      0x7ffc84997ccc

```

进程内存布局分析：

通过readelf+objdump分析进程可执行文件
查看可执行文件头
```cpp
zh@zh-virtual-machine:~/anything_test$ readelf -edSW ./a.out
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00
  Class:                             ELF64//64位
  Data:                              2's complement, little endian//小段字节序
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              EXEC (Executable file)//可执行文件
  Machine:                           Advanced Micro Devices X86-64
  Version:                           0x1
  Entry point address:               0x400890//程序入口地址
  Start of program headers:          64 (bytes into file)
  Start of section headers:          8704 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         9
  Size of section headers:           64 (bytes)
  Number of section headers:         30
  Section header string table index: 27

Section Headers:
  [Nr] Name              Type            Address          Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            0000000000000000 000000 000000 00      0   0  0
  [ 1] .interp           PROGBITS        0000000000400238 000238 00001c 00   A  0   0  1
  [ 2] .note.ABI-tag     NOTE            0000000000400254 000254 000020 00   A  0   0  4
  [ 3] .note.gnu.build-id NOTE            0000000000400274 000274 000024 00   A  0   0  4
  [ 4] .gnu.hash         GNU_HASH        0000000000400298 000298 000038 00   A  5   0  8
  [ 5] .dynsym           DYNSYM          00000000004002d0 0002d0 0001f8 18   A  6   1  8
  [ 6] .dynstr           STRTAB          00000000004004c8 0004c8 000124 00   A  0   0  1
  [ 7] .gnu.version      VERSYM          00000000004005ec 0005ec 00002a 02   A  5   0  2
  [ 8] .gnu.version_r    VERNEED         0000000000400618 000618 000050 00   A  6   2  8
  [ 9] .rela.dyn         RELA            0000000000400668 000668 000018 18   A  5   0  8
  [10] .rela.plt         RELA            0000000000400680 000680 000120 18   A  5  12  8
  [11] .init             PROGBITS        00000000004007a0 0007a0 00001a 00  AX  0   0  4
  [12] .plt              PROGBITS        00000000004007c0 0007c0 0000d0 10  AX  0   0 16
  [13] .text             PROGBITS        0000000000400890 000890 0004e2 00  AX  0   0 16//文本段起始地址
  [14] .fini             PROGBITS        0000000000400d74 000d74 000009 00  AX  0   0  4
  [15] .rodata           PROGBITS        0000000000400d80 000d80 000769 00   A  0   0  8//read only data段起始地址
  [16] .eh_frame_hdr     PROGBITS        00000000004014ec 0014ec 00003c 00   A  0   0  4
  [17] .eh_frame         PROGBITS        0000000000401528 001528 000114 00   A  0   0  8
  [18] .init_array       INIT_ARRAY      0000000000601df0 001df0 000008 00  WA  0   0  8
  [19] .fini_array       FINI_ARRAY      0000000000601df8 001df8 000008 00  WA  0   0  8
  [20] .jcr              PROGBITS        0000000000601e00 001e00 000008 00  WA  0   0  8
  [21] .dynamic          DYNAMIC         0000000000601e08 001e08 0001f0 10  WA  6   0  8
  [22] .got              PROGBITS        0000000000601ff8 001ff8 000008 08  WA  0   0  8
  [23] .got.plt          PROGBITS        0000000000602000 002000 000078 08  WA  0   0  8
  [24] .data             PROGBITS        0000000000602078 002078 000024 00  WA  0   0  8//data段起始地址
  [25] .bss              NOBITS          000000000060209c 00209c 000024 00  WA  0   0  4//bss段起始地址
  [26] .comment          PROGBITS        0000000000000000 00209c 000056 01  MS  0   0  1
  [27] .shstrtab         STRTAB          0000000000000000 0020f2 000108 00      0   0  1
  [28] .symtab           SYMTAB          0000000000000000 002980 000840 18     29  51  8
  [29] .strtab           STRTAB          0000000000000000 0031c0 000324 00      0   0  1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), l (large)
  I (info), L (link order), G (group), T (TLS), E (exclude), x (unknown)
  O (extra OS processing required) o (OS specific), p (processor specific)

Program Headers:
  Type           Offset   VirtAddr           PhysAddr           FileSiz  MemSiz   Flg Align
  PHDR           0x000040 0x0000000000400040 0x0000000000400040 0x0001f8 0x0001f8 R E 0x8
  INTERP         0x000238 0x0000000000400238 0x0000000000400238 0x00001c 0x00001c R   0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
  LOAD           0x000000 0x0000000000400000 0x0000000000400000 0x00163c 0x00163c R E 0x200000
  LOAD           0x001df0 0x0000000000601df0 0x0000000000601df0 0x0002ac 0x0002d0 RW  0x200000
  DYNAMIC        0x001e08 0x0000000000601e08 0x0000000000601e08 0x0001f0 0x0001f0 RW  0x8
  NOTE           0x000254 0x0000000000400254 0x0000000000400254 0x000044 0x000044 R   0x4
  GNU_EH_FRAME   0x0014ec 0x00000000004014ec 0x00000000004014ec 0x00003c 0x00003c R   0x4
  GNU_STACK      0x000000 0x0000000000000000 0x0000000000000000 0x000000 0x000000 RW  0x10
  GNU_RELRO      0x001df0 0x0000000000601df0 0x0000000000601df0 0x000210 0x000210 R   0x1

 Section to Segment mapping:
  Segment Sections...
   00
   01     .interp
   02     .interp .note.ABI-tag .note.gnu.build-id .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.dyn .rela.plt .init .plt .text .fini .rodata .eh_frame_hdr .eh_frame
   03     .init_array .fini_array .jcr .dynamic .got .got.plt .data .bss
   04     .dynamic
   05     .note.ABI-tag .note.gnu.build-id
   06     .eh_frame_hdr
   07
   08     .init_array .fini_array .jcr .dynamic .got

Dynamic section at offset 0x1e08 contains 26 entries:
  Tag        Type                         Name/Value
 0x0000000000000001 (NEEDED)             Shared library: [libfunc.so]//依赖动态库
 0x0000000000000001 (NEEDED)             Shared library: [libpthread.so.0]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
 0x000000000000000c (INIT)               0x4007a0
 0x000000000000000d (FINI)               0x400d74
 0x0000000000000019 (INIT_ARRAY)         0x601df0
 0x000000000000001b (INIT_ARRAYSZ)       8 (bytes)
 0x000000000000001a (FINI_ARRAY)         0x601df8
 0x000000000000001c (FINI_ARRAYSZ)       8 (bytes)
 0x000000006ffffef5 (GNU_HASH)           0x400298
 0x0000000000000005 (STRTAB)             0x4004c8
 0x0000000000000006 (SYMTAB)             0x4002d0
 0x000000000000000a (STRSZ)              292 (bytes)
 0x000000000000000b (SYMENT)             24 (bytes)
 0x0000000000000015 (DEBUG)              0x0
 0x0000000000000003 (PLTGOT)             0x602000
 0x0000000000000002 (PLTRELSZ)           288 (bytes)
 0x0000000000000014 (PLTREL)             RELA
 0x0000000000000017 (JMPREL)             0x400680
 0x0000000000000007 (RELA)               0x400668
 0x0000000000000008 (RELASZ)             24 (bytes)
 0x0000000000000009 (RELAENT)            24 (bytes)
 0x000000006ffffffe (VERNEED)            0x400618
 0x000000006fffffff (VERNEEDNUM)         2
 0x000000006ffffff0 (VERSYM)             0x4005ec
 0x0000000000000000 (NULL)               0x0


```
程序入口地址是0x400890，也就是.text段起始位置，依赖三个动态库

用hexdump 16进制和ascII打印可执行文件内容
```cpp
zh@zh-virtual-machine:~/anything_test$ hexdump -C -s 0x0  a.out
00000000  7f 45 4c 46 02 01 01 00  00 00 00 00 00 00 00 00  |.ELF............|
00000010  02 00 3e 00 01 00 00 00  90 08 40 00 00 00 00 00  |..>.......@.....|
00000020  40 00 00 00 00 00 00 00  00 22 00 00 00 00 00 00  |@........"......|
00000030  00 00 00 00 40 00 38 00  09 00 40 00 1e 00 1b 00  |....@.8...@.....|
00000040  06 00 00 00 05 00 00 00  40 00 00 00 00 00 00 00  |........@.......|
00000050  40 00 40 00 00 00 00 00  40 00 40 00 00 00 00 00  |@.@.....@.@.....|
00000060  f8 01 00 00 00 00 00 00  f8 01 00 00 00 00 00 00  |................|
00000070  08 00 00 00 00 00 00 00  03 00 00 00 04 00 00 00  |................|
00000080  38 02 00 00 00 00 00 00  38 02 40 00 00 00 00 00  |8.......8.@.....|
00000090  38 02 40 00 00 00 00 00  1c 00 00 00 00 00 00 00  |8.@.............|
000000a0  1c 00 00 00 00 00 00 00  01 00 00 00 00 00 00 00  |................|
000000b0  01 00 00 00 05 00 00 00  00 00 00 00 00 00 00 00  |................|
000000c0  00 00 40 00 00 00 00 00  00 00 40 00 00 00 00 00  |..@.......@.....|
000000d0  3c 16 00 00 00 00 00 00  3c 16 00 00 00 00 00 00  |<.......<.......|
000000e0  00 00 20 00 00 00 00 00  01 00 00 00 06 00 00 00  |.. .............|
000000f0  f0 1d 00 00 00 00 00 00  f0 1d 60 00 00 00 00 00  |..........`.....|
00000100  f0 1d 60 00 00 00 00 00  ac 02 00 00 00 00 00 00  |..`.............|
00000110  d0 02 00 00 00 00 00 00  00 00 20 00 00 00 00 00  |.......... .....|
00000120  02 00 00 00 06 00 00 00  08 1e 00 00 00 00 00 00  |................|
00000130  08 1e 60 00 00 00 00 00  08 1e 60 00 00 00 00 00  |..`.......`.....|
00000140  f0 01 00 00 00 00 00 00  f0 01 00 00 00 00 00 00  |................|
00000150  08 00 00 00 00 00 00 00  04 00 00 00 04 00 00 00  |................|
00000160  54 02 00 00 00 00 00 00  54 02 40 00 00 00 00 00  |T.......T.@.....|
00000170  54 02 40 00 00 00 00 00  44 00 00 00 00 00 00 00  |T.@.....D.......|
00000180  44 00 00 00 00 00 00 00  04 00 00 00 00 00 00 00  |D...............|
00000190  50 e5 74 64 04 00 00 00  ec 14 00 00 00 00 00 00  |P.td............|
000001a0  ec 14 40 00 00 00 00 00  ec 14 40 00 00 00 00 00  |..@.......@.....|
000001b0  3c 00 00 00 00 00 00 00  3c 00 00 00 00 00 00 00  |<.......<.......|
000001c0  04 00 00 00 00 00 00 00  51 e5 74 64 06 00 00 00  |........Q.td....|
000001d0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
000001f0  00 00 00 00 00 00 00 00  10 00 00 00 00 00 00 00  |................|
00000200  52 e5 74 64 04 00 00 00  f0 1d 00 00 00 00 00 00  |R.td............|
00000210  f0 1d 60 00 00 00 00 00  f0 1d 60 00 00 00 00 00  |..`.......`.....|
00000220  10 02 00 00 00 00 00 00  10 02 00 00 00 00 00 00  |................|
00000230  01 00 00 00 00 00 00 00  2f 6c 69 62 36 34 2f 6c  |......../lib64/l|
00000240  64 2d 6c 69 6e 75 78 2d  78 38 36 2d 36 34 2e 73  |d-linux-x86-64.s|
00000250  6f 2e 32 00 04 00 00 00  10 00 00 00 01 00 00 00  |o.2.............|
00000260  47 4e 55 00 00 00 00 00  02 00 00 00 06 00 00 00  |GNU.............|
00000270  18 00 00 00 04 00 00 00  14 00 00 00 03 00 00 00  |................|
00000280  47 4e 55 00 48 02 b2 84  c5 c4 3b 05 c0 6d 04 41  |GNU.H.....;..m.A|
00000290  18 63 87 78 6e 1f e0 cb  03 00 00 00 10 00 00 00  |.c.xn...........|
000002a0  01 00 00 00 06 00 00 00  88 c0 20 01 00 04 40 09  |.......... ...@.|
000002b0  10 00 00 00 12 00 00 00  14 00 00 00 42 45 d5 ec  |............BE..|
000002c0  bb e3 92 7c d8 71 58 1c  b9 8d f1 0e eb d3 ef 0e  |...|.qX.........|
000002d0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
000002e0  00 00 00 00 00 00 00 00  f1 00 00 00 12 00 00 00  |................|
000002f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000300  86 00 00 00 12 00 00 00  00 00 00 00 00 00 00 00  |................|
00000310  00 00 00 00 00 00 00 00  0c 00 00 00 20 00 00 00  |............ ...|
00000320  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000330  ac 00 00 00 12 00 00 00  00 00 00 00 00 00 00 00  |................|
00000340  00 00 00 00 00 00 00 00  c2 00 00 00 12 00 00 00  |................|
00000350  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000360  b1 00 00 00 12 00 00 00  00 00 00 00 00 00 00 00  |................|
00000370  00 00 00 00 00 00 00 00  c9 00 00 00 12 00 00 00  |................|
00000380  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000390  df 00 00 00 12 00 00 00  00 00 00 00 00 00 00 00  |................|
000003a0  00 00 00 00 00 00 00 00  d0 00 00 00 12 00 00 00  |................|
000003b0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
000003c0  28 00 00 00 20 00 00 00  00 00 00 00 00 00 00 00  |(... ...........|
000003d0  00 00 00 00 00 00 00 00  71 00 00 00 12 00 00 00  |........q.......|
000003e0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
000003f0  d8 00 00 00 12 00 00 00  00 00 00 00 00 00 00 00  |................|
00000400  00 00 00 00 00 00 00 00  95 00 00 00 12 00 00 00  |................|
00000410  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000420  37 00 00 00 20 00 00 00  00 00 00 00 00 00 00 00  |7... ...........|
00000430  00 00 00 00 00 00 00 00  4b 00 00 00 20 00 00 00  |........K... ...|
00000440  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000450  f6 00 00 00 10 00 18 00  9c 20 60 00 00 00 00 00  |......... `.....|
00000460  00 00 00 00 00 00 00 00  09 01 00 00 10 00 19 00  |................|
00000470  c0 20 60 00 00 00 00 00  00 00 00 00 00 00 00 00  |. `.............|
00000480  fd 00 00 00 10 00 19 00  9c 20 60 00 00 00 00 00  |......... `.....|
00000490  00 00 00 00 00 00 00 00  65 00 00 00 12 00 0b 00  |........e.......|
000004a0  a0 07 40 00 00 00 00 00  00 00 00 00 00 00 00 00  |..@.............|
000004b0  6b 00 00 00 12 00 0e 00  74 0d 40 00 00 00 00 00  |k.......t.@.....|
000004c0  00 00 00 00 00 00 00 00  00 6c 69 62 66 75 6e 63  |.........libfunc|
000004d0  2e 73 6f 00 5f 49 54 4d  5f 64 65 72 65 67 69 73  |.so._ITM_deregis|
000004e0  74 65 72 54 4d 43 6c 6f  6e 65 54 61 62 6c 65 00  |terTMCloneTable.|
000004f0  5f 5f 67 6d 6f 6e 5f 73  74 61 72 74 5f 5f 00 5f  |__gmon_start__._|
00000500  4a 76 5f 52 65 67 69 73  74 65 72 43 6c 61 73 73  |Jv_RegisterClass|
00000510  65 73 00 5f 49 54 4d 5f  72 65 67 69 73 74 65 72  |es._ITM_register|
00000520  54 4d 43 6c 6f 6e 65 54  61 62 6c 65 00 5f 69 6e  |TMCloneTable._in|
00000530  69 74 00 5f 66 69 6e 69  00 66 75 6e 63 00 6c 69  |it._fini.func.li|
00000540  62 70 74 68 72 65 61 64  2e 73 6f 2e 30 00 70 74  |bpthread.so.0.pt|
00000550  68 72 65 61 64 5f 63 72  65 61 74 65 00 70 74 68  |hread_create.pth|
00000560  72 65 61 64 5f 6a 6f 69  6e 00 6c 69 62 63 2e 73  |read_join.libc.s|
00000570  6f 2e 36 00 70 75 74 73  00 5f 5f 73 74 61 63 6b  |o.6.puts.__stack|
00000580  5f 63 68 6b 5f 66 61 69  6c 00 67 65 74 70 69 64  |_chk_fail.getpid|
00000590  00 70 72 69 6e 74 66 00  67 65 74 63 68 61 72 00  |.printf.getchar.|
000005a0  6d 61 6c 6c 6f 63 00 5f  5f 6c 69 62 63 5f 73 74  |malloc.__libc_st|
000005b0  61 72 74 5f 6d 61 69 6e  00 66 72 65 65 00 5f 65  |art_main.free._e|
000005c0  64 61 74 61 00 5f 5f 62  73 73 5f 73 74 61 72 74  |data.__bss_start|
000005d0  00 5f 65 6e 64 00 47 4c  49 42 43 5f 32 2e 32 2e  |._end.GLIBC_2.2.|
000005e0  35 00 47 4c 49 42 43 5f  32 2e 34 00 00 00 02 00  |5.GLIBC_2.4.....|
000005f0  03 00 00 00 02 00 02 00  04 00 02 00 02 00 02 00  |................|
00000600  00 00 00 00 02 00 03 00  00 00 00 00 01 00 01 00  |................|
00000610  01 00 01 00 01 00 00 00  01 00 01 00 76 00 00 00  |............v...|
00000620  10 00 00 00 20 00 00 00  75 1a 69 09 00 00 03 00  |.... ...u.i.....|
00000630  0e 01 00 00 00 00 00 00  01 00 02 00 a2 00 00 00  |................|
00000640  10 00 00 00 00 00 00 00  14 69 69 0d 00 00 04 00  |.........ii.....|
00000650  1a 01 00 00 10 00 00 00  75 1a 69 09 00 00 02 00  |........u.i.....|
00000660  0e 01 00 00 00 00 00 00  f8 1f 60 00 00 00 00 00  |..........`.....|
00000670  06 00 00 00 0a 00 00 00  00 00 00 00 00 00 00 00  |................|
00000680  18 20 60 00 00 00 00 00  07 00 00 00 01 00 00 00  |. `.............|
00000690  00 00 00 00 00 00 00 00  20 20 60 00 00 00 00 00  |........  `.....|
000006a0  07 00 00 00 02 00 00 00  00 00 00 00 00 00 00 00  |................|
000006b0  28 20 60 00 00 00 00 00  07 00 00 00 04 00 00 00  |( `.............|
...
00000bf0  c6 bf a8 13 40 00 b8 00  00 00 00 e8 20 fc ff ff  |....@....... ...|
00000c00  e8 3b fc ff ff bf 00 10  00 00 e8 61 fc ff ff 48  |.;.........a...H|
00000c10  89 45 e8 48 8b 45 e8 48  89 c6 bf f0 13 40 00 b8  |.E.H.E.H.....@..|
00000c20  00 00 00 00 e8 f7 fb ff  ff e8 12 fc ff ff bf 00  |................|
00000c30  00 01 00 e8 38 fc ff ff  48 89 45 f0 48 8b 45 f0  |....8...H.E.H.E.|
00000c40  48 89 c6 bf 38 14 40 00  b8 00 00 00 00 e8 ce fb  |H...8.@.........|
00000c50  ff ff e8 e9 fb ff ff bf  00 00 20 00 e8 0f fc ff  |.......... .....|
00000c60  ff 48 89 45 f8 48 8b 45  f8 48 89 c6 bf 80 14 40  |.H.E.H.E.H.....@|
00000c70  00 b8 00 00 00 00 e8 a5  fb ff ff e8 c0 fb ff ff  |................|
00000c80  bf c4 14 40 00 e8 66 fb  ff ff 48 8b 45 f8 48 89  |...@..f...H.E.H.|
00000c90  c7 e8 3a fb ff ff e8 a5  fb ff ff bf ce 14 40 00  |..:...........@.|
00000ca0  e8 4b fb ff ff b8 00 00  00 00 e8 b1 fb ff ff e8  |.K..............|
00000cb0  8c fb ff ff bf db 14 40  00 e8 32 fb ff ff 48 8d  |.......@..2...H.|
00000cc0  45 e0 b9 00 00 00 00 ba  7d 09 40 00 be 00 00 00  |E.......}.@.....|
00000cd0  00 48 89 c7 e8 07 fb ff  ff 48 8b 45 e0 be 00 00  |.H.......H.E....|
00000ce0  00 00 48 89 c7 e8 96 fb  ff ff e8 51 fb ff ff c9  |..H........Q....|
00000cf0  c3 66 2e 0f 1f 84 00 00  00 00 00 0f 1f 44 00 00  |.f...........D..|
00000d00  41 57 41 89 ff 41 56 49  89 f6 41 55 49 89 d5 41  |AWA..AVI..AUI..A|
00000d10  54 4c 8d 25 d8 10 20 00  55 48 8d 2d d8 10 20 00  |TL.%.. .UH.-.. .|
00000d20  53 4c 29 e5 31 db 48 c1  fd 03 48 83 ec 08 e8 6d  |SL).1.H...H....m|
00000d30  fa ff ff 48 85 ed 74 1e  0f 1f 84 00 00 00 00 00  |...H..t.........|
00000d40  4c 89 ea 4c 89 f6 44 89  ff 41 ff 14 dc 48 83 c3  |L..L..D..A...H..|
00000d50  01 48 39 eb 75 ea 48 83  c4 08 5b 5d 41 5c 41 5d  |.H9.u.H...[]A\A]|
00000d60  41 5e 41 5f c3 66 66 2e  0f 1f 84 00 00 00 00 00  |A^A_.ff.........|
00000d70  f3 c3 00 00 48 83 ec 08  48 83 c4 08 c3 00 00 00  |....H...H.......|
00000d80  01 00 02 00 00 00 00 00  68 65 6c 6c 6f 00 00 00  |........hello...|
00000d90  00 00 00 00 0a 00 00 00  61 6c 6c 6f 63 20 30 78  |........alloc 0x|
00000da0  31 30 30 30 30 20 62 79  74 65 73 20 73 74 61 63  |10000 bytes stac|
00000db0  6b 20 66 6f 72 20 66 75  6e 63 32 2c 20 62 75 66  |k for func2, buf|
00000dc0  20 73 74 61 72 74 20 61  64 64 72 3a 25 70 2c 20  | start addr:%p, |
00000dd0  65 6e 64 20 61 64 64 72  3a 25 70 0a 00 00 00 00  |end addr:%p.....|
00000de0  6d 61 6c 6c 6f 63 20 30  78 31 30 30 30 20 62 79  |malloc 0x1000 by|
00000df0  74 65 73 20 68 65 61 70  20 66 6f 72 20 66 75 6e  |tes heap for fun|
00000e00  63 32 2c 20 62 75 66 20  73 74 61 72 74 20 61 64  |c2, buf start ad|
00000e10  64 72 3a 25 70 2c 20 65  6e 64 20 61 64 64 72 3a  |dr:%p, end addr:|
00000e20  25 70 0a 00 66 75 6e 32  20 68 61 73 20 62 65 65  |%p..fun2 has bee|
00000e30  6e 20 6f 76 65 72 2c 20  66 72 65 65 20 68 65 61  |n over, free hea|
00000e40  70 00 00 00 00 00 00 00  63 75 72 72 65 6e 74 20  |p.......current |
00000e50  70 72 6f 63 65 73 73 20  70 69 64 3a 20 20 20 20  |process pid:    |
00000e60  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
*
00000e80  20 20 20 20 20 20 20 20  25 64 0a 00 00 00 00 00  |        %d......|
00000e90  61 72 67 63 3a 20 20 20  20 20 20 20 20 20 20 20  |argc:           |
00000ea0  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
*
00000ed0  25 70 2c 20 61 72 67 76  3a 25 70 0a 00 00 00 00  |%p, argv:%p.....|
00000ee0  67 6c 6f 62 61 6c 20 73  74 72 69 6e 67 20 70 6f  |global string po|
00000ef0  69 6e 74 20 61 64 64 72  65 73 73 3a 20 20 20 20  |int address:    |
00000f00  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
*
00000f20  25 70 0a 00 00 00 00 00  67 6c 6f 62 61 6c 20 73  |%p......global s|
00000f30  74 72 69 6e 67 20 61 64  64 72 65 73 73 3a 20 20  |tring address:  |
00000f40  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
*
00000f60  20 20 20 20 20 20 20 20  25 70 0a 00 00 00 00 00  |        %p......|
00000f70  75 6e 69 6e 69 74 69 61  6c 69 7a 65 64 20 67 6c  |uninitialized gl|
00000f80  6f 62 61 6c 20 73 74 61  74 69 63 20 69 6e 74 65  |obal static inte|
00000f90  67 65 72 20 61 64 64 72  65 73 73 3a 20 20 20 20  |ger address:    |
00000fa0  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
00000fb0  25 70 0a 00 00 00 00 00  69 6e 69 74 69 61 6c 69  |%p......initiali|
00000fc0  7a 65 64 20 74 6f 20 7a  65 72 6f 20 67 6c 6f 62  |zed to zero glob|
00000fd0  61 6c 20 73 74 61 74 69  63 20 69 6e 74 65 67 65  |al static intege|
00000fe0  72 20 61 64 64 72 65 73  73 3a 20 20 20 20 20 20  |r address:      |
00000ff0  20 20 20 20 20 20 20 20  25 70 0a 00 00 00 00 00  |        %p......|
00001000  69 6e 69 74 69 61 6c 69  7a 65 64 20 74 6f 20 6e  |initialized to n|
00001010  6f 2d 7a 65 72 6f 20 67  6c 6f 62 61 6c 20 73 74  |o-zero global st|
00001020  61 74 69 63 20 69 6e 74  65 67 65 72 20 61 64 64  |atic integer add|
00001030  72 65 73 73 3a 20 20 20  20 20 20 20 20 20 20 20  |ress:           |
00001040  25 70 0a 00 00 00 00 00  75 6e 69 6e 69 74 69 61  |%p......uninitia|
00001050  6c 69 7a 65 64 20 67 6c  6f 62 61 6c 20 69 6e 74  |lized global int|
00001060  65 67 65 72 20 61 64 64  72 65 73 73 3a 20 20 20  |eger address:   |
00001070  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
00001080  20 20 20 20 20 20 20 20  25 70 0a 00 00 00 00 00  |        %p......|
00001090  69 6e 69 74 69 61 6c 69  7a 65 64 20 74 6f 20 7a  |initialized to z|
000010a0  65 72 6f 20 67 6c 6f 62  61 6c 20 69 6e 74 65 67  |ero global integ|
000010b0  65 72 20 61 64 64 72 65  73 73 3a 20 20 20 20 20  |er address:     |
000010c0  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
000010d0  25 70 0a 00 00 00 00 00  69 6e 69 74 69 61 6c 69  |%p......initiali|
000010e0  7a 65 64 20 74 6f 20 6e  6f 2d 7a 65 72 6f 20 67  |zed to no-zero g|
000010f0  6c 6f 62 61 6c 20 69 6e  74 65 67 65 72 20 61 64  |lobal integer ad|
00001100  64 72 65 73 73 3a 20 20  20 20 20 20 20 20 20 20  |dress:          |
00001110  20 20 20 20 20 20 20 20  25 70 0a 00 00 00 00 00  |        %p......|
00001120  75 6e 69 6e 69 74 69 61  6c 69 7a 65 64 20 63 6f  |uninitialized co|
00001130  6e 73 74 20 67 6c 6f 62  61 6c 20 69 6e 74 65 67  |nst global integ|
00001140  65 72 20 61 64 64 72 65  73 73 3a 20 20 20 20 20  |er address:     |
00001150  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
00001160  25 70 0a 00 00 00 00 00  69 6e 69 74 69 61 6c 69  |%p......initiali|
00001170  7a 65 64 20 74 6f 20 7a  65 72 6f 20 63 6f 6e 73  |zed to zero cons|
00001180  74 20 67 6c 6f 62 61 6c  20 69 6e 74 65 67 65 72  |t global integer|
00001190  20 61 64 64 72 65 73 73  3a 20 20 20 20 20 20 20  | address:       |
000011a0  20 20 20 20 20 20 20 20  25 70 0a 00 00 00 00 00  |        %p......|
000011b0  69 6e 69 74 69 61 6c 69  7a 65 64 20 74 6f 20 6e  |initialized to n|
000011c0  6f 2d 7a 65 72 6f 20 63  6f 6e 73 74 20 67 6c 6f  |o-zero const glo|
000011d0  62 61 6c 20 69 6e 74 65  67 65 72 20 61 64 64 72  |bal integer addr|
000011e0  65 73 73 3a 20 20 20 20  20 20 20 20 20 20 20 20  |ess:            |
000011f0  25 70 0a 00 00 00 00 00  75 6e 69 6e 69 74 69 61  |%p......uninitia|
00001200  6c 69 7a 65 64 20 6c 6f  63 61 6c 20 73 74 61 74  |lized local stat|
00001210  69 63 20 69 6e 74 65 67  65 72 20 61 64 64 72 65  |ic integer addre|
00001220  73 73 3a 20 20 20 20 20  20 20 20 20 20 20 20 20  |ss:             |
00001230  20 20 20 20 20 20 20 20  25 70 0a 00 00 00 00 00  |        %p......|
00001240  69 6e 69 74 69 61 6c 69  7a 65 64 20 74 6f 20 7a  |initialized to z|
00001250  65 72 6f 20 6c 6f 63 61  6c 20 73 74 61 74 69 63  |ero local static|
00001260  20 69 6e 74 65 67 65 72  20 61 64 64 72 65 73 73  | integer address|
00001270  3a 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |:               |
00001280  25 70 0a 00 00 00 00 00  69 6e 69 74 69 61 6c 69  |%p......initiali|
00001290  7a 65 64 20 74 6f 20 6e  6f 2d 7a 65 72 6f 20 6c  |zed to no-zero l|
000012a0  6f 63 61 6c 20 73 74 61  74 69 63 20 69 6e 74 65  |ocal static inte|
000012b0  67 65 72 20 61 64 64 72  65 73 73 3a 20 20 20 20  |ger address:    |
000012c0  20 20 20 20 20 20 20 20  25 70 0a 00 00 00 00 00  |        %p......|
000012d0  75 6e 69 6e 69 74 69 61  6c 69 7a 65 64 20 6c 6f  |uninitialized lo|
000012e0  63 61 6c 20 63 6f 6e 73  74 20 69 6e 74 65 67 65  |cal const intege|
000012f0  72 20 61 64 64 72 65 73  73 3a 20 20 20 20 20 20  |r address:      |
00001300  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
00001310  25 70 0a 00 00 00 00 00  69 6e 69 74 69 61 6c 69  |%p......initiali|
00001320  7a 65 64 20 74 6f 20 7a  65 72 6f 20 6c 6f 63 61  |zed to zero loca|
00001330  6c 20 63 6f 6e 73 74 20  69 6e 74 65 67 65 72 20  |l const integer |
00001340  61 64 64 72 65 73 73 3a  20 20 20 20 20 20 20 20  |address:        |
00001350  20 20 20 20 20 20 20 20  25 70 0a 00 00 00 00 00  |        %p......|
00001360  75 6e 69 6e 69 74 69 61  6c 69 7a 65 64 20 6e 6f  |uninitialized no|
00001370  2d 7a 65 72 6f 20 6c 6f  63 61 6c 20 63 6f 6e 73  |-zero local cons|
00001380  74 20 69 6e 74 65 67 65  72 20 61 64 64 72 65 73  |t integer addres|
00001390  73 3a 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |s:              |
000013a0  25 70 0a 00 00 00 00 00  6c 6f 63 61 6c 20 76 61  |%p......local va|
000013b0  72 3a 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |r:              |
000013c0  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
*
000013e0  20 20 20 20 20 20 20 20  25 70 0a 00 00 00 00 00  |        %p......|
000013f0  6d 61 6c 6c 6f 63 20 30  78 31 30 30 30 20 62 79  |malloc 0x1000 by|
00001400  74 65 73 20 66 6f 72 20  70 74 72 31 2c 20 73 74  |tes for ptr1, st|
00001410  61 72 74 20 61 64 64 72  65 73 73 3a 20 20 20 20  |art address:    |
00001420  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
00001430  25 70 0a 00 00 00 00 00  6d 61 6c 6c 6f 63 20 30  |%p......malloc 0|
00001440  78 31 30 30 30 30 20 62  79 74 65 73 20 66 6f 72  |x10000 bytes for|
00001450  20 70 74 72 32 2c 20 73  74 61 72 74 20 61 64 64  | ptr2, start add|
00001460  72 65 73 73 3a 20 20 20  20 20 20 20 20 20 20 20  |ress:           |
00001470  20 20 20 20 20 20 20 25  70 0a 00 00 00 00 00 00  |       %p.......|
00001480  6d 61 6c 6c 6f 63 20 32  4d 20 62 79 74 65 73 20  |malloc 2M bytes |
00001490  66 6f 72 20 70 74 72 33  2c 20 73 74 61 72 74 20  |for ptr3, start |
000014a0  61 64 64 72 65 73 73 3a  20 20 20 20 20 20 20 20  |address:        |
000014b0  20 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  |                |
000014c0  25 70 0a 00 66 72 65 65  20 70 74 72 33 00 72 75  |%p..free ptr3.ru|
000014d0  6e 6e 69 6e 67 20 66 75  6e 63 00 72 75 6e 6e 69  |nning func.runni|
000014e0  6e 67 20 66 75 6e 63 32  00 00 00 00 01 1b 03 3b  |ng func2.......;|
000014f0  38 00 00 00 06 00 00 00  d4 f2 ff ff 84 00 00 00  |8...............|
00001500  a4 f3 ff ff 54 00 00 00  91 f4 ff ff ac 00 00 00  |....T...........|
00001510  4f f5 ff ff cc 00 00 00  14 f8 ff ff ec 00 00 00  |O...............|
00001520  84 f8 ff ff 34 01 00 00  14 00 00 00 00 00 00 00  |....4...........|
00001530  01 7a 52 00 01 78 10 01  1b 0c 07 08 90 01 07 10  |.zR..x..........|
00001540  14 00 00 00 1c 00 00 00  48 f3 ff ff 2a 00 00 00  |........H...*...|
00001550  00 00 00 00 00 00 00 00  14 00 00 00 00 00 00 00  |................|
00001560  01 7a 52 00 01 78 10 01  1b 0c 07 08 90 01 00 00  |.zR..x..........|
...
00002090  0a 00 00 00 14 00 00 00  0a 00 00 00 47 43 43 3a  |............GCC:|
000020a0  20 28 55 62 75 6e 74 75  20 34 2e 38 2e 34 2d 32  | (Ubuntu 4.8.4-2|
000020b0  75 62 75 6e 74 75 31 7e  31 34 2e 30 34 2e 34 29  |ubuntu1~14.04.4)|
000020c0  20 34 2e 38 2e 34 00 47  43 43 3a 20 28 55 62 75  | 4.8.4.GCC: (Ubu|
000020d0  6e 74 75 20 34 2e 38 2e  34 2d 32 75 62 75 6e 74  |ntu 4.8.4-2ubunt|
000020e0  75 31 7e 31 34 2e 30 34  2e 33 29 20 34 2e 38 2e  |u1~14.04.3) 4.8.|
000020f0  34 00 00 2e 73 79 6d 74  61 62 00 2e 73 74 72 74  |4...symtab..strt|
00002100  61 62 00 2e 73 68 73 74  72 74 61 62 00 2e 69 6e  |ab..shstrtab..in|
00002110  74 65 72 70 00 2e 6e 6f  74 65 2e 41 42 49 2d 74  |terp..note.ABI-t|
00002120  61 67 00 2e 6e 6f 74 65  2e 67 6e 75 2e 62 75 69  |ag..note.gnu.bui|
00002130  6c 64 2d 69 64 00 2e 67  6e 75 2e 68 61 73 68 00  |ld-id..gnu.hash.|
00002140  2e 64 79 6e 73 79 6d 00  2e 64 79 6e 73 74 72 00  |.dynsym..dynstr.|
00002150  2e 67 6e 75 2e 76 65 72  73 69 6f 6e 00 2e 67 6e  |.gnu.version..gn|
00002160  75 2e 76 65 72 73 69 6f  6e 5f 72 00 2e 72 65 6c  |u.version_r..rel|
00002170  61 2e 64 79 6e 00 2e 72  65 6c 61 2e 70 6c 74 00  |a.dyn..rela.plt.|
00002180  2e 69 6e 69 74 00 2e 74  65 78 74 00 2e 66 69 6e  |.init..text..fin|
00002190  69 00 2e 72 6f 64 61 74  61 00 2e 65 68 5f 66 72  |i..rodata..eh_fr|
000021a0  61 6d 65 5f 68 64 72 00  2e 65 68 5f 66 72 61 6d  |ame_hdr..eh_fram|
000021b0  65 00 2e 69 6e 69 74 5f  61 72 72 61 79 00 2e 66  |e..init_array..f|
000021c0  69 6e 69 5f 61 72 72 61  79 00 2e 6a 63 72 00 2e  |ini_array..jcr..|
000021d0  64 79 6e 61 6d 69 63 00  2e 67 6f 74 00 2e 67 6f  |dynamic..got..go|
000021e0  74 2e 70 6c 74 00 2e 64  61 74 61 00 2e 62 73 73  |t.plt..data..bss|
000021f0  00 2e 63 6f 6d 6d 65 6e  74 00 00 00 00 00 00 00  |..comment.......|
00002200  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00002240  1b 00 00 00 01 00 00 00  02 00 00 00 00 00 00 00  |................|
00002250  38 02 40 00 00 00 00 00  38 02 00 00 00 00 00 00  |8.@.....8.......|
00002260  1c 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00002270  01 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00002280  23 00 00 00 07 00 00 00  02 00 00 00 00 00 00 00  |#...............|
00002290  54 02 40 00 00 00 00 00  54 02 00 00 00 00 00 00  |T.@.....T.......|
000022a0  20 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  | ...............|
000022b0  04 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
000022c0  31 00 00 00 07 00 00 00  02 00 00 00 00 00 00 00  |1...............|
000022d0  74 02 40 00 00 00 00 00  74 02 00 00 00 00 00 00  |t.@.....t.......|
000022e0  24 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |$...............|
000022f0  04 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00002300  44 00 00 00 f6 ff ff 6f  02 00 00 00 00 00 00 00  |D......o........|
00002310  98 02 40 00 00 00 00 00  98 02 00 00 00 00 00 00  |..@.............|
00002320  38 00 00 00 00 00 00 00  05 00 00 00 00 00 00 00  |8...............|
00002330  08 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00002340  4e 00 00 00 0b 00 00 00  02 00 00 00 00 00 00 00  |N...............|
00002350  d0 02 40 00 00 00 00 00  d0 02 00 00 00 00 00 00  |..@.............|
00002360  f8 01 00 00 00 00 00 00  06 00 00 00 01 00 00 00  |................|
00002370  08 00 00 00 00 00 00 00  18 00 00 00 00 00 00 00  |................|
00002380  56 00 00 00 03 00 00 00  02 00 00 00 00 00 00 00  |V...............|
00002390  c8 04 40 00 00 00 00 00  c8 04 00 00 00 00 00 00  |..@.............|
000023a0  24 01 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |$...............|
000023b0  01 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
000023c0  5e 00 00 00 ff ff ff 6f  02 00 00 00 00 00 00 00  |^......o........|
...
000031a0  00 00 00 00 00 00 00 00  1e 03 00 00 12 00 0b 00  |................|
000031b0  a0 07 40 00 00 00 00 00  00 00 00 00 00 00 00 00  |..@.............|
000031c0  00 63 72 74 73 74 75 66  66 2e 63 00 5f 5f 4a 43  |.crtstuff.c.__JC|
000031d0  52 5f 4c 49 53 54 5f 5f  00 64 65 72 65 67 69 73  |R_LIST__.deregis|
000031e0  74 65 72 5f 74 6d 5f 63  6c 6f 6e 65 73 00 72 65  |ter_tm_clones.re|
000031f0  67 69 73 74 65 72 5f 74  6d 5f 63 6c 6f 6e 65 73  |gister_tm_clones|
00003200  00 5f 5f 64 6f 5f 67 6c  6f 62 61 6c 5f 64 74 6f  |.__do_global_dto|
00003210  72 73 5f 61 75 78 00 63  6f 6d 70 6c 65 74 65 64  |rs_aux.completed|
00003220  2e 36 39 38 32 00 5f 5f  64 6f 5f 67 6c 6f 62 61  |.6982.__do_globa|
00003230  6c 5f 64 74 6f 72 73 5f  61 75 78 5f 66 69 6e 69  |l_dtors_aux_fini|
00003240  5f 61 72 72 61 79 5f 65  6e 74 72 79 00 66 72 61  |_array_entry.fra|
00003250  6d 65 5f 64 75 6d 6d 79  00 5f 5f 66 72 61 6d 65  |me_dummy.__frame|
00003260  5f 64 75 6d 6d 79 5f 69  6e 69 74 5f 61 72 72 61  |_dummy_init_arra|
00003270  79 5f 65 6e 74 72 79 00  74 65 73 74 32 2e 63 00  |y_entry.test2.c.|
00003280  61 00 62 00 63 00 6d 2e  33 39 36 36 00 6e 2e 33  |a.b.c.m.3966.n.3|
00003290  39 36 37 00 6f 2e 33 39  36 38 00 5f 5f 46 52 41  |967.o.3968.__FRA|
000032a0  4d 45 5f 45 4e 44 5f 5f  00 5f 5f 4a 43 52 5f 45  |ME_END__.__JCR_E|
000032b0  4e 44 5f 5f 00 5f 5f 69  6e 69 74 5f 61 72 72 61  |ND__.__init_arra|
000032c0  79 5f 65 6e 64 00 5f 44  59 4e 41 4d 49 43 00 5f  |y_end._DYNAMIC._|
000032d0  5f 69 6e 69 74 5f 61 72  72 61 79 5f 73 74 61 72  |_init_array_star|
000032e0  74 00 5f 47 4c 4f 42 41  4c 5f 4f 46 46 53 45 54  |t._GLOBAL_OFFSET|
000032f0  5f 54 41 42 4c 45 5f 00  5f 5f 6c 69 62 63 5f 63  |_TABLE_.__libc_c|
00003300  73 75 5f 66 69 6e 69 00  66 72 65 65 40 40 47 4c  |su_fini.free@@GL|
00003310  49 42 43 5f 32 2e 32 2e  35 00 70 74 68 72 65 61  |IBC_2.2.5.pthrea|
00003320  64 5f 63 72 65 61 74 65  40 40 47 4c 49 42 43 5f  |d_create@@GLIBC_|
00003330  32 2e 32 2e 35 00 5f 49  54 4d 5f 64 65 72 65 67  |2.2.5._ITM_dereg|
00003340  69 73 74 65 72 54 4d 43  6c 6f 6e 65 54 61 62 6c  |isterTMCloneTabl|
00003350  65 00 64 61 74 61 5f 73  74 61 72 74 00 70 75 74  |e.data_start.put|
00003360  73 40 40 47 4c 49 42 43  5f 32 2e 32 2e 35 00 64  |s@@GLIBC_2.2.5.d|
00003370  00 6c 00 67 65 74 70 69  64 40 40 47 4c 49 42 43  |.l.getpid@@GLIBC|
00003380  5f 32 2e 32 2e 35 00 68  00 5f 65 64 61 74 61 00  |_2.2.5.h._edata.|
00003390  5f 66 69 6e 69 00 5f 5f  73 74 61 63 6b 5f 63 68  |_fini.__stack_ch|
000033a0  6b 5f 66 61 69 6c 40 40  47 4c 49 42 43 5f 32 2e  |k_fail@@GLIBC_2.|
000033b0  34 00 66 00 70 72 69 6e  74 66 40 40 47 4c 49 42  |4.f.printf@@GLIB|
000033c0  43 5f 32 2e 32 2e 35 00  5f 5f 6c 69 62 63 5f 73  |C_2.2.5.__libc_s|
000033d0  74 61 72 74 5f 6d 61 69  6e 40 40 47 4c 49 42 43  |tart_main@@GLIBC|
000033e0  5f 32 2e 32 2e 35 00 5f  5f 64 61 74 61 5f 73 74  |_2.2.5.__data_st|
000033f0  61 72 74 00 67 65 74 63  68 61 72 40 40 47 4c 49  |art.getchar@@GLI|
00003400  42 43 5f 32 2e 32 2e 35  00 5f 5f 67 6d 6f 6e 5f  |BC_2.2.5.__gmon_|
00003410  73 74 61 72 74 5f 5f 00  5f 5f 64 73 6f 5f 68 61  |start__.__dso_ha|
00003420  6e 64 6c 65 00 67 5f 73  74 72 00 5f 49 4f 5f 73  |ndle.g_str._IO_s|
00003430  74 64 69 6e 5f 75 73 65  64 00 66 75 6e 63 32 00  |tdin_used.func2.|
00003440  66 75 6e 63 00 5f 5f 6c  69 62 63 5f 63 73 75 5f  |func.__libc_csu_|
00003450  69 6e 69 74 00 6d 61 6c  6c 6f 63 40 40 47 4c 49  |init.malloc@@GLI|
00003460  42 43 5f 32 2e 32 2e 35  00 5f 65 6e 64 00 5f 73  |BC_2.2.5._end._s|
00003470  74 61 72 74 00 67 00 65  00 5f 5f 62 73 73 5f 73  |tart.g.e.__bss_s|
00003480  74 61 72 74 00 6d 61 69  6e 00 70 74 68 72 65 61  |tart.main.pthrea|
00003490  64 5f 6a 6f 69 6e 40 40  47 4c 49 42 43 5f 32 2e  |d_join@@GLIBC_2.|
000034a0  32 2e 35 00 5f 4a 76 5f  52 65 67 69 73 74 65 72  |2.5._Jv_Register|
000034b0  43 6c 61 73 73 65 73 00  5f 5f 54 4d 43 5f 45 4e  |Classes.__TMC_EN|
000034c0  44 5f 5f 00 5f 49 54 4d  5f 72 65 67 69 73 74 65  |D__._ITM_registe|
000034d0  72 54 4d 43 6c 6f 6e 65  54 61 62 6c 65 00 5f 69  |rTMCloneTable._i|
000034e0  6e 69 74 00                                       |nit.|
000034e4

```
hexdump用来对二进制文件进行格式化输出

objdump使用：
```cpp
zh@zh-virtual-machine:~/anything_test$ objdump -x a.out//常用

a.out:     file format elf64-x86-64
a.out
architecture: i386:x86-64, flags 0x00000112:
EXEC_P, HAS_SYMS, D_PAGED
start address 0x0000000000400890

Program Header:
    PHDR off    0x0000000000000040 vaddr 0x0000000000400040 paddr 0x0000000000400040 align 2**3
         filesz 0x00000000000001f8 memsz 0x00000000000001f8 flags r-x
  INTERP off    0x0000000000000238 vaddr 0x0000000000400238 paddr 0x0000000000400238 align 2**0
         filesz 0x000000000000001c memsz 0x000000000000001c flags r--
    LOAD off    0x0000000000000000 vaddr 0x0000000000400000 paddr 0x0000000000400000 align 2**21
         filesz 0x000000000000163c memsz 0x000000000000163c flags r-x
    LOAD off    0x0000000000001df0 vaddr 0x0000000000601df0 paddr 0x0000000000601df0 align 2**21
         filesz 0x00000000000002ac memsz 0x00000000000002d0 flags rw-
 DYNAMIC off    0x0000000000001e08 vaddr 0x0000000000601e08 paddr 0x0000000000601e08 align 2**3
         filesz 0x00000000000001f0 memsz 0x00000000000001f0 flags rw-
    NOTE off    0x0000000000000254 vaddr 0x0000000000400254 paddr 0x0000000000400254 align 2**2
         filesz 0x0000000000000044 memsz 0x0000000000000044 flags r--
EH_FRAME off    0x00000000000014ec vaddr 0x00000000004014ec paddr 0x00000000004014ec align 2**2
         filesz 0x000000000000003c memsz 0x000000000000003c flags r--
   STACK off    0x0000000000000000 vaddr 0x0000000000000000 paddr 0x0000000000000000 align 2**4
         filesz 0x0000000000000000 memsz 0x0000000000000000 flags rw-
   RELRO off    0x0000000000001df0 vaddr 0x0000000000601df0 paddr 0x0000000000601df0 align 2**0
         filesz 0x0000000000000210 memsz 0x0000000000000210 flags r--

Dynamic Section:
  NEEDED               libfunc.so
  NEEDED               libpthread.so.0
  NEEDED               libc.so.6
  INIT                 0x00000000004007a0
  FINI                 0x0000000000400d74
  INIT_ARRAY           0x0000000000601df0
  INIT_ARRAYSZ         0x0000000000000008
  FINI_ARRAY           0x0000000000601df8
  FINI_ARRAYSZ         0x0000000000000008
  GNU_HASH             0x0000000000400298
  STRTAB               0x00000000004004c8
  SYMTAB               0x00000000004002d0
  STRSZ                0x0000000000000124
  SYMENT               0x0000000000000018
  DEBUG                0x0000000000000000
  PLTGOT               0x0000000000602000
  PLTRELSZ             0x0000000000000120
  PLTREL               0x0000000000000007
  JMPREL               0x0000000000400680
  RELA                 0x0000000000400668
  RELASZ               0x0000000000000018
  RELAENT              0x0000000000000018
  VERNEED              0x0000000000400618
  VERNEEDNUM           0x0000000000000002
  VERSYM               0x00000000004005ec

Version References:
  required from libpthread.so.0:
    0x09691a75 0x00 03 GLIBC_2.2.5
  required from libc.so.6:
    0x0d696914 0x00 04 GLIBC_2.4
    0x09691a75 0x00 02 GLIBC_2.2.5

Sections:
Idx Name          Size      VMA               LMA               File off  Algn
  0 .interp       0000001c  0000000000400238  0000000000400238  00000238  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  1 .note.ABI-tag 00000020  0000000000400254  0000000000400254  00000254  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  2 .note.gnu.build-id 00000024  0000000000400274  0000000000400274  00000274  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .gnu.hash     00000038  0000000000400298  0000000000400298  00000298  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .dynsym       000001f8  00000000004002d0  00000000004002d0  000002d0  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  5 .dynstr       00000124  00000000004004c8  00000000004004c8  000004c8  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  6 .gnu.version  0000002a  00000000004005ec  00000000004005ec  000005ec  2**1
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  7 .gnu.version_r 00000050  0000000000400618  0000000000400618  00000618  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  8 .rela.dyn     00000018  0000000000400668  0000000000400668  00000668  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  9 .rela.plt     00000120  0000000000400680  0000000000400680  00000680  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
 10 .init         0000001a  00000000004007a0  00000000004007a0  000007a0  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
 11 .plt          000000d0  00000000004007c0  00000000004007c0  000007c0  2**4
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
 12 .text         000004e2  0000000000400890  0000000000400890  00000890  2**4
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
 13 .fini         00000009  0000000000400d74  0000000000400d74  00000d74  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
 14 .rodata       00000769  0000000000400d80  0000000000400d80  00000d80  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
 15 .eh_frame_hdr 0000003c  00000000004014ec  00000000004014ec  000014ec  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
 16 .eh_frame     00000114  0000000000401528  0000000000401528  00001528  2**3
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
 17 .init_array   00000008  0000000000601df0  0000000000601df0  00001df0  2**3
                  CONTENTS, ALLOC, LOAD, DATA
 18 .fini_array   00000008  0000000000601df8  0000000000601df8  00001df8  2**3
                  CONTENTS, ALLOC, LOAD, DATA
 19 .jcr          00000008  0000000000601e00  0000000000601e00  00001e00  2**3
                  CONTENTS, ALLOC, LOAD, DATA
 20 .dynamic      000001f0  0000000000601e08  0000000000601e08  00001e08  2**3
                  CONTENTS, ALLOC, LOAD, DATA
 21 .got          00000008  0000000000601ff8  0000000000601ff8  00001ff8  2**3
                  CONTENTS, ALLOC, LOAD, DATA
 22 .got.plt      00000078  0000000000602000  0000000000602000  00002000  2**3
                  CONTENTS, ALLOC, LOAD, DATA
 23 .data         00000024  0000000000602078  0000000000602078  00002078  2**3
                  CONTENTS, ALLOC, LOAD, DATA
 24 .bss          00000024  000000000060209c  000000000060209c  0000209c  2**2
                  ALLOC
 25 .comment      00000056  0000000000000000  0000000000000000  0000209c  2**0
                  CONTENTS, READONLY
SYMBOL TABLE:
0000000000400238 l    d  .interp    0000000000000000              .interp
0000000000400254 l    d  .note.ABI-tag  0000000000000000              .note.ABI-tag
0000000000400274 l    d  .note.gnu.build-id     0000000000000000              .note.gnu.build-id
0000000000400298 l    d  .gnu.hash  0000000000000000              .gnu.hash
00000000004002d0 l    d  .dynsym    0000000000000000              .dynsym
00000000004004c8 l    d  .dynstr    0000000000000000              .dynstr
00000000004005ec l    d  .gnu.version   0000000000000000              .gnu.version
0000000000400618 l    d  .gnu.version_r 0000000000000000              .gnu.version_r
0000000000400668 l    d  .rela.dyn  0000000000000000              .rela.dyn
0000000000400680 l    d  .rela.plt  0000000000000000              .rela.plt
00000000004007a0 l    d  .init  0000000000000000              .init
00000000004007c0 l    d  .plt   0000000000000000              .plt
0000000000400890 l    d  .text  0000000000000000              .text
0000000000400d74 l    d  .fini  0000000000000000              .fini
0000000000400d80 l    d  .rodata    0000000000000000              .rodata
00000000004014ec l    d  .eh_frame_hdr  0000000000000000              .eh_frame_hdr
0000000000401528 l    d  .eh_frame  0000000000000000              .eh_frame
0000000000601df0 l    d  .init_array    0000000000000000              .init_array
0000000000601df8 l    d  .fini_array    0000000000000000              .fini_array
0000000000601e00 l    d  .jcr   0000000000000000              .jcr
0000000000601e08 l    d  .dynamic   0000000000000000              .dynamic
0000000000601ff8 l    d  .got   0000000000000000              .got
0000000000602000 l    d  .got.plt   0000000000000000              .got.plt
0000000000602078 l    d  .data  0000000000000000              .data
000000000060209c l    d  .bss   0000000000000000              .bss
0000000000000000 l    d  .comment   0000000000000000              .comment
0000000000000000 l    df *ABS*  0000000000000000              crtstuff.c
0000000000601e00 l     O .jcr   0000000000000000              __JCR_LIST__
00000000004008c0 l     F .text  0000000000000000              deregister_tm_clones
00000000004008f0 l     F .text  0000000000000000              register_tm_clones
0000000000400930 l     F .text  0000000000000000              __do_global_dtors_aux
000000000060209c l     O .bss   0000000000000001              completed.6982
0000000000601df8 l     O .fini_array    0000000000000000              __do_global_dtors_aux_fini_array_entry
0000000000400950 l     F .text  0000000000000000              frame_dummy
0000000000601df0 l     O .init_array    0000000000000000              __frame_dummy_init_array_entry
0000000000000000 l    df *ABS*  0000000000000000              test2.c
00000000006020a4 l     O .bss   0000000000000004              a
00000000006020a8 l     O .bss   0000000000000004              b
0000000000602090 l     O .data  0000000000000004              c
00000000006020ac l     O .bss   0000000000000004              m.3966
00000000006020b0 l     O .bss   0000000000000004              n.3967
0000000000602098 l     O .data  0000000000000004              o.3968
0000000000000000 l    df *ABS*  0000000000000000              crtstuff.c
0000000000401638 l     O .eh_frame  0000000000000000              __FRAME_END__
0000000000601e00 l     O .jcr   0000000000000000              __JCR_END__
0000000000000000 l    df *ABS*  0000000000000000
0000000000601df8 l       .init_array    0000000000000000              __init_array_end
0000000000601e08 l     O .dynamic   0000000000000000              _DYNAMIC
0000000000601df0 l       .init_array    0000000000000000              __init_array_start
0000000000602000 l     O .got.plt   0000000000000000              _GLOBAL_OFFSET_TABLE_
0000000000400d70 g     F .text  0000000000000002              __libc_csu_fini
0000000000000000       F *UND*  0000000000000000              free@@GLIBC_2.2.5
0000000000000000       F *UND*  0000000000000000              pthread_create@@GLIBC_2.2.5
0000000000000000  w      *UND*  0000000000000000              _ITM_deregisterTMCloneTable
0000000000602078  w      .data  0000000000000000              data_start
0000000000000000       F *UND*  0000000000000000              puts@@GLIBC_2.2.5
00000000006020b4 g     O .bss   0000000000000004              d
0000000000400d94 g     O .rodata    0000000000000004              l
0000000000000000       F *UND*  0000000000000000              getpid@@GLIBC_2.2.5
0000000000400d90 g     O .rodata    0000000000000004              h
000000000060209c g       .data  0000000000000000              _edata
0000000000400d74 g     F .fini  0000000000000000              _fini
0000000000000000       F *UND*  0000000000000000              __stack_chk_fail@@GLIBC_2.4
0000000000602094 g     O .data  0000000000000004              f
0000000000000000       F *UND*  0000000000000000              printf@@GLIBC_2.2.5
0000000000000000       F *UND*  0000000000000000              __libc_start_main@@GLIBC_2.2.5
0000000000602078 g       .data  0000000000000000              __data_start
0000000000000000       F *UND*  0000000000000000              getchar@@GLIBC_2.2.5
0000000000000000  w      *UND*  0000000000000000              __gmon_start__
0000000000602080 g     O .data  0000000000000000              .hidden __dso_handle
0000000000602088 g     O .data  0000000000000008              g_str
0000000000400d80 g     O .rodata    0000000000000004              _IO_stdin_used
000000000040097d g     F .text  00000000000000be              func2
0000000000000000       F *UND*  0000000000000000              func
0000000000400d00 g     F .text  0000000000000065              __libc_csu_init
0000000000000000       F *UND*  0000000000000000              malloc@@GLIBC_2.2.5
00000000006020c0 g       .bss   0000000000000000              _end
0000000000400890 g     F .text  0000000000000000              _start
00000000006020b8 g     O .bss   0000000000000004              g
00000000006020a0 g     O .bss   0000000000000004              e
000000000060209c g       .bss   0000000000000000              __bss_start
0000000000400a3b g     F .text  00000000000002b6              main
0000000000000000       F *UND*  0000000000000000              pthread_join@@GLIBC_2.2.5
0000000000000000  w      *UND*  0000000000000000              _Jv_RegisterClasses
00000000006020a0 g     O .data  0000000000000000              .hidden __TMC_END__
0000000000000000  w      *UND*  0000000000000000              _ITM_registerTMCloneTable
00000000004007a0 g     F .init  0000000000000000              _init
```
-d/-D反汇编 --start_address=反汇编开始地址  --stop-address=反汇编结束地址
```cpp
zh@zh-virtual-machine:~/anything_test$ objdump -D --start-address=0x401600 --stop-address=0x500000  a.out

a.out:     file format elf64-x86-64


Disassembly of section .eh_frame:

0000000000401600 <__FRAME_END__-0x38>:
  401600:       86 06                   xchg   %al,(%rsi)
  401602:       48 0e                   rex.W (bad)
  401604:       38 83 07 4d 0e 40       cmp    %al,0x400e4d07(%rbx)
  40160a:       6c                      insb   (%dx),%es:(%rdi)
  40160b:       0e                      (bad)
  40160c:       38 41 0e                cmp    %al,0xe(%rcx)
  40160f:       30 41 0e                xor    %al,0xe(%rcx)
  401612:       28 42 0e                sub    %al,0xe(%rdx)
  401615:       20 42 0e                and    %al,0xe(%rdx)
  401618:       18 42 0e                sbb    %al,0xe(%rdx)
  40161b:       10 42 0e                adc    %al,0xe(%rdx)
  40161e:       08 00                   or     %al,(%rax)
  401620:       14 00                   adc    $0x0,%al
  401622:       00 00                   add    %al,(%rax)
  401624:       cc                      int3
  401625:       00 00                   add    %al,(%rax)
  401627:       00 48 f7                add    %cl,-0x9(%rax)
  40162a:       ff                      (bad)
  40162b:       ff 02                   incl   (%rdx)
        ...

0000000000401638 <__FRAME_END__>:
  401638:       00 00                   add    %al,(%rax)

```
![image-20230424012512306](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424012512306.png)

## 编译过程
编译器gcc对源文件编译成可执行文件的过程可分为：预处理，编译，汇编，链接四个阶段
预处理打开原文件中引用的宏gcc -E hello.c -o hello.i
编译过程生成汇编语言文件gcc -S hello.i > hello.s
汇编过程把汇编语言代码翻译成目标机器指令gcc hello.s -c hello.o
链接过程将众多的目标文件(.o文件)链接生成真正的可执行文件

链接过程以main函数所在的目标文件链接出可执行文件，在链接之前，实际文件可分为两类，1个是主函数所在.o文件，其他文件是主函数调用函数所依赖的.o文件。
对于这些依赖的文件，可以将其"打包"成一个文件，在链接是，只需要这个文件和主函数所在的.o文件链接即可。而这样一个文件就叫库。

## 动态库和静态库

静态库扩展名.a，动态库扩展名.so
1.静态库编译的可执行程序会直接包含静态库中的符号，在运行时不再依赖此静态库。使用静态库的可执行程序占用的内存会比较大，而且当静态库函数发生改变时，可执行程序也需要重新编译
2.动态库编译的可执行文件不会将动态库中的符号包含进去，会在运行阶段将动态库的符号加载到进程的地址空间中。使用动态库的可执行程序占用内存比较小，但是在运行时可执行文件会依赖动态库，动态库中函数发生变化不需要重新编译可执行文件，只要重新编译动态库。

#### 1.静态库制作

```cpp
[Tue Jan 25 17:06:55~/proto_test/anything_test/test2]$ gcc test1.c -c
[Tue Jan 25 17:07:00~/proto_test/anything_test/test2]$ gcc test2.c -c
[Tue Jan 25 17:07:04~/proto_test/anything_test/test2]$ ls
test1.c  test1.o  test2.c  test2.o
[Tue Jan 25 17:07:09~/proto_test/anything_test/test2]$ ar rcs libtest.a test1.o test2.o
[Tue Jan 25 17:07:27~/proto_test/anything_test/test2]$ ls
libtest.a  test1.c  test1.o  test2.c  test2.o
```

##### 1.1静态库拆分

```cpp
[Tue Jan 25 17:07:28~/proto_test/anything_test/test2]$ mkdir test1
[Tue Jan 25 17:08:19~/proto_test/anything_test/test2]$ mv libtest.a test1
[Tue Jan 25 17:08:23~/proto_test/anything_test/test2]$ cd test1/
[Tue Jan 25 17:08:24~/proto_test/anything_test/test2/test1]$ ls
libtest.a
[Tue Jan 25 17:08:24~/proto_test/anything_test/test2/test1]$ ar x libtest.a
[Tue Jan 25 17:08:27~/proto_test/anything_test/test2/test1]$ ls
libtest.a  test1.o  test2.o
[Tue Jan 25 17:08:28~/proto_test/anything_test/test2/test1]$ diff test1.o ../test1.o
[Tue Jan 25 17:08:35~/proto_test/anything_test/test2/test1]$
```

可以看出静态库完全只是对.o文件的一个打包。

##### 1.2向静态库添加文件

```cpp
[Tue Jan 25 17:10:19~/proto_test/anything_test/test2]$ gcc test3.c -c
[Tue Jan 25 17:10:27~/proto_test/anything_test/test2]$ ar r libtest.a test3.o
[Tue Jan 25 17:10:35~/proto_test/anything_test/test2]$ ar t libtest.a
test1.o
test2.o
test3.o
```

##### 1.3从静态库删除文件

```cpp
[Tue Jan 25 17:10:39~/proto_test/anything_test/test2]$ ar d libtest.a test2.o
[Tue Jan 25 17:12:46~/proto_test/anything_test/test2]$ ar t libtest.a
test1.o
test3.o
```

#### 2.动态库制作

```cpp
[Tue Jan 25 17:23:15~/proto_test/anything_test/test2]$ gcc test1.c -c
[Tue Jan 25 17:23:19~/proto_test/anything_test/test2]$ gcc test2.c -c
[Tue Jan 25 17:23:41~/proto_test/anything_test/test2]$ gcc -fPIC -shared -o libtest.so test1.o test2.o
[Tue Jan 25 17:23:48~/proto_test/anything_test/test2]$ ls
libtest.a  libtest.so  test1  test1.c  test1.o  test2.c  test2.o  test3.c  test3.o
```

#### Linux下动态库查找优先级顺序：

##### 1.编译时

1.-L指定的目录
2.环境变量LIBRARY_PATH
3./etc/ld.so.conf(改变后需要执行ldconfig
4./lib
5./usr/lib

##### 2.运行时

1.LD_LIBRARY_PATH
2./etc/ld.so.conf
3./lib
4./usr/lib

#### 3.ld.so.conf

动态装入器找到共享库要依靠两个文件`/etc/ld.so.conf`和`/etc/ld.so.cache`.如果您对 `/etc/ld.so.conf`文件进行 cat 操作，您可能会看到一个与下面类似的清单：

```cpp
$ cat /etc/ld.so.conf
/usr/X11R6/lib
/usr/lib/gcc-lib/i686-pc-linux-gnu/2.95.3
/usr/lib/mozilla
/usr/lib/qt-x11-2.3.1/lib
/usr/local/lib
```

`ld.so.conf` 文件包含一个所有目录（`/lib` 和 `/usr/lib` 除外，它们会自动包含在其中）的清单，动态装入器将在其中查找共享库。
`ld.so.cache`
但是在动态装入器能“看到”这一信息之前，必须将它转换到`ld.so.cache`文件中。可以通过运行 `ldconfig`命令做到这一点：
`# ldconfig`
当 `ldconfig` 操作结束时，您会有一个最新的 `/etc/ld.so.cache` 文件，它反映您对 `/etc/ld.so.conf`所做的更改。从这一刻起，动态装入器在寻找共享库时会查看您在 `/etc/ld.so.conf` 中指定的所有新目录。
可以使用`strings ld.so.cache + grep *.so`查看ld.so.conf是否包含想要的动态库

## nm命令

nm用在在linux下分析二进制文件(.o)，库文件和可执行文件的符号表
参数：
-A 或-o或 --print-file-name：打印出每个符号属于的文件
-a或--debug-syms：打印出所有符号，包括debug符号
-B：BSD码显示
-C或--demangle[=style]：对低级符号名称进行解码，C++文件需要添加
--no-demangle：不对低级符号名称进行解码，默认参数
-D 或--dynamic：显示动态符号而不显示普通符号，一般用于动态库
-f format或--format=format：显示的形式，默认为bsd，可选为sysv和posix
-g或--extern-only：仅显示外部符号
-h或--help：国际惯例，显示命令的帮助信息
-n或-v或--numeric-sort：显示的符号以地址排序，而不是名称排序
-p或--no-sort：不对显示内容进行排序
-P或--portability：使用POSIX.2标准
-V或--version：国际管理，查看版本
--defined-only：仅显示定义的符号，这个从英文翻译过来可能会有偏差，故贴上原文：
Display only defined symbols for each object file

常用参数：
`nm -a -n libtest.so`
`nm -a -n libtest.a`
示例：

```cpp
[Wed Jan 26 15:53:55~/proto_test/anything_test/test2]$ nm -n -a libtest.a

test1.o:
zh@zh-virtual-machine:~/anything_test$ nm -n -a libtest.a

test2.o:
                 U free
                 U func
                 U getchar
                 U getpid
                 U malloc
                 U printf
                 U pthread_create
                 U pthread_join
                 U puts
                 U __stack_chk_fail
0000000000000000 b .bss
0000000000000000 n .comment
0000000000000000 d .data
0000000000000000 B e
0000000000000000 r .eh_frame
0000000000000000 T func2
0000000000000000 D g_str
0000000000000000 n .note.GNU-stack
0000000000000000 r .rodata
0000000000000000 a test2.c
0000000000000000 t .text
0000000000000004 b a
0000000000000004 C d
0000000000000004 C g
0000000000000008 b b
0000000000000008 d c
0000000000000008 R h
000000000000000c D f
000000000000000c R l
000000000000000c b m.3966
0000000000000010 b n.3967
0000000000000010 d o.3968
00000000000000be T main

```

##### 第一列为地址

每一段的起始地址都是0x0，第一列的地址表示相对每一段的偏移。

##### 第二列含义如下

A     ：符号的值是绝对值，不会被更改
B或b  ：未被初始化的全局数据，放在.bss段
C	  ：此符号位于common段，关于common段：https://blog.csdn.net/Chris_Magic/article/details/7275318?spm=1001.2101.3001.6650.2&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7Edefault-2.no_search_link&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7Edefault-2.no_search_link&utm_relevant_index=5
common段数据是未初始化的，属于弱符号，链接时，允许存在出现多个同名弱符号符号，并选取其中一个弱符号作为定义且初始化为0。可存在唯一一个同名强符号，链接时将使用强符号。
D或d  ：已经初始化的全局数据，放在.data段
G或g  ：指被初始化的数据，特指small objects
I     ：另一个符号的间接参考
N     ：debugging 符号
p     ：位于堆栈展开部分
R或r  ：属于只读存储区
S或s  ：指为初始化的全局数据，特指small objects
T或t  ：代码段的数据，.test段
U     ：符号未定义
u	  ：唯一全局符号
vV    ：弱对象，同弱符号
wW	  :弱对象，同弱符号
W或w  ：符号为弱符号，当系统有定义符号时，使用定义符号，当系统未定义符号且定义了弱符号时，使用弱符号。
？    ：unknown符号

##### 第三列为文件中的符号名

## ldd命令

ldd本身是一个shell脚本
`which ldd找到ldd所在位置，which会读取PATH环境变量`
`file ldd查看ldd为shell script`

```cpp
zh@zh-virtual-machine:~/anything_test$ ldd --help
Usage: ldd [OPTION]... FILE...
      --help              print this help and exit
      --version           print version information and exit
  -d, --data-relocs       process data relocations
  -r, --function-relocs   process data and function relocations
  -u, --unused            print unused direct dependencies
  -v, --verbose           print all information
```

查看动态库和可执行文件的依赖库:
`ldd -v a.out`

## ELF文件

| ELF文件类型                      | description                                                  | example                                |
| -------------------------------- | ------------------------------------------------------------ | -------------------------------------- |
| 可重定位文件(Relocatable File)   | 这类文件包含了代码和数据，可被用来链接成可执行文件或者共享目录文件，扩展名为.o | Linux下的.o文件,windows下的.obj文件    |
| 可执行文件(Executable File)      | 这类文件包含了可以直接执行的程序，一般没有扩展名             | Linux /bin下的文件,windows下的.exe文件 |
| 共享目录文件(Shared Object File) | 这类文件包含了代码和数据                                     | Linux的.so文件，windows下的.dll文件    |
| 核心转储文件(Core Dump File)     | 当进程意外终止时，系统可以将该进程的地址空间的内容以及终止时的一些其他信息转储在核心转储文件中 | Linux下的coredump                      |

其中共享目录文件使用方式：
1.链接器使用这类文件，与其他共享目录文件和可重定位文件进行链接，生成新的共享目录文件
2.动态链接器将共享目录文件(.so)与包含main函数的.o文件链接生成可执行文件，共享目录文件将作为进程映像的一部分
3.静态库.a文件，实际是对.o文件的打包，与包含main函数的.o文件静态链接生成可执行文件，可执行文件直接将.a文件中的符号链接进来，可执行文件运行时将不依赖静态库，但是静态库本身如果依赖于共享目录文件，则这些共享目录文件将如2中被加载到进程。