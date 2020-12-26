# NFS 配置

参考 <https://linuxconfig.org/how-to-set-up-a-nfs-server-on-debian-10-buster>
<https://www.howtoforge.com/tutorial/install-nfs-server-and-client-on-debian/>

man 5 exports

## 软件准备

```bash
# server
sudo apt install nfs-kernel-server
#client
apt-get install nfs-common
```

