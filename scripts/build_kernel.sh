#!/bin/zsh

make kernel.elf -C $work_path/kernel

cp $work_path/kernel/kernel.elf $mount_point/kernel.elf