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
	char* input = argv[1];

	Message m = make_request(MsgType::GET_DIRECTORY_CONTENTS);
	if (input != nullptr) {
		memcpy(m.data.fs.name, input, strlen(input));
	} else {
		memcpy(m.data.fs.name, "/", 1);
	}

	Message msg = call(process_ids::FS_FAT32, &m);

	char buf[256];
	size_t buf_pos = 0;
	size_t buf_size = msg.tool_desc.size;

	for (int i = 0; i < msg.tool_desc.size / sizeof(Stat); i++) {
		Stat* s = reinterpret_cast<Stat*>(msg.tool_desc.addr) + i;

		// Skip . and .. entries
		if (strcmp(s->name, ".") == 0 || strcmp(s->name, "..") == 0) {
			continue;
		}

		size_t name_len = strlen(s->name);
		size_t space_needed = name_len + 5; // name + optional '/' + 4 spaces

		if (buf_pos + space_needed >= buf_size) {
			break;
		}

		memcpy(buf + buf_pos, s->name, name_len);
		buf_pos += name_len;

		if (s->type == StatType::DIRECTORY) {
			buf[buf_pos++] = '/';
		}

		memset(buf + buf_pos, ' ', 4);
		buf_pos += 4;
	}

	buf[buf_pos] = '\0'; // Ensure null termination
	printu(buf);

	deallocate_ool_memory(m.sender, msg.tool_desc.addr, msg.tool_desc.size);

	return 0;
}
