#include "ring.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include "../../mm_register.hpp"
#include "graphics/log.hpp"
#include "memory/slab.hpp"
#include "registers.hpp"
#include "trb.hpp"

namespace kernel::hw::usb::xhci
{
Ring::~Ring() {}

void Ring::initialize(size_t buf_size)
{
	cycle_bit_ = true;
	write_index_ = 0;
	buffer_size_ = buf_size;
	void* buffer_ptr;
	ALLOC_OR_RETURN(buffer_ptr, sizeof(trb) * buffer_size_,
					kernel::memory::ALLOC_ZEROED);
	buffer_ = reinterpret_cast<trb*>(buffer_ptr);
}

void Ring::copy_to_last(const std::array<uint32_t, 4>& data)
{
	for (int i = 0; i < 3; i++) {
		buffer_[write_index_].data[i] = data[i];
	}

	buffer_[write_index_].data[3] =
			(data[3] & 0xfffffffeU) | static_cast<uint32_t>(cycle_bit_);
}

trb* Ring::push(const std::array<uint32_t, 4>& data)
{
	auto* trb_ptr = &buffer_[write_index_];
	copy_to_last(data);

	++write_index_;
	if (write_index_ == buffer_size_ - 1) {
		link_trb link{ buffer_ };
		link.bits.toggle_cycle = true;
		copy_to_last(link.data);

		write_index_ = 0;
		cycle_bit_ = !cycle_bit_;
	}

	return trb_ptr;
}

void EventRing::initialize(size_t buf_size,
						   InterrupterRegisterSet* interrupter_register)
{
	cycle_bit_ = true;
	buffer_size_ = buf_size;
	interrupter_register_ = interrupter_register;

	void* buffer_ptr;
	ALLOC_OR_RETURN(buffer_ptr, sizeof(trb) * buffer_size_,
					kernel::memory::ALLOC_ZEROED);
	buffer_ = reinterpret_cast<trb*>(buffer_ptr);

	segment_table_ = reinterpret_cast<event_ring_segment_table_entry*>(
			kernel::memory::alloc(sizeof(event_ring_segment_table_entry),
								  kernel::memory::ALLOC_ZEROED, 64));
	if (segment_table_ == nullptr) {
		kernel::memory::free(buffer_);
		LOG_ERROR("failed to allocate memory for event ring segment table");
		return;
	}

	segment_table_[0].bits.ring_segment_base_address =
			reinterpret_cast<uint64_t>(buffer_);
	segment_table_[0].bits.ring_segment_size = buffer_size_;

	erstsz_bitmap event_ring_segment_table_size =
			interrupter_register_->erstsz.read();
	event_ring_segment_table_size.set_size(1);
	interrupter_register_->erstsz.write(event_ring_segment_table_size);

	write_dequeue_pointer(&buffer_[0]);

	erstba_bitmap event_ring_segment_table_base_address =
			interrupter_register_->erstba.read();
	event_ring_segment_table_base_address.set_pointer(
			reinterpret_cast<uint64_t>(segment_table_));
	interrupter_register_->erstba.write(event_ring_segment_table_base_address);
}

void EventRing::write_dequeue_pointer(trb* p)
{
	auto erdp = interrupter_register_->erdp.read();
	erdp.set_pointer(reinterpret_cast<uint64_t>(p));
	interrupter_register_->erdp.write(erdp);
}

void EventRing::pop()
{
	auto* p = read_dequeue_pointer() + 1;

	trb* segment_begin =
			reinterpret_cast<trb*>(segment_table_[0].bits.ring_segment_base_address);
	trb* segment_end = segment_begin + segment_table_[0].bits.ring_segment_size;

	if (p == segment_end) {
		p = segment_begin;
		cycle_bit_ = !cycle_bit_;
	}

	write_dequeue_pointer(p);
}

void register_command_ring(Ring* r, MemoryMappedRegister<crcr_bitmap>* crcr)
{
	crcr_bitmap value = crcr->read();
	value.bits.ring_cycle_state = true;
	value.bits.command_stop = false;
	value.bits.command_abort = false;
	value.set_pointer(reinterpret_cast<uint64_t>(r->buffer()));
	crcr->write(value);
}
} // namespace kernel::hw::usb::xhci