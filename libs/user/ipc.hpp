#pragma once

#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include <libs/common/process_id.hpp>

void receive_message(message* msg);

void send_message(ProcessId dst, const message* msg);

message wait_for_message(msg_t type);

void initialize_task();

void deallocate_ool_memory(ProcessId sender, void* addr, size_t size);
