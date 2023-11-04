#pragma once

#include <stdint.h>

extern "C" {
uint32_t read_from_io_port(uint16_t port);

void write_to_io_port(uint16_t port, uint32_t value);
}