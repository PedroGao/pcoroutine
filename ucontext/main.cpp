
#include "uthread.h"
#include <stdio.h>

void func2(void *arg)
{
    puts("22");
    puts("22");
    uthread_yield(*(schedule_t *)arg);
    puts("22");
    puts("22");
}

void func3(void *arg)
{
    puts("3333");
    puts("3333");
    uthread_yield(*(schedule_t *)arg);
    puts("3333");
    puts("3333");
}

void schedule_test()
{
    schedule_t s;

    int id1 = uthread_create(s, func3, &s);
    int id2 = uthread_create(s, func2, &s);

    while (!schedule_finished(s))
    {
        uthread_resume(s, id2);
        uthread_resume(s, id1);
    }
    puts("main over");
}
int main()
{
    schedule_test();

    return 0;
}