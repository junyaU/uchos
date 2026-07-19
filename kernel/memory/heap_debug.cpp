/**
 * @file memory/heap_debug.cpp
 * @brief Implementation of the slab-allocator heap-corruption instrumentation
 *
 * The whole translation unit is empty unless KERNEL_HEAP_DEBUG_ENABLED is set,
 * so the release build carries none of this code.
 *
 * Side tables (std::unordered_map) are allocated through the C++ runtime, which
 * is backed by newlib's malloc/sbrk heap, not the slab allocator. That keeps
 * these hooks from recursing back into alloc()/free() while they run inside
 * them (the same property the alignment map in slab.cpp already relies on).
 */

#include "memory/heap_debug.hpp"

#ifdef KERNEL_HEAP_DEBUG_ENABLED

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include "log/log.hpp"

namespace kernel::memory::heap_debug
{
namespace
{

// Fill byte for the redzones bracketing each allocation. A one-byte write past
// either end lands here and is caught at free().
constexpr uint8_t REDZONE_BYTE = 0xF9;

// 0xDEADBEEF stamped over a freed object. If anything writes to the object
// while it is free (use-after-free), the pattern breaks and the corruption is
// caught when the object is next handed out.
constexpr uint32_t POISON_WORD = 0xDEADBEEF;

inline uint8_t poison_byte(size_t i)
{
	return static_cast<uint8_t>(POISON_WORD >> (8 * (i & 3)));
}

struct AllocInfo {
	void* raw;			///< Slab object base (start of the head redzone)
	size_t user_size;	///< Bytes the caller asked for
	size_t object_size; ///< Slab object size backing this allocation
	void* caller;		///< __builtin_return_address(0) of the alloc() caller
};

// Live allocations keyed by the user pointer handed to the caller.
std::unordered_map<void*, AllocInfo> live_allocs;

// Objects currently free and holding the poison pattern, keyed by raw base and
// mapping to object size. Distinguishes a reused (previously freed) object,
// whose poison must be intact, from a fresh slab object never poisoned.
std::unordered_map<void*, size_t> poisoned_objs;

Stats g_stats = { 0, 0, 0 };

// Depth of nested ExpectedViolation scopes currently alive. While non-zero,
// violation LOG_ERROR calls are suppressed (stats() still updates).
int g_expected_violations = 0;

void poison_fill(void* addr, size_t n)
{
	auto* p = static_cast<uint8_t*>(addr);
	for (size_t i = 0; i < n; ++i) {
		p[i] = poison_byte(i);
	}
}

bool poison_intact(const void* addr, size_t n)
{
	const auto* p = static_cast<const uint8_t*>(addr);
	for (size_t i = 0; i < n; ++i) {
		if (p[i] != poison_byte(i)) {
			return false;
		}
	}
	return true;
}

void redzone_fill(void* addr, size_t n) { memset(addr, REDZONE_BYTE, n); }

bool redzone_intact(const void* addr, size_t n)
{
	const auto* p = static_cast<const uint8_t*>(addr);
	for (size_t i = 0; i < n; ++i) {
		if (p[i] != REDZONE_BYTE) {
			return false;
		}
	}
	return true;
}

} // namespace

void initialize()
{
	// The kernel never runs global constructors (KernelMain jumps straight to
	// Main), so these globals are only zero-initialized: a zeroed unordered_map
	// has max_load_factor == 0, and the first insert would divide by it and
	// spin forever. Assigning a freshly-constructed map installs a valid state,
	// exactly as slab.cpp does for aligned_to_raw_addr_map.
	live_allocs = std::unordered_map<void*, AllocInfo>();
	poisoned_objs = std::unordered_map<void*, size_t>();
	g_stats = { 0, 0, 0 };
	g_expected_violations = 0;
}

size_t redzone_reserve() { return REDZONE_SIZE; }

void* on_alloc(void* raw, size_t user_size, size_t object_size, void* caller)
{
	// A previously-freed object must still hold its poison. If not, something
	// wrote to it after it was freed.
	auto pit = poisoned_objs.find(raw);
	if (pit != poisoned_objs.end()) {
		if (!poison_intact(raw, pit->second)) {
			++g_stats.use_after_free;
			if (g_expected_violations == 0) {
				LOG_ERROR(
						"heap-debug: use-after-free in object %p (size %lu), "
						"reallocated from %p",
						raw, static_cast<unsigned long>(pit->second), caller);
			}
		}
		poisoned_objs.erase(pit);
	}

	// The payload starts at the object boundary so its natural alignment is
	// preserved (kernel stacks, page tables and DMA buffers rely on that).
	// alloc() inflated the request by redzone_reserve(), so the slack after the
	// payload holds the tail redzone:
	//   [raw, raw + user_size)        payload
	//   [raw + user_size, raw + obj)  tail redzone (>= REDZONE_SIZE)
	const auto base = reinterpret_cast<uintptr_t>(raw);
	const uintptr_t tail = base + user_size;
	redzone_fill(reinterpret_cast<void*>(tail), base + object_size - tail);

	live_allocs[raw] = AllocInfo{ raw, user_size, object_size, caller };
	return raw;
}

void* raw_from_user(void* user)
{
	auto it = live_allocs.find(user);
	return it == live_allocs.end() ? nullptr : it->second.raw;
}

void* on_free(void* user, void* caller)
{
	auto it = live_allocs.find(user);
	if (it == live_allocs.end()) {
		++g_stats.double_free;
		if (g_expected_violations == 0) {
			LOG_ERROR("heap-debug: invalid or double free of %p (freed from %p)",
					  user, caller);
		}
		return nullptr;
	}

	const AllocInfo info = it->second;
	const auto base = reinterpret_cast<uintptr_t>(info.raw);

	// Tail redzone: [raw + user_size, raw + object_size)
	const uintptr_t tail = base + info.user_size;
	if (!redzone_intact(reinterpret_cast<void*>(tail),
						base + info.object_size - tail)) {
		++g_stats.redzone;
		if (g_expected_violations == 0) {
			LOG_ERROR(
					"heap-debug: buffer overflow on %p (size %lu, allocated from "
					"%p, freed from %p)",
					user, static_cast<unsigned long>(info.user_size), info.caller,
					caller);
		}
	}

	poison_fill(info.raw, info.object_size);
	poisoned_objs[info.raw] = info.object_size;
	live_allocs.erase(it);
	return info.raw;
}

size_t live_bytes()
{
	size_t total = 0;
	for (const auto& kv : live_allocs) {
		total += kv.second.user_size;
	}
	return total;
}

size_t live_count() { return live_allocs.size(); }

void report_leaks(int top_n)
{
	// Aggregate live allocations by their recorded caller.
	std::unordered_map<void*, size_t> bytes_by_caller;
	std::unordered_map<void*, size_t> count_by_caller;
	for (const auto& kv : live_allocs) {
		bytes_by_caller[kv.second.caller] += kv.second.user_size;
		count_by_caller[kv.second.caller] += 1;
	}

	LOG_TEST("heap-debug: %lu live allocations, %lu bytes; top %d callers:",
			 static_cast<unsigned long>(live_allocs.size()),
			 static_cast<unsigned long>(live_bytes()), top_n);

	// Selection sort over the distinct callers: the table is tiny and this
	// avoids pulling <algorithm> and a comparator into a debug-only path.
	std::unordered_map<void*, bool> done;
	for (int rank = 0; rank < top_n; ++rank) {
		void* best = nullptr;
		size_t best_bytes = 0;
		for (const auto& kv : bytes_by_caller) {
			if (done[kv.first]) {
				continue;
			}
			if (best == nullptr || kv.second > best_bytes) {
				best = kv.first;
				best_bytes = kv.second;
			}
		}
		if (best == nullptr) {
			break;
		}
		done[best] = true;
		LOG_TEST("  #%d %p: %lu bytes in %lu allocations", rank + 1, best,
				 static_cast<unsigned long>(best_bytes),
				 static_cast<unsigned long>(count_by_caller[best]));
	}
}

Stats stats() { return g_stats; }

ExpectedViolation::ExpectedViolation() { ++g_expected_violations; }

ExpectedViolation::~ExpectedViolation() { --g_expected_violations; }

} // namespace kernel::memory::heap_debug

#endif // KERNEL_HEAP_DEBUG_ENABLED
