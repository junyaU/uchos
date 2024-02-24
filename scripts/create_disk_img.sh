#!/bin/zsh

if [ -e $disk_img ]; then
    rm $disk_img
fi

sudo mkdir -p $mount_point

qemu-img create -f raw $disk_img 200M
mkfs.fat -n 'UCH OS' -s 2 -f 2 -R 32 -F 32 $disk_img
sudo mount -o loop $disk_img $mount_point
