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

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>
#include "error.hpp"

namespace kernel::memory
{

constexpr int ALLOC_UNINITIALIZED = 0;
constexpr int ALLOC_ZEROED = (1 << 0);

enum class SlabStatus : uint8_t {
	FULL,
	PARTIAL,
	FREE,
};

class MObject
{
public:
	MObject() = default;

	void increase_usage_count() { usage_count_++; }

private:
	unsigned int usage_count_;
};

class MCache;

class MSlab
{
public:
	MSlab(void* base_addr, size_t num_objs);

	void set_position_in_list(std::list<std::unique_ptr<MSlab>>::iterator it)
	{
		position_in_list_ = it;
	}

	SlabStatus status() const { return status_; }

	void set_status(SlabStatus status) { status_ = status; }

	bool is_full() const { return num_in_use_ == objects_.size(); }

	bool is_empty() const { return num_in_use_ == 0; }

	void* alloc_object(size_t obj_size);

	void free_object(void* addr, size_t obj_size);

	void move_list(MCache& cache, SlabStatus to);

private:
	std::list<std::unique_ptr<MSlab>>::iterator position_in_list_;
	std::vector<MObject> objects_;
	void* base_addr_;
	size_t num_in_use_;
	std::list<size_t> free_objects_index_;
	SlabStatus status_;
};

class MCache
{
public:
	MCache(char name[20], size_t object_size);

	char* name() { return name_; }
	size_t object_size() const { return object_size_; }

	void decrease_num_active_objects() { num_active_objects_--; }
	void decrease_num_active_slabs() { num_active_slabs_--; }

	bool grow();
	void* alloc();

	std::list<std::unique_ptr<MSlab>> slabs_full_;
	std::list<std::unique_ptr<MSlab>> slabs_partial_;
	std::list<std::unique_ptr<MSlab>> slabs_free_;

private:
	char name_[20];
	size_t object_size_;
	size_t num_active_objects_;
	size_t num_total_objects_;
	size_t num_active_slabs_;
	size_t num_total_slabs_;
	size_t num_pages_per_slab_;
};

extern std::list<std::unique_ptr<MCache>> cache_chain;

/**
 * @brief Get a cache by name from the cache chain
 * @param name Name of the cache to retrieve
 * @return Pointer to the found cache, or nullptr if not found
 */
MCache* get_cache_in_chain(char* name);

/**
 * @brief Create a new memory cache
 * @param name Name of the cache
 * @param obj_size Size of objects in the cache
 * @return Reference to the created cache
 */
MCache& m_cache_create(const char* name, size_t obj_size);

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

struct FreeDeleter {
	void operator()(void* p) { free(p); }
};

/**
 * @brief Initialize the slab allocator
 * @note Must be called once during kernel initialization
 */
void initialize_slab_allocator();

} // namespace kernel::memory

/**
 * @brief Allocate memory or return void (for void functions)
 * @param ptr Variable to store allocated pointer
 * @param size Size to allocate
 * @param flags Allocation flags
 * @note Returns void if allocation fails, suitable for void functions
 */
#define ALLOC_OR_RETURN(ptr, size, flags)                                           \
	do {                                                                            \
		(ptr) = kernel::memory::alloc(size, flags);                                 \
		if ((ptr) == nullptr) {                                                     \
			LOG_ERROR("Memory allocation failed: %s (size=%zu)", #ptr,              \
					  (size_t)(size));                                              \
			return;                                                                 \
		}                                                                           \
	} while (0)

/**
 * @brief Allocate memory or return error code
 * @param ptr Variable to store allocated pointer
 * @param size Size to allocate
 * @param flags Allocation flags
 * @note Returns ERR_NO_MEMORY if allocation fails
 */
#define ALLOC_OR_RETURN_ERROR(ptr, size, flags)                                     \
	do {                                                                            \
		(ptr) = kernel::memory::alloc(size, flags);                                 \
		if ((ptr) == nullptr) {                                                     \
			LOG_ERROR("Memory allocation failed: %s (size=%zu)", #ptr,              \
					  (size_t)(size));                                              \
			return ERR_NO_MEMORY;                                                   \
		}                                                                           \
	} while (0)

/**
 * @brief Allocate memory or return nullptr
 * @param ptr Variable to store allocated pointer
 * @param size Size to allocate
 * @param flags Allocation flags
 * @note Returns nullptr if allocation fails, suitable for functions returning
 * pointers
 */
#define ALLOC_OR_RETURN_NULL(ptr, size, flags)                                      \
	do {                                                                            \
		(ptr) = kernel::memory::alloc(size, flags);                                 \
		if ((ptr) == nullptr) {                                                     \
			LOG_ERROR("Memory allocation failed: %s (size=%zu)", #ptr,              \
					  (size_t)(size));                                              \
			return nullptr;                                                         \
		}                                                                           \
	} while (0)
