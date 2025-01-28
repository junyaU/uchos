#include "tests/test_cases/memory_test.hpp"
#include "memory/bootstrap_allocator.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"

void test_boot_allocator_basic()
{
	void* single_page = boot_allocator->allocate(PAGE_SIZE);
	ASSERT_NOT_NULL(single_page);
	boot_allocator->free(single_page, PAGE_SIZE);
}

void test_boot_allocator_multi_page()
{
	constexpr size_t NUM_PAGES = 3;
	void* multi_page = boot_allocator->allocate(PAGE_SIZE * NUM_PAGES);
	ASSERT_NOT_NULL(multi_page);

	// check if all pages are marked as allocated
	auto page_index = reinterpret_cast<uintptr_t>(multi_page) / PAGE_SIZE;
	for (size_t i = 0; i < NUM_PAGES; i++) {
		ASSERT_TRUE(boot_allocator->is_bit_set(page_index + i));
	}

	boot_allocator->free(multi_page, PAGE_SIZE * NUM_PAGES);

	// check if all pages are marked as free
	for (size_t i = 0; i < NUM_PAGES; i++) {
		ASSERT_FALSE(boot_allocator->is_bit_set(page_index + i));
	}
}

void test_boot_allocator_allocation_and_free()
{
	void* region1 = boot_allocator->allocate(PAGE_SIZE * 2);
	void* region2 = boot_allocator->allocate(PAGE_SIZE * 2);
	ASSERT_NOT_NULL(region1);
	ASSERT_NOT_NULL(region2);

	boot_allocator->free(region1, PAGE_SIZE * 2);

	// allocate the same region again
	void* region3 = boot_allocator->allocate(PAGE_SIZE * 2);
	ASSERT_EQ(region1, region3);

	boot_allocator->free(region2, PAGE_SIZE * 2);
	boot_allocator->free(region3, PAGE_SIZE * 2);
}

void test_boot_allocator_alignment()
{
	void* aligned_mem = boot_allocator->allocate(100);
	ASSERT_NOT_NULL(aligned_mem);
	ASSERT_EQ(reinterpret_cast<uintptr_t>(aligned_mem) % PAGE_SIZE, 0);

	boot_allocator->free(aligned_mem, PAGE_SIZE);
}

void register_bootstrap_allocator_tests()
{
	test_register("boot_allocator", test_boot_allocator_basic);
	test_register("boot_allocator_multi_page", test_boot_allocator_multi_page);
	test_register("boot_allocator_allocation_and_free",
				  test_boot_allocator_allocation_and_free);
	test_register("boot_allocator_alignment", test_boot_allocator_alignment);
}
