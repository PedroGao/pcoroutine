System := $(shell uname)

all:
	nasm -f elf uthread/switch.s -o uthread/switch.o
	gcc -m32 uthread/test.c uthread/uthread.c uthread/switch.o -o demo1.out

asm:
	gcc -m32 -S -fno-stack-protector coroutine.c

clean:
	-rm -rf *.out*