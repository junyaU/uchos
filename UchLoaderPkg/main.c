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

#include "elf.hpp"
#include "frame_buffer_conf.hpp"
#include "memory_map.hpp"

EFI_STATUS GetMemoryMap(struct MemoryMap* map)
{
	if (map->buffer == NULL) {
		return EFI_BUFFER_TOO_SMALL;
	}

	map->map_size = map->buffer_size;
	return gBS->GetMemoryMap(&map->map_size, (EFI_MEMORY_DESCRIPTOR*)map->buffer,
							 &map->map_key, &map->descriptor_size,
							 &map->descriptor_version);
}

void GetLoadSegmentAddresses(Elf64_Ehdr* ehdr, UINT64* first, UINT64* last)
{
	Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
	*first = MAX_UINT64;
	*last = 0;

	for (Elf64_Half i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_type != PT_LOAD) {
			continue;
		}

		*first = MIN(*first, phdr[i].p_vaddr);
		*last = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz);
	}
}

void CopyLoadSegments(Elf64_Ehdr* ehdr)
{
	Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64)ehdr + ehdr->e_phoff);
	for (Elf64_Half i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_type != PT_LOAD) {
			continue;
		}

		UINT64 segment_in_file = (UINT64)ehdr + phdr[i].p_offset;
		CopyMem((VOID*)phdr[i].p_vaddr, (VOID*)segment_in_file, phdr[i].p_filesz);

		UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
		SetMem((VOID*)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
	}
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** buffer)
{
	EFI_STATUS status;
	EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

	status = gBS->OpenProtocol(image_handle, &gEfiLoadedImageProtocolGuid,
							   (void**)&loaded_image, image_handle, NULL,
							   EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(status)) {
		Print(L"failed to open loaded image protocol: %r\n", status);
		return status;
	}

	status = gBS->OpenProtocol(loaded_image->DeviceHandle,
							   &gEfiSimpleFileSystemProtocolGuid, (void**)&fs,
							   image_handle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(status)) {
		Print(L"failed to open fs protocol: %r\n", status);
		return status;
	}

	return fs->OpenVolume(fs, buffer);
}

EFI_STATUS ReadFile(EFI_FILE_PROTOCOL* file, VOID** buffer)
{
	EFI_STATUS status;
	EFI_FILE_INFO* file_info = NULL;
	UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;

	status = file->GetInfo(file, &gEfiFileInfoGuid, &file_info_size, file_info);
	if (EFI_ERROR(status)) {
		Print(L"failed to get file info: %r\n", status);
		return status;
	}

	status = gBS->AllocatePool(EfiLoaderData, file_info->FileSize, buffer);
	if (EFI_ERROR(status)) {
		Print(L"failed to allocate pages: %r\n", status);
		return status;
	}

	return file->Read(file, &file_info->FileSize, *buffer);
}

EFI_STATUS LoadKernel(EFI_FILE_PROTOCOL* root_dir, UINT64* kernel_buf)
{
	EFI_STATUS status;
	EFI_FILE_PROTOCOL* kernel_file;

	status = root_dir->Open(root_dir, &kernel_file, L"kernel.elf",
							EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status)) {
		Print(L"failed to open file 'kernel.elf': %r\n", status);
		return status;
	}

	VOID* temp_kernel_buf;
	status = ReadFile(kernel_file, &temp_kernel_buf);
	if (EFI_ERROR(status)) {
		Print(L"failed to read kernel file: %r\n", status);
		return status;
	}

	Elf64_Ehdr* kernel_ehdr = (Elf64_Ehdr*)temp_kernel_buf;
	UINT64 kernel_last_addr;
	GetLoadSegmentAddresses(kernel_ehdr, kernel_buf, &kernel_last_addr);

	UINTN num_pages = (kernel_last_addr - *kernel_buf + 0xfff) / 0x1000;

	Print(L"num_pages: %d\n", num_pages);

	status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, num_pages,
								kernel_buf);
	if (EFI_ERROR(status)) {
		Print(L"failed to allocate pages: %r\n", status);
		return status;
	}

	CopyLoadSegments(kernel_ehdr);
	Print(L"Kernel: 0x%0lx - 0x%0lx\n", *kernel_buf, kernel_last_addr);

	return gBS->FreePool(temp_kernel_buf);
}

EFI_STATUS OpenBlockIoProtocolForLoadedImage(EFI_HANDLE image_handle,
											 EFI_BLOCK_IO_PROTOCOL** block_io)
{
	EFI_STATUS status;
	EFI_LOADED_IMAGE_PROTOCOL* loaded_image;

	// find device which stores the boot loader
	status = gBS->OpenProtocol(image_handle, &gEfiLoadedImageProtocolGuid,
							   (VOID**)&loaded_image, image_handle, NULL,
							   EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(status)) {
		return status;
	}

	return gBS->OpenProtocol(loaded_image->DeviceHandle, &gEfiBlockIoProtocolGuid,
							 (VOID**)block_io, image_handle, NULL,
							 EFI_OPEN_PROTOCOL_GET_PROTOCOL);
}

EFI_STATUS ReadBlocks(EFI_BLOCK_IO_PROTOCOL* block_io,
					  UINT32 media_id,
					  UINTN read_bytes,
					  VOID** buffer)
{
	EFI_STATUS status;

	status = gBS->AllocatePool(EfiLoaderData, read_bytes, buffer);
	if (EFI_ERROR(status)) {
		return status;
	}

	return block_io->ReadBlocks(block_io, media_id, 0, read_bytes, *buffer);
}

EFI_STATUS EFIAPI LoaderMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table)
{
	Print(L"hello uchos\n");

	EFI_STATUS status;

	EFI_FILE_PROTOCOL* root_dir;
	status = OpenRootDir(image_handle, &root_dir);
	if (EFI_ERROR(status)) {
		while (1)
			;
	}

	UINT64 kernel_buf;
	status = LoadKernel(root_dir, &kernel_buf);
	if (EFI_ERROR(status)) {
		Print(L"failed to load kernel: %r\n", status);
		while (1)
			;
	}

	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
	status =
			gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void**)&gop);
	if (EFI_ERROR(status)) {
		Print(L"failed to locate GOP: %r\n", status);
		while (1)
			;
	}

	struct FrameBufferConf conf = {
		(UINT8*)gop->Mode->FrameBufferBase,
		gop->Mode->Info->PixelsPerScanLine,
		gop->Mode->Info->HorizontalResolution,
		gop->Mode->Info->VerticalResolution,
		kPixelRGBResv8BitPerColor,
	};

	VOID* rsdp;
	for (UINTN i = 0; i < system_table->NumberOfTableEntries; i++) {
		if (CompareGuid(&gEfiAcpi20TableGuid,
						&system_table->ConfigurationTable[i].VendorGuid)) {
			rsdp = system_table->ConfigurationTable[i].VendorTable;
			break;
		}
	}

	VOID* volume_image;

	EFI_BLOCK_IO_PROTOCOL* block_io;
	status = OpenBlockIoProtocolForLoadedImage(image_handle, &block_io);
	if (EFI_ERROR(status)) {
		Print(L"failed to open block io protocol: %r\n", status);
		while (1)
			;
	}

	EFI_BLOCK_IO_MEDIA* media = block_io->Media;
	UINTN volume_size = (UINTN)media->BlockSize * (media->LastBlock + 1);
	if (volume_size > 16 * 1024 * 1024) {
		volume_size = 16 * 1024 * 1024;
	}

	status = ReadBlocks(block_io, media->MediaId, volume_size, &volume_image);
	if (EFI_ERROR(status)) {
		Print(L"failed to read blocks: %r\n", status);
		while (1)
			;
	}

	CHAR8 memory_map_buf[4096 * 4];
	struct MemoryMap memory_map = {
		sizeof(memory_map_buf), memory_map_buf, 0, 0, 0, 0
	};

	if (EFI_ERROR(GetMemoryMap(&memory_map))) {
		Print(L"fail to get memory map\n");
		while (1)
			;
	}

	status = gBS->ExitBootServices(image_handle, memory_map.map_key);
	if (EFI_ERROR(status)) {
		Print(L"could not exit boot service\n");
		while (1)
			;
	}

	UINT64 entry_addr = *(UINT64*)(kernel_buf + 24);

	typedef void __attribute__((sysv_abi))
	KernelEntryPoint(const struct FrameBufferConf* frame_buffer_conf,
					 const struct MemoryMap* memory_map, const VOID* rsdp,
					 VOID* volume_image);
	((KernelEntryPoint*)entry_addr)(&conf, &memory_map, rsdp, volume_image);

	return EFI_SUCCESS;
}