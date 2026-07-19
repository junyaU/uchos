#include "slab.hpp"
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>
#include "bit_utils.hpp"
#include "buddy_system.hpp"
#include "heap_debug.hpp"
#include "log/log.hpp"
#include "page.hpp"

namespace kernel::memory
{

MCache* get_cache_in_chain(const char* name)
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

MCache::MCache(const char* name, size_t object_size)
	: object_size_(object_size),
	  num_active_objects_(0),
	  num_total_objects_(0),
	  num_active_slabs_(0),
	  num_total_slabs_(0),
	  num_pages_per_slab_(0)
{
	strncpy(name_, name, sizeof(name_) - 1);
	name_[sizeof(name_) - 1] = '\0';

	if (object_size <= kernel::memory::PAGE_SIZE) {
		num_pages_per_slab_ = 1;
	} else {
		num_pages_per_slab_ = object_size / kernel::memory::PAGE_SIZE;
	}
}

MSlab::MSlab(void* base_addr, size_t num_objs)
	: base_addr_(base_addr), num_in_use_(0)
{
	objects_.resize(num_objs);

	for (size_t i = 0; i < num_objs; i++) {
		free_objects_index_.push_back(i);
	}
}

MCache& m_cache_create(const char* name, size_t obj_size)
{
	obj_size = 1UL << bit_width_ceil(obj_size);

	char temp_name[20];
	if (name == nullptr) {
		sprintf(temp_name, "cache-%d", static_cast<int>(obj_size));
		name = temp_name;
	}

	cache_chain.push_back(std::make_unique<kernel::memory::MCache>(name, obj_size));

	return *cache_chain.back();
}

bool MCache::grow()
{
	size_t const bytes_per_slab = num_pages_per_slab_ * kernel::memory::PAGE_SIZE;
	void* addr = kernel::memory::memory_manager->allocate(bytes_per_slab);
	if (addr == nullptr) {
		LOG_ERROR("failed to allocate memory");
		return false;
	}

	size_t const num_objs = bytes_per_slab / object_size_;
	auto slab = std::make_unique<MSlab>(addr, num_objs);

	// Mark every page of the slab so that free() can resolve the owning
	// cache/slab from any object address.
	for (size_t i = 0; i < num_pages_per_slab_; ++i) {
		Page* page = get_page(reinterpret_cast<char*>(addr) +
							  i * kernel::memory::PAGE_SIZE);
		if (page == nullptr) {
			LOG_ERROR("failed to look up page for slab at %p", addr);
			kernel::memory::memory_manager->free(addr, bytes_per_slab);
			return false;
		}

		page->set_cache(this);
		page->set_slab(slab.get());
	}

	++num_total_slabs_;
	num_total_objects_ += num_objs;

	slab->set_status(SlabStatus::FREE);
	slabs_free_.push_back(std::move(slab));
	auto last_it = std::prev(slabs_free_.end());
	(*last_it)->set_position_in_list(last_it);

	return true;
}

void* MCache::alloc()
{
	if (slabs_partial_.empty()) {
		if (slabs_free_.empty() && !grow()) {
			LOG_ERROR("failed to grow");
			return nullptr;
		}

		auto* free_slab = slabs_free_.front().get();
		free_slab->move_list(*this, SlabStatus::PARTIAL);

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
		full_slab->move_list(*this, SlabStatus::FULL);
	}

	return addr;
}

void MSlab::move_list(MCache& cache, SlabStatus to)
{
	if (status_ == to) {
		return;
	}

	auto current_slab = std::move(*position_in_list_);
	switch (status_) {
		case SlabStatus::FREE:
			cache.slabs_free_.erase(position_in_list_);
			break;
		case SlabStatus::PARTIAL:
			cache.slabs_partial_.erase(position_in_list_);
			break;
		case SlabStatus::FULL:
			cache.slabs_full_.erase(position_in_list_);
			break;
	}

	switch (to) {
		case SlabStatus::FREE:
			cache.slabs_free_.push_back(std::move(current_slab));
			position_in_list_ = std::prev(cache.slabs_free_.end());
			break;
		case SlabStatus::PARTIAL:
			cache.slabs_partial_.push_back(std::move(current_slab));
			position_in_list_ = std::prev(cache.slabs_partial_.end());
			break;
		case SlabStatus::FULL:
			cache.slabs_full_.push_back(std::move(current_slab));
			position_in_list_ = std::prev(cache.slabs_full_.end());
			break;
	}

	status_ = to;
}

void* MSlab::alloc_object(size_t obj_size)
{
	if (free_objects_index_.empty()) {
		LOG_ERROR("no free objects");
		return nullptr;
	}

	size_t const obj_index = free_objects_index_.front();

	free_objects_index_.pop_front();

	objects_[obj_index].increase_usage_count();
	objects_[obj_index].set_in_use(true);

	num_in_use_++;

	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base_addr_) +
								   obj_index * obj_size);
}

bool MSlab::free_object(void* addr, size_t obj_size)
{
	const uintptr_t offset = reinterpret_cast<uintptr_t>(addr) -
							 reinterpret_cast<uintptr_t>(base_addr_);
	if (offset % obj_size != 0) {
		LOG_ERROR("free: %p is not an object boundary", addr);
		return false;
	}

	const size_t objs_index = offset / obj_size;
	if (objs_index >= objects_.size()) {
		LOG_ERROR("free: %p is out of slab range", addr);
		return false;
	}

	if (!objects_[objs_index].is_in_use()) {
		LOG_ERROR("double free detected at address: %p", addr);
		return false;
	}

	objects_[objs_index].set_in_use(false);
	free_objects_index_.push_back(objs_index);
	--num_in_use_;

	return true;
}

bool MSlab::is_object_in_use(void* addr, size_t obj_size) const
{
	const uintptr_t offset = reinterpret_cast<uintptr_t>(addr) -
							 reinterpret_cast<uintptr_t>(base_addr_);
	if (offset % obj_size != 0) {
		return false;
	}

	const size_t objs_index = offset / obj_size;
	if (objs_index >= objects_.size()) {
		return false;
	}

	return objects_[objs_index].is_in_use();
}

} // namespace kernel::memory

std::unordered_map<void*, void*> aligned_to_raw_addr_map;

namespace kernel::memory
{

// Largest single allocation: the biggest block the buddy system can back
// a slab with.
constexpr size_t MAX_ALLOC_SIZE = (1UL << MAX_ORDER) * PAGE_SIZE;

void* alloc(size_t size, unsigned flags, int align)
{
	if (size == 0) {
		LOG_ERROR("alloc: size must be non-zero");
		return nullptr;
	}

	if (align != 1 && (align & (align - 1)) != 0) {
		LOG_ERROR("align must be a power of 2");
		return nullptr;
	}

	if (size > MAX_ALLOC_SIZE - (static_cast<size_t>(align) - 1)) {
		LOG_ERROR("alloc: size %lu is too large", size);
		return nullptr;
	}

#ifdef KERNEL_HEAP_DEBUG_ENABLED
	// Reserve room for the head and tail redzones around the payload before
	// rounding up to a cache size class.
	const size_t user_size = size;
	size = 1UL << bit_width_ceil(size + align - 1 + heap_debug::redzone_reserve());
	if (size > MAX_ALLOC_SIZE) {
		LOG_ERROR("alloc: size %lu too large once heap-debug redzones are added",
				  user_size);
		return nullptr;
	}
#else
	size = 1UL << bit_width_ceil(size + align - 1);
#endif

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

#ifdef KERNEL_HEAP_DEBUG_ENABLED
	// on_alloc lays the tail redzone, checks the freed-poison, records
	// provenance, and hands back the object boundary (natural alignment kept).
	// The +align-1 in the size above still guarantees object_size >= align.
	addr = heap_debug::on_alloc(addr, user_size, cache->object_size(),
								__builtin_return_address(0));

	if ((flags & ALLOC_ZEROED) != 0) {
		memset(addr, 0, user_size);
	}

	return addr;
#else
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
#endif
}

void free(void* addr)
{
	if (addr == nullptr) {
		return;
	}

#ifdef KERNEL_HEAP_DEBUG_ENABLED
	// Validate the payload pointer, check its redzones, poison the object, and
	// translate back to the raw slab object. A double/invalid free returns
	// nullptr after reporting the violation.
	addr = heap_debug::on_free(addr, __builtin_return_address(0));
	if (addr == nullptr) {
		return;
	}
#else
	auto it = aligned_to_raw_addr_map.find(addr);
	if (it != aligned_to_raw_addr_map.end()) {
		addr = it->second;
		aligned_to_raw_addr_map.erase(it);
	}
#endif

	Page* p = get_page(addr);
	if (p == nullptr) {
		LOG_ERROR("invalid address");
		return;
	}

	MCache* cache = p->cache();
	MSlab* slab = p->slab();
	if (cache == nullptr || slab == nullptr) {
		LOG_ERROR("free: %p was not allocated by the slab allocator", addr);
		return;
	}

	if (!slab->free_object(addr, cache->object_size())) {
		return;
	}

	cache->decrease_num_active_objects();

	if (slab->status() == SlabStatus::FULL) {
		slab->move_list(*cache, SlabStatus::PARTIAL);
	}

	if (slab->is_empty()) {
		slab->move_list(*cache, SlabStatus::FREE);
		cache->decrease_num_active_slabs();
	}
}

bool is_slab_object_in_use(void* addr)
{
	if (addr == nullptr) {
		return false;
	}

#ifdef KERNEL_HEAP_DEBUG_ENABLED
	// A live payload pointer maps to its raw object; anything else falls
	// through unchanged and is reported as not in use.
	void* raw = heap_debug::raw_from_user(addr);
	if (raw != nullptr) {
		addr = raw;
	}
#else
	auto it = aligned_to_raw_addr_map.find(addr);
	if (it != aligned_to_raw_addr_map.end()) {
		addr = it->second;
	}
#endif

	Page* p = get_page(addr);
	if (p == nullptr || p->cache() == nullptr || p->slab() == nullptr) {
		return false;
	}

	return p->slab()->is_object_in_use(addr, p->cache()->object_size());
}

std::list<std::unique_ptr<MCache>> cache_chain;

void initialize_slab_allocator()
{
	LOG_INFO("Initializing slab allocator...");

	aligned_to_raw_addr_map = std::unordered_map<void*, void*>();

	cache_chain.clear();

#ifdef KERNEL_HEAP_DEBUG_ENABLED
	heap_debug::initialize();
#endif

	LOG_INFO("Initializing slab allocator successfully.");
}

} // namespace kernel::memory