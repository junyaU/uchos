/**
 * @file hardware/usb/xhci/ring.hpp
 *
 * @brief Ring buffer for xHCI
 *
 * This file defines the classes related to the management of ring buffers in the
 * xHCI context. Ring buffers are used in xHCI for managing various types of data
 * transfer and event handling.
 *
 */

#pragma once

#include "registers.hpp"
#include "trb.hpp"
#include <cstdint>

namespace kernel::hw::usb::xhci
{
// Command/Transfer ring
class Ring
{
public:
	Ring() = default;
	Ring(const Ring&) = delete;
	~Ring();
	Ring& operator=(const Ring&) = delete;

	void initialize(size_t buf_size);

	template<class trb_type>
	trb* push(const trb_type& trb)
	{
		return push(trb.data);
	}

	trb* buffer() const { return buffer_; }

private:
	trb* buffer_ = nullptr;
	size_t buffer_size_ = 0;

	bool cycle_bit_;
	size_t write_index_;

	void copy_to_last(const std::array<uint32_t, 4>& data);

	trb* push(const std::array<uint32_t, 4>& data);
};

union event_ring_segment_table_entry {
	std::array<uint32_t, 4> data;

	struct {
		uint64_t ring_segment_base_address;

		uint32_t ring_segment_size : 16;
		uint32_t : 16;

		uint32_t : 32;
	} __attribute__((packed)) bits;
};

class EventRing
{
public:
	void initialize(size_t buf_size, InterrupterRegisterSet* interrupter_register);

	trb* read_dequeue_pointer() const
	{
		return reinterpret_cast<trb*>(interrupter_register_->erdp.read().pointer());
	}

	void write_dequeue_pointer(trb* p);

	trb* front() const { return read_dequeue_pointer(); }

	bool has_front() const { return front()->bits.cycle_bit == cycle_bit_; }

	void pop();

private:
	trb* buffer_;
	size_t buffer_size_;

	bool cycle_bit_;
	event_ring_segment_table_entry* segment_table_;
	InterrupterRegisterSet* interrupter_register_;
};

void register_command_ring(Ring* r, MemoryMappedRegister<crcr_bitmap>* crcr);
} // namespace kernel::hw::usb::xhci