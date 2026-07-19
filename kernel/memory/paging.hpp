/*
 * @file memory/paging.hpp
 *
 * @brief memory paging for x86_64
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

namespace kernel::memory
{

constexpr size_t USER_SPACE_START_INDEX = 256;
constexpr size_t PT_ENTRIES = 512;

/// Mask for a single 9-bit page-table level index (PT_ENTRIES - 1)
constexpr int PT_INDEX_MASK = static_cast<int>(PT_ENTRIES) - 1;

union vaddr_t {
	uint64_t data;

	struct {
		uint64_t offset : 12;
		uint64_t page_table_index : 9;
		uint64_t page_directory_index : 9;
		uint64_t page_directory_pointer_table_index : 9;
		uint64_t page_map_level_4_index : 9;
		uint64_t canonical : 16;
	} __attribute__((packed)) bits;

	int part(int page_table_level) const
	{
		switch (page_table_level) {
			case 0:
				return bits.offset;
			case 1:
				return bits.page_table_index;
			case 2:
				return bits.page_directory_index;
			case 3:
				return bits.page_directory_pointer_table_index;
			case 4:
				return bits.page_map_level_4_index;
			default:
				return 0;
		}
	}

	void set_part(int page_table_level, int value)
	{
		switch (page_table_level) {
			case 0:
				bits.offset = value;
				break;
			case 1:
				bits.page_table_index = value;
				break;
			case 2:
				bits.page_directory_index = value;
				break;
			case 3:
				bits.page_directory_pointer_table_index = value;
				break;
			case 4:
				bits.page_map_level_4_index = value;
				break;
			default:
				break;
		}
	}
};

/// Present bit (bit 0): entry maps to a valid frame/table
constexpr uint64_t PTE_PRESENT = 1ULL << 0;
/// Writable bit (bit 1): entry permits writes
constexpr uint64_t PTE_WRITABLE = 1ULL << 1;
/// Huge-page bit (bit 7): leaf maps a large page instead of a sub-table
constexpr uint64_t PTE_HUGE = 1ULL << 7;

/// Flags for a kernel identity-mapped entry (present + writable)
constexpr uint64_t PTE_KERNEL_FLAGS = PTE_PRESENT | PTE_WRITABLE;
/// Flags for a kernel identity-mapped huge-page entry
constexpr uint64_t PTE_KERNEL_HUGE_FLAGS = PTE_KERNEL_FLAGS | PTE_HUGE;

union page_table_entry {
	uint64_t data;

	struct {
		uint64_t present : 1;
		uint64_t writable : 1;
		uint64_t user_accessible : 1;
		uint64_t write_through : 1;
		uint64_t cache_disabled : 1;
		uint64_t accessed : 1;
		uint64_t dirty : 1;
		uint64_t huge_page : 1;
		uint64_t global : 1;
		uint64_t owned : 1; ///< OS bit: leaf page is owned (slab-backed user
							///< page freed via ref count on clean)
		uint64_t : 2;
		uint64_t address : 40;
		uint64_t : 11;
		uint64_t no_execute : 1;
	} __attribute__((packed)) bits;

	page_table_entry* get_next_level_table() const
	{
		return reinterpret_cast<page_table_entry*>(bits.address << 12);
	}

	void set_next_level_table(page_table_entry* table)
	{
		bits.address = reinterpret_cast<uint64_t>(table) >> 12;
	}
};

page_table_entry* get_active_page_table();

page_table_entry* get_pte(page_table_entry* table, vaddr_t addr, int level);

paddr_t get_paddr(page_table_entry* table, vaddr_t addr);

void dump_page_tables(vaddr_t addr);

page_table_entry* new_page_table();

/**
 * @brief Map num_pages of newly allocated user memory at addr in a table
 * @param page_table Root (PML4) table to build the mapping in
 * @param page_table_level Table level of page_table (4 for a root table)
 * @param addr Start virtual address
 * @param num_pages Number of pages to map
 * @param writable Whether the leaf mappings are writable
 * @return Number of pages left unmapped (0 on success), or -1 on failure
 */
int setup_page_table(page_table_entry* page_table,
					 int page_table_level,
					 vaddr_t addr,
					 size_t num_pages,
					 bool writable);

void setup_page_tables(vaddr_t addr, size_t num_pages, bool writable);

page_table_entry* config_new_page_table();

void clean_page_tables(page_table_entry* table);

void copy_page_tables(page_table_entry* dst,
					  page_table_entry* src,
					  int level,
					  bool writable,
					  int start_index);

void copy_kernel_space(page_table_entry* dst);

page_table_entry* clone_page_table(page_table_entry* src, bool writable);

error_t handle_page_fault(uint64_t error_code, uint64_t fault_addr);

error_t map_frame_to_vaddr(page_table_entry* table,
						   uint64_t frame,
						   size_t num_pages,
						   vaddr_t* start_addr);

error_t unmap_frame(page_table_entry* table, vaddr_t addr, size_t num_pages);

size_t calc_required_pages(vaddr_t start, size_t size);

void initialize_paging();

} // namespace kernel::memory
