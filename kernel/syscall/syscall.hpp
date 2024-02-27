/*
 * @file syscall/syscall.hpp
 *
 * @brief system call
 *
 * This file defines the interface and necessary constants for system calls
 * in the operating system. System calls are the primary mechanism for user mode
 * processes to request services from the kernel.
 *
 */

#pragma once

// system call number
constexpr int SYS_READ = 0;
constexpr int SYS_WRITE = 1;
constexpr int SYS_OPEN = 2;
constexpr int SYS_EXIT = 60;

namespace syscall
{
void initialize();
} // namespace syscall
