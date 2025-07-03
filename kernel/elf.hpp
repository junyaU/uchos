/**
 * @file elf.hpp
 * @brief ELF (Executable and Linkable Format) structures and loader functions
 * 
 * This file defines the structures and functions for handling ELF64 format files,
 * which is the standard executable format for Unix-like systems on x86-64.
 * It includes definitions for ELF headers, program headers, dynamic sections,
 * and relocation entries, as well as functions for loading and executing ELF files.
 * 
 * @date 2024
 */

#pragma once

#include <cstdint>

/**
 * @brief ELF64 data type definitions
 * 
 * These typedefs define the standard data types used in the ELF64 format
 * according to the System V ABI specification.
 */
typedef uintptr_t elf64_addr_t;  ///< Unsigned program address
typedef uint64_t elf64_off_t;    ///< Unsigned file offset
typedef uint16_t elf64_half_t;   ///< Unsigned medium integer
typedef uint32_t elf64_word_t;   ///< Unsigned integer
typedef int32_t elf64_sword_t;   ///< Signed integer
typedef uint64_t elf64_xword_t;  ///< Unsigned long integer
typedef int64_t elf64_sxword_t;  ///< Signed long integer

/**
 * @brief Size of the ELF identification array (e_ident)
 */
#define EI_NIDENT 16

/**
 * @brief ELF64 file header structure
 * 
 * The ELF header is located at the beginning of every ELF file and
 * contains information used to parse the rest of the file.
 */
typedef struct {
	unsigned char e_ident[EI_NIDENT]; ///< ELF identification bytes (magic number, class, etc.)
	elf64_half_t e_type;              ///< Object file type (ET_EXEC, ET_DYN, etc.)
	elf64_half_t e_machine;           ///< Architecture (e.g., EM_X86_64)
	elf64_word_t e_version;           ///< Object file version
	elf64_addr_t e_entry;             ///< Entry point virtual address
	elf64_off_t e_phoff;              ///< Program header table file offset
	elf64_off_t e_shoff;              ///< Section header table file offset
	elf64_word_t e_flags;             ///< Processor-specific flags
	elf64_half_t e_ehsize;            ///< ELF header size in bytes
	elf64_half_t e_phentsize;         ///< Program header table entry size
	elf64_half_t e_phnum;             ///< Program header table entry count
	elf64_half_t e_shentsize;         ///< Section header table entry size
	elf64_half_t e_shnum;             ///< Section header table entry count
	elf64_half_t e_shstrndx;          ///< Section header string table index
} elf64_ehdr_t;

/**
 * @brief ELF file types (e_type values)
 */
#define ET_NONE 0  ///< No file type
#define ET_REL 1   ///< Relocatable file
#define ET_EXEC 2  ///< Executable file
#define ET_DYN 3   ///< Shared object file
#define ET_CORE 4  ///< Core file

/**
 * @brief ELF64 program header structure
 * 
 * Program headers describe segments of the file that should be mapped
 * into memory when the program is loaded.
 */
typedef struct {
	elf64_word_t p_type;    ///< Segment type (PT_LOAD, PT_DYNAMIC, etc.)
	elf64_word_t p_flags;   ///< Segment flags (PF_X, PF_W, PF_R)
	elf64_off_t p_offset;   ///< Segment file offset
	elf64_addr_t p_vaddr;   ///< Segment virtual address
	elf64_addr_t p_paddr;   ///< Segment physical address (usually ignored)
	elf64_xword_t p_filesz; ///< Segment size in file
	elf64_xword_t p_memsz;  ///< Segment size in memory
	elf64_xword_t p_align;  ///< Segment alignment
} elf64_phdr_t;

/**
 * @brief Program header segment types (p_type values)
 */
#define PT_NULL 0     ///< Unused entry
#define PT_LOAD 1     ///< Loadable segment
#define PT_DYNAMIC 2  ///< Dynamic linking information
#define PT_INTERP 3   ///< Interpreter path name
#define PT_NOTE 4     ///< Auxiliary information
#define PT_SHLIB 5    ///< Reserved
#define PT_PHDR 6     ///< Program header table
#define PT_TLS 7      ///< Thread-local storage segment

/**
 * @brief ELF64 dynamic section entry
 * 
 * Dynamic entries provide information needed for dynamic linking.
 */
typedef struct {
	elf64_sxword_t d_tag;   ///< Dynamic entry type
	union {
		elf64_xword_t d_val; ///< Integer value
		elf64_addr_t d_ptr;  ///< Program virtual address
	} d_un;
} elf64_dyn_t;

/**
 * @brief Dynamic section tags (d_tag values)
 */
#define DT_NULL 0     ///< End of dynamic section
#define DT_RELA 7     ///< Address of Rela relocation table
#define DT_RELASZ 8   ///< Size in bytes of Rela table
#define DT_RELAENT 9  ///< Size in bytes of a Rela table entry

/**
 * @brief ELF64 relocation entry with addend
 * 
 * Relocation entries describe how to modify the loaded program
 * to account for its actual load address.
 */
typedef struct {
	elf64_addr_t r_offset;   ///< Location to apply the relocation
	elf64_xword_t r_info;    ///< Symbol index and relocation type
	elf64_sxword_t r_addend; ///< Constant addend for the relocation
} elf64_rela_t;

/**
 * @brief Macros for manipulating relocation info field
 */
#define ELF64_R_SYM(i) ((i) >> 32)                              ///< Extract symbol index from r_info
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)                     ///< Extract relocation type from r_info
#define ELF64_R_INFO(s, t) (((s) << 32) + ((t) & 0xffffffffL))  ///< Construct r_info from symbol and type

/**
 * @brief x86-64 relocation types
 */
#define R_X86_64_RELATIVE 8  ///< Adjust by base address

/**
 * @brief Load an ELF file into memory
 * 
 * Parses the ELF header and program headers, then loads all PT_LOAD
 * segments into their specified memory locations.
 * 
 * @param elf_header Pointer to the ELF header in memory
 * 
 * @note This function assumes the ELF file is already loaded into memory
 */
void load_elf(elf64_ehdr_t* elf_header);

/**
 * @brief Execute an ELF file as a new process
 * 
 * Creates a new process from the ELF file in the buffer and begins
 * execution at the entry point specified in the ELF header.
 * 
 * @param buffer Pointer to the ELF file data in memory
 * @param name Name to assign to the new process
 * @param args Command line arguments to pass to the process
 * 
 * @note This function does not return if successful
 */
void exec_elf(void* buffer, const char* name, const char* args);

/**
 * @brief Get the virtual address of the first PT_LOAD segment
 * 
 * Scans the program headers and returns the virtual address of the
 * first loadable segment. This is typically used to determine the
 * base address of the loaded program.
 * 
 * @param elf_header Pointer to the ELF header
 * @return Virtual address of the first PT_LOAD segment, or 0 if none found
 */
uintptr_t get_first_load_addr(elf64_ehdr_t* elf_header);

/**
 * @brief Check if a file is a valid ELF file
 * 
 * Validates the ELF magic number and other basic header fields to
 * determine if the file is a valid ELF64 executable.
 * 
 * @param elf_header Pointer to potential ELF header
 * @return true if valid ELF file, false otherwise
 * 
 * @note This function checks for ELF64 format specifically
 */
bool is_elf(elf64_ehdr_t* elf_header);
