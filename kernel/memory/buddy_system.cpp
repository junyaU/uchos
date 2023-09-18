#include "buddy_system.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <optional>
#include <sys/types.h>

#include "graphics/system_logger.hpp"

namespace
{
char memory_pool[sizeof(BuddySystem)];

int bit_width(unsigned int x)
{
	if (x == 0) {
		return 0;
	}

	return CHAR_BIT * sizeof(unsigned int) - __builtin_clz(x);
}
} // namespace

BuddySystem* buddy_system;

int BuddySystem::CalculateOrder(size_t size) const
{
	int required_block_count = (size + kMemoryBlockSize - 1) / kMemoryBlockSize;

	int order;
	if ((required_block_count & (required_block_count - 1)) == 0) {
		order = bit_width(required_block_count) - 1;
	} else {
		order = bit_width(required_block_count);
	}

	if (order > kMaxOrder) {
		return -1;
	}

	return order;
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

	auto nextSmallerBlockSize = kMemoryBlockSize * (1 << (order - 1));

	free_lists_[order - 1].push_back(block);
	free_lists_[order - 1].push_back(reinterpret_cast<void*>(
			reinterpret_cast<uintptr_t>(block) + nextSmallerBlockSize));
}

void* BuddySystem::Allocate(size_t size)
{
	int order = CalculateOrder(size);
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

	int order = CalculateOrder(size);
	if (order == -1) {
		system_logger->Printf("invalid size: %d\n", size);
		return;
	}

	while (order <= kMaxOrder) {
		if (order == kMaxOrder) {
			free_lists_[order].push_back(addr);
			return;
		}

		auto buddy_addr = reinterpret_cast<uintptr_t>(addr) ^
						  ((1 << order) * kMemoryBlockSize);

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

void BuddySystem::RegisterMemory(int num_pages, void* addr)
{
	if (!addr || num_pages <= 0) {
		return;
	}

	while (num_pages > 0) {
		const auto order =
				std::min(static_cast<int>(std::log2(num_pages)), kMaxOrder);

		this->free_lists_[order].push_back(addr);

		auto next_addr = reinterpret_cast<uintptr_t>(addr) +
						 kMemoryBlockSize * static_cast<uintptr_t>(1 << order);

		addr = reinterpret_cast<void*>(next_addr);
		num_pages -= 1 << order;
	}
}

void BuddySystem::ShowFreeMemorySize() const
{
	unsigned int current_memory_blocks = CalculateAvailableBlocks();

	system_logger->Printf("free memory size: %u MiB / %u MiB\n",
						  current_memory_blocks * kMemoryBlockSize / 1024 / 1024,
						  total_memory_blocks_ * kMemoryBlockSize / 1024 / 1024);
}

extern "C" caddr_t program_break, program_break_end;

void InitializeHeap()
{
	// 128 MiB
	const int kHeapFrames = 64 * 512;
	const size_t kHeapSize = kHeapFrames * kMemoryBlockSize;

	auto heap = buddy_system->Allocate(kHeapSize);
	if (heap == nullptr) {
		system_logger->Printf("failed to allocate heap\n");
		return;
	}

	program_break = reinterpret_cast<caddr_t>(heap);
	program_break_end = program_break + kHeapSize;
}

void InitializeBuddySystem(const MemoryMap& memory_map)
{
	buddy_system = new (memory_pool) BuddySystem();

	const auto memmap_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
	const auto memmap_end = memmap_base + memory_map.map_size;

	for (uintptr_t iter = memmap_base; iter < memmap_end;
		 iter += memory_map.descriptor_size) {
		auto descriptor = reinterpret_cast<MemoryDescriptor*>(iter);

		if (!IsAvailableMemory(static_cast<MemoryType>(descriptor->type))) {
			continue;
		}

		buddy_system->RegisterMemory(
				static_cast<int>(descriptor->number_of_pages),
				reinterpret_cast<void*>(descriptor->physical_start));
	}

	unsigned int num_blocks = buddy_system->CalculateAvailableBlocks();

	buddy_system->SetTotalMemoryBlocks(num_blocks);
}
