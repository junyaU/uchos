; System V AMD64 Calling Convention
; Registers(args): RDI, RSI, RDX, RCX, R8, R9
; Caller-saved registers: RAX, RDI, RSI, RDX, RCX, R8, R9, R10, R11
; Callee-saved registers: RBX, RBP, R12, R13, R14, R15

bits 64
section .text

global ExecuteContextSwitch ; rdi = new context, rsi = current context
ExecuteContextSwitch:
    ; save current context
    mov [rsi + 0x40], rax
    mov [rsi + 0x48], rbx
    mov [rsi + 0x50], rcx
    mov [rsi + 0x58], rdx
    mov [rsi + 0x60], rdi
    mov [rsi + 0x68], rsi

    lea rax, [rsp + 8] ; save stack pointer
    mov [rsi + 0x70], rax
    mov [rsi + 0x78], rbp

    mov [rsi + 0x80], r8
    mov [rsi + 0x88], r9
    mov [rsi + 0x90], r10
    mov [rsi + 0x98], r11
    mov [rsi + 0xA0], r12
    mov [rsi + 0xA8], r13
    mov [rsi + 0xB0], r14
    mov [rsi + 0xB8], r15
    
    mov rax, cr3
    mov [rsi + 0x00], rax

    mov rax, [rsp] ; save RIP
    mov [rsi + 0x08], rax

    pushfq
    pop qword [rsi + 0x10] ; save RFLAGS

    mov ax, cs
    mov [rsi + 0x20], rax
    mov bx, ss
    mov [rsi + 0x28], rbx
    mov cx, fs
    mov [rsi + 0x30], rcx
    mov dx, gs
    mov [rsi + 0x38], rdx

    fxsave [rsi + 0xC0] ; save FPU state

    ; load new context
    ; iret will pop RIP, CS, RFLAGS, RSP, SS
    push qword [rdi + 0x28] ; SS
    push qword [rdi + 0x70] ; RSP
    push qword [rdi + 0x10] ; RFLAGS
    push qword [rdi + 0x20] ; CS
    push qword [rdi + 0x08] ; RIP

    fxrstor [rdi + 0xC0] ; load FPU state

    mov rax, [rdi + 0x00]
    mov cr3, rax
    mov rax, [rdi + 0x30]
    mov fs, ax
    mov rax, [rdi + 0x38]
    mov gs, ax

    mov rax, [rdi + 0x40]
    mov rbx, [rdi + 0x48]
    mov rcx, [rdi + 0x50]
    mov rdx, [rdi + 0x58]
    mov rsi, [rdi + 0x68]
    mov rbp, [rdi + 0x78]
    
    mov r8, [rdi + 0x80]
    mov r9, [rdi + 0x88]
    mov r10, [rdi + 0x90]
    mov r11, [rdi + 0x98]
    mov r12, [rdi + 0xA0]
    mov r13, [rdi + 0xA8]
    mov r14, [rdi + 0xB0]
    mov r15, [rdi + 0xB8]

    mov rdi, [rdi + 0x60]

    o64 iret

global restore_context ; void restore_context(void* task_context); rdi = task_context
restore_context:
    ; iret will pop RIP, CS, RFLAGS, RSP, SS
    push qword [rdi + 0x28] ; SS
    push qword [rdi + 0x70] ; RSP
    push qword [rdi + 0x10] ; RFLAGS
    push qword [rdi + 0x20] ; CS
    push qword [rdi + 0x08] ; RIP

    fxrstor [rdi + 0xC0] ; load FPU state

    mov rax, [rdi + 0x00]
    mov cr3, rax
    mov rax, [rdi + 0x30]
    mov fs, ax
    mov rax, [rdi + 0x38]
    mov gs, ax

    mov rax, [rdi + 0x40]
    mov rbx, [rdi + 0x48]
    mov rcx, [rdi + 0x50]
    mov rdx, [rdi + 0x58]
    mov rsi, [rdi + 0x68]
    mov rbp, [rdi + 0x78]
    mov r8,  [rdi + 0x80]
    mov r9,  [rdi + 0x88]
    mov r10, [rdi + 0x90]
    mov r11, [rdi + 0x98]
    mov r12, [rdi + 0xa0]
    mov r13, [rdi + 0xa8]
    mov r14, [rdi + 0xb0]
    mov r15, [rdi + 0xb8]

    mov rdi, [rdi + 0x60]

    o64 retf






