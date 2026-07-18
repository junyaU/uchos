#!/bin/bash
#
# Headless kernel test runner for CI.
#
# Builds a minimal FAT32 boot disk (UEFI loader + kernel) with mtools, boots
# it in QEMU with no display, and judges the result from two independent
# signals:
#   1. QEMU's exit status, driven by the kernel writing to isa-debug-exit
#      (0x10 -> status 33 = all tests passed, 0x11 -> status 35 = failures)
#   2. The machine-readable "TEST_SUMMARY: ... result=PASS" marker the kernel
#      prints over COM1 (captured via -serial stdio)
#
# The kernel must be configured with -DKERNEL_TESTS=ON -DKERNEL_TEST_EXIT=ON;
# it then exits right after the main-stage suites, before kernel_service, so
# no userland binaries, virtio storage, TAP network or USB devices are needed.
# Unlike run_qemu.sh this script is non-interactive and needs no sudo, loop
# mounts or GDB stub.
#
# Usage (all inputs overridable via environment variables):
#   KERNEL_ELF=build/UchosKernel LOADER_EFI=Loader.efi ./scripts/run_kernel_tests.sh

set -euo pipefail

KERNEL_ELF="${KERNEL_ELF:-build/UchosKernel}"
LOADER_EFI="${LOADER_EFI:-Loader.efi}"
OVMF_CODE="${OVMF_CODE:-}"
OVMF_VARS="${OVMF_VARS:-}"
TIMEOUT_SEC="${TIMEOUT_SEC:-300}"
WORK_DIR="${WORK_DIR:-}"

if [ -z "$WORK_DIR" ]; then
    WORK_DIR="$(mktemp -d)"
else
    mkdir -p "$WORK_DIR"
fi

# Locate OVMF firmware if not given explicitly (package layout differs
# between Ubuntu releases).
if [ -z "$OVMF_CODE" ]; then
    for candidate in /usr/share/OVMF/OVMF_CODE_4M.fd /usr/share/OVMF/OVMF_CODE.fd OVMF_CODE.fd; do
        if [ -f "$candidate" ]; then
            OVMF_CODE="$candidate"
            break
        fi
    done
fi
if [ -z "$OVMF_VARS" ]; then
    for candidate in /usr/share/OVMF/OVMF_VARS_4M.fd /usr/share/OVMF/OVMF_VARS.fd OVMF_VARS.fd; do
        if [ -f "$candidate" ]; then
            OVMF_VARS="$candidate"
            break
        fi
    done
fi

for required in "$KERNEL_ELF" "$LOADER_EFI" "$OVMF_CODE" "$OVMF_VARS"; do
    if [ -z "$required" ] || [ ! -f "$required" ]; then
        echo "error: required file not found: ${required:-<OVMF firmware>}" >&2
        exit 1
    fi
done

disk_img="$WORK_DIR/test-disk.img"
vars_img="$WORK_DIR/OVMF_VARS.fd"
serial_log="$WORK_DIR/serial.log"

# Assemble the boot disk without mounting anything: FAT32 image holding the
# UEFI loader (as the default boot entry) and the kernel.
rm -f "$disk_img"
truncate -s 128M "$disk_img"
mkfs.fat -n 'UCH OS' -s 2 -f 2 -R 32 -F 32 "$disk_img" > /dev/null
mmd -i "$disk_img" ::/EFI ::/EFI/BOOT
mcopy -i "$disk_img" "$LOADER_EFI" ::/EFI/BOOT/BOOTX64.EFI
mcopy -i "$disk_img" "$KERNEL_ELF" ::/kernel.elf

# OVMF variable store must be writable; work on a copy.
cp "$OVMF_VARS" "$vars_img"

echo "booting kernel tests in QEMU (timeout ${TIMEOUT_SEC}s, log: $serial_log)"

set +e
timeout --foreground "$TIMEOUT_SEC" qemu-system-x86_64 -m 1G \
    -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
    -drive if=pflash,format=raw,file="$vars_img" \
    -drive if=ide,index=0,media=disk,format=raw,file="$disk_img" \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -display none -serial stdio -monitor none -no-reboot \
    | tee "$serial_log"
qemu_status=${PIPESTATUS[0]}
set -e

echo "qemu exit status: $qemu_status"

if [ "$qemu_status" -eq 124 ]; then
    echo "FAIL: timed out after ${TIMEOUT_SEC}s (kernel never reported a test result)" >&2
    exit 1
fi

# (0x10 << 1) | 1 = 33 is the only status the kernel emits on success.
if [ "$qemu_status" -ne 33 ]; then
    echo "FAIL: kernel reported test failures or crashed (status $qemu_status)" >&2
    exit 1
fi

if ! grep -q "TEST_SUMMARY:.*result=PASS" "$serial_log"; then
    echo "FAIL: exit status says PASS but no passing TEST_SUMMARY marker was seen on serial" >&2
    exit 1
fi

echo "PASS: all kernel tests passed"
