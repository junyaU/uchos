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

global call_userland ; void call_userland(int argc, char** argv, uint16_t cs, uint16_t ss, uint64_t rip, uint64_t rsp);
call_userland:
    push rbp
    mov rbp, rsp
    push rcx ; ss
    push r9  ; rsp
    push rdx ; cs
    push r8  ; rip
    o64 retf