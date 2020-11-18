#include <vector>
#include <stdint.h>
#include <stdlib.h>
#include <functional>
#include <stdio.h>

using namespace std;

// 实现有栈协程，栈大小
const size_t DEFAULT_STACK_SIZE = 1024 * 1024 * 2;
const size_t MAX_THREADS = 4;

// 全局 Runtime 指针
static void *RUNTIME = nullptr;

// 在这里地方提前声明
void guard();
void skip();

enum State
{
    Available,
    Running,
    Ready,
};

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

class Thread
{
public:
    size_t id;
    vector<uint8_t> stack{};
    ThreadContext ctx{};
    State state;

public:
    Thread(size_t tid) : id(tid), state(State::Available)
    {
        this->stack.resize(DEFAULT_STACK_SIZE);
    }
    Thread(size_t tid, vector<uint8_t> stack,
           ThreadContext ctx, State state)
    {
        this->id = tid;
        this->stack = stack;
        this->ctx = ctx;
        this->state = state;
    }
};

void t_switch(ThreadContext *old_t, ThreadContext *new_t)
{
    // 切换上下文
    asm volatile(
        "mov     %%rsp, 0(%0)\n\t"
        "mov     %%r15, 8(%0)\n\t"
        "mov     %%r14, 16(%0)\n\t"
        "mov     %%r13, 24(%0)\n\t"
        "mov     %%r12, 32(%0)\n\t"
        "mov     %%rbx, 40(%0)\n\t"
        "mov     %%rbp, 48(%0)\n\t"

        "mov     0(%1), %%rsp\n\t"
        "mov     8(%1), %%r15\n\t"
        "mov     16(%1), %%r14\n\t"
        "mov     24(%1), %%r13\n\t"
        "mov     32(%1), %%r12\n\t"
        "mov     40(%1), %%rbx\n\t"
        "mov     48(%1), %%rbp\n\t"
        "ret\n\t"
        :
        : "r"(old_t), "r"(new_t)
        :);
}

class Runtime
{
private:
    vector<Thread> threads;
    size_t current = 0;

public:
    Runtime()
    {
        size_t base_thread_id = 0;
        vector<uint8_t> stack(DEFAULT_STACK_SIZE, 0);
        ThreadContext ctx{};
        Thread base_thread(base_thread_id, stack, ctx, State::Running);
        vector<Thread> threads;
        threads.push_back(base_thread);
        for (size_t i = 1; i < MAX_THREADS; i++)
        {
            threads.push_back(Thread(i));
        }
        this->threads = threads;
        this->current = base_thread_id;
    }

    void init()
    {
        RUNTIME = this;
    }

    void run()
    {
        while (this->t_yield())
        {
        }
        exit(EXIT_SUCCESS);
    }

    void t_return()
    {
        if (this->current != 0)
        {
            this->threads[this->current].state = State::Available;
            this->t_yield();
        }
    }

    bool t_yield()
    {
        size_t pos = this->current;
        while (this->threads[pos].state != State::Running)
        {
            pos += 1;
            if (pos == this->threads.size())
            {
                pos = 0;
            }
            if (pos == this->current)
            {
                return false;
            }
        }
        if (this->threads[this->current].state != State::Available)
        {
            this->threads[this->current].state = State::Ready;
        }
        this->threads[pos].state = State::Running;
        size_t old_pos = this->current;
        this->current = pos;
        // 切换上下文
        ThreadContext *old_t = &(this->threads[old_pos].ctx);
        ThreadContext *new_t = &(this->threads[pos].ctx);
        // t_switch(old_t, new_t);
        // 切换上下文
        asm volatile(
            "mov     %%rsp, 0(%0)\n\t"
            "mov     %%r15, 8(%0)\n\t"
            "mov     %%r14, 16(%0)\n\t"
            "mov     %%r13, 24(%0)\n\t"
            "mov     %%r12, 32(%0)\n\t"
            "mov     %%rbx, 40(%0)\n\t"
            "mov     %%rbp, 48(%0)\n\t"

            "mov     0(%1), %%rsp\n\t"
            "mov     8(%1), %%r15\n\t"
            "mov     16(%1), %%r14\n\t"
            "mov     24(%1), %%r13\n\t"
            "mov     32(%1), %%r12\n\t"
            "mov     40(%1), %%rbx\n\t"
            "mov     48(%1), %%rbp\n\t"
            "ret\n\t"
            :
            : "r"(old_t), "r"(new_t)
            :);
        return this->threads.size() > 0;
    }

    void spawn(std::function<void()> &f)
    {
        Thread *available = nullptr;
        for (size_t i = 0; i < this->threads.size(); i++)
        {
            if (this->threads[i].state == State::Available)
            {
                available = &(this->threads[i]);
                break;
            }
        }
        if (available == nullptr)
        {
            throw runtime_error("no available thread.");
        }
        // size_t size = available->stack.size();
        uint64_t *s_ptr = (uint64_t *)&(available->stack);
        // s_ptr += (size / 64);
        // s_ptr++;

        void (*pguard)() = guard;
        void (*gskip)() = skip;
        // void (*gf)() = f;
        *s_ptr = (uint64_t)(pguard);
        s_ptr++;
        *s_ptr = (uint64_t)(gskip);
        // s_ptr++;
        // *s_ptr = (uint64_t)&f;
        available->ctx.rsp = (uint64_t)s_ptr;
        available->state = State::Ready;
    }
};

void guard()
{
    printf("thread return\n");
    // 在栈执行完毕前，调用 return，执行其它协程
    Runtime *rt_ptr = (Runtime *)RUNTIME;
    rt_ptr->t_return();
}

void skip()
{
    printf("skip\n");
}

void yield_thread()
{
    Runtime *rt_ptr = (Runtime *)RUNTIME;
    rt_ptr->t_yield();
}

int main(int argc, char const *argv[])
{
    Runtime runtime;
    runtime.init();
    // 加入第一个协程
    runtime.spawn([]() {
        printf("THREAD 1 STARTING\n");
        size_t id = 1;
        for (size_t i = 0; i < 10; i++)
        {
            printf("thread: %ld counter: %ld\n", id, i);
            // 每次输出一个后，让出执行权给 thread2
            yield_thread();
        }
        printf("THREAD 1 FINISHED\n");
    });
    // 加入第二个协程
    runtime.spawn([]() {
        printf("THREAD 2 STARTING\n");
        size_t id = 2;
        for (size_t i = 0; i < 20; i++)
        {
            printf("thread: %ld counter: %ld\n", id, i);
            // 每次输出一个后，让出执行权给 thread1
            yield_thread();
        }
        printf("THREAD 2 FINISHED\n");
    });
    // 运行
    runtime.run();
    return 0;
}
