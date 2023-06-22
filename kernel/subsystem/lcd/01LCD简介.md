# lcd简介

## 应用工程师眼中的lcd

### 分辨率

lcd由一个一个像素点组成，每行由xres个像素，一共有yres行，分辨率就是xres * yres。

![image-20230525180435700](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230525180435700.png)

### 像素

用红绿蓝三颜色来表示，可以用24位数据来表示红绿蓝，也可以用16位等等格式，比如：

- bpp: bit per pixel，每个像素用多少位来表示
- 24bpp: 使用24位表示一个像素，实际会使用32位，有八位没有使用，其余24位各使用8为表示R(red),G(green),B(bule)，即rgb888
- 16bpp: 有rgb565,rgb555
  - rgb565: 5位表示红，6位表示绿，5位表示蓝
  - rgb555: 16位中各使用5位表示rgb,浪费1位

![image-20230525181232819](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230525181232819.png)

### 写像素

假设每个像素的颜色用16位来表示，那么一个LCD的所有像素点假设有xres * y res个，
需要的内存为：xres * yres * 16 / 8，也就是要设置所有像素的颜色，需要这么大小的内存。
这块内存就被称为framebuffer：

* Framebuffer中每块数据对应一个像素
* 每块数据的大小可能是16位、32位，这跟LCD上像素的颜色格式有关
* 设置好LCD硬件后，只需要把颜色数据写入Framebuffer即可

![image-20230525182133009](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230525182133009.png)

## lcd硬件模型

### 统一的LCD硬件模型

![image-20230525183949721](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230525183949721.png)

### MCU常用的8080接口LCD模组

![image-20230525182243044](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230525182243044.png)

8080接口的lcd中，内存,lcd控制器和lcd是合在一起的，被称为LCM。

LCM被接在cpu上，为了和cpu通信，至少需要接入数据总线、读写信号、用来表示命令还是数据的选择信息（这里如果介入地址线会很麻烦，不如直接通过选择信号来表示数据线中的数据是地址还是数据）、和片选信号。这种LCM的分辨率不会太大，内存也比较贵。

### MPU/ARM常用的TFT RGB接口

![image-20230525182930942](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230525182930942.png)

这种接口中，lcd控制器被集成在ARM芯片上，控制器可以直接读取内存的数据，并发送给lcd屏幕。

lcd控制器和lcd直接的接口通常包括如下部分：

- DCLK: 电子枪指向lcd左上角，并向右移动，移动的速度和DCLK相关，DCLK每个周期电子枪移动一次
- RGB三组线: 用来传输RGB数据的数据线
- HSYNC: 当电子枪移动到一行的尾部时，需要一个HSYNC信号，让电子枪移动到下一行的行首
- VSYNC: 当电子枪移动到最后一行尾时，需要VSYNC信号，让电子枪移动到第一行行首
- DE: data enable，并不是每一个DCLK周期的数据都是有效的，只有在DE信号有效时，电子枪才会从RGB中取出颜色

### MIPI标准

**MIPI**表示`Mobile Industry Processor Interface`，即移动产业处理器接口。是MIPI联盟发起的为移动应用处理器制定的开放标准和一个规范。主要是手机内部的接口（摄像头、显示屏接口、射频/基带接口）等标准化，从而减少手机内部接口的复杂程度及增加设计的灵活性。

对于LCD，MIPI接口可以分为3类：

* MIPI-DBI (Display Bus Interface)  

  * 既然是Bus(总线)，就是既能发送数据，也能发送命令，常用的8080接口就属于DBI接口。

  * Type B (i-80 system), 8-/9-/16-/18-/24-bit bus  
  * Type C (Serial data transfer interface, 3/4-line SPI)  
* MIPI-DPI (Display Pixel Interface)，TFT RGB属于DPI接口

  * Pixel(像素)，强调的是操作单个像素，在MPU上的LCD控制器就是这种接口

  * Supports 24 bit/pixel (R: 8-bit, G: 8-bit, B: 8-bit)
  * Supports 18 bit/pixel (R: 6-bit, G: 6-bit, B: 6-bit)
  * Supports 16 bit/pixel (R: 5-bit, G: 6-bit, B: 5-bit)  

* MIPI-DSI (Display Serial Interface)  
  * Serial，相比于DBI、DPI需要使用很多接口线，DSI需要的接口线大为减少
  * Supports one data lane/maximum speed 500Mbps
  * Supports DSI version 1.01
  * Supports D-PHY version 1.00  
    ![image-20210102124710351](https://gitee.com/zhanghang1999/typora-picture/raw/master/005_mipi_dsi.png)

