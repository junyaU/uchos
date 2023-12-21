#!/bin/zsh

cmake -B $work_path/build $work_path/kernel
cmake --build $work_path/build

cp $work_path/build/UchosKernel $mount_point/kernel.elf

for APP in $(ls $work_path/apps); do
    if [ -f $work_path/apps/$APP/$APP ]; then
        cp $work_path/apps/$APP/$APP $mount_point/
    fi
done