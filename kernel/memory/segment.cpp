#include "segment.hpp"
#include "graphics/log.hpp"
#include "interrupt/idt.hpp"
#include "page.hpp"
#include "segment_utils.h"
#include "slab.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <libs/common/types.hpp>

namespace
{
std::array<segment_descriptor, 7> gdt;
std::array<uint32_t, 26> tss;

static_assert((TSS >> 3) + 1 < gdt.size());
} // namespace

void set_code_segment(segment_descriptor& desc,
					  descriptor_type type,
					  unsigned int dpl)
{
	desc.data = 0;
	desc.bits.type = type;
	desc.bits.system_segment = 1;
	desc.bits.descriptor_privilege_level = dpl;
	desc.bits.present = 1;
	desc.bits.available = 0;
	desc.bits.long_mode = 1;
	desc.bits.default_operation_size = 0;
	desc.bits.granularity = 1;
}

void set_data_segment(segment_descriptor& desc,
					  descriptor_type type,
					  unsigned int dpl)
{
	set_code_segment(desc, type, dpl);
	desc.bits.long_mode = 0;
	desc.bits.default_operation_size = 1;
}

void set_system_segment(segment_descriptor& desc,
						descriptor_type type,
						unsigned int dpl,
						uint32_t base,
						uint32_t limit)
{
	set_code_segment(desc, type, dpl);
	desc.bits.system_segment = 0;
	desc.bits.long_mode = 0;
	desc.bits.limit_high = (limit >> 16) & 0xfU;
	desc.bits.limit_low = limit & 0xffffU;

	desc.bits.base_low = base & 0xffffU;
	desc.bits.base_middle = (base >> 16) & 0xffU;
	desc.bits.base_high = (base >> 24) & 0xffU;
}

void setup_segments()
{
	gdt[0].data = 0;
	set_code_segment(gdt[1], descriptor_type::EXECUTE_READ, 0);
	set_data_segment(gdt[2], descriptor_type::READ_WRITE, 0);
	set_data_segment(gdt[3], descriptor_type::READ_WRITE, 3);
	set_code_segment(gdt[4], descriptor_type::EXECUTE_READ, 3);

	load_gdt(sizeof(gdt) - 1, reinterpret_cast<uint64_t>(&gdt));
}

void initialize_segmentation()
{
	LOG_INFO("Initializing segmentation...");
	setup_segments();

	load_data_segment(KERNEL_DS);
	load_code_segment(KERNEL_CS);
	load_stack_segment(KERNEL_SS);
	LOG_INFO("Segmentation initialized successfully.");
}

void set_tss(int index, void* addr)
{
	const uint64_t value = reinterpret_cast<uint64_t>(addr);
	tss[index] = value & 0xffffffff;
	tss[index + 1] = value >> 32;
}

void* allocate_stack(size_t size)
{
	void* stack = kmalloc(size, KMALLOC_UNINITIALIZED);
	if (stack == nullptr) {
		return nullptr;
	}

	return reinterpret_cast<void*>(reinterpret_cast<uint64_t>(stack) + size);
}

void initialize_tss()
{
	const size_t stack_size = PAGE_SIZE * 8;
	void* stack1 = allocate_stack(stack_size);
	void* stack2 = allocate_stack(stack_size);
	void* stack3 = allocate_stack(stack_size);
	if (stack1 == nullptr || stack2 == nullptr) {
		LOG_ERROR("Failed to allocate stack for TSS.");
		return;
	}

	set_tss(1, stack1);
	set_tss(7 + 2 * IST_FOR_TIMER, stack2);
	set_tss(7 + 2 * IST_FOR_XHCI, stack3);
	set_tss(7 + 2 * IST_FOR_SWITCH_TASK, stack3);

	const uint64_t tss_addr = reinterpret_cast<uint64_t>(tss.data());
	set_system_segment(gdt[TSS >> 3], descriptor_type::TSS_AVAILABLE, 0,
					   tss_addr & 0xffffffff, sizeof(tss) - 1);
	gdt[(TSS >> 3) + 1].data = tss_addr >> 32;

	load_tr(TSS);
}
