#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "uthread.h"

struct pair
{
    uthread_t self;
    uthread_t other;
};

void fun2(void *arg)
{
    int i = 0;
    struct pair *p = (struct pair *)arg;
    printf("fun2\n");
    uthread_switch(p->self, p->other);
    printf("func2 end\n");
}

void fun1(void *arg)
{
    int i = 0;
    struct pair *p = (struct pair *)arg;
    char *s = malloc(4096);
    uthread_t u2 = uthread_create(s, 4096);
    struct pair _p = {u2, p->self};
    uthread_run(p->self, u2, fun2, &_p);
    printf("here\n");
    uthread_switch(p->self, u2);
    printf("fun1 end\n");
}

int main(int argc, char const *argv[])
{
    char *s = malloc(4096);
    uthread_t u1 = uthread_create(s, 4096);
    struct pair p = {u1, 0};
    uthread_run(0, u1, fun1, &p);
    printf("return here\n");
    return 0;
}
