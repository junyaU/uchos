#!/bin/zsh

IWYU_PATH=$(which include-what-you-use)
cmake -B $work_path/build $work_path/kernel
cmake --build $work_path/build

cp $work_path/build/UchosKernel $mount_point/kernel.elf