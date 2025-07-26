#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/stat.hpp>
#include <libs/common/types.hpp>
#include <libs/user/console.hpp>
#include <libs/user/ipc.hpp>
#include <libs/user/syscall.hpp>

int main(int argc, char** argv)
{
	pid_t pid = sys_getpid();
	char* input = argv[1];

	Message m = { .type = MsgType::GET_DIRECTORY_CONTENTS, .sender = ProcessId::from_raw(pid) };
	if (input != nullptr) {
		memcpy(m.data.fs.name, input, strlen(input));
	} else {
		memcpy(m.data.fs.name, "/", 1);
	}

	send_message(process_ids::FS_FAT32, &m);

	Message msg = wait_for_message(MsgType::GET_DIRECTORY_CONTENTS);

	char buf[256];
	size_t buf_pos = 0;
	size_t buf_size = msg.tool_desc.size;

	for (int i = 0; i < msg.tool_desc.size / sizeof(stat); i++) {
		stat* s = reinterpret_cast<stat*>(msg.tool_desc.addr) + i;

		// Skip . and .. entries
		if (strcmp(s->name, ".") == 0 || strcmp(s->name, "..") == 0) {
			continue;
		}

		size_t name_len = strlen(s->name);
		size_t space_needed = name_len + 5;  // name + optional '/' + 4 spaces

		if (buf_pos + space_needed >= buf_size) {
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

	buf[buf_pos] = '\0';  // Ensure null termination
	printu(buf);

	deallocate_ool_memory(ProcessId::from_raw(pid), msg.tool_desc.addr, msg.tool_desc.size);

	return 0;
}
