#pragma once

#include <cstdint>

struct InterruptFrame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

void TestInterrupt(InterruptFrame* frame);