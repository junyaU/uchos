#pragma once

class InterruptVector
{
public:
	enum Number {
		kLocalApicTimer = 0x40,
		kXHCI = 0x41,
	};
};
