# 自己实现协程

> 学习如何实现一个用户态线程——协程

## 概述

协程的实现方式大致由如下三种：

1. 利用 glibc ucontext 库来实现协程，借助 ucontext 实现简单，但无法理解协程真正的运作原理。

2. 使用汇编代码来切换上下文，保存上下文信息，汇编代码来实现切换，重点。

3. 利用 setjmp 和 longjmp 来实现，了解即可。

## 实现

rust 抽象能力好，易阅读和理解，见[rust协程](./rust/main.rs)；在线运行可使用：https://play.rust-lang.org/?version=nightly&mode=debug&edition=2018。

## 文档

* [asm 内联汇编使用](./docs/asm.md)

## 资料

* ASM: https://www.jianshu.com/p/57fef17149ae
* 嵌入汇编：https://cloud.tencent.com/developer/article/1434865。
* GCC内联汇编：https://www.jianshu.com/p/1782e14a0766
* ucontext 协程：https://www.jianshu.com/p/a96b31da3ab0；https://blog.csdn.net/qq910894904/article/details/41911175
* 有栈和无栈协程之间的区别：https://blog.csdn.net/weixin_43705457/article/details/106924435。
* 黑科技无栈协程的实现：https://mthli.xyz/coroutines-in-c/。
