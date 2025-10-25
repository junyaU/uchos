#include "fs/path.hpp"
#include <cstring>
#include "fs/fat/fat.hpp"

Path init_path(kernel::fs::DirectoryEntry* root_dir)
{
	Path p = { root_dir, nullptr, root_dir };
	strncpy(p.current_dir_name, "/", sizeof(p.current_dir_name) - 1);
	p.current_dir_name[sizeof(p.current_dir_name) - 1] = '\0';
	strncpy(p.full_path, "/", sizeof(p.full_path) - 1);
	p.full_path[sizeof(p.full_path) - 1] = '\0';
	return p;
}
