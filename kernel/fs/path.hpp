/**
 * @file path.hpp
 * @brief File system path management and directory navigation
 */

#pragma once

#include "fs/fat/fat.hpp"

/**
 * @brief Represents a file system path context
 *
 * This structure maintains the current directory context for file system
 * operations, including references to the current directory, parent directory,
 * and root directory. It enables directory navigation and path resolution.
 */
struct Path {
	/**
	 * @brief Pointer to the current directory entry
	 *
	 * Points to the directory entry representing the current working directory.
	 */
	kernel::fs::DirectoryEntry* current_dir;

	/**
	 * @brief Pointer to the parent directory entry
	 *
	 * Points to the directory entry of the current directory's parent.
	 * For the root directory, this may point to itself.
	 */
	kernel::fs::DirectoryEntry* parent_dir;

	/**
	 * @brief Pointer to the root directory entry
	 *
	 * Points to the file system's root directory entry.
	 * This remains constant throughout the path's lifetime.
	 */
	kernel::fs::DirectoryEntry* root_dir;

	/**
	 * @brief Current directory name
	 *
	 * Stores the name of the current directory for pwd command.
	 * For root directory, this should be "/".
	 */
	char current_dir_name[13];

	/**
	 * @brief Full directory path
	 *
	 * Stores the full path from root to current directory.
	 * Example: "/documents/reports"
	 */
	char full_path[256];

	/**
	 * @brief Check if the current directory is the root directory
	 *
	 * @return true if the current directory is the root directory
	 * @return false otherwise
	 */
	bool is_root() const { return current_dir == root_dir; }
};

/**
 * @brief Initialize a new path structure
 *
 * Creates and initializes a path structure with the given root directory.
 * The current directory and parent directory are initially set to the root.
 *
 * @param root_dir Pointer to the root directory entry
 * @return path Initialized path structure
 *
 * @note The root_dir parameter must not be nullptr
 */
Path init_path(kernel::fs::DirectoryEntry* root_dir);
