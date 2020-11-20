swap_context:
  mov 0x00(%rsp), %rdx  ; 将返回地址保存到 RDX 寄存器
  lea 0x08(%rsp), %rcx  ; 将原 RSP 地址存储到 RCX 寄存器
  mov %r12, 0x00(%rdi)  ; 将当前协程的 R12 寄存器保存到 RDI 指向的 Registers 结构体中，下同
  mov %r13, 0x08(%rdi)
  mov %r14, 0x10(%rdi)
  mov %r15, 0x18(%rdi)
  mov %rdx, 0x20(%rdi)  ; 这里保存了当前协程的返回地址
  mov %rcx, 0x28(%rdi)  ; 这里保存了当前协程的栈顶地址
  mov %rbx, 0x30(%rdi)
  mov %rbp, 0x38(%rdi)  ; 这里保存了当前协程的栈帧地址
  mov 0x00(%rsi), %r12  ; 从 RSI 指向的 Registers 结构体中恢复 R12 寄存器的值，下同
  mov 0x08(%rsi), %r13
  mov 0x10(%rsi), %r14
  mov 0x18(%rsi), %r15
  mov 0x20(%rsi), %rax  ; 恢复返回地址，暂存到 RAX 寄存器
  mov 0x28(%rsi), %rcx  ; 恢复栈顶地址，暂存到 RCX 寄存器
  mov 0x30(%rsi), %rbx
  mov 0x38(%rsi), %rbp  ; 恢复栈帧地址
  mov %rcx, %rsp        ; 恢复栈顶地址
  jmpq *%rax            ; 跳回 RAX 保存的返回地址，完成切换