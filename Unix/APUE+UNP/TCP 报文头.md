# TCP 报文头

![image-20230424005432776](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005432776.png)


#### 标志位

`URG`代表此包是否包含紧急数据

`ACK`确认号，ACK的值在TCP三次握手后的正常传输中是置位的

![image-20230424005440762](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005440762.png)

`PSH`表示收到的tcp包是否需要直接上传到应用层，0表示放在缓冲区，1表示直接上传。此标志可以将缓冲区数据立即推送到应用层而不是等待操作系统的调度。（可以利用发送大量PSH=0的TCP包来破坏传输）

![image-20230424005456135](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005456135.png)



`RST`收到带RST标志的报文，说明主机的连接出现严重的错误（如主机崩溃）必须释放连接，再重新建立连接。或者上次发给主机的数据有问题，主机拒绝响应，带RST标志的TCP报文段被称为复位报文段

![image-20230424005506527](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005506527.png)



`SYN`同步报文，在建立连接时使用

`FIN`表示通知对方本端要关闭连接了，标记数据是否发送完毕。FIN=1，告诉对方：我的数据已经发送完毕，你可以释放连接了，带FIN标志的TCP报文段被称为结束报文段

#### 序号

seq序号，32位，用来标识从TCP源端向目的端发送的字节流，发起方发送数据时对此进行标记。

#### 确认序号

Ack序号，占32位，只有ACK标志位为1时，确认序号字段才有效，确认方Ack=发起方Seq+1，两端配对



### 三次握手

1. ```
   Client将标志位SYN置为1，随机产生一个值seq=J，并将该数据包发送给Server，Client进入SYN_SENT状态，等待Server确认。
   ```

2. ```
   Server收到数据包后由标志位SYN=1知道Client请求建立连接，Server将标志位SYN和ACK都置为1，ack=J+1，随机产生一个值seq=K，并将该数据包发送给Client以确认连接请求，Server进入SYN_RCVD状态。
   ```

3. ```
   Client收到确认后，检查ack是否为J+1，syn是否为1，如果正确则将标志位ACK置为1，ack=k+1，并将该数据包发送给Server，Server检查ack是否为K+1，ACK是否为1，如果正确则连接建立成功，
   Client和Server进入ESTABLISHED状态，完成三次握手，随后Client与Server之间可以开始传输数据了。
   ```



### 四次挥手

由于TCP连接时全双工的，因此，每个方向都必须要单独进行关闭，这一原则是当一方完成数据发送任务后，发送一个FIN来终止这一方向的连接，收到一个FIN只是意味着这一方向上没有数据流动了，即不会再收到数据了，但是在这个TCP连接上仍然能够发送数据，直到这一方向也发送了FIN。首先进行关闭的一方将执行主动关闭，而另一方则执行被动关闭。

1. ```
   Client发送一个FIN，用来关闭Client到Server的数据传送，Client进入FIN_WAIT_1状态。
   ```

2. ```
   Server收到FIN后，发送一个ACK给Client，确认序号为收到序号+1（与SYN相同，一个FIN占用一个序号），Server进入CLOSE_WAIT状态。
   ```

3. ```
   Server发送一个FIN，用来关闭Server到Client的数据传送，Server进入LAST_ACK状态。
   ```

4. ```
   Client收到FIN后，Client进入TIME_WAIT_2状态，接着发送一个ACK给Server，确认序号为收到序号+1，Server进入CLOSED状态，完成四次挥手。
   ```

为什么建立连接是三次握手，而关闭连接却是四次挥手呢？

```
这是因为服务端在LISTEN状态下，收到建立连接请求的SYN报文后，把ACK和SYN放在一个报文里发送给客户端。而关闭连接时，当收到对方的FIN报文时，仅仅表示对方不再发送数据了但是还能接收数据，己方也未必全部数据都发送给对方了，所以己方可以立即close，也可以发送一些数据给对方后，再发送FIN报文给对方来表示同意现在关闭连接，因此，己方ACK和FIN一般都会分开发送。
```

![image-20230424005519953](https://gitee.com/zhanghang1999/typora-picture/raw/master/image-20230424005519953.png)