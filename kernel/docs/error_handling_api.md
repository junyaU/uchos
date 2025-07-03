# UCHos Error Handling API Documentation

## Overview

The UCHos kernel provides a unified error handling mechanism. This document describes the APIs, macros, and best practices related to error handling.

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
if (is_error(result)) {
    LOG_ERROR("Operation failed: %s", error_to_string(result));
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

### LOG_AND_RETURN_IF_ERROR

Logs an error message and returns if the expression evaluates to an error.

```cpp
error_t process_data(void* data) {
    LOG_AND_RETURN_IF_ERROR(validate_data(data), 
                           "Failed to validate data: %p", data);
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

## Advanced Types

### result<T> Type

A type that holds either a value on success or an error code on failure.

```cpp
kernel::result<int> divide(int a, int b) {
    if (b == 0) {
        return ERR_INVALID_ARG;
    }
    return a / b;
}

// Usage example
auto result = divide(10, 2);
if (result.is_ok()) {
    printf("Result: %d\n", result.value());
} else {
    printf("Error: %s\n", error_to_string(result.error()));
}
```

### optional<T> Type

A type that represents whether a value exists or not.

```cpp
kernel::optional<size_t> find_free_index() {
    for (size_t i = 0; i < MAX_SIZE; i++) {
        if (is_free(i)) {
            return i;
        }
    }
    return {};  // empty optional
}

// Usage example
auto index = find_free_index();
if (index.has_value()) {
    use_index(index.value());
} else {
    // Index not found
}
```

### error_chain Class

Chains multiple operations with concise error handling.

```cpp
error_t initialize_system() {
    return kernel::error_chain()
        .then([]() { return load_config(); })
        .then([]() { return initialize_memory(); })
        .then([]() { return start_scheduler(); })
        .on_error([](error_t err) {
            LOG_ERROR("System initialization failed: %s", error_to_string(err));
        })
        .result();
}
```

## Memory Allocation Error Handling

### KMALLOC_OR_RETURN

Returns immediately if memory allocation fails.

```cpp
void process() {
    char* buffer;
    KMALLOC_OR_RETURN(buffer, 1024, KMALLOC_ZEROED);
    // buffer is successfully allocated
}
```

### KMALLOC_OR_RETURN_ERROR

Returns an error code if memory allocation fails.

```cpp
error_t allocate_resources() {
    struct resource* res;
    KMALLOC_OR_RETURN_ERROR(res, sizeof(*res), 0);
    // res is successfully allocated
    return OK;
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

## Helper Functions

### try_with_cleanup

Automatically executes cleanup on error.

```cpp
error_t process_with_temp_buffer() {
    void* temp = kernel::memory::kmalloc(4096, 0);
    if (temp == nullptr) {
        return ERR_NO_MEMORY;
    }
    
    return kernel::try_with_cleanup(
        [&]() {
            // Main processing
            return do_complex_operation(temp);
        },
        [&]() {
            // Cleanup on error
            kernel::memory::kfree(temp);
        }
    );
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
    
    // Allocate resources
    void* config_buffer;
    KMALLOC_OR_RETURN_ERROR(config_buffer, CONFIG_SIZE, KMALLOC_ZEROED);
    
    // Device initialization chain
    error_t result = kernel::error_chain()
        .then([&]() { return detect_device(dev); })
        .then([&]() { return load_device_config(dev, config_buffer); })
        .then([&]() { return enable_device(dev); })
        .on_error([&](error_t err) {
            LOG_ERROR_CODE(err, "Failed to initialize device %s", dev->name);
            kernel::memory::kfree(config_buffer);
        })
        .result();
    
    if (is_ok(result)) {
        kernel::memory::kfree(config_buffer);
    }
    
    return result;
}
```