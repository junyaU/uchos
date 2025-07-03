#pragma once

#include <libs/common/types.hpp>
#include "graphics/log.hpp"
#include <functional>

namespace kernel {

// エラーハンドリング用ユーティリティ関数
inline bool is_ok(error_t err)
{
	return !IS_ERR(err);
}

inline bool is_error(error_t err)
{
	return IS_ERR(err);
}

// エラーコードの文字列表現を取得
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

// エラーログマクロ
#define LOG_ERROR_CODE(err, fmt, ...)                                             \
	do {                                                                          \
		LOG_ERROR(fmt " (error: %s)", ##__VA_ARGS__, error_to_string(err));      \
	} while (0)

// 条件付きエラーリターンマクロ
#define RETURN_IF_ERROR(expression)                                               \
	do {                                                                          \
		error_t __err = (expression);                                             \
		if (IS_ERR(__err)) {                                                      \
			return __err;                                                         \
		}                                                                         \
	} while (0)

// エラーをログ出力してリターンするマクロ
#define LOG_AND_RETURN_IF_ERROR(expression, fmt, ...)                            \
	do {                                                                          \
		error_t __err = (expression);                                             \
		if (IS_ERR(__err)) {                                                      \
			LOG_ERROR_CODE(__err, fmt, ##__VA_ARGS__);                           \
			return __err;                                                         \
		}                                                                         \
	} while (0)

// 結果型（成功時は値、失敗時はエラーコード）
template<typename T>
class result {
public:
	result(T value) : value_(value), error_(OK) {}
	result(error_t error) : value_(), error_(error) {}
	
	bool is_ok() const { return error_ == OK; }
	bool is_error() const { return error_ != OK; }
	
	T value() const { return value_; }
	error_t error() const { return error_; }
	
	T value_or(T default_value) const {
		return is_ok() ? value_ : default_value;
	}
	
private:
	T value_;
	error_t error_;
};

// Optional型（存在するかどうか）
template<typename T>
class optional {
public:
	optional() : has_value_(false), value_() {}
	optional(T value) : has_value_(true), value_(value) {}
	
	bool has_value() const { return has_value_; }
	T value() const { return value_; }
	T value_or(T default_value) const {
		return has_value_ ? value_ : default_value;
	}
	
	explicit operator bool() const { return has_value_; }
	
private:
	bool has_value_;
	T value_;
};

// エラーハンドリングのためのヘルパー関数
template<typename T>
inline error_t try_with_cleanup(T&& func, std::function<void()> cleanup)
{
	error_t result = func();
	if (IS_ERR(result)) {
		cleanup();
	}
	return result;
}

// チェイン可能なエラーハンドリング
class error_chain {
public:
	error_chain() : error_(OK) {}
	
	error_chain& then(std::function<error_t()> func) {
		if (is_ok(error_)) {
			error_ = func();
		}
		return *this;
	}
	
	error_chain& on_error(std::function<void(error_t)> handler) {
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