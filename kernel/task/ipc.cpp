#include "ipc.hpp"
#include <cstddef>
#include <cstdint>
#include <libs/common/message.hpp>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include "error.hpp"
#include "interrupt/irq_guard.hpp"
#include "log/log.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/user.hpp"
#include "task.hpp"

namespace kernel::task
{

namespace
{
// Message type synthesized when the corresponding NotifyType bit is drained
// by a receive. Indexed by the NotifyType value (= bit position).
constexpr MsgType NOTIFY_MESSAGE_TYPES[] = {
	MsgType::NOTIFY_XHCI,		   // NotifyType::XHCI
	MsgType::NOTIFY_VIRTIO_BLK,	   // NotifyType::VIRTIO_BLK
	MsgType::NOTIFY_VIRTIO_NET_RX, // NotifyType::VIRTIO_NET_RX
	MsgType::NOTIFY_VIRTIO_NET_TX, // NotifyType::VIRTIO_NET_TX
	MsgType::NOTIFY_TIMER_TIMEOUT, // NotifyType::TIMER
};
static_assert(sizeof(NOTIFY_MESSAGE_TYPES) / sizeof(NOTIFY_MESSAGE_TYPES[0]) ==
					  static_cast<size_t>(NotifyType::COUNT),
			  "every NotifyType needs a synthesized MsgType");

/**
 * @brief Pop the lowest pending notification bit as a synthesized message
 * @note Caller must have interrupts disabled
 */
bool pop_notification(Task* t, Message* out)
{
	if (t->pending_notifications == 0) {
		return false;
	}

	const int bit = __builtin_ctz(t->pending_notifications);
	t->pending_notifications &= t->pending_notifications - 1;

	*out = Message{ .type = NOTIFY_MESSAGE_TYPES[bit],
					.sender = process_ids::INTERRUPT };

	return true;
}
} // namespace

kernel::memory::unique_kbuf<> make_ool_buffer(size_t size)
{
	if (size == 0) {
		return kernel::memory::unique_kbuf<>{};
	}

	// PAGE_SIZE alignment forces the slab object past PAGE_SIZE, which gives
	// it dedicated pages: nothing else can sit on a page a user may see
	return kernel::memory::make_kbuf(size, kernel::memory::ALLOC_ZEROED,
									 kernel::memory::PAGE_SIZE);
}

error_t reply_with_ool(const Message& req,
					   Message* resp,
					   kernel::memory::unique_kbuf<> buf,
					   uint32_t size)
{
	// A fire-and-forget request (correlation 0) makes reply() a no-op that
	// still returns OK: never attach the payload there, or checking the
	// return value would release a buffer nobody received (leak)
	if (req.correlation == 0 || !buf || size == 0) {
		return reply(req, resp);
	}

	resp->ool.addr = reinterpret_cast<uint64_t>(buf.get());
	resp->ool.size = size;

	const error_t err = reply(req, resp);
	if (IS_ERR(err)) {
		// Not delivered: the buffer is freed on return
		resp->ool = {};
		return err;
	}

	buf.release(); // delivered: the requester owns it now
	return OK;
}

void free_message_ool(Message& m)
{
	if (m.ool.size == 0 || m.ool.addr == 0) {
		return;
	}

	void* addr = reinterpret_cast<void*>(m.ool.addr);
	if (is_user_address(addr, m.ool.size)) {
		// User vaddrs are never kernel-owned; nothing to free here
		return;
	}

	kernel::memory::free(addr);
	m.ool = {};
}

error_t copy_in_ool_from_user(Message* m)
{
	if (m->ool.size == 0) {
		m->ool.addr = 0;
		return OK;
	}

	if (m->ool.size > OOL_MAX_SIZE) {
		return ERR_OOL_LIMIT;
	}

	const void __user* uaddr = reinterpret_cast<const void*>(m->ool.addr);
	if (!is_user_address(uaddr, m->ool.size)) {
		return ERR_INVALID_ARG;
	}

	auto buf = make_ool_buffer(m->ool.size);
	if (!buf) {
		return ERR_NO_MEMORY;
	}

	if (copy_from_user(buf.get(), uaddr, m->ool.size) != m->ool.size) {
		return ERR_INVALID_ARG;
	}

	m->ool.addr = reinterpret_cast<uint64_t>(buf.release());

	return OK;
}

void deliver_ool_to_user(Task* t, Message* m)
{
	if (m->ool.size == 0 || m->ool.addr == 0) {
		return;
	}

	if ((m->ool.addr & (kernel::memory::PAGE_SIZE - 1)) != 0) {
		// Not a make_ool_buffer() buffer: mapping it would expose whatever
		// shares its pages. Protocol bug on the sending side.
		LOG_ERROR("unaligned ool buffer for user delivery: type = %d",
				  static_cast<int>(m->type));
		free_message_ool(*m);
		m->result = ERR_INVALID_ARG;
		return;
	}

	OolRegion* slot = nullptr;
	for (auto& r : t->ool_regions) {
		if (r.kaddr == 0) {
			slot = &r;
			break;
		}
	}

	if (slot == nullptr) {
		// §9: keep the message, drop the payload, surface the error
		LOG_ERROR("ool region table full: task %d", t->id.raw());
		free_message_ool(*m);
		m->result = ERR_OOL_LIMIT;
		return;
	}

	const size_t pages = kernel::memory::calc_required_pages(
			kernel::memory::vaddr_t{ m->ool.addr }, m->ool.size);

	// The kernel is identity-mapped, so the buffer's kernel vaddr is also
	// its physical frame address
	kernel::memory::vaddr_t uaddr;
	const error_t err = kernel::memory::map_frame_to_vaddr(
			t->get_page_table(), m->ool.addr, pages, &uaddr);
	if (IS_ERR(err)) {
		LOG_ERROR_CODE(err, "failed to map ool buffer: task %d", t->id.raw());
		free_message_ool(*m);
		m->result = ERR_NO_MEMORY;
		return;
	}

	*slot = OolRegion{ m->ool.addr, uaddr.data, static_cast<uint32_t>(pages) };
	m->ool.addr = uaddr.data;
}

error_t release_ool_region(Task* t, uint64_t uaddr)
{
	for (auto& r : t->ool_regions) {
		if (r.kaddr == 0 || r.uaddr != uaddr) {
			continue;
		}

		kernel::memory::unmap_frame(t->get_page_table(),
									kernel::memory::vaddr_t{ r.uaddr }, r.pages);
		kernel::memory::free(reinterpret_cast<void*>(r.kaddr));
		r = OolRegion{};
		return OK;
	}

	LOG_ERROR_CODE(ERR_INVALID_ARG, "ool_release: no region at %p in task %d",
				   reinterpret_cast<void*>(uaddr), t->id.raw());
	return ERR_INVALID_ARG;
}

void release_ool_regions(Task* t)
{
	for (auto& r : t->ool_regions) {
		if (r.kaddr != 0) {
			kernel::memory::free(reinterpret_cast<void*>(r.kaddr));
			r = OolRegion{};
		}
	}
}

void release_all_ool(Task* t)
{
	release_ool_regions(t);

	// Undelivered messages and an uncollected reply still own their buffers
	Message m;
	while (try_receive(t, &m)) {
		free_message_ool(m);
	}

	if (t->reply_pending) {
		free_message_ool(t->reply_slot);
		t->reply_pending = false;
	}
}

error_t send_message(ProcessId dst_id, Message& m)
{
	const pid_t dst_raw = dst_id.raw();
	if (dst_raw == -1) {
		LOG_ERROR_CODE(ERR_INVALID_ARG,
					   "invalid destination task id : dest = %d, sender = %d",
					   dst_raw, m.sender.raw());
		return ERR_INVALID_ARG;
	}

	Task* dst = tasks[dst_raw];
	if (dst == nullptr) {
		if (m.type != MsgType::KERNEL_TASK_READY) {
			LOG_ERROR_CODE(ERR_INVALID_TASK, "task %d is not found", dst_raw);
			LOG_ERROR("message type: %d", m.type);
		}
		return ERR_INVALID_TASK;
	}

	// OOL payloads are delivered as-is: kernel buffers move by ownership and
	// the user-space translation happens only at the syscall boundary
	// (issue #314 Stage C), never inside the delivery path.

	// Keep the delivery and the wakeup check atomic so a receiver going to
	// sleep cannot miss the message (lost wakeup)
	const kernel::interrupt::IrqGuard guard;

	// Replies bypass the ring: they are delivered to the caller's reply
	// slot if and only if the correlation matches its outstanding call.
	// Anything else is a protocol bug surfaced here instead of sitting in
	// a queue confusing later receives (issue #314 Stage B).
	if ((m.flags & MSG_FLAG_REPLY) != 0) {
		if (dst->call_correlation == 0 || dst->call_correlation != m.correlation ||
			dst->reply_pending) {
			LOG_ERROR_CODE(ERR_INVALID_ARG,
						   "stale reply dropped: dest = %d, type = %d, corr = %u",
						   dst_raw, static_cast<int>(m.type), m.correlation);
			return ERR_INVALID_ARG;
		}

		dst->reply_slot = m;
		dst->reply_pending = true;

		if (dst->state == TASK_WAITING && dst->wait_reason == WaitReason::REPLY) {
			schedule_task(dst_id);
		}

		return OK;
	}

	if (!dst->messages.push(m)) {
		LOG_ERROR_CODE(ERR_QUEUE_FULL, "message queue full: dest = %d, type = %d",
					   dst_raw, static_cast<int>(m.type));
		return ERR_QUEUE_FULL;
	}

	// Wake only receive-style waits: a NOTIFY waiter is parked until its
	// doorbell fires (waking it would resume a device wait early), and a
	// REPLY waiter only cares about its reply slot — for both, the queued
	// message is handled once the waiter returns to its receive loop
	if (dst->state == TASK_WAITING && (dst->wait_reason == WaitReason::RECEIVE ||
									   dst->wait_reason == WaitReason::NONE)) {
		schedule_task(dst_id);
	}

	return OK;
}

error_t call(ProcessId dst, Message* inout)
{
	Task* t = CURRENT_TASK;

	// A one-way send to yourself is a legitimate event-loop pattern (the
	// shell orders its prompt restore behind queued output this way), but
	// calling yourself can only deadlock: nobody is left to reply.
	if (dst.raw() == t->id.raw()) {
		LOG_ERROR_CODE(ERR_INVALID_ARG, "call to self would deadlock: task %d",
					   t->id.raw());
		return ERR_INVALID_ARG;
	}

	// Correlation 0 is reserved for fire-and-forget, so skip it on wrap
	uint32_t correlation = ++t->next_correlation;
	if (correlation == 0) {
		correlation = ++t->next_correlation;
	}

	// The reply is routed by (sender, correlation); force the sender so a
	// forged value cannot direct someone else's reply slot
	inout->sender = t->id;
	inout->correlation = correlation;
	inout->flags &= ~MSG_FLAG_REPLY;
	inout->result = OK;

	{
		const kernel::interrupt::IrqGuard guard;
		t->call_correlation = correlation;
		t->reply_pending = false;
	}

	const error_t err = send_message(dst, *inout);
	if (IS_ERR(err)) {
		t->call_correlation = 0;
		return err;
	}

	while (true) {
		__asm__("cli");
		if (t->reply_pending) {
			*inout = t->reply_slot;
			t->reply_pending = false;
			t->call_correlation = 0;
			t->state = TASK_RUNNING;
			t->wait_reason = WaitReason::NONE;
			__asm__("sti");
			return OK;
		}

		// Same lost-wakeup discipline as receive_blocking; new requests
		// arriving meanwhile queue in the ring without waking us, which
		// serializes a server's request handling naturally
		t->wait_reason = WaitReason::REPLY;
		t->state = TASK_WAITING;
		__asm__("sti");
		switch_next_task(false);
	}
}

error_t reply(const Message& req, Message* resp)
{
	if (req.correlation == 0) {
		// Fire-and-forget request: nobody is waiting
		return OK;
	}

	resp->sender = CURRENT_TASK->id;
	resp->correlation = req.correlation;
	resp->flags |= MSG_FLAG_REPLY;

	return send_message(req.sender, *resp);
}

[[gnu::no_caller_saved_registers]] void notify(ProcessId dst_id, NotifyType type)
{
	const pid_t dst_raw = dst_id.raw();
	if (dst_raw < 0 || dst_raw >= MAX_TASKS) {
		return;
	}

	const kernel::interrupt::IrqGuard guard;

	Task* dst = tasks[dst_raw];
	if (dst == nullptr) {
		// Interrupt context: a doorbell for a vanished task is dropped
		return;
	}

	dst->pending_notifications |= notify_bit(type);

	if (dst->state != TASK_WAITING) {
		return;
	}

	if (dst->wait_reason == WaitReason::NOTIFY &&
		(dst->wait_notify_mask & notify_bit(type)) == 0) {
		return;
	}

	schedule_task(dst_id);
}

bool try_receive(Task* t, Message* out)
{
	const kernel::interrupt::IrqGuard guard;

	return pop_notification(t, out) || t->messages.pop(out);
}

Message receive_blocking()
{
	Task* t = CURRENT_TASK;
	Message m;

	while (true) {
		__asm__("cli");
		if (pop_notification(t, &m) || t->messages.pop(&m)) {
			t->state = TASK_RUNNING;
			t->wait_reason = WaitReason::NONE;
			__asm__("sti");
			return m;
		}

		// Declare WAITING while interrupts are off so a sender cannot
		// slip in between the emptiness check and the sleep (lost
		// wakeup). The INT in switch_next_task is not blocked by IF.
		t->wait_reason = WaitReason::RECEIVE;
		t->state = TASK_WAITING;
		__asm__("sti");
		switch_next_task(false);
	}
}

void wait_notification(uint32_t mask)
{
	Task* t = CURRENT_TASK;

	while (true) {
		__asm__("cli");
		if ((t->pending_notifications & mask) != 0) {
			t->pending_notifications &= ~mask;
			t->wait_reason = WaitReason::NONE;
			__asm__("sti");
			return;
		}

		// Same lost-wakeup discipline as receive_blocking: the doorbell
		// bit is sticky, so a notify that fired before this point is seen
		// by the check above instead of being lost
		t->wait_reason = WaitReason::NOTIFY;
		t->wait_notify_mask = mask;
		t->state = TASK_WAITING;
		__asm__("sti");
		switch_next_task(false);
	}
}

} // namespace kernel::task
