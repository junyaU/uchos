#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

const int PAGE_SIZE = 4096;

class page
{
public:
	page() : status_{ 0 }, ptr_{ nullptr } {}

	bool is_free() const;
	bool is_used() const;
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