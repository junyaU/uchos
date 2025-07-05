#include "slab.hpp"
#include <cstdint>
#include "bit_utils.hpp"
#include "buddy_system.hpp"
#include "graphics/log.hpp"
#include "page.hpp"
#include "tests/framework.hpp"
#include "tests/test_cases/memory_test.hpp"
#include <cstring>
#include <iterator>
#include <libs/common/types.hpp>
#include <list>
#include <memory>
#include <stdio.h>
#include <unordered_map>
#include <utility>

namespace kernel::memory {

m_cache* get_cache_in_chain(char* name)
{
	if (cache_chain.empty()) {
		return nullptr;
	}

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

	if (object_size <= kernel::memory::PAGE_SIZE) {
		num_pages_per_slab_ = 1;
	} else {
		num_pages_per_slab_ = object_size / kernel::memory::PAGE_SIZE;
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
	obj_size = 1 << bit_width_ceil(obj_size);

	char temp_name[20];
	if (name == nullptr) {
		sprintf(temp_name, "cache-%d", static_cast<int>(obj_size));
		name = temp_name;
	}

	cache_chain.push_back(
			std::make_unique<kernel::memory::m_cache>(const_cast<char*>(name), obj_size));

	return *cache_chain.back();
}

bool m_cache::grow()
{
	size_t const bytes_per_slab = num_pages_per_slab_ * kernel::memory::PAGE_SIZE;
	void* addr = kernel::memory::memory_manager->allocate(bytes_per_slab);
	if (addr == nullptr) {
		LOG_ERROR("failed to allocate memory");
		return false;
	}

	size_t const num_objs = bytes_per_slab / object_size_;
	auto slab = std::make_unique<m_slab>(addr, num_objs);

	page* page = get_page(addr);
	page->set_cache(this);
	page->set_slab(slab.get());

	++num_total_slabs_;
	num_total_objects_ += num_objs;

	slab->set_status(slab_status::FREE);
	slabs_free_.push_back(std::move(slab));
	auto last_it = std::prev(slabs_free_.end());
	(*last_it)->set_position_in_list(last_it);

	return true;
}

void* m_cache::alloc()
{
	if (slabs_partial_.empty()) {
		if (slabs_free_.empty() && !grow()) {
			LOG_ERROR("failed to grow");
			return nullptr;
		}

		auto* free_slab = slabs_free_.front().get();
		free_slab->move_list(*this, slab_status::PARTIAL);

		++num_active_slabs_;
	}

	auto* current_slab = slabs_partial_.front().get();

	void* addr = current_slab->alloc_object(object_size_);
	if (addr == nullptr) {
		LOG_ERROR("failed to allocate object");
		return nullptr;
	}

	++num_active_objects_;
	if (current_slab->is_full()) {
		auto* full_slab = slabs_partial_.front().get();
		full_slab->move_list(*this, slab_status::FULL);
	}

	return addr;
}

void m_slab::move_list(m_cache& cache, slab_status to)
{
	if (status_ == to) {
		return;
	}

	auto current_slab = std::move(*position_in_list_);
	switch (status_) {
		case slab_status::FREE:
			cache.slabs_free_.erase(position_in_list_);
			break;
		case slab_status::PARTIAL:
			cache.slabs_partial_.erase(position_in_list_);
			break;
		case slab_status::FULL:
			cache.slabs_full_.erase(position_in_list_);
			break;
	}

	switch (to) {
		case slab_status::FREE:
			cache.slabs_free_.push_back(std::move(current_slab));
			position_in_list_ = std::prev(cache.slabs_free_.end());
			break;
		case slab_status::PARTIAL:
			cache.slabs_partial_.push_back(std::move(current_slab));
			position_in_list_ = std::prev(cache.slabs_partial_.end());
			break;
		case slab_status::FULL:
			cache.slabs_full_.push_back(std::move(current_slab));
			position_in_list_ = std::prev(cache.slabs_full_.end());
			break;
	}

	status_ = to;
}

void* m_slab::alloc_object(size_t obj_size)
{
	if (free_objects_index_.empty()) {
		LOG_ERROR("no free objects");
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
	const size_t objs_index = (reinterpret_cast<uintptr_t>(addr) -
							   reinterpret_cast<uintptr_t>(base_addr_)) /
							  obj_size;

	free_objects_index_.push_back(objs_index);
	--num_in_use_;
}

} // namespace kernel::memory

std::unordered_map<void*, void*> aligned_to_raw_addr_map;

namespace kernel::memory {

void* alloc(size_t size, unsigned flags, int align)
{
	if (align != 1 && (align & (align - 1)) != 0) {
		LOG_ERROR("align must be a power of 2");
		return nullptr;
	}

	size = 1 << bit_width_ceil(size + align - 1);
	char name[20];
	sprintf(name, "cache-%d", static_cast<int>(size));

	auto* cache = get_cache_in_chain(name);
	if (cache == nullptr) {
		cache = &m_cache_create(name, size);
	}

	void* addr = cache->alloc();
	if (addr == nullptr) {
		LOG_ERROR("failed to allocate memory");
		return nullptr;
	}

	if (align != 1) {
		auto* aligned_addr = reinterpret_cast<void*>(
				align_up(reinterpret_cast<uintptr_t>(addr), align));

		if (aligned_addr != addr) {
			aligned_to_raw_addr_map[aligned_addr] = addr;
		}

		addr = aligned_addr;
	}

	if ((flags & ALLOC_ZEROED) != 0) {
		memset(addr, 0, size);
	}

	return addr;
}

void free(void* addr)
{
	auto it = aligned_to_raw_addr_map.find(addr);
	if (it != aligned_to_raw_addr_map.end()) {
		addr = it->second;
		aligned_to_raw_addr_map.erase(it);
	}

	page* p = get_page(addr);
	if (p == nullptr) {
		LOG_ERROR("invalid address");
		return;
	}

	m_cache* cache = p->cache();
	m_slab* slab = p->slab();

	slab->free_object(addr, cache->object_size());
	cache->decrease_num_active_objects();

	if (slab->status() == slab_status::FULL) {
		slab->move_list(*cache, slab_status::PARTIAL);
	}

	if (slab->is_empty()) {
		slab->move_list(*cache, slab_status::FREE);
		cache->decrease_num_active_slabs();
	}
}

std::list<std::unique_ptr<m_cache>> cache_chain;

void initialize_slab_allocator()
{
	LOG_INFO("Initializing slab allocator...");

	aligned_to_raw_addr_map = std::unordered_map<void*, void*>();

	cache_chain.clear();

	run_test_suite(register_slab_tests);

	LOG_INFO("Initializing slab allocator successfully.");
}

} // namespace kernel::memory