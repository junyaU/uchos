#include "slab.hpp"
#include "../bit_utils.hpp"
#include "../graphics/kernel_logger.hpp"
#include "buddy_system.hpp"
#include "page.hpp"
#include <stdio.h>

m_cache* get_cache_in_chain(char* name)
{
	for (auto& cache : cache_chain) {
		if (strcmp(cache->name(), name) == 0) {
			return cache.get();
		}
	}

	return nullptr;
}

m_cache::m_cache(char name[20], size_t object_size)
	: object_size_(object_size),
	  num_active_objects_(0),
	  num_total_objects_(0),
	  num_active_slabs_(0),
	  num_total_slabs_(0),
	  num_pages_per_slab_(0)
{
	strncpy(name_, name, sizeof(name_) - 1);
	name[sizeof(name_) - 1] = '\0';

	if (object_size_ > 2048) {
		num_pages_per_slab_ = 2;
	} else {
		num_pages_per_slab_ = 1;
	}
}

m_slab::m_slab(void* base_addr, size_t num_objs)
	: base_addr_(base_addr), num_in_use_(0)
{
	objects_.resize(num_objs);

	for (size_t i = 0; i < num_objs; i++) {
		free_objects_index_.push_back(i);
	}
}

m_cache& m_cache_create(const char* name, size_t obj_size)
{
	obj_size = 1 << bit_ceil(obj_size);

	char temp_name[20];
	if (name == nullptr) {
		sprintf(temp_name, "cache-%d", static_cast<int>(obj_size));
		name = temp_name;
	}

	cache_chain.push_back(
			std::make_unique<m_cache>(const_cast<char*>(name), obj_size));

	return *cache_chain.back();
}

bool m_cache::grow()
{
	size_t const bytes_per_slab = num_pages_per_slab_ * PAGE_SIZE;
	void* addr = memory_manager->allocate(bytes_per_slab);
	if (addr == nullptr) {
		klogger->print("m_cache_grow: failed to allocate memory\n");
		return false;
	}

	size_t const num_objs = bytes_per_slab / object_size_;
	auto slab = std::make_unique<m_slab>(addr, num_objs);

	page* page = get_page(addr);
	page->set_cache(this);
	page->set_slab(slab.get());

	num_total_slabs_++;
	num_total_objects_ += num_objs;

	slabs_free_.push_back(std::move(slab));
	auto last_it = std::prev(slabs_free_.end());
	(*last_it)->set_position_in_list(last_it);

	return true;
}

void* m_cache::alloc()
{
	if (slabs_partial_.empty()) {
		if (slabs_free_.empty() && !grow()) {
			klogger->print("m_cache_alloc: failed to grow\n");
			return nullptr;
		}

		auto* free_slab = slabs_free_.front().get();
		free_slab->move_list(slabs_free_, slabs_partial_);

		num_active_slabs_++;
	}

	auto* current_slab = slabs_partial_.front().get();
	void* addr = current_slab->alloc_object(object_size_);
	if (addr == nullptr) {
		klogger->print("m_cache_alloc: failed to allocate object\n");
		return nullptr;
	}

	num_active_objects_++;
	if (current_slab->is_full()) {
		auto* full_slab = slabs_partial_.front().get();
		full_slab->move_list(slabs_partial_, slabs_full_);
	}

	return addr;
}

void m_slab::move_list(std::list<std::unique_ptr<m_slab>>& from,
					   std::list<std::unique_ptr<m_slab>>& to)
{
	auto current_slab = std::move(*position_in_list_);
	from.erase(position_in_list_);
	to.push_back(std::move(current_slab));
	position_in_list_ = std::prev(to.end());
}

void* m_slab::alloc_object(size_t obj_size)
{
	if (free_objects_index_.empty()) {
		return nullptr;
	}

	size_t const obj_index = free_objects_index_.front();
	free_objects_index_.pop_front();
	objects_[obj_index].increase_usage_count();
	num_in_use_++;

	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base_addr_) +
								   obj_index * obj_size);
}

void m_slab::free_object(void* addr, size_t obj_size)
{
	size_t const objs_index = (reinterpret_cast<uintptr_t>(addr) -
							   reinterpret_cast<uintptr_t>(base_addr_)) /
							  obj_size;

	free_objects_index_.push_back(objs_index);
	num_in_use_--;
}

void* kmalloc(size_t size)
{
	size = 1 << bit_ceil(size);

	char name[20];
	sprintf(name, "cache-%d", static_cast<int>(size));

	auto* cache = get_cache_in_chain(name);
	if (cache == nullptr) {
		cache = &m_cache_create(name, size);
	}

	return cache->alloc();
}

void kfree(void* addr)
{
	page* page = get_page(addr);
	m_cache* cache = page->cache();
	m_slab* slab = page->slab();

	slab->free_object(addr, cache->object_size());
	cache->decrease_num_active_objects();

	if (slab->is_empty()) {
		slab->move_list(cache->slabs_partial_, cache->slabs_free_);
		cache->decrease_num_active_slabs();
	} else if (slab->was_previously_full()) {
		slab->move_list(cache->slabs_full_, cache->slabs_partial_);
	}
};

std::list<std::unique_ptr<m_cache>> cache_chain;
void initialize_slab_allocator()
{
	cache_chain = std::list<std::unique_ptr<m_cache>>();
}