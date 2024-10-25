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

define_syscall log_string, 1
define_syscall draw_text, 4
define_syscall fill_rect, 5
define_syscall time, 6
define_syscall ipc, 7
define_syscall fork, 57
define_syscall exec, 59
define_syscall exit, 60
define_syscall wait, 61
define_syscall getpid, 39
define_syscall getppid, 110
