bits 64
section .text

global set_cr3 ; rdi = addr
set_cr3:
    mov cr3, rdi
    ret

global get_cr3
get_cr3:
    mov rax, cr3
    ret