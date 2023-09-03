bits 64
section .text

global LoadCodeSegment ; rdi = cs
LoadCodeSegment:
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

global LoadStackSegment ; rdi = ss
LoadStackSegment:
    push rbp
    mov rbp, rsp
    mov ss, rdi
    mov rsp, rbp
    pop rbp
    ret

global LoadDataSegment ; rdi = ds
LoadDataSegment:
    push rbp
    mov rbp, rsp
    mov ds, rdi
    mov es, rdi
    mov fs, rdi
    mov gs, rdi
    mov rsp, rbp
    pop rbp
    ret

global LoadGDT ; di = size, rsi = offset
LoadGDT:
    push rbp
    mov rbp, rsp
    sub rsp, 0x10
    mov [rsp], di
    mov [rsp + 2], rsi
    lgdt [rsp]
    mov rsp, rbp
    pop rbp
    ret

global SetCR3 ; rdi = addr 
SetCR3:
    mov cr3, rdi
    ret

global GetCR3
GetCR3:
    mov rax, cr3
    ret