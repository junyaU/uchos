; System V AMD64 Calling Convention
; Registers(args): RDI, RSI, RDX, RCX, R8, R9
; Caller-saved registers: RAX, RDI, RSI, RDX, RCX, R8, R9, R10, R11
; Callee-saved registers: RBX, RBP, R12, R13, R14, R15

bits 64
section .text

extern handle_syscall
extern get_current_task_stack

global syscall_entry
syscall_entry:
    push rbp
    push rcx ; save RIP
    push r11 ; save RFLAGS
    push rax ; save syscall number

    mov rcx, r10 ; 4th argument
    mov r9, rax ; save syscall number in 6th argument

    mov rbp, rsp
    and rsp, 0xfffffffffffffff0 ; align stack to 16 bytes

    push rax
    push rdx

    call get_current_task_stack ; save kernel stack pointer to RAX

    mov rdx, [rsp + 0] ; restore RDX
    mov [rax - 16], rdx ; save RDX to kernel stack
    mov rdx, [rsp + 8] ; restore RAX
    mov [rax - 8], rdx ; save RAX to kernel stack

    lea rsp, [rax - 16] ; switch to kernel stack
    pop rdx ; restore RAX
    pop rax ; restore syscall number
    and rsp, 0xfffffffffffffff0 ; align stack to 16 bytes

    call handle_syscall

    mov rsp, rbp ; restore stack pointer

    pop rsi ; restore syscall number
    cmp esi, 60
    je .exit

    pop r11 ; restore RFLAGS
    pop rcx ; restore RIP
    pop rbp
    o64 sysret

.exit:
    mov rsp, rax ; restore kernel stack pointer
    mov eax, 0;

    ; restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ret
