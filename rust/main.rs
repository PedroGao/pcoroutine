#![feature(llvm_asm)]
#![feature(naked_functions)]
use std::ptr;

// 实现有栈协程，栈大小
const DEFAULT_STACK_SIZE: usize = 1024 * 1024 * 2;
const MAX_THREADS: usize = 4;
static mut RUNTIME: usize = 0;

// 运行时
pub struct Runtime {
    threads: Vec<Thread>,
    current: usize, // 当前运行的协程
}

impl Runtime {
    pub fn new() -> Self {
        let base_thread_id = 0;
        let base_thread = Thread {
            id: base_thread_id,
            stack: vec![0_u8; DEFAULT_STACK_SIZE],
            ctx: ThreadContext::default(),
            state: State::Running,
        };

        let mut threads = vec![base_thread];
        let mut available_threads: Vec<Thread> = (1..MAX_THREADS).map(|i| Thread::new(i)).collect();
        threads.append(&mut available_threads);

        Runtime {
            threads,
            current: base_thread_id,
        }
    }

    // 一定记得 init，将 runtime 挂载到全局
    pub fn init(&self) {
        unsafe {
            // 得到当前运行时指针
            let r_ptr: *const Runtime = self;
            // 将地址赋给全局遍历 RUNTIME
            RUNTIME = r_ptr as usize;
        }
    }

    pub fn run(&mut self) -> ! {
        while self.t_yield() {}
        std::process::exit(0);
    }

    pub fn t_return(&mut self) {
        // 当前协程返回
        if self.current != 0 {
            // 将当前协程设置为"可用"
            self.threads[self.current].state = State::Available;
            // 因为返回，所以让出使用权
            self.t_yield();
        }
    }

    // 让出执行权
    fn t_yield(&mut self) -> bool {
        let mut pos = self.current;
        // pos 是当前协程 id
        while self.threads[pos].state != State::Ready {
            pos += 1;
            // 如果后面 id 协程都没有 Ready，即都在运行和可用，没有准备号
            // 则调整到头部再找
            if pos == self.threads.len() {
                pos = 0;
            }
            // 如果找了一圈还没找到，则返回 false
            if pos == self.current {
                return false;
            }
        }
        // 如果当前的协程不是可用的装态，即在运行或者已准备好
        // 因为当前的协程会让出运行权，所以设置为 Ready
        if self.threads[self.current].state != State::Available {
            self.threads[self.current].state = State::Ready;
        }
        // pos 是下一个执行的协程
        // 因此设置 [pos] 的状态为 Running 
        self.threads[pos].state = State::Running;
        // 当前的 id 是旧的 id
        let old_pos = self.current;
        // 设置新的 current
        self.current = pos;
        // 切换上下文
        // 当切换上下文后，rip 的值发生了改变，因此会去执行另外一个协程栈中的函数
        unsafe {
            switch(&mut self.threads[old_pos].ctx, &self.threads[pos].ctx);
        }
        // 防止编译器过度优化
        self.threads.len() > 0
    }

    pub fn spawn(&mut self, f: fn()) {
        // 找到可用的协程
        let available = self
        .threads
        .iter_mut()
        .find(|t| t.state == State::Available)
        .expect("no available thread.");
        // 栈的大小
        let size = available.stack.len();
        unsafe {
            // 得到可用协程的栈顶
            let s_ptr = available.stack.as_mut_ptr().offset(size as isize);
            let s_ptr = (s_ptr as usize & !15) as *mut u8;
            // 向可用协程的栈里面写入函数指针
            std::ptr::write(s_ptr.offset(-16) as *mut u64, guard as u64);
            std::ptr::write(s_ptr.offset(-24) as *mut u64, skip as u64);
            // 写入自定义函数，最先执行
            std::ptr::write(s_ptr.offset(-32) as *mut u64, f as u64);
            available.ctx.rsp = s_ptr.offset(-32) as u64; // 栈顶竟然是可以移动的
        }
        available.state = State::Ready;
    }
}

#[derive(PartialEq, Eq, Debug)]
enum State {
    Available, // 可用，还未分为任务
    Running,
    Ready, // 可运行
}

struct Thread {
    id: usize, // 协程 id
    stack: Vec<u8>, // 栈
    ctx: ThreadContext, // 上下文，寄存器数据
    state: State, // 状态
}

#[derive(Debug, Default)]
#[repr(C)] // 注意，rust 暂时没有稳定和汇编交互的内存布局，所以采用 C 语言内存布局
struct ThreadContext {
    rsp: u64, // 栈顶指针，内存的高地址
    r15: u64, 
    r14: u64, 
    r13: u64, 
    r12: u64, 
    rbx: u64, 
    rbp: u64, 
}

impl Thread {
    fn new(id: usize) -> Self {
        Thread {
            id,
            stack: vec![0_u8; DEFAULT_STACK_SIZE],
            ctx: ThreadContext::default(),
            state: State::Available,
        }
    }
}

// 保护函数，在协程栈的末尾，执行 return 操作，让出协程控制权
fn guard() {
    println!("thread return");
    unsafe {
        let rt_ptr = RUNTIME as *mut Runtime;
        (*rt_ptr).t_return();
    };
}

// skip 函数，无实际作用
#[naked]
fn skip() {
    println!("skip");
}

// 调度协程，寻找一个可用的协程运行
pub fn yield_thread() {
    unsafe {
        // 全局 Runtime 指针
        let rt_ptr = RUNTIME as *mut Runtime;
        (*rt_ptr).t_yield();
    };
}

#[naked]
#[inline(never)]
unsafe fn switch(old: *mut ThreadContext, new: *const ThreadContext) {
    // 将当前寄存器的数据保存到 old 中
    // 将 new 中的值放到寄存器中
    // 注意前面的 0x00 是偏移量，而 old 和 new 是指针
    // 所以其实 switch 是很简单的，把寄存器换个数据就行了
    llvm_asm!("
        mov     %rsp, 0x00($0)
        mov     %r15, 0x08($0)
        mov     %r14, 0x10($0)
        mov     %r13, 0x18($0)
        mov     %r12, 0x20($0)
        mov     %rbx, 0x28($0)
        mov     %rbp, 0x30($0)

        mov     0x00($1), %rsp
        mov     0x08($1), %r15
        mov     0x10($1), %r14
        mov     0x18($1), %r13
        mov     0x20($1), %r12
        mov     0x28($1), %rbx
        mov     0x30($1), %rbp
        ret
        "
    :
    :"r"(old), "r"(new)
    :
    : "volatile", "alignstack"
    );
}

fn main() {
    // 新建运行时，并初始化
    let mut runtime = Runtime::new();
    runtime.init();
    // 加入第一个协程
    runtime.spawn(|| {
        println!("THREAD 1 STARTING");
        let id = 1;
        for i in 0..10 {
            println!("thread: {} counter: {}", id, i);
            // 每次输出一个后，让出执行权给 thread2
            yield_thread();
        }
        println!("THREAD 1 FINISHED");
    });
    // 加入第二个协程
    runtime.spawn(|| {
        println!("THREAD 2 STARTING");
        let id = 2;
        for i in 0..15 {
            println!("thread: {} counter: {}", id, i);
            // 每次输出一个后，让出执行权给 thread1
            yield_thread();
        }
        println!("THREAD 2 FINISHED");
    });
    // 运行
    runtime.run();
}

