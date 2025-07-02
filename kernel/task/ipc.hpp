#pragma once

#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

namespace kernel::task
{

[[gnu::no_caller_saved_registers]] error_t send_message(ProcessId dst, message& m);

} // namespace kernel::task
