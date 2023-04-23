# 用于检测目的地址某个TCP端口是否开启：telnet/curl/ssh/wget
`telnet [ip] [tcp_port]`

```
[Tue Nov 16 14:43:14~]$ telnet 127.0.0.1  22
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.
SSH-2.0-OpenSSH_7.6p1 Ubuntu-4ubuntu0.5
^Cexit
Connection closed by foreign host.
[Tue Nov 16 14:43:35~]$ telnet 127.0.0.1  21
Trying 127.0.0.1...
telnet: Unable to connect to remote host: Connection refused
[Tue Nov 16 14:43:40~]$ telnet 127.0.0.1  23
Trying 127.0.0.1...
telnet: Unable to connect to remote host: Connection refused
[Tue Nov 16 14:43:42~]$ telnet 127.0.0.1  53
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.

```

`curl [ip]:[tcp_port]`
```
[Tue Nov 16 14:46:28~]$ curl 127.0.0.1:22
SSH-2.0-OpenSSH_7.6p1 Ubuntu-4ubuntu0.5
Protocol mismatch.
curl: (56) Recv failure: Connection reset by peer
[Tue Nov 16 14:46:29~]$ curl 127.0.0.1:23
curl: (7) Failed to connect to 127.0.0.1 port 23: Connection refused
[Tue Nov 16 14:46:34~]$ curl 127.0.0.1:53

^C
[Tue Nov 16 14:46:46~]$ curl 127.0.0.1:80
curl: (7) Failed to connect to 127.0.0.1 port 80: Connection refused

```

`ssh -v -p tcp_port username@ip/domain`

```
[Tue Nov 16 14:48:24~]$ ssh -v -p 23 root@127.0.0.1
OpenSSH_7.6p1 Ubuntu-4ubuntu0.5, OpenSSL 1.0.2n  7 Dec 2017
debug1: Reading configuration data /etc/ssh/ssh_config
debug1: /etc/ssh/ssh_config line 19: Applying options for *
debug1: Connecting to 127.0.0.1 [127.0.0.1] port 23.
debug1: connect to address 127.0.0.1 port 23: Connection refused
ssh: connect to host 127.0.0.1 port 23: Connection refused
[Tue Nov 16 14:48:37~]$ ssh -v -p 22 root@127.0.0.1
OpenSSH_7.6p1 Ubuntu-4ubuntu0.5, OpenSSL 1.0.2n  7 Dec 2017
debug1: Reading configuration data /etc/ssh/ssh_config
debug1: /etc/ssh/ssh_config line 19: Applying options for *
debug1: Connecting to 127.0.0.1 [127.0.0.1] port 22.
debug1: Connection established.
...
debug1: Trying private key: /data1/zhanghang22/.ssh/id_ecdsa
debug1: Trying private key: /data1/zhanghang22/.ssh/id_ed25519
debug1: Next authentication method: password
root@127.0.0.1's password:

[Tue Nov 16 14:48:46~]$ ssh -v -p 53 root@127.0.0.1
OpenSSH_7.6p1 Ubuntu-4ubuntu0.5, OpenSSL 1.0.2n  7 Dec 2017
debug1: Reading configuration data /etc/ssh/ssh_config
debug1: /etc/ssh/ssh_config line 19: Applying options for *
debug1: Connecting to 127.0.0.1 [127.0.0.1] port 53.
debug1: Connection established.
...
debug1: Local version string SSH-2.0-OpenSSH_7.6p1 Ubuntu-4ubuntu0.5
^C
[Tue Nov 16 14:48:53~]$ ssh -v -p 56 root@127.0.0.1
OpenSSH_7.6p1 Ubuntu-4ubuntu0.5, OpenSSL 1.0.2n  7 Dec 2017
debug1: Reading configuration data /etc/ssh/ssh_config
debug1: /etc/ssh/ssh_config line 19: Applying options for *
debug1: Connecting to 127.0.0.1 [127.0.0.1] port 56.
debug1: connect to address 127.0.0.1 port 56: Connection refused
ssh: connect to host 127.0.0.1 port 56: Connection refused
[Tue Nov 16 14:48:58~]$

```

`wget [ip]:[tcp_port]`
```
[Tue Nov 16 14:51:10~]$ wget 127.0.0.1:21
--2021-11-16 14:51:13--  http://127.0.0.1:21/
Connecting to 127.0.0.1:21... failed: Connection refused.
[Tue Nov 16 14:51:13~]$ wget 127.0.0.1:23
--2021-11-16 14:51:16--  http://127.0.0.1:23/
Connecting to 127.0.0.1:23... failed: Connection refused.
[Tue Nov 16 14:51:16~]$ wget 127.0.0.1:22
--2021-11-16 14:51:18--  http://127.0.0.1:22/
Connecting to 127.0.0.1:22... connected.
HTTP request sent, awaiting response... 200 No headers, assuming HTTP/0.9
Length: unspecified
Saving to: ‘index.html’

index.html                                             [ <=>                                                                                                            ]      60  --.-KB/s    in 0s

2021-11-16 14:51:18 (7.16 MB/s) - Read error at byte 60 (Connection reset by peer).Retrying.

--2021-11-16 14:51:19--  (try: 2)  http://127.0.0.1:22/
Connecting to 127.0.0.1:22... connected.
HTTP request sent, awaiting response... 200 No headers, assuming HTTP/0.9
Length: unspecified
Saving to: ‘index.html’

index.html                                             [ <=>                                                                                                            ]      60  --.-KB/s    in 0s

2021-11-16 14:51:19 (3.56 MB/s) - Read error at byte 60 (Connection reset by peer).Retrying.

^C
[Tue Nov 16 14:51:19~]$ wget 127.0.0.1:53
--2021-11-16 14:51:22--  http://127.0.0.1:53/
Connecting to 127.0.0.1:53... connected.
HTTP request sent, awaiting response... ^C

```