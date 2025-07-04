/**
 * @file memory/slab.hpp
 * @brief Slab allocator implementation
 *
 * This file contains the implementation of a slab allocator for efficient
 * memory management in a kernel environment. The slab allocator is designed to
 * minimize fragmentation by managing memory in fixed-size blocks called slabs.
 * 
 * @note Uses the unified error handling interface (see error.hpp)
 * @date 2024
 */

#pragma once

#include "error.hpp"
#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>

namespace kernel::memory
{

constexpr int ALLOC_UNINITIALIZED = 0;
constexpr int ALLOC_ZEROED = (1 << 0);

enum class slab_status : uint8_t {
	FULL,
	PARTIAL,
	FREE,
};

class m_object
{
public:
	m_object() = default;

	void increase_usage_count() { usage_count_++; }

private:
	unsigned int usage_count_;
};

class m_cache;

class m_slab
{
public:
	m_slab(void* base_addr, size_t num_objs);

	void set_position_in_list(std::list<std::unique_ptr<m_slab>>::iterator it)
	{
		position_in_list_ = it;
	}

	slab_status status() const { return status_; }

	void set_status(slab_status status) { status_ = status; }

	bool is_full() const { return num_in_use_ == objects_.size(); }

	bool is_empty() const { return num_in_use_ == 0; }

	void* alloc_object(size_t obj_size);

	void free_object(void* addr, size_t obj_size);

	void move_list(m_cache& cache, slab_status to);

private:
	std::list<std::unique_ptr<m_slab>>::iterator position_in_list_;
	std::vector<m_object> objects_;
	void* base_addr_;
	size_t num_in_use_;
	std::list<size_t> free_objects_index_;
	slab_status status_;
};

class m_cache
{
public:
	m_cache(char name[20], size_t object_size);

	char* name() { return name_; }
	size_t object_size() const { return object_size_; }

	void decrease_num_active_objects() { num_active_objects_--; }
	void decrease_num_active_slabs() { num_active_slabs_--; }

	bool grow();
	void* alloc();

	std::list<std::unique_ptr<m_slab>> slabs_full_;
	std::list<std::unique_ptr<m_slab>> slabs_partial_;
	std::list<std::unique_ptr<m_slab>> slabs_free_;

private:
	char name_[20];
	size_t object_size_;
	size_t num_active_objects_;
	size_t num_total_objects_;
	size_t num_active_slabs_;
	size_t num_total_slabs_;
	size_t num_pages_per_slab_;
};

extern std::list<std::unique_ptr<m_cache>> cache_chain;

/**
 * @brief Get a cache by name from the cache chain
 * @param name Name of the cache to retrieve
 * @return Pointer to the found cache, or nullptr if not found
 */
m_cache* get_cache_in_chain(char* name);

/**
 * @brief Create a new memory cache
 * @param name Name of the cache
 * @param obj_size Size of objects in the cache
 * @return Reference to the created cache
 */
m_cache& m_cache_create(const char* name, size_t obj_size);

/**
 * @brief Allocate kernel memory
 * @param size Size to allocate
 * @param flags Allocation flags (e.g., ALLOC_ZEROED)
 * @param align Alignment requirement (default: 1)
 * @return Pointer to allocated memory, or nullptr on failure
 * @note Returns nullptr on failure. Consider using ALLOC_OR_RETURN_ERROR macro
 */
void* alloc(size_t size, unsigned flags, int align = 1);

/**
 * @brief Free kernel memory
 * @param addr Address of memory to free
 * @note Does nothing if addr is nullptr
 */
void free(void* addr);

struct free_deleter {
	void operator()(void* p) { free(p); }
};

/**
 * @brief Initialize the slab allocator
 * @note Must be called once during kernel initialization
 */
void initialize_slab_allocator();

} // namespace kernel::memory

#define ALLOC_OR_RETURN(ptr, size, flags)                                         \
	do {                                                                            \
		(ptr) = kernel::memory::alloc(size, flags);                               \
		if ((ptr) == nullptr) {                                                     \
			LOG_ERROR("failed to allocate memory: %s", #ptr);                       \
			return;                                                                 \
		}                                                                           \
	} while (0)

/**
 * @brief Macro to allocate memory and return error code on failure
 * @param ptr Variable to store allocated pointer
 * @param size Size to allocate
 * @param flags Allocation flags
 * @note Returns ERR_NO_MEMORY if allocation fails
 */
#define ALLOC_OR_RETURN_ERROR(ptr, size, flags)                                   \
	do {                                                                            \
		(ptr) = kernel::memory::alloc(size, flags);                               \
		if ((ptr) == nullptr) {                                                     \
			LOG_ERROR_CODE(ERR_NO_MEMORY, "failed to allocate memory: %s", #ptr);   \
			return ERR_NO_MEMORY;                                                   \
		}                                                                           \
	} while (0)
