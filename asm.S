    .text
    .align 4
    .globl __fiber_switch
__fiber_switch:
    # save context on stack and set 2st arg %rsp
    push %r15
    push %r14
    push %r13
    push %r12
    push %rbp
    push %rbx
    xorq %rax, %rax
    push %rax
    push %rax
    test %rsi, %rsi
    jz 1f
    movq %rsp, (%rsi)
1:
    # restore context on stack with 1st arg
    movq %rdi, %rsp
    pop %rax
    pop %rdi
    pop %rbx
    pop %rbp
    pop %r12
    pop %r13
    pop %r14
    pop %r15
    ret