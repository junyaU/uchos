#include "memory/page.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
#include "memory/bootstrap_allocator.hpp"

namespace kernel::memory
{

std::vector<Page> pages;

void initialize_pages()
{
	const size_t memory_end_index = boot_allocator->end_index();

	pages.resize(memory_end_index);

	for (size_t i = 0; i < memory_end_index; ++i) {
		pages[i].set_ptr(reinterpret_cast<void*>(i * PAGE_SIZE));
		if (boot_allocator->is_bit_set(i)) {
			pages[i].set_used();
		} else {
			pages[i].set_free();
		}
	}
}

Page* get_page(void* ptr)
{
	if (ptr == nullptr) {
		return nullptr;
	}

	const size_t index = reinterpret_cast<uintptr_t>(ptr) / PAGE_SIZE;
	if (index >= pages.size()) {
		return nullptr;
	}

	return &pages[index];
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

} // namespace kernel::memory
