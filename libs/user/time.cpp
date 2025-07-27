#include "time.hpp"
#include "syscall.hpp"

void set_timer(int ms, bool is_periodic, TimeoutAction action, int task_id)
{
	sys_time(ms, static_cast<int>(is_periodic), static_cast<uint8_t>(action),
			 task_id);
}
