#pragma once

#include <cstddef>
#include <list>
#include <vector>

class m_object
{
public:
	m_object() = default;

	bool is_allocated() const { return is_allocated_; }
	void allocate() { is_allocated_ = true; }
	void free() { is_allocated_ = false; }

private:
	bool is_allocated_ = false;
};

class m_slab
{
public:
	m_slab(void* base_addr, size_t num_objs);

	void set_position_in_list(std::list<std::unique_ptr<m_slab>>::iterator it)
	{
		position_in_list_ = it;
	}

	bool is_full() const { return num_in_use_ == objects_.size(); }

	bool is_empty() const { return num_in_use_ == 0; }

	bool was_previously_full() const { return objects_.size() - num_in_use_ == 1; }

	void move_list(std::list<std::unique_ptr<m_slab>>& from,
				   std::list<std::unique_ptr<m_slab>>& to);

	void* alloc_object(size_t obj_size);
	void free_object(void* addr, size_t obj_size);

private:
	std::list<std::unique_ptr<m_slab>>::iterator position_in_list_;
	std::vector<m_object> objects_;
	void* base_addr_;
	size_t num_in_use_;
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

m_cache& m_cache_create(char* name, size_t object_size);

void* kmalloc(size_t size);

void kfree(void* addr);

void initialize_slab_allocator();