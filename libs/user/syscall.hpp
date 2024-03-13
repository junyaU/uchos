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
uint64_t sys_draw_text(const char* text, int x, int y, uint32_t color);
uint64_t sys_fill_rect(int x, int y, int width, int height, uint32_t color);

#ifdef __cplusplus
}
#endif
