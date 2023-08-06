#include "handler.hpp"

#include "system_logger.hpp"

void NotifyEndOfInterrupt() {
    volatile auto eoi = reinterpret_cast<uint32_t*>(0xfee000b0);
    *eoi = 0;
}

__attribute__((interrupt)) void TestInterrupt(InterruptFrame* frame) {
    system_logger->Print("Test Interrupt!\n");
    NotifyEndOfInterrupt();
}