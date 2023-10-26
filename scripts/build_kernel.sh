#!/bin/zsh

IWYU_PATH=$(which include-what-you-use)
cmake -B $work_path/build $work_path/kernel -DCMAKE_CXX_INCLUDE_WHAT_YOU_USE=${IWYU_PATH}
VERBOSE=1 cmake --build $work_path/build

cp $work_path/build/UchosKernel $mount_point/kernel.elf