#pragma once

#include "ipc.hpp"

void set_timer(int ms, bool is_periodic, TimeoutAction action, int task_id);
