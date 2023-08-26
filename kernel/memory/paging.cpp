#include "paging.hpp"

#include <array>

#include "memory.h"

namespace
{
const uint64_t kPageSize4KiB = 4096;
const uint64_t kPageSize2MiB = 512 * kPageSize4KiB;
const uint64_t kPageSize1GiB = 512 * kPageSize2MiB;

const size_t kPageDirectoryCount = 64;

alignas(kPageSize4KiB) std::array<uint64_t, 512> pml4_table;
alignas(kPageSize4KiB) std::array<uint64_t, 512> pdp_table;
alignas(kPageSize4KiB)
		std::array<std::array<uint64_t, 512>, kPageDirectoryCount> page_directory;
} // namespace

void SetupIdentityMapping()
{
	pml4_table[0] = reinterpret_cast<uint64_t>(&pdp_table[0]) | 0x003;
	for (int i_pdpt = 0; i_pdpt < kPageDirectoryCount; i_pdpt++) {
		pdp_table[i_pdpt] =
				reinterpret_cast<uint64_t>(&page_directory[i_pdpt]) | 0x003;
		for (int i_pd = 0; i_pd < 512; i_pd++) {
			page_directory[i_pdpt][i_pd] =
					i_pdpt * kPageSize1GiB + i_pd * kPageSize2MiB | 0x083;
		}
	}
}

void InitializePaging()
{
	SetupIdentityMapping();
	SetCR3(reinterpret_cast<uint64_t>(&pml4_table[0]));
}