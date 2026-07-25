/**
 * @file memory_layout.h
 * @brief User-space virtual memory layout shared by the kernel loader and
 * the user runtime
 *
 * Plain C header (macros, no C++) because libs/user/newlib_support.c
 * consumes it. The kernel side of the contract is kernel/elf.cpp: exec
 * maps the regions declared here before entering ring 3.
 */

#pragma once

/// Base of the per-process user heap. exec maps it eagerly (no demand
/// paging yet); sbrk() hands it out linearly from this address.
#define USER_HEAP_BASE 0xffffa00000000000ULL

/// Heap bytes mapped at exec: 1 MiB. Grow this constant when a process
/// (the future user-space FS server, issue #315 3b) needs more; there is
/// deliberately no brk/mmap syscall for now.
#define USER_HEAP_SIZE (1024ULL * 1024ULL)
