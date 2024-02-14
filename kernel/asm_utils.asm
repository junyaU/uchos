; System V AMD64 Calling Convention
; Registers(args): RDI, RSI, RDX, RCX, R8, R9
; Caller-saved registers: RAX, RDI, RSI, RDX, RCX, R8, R9, R10, R11
; Callee-saved registers: RBX, RBP, R12, R13, R14, R15

bits 64
section .text

extern kernel_stack
extern Main
global KernelMain
KernelMain:
    mov rsp, kernel_stack + 1024 * 1024
    call Main
.fin:
    hlt
    jmp .fin


global read_from_io_port ; uint32_t read_from_io_port(uint16_t addr)
read_from_io_port:
    mov dx, di
    in eax, dx
    ret

global write_to_io_port ; void write_to_io_port(uint16_t addr, uint32_t value)
write_to_io_port:
    mov dx, di
    mov eax, esi
    out dx, eax
    ret

global most_significant_bit ; int most_significant_bit(uint32_t value)
most_significant_bit:
    bsr eax, edi
    ret

global write_msr ; void write_msr(uint32_t msr, uint64_t value)
write_msr:
    mov rdx, rsi
    shr rdx, 32
    mov eax, esi
    mov ecx, edi
    wrmsr
    ret

global call_userland ; void call_userland(int argc, char** argv, uint16_t ss, uint64_t rip, uint64_t rsp, uint64_t* kernel_rsp)
call_userland:
    ; save callee-saved registers
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov [r9], rsp ; save kernel stack pointer

    push rdx ; ss
    push r8  ; rsp
    add rdx, 8 ; get cs
    push rdx ; cs
    push rcx  ; rip
    o64 retf

global exit_userland ; void exit_userland(uint64_t kernel_rsp, int32_t status);
exit_userland:
    mov rsp, rdi
    mov eax, esi

    ; restore callee-saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ret