#pragma once

#include "ipc.hpp"

/// Arm a timer for the calling task; the expiry arrives as a bare
/// NOTIFY_TIMER_TIMEOUT and the caller decides what it means.
void set_timer(int ms, bool is_periodic);
