#include "ring.hpp"
#include "../../../graphics/log.hpp"
#include "../../../memory/slab.hpp"
#include <libs/common/types.hpp>

namespace usb::xhci
{
ring::~ring() {}

void ring::initialize(size_t buf_size)
{
	cycle_bit_ = true;
	write_index_ = 0;
	buffer_size_ = buf_size;
	buffer_ = reinterpret_cast<trb*>(
			kmalloc(sizeof(trb) * buffer_size_, KMALLOC_ZEROED, 64));
	if (buffer_ == nullptr) {
		printk(KERN_ERROR, "failed to allocate memory for ring");
		return;
	}
}

void ring::copy_to_last(const std::array<uint32_t, 4>& data)
{
	for (int i = 0; i < 3; i++) {
		buffer_[write_index_].data[i] = data[i];
	}

	buffer_[write_index_].data[3] =
			(data[3] & 0xfffffffeU) | static_cast<uint32_t>(cycle_bit_);
}

trb* ring::push(const std::array<uint32_t, 4>& data)
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

void event_ring::initialize(size_t buf_size,
							interrupter_register_set* interrupter_register)
{
	cycle_bit_ = true;
	buffer_size_ = buf_size;
	interrupter_register_ = interrupter_register;

	buffer_ = reinterpret_cast<trb*>(
			kmalloc(sizeof(trb) * buffer_size_, KMALLOC_ZEROED, 64));
	if (buffer_ == nullptr) {
		printk(KERN_ERROR, "failed to allocate memory for event ring");
		return;
	}

	segment_table_ = reinterpret_cast<event_ring_segment_table_entry*>(
			kmalloc(sizeof(event_ring_segment_table_entry), KMALLOC_ZEROED, 64));
	if (segment_table_ == nullptr) {
		kfree(buffer_);
		printk(KERN_ERROR, "failed to allocate memory for event ring segment table");
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

void event_ring::write_dequeue_pointer(trb* p)
{
	auto erdp = interrupter_register_->erdp.read();
	erdp.set_pointer(reinterpret_cast<uint64_t>(p));
	interrupter_register_->erdp.write(erdp);
}

void event_ring::pop()
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

void register_command_ring(ring* r, memory_mapped_register<crcr_bitmap>* crcr)
{
	crcr_bitmap value = crcr->read();
	value.bits.ring_cycle_state = true;
	value.bits.command_stop = false;
	value.bits.command_abort = false;
	value.set_pointer(reinterpret_cast<uint64_t>(r->buffer()));
	crcr->write(value);
}
} // namespace usb::xhci