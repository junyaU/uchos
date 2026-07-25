/**
 * @file tests/qemu_exit.hpp
 * @brief Terminate a headless QEMU run through the isa-debug-exit device
 *
 * Shared by the test runner (KERNEL_TEST_EXIT) and the ring-3 smoke test
 * (KERNEL_SMOKE_TEST) so both report through the same exit-status contract
 * that scripts/run_kernel_tests.sh checks.
 */

#pragma once

#include <cstdint>
#include "asm_utils.h"

namespace kernel::tests
{

constexpr uint16_t QEMU_ISA_DEBUG_EXIT_PORT = 0xf4;
constexpr uint8_t QEMU_EXIT_CODE_PASS = 0x10; // QEMU exits with (0x10 << 1) | 1 = 33
constexpr uint8_t QEMU_EXIT_CODE_FAIL = 0x11; // QEMU exits with (0x11 << 1) | 1 = 35

/**
 * @brief Write the overall result to isa-debug-exit so QEMU terminates
 *
 * When the device is absent (interactive run), the write is ignored and
 * boot continues normally.
 *
 * @param passed true exits QEMU with status 33, false with status 35
 */
inline void exit_qemu(bool passed)
{
	write_to_io_port8(QEMU_ISA_DEBUG_EXIT_PORT,
					  passed ? QEMU_EXIT_CODE_PASS : QEMU_EXIT_CODE_FAIL);
}

} // namespace kernel::tests
