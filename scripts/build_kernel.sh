#!/bin/zsh

cmake -B $work_path/build $work_path/kernel
make -C $work_path/build

cp $work_path/build/UchosKernel $mount_point/kernel.elf