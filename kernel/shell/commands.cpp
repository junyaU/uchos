#include "commands.hpp"
#include "../file_system/fat.hpp"
#include "../graphics/terminal.hpp"

namespace shell
{
void ls(terminal& term, const char* path)
{
	auto* entry = file_system::find_directory_entry_by_path(path);
	if (entry == nullptr) {
		term.printf("ls: %s: No such file or directory\n", path);
		return;
	}

	if (entry->attribute == file_system::entry_attribute::ARCHIVE) {
		term.printf("%s\n", path);
		return;
	}

	auto entries = file_system::list_entries_in_directory(entry);
	for (const auto* e : entries) {
		char name[13];
		file_system::read_dir_entry_name(*e, name);
		term.printf("%s\n", name);
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
} // namespace shell