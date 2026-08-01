/**
 * @file services.cpp
 * @brief Boot-time service manifest: the single place that knows which
 * services UCHos starts
 *
 * Extracted from kernel/task/task.cpp (issue #315): keeping the list here
 * cuts the scheduler's reverse dependencies on fs/, hardware/ and net/.
 * Phase 3b replaces entries with ring-3 launches one by one without the
 * task layer noticing: an entry moves to ring 3 by swapping its function
 * pointer for a binary name (issue #315 3b-10).
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

void service_bootstrap();

using kernel::task::InitialTaskInfo;

/// argv tail of the shell's ring-3 launch. Smoke builds tell the shell to
/// run one fork→exec→wait round-trip plus a pongd ping on its own and
/// report the outcome to KERNEL (issues #374, #315 3b-10).
#ifdef KERNEL_SMOKE_TEST_ENABLED
constexpr const char* SHELL_ARGS = "-smoke";
#else
constexpr const char* SHELL_ARGS = nullptr;
#endif

/// Every service the kernel starts at boot, in task-slot order. Entries
/// with a binary name boot through service_bootstrap() and run ring 3;
/// the rest are still ring-0 kernel threads (issue #315 3b).
constexpr InitialTaskInfo SERVICE_MANIFEST[] = {
	{ SystemProcessId::XHCI, "usb_handler", &kernel::hw::usb_handler_service,
	  nullptr, nullptr, true, true },
	{ SystemProcessId::VIRTIO_BLK, "virtio_blk",
	  &kernel::hw::virtio::virtio_blk_service, nullptr, nullptr, true, false },
	{ SystemProcessId::FS_FAT32, "fat32", &kernel::fs::fat32_service, nullptr,
	  nullptr, true, true },
	{ SystemProcessId::SHELL, "shell", &service_bootstrap, "shell", SHELL_ARGS, true,
	  false },
	{ SystemProcessId::VIRTIO_NET, "virtio_net",
	  &kernel::hw::virtio::virtio_net_service, nullptr, nullptr, true, false },
	{ SystemProcessId::NET, "net", &kernel::net::packet_handler_service, nullptr,
	  nullptr, true, false },
	{ SystemProcessId::PONGD, "pongd", &service_bootstrap, "pongd", nullptr, true,
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

// service_bootstrap() finds its binary by scanning the manifest, so a
// binary name without the bootstrap entry (or vice versa) is a dead field.
constexpr bool ring3_entries_use_bootstrap()
{
	for (size_t i = 0; i < SERVICE_COUNT; ++i) {
		if ((SERVICE_MANIFEST[i].binary != nullptr) !=
			(SERVICE_MANIFEST[i].entry == &service_bootstrap)) {
			return false;
		}
	}
	return true;
}
static_assert(ring3_entries_use_bootstrap(),
			  "manifest entries must set binary iff entry is "
			  "service_bootstrap");

/**
 * @brief Generic ring-0 prologue of every ring-3 manifest service
 *
 * Runs as the service task's entry: looks up its own manifest row by task
 * id, pulls the named binary with one synchronous FS_LOAD (a kernel-owned
 * copy arrives as an OOL move), then exec_elf takes ownership and turns
 * this task into the ring-3 service. Generalizes the former shell-only
 * bootstrap (issue #315 3b-10); the FS migration (3b-11) reuses it as is.
 */
void service_bootstrap()
{
	const InitialTaskInfo* self = nullptr;
	for (const auto& info : SERVICE_MANIFEST) {
		if (CURRENT_TASK->id == info.id) {
			self = &info;
			break;
		}
	}
	if (self == nullptr || self->binary == nullptr) {
		LOG_ERROR("no manifest binary for task %d", CURRENT_TASK->id.raw());
		while (true) {
			__asm__("hlt");
		}
	}

	Message m = { .type = MsgType::FS_LOAD, .sender = CURRENT_TASK->id };
	strncpy(m.data.fs.name, self->binary, sizeof(m.data.fs.name) - 1);

	const error_t load_err = kernel::task::call(process_ids::FS_FAT32, &m);

	// Take ownership before inspecting the result (mirrors sys_exec)
	kernel::memory::unique_kbuf<> elf_buf{ reinterpret_cast<void*>(m.ool.addr) };
	if (IS_ERR(load_err) || IS_ERR(m.result) || m.ool.size == 0 || !elf_buf) {
		LOG_ERROR("failed to load service binary: %s", self->binary);
		while (true) {
			__asm__("hlt");
		}
	}

	CURRENT_TASK->is_initialized = true;
	exec_elf(std::move(elf_buf), self->name, self->args);

	// exec_elf only returns on failure (segment/stack/heap mapping). Falling
	// off this function would run off a dead ring-0 stack frame into a
	// garbage RIP and panic the whole system — park the task instead.
	LOG_ERROR("exec failed for service binary: %s", self->binary);
	while (true) {
		__asm__("hlt");
	}
}

} // namespace

namespace kernel
{

const task::InitialTaskInfo* service_manifest(size_t* out_count)
{
	*out_count = SERVICE_COUNT;
	return SERVICE_MANIFEST;
}

} // namespace kernel
