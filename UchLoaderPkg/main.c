#include <Guid/FileInfo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Uefi.h>

EFI_STATUS EFIAPI LoaderMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    Print(L"hello uchos");

    EFI_STATUS status;

    // graphics output protocol
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void **)&gop);
    if (EFI_ERROR(status))
    {
        Print(L"failed to locate GOP: %r\n", status);
        return status;
    }

    EFI_LOADED_IMAGE_PROTOCOL *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_PROTOCOL *root_dir;

    status = gBS->OpenProtocol(image_handle, &gEfiLoadedImageProtocolGuid,
                               (void **)&loaded_image, image_handle, NULL,
                               EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(status))
    {
        Print(L"failed to open loaded image protocol: %r\n", status);
        return status;
    }

    // loaded_image->DeviceHandle はブートローダーが起動したデバイス(ブートローダーが読み込まれたディスク)のハンドル
    status = gBS->OpenProtocol(loaded_image->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid,
                               (void **)&fs, image_handle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(status))
    {
        Print(L"failed to open fs protocol: %r\n", status);
        return status;
    }

    status = fs->OpenVolume(fs, &root_dir);
    if (EFI_ERROR(status))
    {
        Print(L"failed to open root dir: %r\n", status);
        return status;
    }

    EFI_FILE_PROTOCOL *kernel_file;
    status = root_dir->Open(root_dir, &kernel_file, L"kernel.elf", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
    {
        Print(L"failed to open file 'kernel.elf': %r\n", status);
        return status;
    }

    EFI_FILE_INFO *kernel_file_info = NULL;
    UINTN kernel_file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    status = gBS->AllocatePool(EfiLoaderData, kernel_file_info_size, (void **)&kernel_file_info);
    if (EFI_ERROR(status))
    {
        Print(L"failed to allocate pool: %r\n", status);
        return status;
    }

    status = kernel_file->GetInfo(kernel_file, &gEfiFileInfoGuid, &kernel_file_info_size, kernel_file_info);
    if (EFI_ERROR(status))
    {
        Print(L"failed to get file info: %r\n", status);
        return status;
    }

    EFI_PHYSICAL_ADDRESS kernel_buffer = 0x100000;

    status = gBS->AllocatePages(AllocateAddress, EfiLoaderData,
                                (kernel_file_info->FileSize + 0xfff) / 0x1000, (EFI_PHYSICAL_ADDRESS *)&kernel_buffer);
    if (EFI_ERROR(status))
    {
        Print(L"failed to allocate pages: %r\n", status);
        return status;
    }

    status = kernel_file->Read(kernel_file, &kernel_file_info->FileSize, (VOID *)kernel_buffer);
    if (EFI_ERROR(status))
    {
        Print(L"failed to read kernel file: %r\n", status);
        return status;
    }

    UINT64 entry_addr = *(UINT64 *)(kernel_buffer + 24);

    typedef void __attribute__((sysv_abi)) KernelEntryPoint();
    ((KernelEntryPoint *)entry_addr)();

    return EFI_SUCCESS;
}