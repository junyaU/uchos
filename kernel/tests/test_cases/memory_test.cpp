#include "tests/test_cases/memory_test.hpp"
#include "memory/bootstrap_allocator.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"

void test_bootstrap_allocator()
{
	void* single_page = boot_allocator->allocate(PAGE_SIZE);
	ASSERT_NOT_NULL(single_page);
	boot_allocator->free(single_page, PAGE_SIZE);
}

void register_bootstrap_allocator_tests()
{
	test_register("bootstrap_allocator", test_bootstrap_allocator);
}
