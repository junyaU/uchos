#include "buddy_system.hpp"
#include "../bit_utils.hpp"
#include "../graphics/terminal.hpp"
#include "page.hpp"
#include <cstddef>

buddy_system* memory_manager;

int buddy_system::calculate_order(size_t num_pages)
{
	const int order = bit_width_ceil(num_pages);

	if (order > MAX_ORDER) {
		return -1;
	}

	return order;
}

void buddy_system::split_page_block(int order)
{
	auto* page = free_lists_[order].front();
	free_lists_[order].pop_front();

	const int lower_order = order - 1;

	free_lists_[lower_order].push_back(page);
	free_lists_[lower_order].push_back(&pages[page->index() + (1 << (lower_order))]);
}

void* buddy_system::allocate(size_t size)
{
	const int order = calculate_order((size + PAGE_SIZE - 1) / PAGE_SIZE);
	if (order == -1) {
		main_terminal->errorf("invalid size: %d", size);
		return nullptr;
	}

	if (free_lists_[order].empty()) {
		int next_order = -1;
		for (int i = order + 1; i <= MAX_ORDER; i++) {
			if (!free_lists_[i].empty()) {
				next_order = i;
				break;
			}
		}

		if (next_order == -1) {
			main_terminal->errorf("failed to allocate memory: order=%d", order);
			return nullptr;
		}

		for (int i = next_order; i > order; i--) {
			split_page_block(i);
		}
	}

	main_terminal->printf("order: %d\n", order);

	const size_t num_order_pages = 1 << order;

	auto* page = free_lists_[order].front();
	if (page->is_used()) {
		main_terminal->error("page is already used.");
		return nullptr;
	}

	free_lists_[order].pop_front();

	for (size_t i = 0; i < num_order_pages; i++) {
		pages[page->index() + i].set_used();
	}

	return page->ptr();
}

void buddy_system::free(void* addr, size_t size)
{
	auto* start_page = &pages[reinterpret_cast<uintptr_t>(addr) / PAGE_SIZE];
	if (start_page->is_free()) {
		main_terminal->printf("double free detected at address: %p\n", addr);
		return;
	}

	int order = calculate_order((size + PAGE_SIZE - 1) / PAGE_SIZE);
	if (order == -1) {
		main_terminal->printf("invalid size: %d\n", size);
		return;
	}

	while (order <= MAX_ORDER) {
		const size_t num_order_pages = 1 << order;

		if (order == MAX_ORDER) {
			free_lists_[order].push_back(start_page);
			for (size_t i = 0; i < num_order_pages; i++) {
				pages[start_page->index() + i].set_free();
			}
			return;
		}

		auto buddy_addr = reinterpret_cast<uintptr_t>(start_page->ptr()) ^
						  (num_order_pages * PAGE_SIZE);

		page* buddy_start_page = &pages[buddy_addr / PAGE_SIZE];

		auto it = std::find(free_lists_[order].begin(), free_lists_[order].end(),
							buddy_start_page);
		if (it == free_lists_[order].end()) {
			free_lists_[order].push_back(start_page);
			for (size_t i = 0; i < num_order_pages; i++) {
				pages[start_page->index() + i].set_free();
			}
			return;
		}

		free_lists_[order].erase(it);

		start_page = std::min(start_page, buddy_start_page);

		order++;
	}
}

void buddy_system::register_memory(int num_consecutive_pages, page* start_page)
{
	if (num_consecutive_pages <= 0) {
		return;
	}

	while (num_consecutive_pages > 0) {
		const auto order =
				std::min(calculate_order(num_consecutive_pages), MAX_ORDER);

		this->free_lists_[order].push_back(start_page);

		const int num_order_pages = 1 << order;

		start_page = &pages[start_page->index() + num_order_pages];

		num_consecutive_pages -= num_order_pages;
	}
}

void buddy_system::print_free_lists() const
{
	for (int i = 0; i <= MAX_ORDER; i++) {
		main_terminal->printf("order-%d remaining blocks: %d\n", i,
							  free_lists_[i].size());
	}
}

void initialize_memory_manager()
{
	main_terminal->info("initializing memory manager...");

	memory_manager = new buddy_system();

	size_t concecutive_start_index = 0;
	size_t num_consecutive_pages = 0;

	for (size_t i = 0; i < pages.size(); i++) {
		if (pages[i].is_used()) {
			if (num_consecutive_pages > 0) {
				memory_manager->register_memory(num_consecutive_pages,
												&pages[concecutive_start_index]);
			}

			num_consecutive_pages = 0;
			continue;
		}

		if (num_consecutive_pages == 0) {
			concecutive_start_index = i;
		}

		num_consecutive_pages++;
	}

	if (num_consecutive_pages > 0) {
		memory_manager->register_memory(num_consecutive_pages,
										&pages[concecutive_start_index]);
	}

	main_terminal->info("Memory manager initialized successfully.");
}
