#include "file_info.hpp"
#include <cstddef>
#include <libs/common/types.hpp>

const size_t MAX_FILE_ID = 50;
std::unordered_map<fs_id_t, file_cache> file_caches;
size_t id_offset = 0;

file_cache* find_file_cache_by_path(const char* path)
{
	for (auto& [id, cache] : file_caches) {
		if (strcmp(cache.path, path) == 0) {
			return &cache;
		}
	}

	return nullptr;
}

file_cache* create_file_cache(const char* path, size_t total_size, pid_t requester)
{
	fs_id_t id = generate_fs_id();

	file_cache cache(total_size, id, requester);
	memcpy(cache.path, path, strlen(path));
	auto result = file_caches.emplace(id, cache);

	return &result.first->second;
}

// TODO: Handle overflow
fs_id_t generate_fs_id() { return id_offset++ % MAX_FILE_ID; }

void init_read_contexts()
{
	file_caches = std::unordered_map<fs_id_t, file_cache>();
}
