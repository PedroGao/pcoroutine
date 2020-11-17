#ifndef _UTHREAD_H
#define _UTHREAD_H

#include <stdint.h>

typedef void (*start_fun)(void *);
typedef struct uthread *uthread_t;

uthread_t uthread_create(void *stack, uint32_t stack_size);

void uthread_destroy(uthread_t *);

// 这里 p 是 u 的父协程，当 u 执行完毕后，会通过 uthread_switch 返回到 p
void uthread_run(uthread_t p, uthread_t u, start_fun st_fun, void *arg);

void uthread_switch(uthread_t from, uthread_t to);

#endif // _UTHREAD_H