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


global ReadFromIoPort ; uint32_t InL(uint32_t addr)
ReadFromIoPort:
    mov dx, di
    in eax, dx
    ret




