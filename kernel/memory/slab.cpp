#include "slab.hpp"

#include "buddy_system.hpp"
#include "graphics/kernel_logger.hpp"
#include <cstdio>
#include <cstring>
#include <memory>

m_cache* m_cache_create(char* name, size_t obj_size)
{
	m_cache* cache = new m_cache;
	memset(cache, 0, sizeof(m_cache));

	size_t order = 0;
	while (obj_size > (1 << order)) {
		order++;
	}
	cache->object_size = 1 << order;

	if (name == nullptr) {
		name = new char[20];
		sprintf(name, "cache-%d", 1 << order);
	}

	strncpy(cache->name, name, sizeof(cache->name) - 1);
	cache->name[sizeof(cache->name) - 1] = '\0';

	cache->num_pages_per_slab = 1;

	return cache;
}

void m_cache_grow(m_cache* cache)
{
	void* addr = memory_manager->allocate(cache->num_pages_per_slab * PAGE_SIZE);
	if (addr == nullptr) {
		klogger->printf("m_cache_grow: failed to allocate memory\n");
		return;
	}

	auto slab = std::make_unique<m_slab>(addr, cache->slabs_free);
	slab->objects.resize(cache->num_pages_per_slab * PAGE_SIZE / cache->object_size);

	cache->num_total_slabs++;
	cache->num_total_objects += slab->objects.size();

	cache->slabs_free.push_back(std::move(slab));
}

void* kmalloc(size_t size) { return nullptr; }