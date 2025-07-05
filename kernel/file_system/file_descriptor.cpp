#include "file_system/file_descriptor.hpp"
#include <cstdint>
#include <libs/common/types.hpp>
#include <libs/common/process_id.hpp>

namespace kernel::fs
{

std::array<file_descriptor, MAX_FILE_DESCRIPTORS> fds;

file_descriptor* get_fd(int fd)
{
	if (fd < 0 || fd >= MAX_FILE_DESCRIPTORS) {
		return nullptr;
	}

	return &fds[fd];
}

file_descriptor* register_fd(const char* name, size_t size, ProcessId pid)
{
	for (size_t i = 0; i < MAX_FILE_DESCRIPTORS; ++i) {
		if (fds[i].fd == -1) {
			fds[i].fd = i;
			fds[i].pid = pid;
			fds[i].size = size;
			fds[i].offset = 0;
			strncpy(fds[i].name, name, 11);
			return &fds[i];
		}
	}

	return nullptr;
}

void init_fds()
{
	fds = std::array<file_descriptor, MAX_FILE_DESCRIPTORS>();

	for (size_t i = 0; i < MAX_FILE_DESCRIPTORS; ++i) {
		fds[i].fd = -1;
	}
}

} // namespace kernel::fs
