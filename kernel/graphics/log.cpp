#include "log.hpp"
#include "font.hpp"
#include "screen.hpp"
#include "task/task.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/types.hpp>

void printk(int level, const char* format, ...)
{
	if (level != CURRENT_LOG_LEVEL) {
		return;
	}

	va_list ap;
	int result;
	char s[1024];

	va_start(ap, format);
	result = vsprintf(s, format, ap);
	va_end(ap);

	for (size_t i = 0; i < strlen(s) && s[i] != '\0'; ++i) {
		write_ascii(*kscreen, { kernel_cursor_x++ * 8, kernel_cursor_y * 16 }, s[i],
					0xffff00);

		if (s[i] == '\n') {
			kernel_cursor_x = 0;
			++kernel_cursor_y;
			continue;
		}

		if (kernel_cursor_x >= 98) {
			kernel_cursor_x = 0;
			++kernel_cursor_y;
		}
	}

	kernel_cursor_x = 0;
	++kernel_cursor_y;
}

size_t term_file_descriptor::read(void* buf, size_t len)
{
	char* bufc = reinterpret_cast<char*>(buf);
	task* t = CURRENT_TASK;

	if (t->messages.empty()) {
		t->state = TASK_WAITING;
		return 0;
	}

	t->state = TASK_RUNNING;

	size_t read_len = 0;
	while (!t->messages.empty() && read_len < len) {
		const message m = t->messages.front();
		t->messages.pop();

		if (m.type == NOTIFY_KEY_INPUT) {
			bufc[read_len++] = m.data.key_input.ascii;
		}
	}

	return read_len;
}

size_t term_file_descriptor::write(const void* buf, size_t len)
{
	printk(KERN_DEBUG, reinterpret_cast<const char*>(buf));
	return len;
}
