# Linux路由配置

route命令可查看内核路由表：

```
# ifconfig
eth0      Link encap:Ethernet  HWaddr 44:A6:42:42:CD:AE
          inet addr:10.10.114.172  Bcast:10.10.114.255  Mask:255.255.255.0
          inet6 addr: fe80::46a6:42ff:fe42:cdae/64 Scope:Link
          UP BROADCAST RUNNING PROMISC MULTICAST  MTU:1500  Metric:1
          RX packets:834912 errors:0 dropped:584 overruns:0 frame:0
          TX packets:100372 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000
          RX bytes:413804451 (394.6 MiB)  TX bytes:22104775 (21.0 MiB)

eth1      Link encap:Ethernet  HWaddr 44:A6:42:42:CD:AF
          inet addr:10.10.116.198  Bcast:10.10.116.255  Mask:255.255.255.0
          UP BROADCAST MULTICAST  MTU:1500  Metric:1
          RX packets:0 errors:0 dropped:0 overruns:0 frame:0
          TX packets:0 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000
          RX bytes:0 (0.0 B)  TX bytes:0 (0.0 B)
          Interrupt:51 Base address:0x4000

eth2      Link encap:Ethernet  HWaddr 44:A6:42:42:CD:AE
          inet addr:203.0.113.65  Bcast:203.0.113.255  Mask:255.255.255.0
          inet6 addr: fe80::46a6:42ff:fe42:cdae/64 Scope:Link
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:114932 errors:0 dropped:0 overruns:0 frame:0
          TX packets:114111 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000
          RX bytes:11864684 (11.3 MiB)  TX bytes:6180407 (5.8 MiB)

lo        Link encap:Local Loopback
          inet addr:127.0.0.1  Mask:255.0.0.0
          inet6 addr: ::1/128 Scope:Host
          UP LOOPBACK RUNNING  MTU:65536  Metric:1
          RX packets:33 errors:0 dropped:0 overruns:0 frame:0
          TX packets:33 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1
          RX bytes:2196 (2.1 KiB)  TX bytes:2196 (2.1 KiB)


# route
Kernel IP routing table
Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
default         10.10.114.254   0.0.0.0         UG    0      0        0 eth0
10.10.114.0     *               255.255.255.0   U     0      0        0 eth0
203.0.113.0     *               255.255.255.0   U     0      0        0 eth2
224.0.0.0       *               240.0.0.0       U     0      0        0 eth0
```

| 项          | 说明                                                         |
| ----------- | ------------------------------------------------------------ |
| Destination | 目的网段或主机                                               |
| Gateway     | 网关地址,*表示本主机所属网络，不需要路由或未设置             |
| Genmask     | 子网掩码                                                     |
| Flags       | 标记                                                         |
|             | U:路由是活动的                                               |
|             | H:目标是一个主机                                             |
|             | G:路由指向网关                                               |
|             | R:恢复动态路由所产生的表项                                   |
|             | D:由路由的后台程序动态安装                                   |
|             | M:由路由的后台程序修改                                       |
|             | !:拒绝路由                                                   |
| Metric      | 路由距离,到达指定网络所需要的中转数，大型局域网和广域网使用，Linux内核不使用 |
| Ref         | 路由项引用次数，不在Linux内核使用                            |
| Use         | 此路由项被路由软件查找的次数                                 |
| Iface       | 此路由的数据包将发送到的接口                                 |


主机路由:
主机路由是路由选择表中指向单个IP地址或主机名的路由记录。主机路由的Flags字段为H。例如，在下面的示例中，本地主机通过IP地址192.168.1.1的路由器到达IP地址为10.0.0.10的主机。

```
Destination    Gateway       Genmask Flags     Metric    Ref    Use    Iface
-----------    -------     -------            -----     ------    ---    ---    -----
10.0.0.10     192.168.1.1    255.255.255.255   UH       0    0      0    eth0
```

网络路由:
网络路由是代表主机可以到达的网络。网络路由的Flags字段为N。例如，在下面的示例中，本地主机将发送到网络192.19.12的数据包转发到IP地址为192.168.1.1的路由器。

```
Destination    Gateway       Genmask Flags    Metric    Ref     Use    Iface
-----------    -------     -------         -----    -----   ---    ---    -----
192.19.12     192.168.1.1    255.255.255.0      UN      0       0     0    eth0
```

默认路由:
当主机不能在路由表中查找到目标主机的IP地址或网络路由时，数据包就被发送到默认路由（默认网关）上。默认路由的Flags字段为G。例如，在下面的示例中，默认路由是IP地址为192.168.1.1的路由器。

```
Destination    Gateway       Genmask Flags     Metric    Ref    Use    Iface
-----------    -------     ------- -----      ------    ---    ---    -----
default       192.168.1.1     0.0.0.0    UG       0        0     0    eth0
```

route 命令
设置和查看路由表都可以用 route 命令，设置内核路由表的命令格式是：
`route  [add|del] [-net|-host] target [netmask Nm] [gw Gw] [[dev] If]`


其中：
add : 添加一条路由规则
del : 删除一条路由规则
-net : 目的地址是一个网络
-host : 目的地址是一个主机
target : 目的网络或主机
netmask : 目的地址的网络掩码
gw : 路由数据包通过的网关
dev : 为路由指定的网络接口

添加路由：

主机路由：
添加到10.10.113.123的主机路由，指定网卡为eth0，数据通过网关10.10.114.254

```
route add -host 10.10.113.123  gateway 10.10.114.254 dev eth0
```

网络路由

```
route add -net 10.10.113.0 netmask 255.255.255.0 gw 10.10.114.254 dev eth0
```

默认路由：

```
route add default gw 10.10.114.254
```

删除路由：

```
route del -net 10.10.113.0 netmask 255.255.255.0 gw 10.10.114.254 dev eth0
route del -host 10.10.113.123 gw 10.10.114.254 dev eth0
route del default gw 10.10.113.254
```

静态路由组网示例：
PC1 eth0 ------ lan route1 wan ------ lan route2 wan ------ eth0 pc2
PC1有线网卡eth0的IP为：10.10.113.121/24
route1的LAN口IP为10.10.113.254/24   WAN口IP为192.168.1.101/24
route2的LAN口IP为192.168.1.102/24   WAN口IP为192.168.2.254/24
PC2有线网卡eth0的IP为192.168.2.103/24

实现各个网段之间的相互通信：
首先PC1与192.168.1.x/25网段通信：
配置静态路由：

```
route add -net 192.168.1.0/24 gw 10.10.113.254 dev eth0 (PC1->route2)
route add -net 10.10.113.0/24 gw 192.168.1.101 (route2->PC1)
```

此时PC1可ping通route网段

```
route add -net 192.168.2.0/24 gw 192.168.1.102(route1->PC2)
route add -net 192.168.1.0/24 gw 192.168.2.254(PC2->route1)
```

此时PC2可ping通route网段

```
route add -net 192.168.2.0/24 gw 10.10.113.254(PC1->PC2)
route add -net 10.10.113.0/24 gw 192.168.2.254(PC2->PC1)
```

此时PC1和PC2网段可相互ping通