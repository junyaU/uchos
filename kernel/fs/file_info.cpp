#include "file_info.hpp"
#include <cstddef>
#include <cstring>
#include <libs/common/process_id.hpp>
#include <libs/common/types.hpp>
#include <unordered_map>

namespace kernel::fs
{

const size_t MAX_FILE_CACHES = 50;
FileCacheMap file_caches;

namespace
{
fs_id_t next_fs_id = 0;
uint64_t lru_tick = 0;
} // namespace

FileCache* find_file_cache_by_path(FileCacheMap& caches, const char* path)
{
	for (auto& [id, cache] : caches) {
		if (strcmp(cache.path, path) == 0) {
			cache.last_used = ++lru_tick;
			return &cache;
		}
	}

	return nullptr;
}

FileCache* find_file_cache_by_path(const char* path)
{
	return find_file_cache_by_path(file_caches, path);
}

FileCache* create_file_cache(FileCacheMap& caches,
							 const char* path,
							 size_t total_size,
							 ProcessId requester)
{
	if (caches.size() >= MAX_FILE_CACHES) {
		// Evict the least recently used entry to make room.
		auto oldest = caches.begin();
		for (auto it = caches.begin(); it != caches.end(); ++it) {
			if (it->second.last_used < oldest->second.last_used) {
				oldest = it;
			}
		}

		caches.erase(oldest);
	}

	const fs_id_t id = generate_fs_id();

	FileCache cache(total_size, id, requester);
	strncpy(cache.path, path, sizeof(cache.path) - 1);
	cache.path[sizeof(cache.path) - 1] = '\0';
	cache.last_used = ++lru_tick;

	auto result = caches.emplace(id, cache);

	return &result.first->second;
}

FileCache* create_file_cache(const char* path,
							 size_t total_size,
							 ProcessId requester)
{
	return create_file_cache(file_caches, path, total_size, requester);
}

bool remove_file_cache_by_path(const char* path)
{
	for (auto it = file_caches.begin(); it != file_caches.end(); ++it) {
		if (strcmp(it->second.path, path) == 0) {
			file_caches.erase(it);
			return true;
		}
	}

	return false;
}

fs_id_t generate_fs_id()
{
	// Monotonically increasing so ids are never reused across the cache
	// (the old modulo scheme recycled ids and returned another file's
	// cache). 0 is skipped: it is the invalid/untracked request id.
	++next_fs_id;
	if (next_fs_id == 0) {
		++next_fs_id;
	}

	return next_fs_id;
}

void init_read_contexts() { file_caches = std::unordered_map<fs_id_t, FileCache>(); }

} // namespace kernel::fs
