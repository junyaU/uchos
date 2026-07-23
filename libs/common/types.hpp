#pragma once

#include <cstddef>
#include <cstdint>

struct Message;

using error_t = int;
using pid_t = int;
using fd_t = int;
using cluster_t = unsigned long;
using paddr_t = uint64_t;
using fs_id_t = size_t;

using message_handler_t = void (*)(const Message&);

// error codes
#define IS_OK(err) (!IS_ERR(err))
#define IS_ERR(err) (((long)(err)) < 0)

/**
 * @brief Kernel-wide error codes
 *
 * Unscoped enum with a fixed underlying type: values keep their implicit
 * conversion to int (so existing callers need no changes), while giving
 * error_to_string() an exhaustive switch (no default) so -Wswitch catches a
 * missing case at compile time.
 */
enum ErrorCode : error_t {
	OK = 0,
	ERR_FORK_FAILED = -1,
	ERR_NO_MEMORY = -2,
	ERR_INVALID_ARG = -3,
	ERR_INVALID_TASK = -4,
	ERR_INVALID_FD = -5,
	ERR_PAGE_NOT_PRESENT = -6,
	ERR_NO_TASK = -7,
	ERR_NO_FILE = -8,
	ERR_NO_DEVICE = -9,
	ERR_FAILED_INIT_DEVICE = -10,
	ERR_FAILED_WRITE_TO_DEVICE = -11,
	ERR_QUEUE_FULL = -12,
	ERR_FAILED_READ_FROM_DEVICE = -13,
	ERR_NO_SPACE = -14,
	ERR_OOL_LIMIT = -15,
};

// file descriptor
constexpr int NO_FD = -1;
constexpr int STDIN_FILENO = 0;
constexpr int STDOUT_FILENO = 1;
constexpr int STDERR_FILENO = 2;
