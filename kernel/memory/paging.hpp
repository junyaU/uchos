/*
 * @file memory/paging.hpp
 *
 * @brief memory paging for x86_64
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

constexpr size_t USER_SPACE_START_INDEX = 256;

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
		uint64_t : 3;
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

page_table_entry* get_pte(page_table_entry* table, vaddr_t addr, int level);

paddr_t get_paddr(page_table_entry* table, vaddr_t addr);

void dump_page_tables(vaddr_t addr);

page_table_entry* new_page_table();

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

vaddr_t map_frame_to_vaddr(page_table_entry* table, uint64_t frame);

void initialize_paging();
