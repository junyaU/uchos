#include "time.hpp"
#include "syscall.hpp"

void set_timer(int ms, bool is_periodic)
{
	sys_time(ms, static_cast<int>(is_periodic));
}
