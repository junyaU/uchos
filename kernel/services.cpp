/**
 * @file services.cpp
 * @brief Boot-time service manifest: the single place that knows which
 * services UCHos starts
 *
 * Extracted from kernel/task/task.cpp (issue #315): keeping the list here
 * cuts the scheduler's reverse dependencies on fs/, hardware/ and net/.
 * Phase 3b replaces entries with ring-3 launches one by one without the
 * task layer noticing.
 */

#include "services.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <utility>
#include "elf.hpp"
#include "fs/fat/fat.hpp"
#include "hardware/usb_service.hpp"
#include "hardware/virtio/blk.hpp"
#include "hardware/virtio/net.hpp"
#include "log/log.hpp"
#include "memory/slab.hpp"
#include "net/packet_handler.hpp"
#include "task/ipc.hpp"
#include "task/task.hpp"

namespace
{

/**
 * @brief Bootstrap of the user shell
 *
 * Runs as the SHELL task's ring-0 prologue: one synchronous FS_LOAD
 * returns a kernel-owned copy of the shell binary as an OOL move, then
 * exec_elf takes ownership and turns this task into the ring-3 shell.
 * Lives next to the manifest because it is boot wiring, not scheduler
 * code (issue #315).
 */
void shell_service()
{
	Message m = { .type = MsgType::FS_LOAD, .sender = process_ids::SHELL };
	char path[6] = "shell";
	memcpy(m.data.fs.name, path, 6);

	const error_t load_err = kernel::task::call(process_ids::FS_FAT32, &m);

	// Take ownership before inspecting the result (mirrors sys_exec)
	kernel::memory::unique_kbuf<> shell_elf{ reinterpret_cast<void*>(m.ool.addr) };
	if (IS_ERR(load_err) || IS_ERR(m.result) || m.ool.size == 0 || !shell_elf) {
		LOG_ERROR("failed to load shell binary");
		while (true) {
			__asm__("hlt");
		}
	}

	CURRENT_TASK->is_initialized = true;
	exec_elf(std::move(shell_elf), "shell", nullptr);
}

using kernel::task::InitialTaskInfo;

/// Every service the kernel starts at boot, in task-slot order. All still
/// run ring 0 (KERNEL_CS kernel threads) for now; see issue #315 3b.
constexpr InitialTaskInfo SERVICE_MANIFEST[] = {
	{ SystemProcessId::XHCI, "usb_handler", &kernel::hw::usb_handler_service, true,
	  true },
	{ SystemProcessId::VIRTIO_BLK, "virtio_blk",
	  &kernel::hw::virtio::virtio_blk_service, true, false },
	{ SystemProcessId::FS_FAT32, "fat32", &kernel::fs::fat32_service, true, true },
	{ SystemProcessId::SHELL, "shell", &shell_service, true, false },
	{ SystemProcessId::VIRTIO_NET, "virtio_net",
	  &kernel::hw::virtio::virtio_net_service, true, false },
	{ SystemProcessId::NET, "net", &kernel::net::packet_handler_service, true,
	  false },
};

constexpr size_t SERVICE_COUNT =
		sizeof(SERVICE_MANIFEST) / sizeof(SERVICE_MANIFEST[0]);

// create_task() hands out slots in ascending order after the scheduler's
// own KERNEL and IDLE tasks, so each entry's declared SystemProcessId only
// holds if the manifest starts at XHCI and is ordered with no gap.
constexpr bool manifest_matches_slots()
{
	for (size_t i = 0; i < SERVICE_COUNT; ++i) {
		if (static_cast<pid_t>(SERVICE_MANIFEST[i].id) !=
			static_cast<pid_t>(SystemProcessId::XHCI) + static_cast<pid_t>(i)) {
			return false;
		}
	}
	return true;
}
static_assert(manifest_matches_slots(),
			  "SERVICE_MANIFEST must start at XHCI and be ordered by "
			  "SystemProcessId, gap-free");

} // namespace

namespace kernel
{

const task::InitialTaskInfo* service_manifest(size_t* out_count)
{
	*out_count = SERVICE_COUNT;
	return SERVICE_MANIFEST;
}

} // namespace kernel
