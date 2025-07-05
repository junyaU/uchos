#!/bin/zsh

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Build error tracking
BUILD_FAILED=0
FAILED_APPS=()
KERNEL_BUILD_FAILED=0

# Cleanup function to unmount on exit
cleanup() {
    if [ -n "$mount_point" ] && mountpoint -q "$mount_point" 2>/dev/null; then
        sudo umount "$mount_point" 2>/dev/null || true
    fi
    if [ -n "$storage_mount_point" ] && mountpoint -q "$storage_mount_point" 2>/dev/null; then
        sudo umount "$storage_mount_point" 2>/dev/null || true
    fi
}

# Set trap to cleanup on exit
trap cleanup EXIT

# Build kernel
printf "${BLUE}════════════════════════════════════════════════════════════════${NC}\n"
printf "${BLUE}Building UCHos Kernel...${NC}\n"
printf "${BLUE}════════════════════════════════════════════════════════════════${NC}\n"

if ! cmake -B $work_path/build $work_path/kernel; then
    KERNEL_BUILD_FAILED=1
    printf "${RED}✗ Failed to configure kernel build${NC}\n"
elif ! cmake --build $work_path/build; then
    KERNEL_BUILD_FAILED=1
    printf "${RED}✗ Failed to build kernel${NC}\n"
else
    printf "${GREEN}✓ Kernel built successfully${NC}\n"
    sudo cp $work_path/build/UchosKernel $mount_point/kernel.elf
    sudo cp $work_path/build/UchosKernel $storage_mount_point/kernel.elf
fi

if [ $KERNEL_BUILD_FAILED -eq 1 ]; then
    printf "\n${RED}════════════════════════════════════════════════════════════════${NC}\n"
    printf "${RED}❌ KERNEL BUILD FAILED${NC}\n"
    printf "${RED}Please check the build output above for errors${NC}\n"
    printf "${RED}════════════════════════════════════════════════════════════════${NC}\n\n"
    # Clean up mounts before exiting
    sudo umount $mount_point 2>/dev/null || true
    sudo umount $storage_mount_point 2>/dev/null || true
    exit 1
fi

printf "\n${BLUE}════════════════════════════════════════════════════════════════${NC}\n"
printf "${BLUE}Building Userland Applications...${NC}\n"
printf "${BLUE}════════════════════════════════════════════════════════════════${NC}\n"

BASEDIR=$work_path/x86_64-elf

export CPPFLAGS="-I$BASEDIR/include/c++/v1 -I$BASEDIR/include \
-nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS \
-DEFIAPI='__attribute__((ms_abi))'"
export LDFLAGS="-L$BASEDIR/lib"

for MKF in $(ls $work_path/userland/*/Makefile); do
    APP_DIR=$(dirname $MKF)
    APP=$(basename $APP_DIR)
    make ${MAKE_OPTS:-} -C $APP_DIR clean > /dev/null 2>&1
    if ! make ${MAKE_OPTS:-} -C $APP_DIR $APP > /dev/null 2>&1; then
        printf "${RED}✗ Failed to build userland app: $APP${NC}\n"
        BUILD_FAILED=1
        FAILED_APPS+=($APP)
    else
        printf "${GREEN}✓ Built userland app: $APP${NC}\n"
    fi
done

for MKF in $(ls $work_path/userland/commands/*/Makefile); do
    APP_DIR=$(dirname $MKF)
    APP=$(basename $APP_DIR)
    printf "${YELLOW}Building command: $APP${NC}\n"
    make ${MAKE_OPTS:-} -C $APP_DIR clean > /dev/null 2>&1
    if ! make ${MAKE_OPTS:-} -C $APP_DIR $APP; then
        printf "${RED}✗ Failed to build command: $APP${NC}\n"
        BUILD_FAILED=1
        FAILED_APPS+=("commands/$APP")
    else
        printf "${GREEN}✓ Successfully built command: $APP${NC}\n"
    fi
done

# Summary message at the end
if [ $BUILD_FAILED -eq 1 ]; then
    printf "\n${RED}════════════════════════════════════════════════════════════════${NC}\n"
    printf "${RED}❌ USERLAND BUILD FAILED: Some applications failed to build${NC}\n"
    printf "${RED}Failed applications:${NC}\n"
    for app in ${FAILED_APPS[@]}; do
        printf "${RED}  - $app${NC}\n"
    done
    printf "${RED}════════════════════════════════════════════════════════════════${NC}\n\n"
    # Exit with error code to prevent QEMU launch
    exit 1
else
    printf "\n${GREEN}════════════════════════════════════════════════════════════════${NC}\n"
    printf "${GREEN}✅ ALL BUILDS COMPLETED SUCCESSFULLY${NC}\n"
    printf "${GREEN}════════════════════════════════════════════════════════════════${NC}\n\n"
fi

for APP in $(ls $work_path/userland); do
    if [ -f $work_path/userland/$APP/$APP ]; then
        sudo cp $work_path/userland/$APP/$APP $mount_point/
        sudo cp $work_path/userland/$APP/$APP $storage_mount_point/
    fi
done

sudo mkdir -p $mount_point/commands
sudo mkdir -p $storage_mount_point/commands

for COMMAND in $(ls $work_path/userland/commands); do
    if [ -f $work_path/userland/commands/$COMMAND/$COMMAND ]; then
        sudo cp $work_path/userland/commands/$COMMAND/$COMMAND $mount_point/
        sudo cp $work_path/userland/commands/$COMMAND/$COMMAND $storage_mount_point/
    fi
done

for RESOURCE in $(ls $work_path/resource); do
    sudo cp $work_path/resource/$RESOURCE $mount_point/
    sudo cp $work_path/resource/$RESOURCE $storage_mount_point/
done
