#pragma once

class InterruptVector {
   public:
    enum Number {
        kTest = 0x40,
        kLocalApicTimer = 0x41,
    };
};
