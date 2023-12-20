#include "commands.hpp"
#include "../file_system/fat.hpp"
#include "../graphics/terminal.hpp"

namespace command_line
{
void ls(terminal& term, const char* path)
{
	const auto entries_per_cluster =
			file_system::bytes_per_cluster / sizeof(file_system::directory_entry);

	uint32_t cluster_id = file_system::boot_volume_image->root_cluster;
	while (cluster_id != file_system::END_OF_CLUSTERCHAIN) {
		auto* dir_entry =
				file_system::get_sector<file_system::directory_entry>(cluster_id);

		for (int i = 0; i < entries_per_cluster; i++) {
			if (dir_entry[i].name[0] == 0x00) {
				return;
			}

			if (dir_entry[i].name[0] == 0xE5) {
				continue;
			}

			if (dir_entry[i].attribute == file_system::entry_attribute::LONG_NAME) {
				continue;
			}

			char name[13];
			file_system::read_dir_entry_name(dir_entry[i], name);

			term.printf("%s\n", name);
		}

		cluster_id = file_system::next_cluster(cluster_id);
	}
}

void cat(terminal& term, const char* file_name)
{
	auto* entry = file_system::find_directory_entry(file_name, 0);
	if (entry == nullptr) {
		term.printf("File not found: %s\n", file_name);
		return;
	}

	auto cluster_id = entry->first_cluster();
	auto remain_bytes = entry->file_size;

	while (cluster_id != file_system::END_OF_CLUSTERCHAIN) {
		auto* p = file_system::get_sector<char>(cluster_id);

		int i = 0;
		for (; i < file_system::bytes_per_cluster && i < remain_bytes; i++) {
			term.printf("%c", *p);
			++p;
		}

		remain_bytes -= i;
		cluster_id = file_system::next_cluster(cluster_id);
	}
}
} // namespace command_line