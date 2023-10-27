#include "segment.hpp"
#include "segments_operations.h"
#include <array>

namespace
{
std::array<SegmentDescriptor, 3> gdt;
} // namespace

void SetCodeSegment(SegmentDescriptor& desc, DescriptorType type, unsigned int dpl)
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

void SetDataSegment(SegmentDescriptor& desc, DescriptorType type, unsigned int dpl)
{
	SetCodeSegment(desc, type, dpl);
	desc.bits.long_mode = 0;
	desc.bits.default_operation_size = 1;
}

void SetupSegments()
{
	gdt[0].data = 0;
	SetCodeSegment(gdt[1], DescriptorType::kExecuteRead, 0);
	SetDataSegment(gdt[2], DescriptorType::kReadWrite, 0);

	LoadGDT(sizeof(gdt) - 1, reinterpret_cast<uint64_t>(&gdt));
}

void InitializeSegmentation()
{
	SetupSegments();

	LoadDataSegment(kKernelDS);
	LoadCodeSegment(kKernelCS);
	LoadStackSegment(kKernelSS);
}