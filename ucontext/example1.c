#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
    ucontext_t context;
    getcontext(&context); // 保存当前上下文
    puts("Hello World");
    sleep(1);
    setcontext(&context); // 此处的 setcontext 会恢复到 getcontext 处
    // 所以会不断地打印 Hello World
    return 0;
}
