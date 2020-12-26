# Tips of misc

## Lxc 及容器

- 权限不足问题
  在新版本lxc的guest容器中,`journalctl -a`命令常常可以看到大量的systemd的失败警告信息. 在ssh连接到这些容器时,往往在登录阶段会卡住,表现为ssh会卡很久,sshd_config中设置`UseDNS=no`不能解决问题.`su`切换账户时也会很慢.日志显示systemd处理login时失败. 解决办法是在容器`config`文件中加入:
  `lxc.apparmor.profile = unconfined`

## END
