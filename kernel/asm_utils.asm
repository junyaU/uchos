bits 64
section .text

global ReadFromIoPort ; uint32_t InL(uint32_t addr)
ReadFromIoPort:
    mov dx, di
    in eax, dx
    ret

