#pragma once

#include <cstddef>
#include <list>
#include <vector>

struct m_object {
	bool is_allocated = false;
};

struct m_slab {
	m_slab(void* base_addr, std::list<std::unique_ptr<m_slab>>& parent_list)
		: parent_list(parent_list), objects(0), base_addr(base_addr), num_in_use(0)
	{
	}

	std::list<std::unique_ptr<m_slab>>& parent_list;
	std::vector<m_object> objects;
	void* base_addr;
	size_t num_in_use;
	// free どのobjectがfreeかを判断するための配列
};

struct m_cache {
	m_cache(char name[20], size_t object_size);

	char name[20];
	size_t object_size;
	size_t num_active_objects;
	size_t num_total_objects;
	size_t num_active_slabs;
	size_t num_total_slabs;
	size_t num_pages_per_slab;

	std::list<std::unique_ptr<m_slab>> slabs_full;
	std::list<std::unique_ptr<m_slab>> slabs_partial;
	std::list<std::unique_ptr<m_slab>> slabs_free;
};

extern std::list<std::unique_ptr<m_cache>> cache_chain;

m_cache* get_cache_in_chain(char* name);

m_cache& m_cache_create(char* name, size_t object_size);

void m_cache_grow(m_cache* cache);

void* m_cache_alloc(m_cache* cache);

void* kmalloc(size_t size);

void kfree(void* addr);

void initialize_slab_allocator();