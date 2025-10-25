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

#include <functional>
#include <libs/common/types.hpp>
#include "graphics/log.hpp"

namespace kernel
{

/**
 * @brief Check if an error code indicates success
 * @param err Error code to check
 * @return true if success, false if error
 */
inline bool is_ok(error_t err) { return !IS_ERR(err); }

/**
 * @brief Check if an error code indicates an error
 * @param err Error code to check
 * @return true if error, false if success
 */
inline bool is_error(error_t err) { return IS_ERR(err); }

/**
 * @brief Convert error code to human-readable string
 * @param err Error code to convert
 * @return String representation of the error code
 */
inline const char* error_to_string(error_t err)
{
	switch (err) {
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
		default:
			return "Unknown error";
	}
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

/**
 * @brief Log and return if expression evaluates to an error
 * @param expression Expression to evaluate (must return error_t)
 * @param fmt Format string for error message
 * @param ... Format arguments
 */
#define LOG_AND_RETURN_IF_ERROR(expression, fmt, ...)                               \
	do {                                                                            \
		const error_t __err = (expression);                                         \
		if (IS_ERR(__err)) {                                                        \
			LOG_ERROR_CODE(__err, fmt, ##__VA_ARGS__);                              \
			return __err;                                                           \
		}                                                                           \
	} while (0)

/**
 * @brief Result type that holds either a value on success or an error code on
 * failure
 * @tparam T Type of the success value
 *
 * @example
 * result<int> divide(int a, int b) {
 *     if (b == 0) return ERR_INVALID_ARG;
 *     return a / b;
 * }
 */
template<typename T>
class Result
{
public:
	Result(T value) : value_(value), error_(OK) {}
	Result(error_t error) : value_(), error_(error) {}

	bool is_ok() const { return error_ == OK; }
	bool is_error() const { return error_ != OK; }

	T value() const { return value_; }
	error_t error() const { return error_; }

	T value_or(T default_value) const { return is_ok() ? value_ : default_value; }

private:
	T value_;
	error_t error_;
};

/**
 * @brief Optional type that represents whether a value exists or not
 * @tparam T Type of the held value
 *
 * @example
 * optional<int> find_index(int* arr, int size, int target) {
 *     for (int i = 0; i < size; i++) {
 *         if (arr[i] == target) return i;
 *     }
 *     return {}; // empty optional
 * }
 */
template<typename T>
class Optional
{
public:
	Optional() : has_value_(false), value_() {}
	Optional(T value) : has_value_(true), value_(value) {}

	bool has_value() const { return has_value_; }
	T value() const { return value_; }
	T value_or(T default_value) const { return has_value_ ? value_ : default_value; }

	explicit operator bool() const { return has_value_; }

private:
	bool has_value_;
	T value_;
};

/**
 * @brief Helper function that executes cleanup on error
 * @tparam T Type of the function to execute
 * @param func Function to execute (must return error_t)
 * @param cleanup Cleanup function to execute on error
 * @return Result of func execution
 */
template<typename T>
inline error_t try_with_cleanup(T&& func, std::function<void()> cleanup)
{
	error_t result = func();
	if (IS_ERR(result)) {
		cleanup();
	}
	return result;
}

/**
 * @brief Class for chaining multiple operations with concise error handling
 *
 * @example
 * error_t result = error_chain()
 *     .then([]() { return step1(); })
 *     .then([]() { return step2(); })
 *     .on_error([](error_t err) { LOG_ERROR("Failed: %s", error_to_string(err)); })
 *     .result();
 */
class ErrorChain
{
public:
	ErrorChain() : error_(OK) {}

	ErrorChain& then(std::function<error_t()> func)
	{
		if (is_ok(error_)) {
			error_ = func();
		}
		return *this;
	}

	ErrorChain& on_error(std::function<void(error_t)> handler)
	{
		if (is_error(error_)) {
			handler(error_);
		}
		return *this;
	}

	error_t result() const { return error_; }

private:
	error_t error_;
};

} // namespace kernel