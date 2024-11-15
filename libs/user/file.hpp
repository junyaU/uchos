#pragma once

#include <libs/common/types.hpp>

fd_t fs_open(const char* path, int flags);

size_t fs_read(fd_t fd, void* buf, size_t count);

void fs_close(fd_t fd);

fd_t fs_create(const char* path);

void get_cwd(char* buf, size_t size);
