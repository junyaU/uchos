#include "elf.hpp"
#include "asm_utils.h"
#include "graphics/terminal.hpp"
#include "memory/page.hpp"
#include "memory/paging.hpp"
#include "memory/segment.hpp"
#include "types.hpp"
#include <cstdint>
#include <cstring>

int make_args(char* command,
			  char* args,
			  char** argv,
			  int argv_len,
			  char* arg_buf,
			  int arg_buf_len)
{
	int argc = 0;
	int arg_buf_index = 0;

	auto push_to_argv = [&](const char* s) {
		if (argc >= argv_len || arg_buf_index >= arg_buf_len) {
			return;
		}

		argv[argc] = &arg_buf[arg_buf_index];
		++argc;
		strncpy(&arg_buf[arg_buf_index], s, arg_buf_len - arg_buf_index);
		arg_buf_index += strlen(s) + 1;
	};

	push_to_argv(command);

	if (args == nullptr) {
		return argc;
	}

	char* p = args;
	while (true) {
		while (*p == ' ') {
			++p;
		}

		if (*p == 0) {
			break;
		}

		const char* arg = p;
		while (*p != ' ' && *p != 0) {
			++p;
		}

		const bool is_end = *p == 0;
		*p = 0;
		push_to_argv(arg);

		if (is_end) {
			break;
		}

		++p;
	}

	return argc;
}

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

bool is_elf(elf64_ehdr_t* elf_header)
{
	return memcmp(elf_header->e_ident,
				  "\x7f"
				  "ELF",
				  4) == 0;
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
		printk(KERN_ERROR, "invalid load address: %p", first_load_addr);
		return;
	}

	copy_load_segment(elf_header);
}

void exec_elf(void* buffer, char* name, const char* args)
{
	auto* elf_header = reinterpret_cast<elf64_ehdr_t*>(buffer);
	if (!is_elf(elf_header)) {
		printk(KERN_ERROR, "not an ELF file: %s", name);
		return;
	}

	load_elf(elf_header);

	const linear_address argv_addr{ 0xffff'ffff'ffff'f000 };
	setup_page_tables(argv_addr, 1);

	auto* argv = reinterpret_cast<char**>(argv_addr.data);
	const int arg_v_len = 32;
	auto* arg_buf =
			reinterpret_cast<char*>(argv_addr.data + arg_v_len * sizeof(char**));
	const int arg_buf_len = PAGE_SIZE - arg_v_len * sizeof(char**);
	const int argc = make_args(name, const_cast<char*>(args), argv, arg_v_len,
							   arg_buf, arg_buf_len);

	const linear_address stack_addr{ 0xffff'ffff'ffff'f000 };
	setup_page_tables(stack_addr, 1);

	task* t = CURRENT_TASK;

	auto entry_addr = elf_header->e_entry;
	call_userland(argc, argv, USER_SS, entry_addr, stack_addr.data + PAGE_SIZE - 8,
				  &t->kernel_stack_top);
}