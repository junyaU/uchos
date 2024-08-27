#pragma once

#include <cstddef>
#include <cstdint>

using error_t = int;
using pid_t = int;
using fd_t = int;
using cluster_t = unsigned long;
using paddr_t = uint64_t;
using fs_id_t = size_t;

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
constexpr int ERR_FAILED_INIT_DEVICE = -9;
constexpr int ERR_FAILED_WRITE_TO_DEVICE = -10;

// task ids
constexpr int KERNEL_TASK_ID = 0;
constexpr int IDLE_TASK_ID = 1;
constexpr int XHCI_TASK_ID = 2;
constexpr int FS_TASK_ID = 3;
constexpr int VIRTIO_BLK_TASK_ID = 4;
constexpr int FS_FAT32_TASK_ID = 5;
constexpr int SHELL_TASK_ID = 6;
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

#define ASSERT_OK(expression)                                                       \
	do {                                                                            \
		error_t __err = (expression);                                               \
		if (IS_ERR(__err)) {                                                        \
			return __err;                                                           \
		}                                                                           \
	} while (0)
