/*
 * @file memory/buddy_system.hpp
 *
 * @brief Buddy Memory Allocation System
 *
 * Implements a buddy memory allocation system, which is an efficient method
 * for allocating and deallocating memory blocks of various sizes. The system
 * keeps track of free memory blocks in different sizes (orders) and allows
 * quick allocation and deallocation by splitting and merging these blocks.
 *
 */

#pragma once

#include "memory/custom_allocators.hpp"
#include "memory/page.hpp"
#include <array>
#include <cstddef>
#include <list>

namespace kernel::memory {

static const auto MAX_ORDER = 18;

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
	void* allocate(size_t size);

	/**
	 * \brief Frees a block of memory allocated by the buddy system.
	 *
	 * This function frees a block of memory allocated by the buddy system. The
	 * block is added to the appropriate free list for future allocations.
	 *
	 * \param addr A pointer to the memory to be freed.
	 */
	void free(void* addr, size_t size);

	/**
	 * \brief Registers a block of memory pages with the buddy system.
	 *
	 * This function divides the given memory into appropriate order blocks
	 * and adds them to the system's free lists for future allocations.
	 *
	 * \param num_consecutive_pages The number of memory pages in the block.
	 * \param start_page The starting address of the memory block.
	 */
	void register_memory_blocks(size_t num_total_pages, Page* start_page);

	void print_free_lists(int order = -1) const;

private:
	/**
	 * \brief Splits a memory block of the given order into two smaller blocks.
	 *
	 * This function is used in the buddy memory allocation system to split
	 * larger blocks of memory into two smaller, equally-sized blocks when no
	 * blocks of the desired size are available.
	 *
	 * \param order The order of the memory block to be split.
	 */
	void split_memory_block(int order);

	/**
	 * \brief Calculates the order of a memory block of the given size.
	 *
	 * This function calculates the order of a memory block of the given size.
	 * The order is the power of two that is equal to or greater than the size.
	 * -1 is returned if the size is too large to be allocated by the buddy
	 * system.
	 *
	 * \param size The size of the memory block.
	 * \return The order of the memory block.
	 */

	static size_t calculate_order(size_t num_pages);

	std::array<std::list<Page*, PoolAllocator<Page*, PAGE_SIZE>>, MAX_ORDER + 1>
			free_lists_;
};

extern BuddySystem* memory_manager;

void initialize_memory_manager();

} // namespace kernel::memory
