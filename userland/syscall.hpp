#ifdef __cplusplus
#include <cstddef>
#include <cstdint>

extern "C" {
#else
#include <stddef.h>
#include <stdint.h>
#endif

uint64_t sys_log_string(uint64_t, uint64_t, uint64_t);
void sys_exit(int exit_code);

uint64_t sys_open(const char* path, int flags);
uint64_t sys_read(int fd, void* buf, size_t count);

#ifdef __cplusplus
}
#endif
