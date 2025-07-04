#!/bin/bash

# environment : Ubuntu 22.04 LTS

export PATH=/usr/local/Cellar/dosfstools/4.2/sbin:$PATH
export PATH=/usr/local/opt/llvm/bin:$PATH

mount_point="mnt"
storage_mount_point="smnt"
disk_img="disk.img"
storage_img="storage.img"
work_path="$HOME/src/github.com/junyaU/uchos"

source ./scripts/create_disk_img.sh

if ! source ./scripts/build_loader.sh; then
    echo "Loader build failed. Aborting QEMU launch."
    exit 1
fi

if ! source ./scripts/build_kernel.sh; then
    echo "Kernel or userland build failed. Aborting QEMU launch."
    exit 1
fi

sudo umount $mount_point
sudo umount $storage_mount_point

qemu-system-x86_64 -m 1G \
    -d cpu_reset \
    -drive if=pflash,format=raw,file=OVMF_CODE.fd \
    -drive if=pflash,format=raw,file=OVMF_VARS.fd \
    -drive if=ide,index=0,media=disk,format=raw,file=$disk_img \
    -device nec-usb-xhci,id=xhci \
    -device usb-kbd \
    -drive if=none,id=drive-virtio-disk0,format=raw,file=$storage_img \
    -device virtio-blk-pci,drive=drive-virtio-disk0,id=virtio-disk0 \
    -monitor stdio -S -gdb tcp::12345
