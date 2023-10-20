#include "slab.hpp"

#include "bit_utils.hpp"
#include "buddy_system.hpp"
#include "graphics/kernel_logger.hpp"
#include "memory/page.hpp"

#include <cstdio>
#include <cstring>
#include <memory>

m_cache* get_cache_in_chain(char* name)
{
	for (auto& cache : cache_chain) {
		if (strcmp(cache->name, name) == 0) {
			return cache.get();
		}
	}

	return nullptr;
}

m_cache::m_cache(char name[20], size_t object_size)
	: object_size(object_size),
	  num_active_objects(0),
	  num_total_objects(0),
	  num_active_slabs(0),
	  num_total_slabs(0),
	  num_pages_per_slab(0)
{
	strncpy(this->name, name, sizeof(this->name) - 1);
	this->name[sizeof(this->name) - 1] = '\0';

	// TODO
	this->num_pages_per_slab = 1;
}

m_cache& m_cache_create(char* name, size_t obj_size)
{
	obj_size = 1 << bit_ceil(obj_size);

	char temp_name[20];
	if (name == nullptr) {
		sprintf(temp_name, "cache-%d", static_cast<int>(obj_size));
		name = temp_name;
	}

	cache_chain.push_back(std::make_unique<m_cache>(name, obj_size));

	return *cache_chain.back();
}

void m_cache_grow(m_cache* cache)
{
	size_t bytes_per_slab = cache->num_pages_per_slab * PAGE_SIZE;
	void* addr = memory_manager->allocate(bytes_per_slab);
	if (addr == nullptr) {
		klogger->printf("m_cache_grow: failed to allocate memory\n");
		return;
	}

	auto slab = std::make_unique<m_slab>(addr, cache->slabs_free);
	slab->objects.resize(bytes_per_slab / cache->object_size);

	page* page = get_page(addr);
	page->set_cache(cache);
	page->set_slab(slab.get());

	cache->num_total_slabs++;
	cache->num_total_objects += slab->objects.size();

	cache->slabs_free.push_back(std::move(slab));
}

void* m_cache_alloc(m_cache* cache)
{
	if (cache->slabs_partial.empty()) {
		if (cache->slabs_free.empty()) {
			m_cache_grow(cache);
		}

		auto free_slab = std::move(cache->slabs_free.front());
		cache->slabs_free.pop_front();

		cache->slabs_partial.push_back(std::move(free_slab));
		cache->num_active_slabs++;
	}

	auto current_slab = cache->slabs_partial.front().get();
	for (size_t i = 0; i < current_slab->objects.size(); i++) {
		if (!current_slab->objects[i].is_allocated) {
			current_slab->objects[i].is_allocated = true;
			current_slab->num_in_use++;

			cache->num_active_objects++;
			if (current_slab->num_in_use == current_slab->objects.size()) {
				auto full_slab = std::move(cache->slabs_partial.front());
				cache->slabs_partial.pop_front();
				cache->slabs_full.push_back(std::move(full_slab));
			}

			return reinterpret_cast<void*>(
					reinterpret_cast<uintptr_t>(current_slab->base_addr) +
					i * cache->object_size);
		}
	}

	return nullptr;
}

void* kmalloc(size_t size)
{
	size = 1 << bit_ceil(size);

	char name[20];
	sprintf(name, "cache-%d", static_cast<int>(size));

	auto cache = get_cache_in_chain(name);
	if (cache == nullptr) {
		cache = &m_cache_create(name, size);
	}

	return m_cache_alloc(cache);
}

std::list<std::unique_ptr<m_cache>> cache_chain;
void initialize_slab_allocator()
{
	cache_chain = std::list<std::unique_ptr<m_cache>>();
}