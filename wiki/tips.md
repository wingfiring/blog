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

- enca修改文本文件内容编码
  - 检测编码：`enca -L zh_CN file`
  - 转换编码: `enca -L zh_CN -x UTF-8 file`
  - `iconv -f GB2312 -t UTF-8` 可以强行转，但是如果遇到输入的编码有错误则会中止。
  - convmv 用于转换文件名

- 转换音频文件格式和tag编码

  如从ape转换成flac. soxi可用来检测文件tags，要检测mp3，可以`apt install libsox-fmt-mp3`

  soxi在显示文本信息时，总是强行做了一次UTF-8，可以用`id3v2 -l`查看真实信息。id3v2有时获取不到所有tag，exiftool更好。
  
  `mid3iconv -e GB2312` 可以将tag内容从GB2312编码转成UTF-8.还可以通过参数`-d`查看所转的内容。
