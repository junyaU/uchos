#include "buddy_system.hpp"

#include <algorithm>
#include <cmath>
#include <sys/types.h>

#include "graphics/system_logger.hpp"

namespace
{
char memory_pool[sizeof(BuddySystem)];
}

BuddySystem* buddy_system;

BuddySystem::BuddySystem() {}

int BuddySystem::CalculateOrder(size_t size) const
{
	int num_pages = (size + kMemoryBlockSize - 1) / kMemoryBlockSize;

	int order = 0;
	while (num_pages > static_cast<int>(std::pow(2, order))) {
		order++;
	}

	if (order > kMaxOrder) {
		return -1;
	}

	return order;
}

void BuddySystem::SplitMemoryBlock(int order)
{
	auto block = free_lists_[order].front();
	free_lists_[order].pop_front();

	auto nextSmallerBlockSize =
			kMemoryBlockSize * static_cast<int>(std::pow(2, order - 1));

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
						  (static_cast<int>(std::pow(2, order)) * kMemoryBlockSize);

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

		auto next_addr =
				reinterpret_cast<uintptr_t>(addr) +
				kMemoryBlockSize * static_cast<uintptr_t>(std::pow(2, order));

		addr = reinterpret_cast<void*>(next_addr);
		num_pages -= static_cast<int>(std::pow(2, order));
	}
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
}
