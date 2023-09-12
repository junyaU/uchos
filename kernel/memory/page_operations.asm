bits 64
section .text

global SetCR3 ; rdi = addr 
SetCR3:
    mov cr3, rdi
    ret

global GetCR3
GetCR3:
    mov rax, cr3
    ret