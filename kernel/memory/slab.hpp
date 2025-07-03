/*
 * @file memory/slab.hpp
 *
 * @brief slab allocator
 *
 * This file contains the implementation of a slab allocator for efficient
 * memory management in a kernel environment. The slab allocator is designed to
 * minimize fragmentation by managing memory in fixed-size blocks called slabs.
 *
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>

namespace kernel::memory {

constexpr int KMALLOC_UNINITIALIZED = 0;
constexpr int KMALLOC_ZEROED = (1 << 0);

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

m_cache* get_cache_in_chain(char* name);

m_cache& m_cache_create(const char* name, size_t obj_size);

void* kmalloc(size_t size, unsigned flags, int align = 1);

void kfree(void* addr);

struct kfree_deleter {
	void operator()(void* p) { kfree(p); }
};

void initialize_slab_allocator();

} // namespace kernel::memory

#define KMALLOC_OR_RETURN(ptr, size, flags)                                         \
	do {                                                                            \
		(ptr) = kmalloc(size, flags);                                               \
		if ((ptr) == nullptr) {                                                     \
			LOG_ERROR("failed to allocate memory: %s", #ptr);                       \
			return;                                                                 \
		}                                                                           \
	} while (0)
