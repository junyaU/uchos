bits 64
section .text

%macro define_syscall 2
global sys_%1
sys_%1:
    mov rax, %2
    mov r10, rcx
    syscall
    ret
%endmacro

define_syscall read, 0
define_syscall log_string, 1
define_syscall open, 2
define_syscall draw_text, 4
define_syscall fill_rect, 5
define_syscall exit, 60
