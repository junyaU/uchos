/*
 * @file msr.hpp
 *
 *  @brief Model specific registers
 *
 * This file defines constants for various Model Specific Registers (MSRs)
 * used in the x86 architecture. MSRs are used to control features such as
 * system calls, task switches, and machine check exceptions. They provide an
 * interface for the kernel and other privileged code to configure and manage
 * low-level processor settings and features.
 *
 */

#pragma once

#include <cstdint>

static constexpr uint32_t IA32_EFER = 0xC0000080;
static constexpr uint32_t IA32_STAR = 0xC0000081;
static constexpr uint32_t IA32_LSTAR = 0xC0000082;
static constexpr uint32_t IA32_FMASK = 0xC0000084;