# mini2440 NandFlash

为了支持NandFlash的bootloader，s3c2440配置了一个内置SRAM缓冲器，叫做SteppingStone。引导启动时，NandFlash存储器开始的前4K字节将被加载到SteppingStone中并执行加载到其中的引导代码。引导代码通常会复制NandFlash中的内容到SDRAM中。



mini2440的NandFlash:
存储器接口：支持256字，512字节，1K字和2K字节的页。
用户可直接访问NandFlash存储器。
硬件ECC纠错。
SteppingStone 4K的SRAM可在NandFlash引导启动后用于其他用途。

![image-20230424014323701](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014323701.png)

当自动引导启动期间，ECC 不会去检测，所以，NAND Flash 的开始 4KB 不应当包含位相关的错误。



S3C2440 NandFlash引脚配置：

![image-20230424014354421](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014354421.png)

![image-20230424014411583](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014411583.png)

mini2440上用的是一块K9F2G08UXA的256M*8bit的NandFlash芯片，结构如下所示：

![image-20230424014427442](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014427442.png)

![image-20230424014438403](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014438403.png)

存储器详细结构如上图。页大小为2K，块大小为128K=64页

地址周期：

![image-20230424014450975](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014450975.png)

此芯片只有8个IO口，所有的数据访问和命令发送都通过这8根地址线实现，通过对总线的控制实现不同操作，在5个周期内使用8个IO口实现行列地址的发送和访问。

相关总线引脚：
IO0-IO7：IO引脚用来输入命令，地址和数据，在读操作时用来输出数据，当IO引脚为浮空高阻态或闲篇未被选中时，IO口被禁用。

CLE：当CLE为高电平，且WE上升沿时，命令通过IO口锁存到地址寄存器中。

ALE：当ALE为高电平，且WE为上升沿时，地址通过IO口锁存到地址寄存器中。

CE：用来选择要控制的设备，低电平有效。

RE：读使能，低电平有效。数据在RE下降沿之后的一段时间tREA内有效，同时通过一个内部地址计数递增地址。

WE：写使能，低电平有效。当WE为上升沿是，数据，命令，地址被锁存。

WP：写保护，低电平有效。

R/B：就绪、忙状态输出。为低电平时，表示设备在进行编程，擦除，读取等操作，当操作完成时返回高电平输出。

![image-20230424014504661](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014504661.png)

![image-20230424014521137](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014521137.png)

![image-20230424014548290](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014548290.png)

初始无效块的确定：初始出厂的芯片，每一块的第2048列中写入的值为非FFh则代表此块为坏块：

![image-20230424014559221](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014559221.png)

写入操作流程：

![image-20230424014617410](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014617410.png)

擦除和读取操作：

![image-20230424014643190](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014643190.png)

理论上，CE片选信号在进行NandFlash操作时必须有效（低电平），但是在进行数据读取和串行访问步骤时，可以取取消CE的有效，成为与CE无关，可以节省电力消耗。

![image-20230424014707004](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014707004.png)

![image-20230424014719258](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014719258.png)

读ID操作：



![image-20230424014731292](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014731292.png)

CE有效即片选选中时，芯片有效，CLE高电平有效时（WE也有效）写入读取ID命令0x90h，随后在ALE有效时，写入5个周期地址，由于都ID操作的特殊性，地址直接写入0x00即可，随后在RE的5个有效周期内读取5个字节的ID。

![image-20230424014742258](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014742258.png)

页的读取操作：

![image-20230424014753668](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014753668.png)

![image-20230424014807042](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014807042.png)

页内随机数据的输出：

![image-20230424014818331](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014818331.png)

写入操作：

![image-20230424014829397](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014829397.png)

![image-20230424014841204](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014841204.png)

![image-20230424014856545](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424014856545.png)