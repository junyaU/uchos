#pragma once

#include <stdint.h>

extern "C" {
uint32_t read_from_io_port(uint16_t port);

void write_to_io_port(uint16_t port, uint32_t value);

int most_significant_bit(uint32_t value);

void write_msr(uint32_t msr, uint64_t value);

void call_userland(int argc,
				   char** argv,
				   uint16_t ss,
				   uint64_t rip,
				   uint64_t rsp,
				   uint64_t* kernel_rsp);

void exit_userland(uint64_t kernel_rsp, int32_t status);
}
