#ifdef __cplusplus
#include <cstddef>
#include <cstdint>

extern "C" {
#else
#include <stddef.h>
#include <stdint.h>
#endif

uint64_t sys_read(int fd, uint64_t buf, uint64_t count);
uint64_t sys_write(int fd, uint64_t buf, uint64_t count);
uint64_t sys_log_string(uint64_t, uint64_t, uint64_t);
void sys_exit(int exit_code);

uint64_t sys_draw_text(const char* text, int x, int y, uint32_t color);
uint64_t sys_fill_rect(int x, int y, int width, int height, uint32_t color);
uint64_t sys_time(int ms, int is_periodic, uint8_t action, int task_id);
uint64_t sys_ipc(int dest, int src, const void* m, int flags);
uint64_t sys_fork();
uint64_t sys_exec(const char* path, const char* args);
uint64_t sys_wait(int* status);
uint64_t sys_getpid();
uint64_t sys_getppid();

#ifdef __cplusplus
}
#endif
