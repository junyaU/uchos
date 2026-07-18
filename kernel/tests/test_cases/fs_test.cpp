/**
 * @file fs_test.cpp
 * @brief FAT32 file system test implementation
 */

#include "tests/test_cases/fs_test.hpp"
#include <cstdio>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/stat.hpp>
#include "fs/fat/fat.hpp"
#include "fs/fat/internal_common.hpp"
#include "fs/file_descriptor.hpp"
#include "fs/file_info.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"


using namespace kernel::fs::fat;

// 8.3 names are up to 12 characters ("ABCDEFGH.EXT") plus a null terminator;
// these buffers used to be 11 bytes and overflowed (issue #313).
static_assert(sizeof(Stat::name) >= 13, "Stat::name must hold an 8.3 name");
static_assert(sizeof(kernel::fs::FileInfo::name) >= 13,
			  "FileInfo::name must hold an 8.3 name");
static_assert(sizeof(kernel::fs::FileDescriptor::name) >= 13,
			  "FileDescriptor::name must hold an 8.3 name");
static_assert(sizeof(kernel::fs::FileCache::path) >= 13,
			  "FileCache::path must hold an 8.3 name");

/**
 * @brief Test basic file write within existing cluster
 */
void test_write_existing_file()
{
	// Create a test file entry
	kernel::fs::DirectoryEntry test_entry;
	memset(&test_entry, 0, sizeof(test_entry));
	memcpy(test_entry.name, "TEST    TXT", 11);
	test_entry.attribute = kernel::fs::entry_attribute::ARCHIVE;
	test_entry.file_size = 512; // Small file within one cluster
	test_entry.first_cluster_low = 10;
	test_entry.first_cluster_high = 0;

	// Verify file was created correctly
	ASSERT_EQ(test_entry.first_cluster(), 10);
	ASSERT_EQ(test_entry.file_size, 512);
}

/**
 * @brief Test file write and read consistency
 */
void test_write_read_verify()
{
	// This test would require full FAT32 task initialization
	// For Phase 1, we test the basic structures

	// Test write buffer allocation
	const size_t write_size = 256;
	void* write_buffer =
			kernel::memory::alloc(write_size, kernel::memory::ALLOC_ZEROED);
	ASSERT_NOT_NULL(write_buffer);

	// Fill buffer with test data
	const char* test_data = "Hello, FAT32 file system!";
	memcpy(write_buffer, test_data, strlen(test_data) + 1);

	// Verify buffer contents
	ASSERT_EQ(strcmp(static_cast<char*>(write_buffer), test_data), 0);

	// Clean up
	kernel::memory::free(write_buffer);
}

/**
 * @brief Test cluster calculation functions
 */
void test_cluster_calculations()
{
	// Test calc_start_sector function
	// This assumes standard FAT32 parameters
	// In real test, these values should be initialized from VOLUME_BPB

	// Mock cluster 2 should start after reserved sectors and FAT tables
	// const unsigned int expected_sector = VOLUME_BPB->reserved_sector_count +
	//                                    VOLUME_BPB->num_fats *
	//                                    VOLUME_BPB->fat_size_32;
	// ASSERT_EQ(calc_start_sector(2), expected_sector);

	// For now, just test that the function exists
	// Real test would require FAT32 task initialization
}

/**
 * @brief Test FAT table update
 */
void test_fat_table_update()
{
	// Test FAT table entry update
	// In Phase 1, we only test the structure, not persistence

	// This test would require FAT_TABLE to be initialized
	// For now, we verify the END_OF_CLUSTER_CHAIN constant
	ASSERT_EQ(kernel::fs::END_OF_CLUSTER_CHAIN, 0x0FFFFFFFLU);
}

/**
 * @brief Test file extension with new cluster allocation
 */
void test_write_extend_file()
{
	// Test creating a file entry with no initial cluster
	kernel::fs::DirectoryEntry test_entry;
	memset(&test_entry, 0, sizeof(test_entry));
	memcpy(test_entry.name, "EXTEND  TXT", 11);
	test_entry.attribute = kernel::fs::entry_attribute::ARCHIVE;
	test_entry.file_size = 0;
	test_entry.first_cluster_low = 0;
	test_entry.first_cluster_high = 0;

	// Verify initial state
	ASSERT_EQ(test_entry.first_cluster(), 0);
	ASSERT_EQ(test_entry.file_size, 0);

	// Simulate cluster allocation (would be done by allocate_cluster_chain)
	test_entry.first_cluster_low = 100;
	test_entry.first_cluster_high = 0;

	// Simulate file size update after write
	test_entry.file_size = 4096; // One cluster worth of data

	// Verify updated state
	ASSERT_EQ(test_entry.first_cluster(), 100);
	ASSERT_EQ(test_entry.file_size, 4096);
}

/**
 * @brief Test FAT table persistence structure
 */
void test_fat_table_persistence()
{
	// Test that we can create a FAT table structure
	// In a real test, this would verify write_fat_table_to_disk functionality

	// Simulate FAT table entry allocation
	const cluster_t test_cluster = 50;
	const cluster_t next_cluster = 51;

	// Verify cluster chain logic
	ASSERT_TRUE(test_cluster < next_cluster);
	ASSERT_TRUE(next_cluster != kernel::fs::END_OF_CLUSTER_CHAIN);
}

/**
 * @brief Test that generated fs ids are monotonically increasing and nonzero
 *
 * The old implementation cycled ids modulo the cache capacity, so ids
 * collided and reads were served from another file's cache (issue #313).
 */
void test_fs_id_monotonic_and_nonzero()
{
	fs_id_t prev = kernel::fs::generate_fs_id();
	ASSERT_TRUE(prev != 0);

	for (int i = 0; i < 200; ++i) {
		const fs_id_t id = kernel::fs::generate_fs_id();
		ASSERT_TRUE(id != 0);
		ASSERT_TRUE(id > prev);
		prev = id;
	}
}

/**
 * @brief Test that filling the cache beyond capacity terminates and evicts
 *
 * The old eviction loop had no iterator increment and spun forever once the
 * cache reached 50 entries (issue #313).
 */
void test_file_cache_eviction_terminates()
{
	// A private map keeps the test fully isolated from the live FS task,
	// which uses the global file_caches concurrently (issue #313: swapping
	// the global out mid-boot lost the shell's in-flight cache).
	kernel::fs::FileCacheMap local_caches;

	bool all_created = true;
	char path[13];
	for (int i = 0; i < 60; ++i) {
		snprintf(path, sizeof(path), "F%d", i);
		kernel::fs::FileCache* c = kernel::fs::create_file_cache(
				local_caches, path, 16, ProcessId::from_raw(1));
		all_created = all_created && c != nullptr && c->id != 0;
	}

	ASSERT_TRUE(all_created);
	ASSERT_TRUE(local_caches.size() <= 50);
	ASSERT_TRUE(kernel::fs::find_file_cache_by_path(local_caches, "F0") ==
				nullptr);
	ASSERT_TRUE(kernel::fs::find_file_cache_by_path(local_caches, "F59") !=
				nullptr);
}

/**
 * @brief Test that eviction removes the least recently used entry
 */
void test_file_cache_lru_order()
{
	kernel::fs::FileCacheMap local_caches;

	char path[13];
	for (int i = 0; i < 50; ++i) {
		snprintf(path, sizeof(path), "L%d", i);
		kernel::fs::create_file_cache(local_caches, path, 16,
									  ProcessId::from_raw(1));
	}

	// Touch the oldest entry so "L1" becomes the least recently used one
	ASSERT_TRUE(kernel::fs::find_file_cache_by_path(local_caches, "L0") !=
				nullptr);

	// Cache is full: this create must evict exactly the LRU entry ("L1")
	kernel::fs::create_file_cache(local_caches, "L50", 16,
								  ProcessId::from_raw(1));

	ASSERT_TRUE(kernel::fs::find_file_cache_by_path(local_caches, "L0") !=
				nullptr);
	ASSERT_TRUE(kernel::fs::find_file_cache_by_path(local_caches, "L1") ==
				nullptr);
	ASSERT_TRUE(kernel::fs::find_file_cache_by_path(local_caches, "L50") !=
				nullptr);
}

/**
 * @brief Test 8.3 name conversion at the maximum length boundary
 *
 * A full 8.3 name ("ABCDEFGH.EXT") is 12 characters plus null terminator and
 * used to overflow the 11-byte destination buffers (issue #313).
 */
void test_read_dir_entry_name_boundary()
{
	kernel::fs::DirectoryEntry entry;
	memset(&entry, 0, sizeof(entry));
	memcpy(entry.name, "ABCDEFGHEXT", 11);

	char dest[16];
	memset(dest, 0x7F, sizeof(dest));
	read_dir_entry_name_raw(entry, dest);

	ASSERT_EQ(strcmp(dest, "ABCDEFGH.EXT"), 0);
	ASSERT_EQ(strlen(dest), 12UL);
	// Nothing may be written past the 13 bytes an 8.3 name needs
	ASSERT_EQ(dest[13], 0x7F);
	ASSERT_TRUE(entry_name_is_equal(entry, "ABCDEFGH.EXT"));

	// A converted name must fit into Stat::name without truncation
	Stat s;
	memset(&s, 0, sizeof(s));
	read_dir_entry_name(entry, s.name);
	ASSERT_EQ(strcmp(s.name, "abcdefgh.ext"), 0);

	// Name without extension keeps trailing spaces trimmed
	kernel::fs::DirectoryEntry no_ext;
	memset(&no_ext, 0, sizeof(no_ext));
	memcpy(no_ext.name, "NOEXT      ", 11);
	read_dir_entry_name_raw(no_ext, dest);
	ASSERT_EQ(strcmp(dest, "NOEXT"), 0);
}

/**
 * @brief Test cluster chain allocation against a full disk
 *
 * allocate_cluster_chain / extend_cluster_chain used to scan past the end of
 * the FAT when no free cluster existed (issue #313).
 */
void test_cluster_chain_disk_full()
{
	// A private FAT keeps the test isolated from the live FS task; the
	// globals must never be swapped while the FS task may run (issue #313)
	constexpr size_t total_test_clusters = 8;
	constexpr uint32_t canary_value = 0xDEADBEEF;
	uint32_t fake_fat[total_test_clusters + 4];
	memset(fake_fat, 0, sizeof(fake_fat));
	fake_fat[0] = 0x0FFFFFF8;
	fake_fat[1] = 0x0FFFFFFF;
	for (size_t i = total_test_clusters; i < total_test_clusters + 4; ++i) {
		fake_fat[i] = canary_value;
	}

	const size_t free_before = count_free_clusters(fake_fat, total_test_clusters);
	const cluster_t first_chain =
			allocate_cluster_chain(fake_fat, total_test_clusters, 4);
	const size_t free_mid = count_free_clusters(fake_fat, total_test_clusters);

	// Only 2 clusters left: this must fail and roll back, not scan past the
	// end of the FAT
	const cluster_t failed_chain =
			allocate_cluster_chain(fake_fat, total_test_clusters, 4);
	const size_t free_after = count_free_clusters(fake_fat, total_test_clusters);

	// Fully exhaust the remaining clusters, then fail again
	const cluster_t second_chain =
			allocate_cluster_chain(fake_fat, total_test_clusters, 2);
	const cluster_t exhausted_chain =
			allocate_cluster_chain(fake_fat, total_test_clusters, 1);

	bool canaries_intact = true;
	for (size_t i = total_test_clusters; i < total_test_clusters + 4; ++i) {
		canaries_intact = canaries_intact && fake_fat[i] == canary_value;
	}

	ASSERT_EQ(free_before, 6UL);
	ASSERT_EQ(first_chain, 2UL);
	ASSERT_EQ(free_mid, 2UL);
	ASSERT_EQ(failed_chain, 0UL);
	ASSERT_EQ(free_after, 2UL);
	ASSERT_TRUE(second_chain != 0);
	ASSERT_EQ(exhausted_chain, 0UL);
	ASSERT_TRUE(canaries_intact);
}

/**
 * @brief Register all file system test cases
 */
void register_fs_tests()
{
	test_register("test_write_existing_file", test_write_existing_file);
	test_register("test_write_read_verify", test_write_read_verify);
	test_register("test_cluster_calculations", test_cluster_calculations);
	test_register("test_fat_table_update", test_fat_table_update);
	test_register("test_write_extend_file", test_write_extend_file);
	test_register("test_fat_table_persistence", test_fat_table_persistence);
	test_register("test_fs_id_monotonic_and_nonzero",
				  test_fs_id_monotonic_and_nonzero);
	test_register("test_file_cache_eviction_terminates",
				  test_file_cache_eviction_terminates);
	test_register("test_file_cache_lru_order", test_file_cache_lru_order);
	test_register("test_read_dir_entry_name_boundary",
				  test_read_dir_entry_name_boundary);
	test_register("test_cluster_chain_disk_full", test_cluster_chain_disk_full);
}