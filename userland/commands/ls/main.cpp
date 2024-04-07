#include "libs/common/message.hpp"
#include <cstdlib>
#include <cstring>
#include <libs/common/types.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

extern "C" int main(int argc, char** argv)
{
	message m = { .type = IPC_FILE_SYSTEM_OPERATION, .sender = CHILD_TASK };
	m.data.fs_operation.operation = FS_OP_LIST;

	char* input = argv[1];

	if (input == nullptr) {
		memcpy(m.data.fs_operation.path, "/", 2);
	} else {
		memcpy(m.data.fs_operation.path, input, strlen(input));
	}

	send_message(FS_TASK_ID, &m);

	exit(0);
}
