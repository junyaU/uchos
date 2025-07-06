#include "file_system/path.hpp"
#include "file_system/fat.hpp"
#include <cstring>

path init_path(kernel::fs::directory_entry* root_dir)
{
	path p = { root_dir, nullptr, root_dir };
	strcpy(p.current_dir_name, "/");
	strcpy(p.full_path, "/");
	return p;
}