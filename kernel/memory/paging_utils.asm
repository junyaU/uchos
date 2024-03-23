; System V AMD64 Calling Convention
; Registers(args): RDI, RSI, RDX, RCX, R8, R9
; Caller-saved registers: RAX, RDI, RSI, RDX, RCX, R8, R9, R10, R11
; Callee-saved registers: RBX, RBP, R12, R13, R14, R15

bits 64
section .text

global flush_tlb ; void flash_tlb(uint64_t addr);
flush_tlb:
    invlpg [rdi]
    ret

global set_cr3 ; rdi = addr
set_cr3:
    mov cr3, rdi
    ret

global get_cr3
get_cr3:
    mov rax, cr3
    ret

global get_cr2
get_cr2:
    mov rax, cr2
    ret
