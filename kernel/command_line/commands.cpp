#include "commands.hpp"
#include "../file_system/fat.hpp"
#include "../graphics/terminal.hpp"

namespace command_line
{
void ls(terminal& term, const char* path)
{
	const auto entries_per_cluster =
			file_system::bytes_per_cluster / sizeof(file_system::directory_entry);

	while (file_system::boot_volume_image->root_cluster !=
		   file_system::END_OF_CLUSTERCHAIN) {
		auto* dir_entry = file_system::get_sector<file_system::directory_entry>(
				file_system::boot_volume_image->root_cluster);

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
	}
}
} // namespace command_line