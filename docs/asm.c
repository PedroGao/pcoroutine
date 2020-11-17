#include <stdio.h>

int main(int argc, char const *argv[])
{
    int a = 10, b;
    asm("movl %1, %%eax\n\t"
        "movl %%eax, %0\n\t"
        : "=r"(b) /* output */
        : "r"(a)  /* input */
        : "%eax"  /* clobbered register */
    );
    printf("a: %d, b: %d\n", a, b);
    return 0;
}
