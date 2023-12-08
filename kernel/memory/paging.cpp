#include "paging.hpp"
#include "../graphics/terminal.hpp"
#include "page_operations.h"
#include <array>

namespace
{
const uint64_t PAGE_4KIB = 4096;
const uint64_t PAGE_2MIB = 512 * PAGE_4KIB;
const uint64_t PAGE_1GIB = 512 * PAGE_2MIB;

const size_t PAGE_DIRECTORY_COUNT = 64;

alignas(PAGE_4KIB) std::array<uint64_t, 512> pml4_table;
alignas(PAGE_4KIB) std::array<uint64_t, 512> pdp_table;
alignas(PAGE_4KIB)
		std::array<std::array<uint64_t, 512>, PAGE_DIRECTORY_COUNT> page_directory;
} // namespace

void setup_identity_mapping()
{
	pml4_table[0] = reinterpret_cast<uint64_t>(&pdp_table) | 0x003;
	for (int i_pdpt = 0; i_pdpt < PAGE_DIRECTORY_COUNT; i_pdpt++) {
		pdp_table[i_pdpt] =
				reinterpret_cast<uint64_t>(&page_directory[i_pdpt]) | 0x003;
		for (int i_pd = 0; i_pd < 512; i_pd++) {
			page_directory[i_pdpt][i_pd] =
					i_pdpt * PAGE_1GIB + i_pd * PAGE_2MIB | 0x083;
		}
	}
}

void initialize_paging()
{
	main_terminal->info("Initializing paging...");
	setup_identity_mapping();
	set_cr3(reinterpret_cast<uint64_t>(&pml4_table));
	main_terminal->info("Paging initialized successfully.");
}