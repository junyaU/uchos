#include <new>
#include <cerrno>
#include <cstddef>
#include <cstdlib>

std::new_handler std::get_new_handler() noexcept // NOLINT(cert-dcl58-cpp)
{
	return [] { exit(1); };
}

extern "C" int
posix_memalign(void** /*unused*/, size_t /*unused*/, size_t /*unused*/)
{
	return ENOMEM;
}
