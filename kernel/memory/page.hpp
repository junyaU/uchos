#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

const int PAGE_SIZE = 4096;

class page
{
public:
	page() : status_{ 0 }, ptr_{ nullptr } {}

	size_t index() const { return reinterpret_cast<uintptr_t>(ptr_) / PAGE_SIZE; }

	bool is_free() const { return status_ == 0; }
	bool is_used() const { return status_ == 1; }
	bool is_reserved() const;

	void* ptr() const { return ptr_; }

	void set_ptr(void* ptr) { ptr_ = ptr; }

	void set_free() { status_ = 0; }
	void set_used() { status_ = 1; }
	void set_reserved();

private:
	int status_;
	void* ptr_;
};

extern std::vector<page> pages;

void initialize_pages();

void print_available_memory();