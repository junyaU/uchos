#pragma once

#include "ipc.hpp"

void set_timer(int ms, bool is_periodic, timeout_action_t action, int task_id);
