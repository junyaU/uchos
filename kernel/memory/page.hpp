#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

const int PAGE_SIZE = 4096;

class page
{
public:
	page() : status_{ 0 }, ptr_{ nullptr }, adjacent_ptr_{ nullptr } {}

	bool is_free() const;
	bool is_used() const;
	bool is_reserved() const;

	void* ptr() const { return ptr_; }
	void* adjacent_ptr() const { return adjacent_ptr_; }

	void set_ptr(void* ptr);
	void set_adjacent_ptr(void* adjacent_ptr) { adjacent_ptr_ = adjacent_ptr; }

	void set_free();
	void set_used();
	void set_reserved();

private:
	int status_;
	void* ptr_;
	void* adjacent_ptr_;
};

const int MAX_PAGES = 260000;

page* alloc_page();
