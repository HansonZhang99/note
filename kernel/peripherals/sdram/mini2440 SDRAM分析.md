# mini2440 SDRAM分析

soc:s3c2440



## SDRAM

 同步动态随机存取内存（synchronous dynamic random-access memory，简称SDRAM）是有一个同步接口的动态随机存取内存（DRAM）。通常DRAM是有一个异步接口的，这样它可以随时响应控制输入的变化。而SDRAM有一个同步接口，在响应控制输入前会等待一个时钟信号，这样就能和计算机的系统总线同步。时钟被用来驱动一个有限状态机，对进入的指令进行管线(Pipeline)操作。这使得SDRAM与没有同步接口的异步DRAM(asynchronous DRAM)相比，可以有一个更复杂的操作模式。

SDRAM内部是一个存储阵列。（如果是管道式存储就无法做到随机存取了），整列如同一个表格，对表格进行检索可以通过指定一个行（Row）和一个列（Column）即可找到所需的数据，这就是内存寻址的基本原理（CPU通过地址总线指定行和列即可定位到内存也就是SDRAM的指定位置进行存取操作）。对于内存，这个单元格可以称为存储单元，这个表格叫逻辑Bank(Logical Bank)。

由于技术、成本等原因，不可能只做一个全容量的L-Bank，而且最重要的是，由于SDRAM的工作原理限制，单一的L-Ban k将会造成非常严重的寻址冲突，大幅降低内存效率（请自行了解）。所以人们在SDRAM内部分割成多个L-Bank，较 早以前是两个，目前基本都是4个，这也是SDRAM规范中的最高L-Bank数量。到了RDRAM则最多达到了32个，在最新DDR-Ⅱ的标准中，L-Bank的数量也提高到了8个。这样，在进行寻址时就要先确定是哪个L-Bank，然后再在这个选定的L-Bank中选择相应的行与列进行寻址。

也就是说一块32MB的内存，不会将其当为一整块内存进行访问，我们会将其分为4个L-Bank，以这里使用的HY57V561620(L)T为例，它将32M的内存分成4个L-bank,每个L-bank的大小是4M，因为它的为宽是16位，两个字节，所以32=4*4*2.

## HY57V561620(L)T 32MB SDRAM

芯片管脚图和含义：

![image-20230424013412393](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013412393.png)

![image-20230424013426126](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013426126.png)

![image-20230424013518324](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013518324.png)

芯片有4个Bank，通过BA1和BA0选择访问的Bank。A0-A12为地址总线，总的寻址空间为2^13 = 8KB，但是如何做到对芯片的32MB的空间的访问的？SDRAM的行地址线和列地址线是分时复用的，即地址需要分两次送出，先送出行地址（nSRAS行有效操作），再送出列地址（nSCAS列有效操作）。大幅减少地址线的数目，提高器件的性能和减少制作的复杂度，但是寻址过程也会因此变的复杂。实际上，现在的SDRAM都是以L-bank为基本寻址对象的，由L-bank地址线BAn控制L-bank间的选择，行地址线和列地址线贯穿连接所有的L-bank，每个l-bank的数据宽度和整个存储器的宽度相同，这样可以加快数据的存储速度。同时，BAn还可以使为被选中的L-banl工作于低功耗的模式下，从而降低器件的功耗。

上述内存计算：由datasheet表格可以看出，A0-A12在分时复用时，A0-A12可以完全作为行地址，而列地址为A0-A8。这样行寻址范围为8KB，列寻址范围为512B，这是针对一个L-Bank，而芯片有4个L-Bank，且芯片的位宽为16bit也就是2Byte，所以总的可寻址空间为8K*512B*4L-Bank*2Byte = 32MB。

## SDRAM连线方式

![image-20230424013442615](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013442615.png)

![image-20230424013453289](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013453289.png)

s3c2440有8个Bank（这个Bank和前面的L-Bank不同），每个Bank为128M，总共可以有1G/128M = 8个Bank其中Bank0-Bank5可以接ROM和SRAM，另外两个可以接ROM，SRAM和SDRAM。

上图为s3c2440A CPU的内存映射图，不同的存储器有不同的mmc，连线方式也因CPU不同而不同。由图可看出，当设备从NandFlash启动时，mmc可以管理8个段，分别是nGCS0-nGCS7，外加一个4KB的SRAM；当从NandFlash启动时，mmc将这4KB的片上内存指定到0x0地址处占用nGCS0。此外还可以看出，nGCS0-nGCS5只能接SROM，而nGCS6-nGCS7可以接SROM或SDRAM。此处使用的SDRAM只能接在nGCS6或nGCS7上

结合寻址图与红框部分，其实nGCS0-nGCS7就是对应的8个Bank，其中只有Bank6和Bank7可以接SDRAM。

![image-20230424013554322](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013554322.png)

从上图的s3c2440CPU引脚也可以看出CPU的地址总线根数为2^27 = 128MB。

![image-20230424013607099](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013607099.png)

对于CPU可以外接8个Bank，CPU的nGCS0-nCGS7作为片选对8个Bank进行访问。Bank和L-Bank的区别，Bank为CPU的概念，表示该CPU可以接多少个Bank，每个Bank有多大，通过CPU的nGCS0-nGCS7进行片选；而L-Bank是SDRAM的概念，表示该SDRAM可细分为多少个L-Bank，在如上的SDRAM芯片中，L-Bank通过SDRAM的BA0和BA1进行片选。



SDRAM与CPU的接线

![image-20230424013631981](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013631981.png)

SDRAM的并联

![image-20230424013644571](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013644571.png)

1块SDRAM的位宽为16bit，两块SDRAM的并联后可以组成32位的位宽，可寻址空间变为64MB（实际CPU寻址一次内存返回4Byte给CPU），但是和CPU的接线不变，仅仅是SDRAM进行了并联。

## 内存寻址

内存寻址一次就是地址总线的行与列交叉一次，得到一个存储单元，**也就是说内存寻址是以"存储单元(本例为4个字节,低16位与高16位合成一个存储单元)"为单位的,而不是以"字节"为单位的。**但是,cpu寻址是以字节为单位的.也就是说,cpu移动到下一个才一个字节,而本例内存移动到下一个就是4个字节.所以我们写程序时常常说要字节对齐就是这个原因。

CPU寻址内存     内存返回给CPU

0000 0000     第0个单元(其实包含0000 0000 至 0000 0011 这4个字节) 

0000 0100     第1个单元(其实包含0000 0100 至 0000 0111 这4个字节) 

00000101     第1个单元(其实包含0000 0100 至 0000 0111 这4个字节) **因为s3c2440没接A1和A0,所以相于逻辑与0xFFFF FF00,取到内存的4个字节后再用低两位去选择** 

0000 1100     第3个单元(其实包含0000 1100 至 0000 1111 这4个字节) 



因为内存对齐的存在，可以看出，当CPU寻址0x00000100和0x00000101时，内存返回给CPU的是同样的4个字节。即内存并不关心CPU给的寻址空间的低2位，因为他们返回的是同一块地址空间。所以对CPU与SDRAM的地址总线接线，CPU并不需要给SDRAM最低两位即LADDR[1:0]，它们的值无论多少，内存的返回都只取决于LADDR[2]以及后面的值，因此SDRAM的A0最低位接CPU的LADDR2即可，取到4Byte后，CPU再根据没有接的低两位取得4字节中对应字节即可。同理对于位宽为16bit的SDRAM，LADDR[0]不需要接，SDRAM的A0接CPU的LADDR1。**A0接CPU地址线的哪一根是由SDRAM的位宽决定的。**



![image-20230424013657632](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013657632.png)

SDRAM的片选BA0和BA1（寻址空间的最高两位）由上表格可知是接到了CPU的LADDR24和LADDR25。BA0和BA1为SDRAM寻址的最高两位（选择L-Bank的两位）而当BA0和BA1接到这两位时，CPU的总寻址空间为2^26 = 64MByte刚好为SDRAM的大小，（根据2440的datasheet也可以看出外接SDRAM的大小与CPU的总寻址空间是相同的）即SDRAM的片选BA0和BA1接CPU哪里取决于SDRAM的容量。

## CPU对SDRAM的访问步骤

1.确定访问那一块SDRAM:根据nGCSn选择片选

2.SDRAM中有四个L-Bank，需要两根地址信号来选中其中一个SDRAM后，此处根据LADDR24和LADDR25

3.对被选中的L-Bank进行行列寻址

4.找到存储单元后，对选中芯片进行数据传输

## 时序

![image-20230424013708808](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013708808.png)

![image-20230424013736160](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013736160.png)

1.SDRAM预充电操作（preCharge）：     

从存储体的结构图上可以看出，原本逻辑状态为1的电容在读取操作后，会因放电而变为逻辑0。由于SDRAM的寻址具有独占性，所以在进行完读写操作后，如果要对同一L-Bank的另一行进行寻址，就要将原先操作行关闭，重新发送行/列地址。在对原先操作行进行关闭时，DRAM为了在关闭当前行时保持数据，要对存储体中原有的信息进行重写，这个充电重写和关闭操作行过程叫做预充电，发送预充电信号时，意味着先执行存储体充电，然后关闭当前L-Bank操作行。S3C2440手册给出的时序图（如上）在进行读写操作前也进行了一次预充电操作，结合命令表，可以看出在时序图最开始时，nSCs和nSRAS被拉低，而nSCAS被拉高，nWE被拉低，此时A10地址线为灰色表示不关心。根据命令表可以看出此时在执行preCharge ALL Bank或者preCharge select Bank命令，具体由A10的电平决定，高会对所有Bank预充电，而低会对选中的Bank预充电（此时需要发送对应的Bank，即此时BA线为Vaild有效）。



2.SDRAM Active操作：     

该命令需要携带bank address和Row address的参数，告知具体是要选择后续要访问哪一个bank的哪一行（后续要访问的具体数据的column地址在READ或者WRITE命令提供）。一个bank中，同一时间只能active（让该行的Word line设置为高电平，以便导通cell的晶体管）一行。如果想要发送ACTIVE命令去本bank中的另外一行，直接发送是不行的，必须要close（PRECHARGE）之前打开的那个行。如果想要发送ACTIVE命令去另外一个bank中的某一行则没有这个限制（要满足tRRD的要求），这种跨bank的访问可以减少访问行的开销。根据结合命令表和时序图，在preCharge操作经过Trp时间后，nSCs和nSRAS被拉低，而nSCAS被拉高，此时地址总线为行有效，发送行地址RA，BA为Bank选择线，此时有效，nWE被拉高。最后组成Active命令。



3.SDRAM写操作：     

在Active命令后，可以对SDRAM发送读写命令，继续结合时序图，在Active命令发出后经过Trcd时间，发送列地址（此时nCACS被拉低代表列有效）同时nWE被拉低代表写操作，A10地址线此时不用做列地址（前面可以看到列地址线为A0-A8），A10的低高电平分别到表两种写操作Write和Write Auto-preCharge。Wirte操作即常规写操作，此时在数据总线上传输数据到SDRAM，而Write Auto-preCharge的操作相当于Write+preCharge即Write后进行precharge关闭操作。SDRAM的基本写操作也需要控制线和地址线相配合地发出一系列命令来完成。先发出芯片有效命令，并锁定相应的L-BANK地址（BA0、BA1给出）和行地址（A0～A12给出）。芯片有效命令发出后必须等待大于tRCD的时间后，发出写命令数据，待写入数据依次送到DQ（数据线）上。在最后一个数据写入后，延迟tWR时间。发出预充电命令，关闭已经激活的页。等待tRP时间后，可以展开下一次操作。写操作可以有突发写和非突发写两种。突发长度同读操作。



4.SDRAM读操作：     

结合时序图，在写操作时，发给SDRAM的是常规的Write命令（A10为低电平），后续可以继续对此地址进行其他操作，此时nWE被拉高，对SDRAM的操作由写转变为读，读与写一样由A10地址线决定着两种Read操作，此时A10为低电平代表常规的读操作，发送完读命令，在选定列地址后，就已经确定了具体的存储单元，剩下就是等待数据通过数据 I/O通道（DQ）输出到内存数据总线上了。但是在列地址选通信号CAS发出之后，仍要经过一定的时间才能有数据输出，从CAS与读取命令发出到第一笔数据输出的这段时间，被定义为 CL（CAS Latency，CAS潜伏期）。由于CL只在读取时出现，所以CL又被称为读取潜伏期（RL，Read Latency）。CL的单位与tRCD一样，也是时钟周期数，具体耗时由时钟频率决定（笔者官方手册CL=3）。不过，CAS并不是在经过CL周期之后才送达存储单元。实际上CAS与RAS一样是瞬间到达的。由于芯片体积的原因，存储单元中的电容容量很小，所以信号要经过放大来保证其有效的识别性，这个放大/驱动工作由S-AMP负责。但它要有一个准备时间才能保证信号的发送强度，这段时间我们称之为tAC（Access Time from CLK，时钟触发后的访问时间）。SDRAM的基本读操作需要控制线和地址线相配合地发出一系列命令来完成。先发出芯片有效命令（ACTIVE），并锁定相应的L-BANK地址（BA0、BA1给出）和行地址（A0～A12给出）。芯片激活命令后必须等待大于tRCD(SDRAM的RAS到CAS的延迟指标)时间后，发出读命令。CL（CAS延迟值）个时钟周期后，读出数据依次出现在数据总线上。在读操作的最后，要向SDRAM发出预充电（PRECHARGE）命令，以关闭已经激活的L-BANK。等待tRP时间（PRECHAREG命令后，相隔tRP时间，才可再次访问该行）后，可以开始下一次的读、写操作。SDRAM的读操作支持突发模式（Burst Mode），突发长度为1、2、4、8可选。



5.突发:     

突发（Burst）是指在同一行中相邻的存储单元连续进行数据传输的方式，连续传输所涉及到存储单元（列）数量就是突发长度（Burst Length，简称BL）。 在目前，由于内存控制器一次读/写P-Bank位宽的数据，也就是8个字节，但是在现实中小于8个字节的数据很少见，所以一般都要经过多个周期进行数据的传输，上文写到的读/写操作，都是一次对一个存储单元进行寻址，如果要连续读/写，还要对当前存储单元的下一单元进行寻址，也就是要不断的发送列地址与读/写命令（行地址不变，所以不用再对行寻址）（时序图中BL为1代表突发字节为1Byte，所以在SDRAM的读写时每次都要发送列地址）。虽然由于读/写延迟相同可以让数据传输在I/O端是连续的，但是它占用了大量的内存控制资源，在数据进行连续传输时无法输入新的命令效率很低。为此，引入了突发传输机制，只要指定起始列地址与突发长度，内存就会依次自动对后面相应长度数据的数据存储单元进行读/写操作而不再需要控制器连续地提供列地址，这样，除了第一笔数据的传输需要若干个周期（主要是之间的延迟，一般的是tRCD + CL）外，其后每个数据只需一个周期即可。



6.数据掩码     

在讲述读/写操作时，我们谈到了突发长度。如果BL=4，那么也就是说一次就传送4×64bit的数据。但是，如果其中的第二笔数据是不需要的，怎么办？还都传输吗？为了屏蔽潜水射流曝气机的数据，人们采用了数据掩码（Data I/O Mask，简称DQM）技术。通过DQM，内存可以控制I/O端口取消哪些输出或输入的数据。这里需要强调的是，在读取时，被屏蔽的数据仍然会从存储体传出，只是在“掩码逻辑单元”处被屏蔽。DQM由北桥控制，为了精确屏蔽一个P-Bank位宽中的每个字节，每个DIMM有8个DQM信号线，每个信号针对一个字节。这样，对于4bit位宽芯片，两个芯片共用一个DQM信号线，对于8bit位宽芯片，一个芯片占用一个DQM信号，而对于16bit位宽芯片，则需要两个DQM引脚。 SDRAM官方规定，在读取时DQM发出两个时钟周期后生效，而在写入时，DQM与写入命令一样是立即成效。

![image-20230424013750421](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013750421.png)

读取时数据掩码操作，DQM在两个周期后生效，突发周期的第二笔数据被取消

![image-20230424013803908](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013803908.png)

写入时数据掩码操作，DQM立即生效，突发周期的第二笔数据被取消



7.刷新：     

SDRAM之所以成为DRAM就是因为它要不断进行刷新（Refresh）才能保留住数据，因此它是SDRAM最重要的操作。 刷新操作与预充电中重写的操作一样，都是用S-AMP先读再写。但为什么有预充电操作还要进行刷新呢？因为预充电是对一个或所有 L-Bank中的工作行操作，并且是不定期的，而刷新则是有固定的周期，依次对所有行进行操作，以保留那些很长时间没经历重写的存储体中的数据。但与所有L-Bank预充电不同的是，这里的行是指所有L-Bank中地址相同的行，而预充电中各L-Bank中的工作行地址并不是一定是相同的。那么要隔多长时间重复一次刷新呢？**目前公认的标准是，存储体中电容的数据有效保存期上限是64ms**（毫秒，1/1000秒），也就是说每一行刷新的循环周期是64ms。**这样刷新时间间隔就是：64m/行数s。**我们在看内存规格时，经常会看到4096 Refresh Cycles/64ms或8192 Refresh Cycles/64ms的标识，这里的4096与8192就代表这个芯片中每个L-Bank的行数。刷新命令一次对一行有效，刷新间隔也是随总行数而变化，4096行时为 15.625μs（微秒，1/1000毫秒），8192行时就为 7.8125μs。刷新操作分为两种：Auto Refresh，简称AR与Self Refresh，简称SR。不论是何种刷新方式，都不需要外部提供行地址信息，因为这是一个内部的自动操作。对于 AR，SDRAM内部有一个行地址生成器（也称刷新计数器）用来自动的依次生成行地址。由于刷新是针对一行中的所有存储体进行，所以无需列寻址，或者说CAS在 RAS之前有效。所以，AR又称CBR（CAS Before RAS，列提前于行定位）式刷新。由于刷新涉及到所有L-Bank，因此在刷新过程中，所有 L-Bank都停止工作，**而每次刷新所占用的时间为9个时钟周期（PC133标准），**之后就可进入正常的工作状态，也就是说在这9个时钟期间内，所有工作指令只能等待而无法执行。64ms之后则再次对同一行进行刷新，如此周而复始进行循环刷新。显然，刷新操作肯定会对SDRAM的性能造成影响，但这是没办法的事情，也是DRAM相对于 SRAM（静态内存，无需刷新仍能保留数据）取得成本优势的同时所付出的代价。SR则主要用于休眠模式低功耗状态下的数据保存，这方面最著名的应用就是 STR（Suspend to RAM，休眠挂起于内存）。在发出AR命令时，将CKE置于无效状态，就进入了SR模式，此时不再依靠系统时钟工作，而是根据内部的时钟进行刷新操作。在SR期间除了CKE之外的所有外部信号都是无效的（无需外部提供刷新指令），只有重新使CKE有效才能退出自刷新模式并进入正常操作状态。

![image-20230424013832650](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424013832650.png)

ref:

http://www.360doc.com/content/18/0101/18/51484742_718178899.shtml

https://blog.csdn.net/qq_41982581/article/details/88206339

https://blog.csdn.net/lee244868149/article/details/50450232



## u-boot初始化配置（u-boot-2009)

### 寄存器设置

**总线宽度和等待控制寄存器（BWSCON）：**

UB、LB：启动/禁止SDRAM的数据掩码引脚（UB/LB），SDRAM没有高低位掩码引脚，此位为0，SRAM连接有UB/LB管脚，设置为1。

`BWSCON = 0010 0010 0000 0000 0000 0000 0000 0000b = 0x22000000`



**BANK 控制寄存器（BANKCONn：nGCS0 至 nGCS5）:**

`BANKCON0-BANKCON5 用来接ROM使用默认值：0x0700`



**BANK 控制寄存器（BANKCONn：nGCS6 至 nGCS7）:接RAM**

`BANKCON6- BANKCON7 = 1 1000 0000 0000 0001b = 0x18001`



**刷新控制寄存器:**

`REFRESH = 1000 1100 0000 0100 1111 0100b = 0x8c04f4`

每次刷新占用9个周期，Trc = 9,Trp =2，Tsrc = 9 -2 = 7。

8192 refresh cycles / 64ms（来源于SDRAM芯片）

每行刷新时间为64/8192 = 7.8125us

刷新周期 = (2^11- 刷新计数 + 1) / HCLK

刷新计数 = 2^11 - 7.8125 * 100 +1 = 1267.75 约等于 1268 (100 1111 0100b = 0x4f4)



**Bank 大小寄存器**

BANKSIZE = 1011 0001b = 0xb1



**SDRAM 模式 寄存器组寄存器（MRSR）**

`MRSRB6 = 0000 0000 0010 0000b = 0x0020`



`u-boot lowlevel_init.S:`

```assembly
#include <config.h>


/* some parameters for the board */


/*
*
* Taken from linux/arch/arm/boot/compressed/head-s3c2410.S
*
* Copyright (C) 2002 Samsung Electronics SW.LEE  <hitchcar@sec.samsung.com>
*
*/


#define BWSCON    0x48000000


/* BWSCON */
#define DW8            (0x0)
#define DW16            (0x1)
#define DW32            (0x2)
#define WAIT            (0x1<<2)
#define UBLB            (0x1<<3)


#define B1_BWSCON        (DW32)
#define B2_BWSCON        (DW16)
#define B3_BWSCON        (DW16 + WAIT + UBLB)
#define B4_BWSCON        (DW16)
#define B5_BWSCON        (DW16)
#define B6_BWSCON        (DW32)
#define B7_BWSCON        (DW32)


/* BANK0CON */
#define B0_Tacs            0x0    /*  0clk */
#define B0_Tcos            0x0    /*  0clk */
#define B0_Tacc            0x7    /* 14clk */
#define B0_Tcoh            0x0    /*  0clk */
#define B0_Tah            0x0    /*  0clk */
#define B0_Tacp            0x0
#define B0_PMC            0x0    /* normal */


/* BANK1CON */
#define B1_Tacs            0x0    /*  0clk */
#define B1_Tcos            0x0    /*  0clk */
#define B1_Tacc            0x7    /* 14clk */
#define B1_Tcoh            0x0    /*  0clk */
#define B1_Tah            0x0    /*  0clk */
#define B1_Tacp            0x0
#define B1_PMC            0x0


#define B2_Tacs            0x0
#define B2_Tcos            0x0
#define B2_Tacc            0x7
#define B2_Tcoh            0x0
#define B2_Tah            0x0
#define B2_Tacp            0x0
#define B2_PMC            0x0


#define B3_Tacs            0x0    /*  0clk */
#define B3_Tcos            0x3    /*  4clk */
#define B3_Tacc            0x7    /* 14clk */
#define B3_Tcoh            0x1    /*  1clk */
#define B3_Tah            0x0    /*  0clk */
#define B3_Tacp            0x3     /*  6clk */
#define B3_PMC            0x0    /* normal */


#define B4_Tacs            0x0    /*  0clk */
#define B4_Tcos            0x0    /*  0clk */
#define B4_Tacc            0x7    /* 14clk */
#define B4_Tcoh            0x0    /*  0clk */
#define B4_Tah            0x0    /*  0clk */
#define B4_Tacp            0x0
#define B4_PMC            0x0    /* normal */


#define B5_Tacs            0x0    /*  0clk */
#define B5_Tcos            0x0    /*  0clk */
#define B5_Tacc            0x7    /* 14clk */
#define B5_Tcoh            0x0    /*  0clk */
#define B5_Tah            0x0    /*  0clk */
#define B5_Tacp            0x0
#define B5_PMC            0x0    /* normal */


#define B6_MT            0x3    /* SDRAM */
#define B6_Trcd            0x1
#define B6_SCAN            0x1    /* 9bit */


#define B7_MT            0x3    /* SDRAM */
#define B7_Trcd            0x1    /* 3clk */
#define B7_SCAN            0x1    /* 9bit */


/* REFRESH parameter */
#define REFEN            0x1    /* Refresh enable */
#define TREFMD            0x0    /* CBR(CAS before RAS)/Auto refresh */
#define Trp            0x0    /* 2clk */
#define Trc            0x3    /* 7clk */
#define Tchr            0x2    /* 3clk */
#define REFCNT            1268    /* period=15.6us, HCLK=60Mhz, (2048+1-15.6*60) */
/**************************************/


.globl lowlevel_init
lowlevel_init:
    /* memory control configuration */
    /* make r0 relative the current location so that it */
    /* reads SMRDATA out of FLASH rather than memory ! */
    ldr     r0, =SMRDATA
    ldr    r1, =CONFIG_SYS_TEXT_BASE
    sub    r0, r0, r1
    ldr    r1, =BWSCON    /* Bus Width Status Controller */
    add     r2, r0, #13*4
0:
    ldr     r3, [r0], #4
    str     r3, [r1], #4
    cmp     r2, r0
    bne     0b


    /* everything is fine now */
    mov    pc, lr


    .ltorg
/* the literal pools origin */


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
```

