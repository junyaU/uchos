#pragma once

#include <libs/common/types.hpp>

fd_t fs_open(const char* path, int flags);

size_t fs_read(fd_t fd, void* buf, size_t count);

size_t fs_write(fd_t fd, const void* buf, size_t count);

void fs_close(fd_t fd);

fd_t fs_create(const char* path);

void fs_pwd(char* buf, size_t size);

void fs_change_dir(char* buf, const char* path);

int fs_dup2(fd_t oldfd, fd_t newfd);
