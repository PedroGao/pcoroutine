#![feature(llvm_asm)]

const SSIZE: isize = 100; // 100 字节

#[derive(Debug, Default)]
#[repr(C)]
struct Context {
    rsp: u64,
}

fn flash() {
    println!("Flash!");
}

fn hello() -> ! {
    println!("I LOVE WAKING UP ON A NEW STACK!");

    loop {}
}

unsafe fn gt_switch(new: *const Context) {
    llvm_asm!("
        mov 0x00($0), %rsp
        ret
        "
    :
    : "r"(new)
    :
    : "alignstack"
    );
}

fn main() {
    let mut ctx = Context::default();
    let mut stack = vec![0_u8; SSIZE as usize];

    unsafe {
        let stack_ptr = stack.as_mut_ptr();
        let stack_bottom = stack_ptr.offset(SSIZE);
        println!("sp: {}", stack_bottom as u64);
        let sb_aligned = (stack_bottom as usize & !15) as *mut u8; // 16字节对齐
        println!("sp aligned: {}", sb_aligned as u64);
        std::ptr::write(sb_aligned.offset(-16) as *mut u64, hello as u64);
        std::ptr::write(sb_aligned.offset(-24) as *mut u64, flash as u64); // 第一个 16 字节对齐，后面的不需要
        for i in (0..SSIZE).rev() { // 小端序
            println!("mem: {}, val: {}", 
            stack_ptr.offset(i as isize) as usize,
            *stack_ptr.offset(i as isize))
        }
        ctx.rsp = sb_aligned.offset(-24) as u64;
        gt_switch(&mut ctx);
    }
}