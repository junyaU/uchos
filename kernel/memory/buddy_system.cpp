#include "buddy_system.hpp"
#include "bit_utils.hpp"
#include "graphics/log.hpp"
#include "memory/page.hpp"
#include "tests/framework.hpp"
#include "tests/test_cases/memory_test.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace kernel::memory {

BuddySystem* memory_manager;

size_t BuddySystem::calculate_order(size_t num_pages)
{
	const size_t order = bit_width_ceil(num_pages);

	if (order > MAX_ORDER) {
		return MAX_ORDER;
	}

	return order;
}

void BuddySystem::split_memory_block(int order)
{
	auto* block = free_lists_[order].front();
	free_lists_[order].pop_front();

	const int lower_order = order - 1;
	const auto block_size = (1 << lower_order) * kernel::memory::PAGE_SIZE;

	auto buddy_addr = reinterpret_cast<uintptr_t>(block->ptr()) ^ block_size;
	auto* buddy_block = get_page(reinterpret_cast<void*>(buddy_addr));

	free_lists_[lower_order].push_back(block);
	free_lists_[lower_order].push_back(buddy_block);
}

void* BuddySystem::allocate(size_t size)
{
	const int order = calculate_order((size + kernel::memory::PAGE_SIZE - 1) / kernel::memory::PAGE_SIZE);
	if (order == -1) {
		LOG_ERROR("invalid size: %d", size);
		return nullptr;
	}

	if (free_lists_[order].empty()) {
		int next_order = -1;
		for (int i = order + 1; i <= MAX_ORDER; ++i) {
			if (!free_lists_[i].empty()) {
				next_order = i;
				break;
			}
		}

		if (next_order == -1) {
			return nullptr;
		}

		for (int i = next_order; i > order; --i) {
			split_memory_block(i);
		}
	}

	const size_t num_order_pages = 1 << order;

	auto* page = free_lists_[order].front();
	if (page->is_used()) {
		LOG_ERROR("order-%d : page is already used: %p", order, page->ptr());

		return nullptr;
	}

	free_lists_[order].pop_front();

	for (size_t i = 0; i < num_order_pages; ++i) {
		pages[page->index() + i].set_used();
	}

	return page->ptr();
}

void BuddySystem::free(void* addr, size_t size)
{
	auto* start_page = &pages[reinterpret_cast<uintptr_t>(addr) / kernel::memory::PAGE_SIZE];
	if (start_page->is_free()) {
		LOG_ERROR("double free detected at address: %p", addr);
		return;
	}

	int order = calculate_order((size + kernel::memory::PAGE_SIZE - 1) / kernel::memory::PAGE_SIZE);
	if (order == -1) {
		LOG_ERROR("invalid size: %d", size);
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
						  (num_order_pages * kernel::memory::PAGE_SIZE);

		Page* buddy_block = get_page(reinterpret_cast<void*>(buddy_addr));

		auto it = std::find(free_lists_[order].begin(), free_lists_[order].end(),
							buddy_block);
		if (it == free_lists_[order].end()) {
			free_lists_[order].push_back(start_page);
			for (size_t i = 0; i < num_order_pages; i++) {
				pages[start_page->index() + i].set_free();
			}

			return;
		}

		free_lists_[order].erase(it);

		start_page = std::min(start_page, buddy_block);

		++order;
	}
}

void BuddySystem::register_memory_blocks(size_t num_total_pages, Page* start_page)
{
	if (num_total_pages <= 0) {
		return;
	}

	auto calc_max_order_in_total_pages = [](size_t num_total_pages) -> size_t {
		size_t order = calculate_order(num_total_pages);
		if (num_total_pages < (1 << order)) {
			--order;
		}

		return order;
	};

	size_t order = calc_max_order_in_total_pages(num_total_pages);

	const uintptr_t alignment_size = (1 << order) * kernel::memory::PAGE_SIZE;
	const uintptr_t page_addr = reinterpret_cast<uintptr_t>(start_page->ptr());
	if ((page_addr % alignment_size) != 0) {
		const auto aligned_addr =
				static_cast<uintptr_t>(align_up(page_addr, alignment_size));

		const size_t num_rounded_up_pages = (aligned_addr - page_addr) / kernel::memory::PAGE_SIZE;
		if ((num_total_pages - num_rounded_up_pages) < (1 << order)) {
			--order;
		}

		register_memory_blocks(num_rounded_up_pages, start_page);
		num_total_pages -= num_rounded_up_pages;
		order = calc_max_order_in_total_pages(num_total_pages);

		start_page = &pages[start_page->index() + num_rounded_up_pages];
	}

	while (num_total_pages > 0) {
		this->free_lists_[order].push_back(start_page);
		const size_t num_pages = 1 << order;

		start_page = &pages[start_page->index() + num_pages];

		num_total_pages -= num_pages;

		order = calc_max_order_in_total_pages(num_total_pages);
	}
}

void BuddySystem::print_free_lists(int order) const
{
	if (order != -1) {
		LOG_INFO("order-%d remaining blocks: %d", order, free_lists_[order].size());
		for (const auto& page : free_lists_[order]) {
			LOG_DEBUG(" %p", page->ptr());
		}
		LOG_DEBUG("\n");
		return;
	}

	for (int i = 0; i <= MAX_ORDER; i++) {
		LOG_INFO("order-%d remaining blocks: %d", i, free_lists_[i].size());
		for (const auto& page : free_lists_[i]) {
			LOG_DEBUG(" %p", page->ptr());
		}
		LOG_DEBUG("\n");
	}
}

void initialize_memory_manager()
{
	LOG_INFO("Initializing memory manager...");

	memory_manager = new BuddySystem();

	size_t start_index = 0;
	size_t num_total_pages = 0;

	for (size_t i = 0; i < pages.size(); i++) {
		if (pages[i].is_used()) {
			if (num_total_pages > 0) {
				memory_manager->register_memory_blocks(num_total_pages,
													   &pages[start_index]);
			}

			num_total_pages = 0;
			continue;
		}

		if (num_total_pages == 0) {
			start_index = i;
		}

		++num_total_pages;
	}

	if (num_total_pages > 0) {
		memory_manager->register_memory_blocks(num_total_pages, &pages[start_index]);
	}

	run_test_suite(register_buddy_system_tests);

	LOG_INFO("Memory manager initialized successfully.");
}

} // namespace kernel::memory
