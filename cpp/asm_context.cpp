#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

struct ThreadContext
{
    uint64_t rsp = 0;
    uint64_t r15 = 0;
    uint64_t r14 = 0;
    uint64_t r13 = 0;
    uint64_t r12 = 0;
    uint64_t rbx = 0;
    uint64_t rbp = 0;
};

void t_switch(ThreadContext *old_t, ThreadContext *new_t)
{
    // 切换上下文
    asm volatile(
        // "movq     %%rsp, 0(%0)\n\t"
        "movq     %%r15, 8(%0)\n\t"
        "movq     %%r14, 16(%0)\n\t"
        // "movq     %%r13, 24(%0)\n\t"
        // "movq     %%r12, 32(%0)\n\t"
        // "movq     %%rbx, 40(%0)\n\t"
        // "movq     %%rbp, 48(%0)\n\t"

        // "movq     0(%1), %%rsp\n\t"
        "movq     8(%1), %%r15\n\t"
        "movq     16(%1), %%r14\n\t"
        // "movq     24(%1), %%r13\n\t"
        // "movq     32(%1), %%r12\n\t"
        // "movq     40(%1), %%rbx\n\t"
        // "movq     48(%1), %%rbp\n\t"
        // "ret\n\t"
        :
        : "r"(old_t), "r"(new_t)
        :);
}

int main(int argc, char const *argv[])
{
    ThreadContext c1;
    ThreadContext c2;
    // c1.rsp = 123;
    c2.r15 = 321;
    t_switch(&c1, &c2);
    printf("c1 r15: %ld\n", c1.r15);
    printf("c2 r15: %ld\n", c2.r15);
    return 0;
}
