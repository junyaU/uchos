#include "buddy_system.hpp"

#include <algorithm>
#include <climits>
#include <optional>
#include <sys/types.h>

#include "graphics/system_logger.hpp"
#include "memory/bootstrap_allocator.hpp"

namespace
{
int bit_width(unsigned int x)
{
	if (x == 0) {
		return 0;
	}

	return CHAR_BIT * sizeof(unsigned int) - __builtin_clz(x);
}
} // namespace

BuddySystem* buddy_system;

int BuddySystem::__calculate_order(int required_blocks) const
{
	int order;
	if ((required_blocks & (required_blocks - 1)) == 0) {
		order = bit_width(required_blocks) - 1;
	} else {
		order = bit_width(required_blocks);
	}

	if (order > kMaxOrder) {
		return -1;
	}

	return order;
}

int BuddySystem::calculate_order(size_t size) const
{
	return __calculate_order((size + PAGE_SIZE - 1) / PAGE_SIZE);
}

int BuddySystem::calculate_order(int num_pages) const
{
	return __calculate_order(num_pages);
}

unsigned int BuddySystem::CalculateAvailableBlocks() const
{
	int num_blocks = 0;
	for (int order = 0; order <= kMaxOrder; order++) {
		num_blocks += free_lists_[order].size() * (1 << order);
	}

	return num_blocks;
}

void BuddySystem::SplitMemoryBlock(int order)
{
	auto block = free_lists_[order].front();
	free_lists_[order].pop_front();

	auto nextSmallerBlockSize = PAGE_SIZE * (1 << (order - 1));

	free_lists_[order - 1].push_back(block);
	free_lists_[order - 1].push_back(reinterpret_cast<void*>(
			reinterpret_cast<uintptr_t>(block) + nextSmallerBlockSize));
}

void* BuddySystem::Allocate(size_t size)
{
	int order = calculate_order(size);
	if (order == -1) {
		system_logger->Printf("invalid size: %d\n", size);
		return nullptr;
	}

	if (free_lists_[order].empty()) {
		int next_order = -1;
		for (int i = order + 1; i <= kMaxOrder; i++) {
			if (!free_lists_[i].empty()) {
				next_order = i;
				break;
			}
		}

		if (next_order == -1) {
			system_logger->Printf("failed to allocate memory: order=%d\n", order);
			return nullptr;
		}

		for (int i = next_order; i > order; i--) {
			SplitMemoryBlock(i);
		}
	}

	auto addr = free_lists_[order].front();
	free_lists_[order].pop_front();

	return addr;
}

void BuddySystem::Free(void* addr, size_t size)
{
	for (int i = 0; i <= kMaxOrder; i++) {
		auto it = std::find(free_lists_[i].begin(), free_lists_[i].end(), addr);
		if (it != free_lists_[i].end()) {
			system_logger->Printf(
					"double free detected at address: %p in order: %d\n", addr, i);
			return;
		}
	}

	int order = calculate_order(size);
	if (order == -1) {
		system_logger->Printf("invalid size: %d\n", size);
		return;
	}

	while (order <= kMaxOrder) {
		if (order == kMaxOrder) {
			free_lists_[order].push_back(addr);
			return;
		}

		auto buddy_addr =
				reinterpret_cast<uintptr_t>(addr) ^ ((1 << order) * PAGE_SIZE);

		auto it = std::find(free_lists_[order].begin(), free_lists_[order].end(),
							reinterpret_cast<void*>(buddy_addr));
		if (it == free_lists_[order].end()) {
			free_lists_[order].push_back(addr);
			return;
		}

		free_lists_[order].erase(it);

		addr = reinterpret_cast<void*>(
				std::min(reinterpret_cast<uintptr_t>(addr), buddy_addr));

		order++;
	}
}

void BuddySystem::register_memory(size_t num_consecutive_pages, page* start_page)
{
	if (num_consecutive_pages <= 0) {
		return;
	}

	while (num_consecutive_pages > 0) {
		const auto order =
				std::min(calculate_order(num_consecutive_pages), kMaxOrder);

		this->free_lists_[order].push_back(start_page);

		int num_order_pages = 1 << order;

		start_page = &pages[start_page->index() + num_order_pages];

		num_consecutive_pages -= num_order_pages;
	}
}

void initialize_buddy_system()
{
	buddy_system = new BuddySystem();

	size_t concecutive_start_index = 0;
	size_t num_consecutive_pages = 0;
	for (size_t i = 0; i < pages.size(); i++) {
		if (pages[i].is_used()) {
			if (num_consecutive_pages > 0) {
				buddy_system->register_memory(num_consecutive_pages,
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
		buddy_system->register_memory(num_consecutive_pages,
									  &pages[concecutive_start_index]);
	}
}
