#include "commands.hpp"
#include "../file_system/fat.hpp"
#include "../graphics/terminal.hpp"
#include <cstddef>

void write_fd(terminal& term,
			  const char* s,
			  file_system::directory_entry* redirect_entry)
{
	if (redirect_entry == nullptr) {
		term.fds_[STDOUT_FILENO]->write(s, strlen(s));
		return;
	}

	file_system::file_descriptor(*redirect_entry).write(s, strlen(s));
}

namespace shell
{
void ls(terminal& term,
		const char* path,
		file_system::directory_entry* redirect_entry)
{
	auto* entry = file_system::find_directory_entry_by_path(path);
	if (entry == nullptr) {
		term.printf("ls: %s: No such file or directory\n", path);
		return;
	}

	if (entry->attribute == file_system::entry_attribute::ARCHIVE) {
		write_fd(term, path, redirect_entry);
		write_fd(term, "\n", redirect_entry);
		return;
	}

	auto entries = file_system::list_entries_in_directory(entry);
	for (const auto* e : entries) {
		char name[13];
		file_system::read_dir_entry_name(*e, name);
		write_fd(term, name, redirect_entry);
		write_fd(term, "\n", redirect_entry);
	}
}

void cat(terminal& term, const char* file_name)
{
	auto* entry = file_system::find_directory_entry_by_path(file_name);
	if (entry == nullptr) {
		term.printf("cat: %s: No such file or directory\n", file_name);
		return;
	}

	auto cluster_id = entry->first_cluster();
	auto remain_bytes = entry->file_size;

	while (cluster_id != file_system::END_OF_CLUSTER_CHAIN) {
		auto* p = file_system::get_sector<char>(cluster_id);

		int i = 0;
		while (i < file_system::BYTES_PER_CLUSTER && i < remain_bytes) {
			size_t len = term.print(&p[i], 1);
			i += len;
		}

		remain_bytes -= i;
		cluster_id = file_system::next_cluster(cluster_id);
	}
}

void echo(terminal& term,
		  const char* s,
		  file_system::directory_entry* redirect_entry)
{
	write_fd(term, s, redirect_entry);
}
} // namespace shell