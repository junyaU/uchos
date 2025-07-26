#include "memory/paging.hpp"
#include "graphics/log.hpp"
#include "memory/page.hpp"
#include "memory/slab.hpp"
#include "paging_utils.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <libs/common/types.hpp>

namespace kernel::memory {

namespace
{
constexpr uint64_t PAGE_2MIB = 512 * PAGE_SIZE;
constexpr uint64_t PAGE_1GIB = 512 * PAGE_2MIB;

constexpr size_t PAGE_DIRECTORY_COUNT = 64;

alignas(PAGE_SIZE) std::array<uint64_t, 512> pml4_table;
alignas(PAGE_SIZE) std::array<uint64_t, 512> pdp_table;
alignas(PAGE_SIZE)
		std::array<std::array<uint64_t, 512>, PAGE_DIRECTORY_COUNT> page_directory;

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
} // anonymous namespace

page_table_entry* get_active_page_table()
{
	return reinterpret_cast<page_table_entry*>(get_cr3());
}

page_table_entry* get_pte(page_table_entry* table, vaddr_t addr, int level)
{
	auto* next_table = table;
	for (int i = 4; i > level; --i) {
		const int index = addr.part(i);
		auto entry = next_table[index];
		if (!entry.bits.present) {
			return nullptr;
		}

		next_table = entry.get_next_level_table();
	}

	return &next_table[addr.part(level)];
}

paddr_t get_paddr(page_table_entry* table, vaddr_t addr)
{
	auto* pte = get_pte(table, addr, 1);
	if (pte == nullptr) {
		return paddr_t{ 0 };
	}

	return paddr_t{ pte->bits.address << 12 };
}

void dump_page_table(page_table_entry* table, int page_table_level, vaddr_t addr)
{
	const auto page_table_index = addr.part(page_table_level);
	auto entry = table[page_table_index];
	if (!entry.bits.present) {
		return;
	}

	if (page_table_level > 1) {
		dump_page_table(entry.get_next_level_table(), page_table_level - 1, addr);
	}

	LOG_ERROR("page_table_level=%d, index=%d entry=%p", page_table_level,
			  page_table_index, entry.data);
}

void dump_page_tables(vaddr_t addr)
{
	LOG_ERROR("dest_addr=%p", addr.data);
	dump_page_table(get_active_page_table(), 4, addr);
}

page_table_entry* new_page_table()
{
	void* addr;
	ALLOC_OR_RETURN_NULL(addr, PAGE_SIZE, ALLOC_ZEROED);

	return reinterpret_cast<page_table_entry*>(addr);
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
					 vaddr_t addr,
					 size_t num_pages,
					 bool writable)
{
	while (num_pages > 0) {
		const int page_table_index = addr.part(page_table_level);
		auto* child_table = set_new_page_table(page_table[page_table_index]);
		if (child_table == nullptr) {
			LOG_ERROR("Failed to setup page table: level=%d", page_table_level);
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
				LOG_ERROR("Failed to setup page table: level=%d", page_table_level);
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

void setup_page_tables(vaddr_t addr, size_t num_pages, bool writable)
{
	const int num_remaining_pages =
			setup_page_table(get_active_page_table(), 4, addr, num_pages, writable);
	if (num_remaining_pages == -1) {
		LOG_ERROR("Failed to setup page tables.");
		return;
	}

	for (size_t i = 0; i < num_pages; i++) {
		flush_tlb(addr.data + i * kernel::memory::PAGE_SIZE);
	}
}

page_table_entry* config_new_page_table()
{
	page_table_entry* page_table = new_page_table();
	if (page_table == nullptr) {
		return nullptr;
	}

	copy_kernel_space(page_table);
	set_cr3(reinterpret_cast<uint64_t>(page_table));

	return page_table;
}

void clean_page_table(page_table_entry* table, int page_table_level)
{
	for (int i = 0; i < 512; ++i) {
		auto entry = table[i];
		if (!entry.bits.present) {
			continue;
		}

		if (page_table_level > 1) {
			clean_page_table(entry.get_next_level_table(), page_table_level - 1);
		}

		// skip CoW pages
		if (!table[i].bits.writable && page_table_level == 1) {
			continue;
		}

		kernel::memory::free(entry.get_next_level_table());
		table[i].data = 0;
	}
}

void clean_page_tables(page_table_entry* table)
{
	for (int i = USER_SPACE_START_INDEX; i < 512; ++i) {
		auto entry = table[i];
		if (!entry.bits.present) {
			continue;
		}

		clean_page_table(entry.get_next_level_table(), 3);
		kernel::memory::free(entry.get_next_level_table());
		table[i].data = 0;
	}

	kernel::memory::free(table);
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
						  vaddr_t addr,
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
		LOG_ERROR("Failed to allocate memory for page table.");
		return ERR_NO_MEMORY;
	}

	const auto aligned_addr = addr & ~0xfff;
	memcpy(page, reinterpret_cast<void*>(aligned_addr), kernel::memory::PAGE_SIZE);

	set_page_table_entry(get_active_page_table(), vaddr_t{ addr }, page);

	return OK;
}

void copy_kernel_space(page_table_entry* dst)
{
	memcpy(dst, get_active_page_table(), 256 * sizeof(page_table_entry));
}

page_table_entry* clone_page_table(page_table_entry* src, bool writable)
{
	auto* table = new_page_table();
	if (table == nullptr) {
		LOG_ERROR("Failed to allocate memory for page table.");
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
		ASSERT_OK(copy_target_page(fault_addr));
	} else {
		LOG_ERROR("Page fault: user=%d, rw=%d, exist=%d", user, rw, exist);
		return ERR_PAGE_NOT_PRESENT;
	}

	return OK;
}

vaddr_t create_vaddr_from_index(int pml4_i, int pdpt_i, int pd_i, int pt_i)
{
	vaddr_t addr;
	addr.data = 0;

	addr.set_part(4, pml4_i & 0x1FF);
	addr.set_part(3, pdpt_i & 0x1FF);
	addr.set_part(2, pd_i & 0x1FF);
	addr.set_part(1, pt_i & 0x1FF);

	if ((addr.bits.page_map_level_4_index & 0x100) != 0) {
		addr.bits.canonical = 0xFFFF;
	} else {
		addr.bits.canonical = 0x0000;
	}

	return addr;
}

error_t map_frame_to_vaddr(page_table_entry* table,
						   uint64_t frame,
						   size_t num_pages,
						   vaddr_t* start_addr)
{
	int indices[] = { 0, 0, 0, 0 };
	size_t consecutive_pages = 0;

	for (int level = 4; level >= 1; --level) {
		const int start = (level == 4) ? USER_SPACE_START_INDEX : 0;
		for (int i = start; i < PT_ENTRIES; ++i) {
			if (level == 1) {
				for (size_t j = 0; j < num_pages; ++j) {
					if (!table[i + j].bits.present) {
						++consecutive_pages;
					} else {
						consecutive_pages = 0;
						break;
					}
				}

				if (consecutive_pages < num_pages) {
					continue;
				}

				for (size_t j = 0; j < num_pages; ++j) {
					table[i + j].bits.present = 1;
					table[i + j].bits.writable = 0;
					table[i + j].bits.user_accessible = 1;
					table[i + j].bits.address = (frame + j * kernel::memory::PAGE_SIZE) >> 12;

					indices[4 - level] = i;

					const vaddr_t addr = create_vaddr_from_index(indices[0], indices[1],
														   indices[2], indices[3]);

					if (j == 0) {
						*start_addr = addr;
					}

					flush_tlb(addr.data);
				}
			} else {
				if (!table[i].bits.present) {
					continue;
				}

				indices[4 - level] = i;
				table = table[i].get_next_level_table();
				break;
			}

			return OK;
		}
	}

	return ERR_NO_MEMORY;
}

error_t unmap_frame(page_table_entry* table, vaddr_t addr, size_t num_pages)
{
	for (size_t i = 0; i < num_pages; i++) {
		const vaddr_t target_addr{ addr.data + i * kernel::memory::PAGE_SIZE };
		auto* pte = get_pte(table, target_addr, 1);
		if (pte == nullptr) {
			return ERR_INVALID_ARG;
		}

		pte->data = 0;
		flush_tlb(target_addr.data);
	}

	return OK;
}

size_t calc_required_pages(vaddr_t start, size_t size)
{
	const vaddr_t end{ start.data + size };
	const vaddr_t start_page{ start.data - start.part(0) };
	const vaddr_t end_page{ end.data - end.part(0) + kernel::memory::PAGE_SIZE };
	return (end_page.data - start_page.data) / kernel::memory::PAGE_SIZE;
}

void initialize_paging()
{
	LOG_INFO("Initializing paging...");
	setup_identity_mapping();
	set_cr3(reinterpret_cast<uint64_t>(&pml4_table));
	LOG_INFO("Paging initialized successfully.");
}

} // namespace kernel::memory
