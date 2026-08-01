#!/bin/bash
#
# Replay a CI run's booted binaries on the local QEMU ("replay-first").
#
# When CI fails, the first question is "bad code or different environment?"
# Rebuilding locally cannot answer it: the local toolchain produces a
# different binary. This script downloads the EXACT kernel and userland
# binaries the CI run booted (the smoke-debug-binaries artifact) and feeds
# them to scripts/run_kernel_tests.sh unchanged, so the only remaining
# variable is the environment (QEMU/OVMF):
#   - fails locally too  -> deterministic code bug; debug locally, no CI loop
#   - passes locally     -> the difference is the environment, not the code
# (This one experiment is what pinned issue #384 down to the QEMU exception
# delivery difference.)
#
# Usage:
#   ./scripts/replay_ci_run.sh [run-id]
#
#   run-id     GitHub Actions run id (the number in the run's URL). When
#              omitted, the most recent FAILED run is used.
#
# Environment overrides (all optional):
#   LOADER_EFI / OVMF_CODE / OVMF_VARS  as in run_kernel_tests.sh; the
#              loader is additionally searched in the local EDK2 build tree
#   QEMU_DEBUG e.g. "int,cpu_reset,guest_errors" for an exception trace
#   TIMEOUT_SEC / WORK_DIR              passed through
#
# Requires: gh (authenticated), plus run_kernel_tests.sh's own dependencies.

set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
run_id="${1:-}"

if [ -z "$run_id" ]; then
    run_id="$(gh api "repos/{owner}/{repo}/actions/runs?status=failure&per_page=1" \
                  --jq '.workflow_runs[0].id')"
    if [ -z "$run_id" ] || [ "$run_id" = "null" ]; then
        echo "error: no failed run found; pass a run id explicitly" >&2
        exit 1
    fi
    echo "replaying the most recent failed run: $run_id"
fi

WORK_DIR="${WORK_DIR:-$(mktemp -d -t uchos-replay-XXXX)}"
bins_dir="$WORK_DIR/ci-bins"
ci_logs_dir="$WORK_DIR/ci-logs"

echo "downloading artifacts of run $run_id into $WORK_DIR"
gh run download "$run_id" -n smoke-debug-binaries -D "$bins_dir" || {
    echo "error: run $run_id has no smoke-debug-binaries artifact" >&2
    echo "(only runs that reached the build steps upload it)" >&2
    exit 1
}
# The CI serial log rides along for comparing against the local replay.
gh run download "$run_id" -n kernel-test-serial-log -D "$ci_logs_dir" ||
    echo "note: no kernel-test-serial-log artifact on this run"

kernel_elf="$bins_dir/build/UchosKernel"
shell_bin="$bins_dir/userland/shell/shell"
echo_bin="$bins_dir/userland/commands/echo/echo"
if [ ! -f "$kernel_elf" ]; then
    echo "error: artifact did not contain build/UchosKernel" >&2
    exit 1
fi

smoke_bins=""
if [ -f "$shell_bin" ] && [ -f "$echo_bin" ]; then
    smoke_bins="$shell_bin $echo_bin"
else
    echo "note: userland binaries missing from the artifact; kernel tests only"
fi

# The loader is not part of the artifact (it is environment-neutral EFI and
# rarely the suspect); fall back to the local EDK2 build tree.
if [ -z "${LOADER_EFI:-}" ]; then
    for candidate in "$repo_root/Loader.efi" \
        "$HOME/edk2/Build/UchLoaderPkgX64/DEBUG_CLANG38/X64/UchLoaderPkg/Loader/OUTPUT/Loader.efi"; do
        if [ -f "$candidate" ]; then
            LOADER_EFI="$candidate"
            break
        fi
    done
fi

echo "kernel : $kernel_elf"
echo "smoke  : ${smoke_bins:-<none>}"
echo "loader : ${LOADER_EFI:-<not found>}"

cd "$repo_root"
set +e
KERNEL_ELF="$kernel_elf" \
    LOADER_EFI="${LOADER_EFI:-}" \
    SMOKE_BINS="$smoke_bins" \
    WORK_DIR="$WORK_DIR/qemu" \
    ./scripts/run_kernel_tests.sh
status=$?
set -e

echo ""
echo "== replay verdict =========================================="
echo "local replay exit: $status (0 = the CI binaries PASS here)"
if [ -f "$ci_logs_dir/serial.log" ]; then
    echo "compare serial logs:"
    echo "  CI    : $ci_logs_dir/serial.log"
    echo "  local : $WORK_DIR/qemu/serial.log"
fi
if [ "$status" -eq 0 ]; then
    echo "-> the exact CI binaries pass locally: suspect the ENVIRONMENT"
    echo "   (QEMU/OVMF behaviour), not the code or the toolchain"
else
    echo "-> fails locally too: deterministic code bug, debug it here"
    echo "   (rerun with QEMU_DEBUG=int,cpu_reset,guest_errors for a trace)"
fi
exit "$status"
