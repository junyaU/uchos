#pragma once

namespace kernel
{

// パニック関数のプロトタイプ
[[noreturn]] void
panic(const char* file, int line, const char* func, const char* fmt, ...);

// カーネルパニックマクロ
#define PANIC(fmt, ...)                                                             \
	kernel::panic(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

// アサーションマクロ
#define ASSERT(condition)                                                           \
	do {                                                                            \
		if (!(condition)) {                                                         \
			PANIC("Assertion failed: %s", #condition);                              \
		}                                                                           \
	} while (0)

// デバッグ時のみ有効なアサーション
#ifdef DEBUG
#define DEBUG_ASSERT(condition) ASSERT(condition)
#else
#define DEBUG_ASSERT(condition) ((void)0)
#endif

// ヌルポインタチェック
#define ASSERT_NOT_NULL(ptr)                                                        \
	do {                                                                            \
		if ((ptr) == nullptr) {                                                     \
			PANIC("Null pointer: %s", #ptr);                                        \
		}                                                                           \
	} while (0)

// 範囲チェック
#define ASSERT_IN_RANGE(value, min, max)                                            \
	do {                                                                            \
		if ((value) < (min) || (value) > (max)) {                                   \
			PANIC("Value %s = %ld out of range [%ld, %ld]", #value, (long)(value),  \
				  (long)(min), (long)(max));                                        \
		}                                                                           \
	} while (0)

// 未実装機能
#define NOT_IMPLEMENTED() PANIC("Not implemented")

// 到達不能コード
#define UNREACHABLE() PANIC("Unreachable code reached")

} // namespace kernel