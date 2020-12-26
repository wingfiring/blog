# Tips of dev tools

## Git代理设置

```bash
git config --global https.proxy http://127.0.0.1:1080

git config --global https.proxy https://127.0.0.1:1080

git config --global --unset http.proxy

git config --global --unset https.proxy

git config --global http.proxy socks5://127.0.0.1:1080
git config --global https.proxy socks5://127.0.0.1:1080

npm config delete proxy
```

## Make

- `make -f makefile` 默认文件为`Makefile`和`makefile`, `-f`指定文件名,可以带路径
- 一般格式:

    ```makefile
    target [target ...]: [depends1] [depends2 ...]
    [tab] process commands
    ```

    上述`[tab]`必须是tab键,不能是空格. depends的术语应该是叫`prerequisites`.
    target可以是输出文件名,也可以是伪目标, 如:

    ```makefile
    .PHONY: clean
    clean:
        rm *.o
    ```

- makefile的每一行在一个单独的shell中执行.因此,上一行设置的局部环境没发被下一行获取.变通的方法之一是写成一行,另一个办法是行尾用`\`续行(等于还是一行).还有一个是`.ONESHELL`:

    ```makefile
    .ONESHELL:
    var-kept:
        export foo=bar;
        echo "foo=[$$foo]"
        #注意,对shell变量取值要写两个$符号,因为make会对$转义
    ```

- 命令前面加上`@`关闭命令的echo,和bat文件一样.
- 通配符`%`可以这样写:

    ```makefile
    %.o: %.c
        $(CC) -o $@ $<
    # 为所有.c文件生成.o目标
    # 注意,这里并不会扫描磁盘文件,而是当某条规则依赖这里的某个.o目标时,就会自动实例化一条这样的规则.
    ```

- 变量定义

    ```makefile
    txt = Hello World
    test:
        @echo $(txt)
    ```

- 变量赋值和扩展

    ```makefile
    VARIABLE = value
    # 在执行时扩展，允许递归扩展。

    VARIABLE := value
    # 在定义时扩展。

    VARIABLE ?= value
    # 只有在该变量为空时才设置值。

    VARIABLE += value
    # 将值追加到变量的尾端。
    ```

- 内建变量

  - Command

      |command|descrption|
      |:------|:---------|
      |AR|Archive-maintaining program; default ‘ar’.|
      |AS|Program for compiling assembly files; default ‘as’.|
      |CC|Program for compiling C programs; default ‘cc’.|
      |CXX|Program for compiling C++ programs; default ‘g++’.|
      |CPP|Program for running the C preprocessor, with results to standard output; default ‘$(CC) -E’.|
      |FC|Program for compiling or preprocessing Fortran and Ratfor programs; default ‘f77’.|
      |M2C|Program to use to compile Modula-2 source code; default ‘m2c’.|
      |PC|Program for compiling Pascal programs; default ‘pc’.|
      |CO|Program for extracting a file from RCS; default ‘co’.|
      |GET|Program for extracting a file from SCCS; default ‘get’.|
      |LEX|Program to use to turn Lex grammars into source code; default ‘lex’.|
      |YACC|Program to use to turn Yacc grammars into source code; default ‘yacc’.|
      |LINT|Program to use to run lint on source code; default ‘lint’.|
      |MAKEINFO|Program to convert a Texinfo source file into an Info file; default ‘makeinfo’.|
      |TEX|Program to make TeX DVI files from TeX source; default ‘tex’.|
      |TEXI2DVI|Program to make TeX DVI files from Texinfo source; default ‘texi2dvi’.|
      |WEAVE|Program to translate Web into TeX; default ‘weave’.|
      |CWEAVE|Program to translate C Web into TeX; default ‘cweave’.|
      |TANGLE|Program to translate Web into Pascal; default ‘tangle’.|
      |CTANGLE|Program to translate C Web into C; default ‘ctangle’.|
      |RM|Command to remove a file; default ‘rm -f’.|

  - Flags
      |flag|descrption|
      |:------|:---------|
      |ARFLAGS|Flags to give the archive-maintaining program; default ‘rv’.|
      |ASFLAGS|Extra flags to give to the assembler (when explicitly invoked on a ‘.s’ or ‘.S’ file).|
      |CFLAGS|Extra flags to give to the C compiler.|
      |CXXFLAGS|Extra flags to give to the C++ compiler.|
      |COFLAGS|Extra flags to give to the RCS co program.|
      |CPPFLAGS|Extra flags to give to the C preprocessor and programs that use it (the C and Fortran compilers).|
      |FFLAGS|Extra flags to give to the Fortran compiler.|
      |GFLAGS|Extra flags to give to the SCCS get program.|
      |LDFLAGS|Extra flags to give to compilers when they are supposed to invoke the linker, ‘ld’, such as -L. Libraries (-lfoo) should be added to the LDLIBS variable instead.|
      |LDLIBS|Library flags or names given to compilers when they are supposed to invoke the linker, ‘ld’. LOADLIBES is a deprecated (but still supported) alternative to LDLIBS. Non-library linker flags, such as -L, should go in the LDFLAGS variable.|
      |LFLAGS|Extra flags to give to Lex.|
      |YFLAGS|Extra flags to give to Yacc.|
      |PFLAGS|Extra flags to give to the Pascal compiler.|
      |RFLAGS|Extra flags to give to the Fortran compiler for Ratfor programs.|
      |LINTFLAGS|Extra flags to give to lint.|

- 自动变量
  - `$@` 指代当前目标

      ```makefile
      %.o: %.c
        $(CC) -o $@ $<
        #注意,这里的 $< 是某一次实例化规则中的第一个前置项,不是所有实例化中的第一个,别搞混了.
      ```

  - `@<` 指代第一个依赖(前置)条件
  - `@?` 指代比目标更新的所有前置条件
  - `$^` 指代所有前置条件,之间以空格分隔
  - `$(@D)` 和 `$(@F)` 分别指向 `$@`的目录名和文件名. 如`src/a.cpp`分解为 `src`和`a.cpp`
  - `$(<D)` 和 `$(<F)` 类似上条, 用于第一个前置条件

- 分支和循环

    ```makefile
    ifeq ($(CC),gcc)
        libs=$(libs_for_gcc)
    else
        libs=$(normal_libs)
    endif

    LIST = one two three
    all:
        for i in $(LIST); do \
            echo $$i; \
        done

    # 等同于

    all:
        for i in one two three; do \
            echo $i; \
        done
    ```

- 函数,格式如下

    ```makefile
    $(function arguments)
    #或者
    ${function arguments}

    # shell 函数
    srcfiles := $(shell echo src/{00..99}.txt)

    #wildcard 函数
    srcfiles := $(wildcard src/*.txt)

    #subst 函数
    $(subst from,to,text)
    $(subst ee,EE,feet on the street) #output: fEEt on the strEEt

    #patsubst 函数
    $(patsubst pattern,replacement,text)
    $(patsubst %.c,%.o,x.c.c bar.c) #output:x.c.o bar.o

    min: $(OUTPUT:.js=.min.js) # 变量OUTPUT中所有后缀名替换


    ```

## End
