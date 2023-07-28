#!/bin/zsh

loader_pkg_name="UchLoaderPkg"
edk2_path="$HOME/edk2"
work_path="$HOME/uchos"

cd $edk2_path
source edksetup.sh 
build

cd $work_path
mkdir -p $mount_point/EFI/BOOT


cp ~/edk2/Build/${loader_pkg_name}X64/DEBUG_CLANGPDB/X64/Loader.efi ${mount_point}/EFI/BOOT
mv ${mount_point}/EFI/BOOT/Loader.efi ${mount_point}/EFI/BOOT/BOOTX64.EFI



