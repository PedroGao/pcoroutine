#include <ucontext.h>
#include <stdio.h>

void func1(void *arg)
{
    puts("1");
    puts("11");
    puts("111");
    puts("1111");
}

void context_test()
{
    char stack[1024 * 128];
    ucontext_t child, main;

    getcontext(&child);                     // 获得当前上下文
    child.uc_stack.ss_sp = stack;           // 指定栈空间
    child.uc_stack.ss_size = sizeof(stack); // 指定栈大小
    child.uc_stack.ss_flags = 0;
    child.uc_link = &main; // 设置后继上下文，调用主函数

    makecontext(&child, (void (*)(void))func1, 0); // 修改上下文指向 func1 函数

    swapcontext(&main, &child); // 切换到 child 上下文，保存当前上下文到 main
    puts("main");               // 如果设置了后继上下文，func1 函数完成后会返回此处
}

int main(int argc, char const *argv[])
{
    context_test();
    return 0;
}
