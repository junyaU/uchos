#pragma once

#include <cstdint>

namespace kernel::hw::usb
{
namespace request_type
{
// target
const int DEVICE = 0;
const int INTERFACE = 1;
const int ENDPOINT = 2;
const int OTHER = 3;

// type
const int STANDARD = 0;
const int CLASS = 1;
const int VENDOR = 2;

// direction
const int OUT = 0;
const int IN = 1;
} // namespace request_type

namespace request
{
const int GET_STATUS = 0;
const int CLEAR_FEATURE = 1;
const int SET_FEATURE = 3;
const int SET_ADDRESS = 5;
const int GET_DESCRIPTOR = 6;
const int SET_DESCRIPTOR = 7;
const int GET_CONFIGURATION = 8;
const int SET_CONFIGURATION = 9;
const int GET_INTERFACE = 10;
const int SET_INTERFACE = 11;
const int SYNCH_FRAME = 12;
const int SET_ENCRYPTION = 13;
const int GET_ENCRYPTION = 14;
const int SET_HANDSHAKE = 15;
const int GET_HANDSHAKE = 16;
const int SET_CONNECTION = 17;
const int SET_SECURITY_DATA = 18;
const int GET_SECURITY_DATA = 19;
const int SET_WUSB_DATA = 20;
const int LOOPBACK_DATA_WRITE = 21;
const int LOOPBACK_DATA_READ = 22;
const int SET_INTERFACE_DS = 23;
const int SET_SEL = 48;
const int SET_ISOCH_DELAY = 49;

const int GET_REPORT = 1;
const int SET_PROTOCOL = 11;
} // namespace request

namespace descriptor_type
{
const int DEVICE = 1;
const int CONFIGURATION = 2;
const int STRING = 3;
const int INTERFACE = 4;
const int ENDPOINT = 5;
const int INTERFACE_POWER = 8;
const int OTG = 9;
const int DEBUG = 10;
const int INTERFACE_ASSOCIATION = 11;
const int BOS = 15;
const int DEVICE_CAPABILITY = 16;
const int HID = 33;
const int SUPERSPEED_USB_ENDPOINT_COMPANION = 48;
const int SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION = 49;
} // namespace descriptor_type

struct SetupStageData {
	union {
		uint8_t data;

		struct {
			uint8_t recipient : 5;
			uint8_t type : 2;
			uint8_t direction : 1;
		} bits;
	} request_type;

	uint8_t request;
	uint16_t value;
	uint16_t index;
	uint16_t length;
} __attribute__((packed));

inline bool operator==(SetupStageData lhs, SetupStageData rhs)
{
	return lhs.request_type.data == rhs.request_type.data &&
		   lhs.request == rhs.request && lhs.value == rhs.value &&
		   lhs.index == rhs.index && lhs.length == rhs.length;
}
}; // namespace kernel::hw::usb
