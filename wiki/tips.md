# Tips

- find 命令处理含有空格的文件名
  - `-print0` 参数可以用‘\0'代替换行，xargs的参数`-0`可以接受以'\0'分隔的字符串，并正确传递。
  - Bash 提示符 `IFS`定义了分隔符，默认是空白字符，可以临时改变IFS的定义：

  ```bash
  (IFS=$'\n'; for i in $(find . -iname *.wav); do  sox $i $(dirname $i)/$(basename $i .wav).flac; done )
  ```

  注意，上述命令最外侧的括号是必要的，行内形式的IFS改变不行，因为展开时机太晚了。
  
- 去掉/tmp目录的noexec属性

  `mount -o remount,exec /tmp`

- LXC容器内使用sshfs
  lxc默认没有fuse设备，需要手动创建：

  `mknod -m 666 /dev/fuse c 10 229`  
  在lxc的config中也许可以指定创建，但是我觉得不应该采用bind选项, 这里只是想独立地mount到管理容器中，而不想干扰host机器。