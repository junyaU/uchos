#include "file_info.hpp"
#include <cstddef>
#include <libs/common/types.hpp>

size_t id_offset = 0;

fs_id_t generate_unique_fs_id()
{
	// TODO: handle overflow
	return id_offset++;
}
