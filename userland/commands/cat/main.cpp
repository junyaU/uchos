#include "libs/common/message.hpp"
#include <cstdlib>
#include <cstring>
#include <libs/common/types.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

extern "C" int main(int argc, char** argv)
{
	char* input = argv[1];
	message m = { .type = IPC_FILE_SYSTEM_OPERATION, .sender = CHILD_TASK };
	m.data.fs_operation.operation = FS_OP_READ;

	if (input == nullptr) {
		const char* msg = "cat: missing file operand\n";
		memcpy(m.data.fs_operation.path, msg, strlen(msg));
	} else {
		memcpy(m.data.fs_operation.path, input, strlen(input));
	}

	send_message(FS_TASK_ID, &m);

	exit(0);
}
