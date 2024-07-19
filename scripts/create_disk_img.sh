#!/bin/zsh

if [ -e $disk_img ]; then
    rm $disk_img
fi

if [ -e $storage_img ]; then
    rm $storage_img
fi

sudo mkdir -p $mount_point
sudo mkdir -p $storage_mount_point

qemu-img create -f raw $disk_img 200M
mkfs.fat -n 'UCH OS' -s 2 -f 2 -R 32 -F 32 $disk_img
sudo mount -o loop $disk_img $mount_point

# Create a storage disk image
qemu-img create -f raw $storage_img 1G
mkfs.fat -n 'UCH STORAGE' -s 4 -f 2 -R 32 -F 32 $storage_img
sudo mount -o loop $storage_img $storage_mount_point
