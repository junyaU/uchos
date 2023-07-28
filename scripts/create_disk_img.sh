#!/bin/zsh

if [ -e $disk_img ]; then
    rm $disk_img
fi

mkdir -p $mount_point

qemu-img create -f raw $disk_img 200M
mkfs.fat -n 'UCH OS' -s 2 -f 2 -R 32 -F 32 $disk_img
hdiutil attach -mountpoint $mount_point $disk_img