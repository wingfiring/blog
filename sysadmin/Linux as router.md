# Linux路由配置笔记

---

## 硬件准备

- CPU: [i3-8300T, TDP 35W](https://www.intel.cn/content/www/cn/zh/products/processors/core/i3-processors/i3-8300t.html)
- Memory: [海盗船 DDR4 2440, 8Gx2](https://item.jd.com/2939894.html)
- Motherboard: [华擎 Z370M-ITX/ac, Intel Z370/LGA 1151](https://item.jd.com/5361693.html) [官网链接](http://www.asrock.com.tw/MB/Intel/Z370M-ITXac/index.cn.asp)
  - 关注特性：
    - 双千兆网卡
    - 802.11ac wifi
    - 蓝牙 4.2
  - 其他特性：2HDMI，1DisplayPort，三显示器同时输出
- Storage: [Intel 760P 512G](https://item.jd.com/12924238452.html), 标称性能：read 3230MB/s, write: 1625MB/s 4k read: 340000 IOPS, 4k write: 275000 IOPS
- 电源: [银欣 300W ST30SF SFX电源 80PLUS铜牌](https://item.jd.com/3945819.html)
- 机箱: [银欣（SG13B 珍宝13 SFF机箱](https://item.jd.com/100000111585.html)
- 风扇: [酷冷至尊T520下压式CPU散热器](https://item.jd.com/27824352454.html)

## 操作系统

Debian stretch (9.8)。安装完成后，apt source添加了Buster和sid。

安装过程中用了wifi联网，提示需要3168-26.ucode，到另一台机器用`apt download firmware-iwlwifi` deb包下来，然后找个u盘，放在u盘的firmware目录下，继续安装。

考虑到会重度使用lxc，btrfs对此有比较好的支持，因此建了一个lvm卷挂到/var/lib/lxc上。

## 系统配置

### 网络配置

#### 主机网络配置

这块板子有两个千兆口，一个是I219-V, 另一个是I211 AT. 一个无线网卡，3168NGW。I219-V芯片官网报价$1.72，够廉价的，用来连wan合适。另一块I211算是小惊喜，除了支持4个收发队列外，还支持巨帧和VLAN，官网报价$2.13。3168NGW官网价格$5.00，比俩千兆口加起来都贵。

物理机器上建两个bridge，brlan和brwan，分别用于桥接局域网和广域网。I219接在brwan上，I211和wifi接在brlan上。开一个lxc容器作为路由，有两块网卡，分跨两个bridge，作为路由。配置阶段为了方便，暂时给brwan分配了一个地址，等配置完就会去掉。物理host机器将来也通过路由容器访问外网，这样更安全。

```config.interfaces

auto lo  brlan brwan eth0 eth1 wlan0
iface lo inet loopback

allow-hotplug eth0 eth1

iface brlan inet static
    bridge_ports eth0 wlan0
    bridge_stp off
    address 192.168.1.10
    netmask 255.255.255.0
    gateway 192.168.1.1


iface brwan inet static
    bridge_ports eth1
    bridge_stp off

```

`brlan`上静态分配内网地址是必要的，因为路由容器启动会比主机晚，也方便容器出问题时直接通过ip地址访问主机。`brwan`上刻意不分配地址，禁止外网直接访问主机，主机访问外网也需要从`brlan`出发，经路由容器NAT，再经`brwan`出去。这样更安全些。为了方便远程配置网络，也可以在配置阶段先分配一个内网地址，等配置完成再去掉。

主机上也不要配置路由，路由由容器充当。安全防御做在路由容器上，这样物理主机的防火墙似乎不需要了，毕竟连ip地址没有，路由也没有。这么做是否安全，我也没把握，希望有专家指正。

理想状况下，主机也不需要配置wlan0，而是交给路由容器去配置。但是不知道怎么把wifi网卡这种设备转给容器访问，留待以后有机会再尝试吧。所以目前的做法是在主机跑hostapd，提供ap，dhcp和dns则由路由容器提供。

#### 创建Lxc容器

创建两个lxc容器，一个用于路由，另一个用于测试。

```bash

lxc-create -t download -n route -- -d debian -r sid  -a amd64
lxc-copy -N test -n route
```

先编辑容器route的配置，/var/lib/lxc/route/config：

```config.lxc

#eth0
lxc.net.0.type = veth
lxc.net.0.flags = up
lxc.net.0.link = brwan
#eth1
lxc.net.1.type = veth
lxc.net.1.flags = up
lxc.net.1.link = brlan

lxc.include = /usr/share/lxc/config/debian.common.conf

lxc.tty.max = 4
lxc.arch = amd64
lxc.start.auto = 1     #自动启动
lxc.uts.name = route

### for openvpn
lxc.mount.entry = /dev/net dev/net none bind,create=dir
lxc.cgroup.devices.allow = c 10:200 rwm
lxc.rootfs.path = btrfs:/var/lib/lxc/route/rootfs

```

test的配置，关键部分：

```config.lxc
lxc.net.0.type = veth
lxc.net.0.flags = up
lxc.net.0.link = brlan
```

新版本的lxc生成的config文件中会有这样两句：

```config.lxc

lxc.apparmor.profile = generated
lxc.apparmor.allow_nesting = 1
```

在config文件所在目录还会生成一个apparmor目录，其中是apparmor的相关配置。我的route其实是以前配置好的，没有apparmor目录内的内容，导致dmesg信息中被一大堆apparmor的错误信息洗版了，什么有用信息都看不到了。修复方法就是复制一份好的过来，修改容器名称相关的部分就好。

#### 路由配置

##### 基础网络配置

`lxc-attach route`进入路由容器，先开启路由功能

```bash

echo "1" > /proc/sys/net/ipv4/ip_forward
```

编辑 `/etc/sysctl.d/10-network.conf`,添加：

```config.sysctl

net.ipv4.ip_forward=1

net.core.default_qdisc=fq
net.ipv4.tcp_congestion_control=bbr
```

后两行是设置Tcp拥塞控制算法为BBR，不是必须的。

配置/etc/network/interfaces：

```config.interfaces

auto lo eth0 eth1
iface lo inet loopback

iface eth0 inet dhcp

iface eth1 inet static
    address 192.168.1.1
    netmask 255.255.255.0
```

`eth0`接外网，`eth1`接内网。因为自己是路由器，`eth1`上就不要指定`gateway`了。重启网卡：

```bash

ifdown eth0
ifdown eth1
ifup eth0
ifup eth1
```

接着开启NAT,其实一句就可以：`iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE`

重启路由后要能自动开启NAT，因此创建文件 /etc/network/if-up.d/iptables

```bash

#! /bin/sh

iptables -F
iptables -t nat -F
iptables -t mangle -F

iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
```

前三句是清空iptables的规则表。保存好文件后别忘了`chmod +x iptables`

##### unbound配置

这个就是用来反域名污染的。配置如下：

```config.unbound

server:
        do-daemonize: no
    interface: 127.0.0.1@1053
    tcp-upstream: yes
    tls-cert-bundle: /etc/ssl/certs/ca-certificates.crt

forward-zone:
    name: "."
    forward-tls-upstream: yes
    forward-addr: 1.1.1.1@853#cloudflare-dns.com
```

一般的dns查询交给dnsmasq来完成，受污染的域名就交给unbound，走DNS-over-TLS

##### Dnsmasq配置

首先安装dnsmasq: `apt install -y dnsmasq`

先修改`/etc/resolv.conf`

```config.resolv

search lan
nameserver 127.0.0.1
```

让域名解析指向本机，这里的`lan`是你的本地域名后缀。你不要search这一行也可以，dnsmasq相应的不配就行了。

创建文件`/etc/hosts.dnsmasq`如下：

```config.hosts

192.168.1.1 route route.lan
192.168.1.10 bare bare.lan
```

这个是添加路由和主机的域名到dnsmasq，因为这俩都不是通过dhcp分配地址的。

不要动`/etc/dnsmasq.conf`，这个文件可以当作文档用，别改乱了。实际的配置可以放在`/etc/dnsmasq.d/default.conf`中：

```config.dnsmasq

interface=eth1 #内网网关接口
dhcp-range=192.168.1.50,192.168.1.200,255.255.255.0,1h  #地址池范围
no-hosts                 #忽略/etc/hosts文件
addn-hosts=/etc/hosts.dnsmasq     #添加静态主机名
domain=lan                 #指定本地域名

cache-size=1500
except-interface=eth0  # 排除wan口

all-servers
server=8.8.8.8
server=114.114.114.114

dhcp-option=121,0.0.0.0/0,192.168.1.1
```

我这里的server用的指定的dns，如果wan口的eth0是通过dhcp获得ip的，而你又想使用isp提供的dns服务器，那就需要编辑dhclient的配置文件，加入一行：

```config.dnsmasq

prepend domain-name-servers 127.0.0.1;
```

完了需要定制一下dnsmasq吧？在中国大陆没办法的事。参考`https://github.com/felixonmars/dnsmasq-china-list`。

我在上面还跑了openvpn，这里就不多说了。

#### AP配置

在物理主机上安装hostapd,配置如下：

```config.hostapd

interface=wlp3s0
driver=nl80211
bridge=brlan
ssid=lostemple
hw_mode=g
#country_code=CN
channel=1
ieee80211n=1
ieee80211ac=1

auth_algs=1
wpa=2
wpa_key_mgmt=WPA-PSK
rsn_pairwise=CCMP
wpa_passphrase=GreatWall
```

这样AP能起来。我本来觉得差不多就可以了，但是性能实在太差了。不但速度慢，信号也差，远一点就衰减得厉害。所以要加入下面的配置：

```config.hostapd

ht_capab=[HT40+][DSSS_CCK-40][RX-STBC1][SHORT-GI-40][SHORT-GI-20]

wmm_enabled=1
wmm_ac_bk_cwmin=4
wmm_ac_bk_cwmax=10
wmm_ac_bk_aifs=7
wmm_ac_bk_txop_limit=0
wmm_ac_bk_acm=0

wmm_ac_be_aifs=3
wmm_ac_be_cwmin=4
wmm_ac_be_cwmax=10
wmm_ac_be_txop_limit=0
wmm_ac_be_acm=0

wmm_ac_vi_aifs=2
wmm_ac_vi_cwmin=3
wmm_ac_vi_cwmax=4
wmm_ac_vi_txop_limit=94
wmm_ac_vi_acm=0

wmm_ac_vo_aifs=2
wmm_ac_vo_cwmin=2
wmm_ac_vo_cwmax=3
wmm_ac_vo_txop_limit=47
wmm_ac_vo_acm=0
```

重点是ht_capab那一行。方括号内的都是参数，要参考hostapd.conf的文档，看ht_capab支持哪些，还要看`iw list`的输出，看网卡实际支持哪些功能，把网卡支持的给加进去。

至于`wmm_enabled`,只是enable一下是不够的，后面那一堆wmm_XXX 参数必须都设置才能真正使得WMM生效。这些参数都是从hostapd.conf的建议值抄来的，具体怎么工作的我还没搞明白，不太想搞这个。

反正，这么操作一下，这个软AP的性能终于赶上原来的无线路由了，性能和稳定性更好。但糟心的是信号传输距离表现不如旧无线路由，看来还是得买ap。

##### 关于5G

为了开AP 5G频段，我至少花了20个小时吧，远超其他所有任务的时间总和加上写这篇笔记。悲剧的是没有搞定无线电监管的问题。最终在Intel 官网上找到官方说明，明确了就是不支持5G的AP模式，放弃了。尝试了很多次，对其他网卡有效的办法，对Intel的网卡就是无效。系统监管地区可以修改，但是无论怎么改，都不能影响网卡上的设置，我怀疑关键是卡在网卡的Firmware上。有两篇特别提到Intel网卡的文章，一个是说是有办法绕过监管，试了，行不通。另一个是一个俄国人问iwlwifi的开发者，说是旧FW曾经是可以的，为什么新的FW不行了。iwlwifi的那位表示很惊讶居然有旧FW能行，然后一再强调别这么干，有法律风险。之前还看到一篇说是修改驱动可以，我很怀疑。中途还打算试试看改FW，看了几眼iwlwifi的源代码和签名问题，放弃了。

以后很长一段时间大概都没有胃口去碰wifi网卡了......

#### 蓝牙

发现自己对蓝牙的应用方面的概念其实了解的很少，需要系统地补补课。bluetoothctl很强大，但我不太会用，虽然用它成功地和我的手机配对了。文档推荐的应用工具blueman是个图形界面工具，我不方便用。配置过程中，唯一值得一记的就是安装bluetoothd要修改一下systemd的配置文件`/etc/systemd/system/bluetooth.target.wants/bluetooth.service`, 禁用sap，修改如下：

```config.systemd

ExecStart=/usr/lib/bluetooth/bluetoothd --noplugin=sap
```

### 问题处理

1. 想开unprivileged container,遇到好几个问题。一方面我升级到了最新的lxc 3.0.3,但是网上的文档都是老的，网站linuxcontainer的是新的，但是很多细节没交代清楚，又和以前不一样。`id_map`换成`idmap`这种还算好找的。apparmor的一些错误一番搜索之后修正了。
