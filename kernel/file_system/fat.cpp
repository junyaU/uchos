#include "fat.hpp"
#include "../elf.hpp"
#include "../graphics/font.hpp"
#include <../../libs/common/types.hpp>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string.h>
#include <utility>
#include <vector>

namespace file_system
{
bios_parameter_block* BOOT_VOLUME_IMAGE;
unsigned long BYTES_PER_CLUSTER;
uint32_t* FAT_TABLE;

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

void register_file_name(const char* name, directory_entry* entry)
{
	memset(entry->name, 0x20, 11);

	const char* dot = strchr(name, '.');
	if (dot == nullptr) {
		for (int i = 0; i < 8 && name[i] != 0; ++i) {
			entry->name[i] = toupper(name[i]);
		}
		return;
	}

	for (int i = 0; i < 8 && i < dot - name; ++i) {
		entry->name[i] = toupper(name[i]);
	}

	for (int i = 0; i < 3 && dot[i + 1] != 0; ++i) {
		entry->name[8 + i] = toupper(dot[i + 1]);
	}
}

bool entry_name_is_equal(const directory_entry& entry, const char* name)
{
	char entry_name[13];
	read_dir_entry_name(entry, entry_name);

	return strcmp(entry_name, name) == 0;
}

uintptr_t get_cluster_addr(cluster_t cluster_id)
{
	const unsigned long start_sector_num =
			BOOT_VOLUME_IMAGE->reserved_sector_count +
			BOOT_VOLUME_IMAGE->num_fats * BOOT_VOLUME_IMAGE->fat_size_32 +
			(cluster_id - 2) * BOOT_VOLUME_IMAGE->sectors_per_cluster;

	return reinterpret_cast<uintptr_t>(BOOT_VOLUME_IMAGE) +
		   start_sector_num * BOOT_VOLUME_IMAGE->bytes_per_sector;
}

void initialize_fat(void* volume_image)
{
	BOOT_VOLUME_IMAGE = reinterpret_cast<bios_parameter_block*>(volume_image);
	BYTES_PER_CLUSTER =
			static_cast<unsigned long>(BOOT_VOLUME_IMAGE->bytes_per_sector) *
			BOOT_VOLUME_IMAGE->sectors_per_cluster;

	const auto fat_offset = BOOT_VOLUME_IMAGE->reserved_sector_count *
							BOOT_VOLUME_IMAGE->bytes_per_sector;

	FAT_TABLE = reinterpret_cast<uint32_t*>(
			reinterpret_cast<uintptr_t>(BOOT_VOLUME_IMAGE) + fat_offset);
}

cluster_t next_cluster(cluster_t cluster_id)
{
	const uint32_t next = FAT_TABLE[cluster_id];
	if (next >= 0x0FFFFFF8UL) {
		return END_OF_CLUSTER_CHAIN;
	}

	return next;
}

directory_entry* find_directory_entry(const char* name, cluster_t cluster_id)
{
	if (cluster_id == 0) {
		cluster_id = BOOT_VOLUME_IMAGE->root_cluster;
	}

	const auto entries_per_cluster = BYTES_PER_CLUSTER / sizeof(directory_entry);

	while (cluster_id != END_OF_CLUSTER_CHAIN) {
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
	const size_t path_len = strlen(path);
	char path_copy[path_len + 1];
	memset(path_copy, 0, sizeof(path_copy));
	if (path != nullptr) {
		memcpy(path_copy, path, path_len + 1);
	}

	to_upper(path_copy);

	auto path_list = parse_path(path_copy);
	auto cluster_id = BOOT_VOLUME_IMAGE->root_cluster;
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
		cluster_id = BOOT_VOLUME_IMAGE->root_cluster;
	}

	const auto entries_per_cluster = BYTES_PER_CLUSTER / sizeof(directory_entry);

	while (cluster_id != END_OF_CLUSTER_CHAIN) {
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
	std::vector<uint8_t> file_buffer(entry.file_size);
	load_file(file_buffer.data(), entry.file_size,
			  const_cast<directory_entry&>(entry));

	char command_name[13];
	read_dir_entry_name(entry, command_name);

	exec_elf(file_buffer.data(), command_name, args);
}

cluster_t extend_cluster_chain(cluster_t last_cluster, int num_clusters)
{
	cluster_t current_cluster = last_cluster;
	size_t num_allocated = 0;
	for (int i = 2; num_allocated < num_clusters; ++i) {
		if (FAT_TABLE[i] != 0) {
			continue;
		}

		FAT_TABLE[current_cluster] = i;
		current_cluster = i;
		++num_allocated;
	}

	FAT_TABLE[current_cluster] = END_OF_CLUSTER_CHAIN;

	return current_cluster;
}

directory_entry* allocate_directory_entry(cluster_t cluster_id)
{
	while (true) {
		auto* dir_entry = get_sector<directory_entry>(cluster_id);
		for (int i = 0; i < BYTES_PER_CLUSTER / sizeof(directory_entry); ++i) {
			if (dir_entry[i].name[0] == 0x00 || dir_entry[i].name[0] == 0xE5) {
				return &dir_entry[i];
			}
		}

		if (next_cluster(cluster_id) == END_OF_CLUSTER_CHAIN) {
			break;
		}

		cluster_id = next_cluster(cluster_id);
	}

	const cluster_t new_cluster = extend_cluster_chain(cluster_id, 1);
	if (new_cluster == cluster_id) {
		return nullptr;
	}

	auto* dir_entry = get_sector<directory_entry>(new_cluster);
	memset(dir_entry, 0, BYTES_PER_CLUSTER);

	return &dir_entry[0];
}

std::pair<const char*, const char*> split_path(char* path)
{
	const char* last_slash = strrchr(path, '/');
	if (last_slash == nullptr) {
		return { nullptr, path };
	}

	if (last_slash == path) {
		return { nullptr, last_slash + 1 };
	}

	if (last_slash[1] == '\0') {
		return { nullptr, nullptr };
	}

	path[last_slash - path] = '\0';

	return { path, last_slash + 1 };
}

directory_entry* create_file(const char* path)
{
	const size_t path_len = strlen(path);
	char file_path[path_len + 1];
	memcpy(file_path, path, path_len + 1);
	auto [parent_path, file_name] = split_path(file_path);

	cluster_t parent_cluster_id = BOOT_VOLUME_IMAGE->root_cluster;
	if (parent_path != nullptr) {
		auto* parent_entry = find_directory_entry_by_path(parent_path);
		if (parent_entry == nullptr) {
			return nullptr;
		}

		parent_cluster_id = parent_entry->first_cluster();
	}

	directory_entry* new_entry = allocate_directory_entry(parent_cluster_id);
	if (new_entry == nullptr) {
		return nullptr;
	}

	new_entry->file_size = 0;
	register_file_name(file_name, new_entry);

	return new_entry;
}

cluster_t allocate_cluster_chain(size_t num_clusters)
{
	cluster_t first_cluster = 2;
	while (FAT_TABLE[first_cluster] != 0) {
		++first_cluster;
	}
	FAT_TABLE[first_cluster] = END_OF_CLUSTER_CHAIN;

	if (num_clusters > 1) {
		extend_cluster_chain(first_cluster, num_clusters - 1);
	}

	return first_cluster;
}

size_t file_descriptor::read(void* buf, size_t len)
{
	uint8_t* p = reinterpret_cast<uint8_t*>(buf);
	len = std::min(len, entry.file_size - current_file_offset);

	size_t total_read = 0;
	while (total_read < len) {
		uint8_t* sector = get_sector<uint8_t>(current_cluster);
		const size_t cluster_remain = BYTES_PER_CLUSTER - current_cluster_offset;
		const size_t read_len = std::min(len - total_read, cluster_remain);

		memcpy(&p[total_read], &sector[current_cluster_offset], read_len);
		total_read += read_len;
		current_cluster_offset += read_len;

		if (current_cluster_offset == BYTES_PER_CLUSTER) {
			current_cluster = next_cluster(current_cluster);
			current_cluster_offset = 0;
		}
	}

	current_file_offset += total_read;
	return total_read;
}

size_t file_descriptor::write(const void* buf, size_t len)
{
	auto num_clusters = [](size_t size) {
		return (size + BYTES_PER_CLUSTER - 1) / BYTES_PER_CLUSTER;
	};

	if (write_cluster == 0) {
		write_cluster = allocate_cluster_chain(num_clusters(len));
		entry.first_cluster_low = write_cluster & 0xFFFF;
		entry.first_cluster_high = (write_cluster >> 16) & 0xFFFF;
	}

	const uint8_t* p = reinterpret_cast<const uint8_t*>(buf);

	size_t total_written = 0;
	while (total_written < len) {
		if (write_cluster_offset == BYTES_PER_CLUSTER) {
			const cluster_t next = next_cluster(write_cluster);
			write_cluster =
					next == END_OF_CLUSTER_CHAIN
							? extend_cluster_chain(write_cluster,
												   num_clusters(len - total_written))
							: next;
			write_cluster_offset = 0;
		}

		uint8_t* sector = get_sector<uint8_t>(write_cluster);
		const size_t cluster_remain = BYTES_PER_CLUSTER - write_cluster_offset;
		const size_t write_len = std::min(len, cluster_remain);
		memcpy(&sector[write_cluster_offset], &p[total_written], write_len);

		total_written += write_len;
		write_cluster_offset += write_len;
	}

	file_written_bytes += total_written;
	entry.file_size = file_written_bytes;

	return total_written;
}

size_t load_file(void* buf, size_t len, directory_entry& entry)
{
	return file_descriptor(entry).read(buf, len);
}

} // namespace file_system
