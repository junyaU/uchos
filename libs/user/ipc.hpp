#pragma once

#include <libs/common/message.hpp>

void receive_message(message* msg);

void send_message(int task_id, const message* msg);

void initialize_task();
