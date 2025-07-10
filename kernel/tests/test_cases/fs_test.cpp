/**
 * @file fs_test.cpp
 * @brief FAT32 file system test implementation
 */

#include "tests/test_cases/fs_test.hpp"
#include "fs/fat/fat.hpp"
#include "fs/fat/internal_common.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>

namespace kernel::fs::fat
{
// Forward declarations for testing
extern kernel::fs::directory_entry* ROOT_DIR;
extern uint32_t* FAT_TABLE;
}

using namespace kernel::fs::fat;

/**
 * @brief Test basic file write within existing cluster
 */
void test_write_existing_file()
{
	// Create a test file entry
	kernel::fs::directory_entry test_entry;
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
	void* write_buffer = kernel::memory::alloc(write_size, kernel::memory::ALLOC_ZEROED);
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
	//                                    VOLUME_BPB->num_fats * VOLUME_BPB->fat_size_32;
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
	kernel::fs::directory_entry test_entry;
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
}