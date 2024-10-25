#pragma once

#include <libs/common/types.hpp>

fd_t fs_open(const char* path, int flags);

size_t fs_read(fd_t fd, void* buf, size_t count);
