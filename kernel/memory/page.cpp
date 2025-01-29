#include "memory/page.hpp"
#include "memory/bootstrap_allocator.hpp"
#include <libs/common/types.hpp>
#include <vector>

std::vector<page> pages;

void initialize_pages()
{
	const size_t memory_start_index = boot_allocator->start_index();
	const size_t memory_end_index = boot_allocator->end_index();

	pages.resize(memory_end_index - memory_start_index);

	for (size_t i = memory_start_index; i < memory_end_index; ++i) {
		const size_t page_index = i - memory_start_index;

		pages[page_index].set_ptr(reinterpret_cast<void*>(i * PAGE_SIZE));
		if (boot_allocator->is_bit_set(i)) {
			pages[page_index].set_used();
		} else {
			pages[page_index].set_free();
		}
	}
}

page* get_page(void* ptr)
{
	if (ptr == nullptr ||
		pages.size() < reinterpret_cast<uintptr_t>(ptr) / PAGE_SIZE) {
		return nullptr;
	}

	return &pages[reinterpret_cast<uintptr_t>(ptr) / PAGE_SIZE];
}

void get_memory_usage(size_t* total_mem, size_t* used_mem)
{
	size_t available_pages = 0;

	for (const auto& page : pages) {
		if (page.is_free()) {
			available_pages++;
		}
	}

	*used_mem = (pages.size() - available_pages) * PAGE_SIZE;
	*total_mem = pages.size() * PAGE_SIZE;
}