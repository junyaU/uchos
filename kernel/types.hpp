#pragma once

using error_t = int;
using task_t = int;
using fd_t = int;

// kmalloc flags
constexpr int KMALLOC_UNINITIALIZED = 0;
constexpr int KMALLOC_ZEROED = (1 << 0);

// error codes
#define IS_OK(err) (!IS_ERR(err))
#define IS_ERR(err) (((long)(err)) < 0)
constexpr int OK = 0;
constexpr int ERR_NO_MEMORY = -1;
constexpr int ERR_INVALID_ARG = -2;
constexpr int ERR_INVALID_TASK = -3;

// task ids
constexpr int KERNEL_TASK_ID = 0;
constexpr int XHCI_TASK_ID = 2;
constexpr int FS_TASK_ID = 3;
constexpr int SHELL_TASK_ID = 4;
constexpr int INTERRUPT_TASK_ID = 100;

// log levels
constexpr int KERN_DEBUG = 0;
constexpr int KERN_ERROR = 1;
constexpr int KERN_INFO = 2;

// file descriptor
constexpr int NO_FD = -1;
constexpr int STDIN_FILENO = 0;
constexpr int STDOUT_FILENO = 1;
constexpr int STDERR_FILENO = 2;

// fat
using cluster_t = unsigned long;