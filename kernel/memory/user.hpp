#pragma once

#include <cstddef>

#define __user

bool is_user_address(const void* addr, size_t n);

size_t copy_to_user(void __user* to, const void* from, size_t n);

size_t copy_from_user(void* to, const void __user* from, size_t n);
