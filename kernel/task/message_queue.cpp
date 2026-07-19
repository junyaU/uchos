/**
 * @file task/message_queue.cpp
 * @brief Fixed-capacity FIFO ring implementation
 */

#include "task/message_queue.hpp"
#include <cstddef>
#include <libs/common/message.hpp>
#include "log/log.hpp"
#include "memory/slab.hpp"

namespace kernel::task
{

MessageQueue::MessageQueue() : ring_{ nullptr }, head_{ 0 }, count_{ 0 }
{
	void* storage = kernel::memory::alloc(CAPACITY * sizeof(Message),
										  kernel::memory::ALLOC_ZEROED);
	if (storage == nullptr) {
		// Leave ring_ null: push() then fails and the sender sees
		// ERR_QUEUE_FULL instead of the kernel dereferencing nullptr.
		LOG_ERROR("failed to allocate message ring");
		return;
	}

	ring_ = static_cast<Message*>(storage);
}

MessageQueue::~MessageQueue() { kernel::memory::free(ring_); }

bool MessageQueue::push(const Message& m)
{
	if (ring_ == nullptr || count_ >= CAPACITY) {
		return false;
	}

	ring_[(head_ + count_) % CAPACITY] = m;
	++count_;

	return true;
}

bool MessageQueue::pop(Message* out)
{
	if (count_ == 0) {
		return false;
	}

	*out = ring_[head_];
	head_ = (head_ + 1) % CAPACITY;
	--count_;

	return true;
}

} // namespace kernel::task
