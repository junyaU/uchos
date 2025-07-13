#include "graphics/font.hpp"
#include "graphics/screen.hpp"
#include "interrupt/idt.hpp"
#include "memory/bootstrap_allocator.hpp"
#include "memory/buddy_system.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/segment.hpp"
#include "memory/slab.hpp"
#include "syscall/syscall.hpp"
#include "task/builtin.hpp"
#include "task/task.hpp"
#include "tests/framework.hpp"
#include "tests/test_cases/stdio_test.hpp"
#include "tests/test_cases/virtio_blk_test.hpp"
#include "tests/test_cases/fd_test.hpp"
#include "timers/acpi.hpp"
#include "timers/local_apic.hpp"
#include "timers/timer.hpp"

struct FrameBufferConf;
struct MemoryMap;

// 1MiB　
extern "C" {
char kernel_stack[1024 * 1024];
}

extern "C" void Main(const FrameBufferConf& frame_buffer_conf,
					 const MemoryMap& memory_map,
					 const kernel::timers::acpi::root_system_description_pointer& rsdp)
{
	kernel::graphics::initialize(frame_buffer_conf, { 0, 0, 0 });

	kernel::graphics::initialize_font();

	kernel::memory::initialize_segmentation();

	kernel::memory::initialize_paging();

	kernel::interrupt::initialize_interrupt();

	kernel::memory::initialize(memory_map);

	kernel::memory::initialize_heap();

	kernel::memory::initialize_pages();

	kernel::memory::initialize_memory_manager();

	kernel::memory::disable();

	kernel::memory::initialize_slab_allocator();

	kernel::memory::initialize_tss();

	kernel::timers::acpi::initialize(rsdp);

	kernel::timers::initialize();

	kernel::timers::local_apic::initialize();

	kernel::syscall::initialize();

	kernel::task::initialize();

	run_test_suite(register_virtio_blk_tests);
	run_test_suite(register_stdio_tests);
	run_test_suite(register_fd_tests);

	kernel::task::kernel_service();
}
