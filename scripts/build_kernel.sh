#!/bin/zsh

cmake -B $work_path/build $work_path/kernel
cmake --build $work_path/build

sudo cp $work_path/build/UchosKernel $mount_point/kernel.elf

for APP in $(ls $work_path/userland); do
    if [ -f $work_path/userland/$APP/$APP ]; then
        sudo cp $work_path/userland/$APP/$APP $mount_point/
    fi
done