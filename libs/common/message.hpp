#pragma once

#include <cstddef>
#include <cstdint>
#include "process_id.hpp"
#include "types.hpp"

#define __user

// flags for sys_ipc
constexpr int IPC_RECV = 0;
constexpr int IPC_SEND = 1;
constexpr int IPC_CALL = 2;	 ///< send + block until the matching reply
constexpr int IPC_REPLY = 3; ///< reply to a received request (server side)
/// Unmap + free an OOL region previously delivered to this task. The target
/// region is named by Message::ool.addr (the received user vaddr).
constexpr int IPC_OOL_RELEASE = 4;

/// Message::flags bit marking a reply; correlation then names the request
/// it answers. Set by the kernel reply path, never by hand.
constexpr uint32_t MSG_FLAG_REPLY = 1U << 0;

// Sector size of the block-device IPC contract: blk.sector is expressed in
// this unit and blk.len is expected to be a multiple of it.
constexpr size_t SECTOR_SIZE = 512;

/// Inline payload ceiling: anything larger travels as an OOL buffer.
constexpr size_t MSG_INLINE_MAX = 128;

/// Upper bound for a single OOL payload; the user->kernel copy-in at the
/// syscall boundary refuses anything larger (issue #314 Stage C).
constexpr size_t OOL_MAX_SIZE = 16UL * 1024 * 1024;

/**
 * @brief Message types: one-way notifications are NOTIFY_*, RPCs are
 * <server>_<operation> (issue #314 Stage C naming)
 *
 * The first five NOTIFY_* entries are synthesized from NotifyType doorbell
 * bits when a plain receive drains them (kernel/task/ipc.cpp).
 */
enum class MsgType : int32_t {
	NOTIFY_XHCI,
	NOTIFY_VIRTIO_BLK,
	NOTIFY_VIRTIO_NET_RX,
	NOTIFY_VIRTIO_NET_TX,
	NOTIFY_TIMER_TIMEOUT,
	NOTIFY_KEY_INPUT,
	NOTIFY_WRITE,
	// Shell-internal marker the shell sends to itself after sys_wait: FIFO
	// delivery queues it behind everything the finished command printed,
	// so the prompt is restored only after that output is drawn.
	SHELL_COMMAND_DONE,
	KERNEL_TASK_READY,
	KERNEL_MEMORY_USAGE,
	KERNEL_PCI_LIST,
	BLK_READ,
	BLK_WRITE,
	NET_RX,
	NET_TX,
	NET_SEND,
	FS_STAT,
	FS_LOAD,
	FS_LIST_DIR,
	FS_OPEN,
	FS_CLOSE,
	FS_READ,
	FS_WRITE,
	FS_MKFILE,
	FS_REGISTER_PATH,
	FS_PWD,
	FS_CHANGE_DIR,
	FS_DUP2,
	MAX_MESSAGE_TYPE, // must be the last
};

constexpr int32_t TOTAL_MESSAGE_TYPES =
		static_cast<int32_t>(MsgType::MAX_MESSAGE_TYPE);

/**
 * @brief Descriptor of an out-of-line payload attached to a Message
 *
 * addr holds a sender-space vaddr on send and a receiver-space vaddr on
 * receive; the kernel translates only at the syscall boundary (copy-in on
 * user send, map on user receive). Ownership always moves with the message:
 * the receiver frees a kernel buffer, a user receiver calls ool_release().
 */
struct OolDesc {
	uint64_t addr;
	uint32_t size; ///< Payload bytes; 0 = no OOL payload
	uint32_t reserved;
};

/// One device record in the KERNEL_PCI_LIST reply's OOL array.
struct PciDeviceInfo {
	uint16_t vendor_id;
	uint16_t device_id;
	char bus_address[8];
};

struct Message {
	MsgType type;
	ProcessId sender;
	/// Request/reply pairing id, assigned by the kernel in call(): a reply
	/// is delivered only to the caller whose outstanding call carries the
	/// same value. 0 = fire-and-forget (reply() becomes a no-op).
	uint32_t correlation;
	uint32_t flags; ///< MSG_FLAG_* bits
	/// Reply: OK or a negative error_t describing the server-side outcome;
	/// payload fields are only valid when this is OK. Requests carry OK.
	int32_t result;
	OolDesc ool;

	union {
		struct {
			int task_id;
		} init;

		struct {
			uint8_t key_code;
			uint8_t modifier;
			uint8_t ascii;
			int press;
		} key_input;

		/// Inline stdout payload (NOTIFY_WRITE); larger writes travel OOL
		struct {
			char buf[MSG_INLINE_MAX];
		} write;

		struct {
			unsigned int total;
			unsigned int used;
		} memory_usage;

		struct {
			unsigned int sector;
			size_t len;
		} blk;

		struct {
			fd_t fd;
			int operation;
			size_t len;
			size_t offset;
			char name[64];
		} fs;
	} data;
};

static_assert(sizeof(Message{}.data) <= MSG_INLINE_MAX,
			  "inline payload union must stay within MSG_INLINE_MAX");
static_assert(sizeof(Message) <= 192, "Message must stay ring-friendly");
