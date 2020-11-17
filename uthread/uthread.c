#include <stdio.h>
#include <stdlib.h>
#include "uthread.h"

struct uthread
{
    // 0:ebp,1:esp,2:ebx,3:edi,4:esi
    uint32_t reg[5];        // 寄存器值
    uint32_t parent_reg[5]; // 父协程寄存器值
    struct uthread *parent; // 父协程
    uint32_t __exit;        // 退出状态
    void *stack;
    start_fun __start_fun;
    void *start_arg;
    uint32_t stack_size;
};

uthread_t uthread_create(void *stack, uint32_t stack_size)
{
    uthread_t u = calloc(1, sizeof(*u));      // 分配内存
    u->stack = stack;                         // 设置协程的栈
    u->stack_size = stack_size;               // 设置栈的大小
    u->reg[0] = (uint32_t)stack + stack_size; // 设置 ebp
    u->reg[4] = (uint32_t)stack + stack_size; // 设置 esi
    u->__exit = 0;                            // 退出状态为 0
    return u;
}

extern void uthread_run1(uthread_t u, start_fun st_fun, void *arg);
extern void uthread_run2(uthread_t p, uthread_t u, start_fun st_fun, void *arg);

void uthread_run(uthread_t p, uthread_t u, start_fun st_fun, void *arg)
{
    u->parent = p;
    if (u->parent) // 如果有父协程
    {
        uthread_run2(p, u, st_fun, arg); // 运行子协程后，switch 到父协程
        if (u->__exit)                   // 退出正常
        {
            uthread_switch(u, p);
        }
    }
    else
    {
        uthread_run1(u, st_fun, arg);
    }
}

void uthread_destroy(uthread_t *u)
{
    free(*u); // 释放内存
    *u = 0;
}