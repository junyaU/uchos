#pragma once

#include "file_system/fat.hpp"

struct path {
	file_system::directory_entry* current_dir;
	file_system::directory_entry* parent_dir;
	file_system::directory_entry* root_dir;

	bool is_root() const { return current_dir == root_dir; }
};

path init_path(file_system::directory_entry* root_dir);
