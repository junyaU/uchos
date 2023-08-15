#include "handler.hpp"

namespace {
void NotifyEndOfInterrupt() {
    volatile auto eoi = reinterpret_cast<uint32_t*>(0xfee000b0);
    *eoi = 0;
}
}  // namespace

__attribute__((interrupt)) void TestInterrupt(InterruptFrame* frame) {
    system_logger->Print("Test Interrupt!\n");
    NotifyEndOfInterrupt();
}

__attribute__((interrupt)) void TimerInterrupt(InterruptFrame* frame) {
    system_logger->Print("Timer Interrupt!\n");
    NotifyEndOfInterrupt();
}
