#pragma once

#include <libs/common/message.hpp>
#include <libs/common/types.hpp>
#include <libs/common/process_id.hpp>

void receive_message(Message* msg);

void send_message(ProcessId dst, const Message* msg);

Message wait_for_message(MsgType type);

void initialize_task();

void deallocate_ool_memory(ProcessId sender, void* addr, size_t size);
