/**
 * @file page.hpp
 * @brief Page management for memory allocation system
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace kernel::memory
{

class MCache;
class MSlab;

/**
 * @brief Size of a memory page in bytes
 *
 * Standard x86-64 page size of 4KB used throughout the memory management system.
 */
constexpr size_t PAGE_SIZE = 4096;

/**
 * @brief Represents a single memory page in the system
 *
 * This class manages individual pages of memory, tracking their allocation
 * status and association with slab allocator structures. Pages can be free
 * or allocated to a specific cache/slab.
 */
class Page
{
public:
	/**
	 * @brief Default constructor
	 *
	 * Initializes a page as free with no associated pointer.
	 */
	Page() : status_{ 0 }, ptr_{ nullptr } {}

	/**
	 * @brief Get the page index based on its memory address
	 *
	 * @return size_t Page index calculated from the pointer address
	 */
	size_t index() const { return reinterpret_cast<uintptr_t>(ptr_) / PAGE_SIZE; }

	/**
	 * @brief Check if the page is free
	 *
	 * @return true if the page is not allocated
	 * @return false if the page is in use
	 */
	bool is_free() const { return status_ == 0; }

	/**
	 * @brief Check if the page is in use
	 *
	 * @return true if the page is allocated
	 * @return false if the page is free
	 */
	bool is_used() const { return status_ == 1; }

	/**
	 * @brief Get the memory pointer for this page
	 *
	 * @return void* Pointer to the start of the page memory
	 */
	void* ptr() const { return ptr_; }

	/**
	 * @brief Set the memory pointer for this page
	 *
	 * @param ptr Pointer to the start of the page memory
	 */
	void set_ptr(void* ptr) { ptr_ = ptr; }

	/**
	 * @brief Mark the page as free
	 */
	void set_free() { status_ = 0; }

	/**
	 * @brief Mark the page as used
	 */
	void set_used() { status_ = 1; }

	/**
	 * @brief Associate this page with a cache
	 *
	 * @param cache Pointer to the cache this page belongs to
	 */
	void set_cache(MCache* cache) { cache_ = cache; }

	/**
	 * @brief Associate this page with a slab
	 *
	 * @param slab Pointer to the slab this page belongs to
	 */
	void set_slab(MSlab* slab) { slab_ = slab; }

	/**
	 * @brief Get the cache this page belongs to
	 *
	 * @return MCache* Pointer to the associated cache, or nullptr
	 */
	MCache* cache() const { return cache_; }

	/**
	 * @brief Get the slab this page belongs to
	 *
	 * @return MSlab* Pointer to the associated slab, or nullptr
	 */
	MSlab* slab() const { return slab_; }

private:
	int status_;	 ///< Page status: 0 = free, 1 = used
	MCache* cache_; ///< Associated cache (for slab allocator)
	MSlab* slab_;	 ///< Associated slab (for slab allocator)
	void* ptr_;		 ///< Pointer to the page's memory
};

/**
 * @brief Global vector containing all system pages
 *
 * This vector tracks all pages in the system, allowing for efficient
 * page lookup and management.
 */
extern std::vector<Page> pages;

/**
 * @brief Initialize the page management system
 *
 * Sets up the global page vector and initializes all pages based on
 * available physical memory.
 */
void initialize_pages();

/**
 * @brief Get the page object for a given memory address
 *
 * @param ptr Memory address within a page
 * @return Page* Pointer to the page object, or nullptr if not found
 */
Page* get_page(void* ptr);

/**
 * @brief Get current memory usage statistics
 *
 * @param[out] total_mem Total memory in bytes
 * @param[out] used_mem Used memory in bytes
 */
void get_memory_usage(size_t* total_mem, size_t* used_mem);

} // namespace kernel::memory
