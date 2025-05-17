# UCHos Development Guide

## Build & Test Commands
- **Build Everything**: `./run_qemu.sh` (builds loader, kernel, and runs QEMU)
- **Build Kernel**: `cmake -B build kernel && cmake --build build`
- **Lint**: `./lint.sh` (uses clang-tidy with project settings)
- **Run Tests**: `cmake -B build kernel && cmake --build build` (tests run on kernel start)
- **Run Specific Test**: Modify kernel/main.cpp to call the specific test suite:
  ```cpp
  run_test_suite(test_suite_name); // e.g., run_test_suite(memory_tests);
  ```

## Code Style Guidelines
- **C++ Standard**: C++17
- **Formatting**: 
  - Use tabs for indentation
  - Max line length ~100 characters
  - K&R style braces with newline after opening brace
- **Naming**:
  - snake_case for variables, functions, namespaces
  - CamelCase for types (classes, structs, enums)
  - ALL_CAPS for macros and constants
- **Headers**: 
  - Use #pragma once
  - Include standard headers first, then project headers
- **Errors**: Use LOG_ERROR (or appropriate log level) for error handling
- **Types**: Prefer explicit types over auto when possible
- **Memory Management**: Follow RAII principles, use smart pointers where applicable
- **Comments**: Meaningful comments for complex logic, no obvious comments