/**
 * @file task/message_queue.hpp
 * @brief Fixed-capacity FIFO ring for a task's incoming IPC messages
 *
 * Replaces the unbounded std::deque<Message>: the ring storage is allocated
 * once when the task is created, so enqueueing a message never touches the
 * heap and the queue depth is bounded by construction (issue #314 Stage A).
 */

#pragma once

#include <cstddef>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>

namespace kernel::task
{

struct Task;

// Every enqueue/dequeue path is funneled through these functions (declared
// here so they can be befriended below); see task/ipc.hpp and task/task.hpp
// for their documentation.
error_t send_message(ProcessId dst, Message& m);
bool try_receive(Task* t, Message* out);
Message receive_blocking();

/**
 * @brief Bounded FIFO message ring owned by a Task
 *
 * The mutating operations are private on purpose: pushing directly onto a
 * task's queue would bypass the wakeup and backpressure rules, so the only
 * entry points are the befriended kernel::task IPC functions (issue #314).
 * Callers of those functions are responsible for interrupt masking; the
 * ring itself does no locking.
 */
class MessageQueue
{
public:
	MessageQueue();
	~MessageQueue();
	MessageQueue(const MessageQueue&) = delete;
	MessageQueue& operator=(const MessageQueue&) = delete;

	size_t size() const { return count_; }
	bool empty() const { return count_ == 0; }

	/**
	 * @brief Message slots per task
	 *
	 * Message is <=192B (static_assert in libs/common/message.hpp), so 64
	 * slots cost ~12KB per task (issue #314 Stage C).
	 */
	static constexpr size_t CAPACITY = 64;

private:
	/**
	 * @brief Append a message to the tail
	 * @return false when the ring is full (or its storage failed to allocate)
	 */
	bool push(const Message& m);

	/**
	 * @brief Pop the head message in FIFO order
	 * @return false when the ring is empty
	 */
	bool pop(Message* out);

	Message* ring_; ///< CAPACITY slots, allocated once at construction
	size_t head_;	///< Index of the oldest message
	size_t count_;	///< Number of queued messages

	friend error_t send_message(ProcessId, Message&);
	friend bool try_receive(Task*, Message*);
	friend Message receive_blocking();
};

} // namespace kernel::task
