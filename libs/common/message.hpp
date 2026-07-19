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

/// Message::flags bit marking a reply; correlation then names the request
/// it answers. Set by the kernel reply path, never by hand.
constexpr uint32_t MSG_FLAG_REPLY = 1U << 0;

// Sector size of the block-device IPC contract: blk_io.sector is expressed in
// this unit and blk_io.len is expected to be a multiple of it.
constexpr size_t SECTOR_SIZE = 512;

enum class MsgType : int32_t {
	NO_TASK = -1,
	NOTIFY_KEY_INPUT,
	NOTIFY_XHCI,
	NOTIFY_TIMER_TIMEOUT,
	NOTIFY_WRITE,
	NOTIFY_VIRTIO_NET_RX,
	NOTIFY_VIRTIO_NET_TX,
	INITIALIZE_TASK,
	IPC_TIME,
	IPC_EXIT_TASK,
	IPC_MEMORY_USAGE,
	IPC_PCI,
	IPC_WRITE_TO_BLK_DEVICE,
	IPC_READ_FROM_BLK_DEVICE,
	IPC_NET_SEND_PACKET,
	IPC_NET_RECV_PACKET,
	IPC_TRANSMIT_TO_NIC,
	IPC_OOL_MEMORY_DEALLOC,
	GET_DIRECTORY_CONTENTS,
	IPC_GET_FILE_INFO,
	IPC_READ_FILE_DATA,
	IPC_RELEASE_FILE_BUFFER,
	FS_OPEN,
	FS_CLOSE,
	FS_READ,
	FS_WRITE,
	FS_MKFILE,
	FS_REGISTER_PATH,
	FS_PWD,
	FS_CHANGE_DIR,
	FS_DUP2,
	// Synthesized from NotifyType::VIRTIO_BLK when the doorbell bit is
	// drained by a plain receive. Appended at the end so the values above
	// keep matching userland binaries built before this entry existed; the
	// MsgType reorganization happens in issue #314 Stage C.
	NOTIFY_VIRTIO_BLK,
	// Shell-internal marker the shell sends to itself after sys_wait: FIFO
	// delivery queues it behind everything the finished command printed,
	// so the prompt is restored only after that output is drawn.
	SHELL_COMMAND_DONE,
	MAX_MESSAGE_TYPE, // must be the last
};

constexpr int32_t TOTAL_MESSAGE_TYPES =
		static_cast<int32_t>(MsgType::MAX_MESSAGE_TYPE);

enum class TimeoutAction : uint8_t {
	TERMINAL_CURSOR_BLINK,
	SWITCH_TASK,
};

struct MsgOolDescT {
	void __user* addr;
	size_t size;
	bool present = false;
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
	MsgOolDescT tool_desc;

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

		struct {
			TimeoutAction action;
		} timer;

		struct {
			char s[128];
			int level;
		} write;

		struct {
			char buf[256];
		} write_shell;

		struct {
			int status;
		} exit_task;

		struct {
			unsigned int total;
			unsigned int used;
		} memory_usage;

		struct {
			unsigned int device_id;
			unsigned int vendor_id;
			char bus_address[8];
		} pci;

		// The request_id/sequence/dst_type correlation fields are gone:
		// request/reply pairing is Message::correlation, and the outcome is
		// Message::result (issue #314 Stage B).
		struct {
			void* buf;
			unsigned int sector;
			size_t len;
		} blk_io;

		struct {
			fd_t fd;
			void* buf;
			char temp_buf[256];
			char name[30];
			size_t len;
			int operation;
		} fs;

		struct {
			uint8_t packet_data[1514]; // Ethernet maximum frame size
			size_t packet_len;
			int result;
		} net;
	} data;
};
