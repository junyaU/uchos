#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <list>

#include "../../UchLoaderPkg/memory_map.hpp"
#include "page.hpp"
#include "pool.hpp"

class BuddySystem
{
public:
	BuddySystem() = default;

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
	void register_memory(size_t num_pages, page* addr);

	/**
	 * \brief Calculates the total number of available memory blocks in the system.
	 *
	 * This function iterates over all free lists in the buddy system and calculates
	 * the sum of available blocks. Each block in a free list of a particular order
	 * represents 2^order individual memory blocks.
	 *
	 * \return The total number of unused memory blocks managed by the buddy system.
	 */
	unsigned int CalculateAvailableBlocks() const;

	void SetTotalMemoryBlocks(unsigned int total_memory_blocks)
	{
		total_memory_blocks_ = total_memory_blocks;
	}

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
	int calculate_order(size_t size) const;

	int calculate_order(int num_pages) const;

	int __calculate_order(int required_blocks) const;

	unsigned int total_memory_blocks_;

	std::array<std::list<void*, PoolAllocator<void*, PAGE_SIZE>>, kMaxOrder + 1>
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
void initialize_buddy_system();
