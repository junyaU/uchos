/**
 * @file open_files.cpp
 * @brief FS-owned open-file ledger (issue #315 3b-8)
 *
 * The FS keeps every piece of per-open-file state here — directory entry,
 * offset, name — keyed by an opaque handle it issues at open time. Clients
 * never see this table; the kernel routing entry only stores the handle.
 * This is exactly the state that used to live in each client's Task
 * fd_table, which the FS had to reach into across the task boundary.
 */

#include <cstring>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "internal_common.hpp"
#include "log/log.hpp"
#include "task/task.hpp"

namespace kernel::fs::fat
{

namespace
{
OpenFileRecord open_files[MAX_OPEN_FILES];

/// Monotonic handle source; 0 is never issued (it means "no handle").
uint32_t next_handle = 1;

/**
 * @brief Free records whose owner task no longer exists
 *
 * Process exit does not notify the FS (that would recreate the reverse
 * dependency issue #315 just removed), so records of dead owners linger
 * until this sweep runs on allocation pressure. A record whose handle was
 * fork-shared with a still-living child is swept too — a documented edge
 * until ring-3 services bring real per-channel accounting (3b-10).
 */
void sweep_dead_owners()
{
	for (auto& r : open_files) {
		if (r.handle != 0 && kernel::task::get_task(r.owner) == nullptr) {
			r = OpenFileRecord{};
		}
	}
}
} // namespace

uint32_t open_file_create(kernel::fs::DirectoryEntry* entry,
						  const char* name,
						  ProcessId owner)
{
	for (int attempt = 0; attempt < 2; ++attempt) {
		for (auto& r : open_files) {
			if (r.handle != 0) {
				continue;
			}

			r.handle = next_handle++;
			if (next_handle == 0) {
				next_handle = 1; // skip the "no handle" value on wrap
			}
			r.owner = owner;
			r.entry = entry;
			r.offset = 0;
			r.refcount = 1;
			strncpy(r.name, name, sizeof(r.name) - 1);
			r.name[sizeof(r.name) - 1] = '\0';

			return r.handle;
		}

		sweep_dead_owners();
	}

	LOG_ERROR("open-file ledger full (owner %d, file %s)", owner.raw(), name);
	return 0;
}

OpenFileRecord* open_file_get(uint32_t handle)
{
	if (handle == 0) {
		return nullptr;
	}

	for (auto& r : open_files) {
		if (r.handle == handle) {
			return &r;
		}
	}

	return nullptr;
}

void open_file_addref(uint32_t handle)
{
	OpenFileRecord* r = open_file_get(handle);
	if (r != nullptr) {
		++r->refcount;
	}
}

void open_file_release(uint32_t handle)
{
	OpenFileRecord* r = open_file_get(handle);
	if (r == nullptr) {
		return;
	}

	if (--r->refcount <= 0) {
		*r = OpenFileRecord{};
	}
}

} // namespace kernel::fs::fat
