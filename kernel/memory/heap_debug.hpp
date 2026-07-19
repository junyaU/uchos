/**
 * @file memory/heap_debug.hpp
 * @brief Optional heap-corruption instrumentation for the slab allocator
 *
 * A minimal, hand-rolled substitute for AddressSanitizer, which cannot run in
 * a freestanding kernel. When the KERNEL_HEAP_DEBUG CMake option is ON the
 * slab allocator (memory/slab.cpp) routes every alloc()/free() through the
 * hooks declared here to catch four classes of heap bug at the point of the
 * violation instead of at some unrelated later crash:
 *
 *   - use-after-free : freed objects are poisoned and re-checked at reuse
 *   - buffer overflow: a canary redzone brackets every allocation
 *   - double / invalid free: freed pointers are validated against a side table
 *   - leaks          : every live allocation records its caller and size
 *
 * When the option is OFF this whole header is empty and slab.cpp compiles to
 * exactly the same code as before: there is zero footprint on release builds.
 */

#pragma once

#ifdef KERNEL_HEAP_DEBUG_ENABLED

#include <cstddef>

namespace kernel::memory::heap_debug
{

/// Minimum bytes of canary placed immediately after every allocation.
///
/// The redzone sits in the slab object's slack, after the payload; the payload
/// pointer itself stays at the object boundary so the allocator keeps handing
/// back naturally-aligned memory (kernel code, TSS/IST stacks, page tables and
/// DMA buffers all rely on that). Underflow is therefore not detected -- doing
/// so would require shifting the pointer off its natural boundary.
constexpr size_t REDZONE_SIZE = 16;

/**
 * @brief Running counts of the violations detected since boot
 *
 * Exposed so that tests can assert a deliberately-planted fault was caught.
 */
struct Stats {
	size_t double_free;	   ///< free() of an untracked / already-freed pointer
	size_t use_after_free; ///< freed-object poison found corrupted at reuse
	size_t redzone;		   ///< tail canary found corrupted at free() (overflow)
};

/**
 * @brief Reset all tracking state
 * @note Called from initialize_slab_allocator()
 */
void initialize();

/**
 * @brief Extra bytes to add to a request before power-of-two rounding
 * @return REDZONE_SIZE, guaranteeing a tail redzone even when the request is
 * already a power of two
 */
size_t redzone_reserve();

/**
 * @brief Post-process a freshly obtained slab object
 *
 * Verifies the object's freed-poison (use-after-free), installs the tail
 * redzone in the object's slack, and records provenance. The payload pointer is
 * the object boundary itself, so its natural alignment is preserved.
 *
 * @param raw Slab object base as returned by MCache::alloc()
 * @param user_size Bytes the caller asked for
 * @param object_size Object size of the owning cache
 * @param caller __builtin_return_address(0) captured in alloc()
 * @return Pointer to hand back to the caller (equal to @p raw)
 */
void* on_alloc(void* raw, size_t user_size, size_t object_size, void* caller);

/**
 * @brief Map a user pointer back to its raw slab object
 * @param user Pointer previously returned by on_alloc()
 * @return Raw slab object, or nullptr if @p user is not a live allocation
 */
void* raw_from_user(void* user);

/**
 * @brief Validate and retire a tracked allocation on free()
 *
 * Detects double/invalid free and redzone corruption, poisons the object so a
 * later use-after-free can be caught, and returns the raw slab object to hand
 * back to the slab.
 *
 * @param user Pointer passed to free()
 * @param caller __builtin_return_address(0) captured in free()
 * @return Raw slab object, or nullptr if @p user was not a live allocation
 * (the violation has already been reported)
 */
void* on_free(void* user, void* caller);

/**
 * @brief Sum of the requested sizes of all currently-live allocations
 * @note Used by the test runner to bracket each suite for leaks
 */
size_t live_bytes();

/// @brief Number of currently-live tracked allocations
size_t live_count();

/**
 * @brief Log the @p top_n allocation sites holding the most bytes
 * @note Resolve the printed caller addresses with
 *       llvm-addr2line -e build/UchosKernel <addr>
 */
void report_leaks(int top_n);

/// @brief Snapshot of the violation counters
Stats stats();

} // namespace kernel::memory::heap_debug

#endif // KERNEL_HEAP_DEBUG_ENABLED
