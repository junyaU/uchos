#!/bin/bash

# Guard (issue #360): a broken .clang-tidy (e.g. comments inside the Checks
# string) silently disables every check, and run-clang-tidy then "succeeds"
# without inspecting anything. Fail loudly instead.
if ! clang-tidy-18 --list-checks -p build kernel/main.cpp 2>/dev/null |
	grep -q '^[[:space:]]\{1,\}[a-z]'; then
	echo "lint.sh: no clang-tidy checks enabled — .clang-tidy is broken" >&2
	exit 1
fi

run-clang-tidy -p build \
  -clang-tidy-binary "clang-tidy-18" \
  -header-filter='^(?!.*/x86_64-elf/).*' \
  -quiet 2>&1 | grep -v "warnings generated"

# The pipeline's exit status is grep's (1 when lint output is empty, i.e.
# clean); report run-clang-tidy's own status instead. Warnings are gated by
# the caller (CI greps for "warning:|error:").
exit "${PIPESTATUS[0]}"
