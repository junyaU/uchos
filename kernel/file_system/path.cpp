#include "file_system/path.hpp"

path init_path(kernel::fs::directory_entry* root_dir)
{
	return { root_dir, nullptr, root_dir };
}