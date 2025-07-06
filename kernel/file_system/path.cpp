#include "file_system/path.hpp"
#include "file_system/fat.hpp"
#include <cstring>

path init_path(kernel::fs::directory_entry* root_dir)
{
	path p = { root_dir, nullptr, root_dir };
	strncpy(p.current_dir_name, "/", sizeof(p.current_dir_name) - 1);
	p.current_dir_name[sizeof(p.current_dir_name) - 1] = '\0';
	strncpy(p.full_path, "/", sizeof(p.full_path) - 1);
	p.full_path[sizeof(p.full_path) - 1] = '\0';
	return p;
}
