# UCHos Error Handling API Documentation

## Overview

The UCHos kernel provides a unified error handling mechanism. This document describes the APIs, macros, and best practices related to error handling, as implemented in `kernel/error.hpp`.

## Error Codes

### Predefined Error Codes

```cpp
constexpr int OK = 0;                          // Success
constexpr int ERR_FORK_FAILED = -1;           // Fork failed
constexpr int ERR_NO_MEMORY = -2;             // Out of memory
constexpr int ERR_INVALID_ARG = -3;           // Invalid argument
constexpr int ERR_INVALID_TASK = -4;          // Invalid task
constexpr int ERR_INVALID_FD = -5;            // Invalid file descriptor
constexpr int ERR_PAGE_NOT_PRESENT = -6;      // Page not present
constexpr int ERR_NO_TASK = -7;               // Task does not exist
constexpr int ERR_NO_FILE = -8;               // File does not exist
constexpr int ERR_NO_DEVICE = -9;             // Device does not exist
constexpr int ERR_FAILED_INIT_DEVICE = -10;   // Device initialization failed
constexpr int ERR_FAILED_WRITE_TO_DEVICE = -11; // Failed to write to device
```

## Basic Error Checking

### Error Checking Functions

```cpp
// Check if an error code indicates success
bool is_ok(error_t err);

// Check if an error code indicates an error
bool is_error(error_t err);

// Convert error code to string
const char* error_to_string(error_t err);
```

### Usage Example

```cpp
error_t result = some_function();
if (kernel::is_error(result)) {
    LOG_ERROR("Operation failed: %s", kernel::error_to_string(result));
    return result;
}
```

## Error Handling Macros

### RETURN_IF_ERROR

Returns immediately if the expression evaluates to an error.

```cpp
error_t do_something() {
    RETURN_IF_ERROR(initialize_device());
    RETURN_IF_ERROR(configure_device());
    return OK;
}
```

### LOG_ERROR_CODE

Logs an error message with an error code.

```cpp
if (buffer == nullptr) {
    LOG_ERROR_CODE(ERR_NO_MEMORY, "Failed to allocate buffer");
    return ERR_NO_MEMORY;
}
```

## Panic and Assertions

### PANIC

Halts the system when an unrecoverable error occurs.

```cpp
if (critical_invariant_violated()) {
    PANIC("Critical invariant violated");
}
```

### ASSERT

Triggers a panic if the condition is false.

```cpp
void set_value(int* ptr, int value) {
    ASSERT(ptr != nullptr);
    *ptr = value;
}
```

### ASSERT_NOT_NULL

Asserts that a pointer is not NULL.

```cpp
void process_buffer(void* buffer) {
    ASSERT_NOT_NULL(buffer);
    // buffer is guaranteed to be non-NULL
}
```

### ASSERT_IN_RANGE

Asserts that a value is within the specified range.

```cpp
void set_priority(int priority) {
    ASSERT_IN_RANGE(priority, 0, MAX_PRIORITY);
    // priority is guaranteed to be in range
}
```

## Best Practices

1. **Early Return**: Detect errors early and return immediately
2. **Error Logging**: Always output appropriate logs on error
3. **Resource Management**: Don't forget to release resources on error
4. **Error Propagation**: Properly propagate lower-layer errors to upper layers
5. **Appropriate Error Codes**: Use appropriate error codes for the situation

## Example: Complete Error Handling

```cpp
error_t initialize_device(device_t* dev) {
    ASSERT_NOT_NULL(dev);

    if (kernel::is_error(detect_device(dev))) {
        LOG_ERROR_CODE(ERR_NO_DEVICE, "Failed to detect device %s", dev->name);
        return ERR_NO_DEVICE;
    }

    RETURN_IF_ERROR(load_device_config(dev));
    RETURN_IF_ERROR(enable_device(dev));

    return OK;
}
```
