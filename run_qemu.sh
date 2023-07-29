#!/bin/zsh

export PATH=/usr/local/Cellar/dosfstools/4.2/sbin:$PATH
export PATH=/usr/local/opt/llvm/bin:$PATH

mount_point="mnt"
disk_img="disk.img"
work_path="$HOME/uchos"

source ./scripts/create_disk_img.sh
source ./scripts/build_loader.sh
source ./scripts/build_kernel.sh

hdiutil detach $mount_point

qemu-system-x86_64 -m 1G -drive if=pflash,format=raw,file=OVMF_CODE.fd -drive if=pflash,format=raw,file=OVMF_VARS.fd -drive if=ide,index=0,media=disk,format=raw,file=$disk_img -device nec-usb-xhci,id=xhci -device usb-mouse -device usb-kbd -monitor stdio -S
