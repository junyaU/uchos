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
# Ring-3 smoke mode (issue #374): setting SMOKE_BINS to a space-separated
# list of userland binaries (must include the shell and echo) additionally
#   - assembles a virtio-blk storage disk holding those binaries,
#   - attaches the interactive boot's virtio devices (virtio-net with a
#     user-mode netdev instead of TAP; no XHCI, see below),
#   - requires the "SMOKE_TEST: ... result=PASS" serial marker.
# The kernel must then also be configured with -DKERNEL_SMOKE_TEST=ON: the
# boot continues past the test suites into userland, the shell runs one
# fork→exec→wait round-trip and the kernel exits QEMU with the combined
# result (a watchdog turns a hang into a marked FAIL).
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
SMOKE_BINS="${SMOKE_BINS:-}"
# QEMU -d categories captured into qemu-debug.log; add "int" to trace the
# exception cascade behind a silent triple fault.
QEMU_DEBUG="${QEMU_DEBUG:-cpu_reset,guest_errors}"

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

# Smoke mode: build the virtio-blk storage disk (same mkfs.fat geometry as
# scripts/create_disk_img.sh) and attach the virtio devices of the
# interactive boot, with a user-mode netdev standing in for the TAP
# interface. Without the virtio-net device the net service would poke an
# absent config space. No XHCI keyboard: the smoke test never types (the
# input path is out of scope, issue #374 approach B) and xHCI init hangs
# QEMU on the GitHub runner; without the device the USB service logs
# "xHCI device not found." and idles harmlessly.
smoke_qemu_args=()
if [ -n "$SMOKE_BINS" ]; then
    storage_img="$WORK_DIR/storage.img"
    rm -f "$storage_img"
    truncate -s 1G "$storage_img"
    mkfs.fat -n 'UCH STORAGE' -s 8 -f 2 -R 32 -F 32 "$storage_img" > /dev/null
    for bin in $SMOKE_BINS; do
        if [ ! -f "$bin" ]; then
            echo "error: smoke binary not found: $bin" >&2
            exit 1
        fi
        mcopy -i "$storage_img" "$bin" ::/
    done
    # X-PciMmio64Mb=0 keeps OVMF from opening a 64-bit MMIO window: newer
    # firmware otherwise places the virtio BARs around 512 GiB, beyond the
    # kernel's 64 GiB identity map, and the first common-cfg access triple
    # faults (exactly what the interactive boot's older firmware never did)
    smoke_qemu_args=(
        -fw_cfg name=opt/ovmf/X-PciMmio64Mb,string=0
        -drive if=none,id=vblk,format=raw,file="$storage_img"
        -device virtio-blk-pci,drive=vblk
        -netdev user,id=net0
        -device virtio-net-pci,netdev=net0,mac=52:54:00:12:34:56
    )
    echo "smoke mode: storage disk with [$SMOKE_BINS]"
fi

qemu_debug_log="$WORK_DIR/qemu-debug.log"

qemu-system-x86_64 --version | head -1
echo "booting kernel tests in QEMU (timeout ${TIMEOUT_SEC}s, log: $serial_log)"

# -d cpu_reset captures the CPU state (RIP etc.) when the guest triple
# faults: with -no-reboot that otherwise shows up only as a silent
# status-0 exit. Resolve the dumped RIP with llvm-addr2line -e $KERNEL_ELF.
set +e
timeout --foreground "$TIMEOUT_SEC" qemu-system-x86_64 -m 1G \
    -d "$QEMU_DEBUG" -D "$qemu_debug_log" \
    -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
    -drive if=pflash,format=raw,file="$vars_img" \
    -drive if=ide,index=0,media=disk,format=raw,file="$disk_img" \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    "${smoke_qemu_args[@]}" \
    -display none -serial stdio -monitor none -no-reboot \
    | tee "$serial_log"
qemu_status=${PIPESTATUS[0]}
set -e

echo "qemu exit status: $qemu_status"

if [ "$qemu_status" -ne 33 ] && [ -s "$qemu_debug_log" ]; then
    echo "---- qemu debug log (last 150 lines) ----"
    tail -150 "$qemu_debug_log"
    echo "----------------------------------------"
fi

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

if [ -n "$SMOKE_BINS" ] && ! grep -q "SMOKE_TEST:.*result=PASS" "$serial_log"; then
    echo "FAIL: exit status says PASS but no passing SMOKE_TEST marker was seen on serial" >&2
    exit 1
fi

if [ -n "$SMOKE_BINS" ]; then
    echo "PASS: all kernel tests and the ring-3 smoke test passed"
else
    echo "PASS: all kernel tests passed"
fi
