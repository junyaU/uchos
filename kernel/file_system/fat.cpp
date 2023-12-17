#include "fat.hpp"
#include <cstdint>

namespace file_system
{
bios_parameter_block* boot_volume_image;
unsigned long bytes_per_cluster;

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
} // namespace file_system