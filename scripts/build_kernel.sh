#!/bin/zsh

cmake -B $work_path/build $work_path/kernel
cmake --build $work_path/build

sudo cp $work_path/build/UchosKernel $mount_point/kernel.elf

BASEDIR=$work_path/x86_64-elf

export CPPFLAGS="-I$BASEDIR/include/c++/v1 -I$BASEDIR/include \
-nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS \
-DEFIAPI='__attribute__((ms_abi))'"
export LDFLAGS="-L$BASEDIR/lib"

for MKF in $(ls $work_path/userland/*/Makefile); do
    APP_DIR=$(dirname $MKF)
    APP=$(basename $APP_DIR)
    make ${MAKE_OPTS:-} -C $APP_DIR $APP
done

for APP in $(ls $work_path/userland); do
    if [ -f $work_path/userland/$APP/$APP ]; then
        sudo cp $work_path/userland/$APP/$APP $mount_point/
    fi
done

for RESOURCE in $(ls $work_path/resource); do
    sudo cp $work_path/resource/$RESOURCE $mount_point/
done
