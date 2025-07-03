#pragma once

#include "file_system/fat.hpp"

struct path {
	kernel::fs::directory_entry* current_dir;
	kernel::fs::directory_entry* parent_dir;
	kernel::fs::directory_entry* root_dir;

	bool is_root() const { return current_dir == root_dir; }
};

path init_path(kernel::fs::directory_entry* root_dir);
