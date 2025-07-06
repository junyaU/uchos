#!/bin/bash
run-clang-tidy -p build \
  -clang-tidy-binary "clang-tidy-18" \
  -header-filter='^(?!.*/x86_64-elf/).*' \
  -quiet 2>&1 | grep -v "warnings generated"
