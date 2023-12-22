#include "fat.hpp"
#include "elf.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

namespace file_system
{
bios_parameter_block* boot_volume_image;
unsigned long bytes_per_cluster;

std::vector<char*> make_args(char* command, char* args)
{
	std::vector<char*> argv;
	argv.push_back(command);

	char* p = args;
	while (true) {
		while (*p == ' ') {
			++p;
		}

		if (*p == '\0') {
			break;
		}

		argv.push_back(p);

		while (*p != ' ' && *p != '\0') {
			++p;
		}

		if (*p == '\0') {
			break;
		}

		*p = '\0';
		++p;
	}

	return argv;
}

void read_dir_entry_name(const directory_entry& entry, char* dest)
{
	char extention[5] = ".";

	memcpy(dest, &entry.name[0], 8);
	dest[8] = 0;

	for (int i = 7; i >= 0 && dest[i] == 0x20; i--) {
		dest[i] = 0;
	}

	memcpy(extention + 1, &entry.name[8], 3);
	extention[4] = 0;
	for (int i = 2; i >= 0 && extention[i + 1] == 0x20; i--) {
		extention[i + 1] = 0;
	}

	if (extention[1] != 0) {
		strlcat(dest, extention, 13);
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

int execute_file(const directory_entry& entry, const char* args)
{
	auto cluster_id = entry.first_cluster();
	auto remain_bytes = static_cast<unsigned long>(entry.file_size);

	std::vector<uint8_t> file_buffer(remain_bytes);
	auto* p = file_buffer.data();

	while (cluster_id != END_OF_CLUSTERCHAIN) {
		const auto copy_bytes = std::min(bytes_per_cluster, remain_bytes);
		memcpy(p, get_sector<uint8_t>(cluster_id), copy_bytes);

		p += copy_bytes;
		remain_bytes -= copy_bytes;

		cluster_id = next_cluster(cluster_id);
	}

	auto* elf_header = reinterpret_cast<elf64_ehdr_t*>(file_buffer.data());
	if (memcmp(elf_header->e_ident,
			   "\x7f"
			   "ELF",
			   4) != 0) {
		using func_t = void (*)();
		auto f = reinterpret_cast<func_t>(file_buffer.data());
		f();
		return 0;
	}

	char* command_name = nullptr;
	read_dir_entry_name(entry, command_name);
	auto argv = make_args(command_name, const_cast<char*>(args));

	auto entry_addr = elf_header->e_entry;
	entry_addr += reinterpret_cast<uintptr_t>(file_buffer.data());
	using func_t = int (*)(int, char**);
	auto f = reinterpret_cast<func_t>(entry_addr);
	return f(argv.size(), argv.data());
}
} // namespace file_system