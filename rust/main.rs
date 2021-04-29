#![feature(llvm_asm, naked_functions)]
#![allow(dead_code, unsupported_naked_functions)]

// 实现有栈协程，栈大小
const DEFAULT_STACK_SIZE: usize = 1024 * 2;
const MAX_THREADS: usize = 4;
static mut RUNTIME: usize = 0;

// 运行时
pub struct Runtime {
    coroutines: Vec<Coroutine>,
    current: usize, // 当前运行的协程
}

impl Runtime {
    pub fn new() -> Self {
        let base_id = 0;
        let r = Coroutine {
            id: base_id,
            stack: vec![0_u8; DEFAULT_STACK_SIZE],
            ctx: Context::default(),
            state: State::Running,
        };
        let mut coroutines = vec![r];
        let mut availables: Vec<Coroutine> = (1..MAX_THREADS).map(|i| Coroutine::new(i)).collect();
        coroutines.append(&mut availables);

        Runtime {
            coroutines,
            current: base_id,
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
            self.coroutines[self.current].state = State::Available;
            // 因为返回，所以让出使用权
            self.t_yield();
        }
    }

    // 让出执行权
    fn t_yield(&mut self) -> bool {
        let mut pos = self.current;
        // pos 是当前协程 id
        while self.coroutines[pos].state != State::Ready {
            pos += 1;
            // 如果后面 id 协程都没有 Ready，即都在运行和可用，没有准备号
            // 则调整到头部再找
            if pos == self.coroutines.len() {
                pos = 0;
            }
            // 如果找了一圈还没找到，则返回 false
            if pos == self.current {
                return false;
            }
        }
        // 如果当前的协程不是可用的装态，即在运行或者已准备好
        // 因为当前的协程会让出运行权，所以设置为 Ready
        if self.coroutines[self.current].state != State::Available {
            self.coroutines[self.current].state = State::Ready;
        }
        // pos 是下一个执行的协程
        // 因此设置 [pos] 的状态为 Running 
        self.coroutines[pos].state = State::Running;
        // 当前的 id 是旧的 id
        let old_pos = self.current;
        // 设置新的 current
        self.current = pos;
        // 切换上下文
        // 当切换上下文后，rip 的值发生了改变，因此会去执行另外一个协程栈中的函数
        unsafe {
            let old: *mut Context = &mut self.coroutines[old_pos].ctx;
            let new: *mut Context = &mut self.coroutines[pos].ctx;
            llvm_asm!("
                mov $0, %rdi
                mov $1, %rsi"
                :
                : "r"(old), "r"(new)
            ); // 将上下文地址赋给寄存器
            switch();
        }
        // 防止编译器过度优化
        self.coroutines.len() > 0
    }

    pub fn spawn(&mut self, f: fn()) {
        // 找到可用的协程
        let available = self
        .coroutines
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
            std::ptr::write(s_ptr.offset(-16) as *mut u64, guard as u64); // 指令 16 字节对齐
            std::ptr::write(s_ptr.offset(-24) as *mut u64, skip as u64);
            // 写入自定义函数，最先执行
            std::ptr::write(s_ptr.offset(-32) as *mut u64, f as u64);
            available.ctx.rsp = s_ptr.offset(-32) as u64; // 第一条指令必须写入 rsp
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

struct Coroutine {
    id: usize, // 协程 id
    stack: Vec<u8>, // 栈
    ctx: Context, // 上下文，寄存器数据
    state: State, // 状态
}

#[derive(Debug, Default)]
#[repr(C)] // 注意，rust 暂时没有稳定和汇编交互的内存布局，所以采用 C 语言内存布局
struct Context {
    rsp: u64, // 栈顶指针，内存的高地址
    r15: u64, 
    r14: u64, 
    r13: u64, 
    r12: u64, 
    rbx: u64, 
    rbp: u64, 
}

impl Coroutine {
    fn new(id: usize) -> Self {
        Coroutine {
            id,
            stack: vec![0_u8; DEFAULT_STACK_SIZE],
            ctx: Context::default(),
            state: State::Available,
        }
    }
}

// 保护函数，在协程栈的末尾，执行 return 操作，让出协程控制权
fn guard() {
    unsafe {
        let rt_ptr = RUNTIME as *mut Runtime;
        (*rt_ptr).t_return();
    };
}

// skip 函数，无实际作用
#[naked]
fn skip() {}

// 调度协程，寻找一个可用的协程运行
pub fn co_yield() {
    unsafe {
        // 全局 Runtime 指针
        let rt_ptr = RUNTIME as *mut Runtime;
        (*rt_ptr).t_yield();
    };
}

#[naked]
#[inline(never)]
unsafe fn switch() {
    llvm_asm!("
        mov     %rsp, 0x00(%rdi)
        mov     %r15, 0x08(%rdi)
        mov     %r14, 0x10(%rdi)
        mov     %r13, 0x18(%rdi)
        mov     %r12, 0x20(%rdi)
        mov     %rbx, 0x28(%rdi)
        mov     %rbp, 0x30(%rdi)

        mov     0x00(%rsi), %rsp
        mov     0x08(%rsi), %r15
        mov     0x10(%rsi), %r14
        mov     0x18(%rsi), %r13
        mov     0x20(%rsi), %r12
        mov     0x28(%rsi), %rbx
        mov     0x30(%rsi), %rbp
        ret
        "
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
            co_yield();
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
            co_yield();
        }
        println!("THREAD 2 FINISHED");
    });
    // 运行
    runtime.run();
}

