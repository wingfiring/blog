# Tips of text process

## Text process

1. Grep提取IP地址

    ```bash
    grep -oE "\b([0-9]{1,3}\.){3}[0-9]{1,3}\b" dnsmasq.log
    ```

    `-o`是仅显示匹配部分
    从dnsmasq日中中提取域名查询结果:

    ```bash
    grep -oE "reply .* is \b([0-9]{1,3}\.){3}[0-9]{1,3}\b$" dnsmasq.log | awk '{print $2, $4}' |sort|uniq
    ```

2. Linux常用工具

    ```bash
    apt install -y command-not-found ping iputils-ping nslookup dnsutils traceroute procps openssh-server sudo openssh whois

    apt install -y less lsof iftop tcpdump netcat-openbsd tmux mtr curl axel nethogs iptables ipset iputils-tracepath nmap
    ```

3. The End
