#include "ipc.hpp"
#include "syscall.hpp"

void receive_message(message* msg) { sys_ipc(0, 0, msg, IPC_RECV); }
