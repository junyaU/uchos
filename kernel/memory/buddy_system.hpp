#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <list>

#include "../../UchLoaderPkg/memory_map.hpp"
#include "pool.hpp"

static const auto kMemoryBlockSize = 4 * 1024;

class BuddySystem
{
public:
	BuddySystem();

	/**
	 * \brief Allocates memory of the specified size using the buddy system.
	 *
	 * This function attempts to find a block of memory of the requested size.
	 * If no block of the exact size is available, it tries to split a larger block
	 * until a block of the desired size is available.
	 *
	 * \param size The amount of memory to allocate in bytes.
	 * \return A pointer to the allocated memory, or nullptr if the allocation
	 * failed.
	 */
	void* Allocate(size_t size);

	/**
	 * \brief Frees a block of memory allocated by the buddy system.
	 *
	 * This function frees a block of memory allocated by the buddy system. The
	 * block is added to the appropriate free list for future allocations.
	 *
	 * \param addr A pointer to the memory to be freed.
	 */
	void Free(void* addr, size_t size);

	/**
	 * \brief Registers a block of memory pages with the buddy system.
	 *
	 * This function divides the given memory into appropriate order blocks
	 * and adds them to the system's free lists for future allocations.
	 *
	 * \param num_pages The number of memory pages in the block.
	 * \param addr The starting address of the memory block.
	 */
	void RegisterMemory(int num_pages, void* addr);

private:
	static const auto kMaxOrder = 18;

	/**
	 * \brief Splits a memory block of the given order into two smaller blocks.
	 *
	 * This function is used in the buddy memory allocation system to split larger
	 * blocks of memory into two smaller, equally-sized blocks when no blocks of the
	 * desired size are available.
	 *
	 * \param order The order of the memory block to be split.
	 */
	void SplitMemoryBlock(int order);

	/**
	 * \brief Calculates the order of a memory block of the given size.
	 *
	 * This function calculates the order of a memory block of the given size.
	 * The order is the power of two that is equal to or greater than the size.
	 * -1 is returned if the size is too large to be allocated by the buddy system.
	 *
	 * \param size The size of the memory block.
	 * \return The order of the memory block.
	 */
	int CalculateOrder(size_t size) const;

	std::array<std::list<void*, PoolAllocator<void*, 4 * 1024>>, kMaxOrder + 1>
			free_lists_;
};

extern BuddySystem* buddy_system;

/**
 * \brief Initializes the buddy system using the provided UEFI memory map.
 *
 * This function iterates over the UEFI memory map and registers available
 * memory regions with the buddy system.
 *
 * \param memory_map A reference to the UEFI memory map.
 */
void InitializeBuddySystem(const MemoryMap& memory_map);

/**
 * \brief Initializes the heap memory for the system.
 *
 * This function allocates a contiguous block of memory to serve as the system's
 * heap. The size of the heap is defined by kHeapFrames and kMemoryBlockSize. After
 * allocation, the program break pointers (`program_break` and `program_break_end`)
 * are set to the boundaries of the allocated heap memory.
 */
void InitializeHeap();