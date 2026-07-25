#pragma once

#include <sys/types.h>
#include <libs/common/types.hpp>

// FS wrappers propagate errors as negative error_t values (issue #356 /
// #315-3a): no more crushing to -1 or 0. Check with IS_ERR / IS_OK, not
// comparisons against specific negative values.

/// @return The opened fd, or a negative error_t
fd_t fs_open(const char* path, int flags);

/// @return Bytes read, or a negative error_t
ssize_t fs_read(fd_t fd, void* buf, size_t count);

/// @return Bytes written, or a negative error_t
ssize_t fs_write(fd_t fd, const void* buf, size_t count);

void fs_close(fd_t fd);

/// @return The created file's fd, or a negative error_t
fd_t fs_create(const char* path);

/// @return OK on success (buf is filled), or a negative error_t (buf is "")
error_t fs_pwd(char* buf, size_t size);

/// @return OK on success (buf holds the new cwd), or a negative error_t
error_t fs_change_dir(char* buf, const char* path);

/// @return newfd on success, or a negative error_t
fd_t fs_dup2(fd_t oldfd, fd_t newfd);
