#pragma once

class m_cache;
class m_slab;

#include <cstddef>
#include <cstdint>
#include <vector>

constexpr size_t PAGE_SIZE = 4096;

class page
{
public:
	page() : status_{ 0 }, ptr_{ nullptr } {}

	size_t index() const { return reinterpret_cast<uintptr_t>(ptr_) / PAGE_SIZE; }

	bool is_free() const { return status_ == 0; }
	bool is_used() const { return status_ == 1; }

	void* ptr() const { return ptr_; }

	void set_ptr(void* ptr) { ptr_ = ptr; }

	void set_free() { status_ = 0; }
	void set_used() { status_ = 1; }

	void set_cache(m_cache* cache) { cache_ = cache; }
	void set_slab(m_slab* slab) { slab_ = slab; }

	m_cache* cache() const { return cache_; }
	m_slab* slab() const { return slab_; }

private:
	int status_;
	m_cache* cache_;
	m_slab* slab_;
	void* ptr_;
};

extern std::vector<page> pages;

void initialize_pages();

page* get_page(void* ptr);

void get_memory_usage(size_t* total_mem, size_t* used_mem);
