bits 64
section .text

extern switch_task_by_timer_interrupt

global on_timer_interrupt ; void on_timer_interrupt(void)
on_timer_interrupt:
    push rbp
    mov rbp, rsp

    sub rsp, 512
    fxsave [rsp]
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push qword [rbp] ; rbp
    push qword [rbp + 0x20] ; rsp
    push rsi
    push rdi
    push rdx
    push rcx
    push rbx
    push rax

    mov ax, fs
    mov bx, gs
    mov rcx, cr3

    push rbx
    push rax
    push qword [rbp + 0x28] ; ss
    push qword [rbp + 0x10] ; cs
    push rbp
    push qword [rbp + 0x18] ; rflags
    push qword [rbp + 0x08] ; rip
    push rcx

    mov rdi, rsp ; rdi = context
    call switch_task_by_timer_interrupt

    ; restore context (switch_task_by_timer_interruptでコンテキストがスイッチされなかった場合のみここに戻ってくる)
    add rsp, 8*8
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rdi
    pop rsi
    add rsp, 8*2
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    fxrstor [rsp]

    mov rsp, rbp
    pop rbp
    iretq




