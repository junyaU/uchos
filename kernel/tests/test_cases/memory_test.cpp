#include "tests/test_cases/memory_test.hpp"
#include "memory/bootstrap_allocator.hpp"
#include "memory/buddy_system.hpp"
#include "memory/slab.hpp"
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

void test_buddy_system_basic()
{
	// Test basic allocation
	void* single_page = memory_manager->allocate(PAGE_SIZE);
	ASSERT_NOT_NULL(single_page);

	// Test alignment
	ASSERT_EQ(reinterpret_cast<uintptr_t>(single_page) % PAGE_SIZE, 0);

	// Test deallocation
	memory_manager->free(single_page, PAGE_SIZE);
}

void test_buddy_system_multi_page()
{
	// Test allocation of multiple pages
	constexpr size_t NUM_PAGES = 3;
	void* multi_page = memory_manager->allocate(PAGE_SIZE * NUM_PAGES);
	ASSERT_NOT_NULL(multi_page);

	// Test alignment for larger allocations
	ASSERT_EQ(reinterpret_cast<uintptr_t>(multi_page) % PAGE_SIZE, 0);

	memory_manager->free(multi_page, PAGE_SIZE * NUM_PAGES);
}

void test_buddy_system_split_and_merge()
{
	// Allocate a large block that will require splitting
	void* large_block = memory_manager->allocate(PAGE_SIZE * 4);
	ASSERT_NOT_NULL(large_block);

	// Allocate a smaller block
	void* small_block = memory_manager->allocate(PAGE_SIZE);
	ASSERT_NOT_NULL(small_block);

	// Free both blocks
	memory_manager->free(large_block, PAGE_SIZE * 4);
	memory_manager->free(small_block, PAGE_SIZE);
}

void test_buddy_system_error_handling()
{
	// Test allocation with invalid size (0)
	void* invalid_alloc = memory_manager->allocate(0);
	ASSERT_NULL(invalid_alloc);

	// Test allocation with very large size
	void* too_large =
			memory_manager->allocate(PAGE_SIZE * (1ULL << (MAX_ORDER + 1)));
	ASSERT_NULL(too_large);
}

void register_buddy_system_tests()
{
	test_register("buddy_system_basic", test_buddy_system_basic);
	test_register("buddy_system_multi_page", test_buddy_system_multi_page);
	test_register("buddy_system_split_and_merge", test_buddy_system_split_and_merge);
	test_register("buddy_system_error_handling", test_buddy_system_error_handling);
}

void test_slab_basic()
{
	// Test basic allocation and free
	void* ptr = kmalloc(64, KMALLOC_UNINITIALIZED);
	ASSERT_NOT_NULL(ptr);
	kfree(ptr);
}

void test_slab_cache_creation()
{
	// Test cache creation with specific size
	constexpr size_t OBJ_SIZE = 128;
	auto& cache = m_cache_create("test-cache", OBJ_SIZE);

	// Allocate from the cache
	void* obj = cache.alloc();
	ASSERT_NOT_NULL(obj);

	// Get the cache by name and verify it exists
	auto* found_cache = get_cache_in_chain(const_cast<char*>("test-cache"));
	ASSERT_NOT_NULL(found_cache);
	ASSERT_EQ(found_cache->object_size(), OBJ_SIZE);
}

void test_slab_aligned_allocation()
{
	// Test allocation with alignment
	constexpr size_t ALIGN = 16;
	void* ptr = kmalloc(64, KMALLOC_UNINITIALIZED, ALIGN);
	ASSERT_NOT_NULL(ptr);
	ASSERT_EQ(reinterpret_cast<uintptr_t>(ptr) % ALIGN, 0);
	kfree(ptr);
}

void test_slab_zeroed_allocation()
{
	// Test zero-initialized allocation
	constexpr size_t SIZE = 64;
	void* ptr = kmalloc(SIZE, KMALLOC_ZEROED);
	ASSERT_NOT_NULL(ptr);

	// Verify memory is zeroed
	uint8_t* bytes = static_cast<uint8_t*>(ptr);
	for (size_t i = 0; i < SIZE; ++i) {
		ASSERT_EQ(bytes[i], 0);
	}

	kfree(ptr);
}

void test_slab_error_handling()
{
	// Test invalid alignment (not power of 2)
	void* ptr = kmalloc(64, KMALLOC_UNINITIALIZED, 3);
	ASSERT_NULL(ptr);

	// Test allocation with size 0
	ptr = kmalloc(0, KMALLOC_UNINITIALIZED);
	ASSERT_NULL(ptr);
}

void register_slab_tests()
{
	test_register("slab_basic", test_slab_basic);
	test_register("slab_cache_creation", test_slab_cache_creation);
	test_register("slab_aligned_allocation", test_slab_aligned_allocation);
	test_register("slab_zeroed_allocation", test_slab_zeroed_allocation);
	test_register("slab_error_handling", test_slab_error_handling);
}
