#include "graphics/font.hpp"
#include "graphics/screen.hpp"
#include "hardware/serial.hpp"
#include "interrupt/idt.hpp"
#include "memory/bootstrap_allocator.hpp"
#include "memory/paging.hpp"
#include "memory/segment.hpp"
#include "services.hpp"
#include "syscall/syscall.hpp"
#include "task/builtin.hpp"
#include "task/task.hpp"
#include "tests/runner.hpp"
#ifdef KERNEL_SMOKE_TEST_ENABLED
#include "tests/smoke.hpp"
#endif
#include "timers/acpi.hpp"
#include "timers/local_apic.hpp"
#include "timers/timer.hpp"

struct FrameBufferConf;
struct MemoryMap;

// 1MiB
extern "C" {
char kernel_stack[1024 * 1024];
}

// Bounds of .init_array, synthesized by lld. Nothing ran these before issue
// #383: every global with a dynamic constructor (the slab cache_chain, fs
// tables, IRQ routes, ...) was being used in its zeroed BSS state. A zeroed
// std::vector happens to be a valid empty vector, but a zeroed std::list has
// a null sentinel, so cache_chain grew a phantom node threaded through
// physical page 0 — memory-map-dependent corruption.
// NOLINTBEGIN(readability-identifier-naming): names are fixed by the linker
extern "C" void (*__init_array_start[])();
extern "C" void (*__init_array_end[])();
// NOLINTEND(readability-identifier-naming)

namespace
{
// Must run before anything touches a constructed global. Safe this early:
// every constructor in this kernel default-constructs containers or stamps
// plain values — no allocation, no hardware access.
void run_global_constructors()
{
	for (void (**ctor)() = __init_array_start; ctor != __init_array_end; ++ctor) {
		(*ctor)();
	}
}
} // namespace

extern "C" void Main(const FrameBufferConf& frame_buffer_conf,
					 const MemoryMap& memory_map,
					 const kernel::timers::acpi::RootSystemDescriptionPointer& rsdp)
{
	run_global_constructors();

	kernel::hw::serial::initialize();

	kernel::graphics::initialize(frame_buffer_conf, { 0, 0, 0 });

	kernel::graphics::initialize_font();

	kernel::memory::initialize_segmentation();

	kernel::memory::initialize_paging();

	kernel::interrupt::initialize_interrupt();

	kernel::memory::initialize(memory_map);

	kernel::tests::run_bootstrap_stage_tests();

	kernel::memory::initialize_allocators();

	kernel::memory::initialize_tss();

	kernel::timers::acpi::initialize(rsdp);

	kernel::timers::initialize();

	kernel::tests::run_timer_stage_tests();

	kernel::timers::local_apic::initialize();

	kernel::syscall::initialize();

	size_t num_services = 0;
	const kernel::task::InitialTaskInfo* services =
			kernel::service_manifest(&num_services);
	kernel::task::initialize(services, num_services);

	kernel::tests::run_main_stage_tests();

	// Scheduling starts only after the test suites have finished: a timer
	// interrupt switching to a service task mid-suite would pollute the
	// leak accounting and race against half-initialized boot state.
	kernel::task::start_scheduling();

#ifdef KERNEL_SMOKE_TEST_ENABLED
	// This code is still the KERNEL task: hook the smoke handlers into the
	// message loop kernel_service() is about to enter (issue #374)
	kernel::tests::smoke_init();
#endif

	kernel::task::kernel_service();
}
