#include "page.hpp"
#include "bootstrap_allocator.hpp"
#include "graphics/system_logger.hpp"

#include <array>
#include <vector>

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

void print_available_memory()
{
	size_t available_pages = 0;

	for (const auto& page : pages) {
		if (page.is_free()) {
			available_pages++;
		}
	}

	system_logger->Printf("available memory: %u MiB / %u MiB\n",
						  available_pages * PAGE_SIZE / 1024 / 1024,
						  pages.size() * PAGE_SIZE / 1024 / 1024);
}