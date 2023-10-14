#include "page.hpp"
#include "bootstrap_allocator.hpp"
#include "graphics/system_logger.hpp"

std::vector<page> pages;

void initialize_pages()
{
	const size_t memory_start_index = boot_allocator->start_index();
	const size_t memory_end_index = boot_allocator->end_index();

	pages.resize(memory_end_index - memory_start_index);

	for (size_t i = memory_start_index; i < memory_end_index; i++) {
		size_t page_index = i - memory_start_index;

		pages[page_index].set_ptr(reinterpret_cast<void*>(i * PAGE_SIZE));
		if (boot_allocator->is_bit_set(i)) {
			pages[page_index].set_used();
		} else {
			pages[page_index].set_free();
		}
	}
}