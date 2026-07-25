/**
 * @file file_descriptor.hpp
 * @brief File descriptor entry: a per-process I/O routing record
 *
 * Lives in libs/common because the entry is part of the kernel/service
 * contract (issue #315): the kernel only follows the routing stored here,
 * and the services that create entries (FS server, future console server)
 * will manage them from user space in Phase 3b.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include "process_id.hpp"

/**
 * @brief How I/O on a descriptor is routed
 *
 * The syscall layer follows this instead of interpreting fd numbers or
 * name strings; whoever creates the entry decides the routing.
 */
enum class FdRoute : uint8_t {
	NONE = 0, ///< No I/O possible through this entry
	CONSOLE,  ///< write = one-way NOTIFY_WRITE to dest (console output)
	FS,		  ///< read/write = correlation-matched FS_* RPC to dest
};

/**
 * @brief File descriptor structure
 *
 * Represents an open I/O handle for a process: where operations on it are
 * routed (route/dest) plus the file state the FS still keeps here until it
 * owns fd management itself (issue #315 3b).
 */
struct FileDescriptor {
	char name[13];	///< Label: 8.3 file name for FS fds, "stdout" etc. for std fds
	FdRoute route;	///< Protocol the syscall layer uses for I/O on this fd
	ProcessId dest; ///< Service task the I/O is sent to
	size_t size;	///< File size in bytes
	size_t offset;	///< Current read/write position

	/**
	 * @brief Check if this file descriptor is unused
	 * @return true if the descriptor is not in use (empty name)
	 */
	bool is_unused() const { return name[0] == '\0'; }

	/**
	 * @brief Check if this file descriptor is in use
	 * @return true if the descriptor is in use (non-empty name)
	 */
	bool is_used() const { return name[0] != '\0'; }

	/**
	 * @brief Clear this file descriptor entry
	 *
	 * Resets all fields to their default values, marking the descriptor as
	 * unused.
	 */
	void clear()
	{
		name[0] = '\0';
		route = FdRoute::NONE;
		dest = process_ids::INVALID;
		size = 0;
		offset = 0;
	}

	/**
	 * @brief Check if this file descriptor has the specified name
	 * @param target_name The name to compare against
	 * @return true if the name matches, false otherwise
	 */
	bool has_name(const char* target_name) const
	{
		return strcmp(name, target_name) == 0;
	}
};
