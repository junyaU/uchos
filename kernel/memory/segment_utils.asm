bits 64
section .text

global load_code_segment ; rdi = cs
load_code_segment:
    push rbp
    mov rbp, rsp
    mov rax, .epilogue
    push rdi
    push rax
    o64 retf
.epilogue:
    mov rsp, rbp
    pop rbp
    ret

global load_stack_segment ; rdi = ss
load_stack_segment:
    push rbp
    mov rbp, rsp
    mov ss, rdi
    mov rsp, rbp
    pop rbp
    ret

global load_data_segment ; rdi = ds
load_data_segment:
    push rbp
    mov rbp, rsp
    mov ds, rdi
    mov es, rdi
    mov fs, rdi
    mov gs, rdi
    mov rsp, rbp
    pop rbp
    ret

global load_gdt ; di = size, rsi = offset
load_gdt:
    push rbp
    mov rbp, rsp
    sub rsp, 0x10
    mov [rsp], di
    mov [rsp + 2], rsi
    lgdt [rsp]
    mov rsp, rbp
    pop rbp
    ret

global load_tr ; di = tr
load_tr:
    ltr di
    ret
