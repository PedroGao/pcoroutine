#include <iostream>
#include <stdint.h>
#include <vector>

extern "C" void swap_context(void *, void *) asm("swap_context");

// 上下文的切换原理：
// rdi 是函数第一个参数，rsi 是函数第二个参数
asm(R"(
swap_context:
  mov 0x00(%rsp), %rdx
  lea 0x08(%rsp), %rcx
  mov %r12, 0x00(%rdi)
  mov %r13, 0x08(%rdi)
  mov %r14, 0x10(%rdi)
  mov %r15, 0x18(%rdi)
  mov %rdx, 0x20(%rdi)
  mov %rcx, 0x28(%rdi)
  mov %rbx, 0x30(%rdi)
  mov %rbp, 0x38(%rdi)
  mov 0x00(%rsi), %r12
  mov 0x08(%rsi), %r13
  mov 0x10(%rsi), %r14
  mov 0x18(%rsi), %r15
  mov 0x20(%rsi), %rax
  mov 0x28(%rsi), %rcx
  mov 0x30(%rsi), %rbx
  mov 0x38(%rsi), %rbp
  mov %rcx, %rsp
  jmpq *%rax
)");

// 寄存器数据，实际就是上下文
struct Registers
{
    void *reg[8];
} ma, co;

void func()
{
    std::cout << "Wait!" << std::endl;
    swap_context(&co, &ma); // yield
    std::cout << "Finish!" << std::endl;
    swap_context(&co, &ma); // finish
}

int main(int argc, char const *argv[])
{
    // 将栈的大小设置小一点，方便 debug，这里没有用到栈
    std::vector<char> mem(4096);
    // 这里会得到 mem 的尾地址
    // mem.back 会得到 mem 的尾元素的指针
    // & ～15ull 是为了内存对齐，具体为什么我也不知道

    uintptr_t back = (uintptr_t)(&mem.back());
    uintptr_t back_alighed = back & ~15ull;
    void *stack = (char *)(back_alighed) - sizeof(void *);

    // 序号 4 会赋给 rax 寄存器，然后 jmp rax 则会执行 func 这个函数
    co.reg[4] = (void *)func;
    // 序号 5 会赋值给 rcx 寄存器，然后 rcx 赋值给 rsp
    // rsp 是栈顶，所以 stack 其实就是协程的调用栈
    co.reg[5] = stack;
    // ma 是主协程，即当前的协程，co 是子协程
    swap_context(&ma, &co); // start co
    std::cout << "Resume!" << std::endl;
    swap_context(&ma, &co); // resume co
    return 0;
}
