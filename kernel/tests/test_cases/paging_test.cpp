#include "tests/test_cases/paging_test.hpp"
#include <cstdint>
#include "memory/buddy_system.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/slab.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"

namespace
{
// PML4 index 256 = start of the user half of the address space
constexpr uint64_t TEST_USER_VADDR = 0xffff'8000'1234'5000;
} // namespace

void test_clean_page_tables_frees_user_pages()
{
	using namespace kernel::memory;

	page_table_entry* table = new_page_table();
	ASSERT_NOT_NULL(table);

	const vaddr_t addr{ TEST_USER_VADDR };
	ASSERT_EQ(setup_page_table(table, 4, addr, 2, true), 0);

	auto* pte = get_pte(table, addr, 1);
	ASSERT_NOT_NULL(pte);
	ASSERT_EQ(pte->bits.owned, 1UL);
	void* data_page = pte->get_next_level_table();
	ASSERT_TRUE(is_slab_object_in_use(data_page));

	clean_page_tables(table);

	// Owned data pages and the table pages themselves are released
	ASSERT_FALSE(is_slab_object_in_use(data_page));
	ASSERT_FALSE(is_slab_object_in_use(table));
}

void test_cow_pages_freed_with_last_reference()
{
	using namespace kernel::memory;

	page_table_entry* origin = new_page_table();
	ASSERT_NOT_NULL(origin);

	const vaddr_t addr{ TEST_USER_VADDR };
	ASSERT_EQ(setup_page_table(origin, 4, addr, 1, true), 0);
	void* data_page = get_pte(origin, addr, 1)->get_next_level_table();

	// Fork-style CoW: two clones sharing the same data page read-only
	page_table_entry* clone_a = clone_page_table(origin, false);
	page_table_entry* clone_b = clone_page_table(origin, false);
	ASSERT_NOT_NULL(clone_a);
	ASSERT_NOT_NULL(clone_b);

	auto* pte_a = get_pte(clone_a, addr, 1);
	ASSERT_NOT_NULL(pte_a);
	ASSERT_EQ(pte_a->get_next_level_table(), data_page);
	ASSERT_EQ(pte_a->bits.writable, 0UL);
	ASSERT_EQ(pte_a->bits.owned, 1UL);

	// Releasing the pre-fork snapshot keeps the shared page alive
	clean_page_tables(origin);
	ASSERT_TRUE(is_slab_object_in_use(data_page));

	clean_page_tables(clone_a);
	ASSERT_TRUE(is_slab_object_in_use(data_page));

	// The last reference frees it (issue #313 CoW page leak)
	clean_page_tables(clone_b);
	ASSERT_FALSE(is_slab_object_in_use(data_page));
}

void test_clean_page_tables_skips_foreign_frames()
{
	using namespace kernel::memory;

	page_table_entry* table = new_page_table();
	ASSERT_NOT_NULL(table);

	const vaddr_t addr{ TEST_USER_VADDR };
	ASSERT_EQ(setup_page_table(table, 4, addr, 1, true), 0);

	// Map a buddy-owned frame the way IPC shared memory does
	void* frame = memory_manager->allocate(PAGE_SIZE);
	ASSERT_NOT_NULL(frame);
	vaddr_t mapped{ 0 };
	ASSERT_EQ(map_frame_to_vaddr(table, reinterpret_cast<uint64_t>(frame), 1,
								 &mapped),
			  OK);

	auto* pte = get_pte(table, mapped, 1);
	ASSERT_NOT_NULL(pte);
	ASSERT_EQ(pte->bits.owned, 0UL);

	clean_page_tables(table);

	// The foreign frame is only unmapped, never freed
	Page* page = get_page(frame);
	ASSERT_NOT_NULL(page);
	ASSERT_TRUE(page->is_used());

	memory_manager->free(frame, PAGE_SIZE);
}

void register_paging_tests()
{
	test_register("clean_page_tables_frees_user_pages",
				  test_clean_page_tables_frees_user_pages);
	test_register("cow_pages_freed_with_last_reference",
				  test_cow_pages_freed_with_last_reference);
	test_register("clean_page_tables_skips_foreign_frames",
				  test_clean_page_tables_skips_foreign_frames);
}
