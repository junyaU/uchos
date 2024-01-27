#pragma once

using error_t = int;

// kmalloc flags
#define KMALLOC_UNINITIALIZED 0
#define KMALLOC_ZEROED (1 << 0)

// error codes
#define IS_OK(err) (!IS_ERR(err))
#define IS_ERR(err) (((long)(err)) < 0)
#define OK 0
#define ERR_NO_MEMORY -1
