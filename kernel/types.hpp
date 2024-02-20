#pragma once

using error_t = int;
using task_t = int;
using fd_t = int;

// kmalloc flags
#define KMALLOC_UNINITIALIZED 0
#define KMALLOC_ZEROED (1 << 0)

// error codes
#define IS_OK(err) (!IS_ERR(err))
#define IS_ERR(err) (((long)(err)) < 0)
#define OK 0
#define ERR_NO_MEMORY -1
#define ERR_INVALID_ARG -2
#define ERR_INVALID_TASK -3

// task ids
#define INTERRUPT_TASK_ID 100

// log levels
#define KERN_DEBUG 0
#define KERN_ERROR 1
#define KERN_INFO 2

// file descriptor
#define NO_FD -1
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

// fat
using cluster_t = unsigned long;