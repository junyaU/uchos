#pragma once

class interrupt_vector
{
public:
	enum Number {
		LOCAL_APIC_TIMER = 0x40,
		XHCI = 0x41,
		VIRTIO = 0x42,
		VIRTQUEUE = 0x43,
	};
};
