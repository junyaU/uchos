#include <unistd.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <libs/user/console.hpp>

void printu(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[256];
	vsprintf(buffer, format, args);
	va_end(args);

	write(STDOUT_FILENO, buffer, strlen(buffer));
}
