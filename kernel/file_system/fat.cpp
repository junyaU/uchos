#include "fat.hpp"
#include "../asm_utils.h"
#include "../graphics/terminal.hpp"
#include "../memory/page.hpp"
#include "../memory/paging.hpp"
#include "../memory/segment.hpp"
#include "../task/task.hpp"
#include "elf.hpp"
#include "string.h"
#include "types.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace file_system
{
bios_parameter_block* boot_volume_image;
unsigned long bytes_per_cluster;

std::vector<char*> parse_path(const char* path)
{
	std::vector<char*> result;

	if (path == nullptr) {
		return result;
	}

	while (*path != '\0') {
		while (*path == '/') {
			++path;
		}

		if (*path == 0) {
			break;
		}

		result.push_back(const_cast<char*>(path));

		while (*path != '/' && *path != 0) {
			++path;
		}

		if (*path == 0) {
			break;
		}

		*const_cast<char*>(path) = 0;
		++path;
	}

	return result;
}

int make_args(char* command,
			  char* args,
			  char** argv,
			  int argv_len,
			  char* arg_buf,
			  int arg_buf_len)
{
	int argc = 0;
	int arg_buf_index = 0;

	auto push_to_argv = [&](const char* s) {
		if (argc >= argv_len || arg_buf_index >= arg_buf_len) {
			return;
		}

		argv[argc] = &arg_buf[arg_buf_index];
		++argc;
		strncpy(&arg_buf[arg_buf_index], s, arg_buf_len - arg_buf_index);
		arg_buf_index += strlen(s) + 1;
	};

	push_to_argv(command);

	if (args == nullptr) {
		return argc;
	}

	char* p = args;
	while (true) {
		while (*p == ' ') {
			++p;
		}

		if (*p == 0) {
			break;
		}

		const char* arg = p;
		while (*p != ' ' && *p != 0) {
			++p;
		}

		const bool is_end = *p == 0;
		*p = 0;
		push_to_argv(arg);

		if (is_end) {
			break;
		}

		++p;
	}

	return argc;
}

void read_dir_entry_name(const directory_entry& entry, char* dest)
{
	char extension[5] = ".";

	memcpy(dest, &entry.name[0], 8);
	dest[8] = 0;

	for (int i = 7; i >= 0 && dest[i] == 0x20; i--) {
		dest[i] = 0;
	}

	memcpy(extension + 1, &entry.name[8], 3);
	extension[4] = 0;
	for (int i = 2; i >= 0 && extension[i + 1] == 0x20; i--) {
		extension[i + 1] = 0;
	}

	if (extension[1] != 0) {
		strlcat(dest, extension, 13);
	}
}

bool entry_name_is_equal(const directory_entry& entry, const char* name)
{
	char entry_name[13];
	read_dir_entry_name(entry, entry_name);

	return strcmp(entry_name, name) == 0;
}

uintptr_t get_cluster_addr(unsigned long cluster_id)
{
	const unsigned long start_sector_num =
			boot_volume_image->reserved_sector_count +
			boot_volume_image->num_fats * boot_volume_image->fat_size_32 +
			(cluster_id - 2) * boot_volume_image->sectors_per_cluster;

	return reinterpret_cast<uintptr_t>(boot_volume_image) +
		   start_sector_num * boot_volume_image->bytes_per_sector;
}

void initialize_fat(void* volume_image)
{
	boot_volume_image = reinterpret_cast<bios_parameter_block*>(volume_image);
	bytes_per_cluster =
			static_cast<unsigned long>(boot_volume_image->bytes_per_sector) *
			boot_volume_image->sectors_per_cluster;
}

unsigned long next_cluster(unsigned long cluster_id)
{
	const auto fat_offset = boot_volume_image->reserved_sector_count *
							boot_volume_image->bytes_per_sector;

	uint32_t* fat_table = reinterpret_cast<uint32_t*>(
			reinterpret_cast<uintptr_t>(boot_volume_image) + fat_offset);

	const uint32_t next = fat_table[cluster_id];
	if (next >= 0x0FFFFFF8UL) {
		return END_OF_CLUSTERCHAIN;
	}

	return next;
}

directory_entry* find_directory_entry(const char* name, unsigned long cluster_id)
{
	if (cluster_id == 0) {
		cluster_id = boot_volume_image->root_cluster;
	}

	const auto entries_per_cluster = bytes_per_cluster / sizeof(directory_entry);

	while (cluster_id != END_OF_CLUSTERCHAIN) {
		auto* dir_entry = get_sector<directory_entry>(cluster_id);

		for (int i = 0; i < entries_per_cluster; i++) {
			if (dir_entry[i].name[0] == 0x00) {
				return nullptr;
			}

			if (dir_entry[i].name[0] == 0xE5) {
				continue;
			}

			if (dir_entry[i].attribute == entry_attribute::LONG_NAME) {
				continue;
			}

			if (entry_name_is_equal(dir_entry[i], name)) {
				return &dir_entry[i];
			}
		}

		cluster_id = next_cluster(cluster_id);
	}

	return nullptr;
}

directory_entry* find_directory_entry_by_path(const char* path)
{
	auto path_list = parse_path(path);
	auto cluster_id = boot_volume_image->root_cluster;
	auto* entry = get_sector<directory_entry>(cluster_id);

	for (const auto& path_name : path_list) {
		entry = find_directory_entry(path_name, cluster_id);
		if (entry == nullptr) {
			return nullptr;
		}

		cluster_id = entry->first_cluster();
	}

	return entry;
}

std::vector<directory_entry*> list_entries_in_directory(directory_entry* entry)
{
	std::vector<directory_entry*> result;

	auto cluster_id = entry->first_cluster();
	if (entry->attribute == entry_attribute::VOLUME_ID) {
		cluster_id = boot_volume_image->root_cluster;
	}

	const auto entries_per_cluster = bytes_per_cluster / sizeof(directory_entry);

	while (cluster_id != END_OF_CLUSTERCHAIN) {
		auto* dir_entry = get_sector<directory_entry>(cluster_id);

		for (int i = 0; i < entries_per_cluster; ++i) {
			if (dir_entry[i].name[0] == 0x00) {
				return result;
			}

			if (dir_entry[i].name[0] == 0xE5) {
				continue;
			}

			if (dir_entry[i].attribute == entry_attribute::LONG_NAME) {
				continue;
			}

			result.push_back(&dir_entry[i]);
		}

		cluster_id = next_cluster(cluster_id);
	}

	return result;
}

void execute_file(const directory_entry& entry, const char* args)
{
	auto cluster_id = entry.first_cluster();
	auto remaining_bytes = static_cast<unsigned long>(entry.file_size);

	std::vector<uint8_t> file_buffer(remaining_bytes);
	auto* p = file_buffer.data();

	while (cluster_id != END_OF_CLUSTERCHAIN) {
		const auto copy_bytes = std::min(bytes_per_cluster, remaining_bytes);
		memcpy(p, get_sector<uint8_t>(cluster_id), copy_bytes);

		p += copy_bytes;
		remaining_bytes -= copy_bytes;

		cluster_id = next_cluster(cluster_id);
	}

	auto* elf_header = reinterpret_cast<elf64_ehdr_t*>(file_buffer.data());
	if (!is_elf(elf_header)) {
		printk(KERN_ERROR, "Not an ELF file.");
		return;
	}

	char command_name[13];
	read_dir_entry_name(entry, command_name);

	load_elf(elf_header);

	const linear_address argv_addr{ 0xffff'ffff'ffff'f000 };
	setup_page_tables(argv_addr, 1);
	auto* argv = reinterpret_cast<char**>(argv_addr.data);
	const int arg_v_len = 32;
	auto* arg_buf =
			reinterpret_cast<char*>(argv_addr.data + arg_v_len * sizeof(char**));
	const int arg_buf_len = PAGE_SIZE - arg_v_len * sizeof(char**);
	const int argc = make_args(command_name, const_cast<char*>(args), argv,
							   arg_v_len, arg_buf, arg_buf_len);

	const linear_address stack_addr{ 0xffff'ffff'ffff'f000 };
	setup_page_tables(stack_addr, 1);

	task* t = CURRENT_TASK;

	auto entry_addr = elf_header->e_entry;
	call_userland(argc, argv, USER_SS, entry_addr, stack_addr.data + PAGE_SIZE - 8,
				  &t->kernel_stack_top);

	const auto addr_first = get_first_load_addr(elf_header);
	clean_page_tables(linear_address{ addr_first });
}
} // namespace file_system