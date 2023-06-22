# i2c

## 硬件连接

I2C在硬件上的接法如下所示，主控芯片引出两条线SCL,SDA线，在一条I2C总线上可以接很多I2C设备，我们还会放一个上拉电阻（放一个上拉电阻的原因以后我们再说）。

![image-20230607151012317](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607151012317.png)

## 传输数据类比

怎么通过I2C传输数据，我们需要把数据从主设备发送到从设备上去，也需要把数据从从设备传送到主设备上去，数据涉及到双向传输。

举个例子：

![image-20210220145618978](https://gitee.com/zhanghang1999/typora-picture/raw/master/006_teacher_and_student.png)

体育老师：可以把球发给学生，也可以把球从学生中接过来。

* 发球：
  * 老师：开始了(start)
  * 老师：A！我要发球给你！(地址/方向)
  * 学生A：到！(回应)
  * 老师把球发出去（传输）
  * A收到球之后，应该告诉老师一声（回应）
  * 老师：结束（停止）

* 接球：
  * 老师：开始了(start)
  * 老师：B！把球发给我！(地址/方向)
  * 学生B：到！
  * B把球发给老师（传输）
  * 老师收到球之后，给B说一声，表示收到球了（回应）
  * 老师：结束（停止）

我们就使用这个简单的例子，来解释一下IIC的传输协议：

* 老师说开始了，表示开始信号(start)
* 老师提醒某个学生要发球，表示发送地址和方向(address/read/write)
* 老师发球/接球，表示数据的传输
* 收到球要回应：回应信号(ACK)
* 老师说结束，表示IIC传输结束(P)

##  IIC传输数据的格式

### 写操作

流程如下：

* 主芯片要发出一个start信号
* 然后发出一个设备地址(用来确定是往哪一个芯片写数据)，方向(读/写，0表示写，1表示读)
* 从设备回应(用来确定这个设备是否存在)，然后就可以传输数据
* 主设备发送一个字节数据给从设备，并等待回应
* 每传输一字节数据，接收方要有一个回应信号（确定数据是否接受完成)，然后再传输下一个数据。
* 数据发送完之后，主芯片就会发送一个停止信号。

下图：白色背景表示"主→从"，灰色背景表示"从→主"

![image-20230607151221505](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607151221505.png)

### 读操作

流程如下：

* 主芯片要发出一个start信号
* 然后发出一个设备地址(用来确定是往哪一个芯片写数据)，方向(读/写，0表示写，1表示读)
* 从设备回应(用来确定这个设备是否存在)，然后就可以传输数据

* 从设备发送一个字节数据给主设备，并等待回应
* 每传输一字节数据，接收方要有一个回应信号（确定数据是否接受完成)，然后再传输下一个数据。
* 数据发送完之后，主芯片就会发送一个停止信号。

下图：白色背景表示"主→从"，灰色背景表示"从→主"

![image-20230607151238422](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607151238422.png)

### I2C信号

I2C协议中数据传输的单位是字节，也就是8位。但是要用到9个时钟：前面8个时钟用来传输8数据，第9个时钟用来传输回应信号。传输时，先传输最高位(MSB)。

* 开始信号（S）：SCL为高电平时，SDA山高电平向低电平跳变，开始传送数据。
* 结束信号（P）：SCL为高电平时，SDA由低电平向高电平跳变，结束传送数据。
* 响应信号(ACK)：接收器在接收到8位数据后，在第9个时钟周期，拉低SDA
* SDA上传输的数据必须在SCL为高电平期间保持稳定，SDA上的数据只能在SCL为低电平期间变化

I2C协议信号如下：

![image-20230607151259037](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607151259037.png)

### 协议细节

* 如何在SDA上实现双向传输？
  主芯片通过一根SDA线既可以把数据发给从设备，也可以从SDA上读取数据，连接SDA线的引脚里面必然有两个引脚（发送引脚/接受引脚）。

* 主、从设备都可以通过SDA发送数据，肯定不能同时发送数据，怎么错开时间？
  在9个时钟里，
  前8个时钟由主设备发送数据的话，第9个时钟就由从设备发送数据；
  前8个时钟由从设备发送数据的话，第9个时钟就由主设备发送数据。
* 双方设备中，某个设备发送数据时，另一方怎样才能不影响SDA上的数据？
  设备的SDA中有一个三极管，使用开极/开漏电路(三极管是开极，CMOS管是开漏，作用一样)，如下图：

![image-20230607151410542](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607151410542.png)

真值表如下：
![image-20210220152134970](https://gitee.com/zhanghang1999/typora-picture/raw/master/011_true_value_table.png)



从真值表和电路图我们可以知道：

* 当某一个芯片不想影响SDA线时，那就不驱动这个三极管
* 想让SDA输出高电平，双方都不驱动三极管(SDA通过上拉电阻变为高电平)
* 想让SDA输出低电平，就驱动三极管



从下面的例子可以看看数据是怎么传的（实现双向传输）。
举例：主设备发送（8bit）给从设备

* 前8个clk
  * 从设备不要影响SDA，从设备不驱动三极管
  * 主设备决定数据，主设备要发送1时不驱动三极管，要发送0时驱动三极管

* 第9个clk，由从设备决定数据
  * 主设备不驱动三极管
  * 从设备决定数据，要发出回应信号的话，就驱动三极管让SDA变为0
  * 从这里也可以知道ACK信号是低电平



从上面的例子，就可以知道怎样在一条线上实现双向传输，这就是SDA上要使用上拉电阻的原因。



为何SCL也要使用上拉电阻？
在第9个时钟之后，如果有某一方需要更多的时间来处理数据，它可以一直驱动三极管把SCL拉低。
当SCL为低电平时候，大家都不应该使用IIC总线，只有当SCL从低电平变为高电平的时候，IIC总线才能被使用。
当它就绪后，就可以不再驱动三极管，这是上拉电阻把SCL变为高电平，其他设备就可以继续使用I2C总线了。



对于IIC协议它只能规定怎么传输数据，数据是什么含义由从设备决定。

## SMBus协议

参考资料：

* Linux内核文档：`Documentation\i2c\smbus-protocol.rst`

* SMBus协议：

  * http://www.smbus.org/specs/
* `SMBus_3_0_20141220.pdf`
* I2CTools: `https://mirrors.edge.kernel.org/pub/software/utils/i2c-tools/`

### SMBus是I2C协议的一个子集

SMBus: System Management Bus，系统管理总线。
SMBus最初的目的是为智能电池、充电电池、其他微控制器之间的通信链路而定义的。
SMBus也被用来连接各种设备，包括电源相关设备，系统传感器，EEPROM通讯设备等等。
SMBus 为系统和电源管理这样的任务提供了一条控制总线，使用 SMBus 的系统，设备之间发送和接收消息都是通过 SMBus，而不是使用单独的控制线，这样可以节省设备的管脚数。
SMBus是基于I2C协议的，SMBus要求更严格，SMBus是I2C协议的子集。

![image-20230607163222651](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607163222651.png)

SMBus有哪些更严格的要求？跟一般的I2C协议有哪些差别？

* VDD的极限值不一样

  * I2C协议：范围很广，甚至讨论了高达12V的情况
  * SMBus：1.8V~5V

* 最小时钟频率、最大的`Clock Stretching `

  * Clock Stretching含义：某个设备需要更多时间进行内部的处理时，它可以把SCL拉低占住I2C总线

  * I2C协议：时钟频率最小值无限制，Clock Stretching时长也没有限制
  * SMBus：时钟频率最小值是10KHz，Clock Stretching的最大时间值也有限制

* 地址回应(Address Acknowledge)

  * 一个I2C设备接收到它的设备地址后，是否必须发出回应信号？
  * I2C协议：没有强制要求必须发出回应信号
  * SMBus：强制要求必须发出回应信号，这样对方才知道该设备的状态：busy，failed，或是被移除了

* SMBus协议明确了数据的传输格式
  * I2C协议：它只定义了怎么传输数据，但是并没有定义数据的格式，这完全由设备来定义
  * SMBus：定义了几种数据格式(后面分析)

* REPEATED START Condition(重复发出S信号)
  * 比如读EEPROM时，涉及2个操作：
    * 把存储地址发给设备
    * 读数据
  * 在写、读之间，可以不发出P信号，而是直接发出S信号：这个S信号就是`REPEATED START`
  * 如下图所示
    ![image-20210224100056055](https://gitee.com/zhanghang1999/typora-picture/raw/master/018_repeated_start.png)
* SMBus Low Power Version 
  * SMBus也有低功耗的版本

### SMBus协议分析

对于I2C协议，它只定义了怎么传输数据，但是并没有定义数据的格式，这完全由设备来定义。
对于SMBus协议，它定义了几种数据格式。



**注意**：

* 下面文档中的`Functionality flag`是Linux的某个I2C控制器驱动所支持的功能。
* 比如`Functionality flag: I2C_FUNC_SMBUS_QUICK`，表示需要I2C控制器支持`SMBus Quick Command`

#### symbols(符号)

```shell
S     (1 bit) : Start bit(开始位)
Sr    (1 bit) : 重复的开始位
P     (1 bit) : Stop bit(停止位)
R/W#  (1 bit) : Read/Write bit. Rd equals 1, Wr equals 0.(读写位)
A, N  (1 bit) : Accept and reverse accept bit.(回应位)
Address(7 bits): I2C 7 bit address. Note that this can be expanded as usual to
                get a 10 bit I2C address.
                (地址位，7位地址)
Command Code  (8 bits): Command byte, a data byte which often selects a register on
                the device.
                (命令字节，一般用来选择芯片内部的寄存器)
Data Byte (8 bits): A plain data byte. Sometimes, I write DataLow, DataHigh
                for 16 bit data.
                (数据字节，8位；如果是16位数据的话，用2个字节来表示：DataLow、DataHigh)
Count (8 bits): A data byte containing the length of a block operation.
				(在block操作总，表示数据长度)
[..]:           Data sent by I2C device, as opposed to data sent by the host
                adapter.
                (中括号表示I2C设备发送的数据，没有中括号表示host adapter发送的数据)
```

#### SMBus Quick Command

![image-20230607163334540](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607163334540.png)

只是用来发送一位数据：R/W#本意是用来表示读或写，但是在SMBus里可以用来表示其他含义。
比如某些开关设备，可以根据这一位来决定是打开还是关闭。

```shell
Functionality flag: I2C_FUNC_SMBUS_QUICK
```

#### SMBus Receive Byte

![image-20230607163403917](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607163403917.png)

I2C-tools中的函数：i2c_smbus_read_byte()。
读取一个字节，Host adapter接收到一个字节后不需要发出回应信号(上图中N表示不回应)。

```shell
Functionality flag: I2C_FUNC_SMBUS_READ_BYTE
```

#### SMBus Send Byte

![image-20230607171123489](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171123489.png)

I2C-tools中的函数：i2c_smbus_write_byte()。
发送一个字节。

```shell
Functionality flag: I2C_FUNC_SMBUS_WRITE_BYTE
```



#### SMBus Read Byte

![image-20230607171139834](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171139834.png)

I2C-tools中的函数：i2c_smbus_read_byte_data()。

先发出`Command Code`(它一般表示芯片内部的寄存器地址)，再读取一个字节的数据。
上面介绍的`SMBus Receive Byte`是不发送Comand，直接读取数据。

```shell
Functionality flag: I2C_FUNC_SMBUS_READ_BYTE_DATA
```



#### SMBus Read Word

![image-20210224111404096](https://gitee.com/zhanghang1999/typora-picture/raw/master/023_smbus_read_word.png)

I2C-tools中的函数：i2c_smbus_read_word_data()。

先发出`Command Code`(它一般表示芯片内部的寄存器地址)，再读取2个字节的数据。

```shell
Functionality flag: I2C_FUNC_SMBUS_READ_WORD_DATA
```



#### SMBus Write Byte

![image-20230607171152938](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171152938.png)

I2C-tools中的函数：i2c_smbus_write_byte_data()。

先发出`Command Code`(它一般表示芯片内部的寄存器地址)，再发出1个字节的数据。

```shell
Functionality flag: I2C_FUNC_SMBUS_WRITE_BYTE_DATA
```

#### SMBus Write Word

![image-20230607171204126](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171204126.png)

I2C-tools中的函数：i2c_smbus_write_word_data()。

先发出`Command Code`(它一般表示芯片内部的寄存器地址)，再发出1个字节的数据。

```shell
Functionality flag: I2C_FUNC_SMBUS_WRITE_WORD_DATA
```



#### SMBus Block Read

![image-20230607171213155](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171213155.png)

I2C-tools中的函数：i2c_smbus_read_block_data()。

先发出`Command Code`(它一般表示芯片内部的寄存器地址)，再发起度操作：

* 先读到一个字节(Block Count)，表示后续要读的字节数
* 然后读取全部数据

```shell
Functionality flag: I2C_FUNC_SMBUS_READ_BLOCK_DATA
```



#### SMBus Block Write

![image-20230607171226470](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171226470.png)

I2C-tools中的函数：i2c_smbus_write_block_data()。

先发出`Command Code`(它一般表示芯片内部的寄存器地址)，再发出1个字节的`Byte Conut`(表示后续要发出的数据字节数)，最后发出全部数据。

```shell
Functionality flag: I2C_FUNC_SMBUS_WRITE_BLOCK_DATA
```



#### I2C Block Read

在一般的I2C协议中，也可以连续读出多个字节。
它跟`SMBus Block Read`的差别在于设备发出的第1个数据不是长度N，如下图所示：

![image-20230607171234011](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171234011.png)

I2C-tools中的函数：i2c_smbus_read_i2c_block_data()。

先发出`Command Code`(它一般表示芯片内部的寄存器地址)，再发出1个字节的`Byte Conut`(表示后续要发出的数据字节数)，最后发出全部数据。

```shell
Functionality flag: I2C_FUNC_SMBUS_READ_I2C_BLOCK
```



#### I2C Block Write

在一般的I2C协议中，也可以连续发出多个字节。
它跟`SMBus Block Write`的差别在于发出的第1个数据不是长度N，如下图所示：

![image-20230607171241187](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171241187.png)

I2C-tools中的函数：i2c_smbus_write_i2c_block_data()。

先发出`Command Code`(它一般表示芯片内部的寄存器地址)，再发出1个字节的`Byte Conut`(表示后续要发出的数据字节数)，最后发出全部数据。

```shell
Functionality flag: I2C_FUNC_SMBUS_WRITE_I2C_BLOCK
```



#### SMBus Block Write - Block Read Process Call

![image-20230607171254401](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171254401.png)

先写一块数据，再读一块数据。

```shell
Functionality flag: I2C_FUNC_SMBUS_BLOCK_PROC_CALL
```



#### Packet Error Checking (PEC)

PEC是一种错误校验码，如果使用PEC，那么在P信号之前，数据发送方要发送一个字节的PEC码(它是CRC-8码)。

以`SMBus Send Byte`为例，下图中，一个未使用PEC，另一个使用PEC：

![image-20230607171302279](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171302279.png)



### SMBus和I2C的建议

因为很多设备都实现了SMBus，而不是更宽泛的I2C协议，所以优先使用SMBus。
即使I2C控制器没有实现SMBus，软件方面也是可以使用I2C协议来模拟SMBus。
所以：Linux建议优先使用SMBus。



## I2C系统的重要结构体

参考资料：

* Linux驱动程序: `drivers/i2c/i2c-dev.c`
* I2CTools: `https://mirrors.edge.kernel.org/pub/software/utils/i2c-tools/`

### I2C硬件框架

![image-20210208125100022](https://gitee.com/zhanghang1999/typora-picture/raw/master/001_i2c_hardware_block.png)



### I2C传输协议

* 写操作

![image-20230607171742170](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171742170.png)

* 读操作

![image-20230607171748859](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171748859.png)



### Linux软件框架

![image-20230607171754902](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171754902.png)



### 重要结构体

使用一句话概括I2C传输：APP通过I2C Controller与I2C Device传输数据。

在Linux中：

* 怎么表示I2C Controller

  * 一个芯片里可能有多个I2C Controller，比如第0个、第1个、……

  * 对于使用者，只要确定是第几个I2C Controller即可

  * 使用i2c_adapter表示一个I2C BUS，或称为I2C Controller

  * 里面有2个重要的成员：

    * nr：第几个I2C BUS(I2C Controller)

    * i2c_algorithm，里面有该I2C BUS的传输函数，用来收发I2C数据

  * i2c_adapter

  ![image-20230607171822758](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171822758.png)

  * i2c_algorithm
    ![image-20230607171831437](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171831437.png)

* 怎么表示I2C Device

  * 一个I2C Device，一定有**设备地址**
  * 它连接在哪个I2C Controller上，即对应的i2c_adapter是什么
  * 使用i2c_client来表示一个I2C Device
    ![image-20230607171843968](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171843968.png)

* 怎么表示要传输的数据

  * 在上面的i2c_algorithm结构体中可以看到要传输的数据被称为：i2c_msg

  * i2c_msg
    ![image-20230607171854358](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607171854358.png)

  * i2c_msg中的flags用来表示传输方向：bit 0等于I2C_M_RD表示读，bit 0等于0表示写

  * 一个i2c_msg要么是读，要么是写

  * 举例：设备地址为0x50的EEPROM，要读取它里面存储地址为0x10的一个字节，应该构造几个i2c_msg？

    * 要构造2个i2c_msg

    * 第一个i2c_msg表示写操作，把要访问的存储地址0x10发给设备

    * 第二个i2c_msg表示读操作

    * 代码如下

      ```c
      u8 data_addr = 0x10;
      i8 data;
      struct i2c_msg msgs[2];
      
      msgs[0].addr   = 0x50;
      msgs[0].flags  = 0;
      msgs[0].len    = 1;
      msgs[0].buf    = &data_addr;
      
      msgs[1].addr   = 0x50;
      msgs[1].flags  = I2C_M_RD;
      msgs[1].len    = 1;
      msgs[1].buf    = &data;
      ```

### 内核里怎么传输数据

使用一句话概括I2C传输：

* APP通过I2C Controller与I2C Device传输数据

* APP通过i2c_adapter与i2c_client传输i2c_msg

* 内核函数i2c_transfer

  * i2c_msg里含有addr，所以这个函数里不需要i2c_client

  ![image-20210223102320133](D:/note/note/note/kernel/subsystem/i2c/pic/04_I2C/016_i2c_transfer.png)

## 无需编写驱动直接访问设备\_I2C-Tools介绍

参考资料：

* Linux驱动程序: `drivers/i2c/i2c-dev.c`
* I2C-Tools-4.2: `https://mirrors.edge.kernel.org/pub/software/utils/i2c-tools/`
* AP3216C：
  * `git clone https://e.coding.net/weidongshan/01_all_series_quickstart.git`
  * 该GIT仓库中的文件《嵌入式Linux应用开发完全手册_韦东山全系列视频文档全集.pdf》
    * 第10.1篇，第十六章 I2C编程





APP访问硬件肯定是需要驱动程序的，
对于I2C设备，内核提供了驱动程序`drivers/i2c/i2c-dev.c`，通过它可以直接使用下面的I2C控制器驱动程序来访问I2C设备。
框架如下：

![image-20210224172517485](https://gitee.com/zhanghang1999/typora-picture/raw/master/030_i2ctools_and_i2c_dev.png)

i2c-tools是一套好用的工具，也是一套示例代码。



### 体验I2C-Tools

使用一句话概括I2C传输：APP通过I2C Controller与I2C Device传输数据。
所以使用I2C-Tools时也需要指定：

* 哪个I2C控制器(或称为I2C BUS、I2C Adapter)
* 哪个I2C设备(设备地址)
* 数据：读还是写、数据本身

#### 用法

i2cdetect：I2C检测

```shell
// 列出当前的I2C Adapter(或称为I2C Bus、I2C Controller)
i2cdetect -l

// 打印某个I2C Adapter的Functionalities, I2CBUS为0、1、2等整数
i2cdetect -F I2CBUS

// 看看有哪些I2C设备, I2CBUS为0、1、2等整数
i2cdetect -y -a I2CBUS

// 效果如下
# i2cdetect -l
i2c-1   i2c             STM32F7 I2C(0x40013000)                 I2C adapter
i2c-2   i2c             STM32F7 I2C(0x5c002000)                 I2C adapter
i2c-0   i2c             STM32F7 I2C(0x40012000)                 I2C adapter

# i2cdetect -F 0
Functionalities implemented by /dev/i2c-0:
I2C                              yes
SMBus Quick Command              yes
SMBus Send Byte                  yes
SMBus Receive Byte               yes
SMBus Write Byte                 yes
SMBus Read Byte                  yes
SMBus Write Word                 yes
SMBus Read Word                  yes
SMBus Process Call               yes
SMBus Block Write                yes
SMBus Block Read                 yes
SMBus Block Process Call         yes
SMBus PEC                        yes
I2C Block Write                  yes
I2C Block Read                   yes

// --表示没有该地址对应的设备, UU表示有该设备并且它已经有驱动程序,
// 数值表示有该设备但是没有对应的设备驱动
# i2cdetect -y -a 0  
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00: 00 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- UU -- -- -- 1e --
20: -- -- UU -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
```

i2cget：I2C读

```shell
# i2cget
Usage: i2cget [-f] [-y] [-a] I2CBUS CHIP-ADDRESS [DATA-ADDRESS [MODE]]
  I2CBUS is an integer or an I2C bus name
  ADDRESS is an integer (0x03 - 0x77, or 0x00 - 0x7f if -a is given)
  MODE is one of:
    b (read byte data, default)
    w (read word data)
    c (write byte/read byte)
    Append p for SMBus PEC
```

eg:

```shell
// 读一个字节: I2CBUS为0、1、2等整数, 表示I2C Bus; CHIP-ADDRESS表示设备地址
i2cget -f -y I2CBUS CHIP-ADDRESS

// 读某个地址上的一个字节: 
//    I2CBUS为0、1、2等整数, 表示I2C Bus
//    CHIP-ADDRESS表示设备地址
//    DATA-ADDRESS: 芯片上寄存器地址
//    MODE：有2个取值, b-使用`SMBus Read Byte`先发出DATA-ADDRESS, 再读一个字节, 中间无P信号
//                   c-先write byte, 在read byte，中间有P信号 
i2cget -f -y I2CBUS CHIP-ADDRESS DATA-ADDRESS MODE  

// 读某个地址上的2个字节: 
//    I2CBUS为0、1、2等整数, 表示I2C Bus
//    CHIP-ADDRESS表示设备地址
//    DATA-ADDRESS: 芯片上寄存器地址
//    MODE：w-表示先发出DATA-ADDRESS，再读2个字节
i2cget -f -y I2CBUS CHIP-ADDRESS DATA-ADDRESS MODE  
```

i2cset：I2C写

```shell
# i2cset
Usage: i2cset [-f] [-y] [-m MASK] [-r] [-a] I2CBUS CHIP-ADDRESS DATA-ADDRESS [VALUE] ... [MODE]
  I2CBUS is an integer or an I2C bus name
  ADDRESS is an integer (0x03 - 0x77, or 0x00 - 0x7f if -a is given)
  MODE is one of:
    c (byte, no value)
    b (byte data, default)
    w (word data)
  i (I2C block data)
    s (SMBus block data)
    Append p for SMBus PEC
```

eg:

```shell
// 写一个字节: I2CBUS为0、1、2等整数, 表示I2C Bus; CHIP-ADDRESS表示设备地址
  //           DATA-ADDRESS就是要写的数据
  i2cset -f -y I2CBUS CHIP-ADDRESS DATA-ADDRESS
  
  // 给address写1个字节(address, value):
  //           I2CBUS为0、1、2等整数, 表示I2C Bus; CHIP-ADDRESS表示设备地址
  //           DATA-ADDRESS: 8位芯片寄存器地址; 
  //           VALUE: 8位数值
  //           MODE: 可以省略，也可以写为b
  i2cset -f -y I2CBUS CHIP-ADDRESS DATA-ADDRESS VALUE [b]
  
  // 给address写2个字节(address, value):
  //           I2CBUS为0、1、2等整数, 表示I2C Bus; CHIP-ADDRESS表示设备地址
  //           DATA-ADDRESS: 8位芯片寄存器地址; 
  //           VALUE: 16位数值
  //           MODE: w
  i2cset -f -y I2CBUS CHIP-ADDRESS DATA-ADDRESS VALUE w
  
  // SMBus Block Write：给address写N个字节的数据
  //   发送的数据有：address, N, value1, value2, ..., valueN
  //   跟`I2C Block Write`相比, 需要发送长度N
  //           I2CBUS为0、1、2等整数, 表示I2C Bus; CHIP-ADDRESS表示设备地址
  //           DATA-ADDRESS: 8位芯片寄存器地址; 
  //           VALUE1~N: N个8位数值
  //           MODE: s
  i2cset -f -y I2CBUS CHIP-ADDRESS DATA-ADDRESS VALUE1 ... VALUEN s
  
  // I2C Block Write：给address写N个字节的数据
  //   发送的数据有：address, value1, value2, ..., valueN
  //   跟`SMBus Block Write`相比, 不需要发送长度N
  //           I2CBUS为0、1、2等整数, 表示I2C Bus; CHIP-ADDRESS表示设备地址
  //           DATA-ADDRESS: 8位芯片寄存器地址; 
  //           VALUE1~N: N个8位数值
  //           MODE: i
  i2cset -f -y I2CBUS CHIP-ADDRESS DATA-ADDRESS VALUE1 ... VALUEN i
```

i2ctransfer：I2C传输(不是基于SMBus)
```shell
# i2ctransfer
Usage: i2ctransfer [-f] [-y] [-v] [-V] [-a] I2CBUS DESC [DATA] [DESC [DATA]]...
  I2CBUS is an integer or an I2C bus name
  DESC describes the transfer in the form: {r|w}LENGTH[@address]
    1) read/write-flag 2) LENGTH (range 0-65535) 3) I2C address (use last one if omitted)
  DATA are LENGTH bytes for a write message. They can be shortened by a suffix:
    = (keep value constant until LENGTH)
    + (increase value by 1 until LENGTH)
    - (decrease value by 1 until LENGTH)
    p (use pseudo random generator until LENGTH with value as seed)

Example (bus 0, read 8 byte at offset 0x64 from EEPROM at 0x50):
  # i2ctransfer 0 w1@0x50 0x64 r8
Example (same EEPROM, at offset 0x42 write 0xff 0xfe ... 0xf0):
  # i2ctransfer 0 w17@0x50 0x42 0xff-
```

eg:

```shell
// Example (bus 0, read 8 byte at offset 0x64 from EEPROM at 0x50):
# i2ctransfer -f -y 0 w1@0x50 0x64 r8

// Example (bus 0, write 3 byte at offset 0x64 from EEPROM at 0x50):
# i2ctransfer -f -y 0 w9@0x50 0x64 val1 val2 val3

// Example 
// first: (bus 0, write 3 byte at offset 0x64 from EEPROM at 0x50)
// and then: (bus 0, read 3 byte at offset 0x64 from EEPROM at 0x50)
# i2ctransfer -f -y 0 w9@0x50 0x64 val1 val2 val3 r3@0x50  
# i2ctransfer -f -y 0 w9@0x50 0x64 val1 val2 val3 r3 //如果设备地址不变,后面的设备地址可省略
```

### 使用I2C-Tools操作传感器AP3216C

AP3216C是红外、光强、距离三合一的传感器，以读出光强、距离值为例，步骤如下：

* 复位：往寄存器0写入0x4
* 使能：往寄存器0写入0x3
* 读光强：读寄存器0xC、0xD得到2字节的光强
* 读距离：读寄存器0xE、0xF得到2字节的距离值

AP3216C的设备地址是0x1E，假设节在I2C BUS0上，操作命令如下：

* 使用SMBus协议

```shell
i2cset -f -y 0 0x1e 0 0x4
i2cset -f -y 0 0x1e 0 0x3
i2cget -f -y 0 0x1e 0xc w
i2cget -f -y 0 0x1e 0xe w
```



* 使用I2C协议

```shell
i2ctransfer -f -y 0 w2@0x1e 0 0x4
i2ctransfer -f -y 0 w2@0x1e 0 0x3
i2ctransfer -f -y 0 w1@0x1e 0xc r2
i2ctransfer -f -y 0 w1@0x1e 0xe r2
```

### I2C-Tools的访问I2C设备的2种方式

I2C-Tools可以通过SMBus来访问I2C设备，也可以使用一般的I2C协议来访问I2C设备。
使用一句话概括I2C传输：APP通过I2C Controller与I2C Device传输数据。
在APP里，有这几个问题：

* 怎么指定I2C控制器？
  * i2c-dev.c提供为每个I2C控制器(I2C Bus、I2C Adapter)都生成一个设备节点：/dev/i2c-0、/dev/i2c-1等待
  * open某个/dev/i2c-X节点，就是去访问该I2C控制器下的设备
* 怎么指定I2C设备？
  * 通过ioctl指定I2C设备的地址
  * ioctl(file,  I2C_SLAVE, address)
    * 如果该设备已经有了对应的设备驱动程序，则返回失败
  * ioctl(file,  I2C_SLAVE_FORCE, address)
    * 如果该设备已经有了对应的设备驱动程序
    * 但是还是想通过i2c-dev驱动来访问它
    * 则使用这个ioctl来指定I2C设备地址
* 怎么传输数据？
  * 两种方式
  * 一般的I2C方式：ioctl(file, I2C_RDWR, &rdwr)
  * SMBus方式：ioctl(file, I2C_SMBUS, &args)

### I2c-Tools代码流程

#### 使用I2C方式

示例代码：i2ctransfer.c

![image-20210224191404322](https://gitee.com/zhanghang1999/typora-picture/raw/master/031_i2ctransfer_flow.png)



#### 5.2 使用SMBus方式

示例代码：i2cget.c、i2cset.c

![image-20230607172917931](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607172917931.png)

## EEPROM访问

### 从芯片手册解析信息

#### 原理图

从芯片手册上可以知道，AT24C02的设备地址跟它的A2、A1、A0引脚有关：

![image-20230607173005171](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607173005171.png)

打开I2C模块的原理图：

![image-20230607173017516](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607173017516.png)

从原理图可知，A2A1A0都是0，所以AT24C02的设备地址是：0b1010000，即0x50。

#### 写数据

![image-20230607173110216](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607173110216.png)

#### 读数据

可以读1个字节，也可以连续读出多个字节。
连续读多个字节时，芯片内部的地址会自动累加。
当地址到达存储空间最后一个地址时，会从0开始。

![image-20230607173125505](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607173125505.png)

可以自己编程或者使用i2c-tools访问设备

## 内核i2c驱动代码流程

相关文件：

i2c-dev.c

i2c-core.c

硬件相关:

i2c-imx.c (imx6ull i2c控制器注册)

i2c-s3c2410.c(s3c2440 2ic控制器注册)

at24.c(at24 eeprom从设备驱动注册)

https://gitee.com/zhanghang1999/typora-picture/blob/master/i2c(linux4.9.88).html



### 注册适配器方法

注册一个已经编号的adapter到内核:

```cpp
int i2c_add_numbered_adapter(struct i2c_adapter *adap)
```

注册一个未编号的adapter到内核：

```cpp
int i2c_add_adapter(struct i2c_adapter *adapter)
```

### 注册从设备方法

i2c_client表示一个I2C设备:

#### i2c_register_board_info

2440中使用此方法静态注册了一个i2c设备，会在adapter注册时扫描到此设备并注册到内核

```cpp
int i2c_register_board_info(int busnum, struct i2c_board_info const *info, unsigned len);
```

#### dt

使用设备树，在设备树的i2c控制器节点下挂接设备，设备会在adapter注册时扫描控制器下子节点即从设备，注册到内核

```cpp
	i2c1: i2c@400a0000 {
		/* ... master properties skipped ... */
		clock-frequency = <100000>;

		flash@50 {
			compatible = "atmel,24c256";
			reg = <0x50>;
		};

		pca9532: gpio@60 {
			compatible = "nxp,pca9532";
			gpio-controller;
			#gpio-cells = <2>;
			reg = <0x60>;
		};
	};
```

#### i2c_new_device/i2c_new_probed_device

直接调用i2c_new_device/i2c_new_probed_device:

```cpp
struct i2c_client *
i2c_new_device(struct i2c_adapter *adap, struct i2c_board_info const *info)
    
    struct i2c_client *
i2c_new_probed_device(struct i2c_adapter *adap,
		      struct i2c_board_info *info,
		      unsigned short const *addr_list,
		      int (*probe)(struct i2c_adapter *, unsigned short addr))
```

差别：

* i2c_new_device：会创建i2c_client，即使该设备并不存在

* i2c_new_probed_device：

  * 它成功的话，会创建i2c_client，并且表示这个设备肯定存在

  * I2C设备的地址可能发生变化，比如AT24C02的引脚A2A1A0电平不一样时，设备地址就不一样

  * 可以罗列出可能的地址

  * i2c_new_probed_device使用这些地址判断设备是否存在

#### 来自adapter的属性文件

通过用户空间(user-space)生成
调试时、或者不方便通过代码明确地生成i2c_client时，可以通过用户空间来生成。

```shell
  // 创建一个i2c_client, .name = "eeprom", .addr=0x50, .adapter是i2c-3
  # echo eeprom 0x50 > /sys/bus/i2c/devices/i2c-3/new_device
  
  // 删除一个i2c_client
  # echo 0x50 > /sys/bus/i2c/devices/i2c-3/delete_device
```

#### i2c_add_driver/i2c_register_driver

注册一个i2c_driver，这个函数最后会遍历系统的adapter链表，并使用apapter探测i2c_driver->address_list成员中的地址列表，如果探测到的地址真是存在，该地址存在从设备，并注册设备.

此外，i2c_add_driver会倒置probe函数被调用，probe中可以什么也不做（只提供address_list)，也可以提供操作该从设备的接口，如使用nvmem_register为eeporm类似的设备创建另外一种访问方式（如at24.c),或者普通字符设备驱动程序

```cpp
#define i2c_add_driver(driver) \
	i2c_register_driver(THIS_MODULE, driver)

int i2c_register_driver(struct module *owner, struct i2c_driver *driver)
```

## 编写i2c设备驱动

### i2c_driver

```cpp
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/mod_devicetable.h>
#include <linux/log2.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/nvmem-provider.h>
#include <linux/platform_data/at24.h>
#include <asm/uaccess.h>
static int major = 0;
static struct class *ap3216_class;
struct i2c_client *g_ap3216_client;


static ssize_t ap3216_read(struct file *file, char __user *buf, size_t count,
		loff_t *offset)
{
	int size = 6;
	unsigned short value;
	unsigned char valBuf[6];
	int ret;
	if(count < size)
	{
		return -EINVAL;
	}

	value = i2c_smbus_read_word_data(g_ap3216_client, 0xa);
	valBuf[0] = value & 0xff;
	valBuf[1] = (value >> 8) & 0xff;
	value = i2c_smbus_read_word_data(g_ap3216_client, 0xc);
	valBuf[2] = value & 0xff;
	valBuf[3] = (value >> 8) & 0xff;
	value = i2c_smbus_read_word_data(g_ap3216_client, 0xe);
	valBuf[4] = value & 0xff;
	valBuf[5] = (value >> 8) & 0xff;
	
	ret = copy_to_user(buf, valBuf, size);
	return size;
}

static int ap3216_open(struct inode *inode, struct file *file)
{
	i2c_smbus_write_byte_data(g_ap3216_client, 0, 0x4);//reset
	mdelay(20);
	i2c_smbus_write_byte_data(g_ap3216_client, 0, 0x3);//ALS+PS+IR acitve
	mdelay(350);

	return 0;
}
static struct file_operations ap3216_fops = 
{

	.owner = THIS_MODULE,
	.read = ap3216_read,
	.open = ap3216_open,
};

static const struct of_device_id ap3216_of_match[] = {
	{ .compatible = "atmel,ap3216", },
	{ }
};
	
static const struct i2c_device_id ap3216_ids[] = {
	{ "ap3216", 0 },
	{ /* END OF LIST */ }
};	
	
static int ap3216_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk("ap3216_probe, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	g_ap3216_client = client;
	major = register_chrdev(0, "ap3216_chrdev", &ap3216_fops);
	if(major < 0)
	{
		printk("register_chrdev error, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	ap3216_class = class_create(THIS_MODULE, "ap3216_class");

	device_create(ap3216_class , NULL, MKDEV(major, 0), NULL, "ap3216_device");
	return 0;
}

static int ap3216_remove(struct i2c_client *client)
{
	printk("ap3216_remove, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	device_destroy(ap3216_class, MKDEV(major, 0));

	class_destroy(ap3216_class);
	
	unregister_chrdev(major, "ap3216_chrdev");

	return 0;
}
static struct i2c_driver ap3216_driver = {
	.driver = {
		.name = "ap3216_driver",
		.of_match_table = ap3216_of_match,
	},
	
	.probe = ap3216_probe,
	.remove = ap3216_remove,
	.id_table = ap3216_ids,
};

static int __init ap3216_init(void)
{
	
	printk("ap3216_init, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	return i2c_add_driver(&ap3216_driver);
}
module_init(ap3216_init);

static void __exit ap3216_exit(void)
{
	i2c_del_driver(&ap3216_driver);
}
module_exit(ap3216_exit);

MODULE_AUTHOR("hanson zhang");
MODULE_LICENSE("GPL");

```

### i2c_client

```cpp
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/mod_devicetable.h>
#include <linux/log2.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/nvmem-provider.h>
#include <linux/platform_data/at24.h>
#include <asm/uaccess.h>

struct i2c_client *client;

static struct i2c_board_info ap3216_i2c_info[] = {
	{ I2C_BOARD_INFO("ap3216", 0x1e), },
};

const unsigned short addr_list[] = {
		0x1e, I2C_CLIENT_END
};
#if 1
static int __init ap3216_client_init(void)
{
	struct i2c_adapter *adp;
	printk("ap3216_client_init, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	
	adp = i2c_get_adapter(0);
	
	client = i2c_new_device(adp, ap3216_i2c_info);
	
	
	i2c_put_adapter(adp);
	return 0;
}
#else
static int __init ap3216_client_init(void)
{
	struct i2c_adapter *adp;
	printk("ap3216_client_init, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	adp = i2c_get_adapter(0);//挂到i2c-0下
	
	client = i2c_new_probed_device(adp, ap3216_i2c_info, addr_list, NULL);
	
	i2c_put_adapter(adp);
	return 0;
}
#endif
module_init(ap3216_client_init);

static void __exit ap3216_client_exit(void)
{
	i2c_unregister_device(client);
}
module_exit(ap3216_client_exit);

MODULE_AUTHOR("hanson zhang");
MODULE_LICENSE("GPL");


```

### auto probe(不需要i2c_client)

```cpp
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/mod_devicetable.h>
#include <linux/log2.h>
#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/nvmem-provider.h>
#include <linux/platform_data/at24.h>
#include <asm/uaccess.h>
static int major = 0;
static struct class *ap3216_class;
struct i2c_client *g_ap3216_client;


static ssize_t ap3216_read(struct file *file, char __user *buf, size_t count,
		loff_t *offset)
{
	int size = 6;
	unsigned short value;
	unsigned char valBuf[6];
	int ret;
	if(count < size)
	{
		return -EINVAL;
	}

	value = i2c_smbus_read_word_data(g_ap3216_client, 0xa);
	valBuf[0] = value & 0xff;
	valBuf[1] = (value >> 8) & 0xff;
	value = i2c_smbus_read_word_data(g_ap3216_client, 0xc);
	valBuf[2] = value & 0xff;
	valBuf[3] = (value >> 8) & 0xff;
	value = i2c_smbus_read_word_data(g_ap3216_client, 0xe);
	valBuf[4] = value & 0xff;
	valBuf[5] = (value >> 8) & 0xff;
	
	ret = copy_to_user(buf, valBuf, size);
	return size;
}

static int ap3216_open(struct inode *inode, struct file *file)
{
	i2c_smbus_write_byte_data(g_ap3216_client, 0, 0x4);//reset
	mdelay(20);
	i2c_smbus_write_byte_data(g_ap3216_client, 0, 0x3);//ALS+PS+IR acitve
	mdelay(350);

	return 0;
}
static struct file_operations ap3216_fops = 
{
	.owner = THIS_MODULE,
	.read = ap3216_read,
	.open = ap3216_open,
};

static const struct of_device_id ap3216_of_match[] = {
	{ .compatible = "atmel,ap3216", },
	{ }
};
	
static const struct i2c_device_id ap3216_ids[] = {
	{ "ap3216", 0 },
	{ /* END OF LIST */ }
};	
	
static int ap3216_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk("ap3216_probe, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	g_ap3216_client = client;
	major = register_chrdev(0, "ap3216_chrdev", &ap3216_fops);
	if(major < 0)
	{
		printk("register_chrdev error, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}
	ap3216_class = class_create(THIS_MODULE, "ap3216_class");

	device_create(ap3216_class , NULL, MKDEV(major, 0), NULL, "ap3216_device");
	return 0;
}

static int ap3216_remove(struct i2c_client *client)
{
	printk("ap3216_remove, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	device_destroy(ap3216_class, MKDEV(major, 0));

	class_destroy(ap3216_class);
	
	unregister_chrdev(major, "ap3216_chrdev");

	return 0;
}

static const unsigned short normal_i2c[] = {
	0x1e,I2C_CLIENT_END
};
/*
static int i2c_imx_probe(struct platform_device *pdev)
{
	...
	i2c_imx->adapter.class = 1;//此方法需要设置适配器的class值
}

*/
static int ap3216_detect(struct i2c_client *client,
			  struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	
	printk("ap3216_detect, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
	{
		
		printk("i2c_check_functionality error, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
		return -ENODEV;
	}
	strlcpy(info->type, "ap3216",I2C_NAME_SIZE);

	return 0;
}

static struct i2c_driver ap3216_driver = {
	.driver = {
		.name = "ap3216_driver",
		.of_match_table = ap3216_of_match,
	},
	.class = I2C_CLASS_HWMON,
	.detect = ap3216_detect,
	.address_list = normal_i2c,
	.probe = ap3216_probe,
	.remove = ap3216_remove,
	.id_table = ap3216_ids,
};

static int __init ap3216_init(void)
{
	
	printk("ap3216_init, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	return i2c_add_driver(&ap3216_driver);
}
module_init(ap3216_init);

static void __exit ap3216_exit(void)
{
	printk("ap3216_exit, %s  %s  %d\n", __FILE__, __FUNCTION__, __LINE__);
	i2c_del_driver(&ap3216_driver);
}
module_exit(ap3216_exit);

MODULE_AUTHOR("hanson zhang");
MODULE_LICENSE("GPL");

```

## 虚拟的i2c适配器编写

```cpp
#include <linux/clk.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/dmapool.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_data/i2c-imx.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/sched.h>
#include <linux/slab.h>

static struct i2c_adapter *adapter;
static unsigned char eeprom_buffer[512];
static int eeprom_address_index = 0;

static int i2c_eeprom_transfer(struct i2c_adapter *adapter, struct i2c_msg *msg)
{
	int i;
	if(msg->flags & I2C_M_RD)
	{
		for(i = 0; i < msg->len; i++)
		{
			msg->buf[i] = eeprom_buffer[eeprom_address_index++];
			if(eeprom_address_index == 512)
				eeprom_address_index = 0;
		}
	}
	else 
	{
		if(msg->len >= 1)
		{
			eeprom_address_index = msg->buf[0];
			for(i = 1; i < msg->len; i++)
			{
				eeprom_buffer[eeprom_address_index++] = msg->buf[i];
				if(eeprom_address_index == 512)
					eeprom_address_index = 0;
			}
		}
	}
	return 0;
}

static int i2c_virtual_xfer(struct i2c_adapter *adapter,
						struct i2c_msg *msgs, int num)
{
	int i;
	for(i = 0; i < num; i++)
	{
		if(msgs[i].addr == 0x60)
		{
			i2c_eeprom_transfer(adapter, &msgs[i]);
		}
		else 
		{
			i = -EIO;
			break;
		}
	}

	return i;
}

static u32 i2c_virtual_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL
		| I2C_FUNC_SMBUS_READ_BLOCK_DATA;
}

static struct i2c_algorithm i2c_virtual_algo = {
	.master_xfer	= i2c_virtual_xfer,
	.functionality	= i2c_virtual_func,
};




static int i2c_virtual_adapter_probe(struct platform_device *pdev)
{
	adapter = kzalloc(sizeof(struct i2c_adapter), GFP_KERNEL);
	adapter->algo = &i2c_virtual_algo;
	adapter->nr = -1;
	
	strlcpy(adapter->name, "i2c-bus-virtual", sizeof(adapter->name));
	
	memset(eeprom_buffer, 0, 512);
	
	i2c_add_adapter(adapter);
	
	return 0;
}

static int i2c_virtual_adapter_remove(struct platform_device *pdev)
{
	i2c_del_adapter(adapter);
	kfree(adapter);
	return 0;
}

static const struct of_device_id i2c_virtual_adapter_ids[] = {
	{ .compatible = "none,i2c-bus-virtual"},

	{ /* sentinel */ }
};	

static struct platform_driver i2c_virtual_adapter_driver = {
	.probe = i2c_virtual_adapter_probe,
	.remove = i2c_virtual_adapter_remove,
	.driver = {
		.name = "i2c_virtual_adapter",
		.of_match_table = i2c_virtual_adapter_ids,
	},
};

static int __init i2c_virtual_adapter_init(void)
{
	return platform_driver_register(&i2c_virtual_adapter_driver);
}
module_init(i2c_virtual_adapter_init);

static void __exit i2c_virtual_adapter_exit(void)
{
	platform_driver_unregister(&i2c_virtual_adapter_driver);
}
module_exit(i2c_virtual_adapter_exit);




MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("i2c virtual adapter test");

```

设备树：(根节点下)

```shell
	i2c-bus-virtual {
		 compatible = "100ask,i2c-bus-virtual";
	};
```

这个程序虚拟一个i2c适配器，并虚拟一个eeprom地址为0x60，通过i2c-tools可以访问这个eeprom。

### 访问

* 列出I2C总线

  ```shell
  i2cdetect -l
  ```

  结果类似下列的信息：

  ```shell
  i2c-1   i2c             21a4000.i2c                             I2C adapter
  i2c-4   i2c             i2c-bus-virtual                         I2C adapter
  i2c-0   i2c             21a0000.i2c                             I2C adapter
  ```

  **注意**：不同的板子上，i2c-bus-virtual的总线号可能不一样，上问中总线号是4。

  

* 检查虚拟总线下的I2C设备

  ```shell
  // 假设虚拟I2C BUS号为4
  [root@100ask:~]# i2cdetect -y -a 4
       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
  00: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  70: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
  ```

  

* 读写模拟的EEPROM

  ```shell
  // 假设虚拟I2C BUS号为4
  [root@100ask:~]# i2cset -f -y 4 0x50 0 0x55   // 往0地址写入0x55
  [root@100ask:~]# i2cget -f -y 4 0x50 0        // 读0地址
  0x55
  ```

  

## gpio模拟i2c驱动分析

相关文件:

i2c-gpio.c

i2c-algo-bit.c

https://gitee.com/zhanghang1999/typora-picture/blob/master/i2c(linux4.9.88).html

## gpio模拟i2c实操

### 确认内核已经将i2c-gpio.c编译进内核

1.从i2c-gpio.c所在目录找到Makefile，找到对应的编译项

![image-20230607221557633](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607221557633.png)

2.从内核顶层目录的.config找到此项

![image-20230607221653027](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607221653027.png)

3.make menuconfig中搜索此项

![image-20230607221739490](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607221739490.png)

4.将其勾选为编译成模块

![image-20230607221839963](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607221839963.png)

5.执行make modules编译

6.在i2c-gpio.c所在目录生成i2c-gpio.ko，在板子上insmod

### 编写设备树

使用开发板扩展接口，接入eeporm，地址0x50:

![image-20230607223233504](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607223233504.png)

![image-20230607223407320](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607223407320.png)

diff文件

```shell
root@ubuntu:/home/zhhhhhh/bsp/IMX6ULL/100ask_imx6ull-sdk/Linux-4.9.88/arch/arm/boot/dts# diff 100ask_imx6ull-14x14.dts  ../../../../../orig_dts/100ask_imx6ull-14x14.dts
24,40c24
<     i2c_gpio_controller{
<       compatible = "i2c-gpio";
<       pinctrl-names = "default";
<       pinctrl-0 = <&pinctrl_i2c_gpio>;
<       gpios = <&gpio4 25 GPIO_ACTIVE_HIGH  /*sda*/
<                &gpio4 27 GPIO_ACTIVE_HIGH>; /*scl*/
<               clock-frequency = <100000>; /*100khz*/
<               status = "okay";
<               #address-cells = <1>;
<               #size-cells = <0>;
<       at24: at24@50 {
<                       compatible = "at24_device";
<                         reg = <0x50>;
<                         status = "okay";
<                 };
<
<     };
---
>
53,58d36
<       virtual_uart:virtual_uart{
<               compatible = "virtual_uart";
<               interrupt-parent = <&intc>;
<               interrupts = <GIC_SPI 99 IRQ_TYPE_LEVEL_HIGH>;
<
<       };
105a84
>
425,430d403
<       pinctrl_i2c_gpio: i2c_gpio_grp {                /*!< Function assigned for the core: Cortex-A7[ca7] */
<             fsl,pins = <
<                 MX6UL_PAD_CSI_DATA04__GPIO4_IO25           0x000010B0
<                 MX6UL_PAD_CSI_DATA06__GPIO4_IO27           0x000010B0
<               >;
<       };
937c910
<     status = "disabled";#这个节点使用了gpio模拟i2c时使用的节点，暂时屏蔽，否则会打印出错信息：
#[  103.403158] imx6ul-pinctrl 20e0000.iomuxc: pin MX6UL_PAD_CSI_DATA04 already requested by 2008000.ecspi; cannot claim for i2c_gpio_controller
#[  103.422760] imx6ul-pinctrl 20e0000.iomuxc: pin-125 (i2c_gpio_controller) status -22
#[  103.431572] imx6ul-pinctrl 20e0000.iomuxc: could not request pin 125 (MX6UL_PAD_CSI_DATA04) from group i2c_gpio_grp  on device 20e0000.iomuxc
#[  103.444976] i2c-gpio i2c_gpio_controller: Error applying setting, reverse things back
#[  103.454755] i2c-gpio: probe of i2c_gpio_controller failed with error -22

---
>     status = "okay";

```

![image-20230607233936661](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607233936661.png)

![image-20230607234023772](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607234023772.png)

![image-20230607234333649](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607234333649.png)

节点访问：

![image-20230607234353345](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230607234353345.png)

### 其他

内核自带at24.c，这是一个at24eeporm的i2c驱动程序，尝试使用dts中的i2c-gpio控制器的子节点与其匹配：

1. 尝试使用此驱动程序与i2c-4下的at24设备进行匹配时发现无法匹配（正常的i2c总线上设备和驱动匹配成功，会调用驱动程序的probe函数，最后会在/sys/bus/i2c/devices/4-0050下创建指向/sys/bus/i2c/drivers/at24驱动程序的链接，但是并没有出现这个链接文件）

2. 起初推测是gpio模拟i2c和正常i2c适配器不一样，最后发现即时将设备接在i2c-0下，也无法产生设备文件，而同处于i2c/devices/下的其他设备却正常能和自己的驱动程序匹配。

3. 通过使用stap脚本跟踪注册流程，发现at24 driver的probe函数返回错误，加入调试信息，发现是因为dts的compatible = "at24"（调试修改过)和i2c_driver的id_table的at24项匹配，而此项是空的，导致probe调用出错，解决办法：

   1. 将dts的comaptible改成与id_table中能正常匹配的项如compatible = "24c02"即可

   2. 阅读at24.c porbe函数源码，有另一种改法，需要提供at24设备的额外信息，可以参考mach-mini2440.c中at24设备的静态注册，在imx6ull中同样可以使用这种注册方式，这里选择使用设备树构造一个节点，然后在probe中解析这个节点：

      dts:

      ![image-20230608211928520](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608211928520.png)

      

      at24 probe函数新增部分

      ```cpp
      static int at24_probe(struct i2c_client *client, const struct i2c_device_id *id)
      {
      	struct at24_platform_data chip;
      	kernel_ulong_t magic = 0;
      	bool writable;
      	int use_smbus = 0;
      	int use_smbus_write = 0;
      	struct at24_data *at24;
      	int err;
      	unsigned i, num_addresses;
      	u8 test_byte;
      	/***********************************************新增部分******************************************************/
      	struct at24_platform_data *apd = NULL;
      	struct device_node *extra_node, *node;
      	node = client->dev.of_node;
      	int byte_len = -1;
      	int page_size =-1;
      	if(node)
      		extra_node = of_get_child_by_name(node, "extralll");
      	if(extra_node)
      	{
      		apd = devm_kcalloc(&client->dev, 1, sizeof(*apd), GFP_KERNEL)；
      		
      		if (of_property_read_u32(extra_node, "byte_len", &byte_len) == 0) {
              	// 成功读取 byte_len 属性的值
              	printk(KERN_WARNING "read extra data: byte_len = %d\n", byte_len);
          	}
      
          	if (of_property_read_u32(extra_node, "page_size", &page_size) == 0) {
              	// 成功读取 page_size 属性的值
              	printk(KERN_WARNING "read extra data: page_size = %d\n", page_size);
          	}
      		
      		of_node_put(extra_node);
      
      		if(byte_len && page_size)
      		{
      			apd->byte_len = byte_len;
      			apd->page_size = page_size;
      			client->dev.platform_data = (void *)apd;
      			
              	printk(KERN_WARNING "info.platform_data = (void *)apd;\n");
      		}
      			
      	}
          /***********************************************************************************************************/
      	if (client->dev.platform_data) {//第二种方法走这个分支
      		chip = *(struct at24_platform_data *)client->dev.platform_data;
      	} else {//第一种方法走这里
      		if (id) {
      			magic = id->driver_data;
      		} else {
      			const struct acpi_device_id *aid;
      
      			aid = acpi_match_device(at24_acpi_ids, &client->dev);
      			if (aid)
      				magic = aid->driver_data;
      		}
      		if (!magic)
      			return -ENODEV;
      
      		chip.byte_len = BIT(magic & AT24_BITMASK(AT24_SIZE_BYTELEN));
      		magic >>= AT24_SIZE_BYTELEN;
      		chip.flags = magic & AT24_BITMASK(AT24_SIZE_FLAGS);
      		/*
      		 * This is slow, but we can't know all eeproms, so we better
      		 * play safe. Specifying custom eeprom-types via platform_data
      		 * is recommended anyhow.
      		 */
      		chip.page_size = 1;
      
      		/* update chipdata if OF is present */
      		at24_get_ofdata(client, &chip);
      
      		chip.setup = NULL;
      		chip.context = NULL;
      	}
      
      	if (!is_power_of_2(chip.byte_len))
      		dev_warn(&client->dev,
      			"byte_len looks suspicious (no power of 2)!\n");
      	if (!chip.page_size) {
      		dev_err(&client->dev, "page_size must not be 0!\n");
      		return -EINVAL;
      	}
      	if (!is_power_of_2(chip.page_size))
      		dev_warn(&client->dev,
      			"page_size looks suspicious (no power of 2)!\n");
      
      	/*
      	 * REVISIT: the size of the EUI-48 byte array is 6 in at24mac402, while
      	 * the call to ilog2() in AT24_DEVICE_MAGIC() rounds it down to 4.
      	 *
      	 * Eventually we'll get rid of the magic values altoghether in favor of
      	 * real structs, but for now just manually set the right size.
      	 */
      	if (chip.flags & AT24_FLAG_MAC && chip.byte_len == 4)
      		chip.byte_len = 6;
      
      	/* Use I2C operations unless we're stuck with SMBus extensions. */
      	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
      		if (chip.flags & AT24_FLAG_ADDR16)
      			return -EPFNOSUPPORT;
      
      		if (i2c_check_functionality(client->adapter,
      				I2C_FUNC_SMBUS_READ_I2C_BLOCK)) {
      			use_smbus = I2C_SMBUS_I2C_BLOCK_DATA;
      		} else if (i2c_check_functionality(client->adapter,
      				I2C_FUNC_SMBUS_READ_WORD_DATA)) {
      			use_smbus = I2C_SMBUS_WORD_DATA;
      		} else if (i2c_check_functionality(client->adapter,
      				I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
      			use_smbus = I2C_SMBUS_BYTE_DATA;
      		} else {
      			return -EPFNOSUPPORT;
      		}
      
      		if (i2c_check_functionality(client->adapter,
      				I2C_FUNC_SMBUS_WRITE_I2C_BLOCK)) {
      			use_smbus_write = I2C_SMBUS_I2C_BLOCK_DATA;
      		} else if (i2c_check_functionality(client->adapter,
      				I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
      			use_smbus_write = I2C_SMBUS_BYTE_DATA;
      			chip.page_size = 1;
      		}
      	}
      
      	if (chip.flags & AT24_FLAG_TAKE8ADDR)
      		num_addresses = 8;
      	else
      		num_addresses =	DIV_ROUND_UP(chip.byte_len,
      			(chip.flags & AT24_FLAG_ADDR16) ? 65536 : 256);//这里将at24的存储空间分割成多个段，每段都是一个设备，只有第二种方法才会走这里
      
      	at24 = devm_kzalloc(&client->dev, sizeof(struct at24_data) +
      		num_addresses * sizeof(struct i2c_client *), GFP_KERNEL);
      	if (!at24)
      		return -ENOMEM;
      
      	mutex_init(&at24->lock);
      	at24->use_smbus = use_smbus;
      	at24->use_smbus_write = use_smbus_write;
      	at24->chip = chip;
      	at24->num_addresses = num_addresses;
      
      	if ((chip.flags & AT24_FLAG_SERIAL) && (chip.flags & AT24_FLAG_MAC)) {
      		dev_err(&client->dev,
      			"invalid device data - cannot have both AT24_FLAG_SERIAL & AT24_FLAG_MAC.");
      		return -EINVAL;
      	}
      
      	if (chip.flags & AT24_FLAG_SERIAL) {
      		at24->read_func = at24_eeprom_read_serial;
      	} else if (chip.flags & AT24_FLAG_MAC) {
      		at24->read_func = at24_eeprom_read_mac;
      	} else {
      		at24->read_func = at24->use_smbus ? at24_eeprom_read_smbus
      						  : at24_eeprom_read_i2c;
      	}
      
      	if (at24->use_smbus) {
      		if (at24->use_smbus_write == I2C_SMBUS_I2C_BLOCK_DATA)
      			at24->write_func = at24_eeprom_write_smbus_block;
      		else
      			at24->write_func = at24_eeprom_write_smbus_byte;
      	} else {
      		at24->write_func = at24_eeprom_write_i2c;
      	}
      
      	writable = !(chip.flags & AT24_FLAG_READONLY);
      	if (writable) {
      		if (!use_smbus || use_smbus_write) {
      
      			unsigned write_max = chip.page_size;
      
      			if (write_max > io_limit)
      				write_max = io_limit;
      			if (use_smbus && write_max > I2C_SMBUS_BLOCK_MAX)
      				write_max = I2C_SMBUS_BLOCK_MAX;
      			at24->write_max = write_max;
      
      			/* buffer (data + address at the beginning) */
      			at24->writebuf = devm_kzalloc(&client->dev,
      				write_max + 2, GFP_KERNEL);
      			if (!at24->writebuf)
      				return -ENOMEM;
      		} else {
      			dev_warn(&client->dev,
      				"cannot write due to controller restrictions.");
      		}
      	}
      
      	at24->client[0] = client;
      
      	/* use dummy devices for multiple-address chips */
      	for (i = 1; i < num_addresses; i++) {
      		at24->client[i] = i2c_new_dummy(client->adapter,
      					client->addr + i);//主次这几个被分割的设备(i=1开始，是因为这个设备在adapter中已经被创建了一次)
      		if (!at24->client[i]) {
      			dev_err(&client->dev, "address 0x%02x unavailable\n",
      					client->addr + i);
      			err = -EADDRINUSE;
      			goto err_clients;
      		}
      	}
      
      	i2c_set_clientdata(client, at24);
      
      	/*
      	 * Perform a one-byte test read to verify that the
      	 * chip is functional.
      	 */
      	err = at24_read(at24, 0, &test_byte, 1);
      	if (err) {
      		err = -ENODEV;
      		goto err_clients;
      	}
      
      	at24->nvmem_config.name = dev_name(&client->dev);
      	at24->nvmem_config.dev = &client->dev;
      	at24->nvmem_config.read_only = !writable;
      	at24->nvmem_config.root_only = true;
      	at24->nvmem_config.owner = THIS_MODULE;
      	at24->nvmem_config.compat = true;
      	at24->nvmem_config.base_dev = &client->dev;
      	at24->nvmem_config.reg_read = at24_read;
      	at24->nvmem_config.reg_write = at24_write;
      	at24->nvmem_config.priv = at24;
      	at24->nvmem_config.stride = 1;
      	at24->nvmem_config.word_size = 1;
      	at24->nvmem_config.size = chip.byte_len;
      
      	at24->nvmem = nvmem_register(&at24->nvmem_config);//eeprom可以通过nvmem方式访问
      
      	if (IS_ERR(at24->nvmem)) {
      		err = PTR_ERR(at24->nvmem);
      		goto err_clients;
      	}
      
      	dev_info(&client->dev, "%u byte %s EEPROM, %s, %u bytes/write\n",
      		chip.byte_len, client->name,
      		writable ? "writable" : "read-only", at24->write_max);
      	if (use_smbus == I2C_SMBUS_WORD_DATA ||
      	    use_smbus == I2C_SMBUS_BYTE_DATA) {
      		dev_notice(&client->dev, "Falling back to %s reads, "
      			   "performance will suffer\n", use_smbus ==
      			   I2C_SMBUS_WORD_DATA ? "word" : "byte");
      	}
      
      	/* export data to kernel code */
      	if (chip.setup)
      		chip.setup(at24->nvmem, chip.context);
      
      	return 0;
      
      err_clients:
      	for (i = 1; i < num_addresses; i++)
      		if (at24->client[i])
      			i2c_unregister_device(at24->client[i]);
      
      	return err;
      }
      

最后的效果如下：

![image-20230608223247119](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608223247119.png)



![image-20230608223403424](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230608223403424.png)



调试使用的stap文件:

```shell
probe begin {
        printf("begin\n")/*开始检测，脚本启动需要一定时间，最迟也需要等begin打印出来，脚本才会开始监视内核*/
}
/*这里表示要监视内核中'*'任意路径下fs/open.c中所有函数*/
probe kernel.function("*@drivers/base/*.c").call {
        if (execname() == "insmod") {
                printf("%s -> %s\n", thread_indent(4), ppfunc())/*当pid为命令行指定pid时，打印进程名和函数名*/
        }
}
/*同上，不同在于这里是函数返回时*/
probe kernel.function("*@drivers/base/*.c").return {
        if (execname() == "insmod") {
                printf("%s -> %s\n", thread_indent(-4), ppfunc())
        }
}

/*这里表示要监视内核中'*'任意路径下fs/open.c中所有函数*/
probe kernel.function("*@drivers/i2c/*.c").call {
        if (execname() == "insmod") {
                printf("%s -> %s\n", thread_indent(4), ppfunc())/*当pid为命令行指定pid时，打印进程名和函数名*/
        }
}
/*同上，不同在于这里是函数返回时*/
probe kernel.function("*@drivers/i2c/*.c").return {
        if (execname() == "insmod") {
                printf("%s -> %s\n", thread_indent(-4), ppfunc())
        }
}
/*这里表示要监视内核中'*'任意路径下fs/open.c中所有函数*/
probe kernel.function("*@drivers/misc/eeprom/at24.c").call {
        if (execname() == "insmod") {/*只检测insmod的调用*/
                printf("%s -> %s\n", thread_indent(4), ppfunc())/*当pid为命令行指定pid时，打印进程名和函数名*/
        }
}
/*同上，不同在于这里是函数返回时*/
probe kernel.function("*@drivers/misc/eeprom/at24.c").return {
        if (execname() == "insmod") {
                printf("%s -> %s\n", thread_indent(-4), ppfunc())
        }
}

```

