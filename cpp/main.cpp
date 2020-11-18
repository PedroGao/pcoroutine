#include <vector>

// 实现有栈协程，栈大小
#define DEFAULT_STACK_SIZE 1024 * 1024 * 2;
#define MAX_THREADS 4;

static int *RUNTIME = 0;

class Runtime
{
private:
    /* data */
public:
    Runtime(/* args */);
    ~Runtime();
};
