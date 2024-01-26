#pragma once

using error_t = int;

// kmalloc flags
#define KMALLOC_UNINITIALIZED 0
#define KMALLOC_ZEROED (1 << 0)
