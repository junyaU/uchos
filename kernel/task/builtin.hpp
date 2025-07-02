#pragma once

namespace kernel::task
{

[[noreturn]] void task_idle();

void task_kernel();

void task_shell();

void task_usb_handler();

} // namespace kernel::task
