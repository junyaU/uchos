#include "segment.hpp"
#include "segments_operations.h"
#include <array>

namespace
{
std::array<segment_descriptor, 3> gdt;
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

void setup_segments()
{
	gdt[0].data = 0;
	set_code_segment(gdt[1], descriptor_type::kExecuteRead, 0);
	set_data_segment(gdt[2], descriptor_type::kReadWrite, 0);

	load_gdt(sizeof(gdt) - 1, reinterpret_cast<uint64_t>(&gdt));
}

void initialize_segmentation()
{
	setup_segments();

	load_data_segment(KERNEL_DS);
	load_code_segment(KERNEL_CS);
	load_stack_segment(KERNEL_SS);
}