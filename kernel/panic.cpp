#include "panic.hpp"
#include "graphics/log.hpp"
#include <cstdarg>
#include <cstdio>

namespace kernel {

[[noreturn]] void panic(const char* file, int line, const char* func, const char* fmt, ...)
{
	// 割り込みを無効化
	asm volatile("cli");
	
	// パニックメッセージの出力
	LOG_ERROR("KERNEL PANIC at %s:%d in %s()", file, line, func);
	
	// フォーマットされたメッセージを出力
	if (fmt != nullptr) {
		char buffer[256];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, sizeof(buffer), fmt, args);
		va_end(args);
		LOG_ERROR("  %s", buffer);
	}
	
	// スタックトレース（将来実装）
	// print_stack_trace();
	
	// システムを停止
	while (true) {
		asm volatile("hlt");
	}
}

} // namespace kernel