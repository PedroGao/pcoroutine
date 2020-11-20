#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <queue>
#include <vector>

extern "C" void swap_context(void *, void *) asm("swap_context");
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

struct Context
{
    void *reg[8];
    std::vector<char> mem;
    Context(void (*func)() = nullptr) : mem(4096)
    {
        // 执行函数，可以为空指针
        reg[4] = (void *)func;
        // 调用栈
        reg[5] = (char *)((uintptr_t)(&mem.back()) & ~15ull) - sizeof(void *);
    }
} ma;

Context *current = nullptr;

// 恢复协程的使用
void resume_coroutine(Context *coroutine)
{
    current = coroutine;
    swap_context(&ma, current);
}

// 获得时间戳
uint64_t GetMs()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_nsec / 1000000 + ts.tv_sec * 1000ull;
}

struct Task
{
    uint64_t expire;                                                          // 过期时间
    Context *coroutine;                                                       // 当前任务上的协程
    bool operator<(const Task &other) const { return expire > other.expire; } // 优先队列将 expire 最小的放到最前面
};

std::priority_queue<Task> tasks;

void coroutine_sleep(int ms)
{
    // 添加新的任务
    uint64_t expire = GetMs() + ms;
    tasks.push(Task{.expire = expire, .coroutine = current});
    // 把执行权让回主协程
    swap_context(current, &ma);
}

void event_loop(int timeout_in_seconds)
{
    uint64_t start = GetMs();
    while (true)
    {
        // 1000 毫秒检查一次
        usleep(1000);

        while (!tasks.empty())
        {
            // 如果有任务过期
            if (GetMs() > tasks.top().expire)
            {
                Task task = tasks.top();
                tasks.pop();
                // 取出任务并执行
                resume_coroutine(task.coroutine);
            }
            else
            {
                break;
            }
        }
        // 超过了时间则退出
        if ((GetMs() - start) > timeout_in_seconds * 1000)
        {
            break;
        }
    }
}

void func1()
{
    while (true)
    {
        std::cout << "Coroutine 1 print per 500ms" << std::endl;
        coroutine_sleep(500);
    }
}

void func2()
{
    while (true)
    {
        std::cout << "Coroutine 2 print per 1000ms" << std::endl;
        coroutine_sleep(1000);
    }
}

int main(int argc, char const *argv[])
{
    Context co1(func1);
    resume_coroutine(&co1);

    Context co2(func2);
    resume_coroutine(&co2);
    // 5 秒
    event_loop(5);
    return 0;
}
