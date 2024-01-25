#include "page.hpp"
#include "../graphics/terminal.hpp"
#include "bootstrap_allocator.hpp"

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

void print_available_memory()
{
	size_t available_pages = 0;

	for (const auto& page : pages) {
		if (page.is_free()) {
			available_pages++;
		}
	}

	main_terminal->printf("available memory: %u MiB / %u MiB\n",
						  available_pages * PAGE_SIZE / 1024 / 1024,
						  pages.size() * PAGE_SIZE / 1024 / 1024);
}