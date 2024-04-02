#include "paging.hpp"
#include "../graphics/log.hpp"
#include "../types.hpp"
#include "page.hpp"
#include "paging_utils.h"
#include "slab.hpp"
#include <array>
#include <cstdint>
#include <cstring>

namespace
{
constexpr uint64_t PAGE_2MIB = 512 * PAGE_SIZE;
constexpr uint64_t PAGE_1GIB = 512 * PAGE_2MIB;

constexpr size_t PAGE_DIRECTORY_COUNT = 64;

alignas(PAGE_SIZE) std::array<uint64_t, 512> pml4_table;
alignas(PAGE_SIZE) std::array<uint64_t, 512> pdp_table;
alignas(PAGE_SIZE)
		std::array<std::array<uint64_t, 512>, PAGE_DIRECTORY_COUNT> page_directory;
} // namespace

void setup_identity_mapping()
{
	pml4_table[0] = reinterpret_cast<uint64_t>(&pdp_table) | 0x003;
	for (size_t i_pdpt = 0; i_pdpt < PAGE_DIRECTORY_COUNT; i_pdpt++) {
		pdp_table[i_pdpt] =
				reinterpret_cast<uint64_t>(&page_directory[i_pdpt]) | 0x003;
		for (size_t i_pd = 0; i_pd < 512; i_pd++) {
			page_directory[i_pdpt][i_pd] =
					i_pdpt * PAGE_1GIB + i_pd * PAGE_2MIB | 0x083;
		}
	}
}

void dump_page_table(page_table_entry* table,
					 int page_table_level,
					 linear_address addr)
{
	const auto page_table_index = addr.part(page_table_level);
	auto entry = table[page_table_index];
	if (!entry.bits.present) {
		return;
	}

	if (page_table_level > 1) {
		dump_page_table(entry.get_next_level_table(), page_table_level - 1, addr);
	}

	printk(CURRENT_LOG_LEVEL, "page_table_level=%d, index=%d entry=%p",
		   page_table_level, page_table_index, entry.data);
}

void dump_page_tables(linear_address addr)
{
	printk(CURRENT_LOG_LEVEL, "dest_addr=%p", addr.data);
	auto* pml4 = reinterpret_cast<page_table_entry*>(get_cr3());
	dump_page_table(pml4, 4, addr);
}

page_table_entry* new_page_table()
{
	auto* base_addr = kmalloc(PAGE_SIZE, KMALLOC_ZEROED);
	if (base_addr == nullptr) {
		printk(KERN_ERROR, "Failed to allocate memory for page table.");
		return nullptr;
	}

	return reinterpret_cast<page_table_entry*>(base_addr);
}

page_table_entry* set_new_page_table(page_table_entry& entry)
{
	if (entry.bits.present) {
		return entry.get_next_level_table();
	}

	auto* child_table = new_page_table();
	if (child_table == nullptr) {
		return nullptr;
	}

	entry.set_next_level_table(child_table);
	entry.bits.present = 1;
	entry.bits.user_accessible = 1;

	return child_table;
}

int setup_page_table(page_table_entry* page_table,
					 int page_table_level,
					 linear_address addr,
					 size_t num_pages,
					 bool writable)
{
	while (num_pages > 0) {
		const int page_table_index = addr.part(page_table_level);
		auto* child_table = set_new_page_table(page_table[page_table_index]);
		if (child_table == nullptr) {
			printk(KERN_ERROR, "Failed to setup page table: level=%d",
				   page_table_level);
			return -1;
		}

		if (page_table_level == 1) {
			page_table[page_table_index].bits.writable = writable;
			--num_pages;
		} else {
			page_table[page_table_index].bits.writable = 1;
			const int num_remaining_pages = setup_page_table(
					child_table, page_table_level - 1, addr, num_pages, writable);
			if (num_remaining_pages == -1) {
				printk(KERN_ERROR, "Failed to setup page table: level=%d",
					   page_table_level);
				return -1;
			}

			num_pages = num_remaining_pages;
		}

		if (page_table_index == 511) {
			break;
		}

		addr.set_part(page_table_level, page_table_index + 1);
		for (int level = page_table_level - 1; level >= 1; --level) {
			addr.set_part(level, 0);
		}
	}

	return num_pages;
}

void setup_page_tables(linear_address addr, size_t num_pages, bool writable)
{
	const int num_remaining_pages =
			setup_page_table(reinterpret_cast<page_table_entry*>(get_cr3()), 4, addr,
							 num_pages, writable);
	if (num_remaining_pages == -1) {
		printk(KERN_ERROR, "Failed to setup page tables.");
		return;
	}

	for (size_t i = 0; i < num_pages; i++) {
		flush_tlb(addr.data + i * PAGE_SIZE);
	}
}

void clean_page_table(page_table_entry* table, int page_table_level)
{
	for (int i = 0; i < 512; i++) {
		auto entry = table[i];
		if (!entry.bits.present) {
			continue;
		}

		if (page_table_level > 1) {
			clean_page_table(entry.get_next_level_table(), page_table_level - 1);
		}

		kfree(entry.get_next_level_table());
		table[i].data = 0;
	}
}

void clean_page_tables(linear_address addr)
{
	auto* pml4 = reinterpret_cast<page_table_entry*>(get_cr3());
	auto* pdpt = pml4[addr.part(4)].get_next_level_table();
	pml4[addr.part(4)].data = 0;

	clean_page_table(pdpt, 3);
	kfree(pdpt);
}

void copy_page_tables(page_table_entry* dst,
					  page_table_entry* src,
					  int level,
					  bool writable,
					  int start_index)
{
	if (level == 1) {
		for (int i = start_index; i < 512; ++i) {
			if (src[i].bits.present) {
				dst[i] = src[i];
				dst[i].bits.writable = writable;
			}
		}
	} else {
		for (int i = start_index; i < 512; ++i) {
			if (src[i].bits.present) {
				auto* new_table = new_page_table();
				dst[i] = src[i];
				dst[i].set_next_level_table(new_table);
				copy_page_tables(new_table, src[i].get_next_level_table(), level - 1,
								 writable, 0);
			}
		}
	}
}

void set_page_table_entry(page_table_entry* table,
						  linear_address addr,
						  page_table_entry* entry)
{
	auto* pdpt = table[addr.part(4)].get_next_level_table();
	auto* pd = pdpt[addr.part(3)].get_next_level_table();
	auto* pt = pd[addr.part(2)].get_next_level_table();
	const size_t pt_index = addr.part(1);
	pt[pt_index].set_next_level_table(entry);
	pt[pt_index].bits.writable = 1;
	pt[pt_index].bits.present = 1;

	flush_tlb(addr.data);
}

error_t copy_target_page(uint64_t addr)
{
	auto* page = new_page_table();
	if (page == nullptr) {
		printk(KERN_ERROR, "Failed to allocate memory for page table.");
		return ERR_NO_MEMORY;
	}

	const auto aligned_addr = addr & ~0xfff;
	memcpy(page, reinterpret_cast<void*>(aligned_addr), PAGE_SIZE);

	set_page_table_entry(reinterpret_cast<page_table_entry*>(get_cr3()),
						 linear_address{ addr }, page);

	return OK;
}

void copy_kernel_space(page_table_entry* dst)
{
	auto* src = reinterpret_cast<page_table_entry*>(get_cr3());
	memcpy(dst, src, 256 * sizeof(page_table_entry));
}

page_table_entry* clone_page_table(page_table_entry* src, bool writable)
{
	auto* table = new_page_table();
	if (table == nullptr) {
		printk(KERN_ERROR, "Failed to allocate memory for page table.");
		return nullptr;
	}

	copy_kernel_space(table);
	copy_page_tables(table, src, 4, writable, USER_SPACE_START_INDEX);

	return table;
}

error_t handle_page_fault(uint64_t error_code, uint64_t fault_addr)
{
	auto exist = error_code & 1;
	auto rw = (error_code >> 1) & 1;
	auto user = (error_code >> 2) & 1;

	if ((user != 0) && (rw != 0) && (exist != 0)) {
		error_t err = copy_target_page(fault_addr);
		if (IS_ERR(err)) {
			printk(KERN_ERROR, "Failed to handle page fault: %d", err);
			return err;
		}
	} else {
		printk(KERN_ERROR, "Page fault: user=%d, rw=%d, exist=%d", user, rw, exist);
		return ERR_PAGE_NOT_PRESENT;
	}

	return OK;
}

void initialize_paging()
{
	printk(KERN_INFO, "Initializing paging...");
	setup_identity_mapping();
	set_cr3(reinterpret_cast<uint64_t>(&pml4_table));
	printk(KERN_INFO, "Paging initialized successfully.");
}
