#pragma once

#include <cstddef>
#include <list>

class m_cache
{
public:
private:
	char name[20];
	size_t object_size;
	size_t object_count;
	size_t num_active_objects;
	size_t num_total_objects;
	size_t num_active_slabs;
	size_t num_total_slabs;

	std::list<m_slab> slabs_full;
	std::list<m_slab> slabs_partial;
	std::list<m_slab> slabs_free;
};

class m_slab
{
public:
private:
};

class m_object
{
public:
private:
};

std::list<m_cache> cache_chain;

void* kmalloc(size_t size);

void kfree(void* addr);