#include <cerrno>
#include <new>

std::new_handler std::get_new_handler() noexcept
{
	return [] { exit(1); };
}

extern "C" int posix_memalign(void**, size_t, size_t) { return ENOMEM; }
