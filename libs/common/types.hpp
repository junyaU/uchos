#pragma once

using error_t = int;
using pid_t = int;
using fd_t = int;
using cluster_t = unsigned long;

// error codes
#define IS_OK(err) (!IS_ERR(err))
#define IS_ERR(err) (((long)(err)) < 0)
constexpr int OK = 0;
constexpr int ERR_FORK_FAILED = -1;
constexpr int ERR_NO_MEMORY = -1;
constexpr int ERR_INVALID_ARG = -2;
constexpr int ERR_INVALID_TASK = -3;
constexpr int ERR_INVALID_FD = -4;
constexpr int ERR_PAGE_NOT_PRESENT = -5;
constexpr int ERR_NO_TASK = -6;
constexpr int ERR_NO_FILE = -7;
constexpr int ERR_NO_DEVICE = -8;

// task ids
constexpr int KERNEL_TASK_ID = 0;
constexpr int XHCI_TASK_ID = 2;
constexpr int FS_TASK_ID = 3;
constexpr int SHELL_TASK_ID = 4;

constexpr int INTERRUPT_TASK = 100;

// log levels
constexpr int KERN_DEBUG = 0;
constexpr int KERN_ERROR = 1;
constexpr int KERN_INFO = 2;

// file descriptor
constexpr int NO_FD = -1;
constexpr int STDIN_FILENO = 0;
constexpr int STDOUT_FILENO = 1;
constexpr int STDERR_FILENO = 2;
