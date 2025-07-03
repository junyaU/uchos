#pragma once

#include <cstddef>
#include <cstdint>

struct message;

using error_t = int;
using pid_t = int;
using fd_t = int;
using cluster_t = unsigned long;
using paddr_t = uint64_t;
using fs_id_t = size_t;

using message_handler_t = void (*)(const message&);

// error codes
#define IS_OK(err) (!IS_ERR(err))
#define IS_ERR(err) (((long)(err)) < 0)
constexpr int OK = 0;
constexpr int ERR_FORK_FAILED = -1;
constexpr int ERR_NO_MEMORY = -2;
constexpr int ERR_INVALID_ARG = -3;
constexpr int ERR_INVALID_TASK = -4;
constexpr int ERR_INVALID_FD = -5;
constexpr int ERR_PAGE_NOT_PRESENT = -6;
constexpr int ERR_NO_TASK = -7;
constexpr int ERR_NO_FILE = -8;
constexpr int ERR_NO_DEVICE = -9;
constexpr int ERR_FAILED_INIT_DEVICE = -10;
constexpr int ERR_FAILED_WRITE_TO_DEVICE = -11;

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
