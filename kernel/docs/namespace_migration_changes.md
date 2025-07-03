# UCHos Namespace Migration and Interface Unification Changelog

## Overview

This document summarizes the changes made during the namespace migration and interface unification of the UCHos kernel.

## Major Changes

### 1. Namespace Organization

All kernel code has been migrated to appropriate namespaces:

- `kernel::memory` - Memory management
- `kernel::graphics` - Graphics and display
- `kernel::interrupt` - Interrupt handling
- `kernel::timers` - Timer management
- `kernel::task` - Task and process management
- `kernel::hw` - Hardware-related
  - `kernel::hw::pci` - PCI devices
  - `kernel::hw::usb` - USB devices
  - `kernel::hw::virtio` - VirtIO devices
- `kernel::fs` - File system
- `kernel::syscall` - System calls

### 2. Initialization Function Unification

All initialization functions have been changed to follow a unified pattern:

**Before:**
```cpp
void initialize_bootstrap_allocator(const MemoryMap& mem_map);
void initialize_screen(const FrameBufferConfig& config, const Color& bg_color);
void initialize_timer();
```

**After:**
```cpp
namespace kernel::memory {
    void initialize(const MemoryMap& mem_map);
}

namespace kernel::graphics {
    void initialize(const FrameBufferConfig& config, const Color& bg_color);
}

namespace kernel::timers {
    void initialize();
}
```

### 3. Memory Management API Namespace Migration

`kmalloc` and `kfree` have been migrated to the `kernel::memory` namespace:

**Before:**
```cpp
void* ptr = kmalloc(size, flags);
kfree(ptr);
```

**After:**
```cpp
void* ptr = kernel::memory::kmalloc(size, flags);
kernel::memory::kfree(ptr);
```

### 4. Unified Error Handling

#### New Error Handling Headers

- `kernel/error.hpp` - Unified error handling interface
- `kernel/panic.hpp` - Panic and assertion facilities

#### Key Features

1. **Error Checking Functions**
   - `is_ok(error_t)` - Check for success
   - `is_error(error_t)` - Check for error
   - `error_to_string(error_t)` - Convert error code to string

2. **Error Handling Macros**
   - `RETURN_IF_ERROR(expr)` - Return immediately on error
   - `LOG_AND_RETURN_IF_ERROR(expr, fmt, ...)` - Log and return on error
   - `LOG_ERROR_CODE(err, fmt, ...)` - Log with error code

3. **Advanced Types**
   - `result<T>` - Value on success, error code on failure
   - `optional<T>` - Represents presence/absence of value
   - `error_chain` - Chain error handling

4. **Panic and Assertions**
   - `PANIC(fmt, ...)` - Kernel panic
   - `ASSERT(condition)` - Conditional assertion
   - `ASSERT_NOT_NULL(ptr)` - NULL check
   - `ASSERT_IN_RANGE(val, min, max)` - Range check

#### Error Code Cleanup

Fixed duplicate error code values for consistency:

```cpp
constexpr int OK = 0;
constexpr int ERR_FORK_FAILED = -1;
constexpr int ERR_NO_MEMORY = -2;  // Previously was -1, now fixed
// ... continued with sequential numbering
```

### 5. Segment Constants Namespace Migration

Kernel segment constants have been migrated to the `kernel::memory` namespace:

**Before:**
```cpp
ctx.cs = KERNEL_CS;
ctx.ss = KERNEL_SS;
```

**After:**
```cpp
ctx.cs = kernel::memory::KERNEL_CS;
ctx.ss = kernel::memory::KERNEL_SS;
```

## Migration Guidelines

1. **Updating Existing Code**
   - Add `kernel::memory::` prefix to all `kmalloc`/`kfree` calls
   - Update initialization function calls to new namespace pattern
   - Replace error handling with new macros

2. **Writing New Code**
   - Use appropriate namespaces
   - Follow unified error handling patterns
   - Write documentation comments

3. **Build Error Resolution**
   - For missing namespace errors, add appropriate prefix
   - Avoid `using namespace`, prefer explicit namespace qualification

## Future Work

1. Further unification of memory management interfaces
2. Device driver interface unification
3. File system API cleanup
4. System call interface improvements