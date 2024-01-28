; System V AMD64 Calling Convention
; Registers(args): RDI, RSI, RDX, RCX, R8, R9
; Caller-saved registers: RAX, RDI, RSI, RDX, RCX, R8, R9, R10, R11
; Callee-saved registers: RBX, RBP, R12, R13, R14, R15

bits 64
section .text

extern handle_syscall

global syscall_entry
syscall_entry:
    push rbp
    push rcx ; save RIP
    push r11 ; save RFLAGS

    mov rcx, r10 ; 4th argument
    mov r9, rax ; save syscall number in 6th argument

    mov rbp, rsp
    and rsp, 0xfffffffffffffff0 ; align stack to 16 bytes

    call handle_syscall

    mov rsp, rbp ; restore stack pointer

    pop r11 ; restore RFLAGS
    pop rcx ; restore RIP
    pop rbp
    o64 sysret
