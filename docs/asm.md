# GCC 内联汇编

## 概念

内联汇编可分为两个基础部分来解释，分别是 `内联` 和 `汇编` 。

* 内联

在 `c/c++` 中，我们可以通过 `inline` 关键字来实现函数的内联。函数内联是将函数跳转调用变成线性执行，即函数调用不再通过 `jmp` 来跳转，而是直接将代码内联到当前的函数，跳转执行变成了
顺序执行，从而提高程序性能。

* 汇编

`c/c++` 会被编译器编译为汇编代码，再由汇编器汇编成机器码，因此在代码中我们可以通过 `asm` 宏来插入必要的汇编代码， `c/c++` 虽然提供了足够底层的操作，但是仍有一些操作需要汇编来实现。

## GCC汇编

在 GCC 中使用 `asm` 必须是 `AT&T` 汇编语法（别问，问就是开源免费），AT&T 语法与 Intel 语法由比较大的差别，在《深入理解计算机系统》一书中，详细介绍了 Intel 语法。

二者的主要差别如下：

1. 源操作数和目的操作数的方向

AT&T 和 Intel 的操作数方向刚好相反，Intel 以第一个操作数作为目的操作数，第二个操作数作为源操作数。而 AT&T 相反，第一个操作数是源操作数，第二个操作数是目的操作数。如下：

``` 

OP-code src dst // AT&T 
OP-code dst src // Intel
```

> 老实说，个人觉得 AT&T 更加符合人的阅读习惯。

2. 寄存器命名

在 Intel 汇编中，寄存器的使用直接用名字即可，但是在 AT&T 中寄存器前面必须由 `%` 的前缀，如果使用 `eax` ，则必须写成 `%eax` 前缀。

3. 立即数

在 Intel 语法中对于立即数也是直接使用的，对于不同的进制需要加上不同的前缀或后缀。
但是在 AT&T 中，立即数需要以 `$` 开头，然后再加上相应的前缀或后缀，二者默认都是 10 进制。

4. 操作数大小

在 AT&T 中，操作符的最后一个字符决定操作内存的长度，如 `b` 表示字节，即 8 位。
而 Intel 语法对于操作内存的大小通过关键字来申明，如操作字节则加上 `byte ptr` 关键字。

例：

Intel 汇编：

``` s
mov al, byte ptr foo
```

AT&T 汇编：

``` s
movb foo, %al
```

5. 内存操作数

对于基址寄存器，Intel 使用方括号 `[]` ，而AT&T 使用小括号 `()` 。
对于一个间接内存寻址，Intel 是这样的：

``` s
section:[base + index * scale + disp]
```

而 AT&T 是这样的：

``` s
section:disp(base, index, scale)
```

注意：在 AT&T 如果一个常数被当作 disp 或 scale，即偏移量的时候，不需要 `$` 前缀。

一些例子对照如下：

| Intel Code  | AT&T Code |
| ----------- | --------- | 
|mov eax, 1 	| movl $1, %eax |
|mov ebx, 0ffh |	movl $0xff, %ebx |
|int 80h  | 	int $0x80 |
|mov ebx, eax |	movl %eax, %ebx |
|mov eax, [ecx] |	movl (%ecx), %eax |
|mov eax, [ebx+3] |	movl 3(%ebx), %eax |
|mov eax, [ebx+20h] | 	movl 0x20(%ebx), %eax |
|add eax, [ebx+ecx*2h] |	addl (%ebx, %ecx, 0x2), %eax |
|lea eax, [ebx+ecx] |	leal (%ebx, %ecx), %eax |
|sub eax, [ebx+ecx*4h-20h] |	subl -0x20(%ebx, %ecx, 0x4), %eax |

## 扩展内联汇编

内联汇编就是通过 `asm` 宏来写汇编，与基本的汇编编写几乎无异，而扩展汇编是 GCC 提供汇编
写法，让汇编更加好用。

在扩展形式中，可以指定操作数，并且可以选择输入输出寄存器，以及指明要修改的寄存器列表。
对于要访问的寄存器，并不一定要要显式指明，也可以留给GCC自己去选择，这可能让GCC更好去优化代码。

扩展内联汇编格式如下:

``` s
asm ( assembler template
        : output operands                /* optional */
        : input operands                   /* optional */
        : list of clobbered registers   /* optional */
);
```

assembler template 为汇编指令部分。括号内的操作数是 C 语言表达是中的常量字符串，
不同部分之间使用冒号分隔，相同部分语句中每个小部分用逗号分隔，最多可以指定 10 个操作数，
一些特殊平台可以使用超过 10 个操作数（不推荐，为了兼容）。

如果没有输出，但是有输入，我们需要保留前面的冒号。如：

``` s
asm ( "cld\n\t"
          "rep\n\t"
          "stosl"
         : /* no output registers */
         : "c" (count), "a" (fill_value), "D" (dest)
         : "%ecx", "%edi"
      );
```

例子：

``` c
int a = 10, b;
asm("movl %1, %%eax\n\t"
    "movl %%eax, %0\n\t"
    : "=r"(b) /* output */
    : "r"(a)  /* input */
    : "%eax"  /* clobbered register */
);
printf("a: %d, b: %d\n", a, b);
```

这个例子可以很好的说明扩展汇编是如何使用的，这段代码实现的功能就是将 `a` 的值赋给 `b` 。

* `b`是输出操作数，使用`%0`来在汇编代码中表示，`a`是输入操作数，使用`%1`来表示。
* `r` 是一个 constraint，意思是让 GCC 去选择一个寄存器来存储变量`a`。输出部分 constraint 前必须要有一个`=`来修饰，说明这是一个输出操作数，并且是只写的。
* 为了区分操作数和寄存器，因此操作数前用`%`来做前缀，而寄存器使用`%%`做前缀。
* 在 clobbered register 部分有一个`%eax`，意思是告诉编译器这段汇编会改变`eax`的值，这样 GCC 就可以在内联汇编的时候注意 eax 的状态。

这段代码结束后， `b` 就会被改写，输出如下：

``` 

a: 10, b: 10
```

## 汇编模板

汇编模板部分就是嵌入在 C 程序中的汇编指令，格式如下：

* 每天指令放在一个双括号内，或者将所有指令都放在一个双括号内。
* 每天指令都要包含一个分隔符。合法的分隔符都是换行符或者分号。用换行符的时候通过后面跟着一个指标符。
* 访问 C 语言变量用 %0，%1... 等等。

## constraints

关于 constraints 的内容有很多，这里先简单的罗列一下常用的：

* “m”: 使用一个内存操作数，内存地址可以是机器支持的范围内。
* “o”: 使用一个内存操作数，但是要求内存地址范围在在同一段内。例如，加上一个小的偏移量来形成一个可用的地址。
* “V”: 内存操作数，但是不在同一个段内。换句话说, 就是使用除了”o” 以外的”m”的所有的情况。
* “i”: 使用一个立即整数操作数(值固定)；也包含仅在编译时才能确定其值的符号常量。
* “n”: 一个确定值的立即数。很多系统不支持汇编常数操作数小于一个字(word)的长度的情况。这时候使用n就比使用i好。
* “g”: 除了通用寄存器以外的任何寄存器，内存和立即整数。

## 例子

### 例一

``` c
int main(int argc, char const *argv[])
{
    int foo = 10, bar = 15;
    asm volatile(
        "addl %%ebx, %%eax"
        : "=a"(foo)
        : "a"(foo), "b"(bar));
    printf("foo + bar = %d\n", foo);
    return 0;
}
```  

在上面代码中，我们强制让 GCC 将 `foo` 的值存储在 `%eax` 中，
将 `bar` 的值存储在 `%ebx` 中，并且让输出值放在`%eax`中。

其中`=`指明这是一个输出寄存器。我们再来看看另外一个把两个数相加的代码段:

```c
__asm__ __volatile__ (
"    lock        \n"
"    addl %1,%0; \n"
: "=m"    (my_var)
: "ir"    (my_int), "m" (my_var)
: /* no clobber-list */
); 
```

## 参考

* GCC 内联汇编基础： https://www.jianshu.com/p/1782e14a0766
* GNU extended asm：https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#:~:text=On%20targets%20such%20as%20x86%2C%20GCC%20supports%20multiple, default%20dialect%20if%20the%20option%20is%20not%20specified.
