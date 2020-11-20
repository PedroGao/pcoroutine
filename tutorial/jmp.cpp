#include <iostream>

// 为了防止 c++ 编译器将 run 函数的名字换掉，所以加上 extern
// run 是一个单参数，无返回值的函数，函数仅仅执行一条汇编函数，即 run
// run 定义在下面的内联汇编代码中
// 参数的类型是 void*
extern "C" void run(void *) asm("run");

// 这里使用了 c++11 的 R语法， *%rdi，是将 rdi 中的值当作内存地址进行
// 跳转，等价于 (%rdi)
// 注意，rdi 实际是第一个参数的值，即上面的 void*
asm(R"(
run:
    jmp *%rdi
)");

void func()
{
    std::cout << "Hello ASM" << std::endl;
}

int main()
{
    // 将 func 转换为函数指针传入 run 函数
    run((void *)func);
    return 0;
}
