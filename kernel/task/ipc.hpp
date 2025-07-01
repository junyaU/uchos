#pragma once

#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include <libs/common/process_id.hpp>

[[gnu::no_caller_saved_registers]] error_t send_message(ProcessId dst, message* m);
