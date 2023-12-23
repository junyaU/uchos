#pragma once

#include <cstdint>

typedef uintptr_t elf64_addr_t;
typedef uint64_t elf64_off_t;
typedef uint16_t elf64_half_t;
typedef uint32_t elf64_word_t;
typedef int32_t elf64_sword_t;
typedef uint64_t elf64_xword_t;
typedef int64_t elf64_sxword_t;

#define EI_NIDENT 16

typedef struct {
	unsigned char e_ident[EI_NIDENT];
	elf64_half_t e_type;
	elf64_half_t e_machine;
	elf64_word_t e_version;
	elf64_addr_t e_entry;
	elf64_off_t e_phoff;
	elf64_off_t e_shoff;
	elf64_word_t e_flags;
	elf64_half_t e_ehsize;
	elf64_half_t e_phentsize;
	elf64_half_t e_phnum;
	elf64_half_t e_shentsize;
	elf64_half_t e_shnum;
	elf64_half_t e_shstrndx;
} elf64_ehdr_t;

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

typedef struct {
	elf64_word_t p_type;
	elf64_word_t p_flags;
	elf64_off_t p_offset;
	elf64_addr_t p_vaddr;
	elf64_addr_t p_paddr;
	elf64_xword_t p_filesz;
	elf64_xword_t p_memsz;
	elf64_xword_t p_align;
} elf64_phdr_t;

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6
#define PT_TLS 7

typedef struct {
	elf64_sxword_t d_tag;
	union {
		elf64_xword_t d_val;
		elf64_addr_t d_ptr;
	} d_un;
} elf64_dyn_t;

#define DT_NULL 0
#define DT_RELA 7
#define DT_RELASZ 8
#define DT_RELAENT 9

typedef struct {
	elf64_addr_t r_offset;
	elf64_xword_t r_info;
	elf64_sxword_t r_addend;
} elf64_rela_t;

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)
#define ELF64_R_INFO(s, t) (((s) << 32) + ((t) & 0xffffffffL))

#define R_X86_64_RELATIVE 8

void load_elf(elf64_ehdr_t* elf_header);
