#!/bin/bash

# Find all C++ source and header files and format them
# Exclude x86_64-elf directory

find . -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.cc" \) \
  ! -path "./x86_64-elf/*" \
  ! -path "./build/*" \
  ! -path "./UchLoaderPkg/*" \
  -exec echo "Formatting: {}" \; \
  -exec clang-format-18 -i {} \;

echo "Formatting complete!"
