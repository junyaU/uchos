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

	Message m = make_request(MsgType::FS_LIST_DIR);
	if (input != nullptr) {
		memcpy(m.data.fs.name, input, strlen(input));
	} else {
		memcpy(m.data.fs.name, "/", 1);
	}

	Message msg = call(process_ids::FS_FAT32, &m);
	if (IS_ERR(msg.result)) {
		printu("ls: failed to read directory");
		return 0;
	}

	// An empty directory has no OOL payload; there is nothing to release
	if (msg.ool.size == 0) {
		printu("");
		return 0;
	}

	const Stat* stats = reinterpret_cast<const Stat*>(msg.ool.addr);
	const size_t num_entries = msg.ool.size / sizeof(Stat);

	char buf[256];
	size_t buf_pos = 0;

	for (size_t i = 0; i < num_entries; i++) {
		const Stat* s = &stats[i];

		// Skip . and .. entries
		if (strcmp(s->name, ".") == 0 || strcmp(s->name, "..") == 0) {
			continue;
		}

		size_t name_len = strlen(s->name);
		size_t space_needed = name_len + 5; // name + optional '/' + 4 spaces

		if (buf_pos + space_needed >= sizeof(buf)) {
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

	ool_release(stats);

	return 0;
}
