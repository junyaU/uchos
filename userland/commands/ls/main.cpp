#include <cstdlib>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/stat.hpp>
#include <libs/common/types.hpp>
#include <libs/common/process_id.hpp>
#include <libs/user/console.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

int main(int argc, char** argv)
{
	pid_t pid = sys_getpid();
	char* input = argv[1];

	message m = { .type = msg_t::GET_DIRECTORY_CONTENTS, .sender = ProcessId::from_raw(pid) };
	if (input != nullptr) {
		memcpy(m.data.fs.name, input, strlen(input));
	} else {
		memcpy(m.data.fs.name, "/", 1);
	}

	send_message(process_ids::FS_FAT32, &m);

	message msg = wait_for_message(msg_t::GET_DIRECTORY_CONTENTS);

	char buf[256];
	size_t buf_pos = 0;
	size_t buf_size = msg.tool_desc.size;

	for (int i = 0; i < msg.tool_desc.size / sizeof(stat); i++) {
		stat* s = reinterpret_cast<stat*>(msg.tool_desc.addr) + i;
		size_t name_len = strlen(s->name);

		if (buf_pos + name_len + 1 >= buf_size) {
			break;
		}

		memcpy(buf + buf_pos, s->name, name_len);
		buf_pos += name_len;

		if (s->type == stat_type_t::DIRECTORY) {
			buf[buf_pos++] = '/';
		}

		memset(buf + buf_pos, ' ', 4);
		buf_pos += 4;
	}

	printu(buf);

	deallocate_ool_memory(ProcessId::from_raw(pid), msg.tool_desc.addr, msg.tool_desc.size);

	return 0;
}
