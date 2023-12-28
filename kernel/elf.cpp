#include "elf.hpp"
#include "graphics/terminal.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include <cstdint>

elf64_phdr_t* get_program_header(elf64_ehdr_t* elf_header)
{
	return reinterpret_cast<elf64_phdr_t*>(reinterpret_cast<uintptr_t>(elf_header) +
										   elf_header->e_phoff);
}

uintptr_t get_first_load_addr(elf64_ehdr_t* elf_header)
{
	auto* program_header = get_program_header(elf_header);
	for (int i = 0; i < elf_header->e_phnum; i++) {
		if (program_header[i].p_type == PT_LOAD) {
			return program_header[i].p_vaddr;
		}
	}

	return 0;
}

void copy_load_segment(elf64_ehdr_t* elf_header)
{
	auto* program_header = get_program_header(elf_header);
	for (int i = 0; i < elf_header->e_phnum; i++) {
		if (program_header[i].p_type != PT_LOAD) {
			continue;
		}

		linear_address dest_addr;
		dest_addr.data = program_header[i].p_vaddr;

		const auto num_pages =
				(program_header[i].p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;

		setup_page_tables(dest_addr, num_pages);

		auto* const src =
				reinterpret_cast<uint8_t*>(elf_header) + program_header[i].p_offset;
		auto* const dest = reinterpret_cast<uint8_t*>(program_header[i].p_vaddr);

		memcpy(dest, src, program_header[i].p_filesz);
		memset(dest + program_header[i].p_filesz, 0,
			   program_header[i].p_memsz - program_header[i].p_filesz);
	}
}

void load_elf(elf64_ehdr_t* elf_header)
{
	if (elf_header->e_type != ET_EXEC) {
		return;
	}

	const auto first_load_addr = get_first_load_addr(elf_header);
	if (first_load_addr < 0xffff'8000'0000'0000) {
		main_terminal->printf("invalid load address: %p\n", first_load_addr);
		return;
	}

	copy_load_segment(elf_header);
}
