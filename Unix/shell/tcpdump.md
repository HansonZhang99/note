# tcpdump



tcpdumptcpdump简单使用：
抓取100(-c)次 eth0网口的包，源地址为10.10.113.235，源端口为22，目的地址为10.10.113.23，目的端口为1000

`tcpdump -i eth0 -c 100 '(src 10.10.113.235 and tcp port 22 and dst 10.10.113.23 and tcp port 1000)'`
抓取100(-c)次 eth0网口的包，源地址为10.10.113.235，源端口为22，目的地址为10.10.113.23

`tcpdump -i eth0 -c 100 '(src 10.10.113.235 and tcp port 22 and dst 10.10.113.23)'`
抓取100(-c)次 eth0网口的包，10.10.113.235:22到10.10.113.23 或 10.10.113.23到10.10.113.235:22

`tcpdump -i eth0 -c 100 '(src 10.10.113.235 and tcp port 22 and dst 10.10.113.23)' or '(src 10.10.113.23 and dst 10.10.113.235 and tcp port 22 )'`
可使用-w wireshark.cap保存为可用wireshark分析的文件