/**
 * @file error.hpp
 * @brief Unified kernel error handling interface
 *
 * This file provides a unified error handling mechanism used throughout the UCHos
 * kernel. It includes various utilities for error code processing, logging, and
 * error propagation.
 *
 * @date 2024
 */

#pragma once

#include <libs/common/types.hpp>
#include "log/log.hpp"

namespace kernel
{

/**
 * @brief Convert error code to human-readable string
 * @param err Error code to convert
 * @return String representation of the error code
 *
 * The switch is exhaustive over ErrorCode (no default label) so -Wswitch
 * flags a missing case here whenever a new ErrorCode value is added.
 */
inline const char* error_to_string(error_t err)
{
	switch (static_cast<ErrorCode>(err)) {
		case OK:
			return "OK";
		case ERR_FORK_FAILED:
			return "Fork failed";
		case ERR_NO_MEMORY:
			return "No memory";
		case ERR_INVALID_ARG:
			return "Invalid argument";
		case ERR_INVALID_TASK:
			return "Invalid task";
		case ERR_INVALID_FD:
			return "Invalid file descriptor";
		case ERR_PAGE_NOT_PRESENT:
			return "Page not present";
		case ERR_NO_TASK:
			return "No task";
		case ERR_NO_FILE:
			return "No file";
		case ERR_NO_DEVICE:
			return "No device";
		case ERR_FAILED_INIT_DEVICE:
			return "Failed to initialize device";
		case ERR_FAILED_WRITE_TO_DEVICE:
			return "Failed to write to device";
		case ERR_QUEUE_FULL:
			return "Message queue full";
		case ERR_FAILED_READ_FROM_DEVICE:
			return "Failed to read from device";
		case ERR_NO_SPACE:
			return "No space";
		case ERR_OOL_LIMIT:
			return "OOL limit exceeded";
	}

	return "Unknown error";
}

/**
 * @brief Log an error message with error code
 * @param err Error code
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
#define LOG_ERROR_CODE(err, fmt, ...)                                               \
	do {                                                                            \
		LOG_ERROR(fmt " (error: %s)", ##__VA_ARGS__, error_to_string(err));         \
	} while (0)

/**
 * @brief Return immediately if expression evaluates to an error
 * @param expression Expression to evaluate (must return error_t)
 */
#define RETURN_IF_ERROR(expression)                                                 \
	do {                                                                            \
		const error_t __err = (expression);                                         \
		if (IS_ERR(__err)) {                                                        \
			return __err;                                                           \
		}                                                                           \
	} while (0)

} // namespace kernel
