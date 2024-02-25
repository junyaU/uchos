#!/bin/zsh

loader_pkg_name="UchLoaderPkg"
edk2_path="$HOME/edk2"

cd $edk2_path
source edksetup.sh
build

cd $work_path
sudo mkdir -p $mount_point/EFI/BOOT

sudo cp ~/edk2/Build/${loader_pkg_name}X64/DEBUG_CLANG38/X64/UchLoaderPkg/Loader/OUTPUT/Loader.efi ${mount_point}/EFI/BOOT
sudo mv ${mount_point}/EFI/BOOT/Loader.efi ${mount_point}/EFI/BOOT/BOOTX64.EFI
