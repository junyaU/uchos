#pragma once

#include <libs/common/message.hpp>
#include <libs/common/types.hpp>

[[gnu::no_caller_saved_registers]] error_t
send_message(task_t dst, const message* m);
