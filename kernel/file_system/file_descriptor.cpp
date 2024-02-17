#include "file_system/file_descriptor.hpp"
#include "file_system/fat.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace file_system
{
size_t file_descriptor::read(void* buf, size_t len)
{
	if (current_cluster == 0) {
		current_cluster = entry.first_cluster();
	}

	uint8_t* p = reinterpret_cast<uint8_t*>(buf);
	len = std::min(len, entry.file_size - current_file_offset);

	size_t total_read = 0;
	while (total_read < len) {
		uint8_t* sector = get_sector<uint8_t>(current_cluster);
		const size_t cluster_remain = bytes_per_cluster - current_cluster_offset;
		const size_t read_len = std::min(len - total_read, cluster_remain);

		memcpy(&p[total_read], &sector[current_cluster_offset], read_len);
		total_read += read_len;
		current_cluster_offset += read_len;

		if (current_cluster_offset == bytes_per_cluster) {
			current_cluster = next_cluster(current_cluster);
			current_cluster_offset = 0;
		}
	}

	current_file_offset += total_read;
	return total_read;
}
} // namespace file_system