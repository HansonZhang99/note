# s3c2440 nandflash ctrl

nandflash在对大容量的数据存储中发挥着重要的作用。相对于norflash，它具有一些优势，但它的一个劣势是很容易产生坏块，因此在使用nandflash时，往往要利用校验算法发现坏块并标注出来，以便以后不再使用该坏块。nandflash没有地址或数据总线，如果是8位nandflash，那么它只有8个IO口，这8个IO口用于传输命令、地址和数据。nandflash主要以page（页）为单位进行读写，以block（块）为单位进行擦除。每一页中又分为main区和spare区，main区用于正常数据的存储，spare区用于存储一些附加信息，如块好坏的标记、块的逻辑地址、页内数据的ECC校验和等。

 三星公司是最主要的*nandflash*供应商，因此在它所开发的各类处理器中，实现对*nandflash*的支持就不足为奇了。*s3c2440*不仅具有*nandflash*的接口，而且还可以利用某些机制实现直接从*nandflash*启动并运行程序。本文只介绍如何对*nandflash*实现读、写、擦除等基本操作，不涉及*nandflash*启动程序的问题。

在这里，我们使用的*nandflash*为*K9F2G08U0A*，它是*8*位的*nandflash*。不同型号的*nandflash*的操作会有所不同，但硬件引脚基本相同，这给产品的开发带来了便利。因为不同型号的*PCB*板是一样的，只要更新一下软件就可以使用不同容量大小的*nandflash*。

*K9F2G08U0A*的一页为（*2K*＋*64*）字节（加号前面的*2K*表示的是*main*区容量，加号后面的*64*表示的是*spare*区容量），它的一块为*64*页，而整个设备包括了*2048*个块。这样算下来一共有*2112M*位容量，如果只算*main*区容量则有*256M*字节（即*256M*×*8*位）。要实现用*8*个*IO*口来要访问这么大的容量，*K9F2G08U0A*规定了用*5*个周期来实现。第一个周期访问的地址为*A0~A7*；第二个周期访问的地址为*A8~A11*，它作用在*IO0~IO3*上，而此时*IO4~IO7*必须为低电平；第三个周期访问的地址为*A12~A19*；第四个周期访问的地址为*A20~A27*；第五个周期访问的地址为*A28*，它作用在*IO0*上，而此时*IO1~IO7*必须为低电平。前两个周期传输的是列地址，后三个周期传输的是行地址。通过分析可知，列地址是用于寻址页内空间，行地址用于寻址页，如果要直接访问块，则需要从地址*A18*开始。

由于所有的命令、地址和数据全部从*8*位*IO*口传输，所以*nandflash*定义了一个命令集来完成各种操作。有的操作只需要一个命令（即一个周期）即可，而有的操作则需要两个命令（即两个周期）来实现。下面的宏定义为*K9F2G08U0A*的常用命令：

```cpp
#define CMD_READ1                 0x00              //页读命令周期1
#define CMD_READ2                 0x30              //页读命令周期2
#define CMD_READID               0x90              //读ID命令
#define CMD_WRITE1               0x80              //页写命令周期1
#define CMD_WRITE2               0x10              //页写命令周期2
#define CMD_ERASE1               0x60              //块擦除命令周期1
#define CMD_ERASE2               0xd0              //块擦除命令周期2
#define CMD_STATUS                0x70              //读状态命令
#define CMD_RESET                 0xff               //复位
#define CMD_RANDOMREAD1         0x05       //随意读命令周期1
#define CMD_RANDOMREAD2         0xE0       //随意读命令周期2
#define CMD_RANDOMWRITE         0x85       //随意写命令
```

在这里，随意读命令和随意写命令可以实现在一页内任意地址地读写。读状态命令可以实现读取设备内的状态寄存器，通过该命令可以获知写操作或擦除操作是否完成（判断第*6*位），以及是否成功完成（判断第*0*位）。



 下面介绍*s3c2440*的*nandflash*控制器。*s3c2440*支持*8*位或*16*位的每页大小为*256*字，*512*字节，*1K*字和*2K*字节的*nandflash*，这些配置是通过系统上电后相应引脚的高低电平来实现的。*s3c2440*还可以硬件产生*ECC*校验码，这为准确及时发现*nandflash*的坏块带来了方便。*nandflash*控制器的主要寄存器有*NFCONF*（*nandflash*配置寄存器），*NFCONT*（*nandflash*控制寄存器），*NFCMMD*（*nandflash*命令集寄存器），*NFADDR*（*nandflash*地址集寄存器），*NFDATA*（*nandflash*数据寄存器），*NFMECCD0/1*（*nandflash*的*main*区*ECC*寄存器），*NFSECCD*（*nandflash*的*spare*区*ECC*寄存器），*NFSTAT*（*nandflash*操作状态寄存器），*NFESTAT0/1*（*nandflash*的*ECC*状态寄存器），*NFMECC0/1*（*nandflash*用于数据的*ECC*寄存器），以及*NFSECC*（*nandflash*用于*IO*的*ECC*寄存器）。



*NFCMMD*，*NFADDR*和*NFDATA*分别用于传输命令，地址和数据，为了方便起见，我们可以定义一些宏定义用于完成上述操作：

```cpp
#define NF_CMD(data)               {rNFCMD  = (data); }        //传输命令
#define NF_ADDR(addr)             {rNFADDR = (addr); }         //传输地址
#define NF_RDDATA()               (rNFDATA)                         //读32位数据
#define NF_RDDATA8()              (rNFDATA8)                       //读8位数据
#define NF_WRDATA(data)         {rNFDATA = (data); }          //写32位数据
#define NF_WRDATA8(data)       {rNFDATA8 = (data); }        //写8位数据
```

其中*rNFDATA8*的定义为*(\*(volatile unsigned char \*)0x4E000010)*。

*NFCONF*主要用到了*TACLS*、*TWRPH0*、*TWRPH1*，这三个变量用于配置*nandflash*的时序。*s3c2440*的数据手册没有详细说明这三个变量的具体含义，但通过它所给出的时序图，我们可以看出，*TACLS*为*CLE/ALE*有效到*nWE*有效之间的持续时间，*TWRPH0*为*nWE*的有效持续时间，*TWRPH1*为*nWE*无效到*CLE/ALE*无效之间的持续时间，这些时间都是以*HCLK*为单位的（本文程序中的*HCLK=100MHz*）。通过查阅*K9F2G08U0A*的数据手册，我们可以找到并计算该*nandflash*与*s3c2440*相对应的时序：*K9F2G08U0A*中的*tWP*与*TWRPH0*相对应，*tCLH*与*TWRPH1*相对应，（*tCLS*－*tWP*）与*TACLS*相对应。*K9F2G08U0A*给出的都是最小时间，*s3c2440*只要满足它的最小时间即可，因此*TACLS*、*TWRPH0*、*TWRPH1*这三个变量取值大一些会更保险。在这里，这三个值分别取*1*，*2*和*0*。*NFCONF*的第*0*位表示的是外接的*nandflash*是*8*位*IO*还是*16*位*IO*，这里当然要选择*8*位的*IO*。*NFCONT*寄存器是另一个需要事先初始化的寄存器。它的第*13*位和第*12*位用于锁定配置，第*8*位到第*10*位用于*nandflash*的中断，第*4*位到第*6*位用于*ECC*的配置，第*1*位用于*nandflash*芯片的选取，第*0*位用于*nandflash*控制器的使能。另外，为了初始化*nandflash*，还需要配置*GPACON*寄存器，使它的第*17*位到第*22*位与*nandflash*芯片的控制引脚相对应。下面的程序实现了初始化*nandflash*控制器：

```cpp
void NF_Init ( void )
{
rGPACON = (rGPACON &~(0x3f<<17)) | (0x3f<<17);            //配置芯片引脚
//TACLS=1、TWRPH0=2、TWRPH1=0，8位IO
rNFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4)|(0<<0);
//非锁定，屏蔽nandflash中断，初始化ECC及锁定main区和spare区ECC，使能nandflash片选及控制器
       rNFCONT = (0<<13)|(0<<12)|(0<<10)|(0<<9)|(0<<8)|(1<<6)|(1<<5)|(1<<4)|(1<<1)|(1<<0);
}
```

为了更好地应用*ECC*和使能*nandflash*片选，我们还需要一些宏定义：

```cpp
#define NF_nFCE_L()                        {rNFCONT &= ~(1<<1); }
#define NF_CE_L()                            NF_nFCE_L()                                   //打开nandflash片选
#define NF_nFCE_H()                       {rNFCONT |= (1<<1); }
#define NF_CE_H()                           NF_nFCE_H()                            //关闭nandflash片选
#define NF_RSTECC()                       {rNFCONT |= (1<<4); }                     //复位ECC
#define NF_MECC_UnLock()             {rNFCONT &= ~(1<<5); }          //解锁main区ECC
#define NF_MECC_Lock()                 {rNFCONT |= (1<<5); }                     //锁定main区ECC
#define NF_SECC_UnLock()                     {rNFCONT &= ~(1<<6); }          //解锁spare区ECC
#define NF_SECC_Lock()                  {rNFCONT |= (1<<6); }                     //锁定spare区ECC
```

 *NFSTAT*是另一个比较重要的寄存器，它的第*0*位可以用于判断*nandflash*是否在忙，第*2*位用于检测*RnB*引脚信号：

```cpp
#define NF_WAITRB()                {while(!(rNFSTAT&(1<<0)));}           //等待nandflash不忙
#define NF_CLEAR_RB()           {rNFSTAT |= (1<<2); }                      //清除RnB信号
#define NF_DETECT_RB()         {while(!(rNFSTAT&(1<<2)));}           //等待RnB信号变高，即不忙
```

下面就详细介绍*K9F2G08U0A*的基本操作，包括复位，读*ID*，页读、写数据，随意读、写数据，块擦除等。

复位操作最简单，只需写入复位命令即可：

```cpp
static void rNF_Reset()
{
       NF_CE_L();                               //打开nandflash片选
       NF_CLEAR_RB();                      //清除RnB信号
       NF_CMD(CMD_RESET);           //写入复位命令
       NF_DETECT_RB();                    //等待RnB信号变高，即不忙
       NF_CE_H();                               //关闭nandflash片选

}
```

读取*K9F2G08U0A*芯片*ID*操作首先需要写入读*ID*命令，然后再写入*0x00*地址，就可以读取到一共五个周期的芯片*ID*，第一个周期为厂商*ID*，第二个周期为设备*ID*，第三个周期至第五个周期包括了一些具体的该芯片信息，这里就不多介绍：

```cpp
static char rNF_ReadID()
{
        char pMID;
        char pDID;
        char cyc3, cyc4, cyc5;

        NF_nFCE_L();                           //打开nandflash片选
        NF_CLEAR_RB();                      //清RnB信号
        NF_CMD(CMD_READID);         //读ID命令
        NF_ADDR(0x0);                        //写0x00地址
        //读五个周期的ID
        pMID = NF_RDDATA8();                   //厂商ID：0xEC
        pDID = NF_RDDATA8();                   //设备ID：0xDA
        cyc3 = NF_RDDATA8();                     //0x10
        cyc4 = NF_RDDATA8();                     //0x95
        cyc5 = NF_RDDATA8();                     //0x44
        NF_nFCE_H();                    //关闭nandflash片选
        return (pDID);
}

```

下面介绍读操作，读操作是以页为单位进行的。如果在读取数据的过程中不进行*ECC*校验判断，则读操作比较简单，在写入读命令的两个周期之间写入要读取的页地址，然后读取数据即可。如果为了更准确地读取数据，则在读取完数据之后还要进行*ECC*校验判断，以确定所读取的数据是否正确。

 

在上文中我们已经介绍过，*nandflash*的每一页有两区：*main*区和*spare*区，*main*区用于存储正常的数据，*spare*区用于存储其他附加信息，其中就包括*ECC*校验码。当我们在写入数据的时候，我们就计算这一页数据的*ECC*校验码，然后把校验码存储到*spare*区的特定位置中，在下次读取这一页数据的时候，同样我们也计算*ECC*校验码，然后与*spare*区中的*ECC*校验码比较，如果一致则说明读取的数据正确，如果不一致则不正确。*ECC*的算法较为复杂，好在*s3c2440*能够硬件产生*ECC*校验码，这样就省去了不少的麻烦事。*s3c2440*即可以产生*main*区的*ECC*校验码，也可以产生*spare*区的*ECC*校验码。因为*K9F2G08U0A*是*8*位*IO*口，因此*s3c2440*共产生*4*个字节的*main*区*ECC*码和*2*个字节的*spare*区*ECC*码。在这里我们规定，在每一页的*spare*区的第*0*个地址到第*3*个地址存储*main*区*ECC*，第*4*个地址和第*5*个地址存储*spare*区*ECC*。产生*ECC*校验码的过程为：在读取或写入哪个区的数据之前，先解锁该区的*ECC*，以便产生该区的*ECC*。在读取或写入完数据之后，再锁定该区的*ECC*，这样系统就会把产生的*ECC*码保存到相应的寄存器中。*main*区的*ECC*保存到*NFMECC0/1*中（因为*K9F2G08U0A*是*8*位*IO*口，因此这里只用到了*NFMECC0*），*spare*区的*ECC*保存到*NFSECC*中。对于读操作来说，我们还要继续读取*spare*区的相应地址内容，已得到上次写操作时所存储的*main*区和*spare*区的*ECC*，并把这些数据分别放入*NFMECCD0/1*和*NFSECCD*的相应位置中。最后我们就可以通过读取*NFESTAT0/1*（因为*K9F2G08U0A*是*8*位*IO*口，因此这里只用到了*NFESTAT0*）中的低*4*位来判断读取的数据是否正确，其中第*0*位和第*1*位为*main*区指示错误，第*2*位和第*3*位为*spare*区指示错误。

下面就给出一段具体的页读操作程序：

```cpp
U8 rNF_ReadPage(U32 page_number)
{
        U32 i, mecc0, secc;
        NF_RSTECC();                   //复位ECC
        NF_MECC_UnLock();          //解锁main区ECC
        NF_nFCE_L();                           //打开nandflash片选
        NF_CLEAR_RB();                      //清RnB信号
        NF_CMD(CMD_READ1);           //页读命令周期1

        //写入5个地址周期
        NF_ADDR(0x00);                                            //列地址A0~A7
        NF_ADDR(0x00);                                            //列地址A8~A11
        NF_ADDR((page_number) & 0xff);                  //行地址A12~A19
        NF_ADDR((page_number >> 8) & 0xff);           //行地址A20~A27
        NF_ADDR((page_number >> 16) & 0xff);         //行地址A28

        NF_CMD(CMD_READ2);           //页读命令周期2
        NF_DETECT_RB();                    //等待RnB信号变高，即不忙
        //读取一页数据内容
        for (i = 0; i < 2048; i++)
        {
                buffer[i] =  NF_RDDATA8();
        }
        NF_MECC_Lock();                     //锁定main区ECC值
        NF_SECC_UnLock();                  //解锁spare区ECC
        mecc0=NF_RDDATA();        //读spare区的前4个地址内容，即第2048~2051地址，这4个字节为main区的ECC
        //把读取到的main区的ECC校验码放入NFMECCD0/1的相应位置内
        rNFMECCD0=((mecc0&0xff00)<<8)|(mecc0&0xff);
        rNFMECCD1=((mecc0&0xff000000)>>8)|((mecc0&0xff0000)>>16);

        NF_SECC_Lock();               //锁定spare区的ECC值
        secc=NF_RDDATA();           //继续读spare区的4个地址内容，即第2052~2055地址，其中前2个字节为spare区的ECC值
        //把读取到的spare区的ECC校验码放入NFSECCD的相应位置内
        rNFSECCD=((secc&0xff00)<<8)|(secc&0xff);
        NF_nFCE_H();             //关闭nandflash片选
        //判断所读取到的数据是否正确
        if ((rNFESTAT0&0xf) == 0x0)
                return 0x66;                  //正确
        else
                return 0x44;                  //错误
}

```

这段程序是把某一页的内容读取到全局变量数组*buffer*中。该程序的输入参数直接就为*K9F2G08U0A*的第几页，例如我们要读取第*128064*页中的内容，可以调用该程序为：*rNF_ReadPage(128064);*。由于第*128064*页是第*2001*块中的第*0*页（*128064*＝*2001*×*64*＋*0*），所以为了更清楚地表示页与块之间的关系，也可以写为：*rNF_ReadPage(2001\*64);*。

 

​    页写操作的大致流程为：在两个写命令周期之间分别写入页地址和数据，当然如果为了保证下次读取该数据时的正确性，还需要把*main*区的*ECC*值和*spare*区的*ECC*值写入到该页的*spare*区内。然后我们还需要读取状态寄存器，以判断这次写操作是否正确。下面就给出一段具体的页写操作程序，其中输入参数也是要写入数据到第几页：

```cpp
U8 rNF_WritePage(U32 page_number)
{
        U32 i, mecc0, secc;
        U8 stat, temp;
        temp = rNF_IsBadBlock(page_number>>6);              //判断该块是否为坏块
        if(temp == 0x33)
                return 0x42;           //是坏块，返回
        NF_RSTECC();                   //复位ECC
        NF_MECC_UnLock();          //解锁main区的ECC
        NF_nFCE_L();             //打开nandflash片选
        NF_CLEAR_RB();        //清RnB信号
        NF_CMD(CMD_WRITE1);                //页写命令周期1

        //写入5个地址周期
        NF_ADDR(0x00);                                     //列地址A0~A7
        NF_ADDR(0x00);                                     //列地址A8~A11
        NF_ADDR((page_number) & 0xff);           //行地址A12~A19
        NF_ADDR((page_number >> 8) & 0xff);    //行地址A20~A27
        NF_ADDR((page_number >> 16) & 0xff);  //行地址A28
        //写入一页数据
        for (i = 0; i < 2048; i++)
        {
                NF_WRDATA8((char)(i+6));
        }
        NF_MECC_Lock();                     //锁定main区的ECC值
        mecc0=rNFMECC0;                    //读取main区的ECC校验码
        //把ECC校验码由字型转换为字节型，并保存到全局变量数组ECCBuf中
        ECCBuf[0]=(U8)(mecc0&0xff);
        ECCBuf[1]=(U8)((mecc0>>8) & 0xff);
        ECCBuf[2]=(U8)((mecc0>>16) & 0xff);
        ECCBuf[3]=(U8)((mecc0>>24) & 0xff);
        NF_SECC_UnLock();                  //解锁spare区的ECC
        //把main区的ECC值写入到spare区的前4个字节地址内，即第2048~2051地址
        for(i=0;i<4;i++)
        {
                NF_WRDATA8(ECCBuf[i]);
        }
        NF_SECC_Lock();                      //锁定spare区的ECC值
        secc=rNFSECC;                   //读取spare区的ECC校验码
        //把ECC校验码保存到全局变量数组ECCBuf中
        ECCBuf[4]=(U8)(secc&0xff);
        ECCBuf[5]=(U8)((secc>>8) & 0xff);
        //把spare区的ECC值继续写入到spare区的第2052~2053地址内
        for(i=4;i<6;i++)
        {
                NF_WRDATA8(ECCBuf[i]);
        }
        NF_CMD(CMD_WRITE2);                //页写命令周期2
        delay(1000);          //延时一段时间，以等待写操作完成
        NF_CMD(CMD_STATUS);                 //读状态命令
        //判断状态值的第6位是否为1，即是否在忙，该语句的作用与NF_DETECT_RB();相同
        do{
                stat = NF_RDDATA8();
        }while(!(stat&0x40));
        NF_nFCE_H();                    //关闭nandflash片选
        //判断状态值的第0位是否为0，为0则写操作正确，否则错误
        if (stat & 0x1)
        {
                temp = rNF_MarkBadBlock(page_number>>6);         //标注该页所在的块为坏块
                if (temp == 0x21)
                        return 0x43            //标注坏块失败
                else
                        return 0x44;           //写操作失败
        }
        else
                return 0x66;                  //写操作成功
}
```

该段程序先判断该页所在的坏是否为坏块，如果是则退出。在最后写操作失败后，还要标注该页所在的块为坏块，其中所用到的函数*rNF_IsBadBlock*和*rNF_MarkBadBlock*，我们在后面介绍。我们再总结一下该程序所返回数值的含义，*0x42*：表示该页所在的块为坏块；*0x43*：表示写操作失败，并且在标注该页所在的块为坏块时也失败；*0x44*：表示写操作失败，但是标注坏块成功；*0x66*：写操作成功。

 

​    擦除是以块为单位进行的，因此在写地址周期是，只需写三个行周期，并且要从*A18*开始写起。与写操作一样，在擦除结束前还要判断是否擦除操作成功，另外同样也存在需要判断是否为坏块以及要标注坏块的问题。下面就给出一段具体的块擦除操作程序：

```cpp
U8 rNF_EraseBlock(U32 block_number)
{
        char stat, temp;
        temp = rNF_IsBadBlock(block_number);     //判断该块是否为坏块
        if(temp == 0x33)
                return 0x42;           //是坏块，返回
        NF_nFCE_L();             //打开片选
        NF_CLEAR_RB();        //清RnB信号
        NF_CMD(CMD_ERASE1);         //擦除命令周期1
        //写入3个地址周期，从A18开始写起
        NF_ADDR((block_number << 6) & 0xff);         //行地址A18~A19
        NF_ADDR((block_number >> 2) & 0xff);         //行地址A20~A27
        NF_ADDR((block_number >> 10) & 0xff);        //行地址A28

        NF_CMD(CMD_ERASE2);         //擦除命令周期2
        delay(1000);          //延时一段时间
        NF_CMD(CMD_STATUS);          //读状态命令

        //判断状态值的第6位是否为1，即是否在忙，该语句的作用与NF_DETECT_RB();相同
        do{
                stat = NF_RDDATA8();
        }while(!(stat&0x40));
        NF_nFCE_H();             //关闭nandflash片选
        //判断状态值的第0位是否为0，为0则擦除操作正确，否则错误
        if (stat & 0x1)
        {
                temp = rNF_MarkBadBlock(page_number>>6);         //标注该块为坏块
                if (temp == 0x21)
                        return 0x43            //标注坏块失败
                else
                        return 0x44;           //擦除操作失败
        }
        else
                return 0x66;                  //擦除操作成功
}

```

该程序的输入参数为*K9F2G08U0A*的第几块，例如我们要擦除第*2001*块，则调用该函数为：*rNF_EraseBlock(2001)*。

 

​    *K9F2G08U0A*除了提供了页读和页写功能外，还提供了页内地址随意读、写功能。页读和页写是从页的首地址开始读、写，而随意读、写实现了在一页范围内任意地址的读、写。随意读操作是在页读操作后输入随意读命令和页内列地址，这样就可以读取到列地址所指定地址的数据。随意写操作是在页写操作的第二个页写命令周期前，输入随意写命令和页内列地址，以及要写入的数据，这样就可以把数据写入到列地址所指定的地址内。下面两段程序实现了随意读和随意写功能，其中随意读程序的输入参数分别为页地址和页内地址，输出参数为所读取到的数据，随意写程序的输入参数分别为页地址，页内地址，以及要写入的数据。

```cpp
U8 rNF_RamdomRead(U32 page_number, U32 add)
{
        NF_nFCE_L();                    //打开nandflash片选
        NF_CLEAR_RB();               //清RnB信号
        NF_CMD(CMD_READ1);           //页读命令周期1
        //写入5个地址周期
        NF_ADDR(0x00);                                            //列地址A0~A7
        NF_ADDR(0x00);                                            //列地址A8~A11
        NF_ADDR((page_number) & 0xff);                  //行地址A12~A19
        NF_ADDR((page_number >> 8) & 0xff);           //行地址A20~A27
        NF_ADDR((page_number >> 16) & 0xff);         //行地址A28
        NF_CMD(CMD_READ2);          //页读命令周期2
        NF_DETECT_RB();                    //等待RnB信号变高，即不忙
        NF_CMD(CMD_RANDOMREAD1);                 //随意读命令周期1
        //页内地址
        NF_ADDR((char)(add&0xff));                          //列地址A0~A7
        NF_ADDR((char)((add>>8)&0x0f));                 //列地址A8~A11
        NF_CMD(CMD_RANDOMREAD2);                //随意读命令周期2
        return NF_RDDATA8();               //读取数据
}


 
U8 rNF_RamdomWrite(U32 page_number, U32 add, U8 dat)
{
        U8 temp,stat;
        NF_nFCE_L();                    //打开nandflash片选
        NF_CLEAR_RB();               //清RnB信号
        NF_CMD(CMD_WRITE1);                //页写命令周期1
        //写入5个地址周期
        NF_ADDR(0x00);                                     //列地址A0~A7
        NF_ADDR(0x00);                                     //列地址A8~A11
        NF_ADDR((page_number) & 0xff);           //行地址A12~A19
        NF_ADDR((page_number >> 8) & 0xff);    //行地址A20~A27
        NF_ADDR((page_number >> 16) & 0xff);  //行地址A28
        NF_CMD(CMD_RANDOMWRITE);                 //随意写命令
        //页内地址
        NF_ADDR((char)(add&0xff));                   //列地址A0~A7
        NF_ADDR((char)((add>>8)&0x0f));          //列地址A8~A11
        NF_WRDATA8(dat);                          //写入数据
        NF_CMD(CMD_WRITE2);                //页写命令周期2

        delay(1000);                 //延时一段时间
        NF_CMD(CMD_STATUS);                        //读状态命令

        //判断状态值的第6位是否为1，即是否在忙，该语句的作用与NF_DETECT_RB();相同
        do{
                stat =  NF_RDDATA8();
        }while(!(stat&0x40));

        NF_nFCE_H();                    //关闭nandflash片选
        //判断状态值的第0位是否为0，为0则写操作正确，否则错误
        if (stat & 0x1)
                return 0x44;                  //失败
        else
                return 0x66;                  //成功
}

```

下面介绍上文中提到的判断坏块以及标注坏块的那两个程序：*rNF_IsBadBlock*和*rNF_MarkBadBlock*。在这里，我们定义在*spare*区的第*6*个地址（即每页的第*2054*地址）用来标注坏块，*0x44*表示该块为坏块。要判断坏块时，利用随意读命令来读取*2054*地址的内容是否为*0x44*，要标注坏块时，利用随意写命令来向*2054*地址写*0x33*。下面就给出这两个程序，它们的输入参数都为块地址，也就是即使仅仅一页出现问题，我们也标注整个块为坏块。

```cpp
U8 rNF_IsBadBlock(U32 block)
{
        return rNF_RamdomRead(block*64, 2054);
}

U8 rNF_MarkBadBlock(U32 block)
{
        U8 result;
        result = rNF_RamdomWrite(block*64, 2054, 0x33);
        if(result == 0x44)
                return 0x21;                  //写坏块标注失败
        else
                return 0x60;                  //写坏块标注成功

}

```

