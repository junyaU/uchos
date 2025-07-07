/**
 * @file common.hpp
 * @brief FAT32 public API declarations
 */

#pragma once

#include <libs/common/message.hpp>

namespace kernel::fs::fat
{

// Execute ELF file loaded from FAT32
void execute_file(void* data, const char* name, const char* args);

} // namespace kernel::fs::fat
