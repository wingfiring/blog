# 翻墙笔记

## 方案

## 软件准备

下载 [kcptun](https://github.com/xtaci/kcptun/releases/download/v20190924/kcptun-linux-amd64-20190924.tar.gz) [rsock](https://github.com/iceonsun/rsock/releases/download/v2.0.1/rsock-Linux-x86_64-20180613.tar.gz)

## 配置

按照[kcptun文档](https://github.com/xtaci/kcptun/blob/master/README.md)的建议，首先需要增加文件句柄数限制，`ulimit -n 65535`, 这个可以放在`.bashrc`中.然后修改`sysctl.conf`.

```config
net.core.rmem_max=26214400 // BDP - bandwidth delay product
net.core.rmem_default=26214400
net.core.wmem_max=26214400
net.core.wmem_default=26214400
net.core.netdev_max_backlog=2048 // proportional to -rcvwnd
```

```bash
KCP Client: ./client_kcptun -r "139.162.81.81:61111" -l ":7890" -mode fast3 -nocomp -autoexpire 900 -sockbuf 16777217 -dscp 46
KCP Server: ./server_kcptun -t "127.0.0.1:64022" -l ":61111" -mode fast3 -nocomp -sockbuf 16777217 -dscp 46
```

### CA 配置

新的EasyRSA版本是3.x了，和2.x不兼容。我也不想研究怎么升级上来了，新开一个CA吧。易用性方面似乎稍有改进。 步骤如下：

1. 创建CA根目录,并初始化

   ```bash
   make-cadir app-ca
   cd app-ca
   ./easyrsa init-pki
   ```

   如果要定制vars文件，可以在`init-pki`之前修改好。也就是预设一些`set_var`值。核心的内容都在`pki`目录下面，永远都不要手工去动里面的任何文件，细节相当之繁琐。重要的文件有：`ca.crt`，`dh.pem`，后面命令会生成这些文件。重要的目录有：`private`,私钥；`reqs`，签名请求；`issued`,签好名的证书。`renewed`和`revoked`也是顾名思义。
2. 初始化CA：

   ```bash
   ./easyrsa build-ca nopass
   ```

   `nopass`的作用是免设置密码，默认是要求设置密码。中途按照提示，填一下CN。随便写，默认是`Easy-RSA CA`.
3. 生成dh文件

    ```bash
    ./easyrsa gen-dh
    ```

    这一步可能很慢。
4. 生成服务器证书：

   ```bash
   ./easyrsa build-server-full openvpn-server nopass
   ```

   `openvpn-server`是随便起的证书名字。最好起个有含义的，好管理。

5. 生成客户端证书

   ```bash
   ./easyrsa build-client-full openvpn-client nopass
   ```

### openvpn配置

1. 生成ta.key

    ```bash
    openvpn --genkey --secret ta.key
    ```

2. 服务器端配置文件

    ```config
    dev tun
    persist-key
    persist-tun
    topology subnet
    port 1194
    proto udp
    keepalive 10 120

    # Location of certificate authority's cert.
    ca /etc/openvpn/server/ca.crt

    # Location of VPN server's TLS cert.
    cert /etc/openvpn/server/server.crt

    # Location of server's TLS key
    key /etc/openvpn/server/server.key

    # Location of DH parameter file.
    dh /etc/openvpn/server/dh.pem

    # The VPN's address block starts here.
    server 10.89.0.0 255.255.255.0

    explicit-exit-notify 1

    # Drop root privileges and switch to the `ovpn` user after startup.
    user ovpn

    # OpenVPN process is exclusive member of ovpn group.
    group ovpn

    # Cryptography options. We force these onto clients by
    # setting them here and not in client.ovpn. See
    # `openvpn --show-tls`, `openvpn --show-ciphers` and
    #`openvpn --show-digests` for all supported options.
    tls-crypt /etc/openvpn/server/ta.key
    auth SHA512    # This needs to be in client.ovpn too though.
    tls-version-min 1.2
    tls-cipher TLS-DHE-RSA-WITH-AES-256-GCM-SHA384:TLS-DHE-RSA-WITH-AES-256-CBC-SHA256
    ncp-ciphers AES-256-GCM:AES-256-CBC

    # Logging options.
    ifconfig-pool-persist ipp.txt
    status openvpn-status.log
    log /var/log/openvpn.log
    verb 3
    ```

3. 客户端配置

    ```config
    client
    dev tun
    persist-key
    persist-tun
    proto udp
    nobind
    user ovpn
    group ovpn
    remote-cert-tls server
    auth SHA512
    verb 3

    remote <服务器地址> 1194

    ca ca.crt
    cert client1.crt
    key client1.key
    tls-crypt ta.key
    ```

## Other

```bash
./server_kcptun -t "127.0.0.1:64022" -l ":61111" -mode fast3 -nocomp -sockbuf 16777217 -dscp 46
./client_kcptun -r "127.0.0.1:61110" -l ":7890" -mode fast3 -nocomp -autoexpire 900 -sockbuf 16777217 -dscp 46

./client_rsock -t 139.162.81.81 -p 62101-62110 --daemon=0 -l 127.0.0.1:61110
./server_rsock -t 127.0.0.1:61111 -p 62101-62110 --daemon=0

scp -P 7890 dam:/home/finger/download/Realtime_Rendering.pdf .

apt install -y command-not-found ping iputils-ping nslookup dnsutils traceroute procps openssh-server sudo openssh whois

apt install -y less lsof iftop tcpdump netcat-openbsd tmux mtr curl axel nethogs iptables ipset iputils-tracepath nmap
```
