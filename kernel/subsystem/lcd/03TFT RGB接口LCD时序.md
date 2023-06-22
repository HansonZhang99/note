# TFT RGB接口LCD时序

## STD7.0TFT1024600-13-F LCD时序图

### 同步模式

#### 时序

##### 水平时序图

![image-20230526175615135](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230526175615135.png)

##### TFT LCD 水平和垂直同步的直观图

![image-20230526180202893](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230526180202893.png)

##### 垂直时序图

![image-20230526180714908](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230526180714908.png)

#### 总结

水平同步信号：电子枪从lcd左上角开始向右移动，当移动到最后时，通过HSYNC信号跳动到下一行行首。当电子枪移动到右下角时，通过垂直同步信号VSYNC移动到左上角。并不是所有的DCLK周期数据都是有效的，lcd有效数据只有灰色部分。

- DCLK: 时钟信号，LCD通过时钟信号周期性获取RGB数据，时钟频率越高，数据传输越快
- DATA: 数据的RGB数据
- HSD: 即HSYNC信号，低电平有效

在水平方向：

- thpw(HSYNC width): HSYNC信号的持续时间
- thb(HBP): 行首无效数据持续时间
- thfp(HFP): 行尾无效数据持续时间
- th: 整个周期
- thd: 数据有效时间

在垂直方向：

VSD: 垂直同步信号，低电平有效

tvpw(VSYNC width): 垂直同步信号持续时间

tvb(VBP): 数据无效持续时间

tvfp(VFP): 数据无效时间

tvd: 数据有效时间

tv: 整个传输周期

### DE模式

#### 时序

##### 水平时序图

![image-20230526182146466](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230526182146466.png)

##### 垂直时序图

![image-20230526182252910](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230526182252910.png)

#### 总结

水平时序：

- DE: 水平方向（一行）数据有效时间内，DE信号处于高电平
- tH: 水平方向总时间
- tHB: 数据无效时间
- tHA: 数据有效时间

垂直时序

- DE: 整个屏幕周期，包括所有行的数据，DATA中每个数据块（1,2,3...)都是一行数据，每行数据都有有效和无效的时间。电子枪从右下角最后一个有效的位置移动到左上角第一个有效位置的时间为垂直同步无效时间
- tV: 垂直方向总时间
- tVB: 数据无效时间
- tVA: 数据有效时间

### 取值

LCD 数据手册提供了两种模式下的一些时间取值：

![image-20230526183838510](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230526183838510.png)

![image-20230526183900058](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230526183900058.png)



## imx6ull soc lcd接口图

![image-20230526184931938](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230526184931938.png)

## lcd 接口图

![image-20230526185403468](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230526185403468.png)![image-20230529001247787](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230529001247787.png)