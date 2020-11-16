System := $(shell uname)

all:
	gcc -m32 demo/demo1.c simple/coroutine.c -o demo1.out

asm:
	gcc -m32 -S -fno-stack-protector coroutine.c

clean:
	-rm -rf *.out*