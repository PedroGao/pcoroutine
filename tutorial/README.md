# 说明

此处有一个麻烦的点，直接用 `g++` 生成可执行文件会报错，但是先生成汇编文件，
然后从汇编文件生存执行文件，则运行正常。

``` sh
g++ -S -Og swap.cpp
g++ -c swap.s -o swap.o
g++ swap.o -o swap
```

或者直接：

``` sh
g++ -Og swap.cpp
```
