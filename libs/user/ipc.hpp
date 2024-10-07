#pragma once

#include <libs/common/message.hpp>
#include <libs/common/types.hpp>

void receive_message(message* msg);

void send_message(pid_t dst, const message* msg);

void initialize_task();

void deallocate_ool_memory(pid_t sender, void* addr, size_t size);
